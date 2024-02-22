/***************************************************************************//**
* \file cy_app_vdm.c
* \version 1.0
*
* \brief
* Implements handlers for vendor defined messages (VDMs)
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "cy_app_config.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_app_vdm.h"
#include "cy_pdstack_common.h"
#include "cy_pdutils.h"
#include "cy_app.h"
#if CY_USE_CONFIG_TABLE
#include "cy_usbpd_config_table.h"
#endif /* CY_USE_CONFIG_TABLE */

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)

#include "cy_pdaltmode_vdm_task.h"
#include "cy_pdaltmode_mngr.h"
#include "cy_pdaltmode_defines.h"
#include "cy_pdaltmode_timer_id.h"
#include "cy_pdaltmode_hw.h"

#if ((TBT_DFP_SUPP) || (TBT_UFP_SUPP))
#include "cy_pdaltmode_intel_vid.h"
#endif /* ((TBT_DFP_SUPP) || (TBT_UFP_SUPP)) */

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
#include "cy_pdaltmode_intel_ridge_common.h"
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */

#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
#if GATKEX_CREEK
cy_pdstack_app_resp_cbk_t app_ridge_resp_callback = NULL;
#endif /* GATKEX_CREEK */
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */

/* Stores the VDM information from the config table into the RAM variables */
void Cy_App_Vdm_Init(cy_stc_pdstack_context_t * context, const cy_stc_app_params_t *appParams)
{
    uint16_t size = 0;
    cy_stc_pdstack_app_status_t* pdAppStatusPtr = Cy_App_GetPdAppStatus(context->port);

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
    cy_stc_pdaltmode_context_t * ptrAltModeContext = context->ptrAltModeContext;

#if CY_USE_CONFIG_TABLE
    /* Calculates the number of VDOs in the D_ID response */
    size = ptrAltModeContext->altModeCfg->disc_id_len;
#endif /* CY_USE_CONFIG_TABLE */
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */

#if !CY_USE_CONFIG_TABLE
    if (appParams->discIdResp != NULL)
    {
        size = appParams->discIdLen;
    }
#endif /* CY_USE_CONFIG_TABLE */

    /* Subtracts 4 bytes from the header and reduces number of VDOs */
    if (size > 4)
    {
        pdAppStatusPtr->vdmIdVdoCnt = (uint8_t)((size - 4) >> 2);
#if CY_USE_CONFIG_TABLE
        /* Updates the D_ID response pointer */
        pdAppStatusPtr->vdmIdVdoP = (cy_pd_pd_do_t *)((const uint8_t *)pd_get_ptr_disc_id(context->ptrUsbPdContext) + 4);
#else
        pdAppStatusPtr->vdmIdVdoP = (cy_pd_pd_do_t *)((const uint8_t *)appParams->discIdResp + 4);
#endif /* CY_USE_CONFIG_TABLE */

        /* Copies the actual discover identity response */
        memcpy ((uint8_t *)pdAppStatusPtr->vdmIdVdoResp, (const uint8_t *)pdAppStatusPtr->vdmIdVdoP, pdAppStatusPtr->vdmIdVdoCnt * 4u);
    }
    else
    {
        pdAppStatusPtr->vdmIdVdoCnt = 0;
    }

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
    /* Calculates the number of VDOs in the D_SVID response */
    size = ptrAltModeContext->altModeCfg->disc_svid_len;

    /* Subtract 4 bytes from the header and reduces the number of VDOs */
    if (size > 4)
    {
        pdAppStatusPtr->vdmSvidVdoCnt = (uint8_t)((size - 4) >> 2);
        /* Updates the D_SVID response pointer */
#if CY_USE_CONFIG_TABLE
        pdAppStatusPtr->vdmSvidVdoP = (cy_pd_pd_do_t *)((const uint8_t *)pd_get_ptr_disc_svid(context->ptrUsbPdContext) + 4);
#endif /* CY_USE_CONFIG_TABLE */
        /* Copies the actual discover identity response */
        memcpy ((uint8_t *)pdAppStatusPtr->vdmSvidVdoResp, (const uint8_t *)pdAppStatusPtr->vdmSvidVdoP, pdAppStatusPtr->vdmSvidVdoCnt * 4u);
    }
    else
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
    {
        pdAppStatusPtr->vdmSvidVdoCnt = 0;
    }

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
    /* Calculates the D_MODE response length */
    size = ptrAltModeContext->altModeCfg->disc_mode_len;

    if (size >= 12)
    {
        /* Stores the D_MODE response length from configuration table */
        pdAppStatusPtr->vdmModeDataLen = ptrAltModeContext->altModeCfg->disc_mode_len;
        /* Store pointer to the D_MODE responses. */
#if CY_USE_CONFIG_TABLE
        pdAppStatusPtr->vdmModeDataP = pd_get_ptr_disc_mode(context->ptrUsbPdContext);
#endif /* CY_USE_CONFIG_TABLE */

        /* Copies the actual discover identity response */
        memcpy ((uint8_t *)pdAppStatusPtr->vdmModeResp, (const uint8_t *)pdAppStatusPtr->vdmModeDataP, pdAppStatusPtr->vdmModeDataLen);
    }
    else
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
    {
        pdAppStatusPtr->vdmModeDataLen = 0;
    }

    //DOCK Related - move during app asset re-work
    /* Updates the vendor and product IDs from the configuration data */
    pdAppStatusPtr->vdmIdVdoResp[1].std_id_hdr.usbVid = context->ptrPortCfg->mfgVid;
    pdAppStatusPtr->vdmIdVdoResp[3].std_prod_vdo.usbPid = context->ptrPortCfg->mfgPid;
#if CY_USE_CONFIG_TABLE 
    (void)appParams;
#endif /* CY_USE_CONFIG_TABLE  */
}

void Cy_App_Vdm_EvalVdmMsg(cy_stc_pdstack_context_t * context, const cy_stc_pdstack_pd_packet_t *vdm, cy_pdstack_vdm_resp_cbk_t vdm_resp_handler)
{
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
    cy_stc_pdaltmode_context_t* ptrAltModeContext = context->ptrAltModeContext;;
#endif /*  ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

    cy_pd_pd_do_t* dobj = NULL;
    uint8_t i, count = 0u;
    bool pd3_live = false;
    cy_stc_pdstack_app_status_t* pdAppStatusPtr = Cy_App_GetPdAppStatus(context->port);

#if CY_PD_REV3_ENABLE
    if (context->dpmConfig.specRevSopLive >= CY_PD_REV3)
    {
        pd3_live = true;
    }
#endif /* CY_PD_REV3_ENABLE */

    /* Assume that the response is ready by default */
    pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_READY;

    if (
            (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.vdmType == CY_PDSTACK_VDM_TYPE_STRUCTURED) &&
            (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType == CY_PDSTACK_CMD_TYPE_INITIATOR)
       )
    {
        /* Copies received VDM header data to VDM response header */
        pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].val = vdm->dat[CY_PD_VDM_HEADER_IDX].val;
#if CY_PD_REV3_ENABLE
        /* Uses the minimum VDM version from the partner's revision and the live revision */
        pdAppStatusPtr->vdmVersion = CY_PDUTILS_GET_MIN (pdAppStatusPtr->vdmVersion,
                vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stVer);
#if MINOR_SVDM_VER_SUPPORT
        pdAppStatusPtr->vdmMinorVersion = CY_PDUTILS_GET_MIN (pdAppStatusPtr->vdmMinorVersion,
                vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stMinVer);
#endif /* MINOR_SVDM_VER_SUPPORT */

#endif /* CY_PD_REV3_ENABLE */

        /* Sets a NAK response by default */
        pdAppStatusPtr->vdmResp.doCount = 1u;
        pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = CY_PDSTACK_CMD_TYPE_RESP_NAK;

#if MUX_DELAY_EN
        /* Saves the pointer to VDM response handler and sets appropriate flag */
        pdAppStatusPtr->isVdmPending = true;
        pdAppStatusPtr->vdmRespCbk = vdm_resp_handler;
#endif /* MUX_DELAY_EN */


        if ((context->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP) || (pd3_live))
        {
            /* VDM commands (D_ID -- EXIT_MODE) should be NAKd if VDO count in VDM
             * command is more than one. */
            if (vdm->len == 1)
            {
                switch(vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd)
                {
                    case CY_PDSTACK_VDM_CMD_DSC_IDENTITY:
                        count = pdAppStatusPtr->vdmIdVdoCnt;
                        if((vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid == CY_PD_STD_SVID) && (count != 0))
                        {
                            pdAppStatusPtr->vdmResp.doCount = count;
                            dobj = pdAppStatusPtr->vdmIdVdoResp;
                            for(i = 0 ; i < count; i++)
                            {
                                pdAppStatusPtr->vdmResp.respBuf[i] = dobj[i];
                            }
#if CY_PD_REV3_ENABLE
                            /* Masks product type (DFP) and connector type fields when VDM is v1.0 */
                            if(pdAppStatusPtr->vdmVersion == 0)
                            {
                                cy_pd_pd_do_t id_hdr = pdAppStatusPtr->vdmResp.respBuf[CY_APP_VDM_VDO_START_IDX];
                                uint8_t max_do_cnt = 4;

                                /* Ensure to clear fields reserved under PD 2.0 */
                                id_hdr.std_id_hdr.prodTypeDfp = 0;
                                id_hdr.std_id_hdr.connType = 0;

                                /* Ensure not to use invalid product types */
                                if (id_hdr.std_id_hdr.prodType == (uint8_t)CY_PDSTACK_PROD_TYPE_PSD)
                                {
                                    id_hdr.std_id_hdr.prodType = (uint8_t)CY_PDSTACK_PROD_TYPE_UNDEF;
                                }

                                /* AMAs provides one extra VDO */
                                if (id_hdr.std_id_hdr.prodType == (uint8_t)CY_PDSTACK_PROD_TYPE_AMA)
                                {
                                    max_do_cnt++;
                                }

                                /* Ensures that the size of the response is valid based
                                 * on the ID header. */
                                if(pdAppStatusPtr->vdmResp.doCount > max_do_cnt)
                                {
                                    pdAppStatusPtr->vdmResp.doCount = max_do_cnt;
                                }

                                pdAppStatusPtr->vdmResp.respBuf[CY_APP_VDM_VDO_START_IDX] = id_hdr;
                            }
#else /* !CY_PD_REV3_ENABLE */
                            app_stat->vdmResp.respBuf[CY_APP_VDM_VDO_START_IDX].std_id_hdr.rsvd1 = 0;
#endif /* CY_PD_REV3_ENABLE */

                            /* Sets VDM response ACKed */
                            pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = CY_PDSTACK_CMD_TYPE_RESP_ACK;
                        }
                        break;

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
                    case CY_PDSTACK_VDM_CMD_DSC_SVIDS:
                        count = pdAppStatusPtr->vdmSvidVdoCnt;
                        if((vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid == CY_PD_STD_SVID) && (count != 0))
                        {
                            pdAppStatusPtr->vdmResp.doCount = count;
                            dobj = pdAppStatusPtr->vdmSvidVdoResp;
                            for(i = 0 ; i < count; i++)
                            {
                                pdAppStatusPtr->vdmResp.respBuf[i] = dobj[i];
                            }
                            /* Sets the VDM Response ACKed */
                            pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = CY_PDSTACK_CMD_TYPE_RESP_ACK;
                        }
                        break;

                    case CY_PDSTACK_VDM_CMD_DSC_MODES:
#if UFP_ALT_MODE_SUPP
                        bool eval_rslt;
                        eval_rslt = Cy_PdAltMode_Mngr_GetModesVdoInfo (ptrAltModeContext, vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid, &dobj, &count);
                        if (eval_rslt == true)
                        {
                            pdAppStatusPtr->vdmResp.doCount = count;
                            for (i = 0; i < count; i++)
                            {
                                pdAppStatusPtr->vdmResp.respBuf[i] = dobj[i];
                            }

#if ((TBT_UFP_SUPP) && (VPRO_WITH_USB4_MODE))
                            if (
                                    (ptrAltModeContext->tbtCfg->vpro_capable)
                               )
                            {
                                /* Sets the VPro available bit in the Mode VDO if enabled in config table */
                                if (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid == CY_PDALTMODE_TBT_INTEL_VID)
                                {
                                    /* Set the VPro enable bit in the Mode VDO */
                                    pdAppStatusPtr->vdmResp.respBuf[1].val |= CY_PDALTMODE_TBT_MODE_VPRO_AVAIL;
                                }
                            }
#endif /* ((TBT_UFP_SUPP) && (VPRO_WITH_USB4_MODE)) */

                            /* Sets the VDM response ACKed */
                            pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = CY_PDSTACK_CMD_TYPE_RESP_ACK;
                        }
#endif /* UFP_ALT_MODE_SUPP */
                        break;

                    case CY_PDSTACK_VDM_CMD_ENTER_MODE:
                        break;

                    case CY_PDSTACK_VDM_CMD_EXIT_MODE:
                        break;

                    case CY_PDSTACK_VDM_CMD_ATTENTION:
                        /* Ignores attention VDM */
                        pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_NOT_REQ;
                        break;
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */

                    default:
                        break;
                }
            }

            /* Under PD2, handle VDMs are based on VDM manager functionality */
            if (!pd3_live)
            {
                if (
                        (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid != /*STD_SVID*/ 0xFF00) &&
                        (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd != (uint8_t)CY_PDSTACK_VDM_CMD_ATTENTION)
                   )
                {
#if UFP_ALT_MODE_SUPP
                    if (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd > CY_PDSTACK_VDM_CMD_DSC_MODES)
                        pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = Cy_PdAltMode_Mngr_EvalRecVdm(ptrAltModeContext, vdm);
#endif /* UFP_ALT_MODE_SUPP */
                    {
                        /* No response needed */
                    }
                }
            }
#if CY_PD_REV3_ENABLE
            else
            {
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
                if(context->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
                {
                    if ((vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd == CY_PDSTACK_VDM_CMD_ENTER_MODE) ||
                            (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd == CY_PDSTACK_VDM_CMD_EXIT_MODE))
                    {
                        pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_READY;
                    }
                    else
                    {
                        if (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd == CY_PDSTACK_VDM_CMD_ATTENTION)
                        {
                            pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_NOT_REQ;
                        }
                        Cy_PdAltMode_Mngr_EvalRecVdm(ptrAltModeContext, vdm);
                    }
                }
                else
                {
                    if (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd > CY_PDSTACK_VDM_CMD_DSC_MODES)
                        pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = Cy_PdAltMode_Mngr_EvalRecVdm(ptrAltModeContext, vdm);
                }
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */
            }
#endif /* CY_PD_REV3_ENABLE */
        }
        else
        {
#if DFP_ALT_MODE_SUPP
            if (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd == CY_PDSTACK_VDM_CMD_ATTENTION) 
            {
                if(ptrAltModeContext->altModeAppStatus->vdmTaskEn == true)
                {
                    /* Evaluates attention VDM */
                    Cy_PdAltMode_Mngr_EvalRecVdm(ptrAltModeContext, vdm);
                }
                /* No response to VDMs received on PD 2.0 connection in the DFP state */
                pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_NOT_REQ;
            }
            else
#else
                if (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd != CY_PDSTACK_VDM_CMD_ATTENTION)
#endif /* DFP_ALT_MODE_SUPP */
                {
                    /*
                      In DFP; NAK structured VDM requests are:
                       a. Discover identity
                       b. Discover SVIDs
                       c. Discover modes
                       d. Enter or exit mode requests
                       e. Non-attention messages addressed to unknown SVIDs
                       */
                    if (
                            (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd <= CY_PDSTACK_VDM_CMD_EXIT_MODE) ||
                            (
#if DP_DFP_SUPP
                             ((vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid & 0xFFFEu) != 0xFF00)
#else
                             (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid != 0xFF00)
#endif /* DP_DFP_SUPP */
                             &&
#if TBT_DFP_SUPP
                             (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid != 0x8087)
#else
                             (1u)
#endif /* TBT_DFP_SUPP */
                            )
                       )
                    {
                        /* Sends a NAK response */
                    }
                    else
                    {
                        /* No response to VDMs received on PD 2.0 connection in DFP state */
                        pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_NOT_REQ;
                    }
                }
        }

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
        /* Sets the VDM version for the response */
        pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stVer = ptrAltModeContext->appStatusContext->vdmVersion;
#if MINOR_SVDM_VER_SUPPORT
        pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stMinVer = ptrAltModeContext->appStatusContext->vdmMinorVersion;
#endif /* MINOR_SVDM_VER_SUPPORT */
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
    }
    else
    {
#if UVDM_SUPP
        if (Cy_PdAltMode_Mngr_EvalRecVdm(port, vdm) == false)
        {
            /* If UVDM not successful then ignore this UVDM */
            pdAppStatusPtr->vdmResp.noResp = VDM_AMS_RESP_FROM_EC;
        }
#else /* !UVDM_SUPP */
        if (
                (!pd3_live)
           )
        {
            /* If UVDM not successful then ignore this UVDM */
            pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_FROM_EC;
        }
        else
        {
            /* PD 3.0 contract: Respond with NOT_Supported */
            pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_NOT_SUPP;
        }
#endif /* UVDM_SUPP */
    }

#if (MUX_DELAY_EN && (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP))
    /* Sends the VDM response only when MUX is idle or alt mode response is NACK. */
    if (
            (pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType != CY_PDSTACK_CMD_TYPE_RESP_ACK) ||
            (ptrAltModeContext->altModeAppStatus->isMuxBusy == false)
       )
    {
        vdm_resp_handler(ptrAltModeContext->pdStackContext, &pdAppStatusPtr->vdmResp);
        pdAppStatusPtr->isVdmPending = false;
    }
#else
    vdm_resp_handler(context, &pdAppStatusPtr->vdmResp);
#endif /* (MUX_DELAY_EN && (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)) */
}

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
#if GATKEX_CREEK
void ridge_mux_delay_for_usb4_cbk (cy_timer_id_t id, void * ptrContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t*) ptrContext;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = ptrPdStackContext->ptrAltModeContext;
#if UFP_ALT_MODE_SUPP
    uint8_t port = ptrPdStackContext->port;
#endif /* UFP_ALT_MODE_SUPP */

    ptrAltModeContext->altModeAppStatus->isMuxBusy = false;
    /* Stops the delay timer */
    Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GR_MUX_DELAY_TIMER);

#if ((CCG_BB_ENABLE != 0) && (UFP_ALT_MODE_SUPP != 0))
#if BILLBOARD_1_2_2_SUPPORT
        if(ptrAltModeContext->altModeAppStatus->usb4Supp)
        {
            Cy_PdAltMode_Mngr_BillboardUpdate(ptrAltModeContext, 0u, (Cy_App_GetRespBuffer(port)->reqStatus == CY_PDSTACK_REQ_ACCEPT ? true: false));
        }
#endif /* BILLBOARD_1_2_2_SUPPORT */
#endif /* ((CCG_BB_ENABLE != 0) && (UFP_ALT_MODE_SUPP != 0)) */

    if (app_ridge_resp_callback)
    {
        app_ridge_resp_callback(ptrPdStackContext, Cy_App_GetRespBuffer(ptrPdStackContext->port));
    }
}       
#endif 

/*
 * Follow "Valid responses to Enter_USB request" table for PD spec addendum.
 * Assumes that the UFP VDO1 is always present as the first product type VDO in the
 * Discover_Identity response, for feature matching purposes.
 */
void Cy_App_Vdm_EvalEnterUsb(cy_stc_pdstack_context_t * context, const cy_stc_pdstack_pd_packet_t *eudo_p, cy_pdstack_app_resp_cbk_t app_resp_handler)
{
    uint8_t port = context->port;
    cy_stc_pdaltmode_context_t * ptrAltModeContext = context->ptrAltModeContext;
    cy_stc_pdstack_app_status_t* pdAppStatusPtr = Cy_App_GetPdAppStatus(context->port);

    /* Basic validity checks by the PD stack includes message length */
    cy_pd_pd_do_t eudo    = eudo_p->dat[0];
    cy_pd_pd_do_t ufp_vdo = pdAppStatusPtr->vdmIdVdoResp[CY_PD_PRODUCT_TYPE_VDO_1_IDX];

#if GATKEX_CREEK
    ptrAltModeContext->altModeAppStatus->isMuxBusy = false;

    if(port == TYPEC_PORT_0_IDX )
    {
        /* Sets the ridge to disconnect between USB and TBT states when device is UFP. */
        ptrAltModeContext->altModeAppStatus->isMuxBusy = true;
        Cy_PdAltMode_Ridge_SetDisconnect(context->ptrAltModeContext);
        Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_PDALTMODE_GET_TIMER_ID(context, CY_PDALTMODE_GR_MUX_DELAY_TIMER),
                CY_PDALTMODE_RIDGE_GR_MUX_VDM_DELAY_TIMER_PERIOD, ridge_mux_delay_for_usb4_cbk);
    }
#endif

    /* Default response: REJECT */
    Cy_App_GetRespBuffer(port)->reqStatus = CY_PDSTACK_REQ_REJECT;

    /* Matches the configuration from the Enter USB DO against the UFP VDO1.*/
    if (pdAppStatusPtr->vdmIdVdoCnt > CY_PD_PRODUCT_TYPE_VDO_1_IDX)
    {
        if (
                (eudo.enterusb_vdo.usbMode == CY_PD_USB_MODE_USB4)       &&
                (ufp_vdo.ufp_vdo_1.devCap & CY_PDSTACK_DEV_CAP_USB_4_0)  &&
                (eudo.enterusb_vdo.cableSpeed >= CY_PDSTACK_USB_GEN_1)
           )
        {
#if (STORE_DETAILS_OF_HOST)
            if(port == TYPEC_PORT_0_IDX)
            {
                ptrAltModeContext->hostDetails->hostEudo.val = eudo_p->dat[0].val;
            }
#endif /* STORE_DETAILS_OF_HOST */

            Cy_App_GetRespBuffer(port)->reqStatus = CY_PDSTACK_REQ_ACCEPT;
        }

        else if (
                    (eudo.enterusb_vdo.usbMode == CY_PD_USB_MODE_USB3)        &&
                    (ufp_vdo.ufp_vdo_1.devCap & CY_PDSTACK_DEV_CAP_USB_3_2)  &&
                    (eudo.enterusb_vdo.cableSpeed >= CY_PDSTACK_USB_GEN_1)
                )
        {
            Cy_App_GetRespBuffer(port)->reqStatus = CY_PDSTACK_REQ_ACCEPT;
        }
        else if (
                    (eudo.enterusb_vdo.usbMode == CY_PD_USB_MODE_USB2) &&
                    (ufp_vdo.ufp_vdo_1.devCap & CY_PDSTACK_DEV_CAP_USB_2_0)
                )
        {
            Cy_App_GetRespBuffer(port)->reqStatus = CY_PDSTACK_REQ_ACCEPT;
        }
    }

    ptrAltModeContext->altModeAppStatus->eudo.val = eudo_p->dat[0].val;

    (void) ufp_vdo;

#if GATKEX_CREEK
    if(ptrAltModeContext->altModeAppStatus->isMuxBusy != false)
    {
        app_ridge_resp_callback = app_resp_handler;
    }
    else
#endif /* GATKEX_CREEK */
    {
#if (CCG_BB_ENABLE != 0)
#if (BILLBOARD_1_2_2_SUPPORT && UFP_ALT_MODE_SUPP)
        if(ptrAltModeContext->altModeAppStatus->usb4Supp)
        {
            Cy_PdAltMode_Mngr_BillboardUpdate(ptrAltModeContext, 0u, (Cy_App_GetRespBuffer(port)->reqStatus == CY_PDSTACK_REQ_ACCEPT ? true: false));
        }
#endif /* (BILLBOARD_1_2_2_SUPPORT && UFP_ALT_MODE_SUPP) */
#endif /* (CCG_BB_ENABLE != 0) */

        app_resp_handler(context, Cy_App_GetRespBuffer(port));
    }
}
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */

/* [] END OF FILE */
