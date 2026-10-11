/***************************************************************************//**
* \file cy_app_vdm.c
* \version 3.0
*
* \brief
* Implements handlers for vendor defined messages (VDMs)
*
********************************************************************************
* \copyright
* Copyright(c) 2024-2026, Infineon Technologies AG, or an affiliate of Infineon
* Technologies AG. All rights reserved.
* This software, associated documentation and materials ("Software") is owned
* by Infineon Technologies AG or one of its affiliates ("Infineon") and is
* protected by and subject to worldwide patent protection, worldwide copyright
* laws, and international treaty provisions. Therefore, you may use this
* Software only as provided in the license agreement accompanying the software
* package from which you obtained this Software. If no license agreement
* applies, then any use, reproduction, modification, translation, or
* compilation of this Software is prohibited without the express written
* permission of Infineon.
* Disclaimer: UNLESS OTHERWISE EXPRESSLY AGREED WITH INFINEON, THIS SOFTWARE IS 
* PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING, 
* BUT NOT LIMITED TO, ALL WARRANTIES OF NON-INFRINGEMENT OF THIRD-PARTY RIGHTS 
* AND IMPLIED WARRANTIES SUCH AS WARRANTIES OF FITNESS FOR A SPECIFIC 
* USE/PURPOSE OR MERCHANTABILITY. Infineon reserves the right to make changes
* to the Software without notice. You are responsible for properly designing,
* programming, and testing the functionality and safety of your intended 
* application of the Software, as well as complying with any legal requirements
* related to its use. Infineon does not guarantee that the Software will be 
* free from intrusion, data theft or loss, or other breaches ("Security 
* Breaches"), and Infineon shall have no liability arising out of any Security
* Breaches. Unless otherwise explicitly approved by Infineon, the Software may
* not be used in any application where a failure of the Product or any 
* consequences of the use thereof can reasonably be expected to result in
* personal injury.
*******************************************************************************/

#include "cy_app_config.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_app_vdm.h"
#include "cy_pdstack_common.h"
#include "cy_pdutils.h"
#include "cy_app.h"
#include "cy_app_internal.h"
#if CY_USE_CONFIG_TABLE
#include "cy_usbpd_config_table.h"
#endif /* CY_USE_CONFIG_TABLE */

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
#include "cy_pdaltmode_mngr.h"
#include "cy_pdaltmode_defines.h"
#include "cy_pdaltmode_timer_id.h"
#include "cy_pdaltmode_hw.h"
#include "cy_pdaltmode_billboard.h"

#if ((TBT_DFP_SUPP) || (TBT_UFP_SUPP))
#include "cy_pdaltmode_intel_vid.h"
#endif /* ((TBT_DFP_SUPP) || (TBT_UFP_SUPP)) */

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
#include "cy_pdaltmode_intel_ridge_common.h"
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */

#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */

#if ((DP_UFP_SUPP) && (DP_2_1_ENABLE))
#include "cy_pdaltmode_dp_sid.h"
#endif /* ((DP_UFP_SUPP) && (DP_2_1_ENABLE)) */

#if ((DP_UFP_SUPP) && (DP_2_1_ENABLE))
#include "cy_pdaltmode_dp_sid.h"
#endif /* ((DP_UFP_SUPP) && (DP_2_1_ENABLE)) */

/* UFP VDO mask to disable USB3 & USB4 capabilities and alternate modes, supporting USB 2.0 speed. */
#define UFP_VDO_DIS_USB3_ALT_MODES_MASK                 (0xF3FFFFC0u)
/* UFP VDO mask to disable USB4 capability and TBT mode, supporting USB 3.2 Gen1. */
#define UFP_VDO_DIS_USB4_TBT_MODES_MASK                 (0xF7FFFFD1u)
/* ID Header VDO mask to clear Modal operation supported bit. */
#define ID_HEADER_VDO_DISABLE_MODAL_OPERATION           (0xFBFFFFFFu)

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
#if GATKEX_CREEK
static cy_pdstack_app_resp_cbk_t app_ridge_resp_callback = NULL;
#endif /* GATKEX_CREEK */
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */

#if (!CY_PD_VDM_DISABLE)
/* Stores the VDM information from the config table into the RAM variables */
void Cy_App_Vdm_Init(cy_stc_pdstack_context_t * context, const cy_stc_app_params_t *appParams)
{
    uint16_t size = 0u;
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
    if (size > 4u)
    {
        pdAppStatusPtr->vdmIdVdoCnt = (uint8_t)((size - 4u) >> 2u);
#if CY_USE_CONFIG_TABLE
        /* Updates the D_ID response pointer */
        CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.3', 'Intentional pointer type conversion.');            
        pdAppStatusPtr->vdmIdVdoP = (cy_pd_pd_do_t *)((const uint8_t *)pd_get_ptr_disc_id(context->ptrUsbPdContext) + 4u);
#else
        CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.3', 'Intentional pointer type conversion.');            
        pdAppStatusPtr->vdmIdVdoP = (cy_pd_pd_do_t *)((const uint8_t *)appParams->discIdResp);
#endif /* CY_USE_CONFIG_TABLE */

        /* Copies the actual discover identity response */
        (void)memcpy ((uint8_t *)pdAppStatusPtr->vdmIdVdoResp, (const uint8_t *)pdAppStatusPtr->vdmIdVdoP, (uint32_t)(pdAppStatusPtr->vdmIdVdoCnt) * 4u);
    }
    else
    {
        pdAppStatusPtr->vdmIdVdoCnt = 0u;
    }

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
    /* Calculates the number of VDOs in the D_SVID response */
    size = ptrAltModeContext->altModeCfg->disc_svid_len;

    /* Subtract 4 bytes from the header and reduces the number of VDOs */
    if (size > 4u)
    {
        pdAppStatusPtr->vdmSvidVdoCnt = (uint8_t)((size - 4u) >> 2u);
        /* Updates the D_SVID response pointer */
#if CY_USE_CONFIG_TABLE
        CY_MISRA_DEVIATE_LINE('MISRA C-2012 Rule 11.3', 'Intentional pointer type conversion.');            
        pdAppStatusPtr->vdmSvidVdoP = (cy_pd_pd_do_t *)((const uint8_t *)pd_get_ptr_disc_svid(context->ptrUsbPdContext) + 4);
#endif /* CY_USE_CONFIG_TABLE */
        /* Copies the actual discover identity response */
        (void)memcpy ((uint8_t *)pdAppStatusPtr->vdmSvidVdoResp, (const uint8_t *)pdAppStatusPtr->vdmSvidVdoP, (uint32_t)(pdAppStatusPtr->vdmSvidVdoCnt) * 4u);
    }
    else
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
    {
        pdAppStatusPtr->vdmSvidVdoCnt = 0u;
    }

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
    /* Calculates the D_MODE response length */
    size = ptrAltModeContext->altModeCfg->disc_mode_len;

    if (size >= 12u)
    {
        /* Stores the D_MODE response length from configuration table */
        pdAppStatusPtr->vdmModeDataLen = (uint8_t)ptrAltModeContext->altModeCfg->disc_mode_len;
        /* Store pointer to the D_MODE responses. */
#if CY_USE_CONFIG_TABLE
        pdAppStatusPtr->vdmModeDataP = pd_get_ptr_disc_mode(context->ptrUsbPdContext);
#endif /* CY_USE_CONFIG_TABLE */

        /* Copies the actual discover identity response */
        (void)memcpy ((uint8_t *)pdAppStatusPtr->vdmModeResp, (const uint8_t *)pdAppStatusPtr->vdmModeDataP, pdAppStatusPtr->vdmModeDataLen);
    }
    else
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
    {
        pdAppStatusPtr->vdmModeDataLen = 0u;
    }

    //DOCK Related - move during app asset re-work
    /* Updates the vendor and product IDs from the configuration data */
    pdAppStatusPtr->vdmIdVdoResp[1].std_id_hdr.usbVid = context->ptrPortCfg->mfgVid;
    pdAppStatusPtr->vdmIdVdoResp[3].std_prod_vdo.usbPid = context->ptrPortCfg->mfgPid;
#if CY_USE_CONFIG_TABLE 
    CY_UNUSED_PARAMETER(appParams);
#endif /* CY_USE_CONFIG_TABLE  */
}
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
/**
 * @brief Checks for USB4 Gen4 or higher Support for UFP port to send
 * discovery identify response.
 *
 * @param ptrAltModeContext Pointer to the Altmode context
 * @return true/false based on USB4 Gen4 Speed capability support
 */
static bool Cy_App_Vdm_IsUfpUSB4Gen4Supp(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    /* Discover ID Response count including header. */
    uint8_t count = ptrAltModeContext->appStatusContext->vdmResp.doCount;
    /* Pointer to Discover ID Response. */
    cy_pd_pd_do_t* vdmDiscIdResp = ptrAltModeContext->appStatusContext->vdmResp.respBuf;
    bool ufpUsb4Gen4Supp = false;

    /* Check whether UFP is USB4 Gen4 capable. */
    /* 1. UFP VDO is present (minimum 5 DWs shall be present).
     * 2. Product should be HUB or peripheral.
     * 3. UFP VDO is non-zero.
     * 4. UFP is USB4 Capable.
     * 5. UFP supports USB4 Gen4 speed.
     *  */
    if (
            (count > CY_PD_PRODUCT_TYPE_VDO_1_IDX) &&
           ((vdmDiscIdResp[CY_PD_ID_HEADER_IDX].std_id_hdr.prodType == (uint32_t)CY_PDSTACK_PROD_TYPE_PERI) ||
            (vdmDiscIdResp[CY_PD_ID_HEADER_IDX].std_id_hdr.prodType == (uint32_t)CY_PDSTACK_PROD_TYPE_HUB)) &&
            (vdmDiscIdResp[CY_PD_PRODUCT_TYPE_VDO_1_IDX].val != CY_PDALTMODE_MNGR_NO_DATA)                       &&
           ((vdmDiscIdResp[CY_PD_PRODUCT_TYPE_VDO_1_IDX].ufp_vdo_1.devCap & (uint32_t)CY_PDSTACK_DEV_CAP_USB_4_0) != 0x0UL) &&
           (vdmDiscIdResp[CY_PD_PRODUCT_TYPE_VDO_1_IDX].ufp_vdo_1.usbSig >= (uint32_t)CY_PDSTACK_USB_GEN_4_SUPP)
       )
    {
        ufpUsb4Gen4Supp = true;
    }

    return ufpUsb4Gen4Supp;
}
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

#if UVDM_SUPP
__attribute__ ((weak)) bool uvdm_cmd_handler (cy_stc_pdstack_context_t * context, uint32_t *rx_pkt, cy_pd_pd_do_t **vdm_rspn_pkt,
    uint8_t *vdo_count)
{
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(rx_pkt);
    CY_UNUSED_PARAMETER(vdm_rspn_pkt);
    CY_UNUSED_PARAMETER(vdo_count);

    return false;
}
#endif /* UVDM_SUPP */

#if (UFP_ALT_MODE_SUPP && BUMBLEBEE_DOCK)
/* Checks for SVID support in current power mode for Bus-powered solutions.
 * The current solution supports only INTEL and DP SVIDs, hence the implementation is limited to these SVIDs only. */
static bool Cy_App_Vdm_IsSvidSupported(uint16_t svid, cy_en_app_alt_modes_dis_state_t altModesDisState)
{
    bool isSvidSupported = true;

    /* Check for the alternate mode requested and
     * Check for alternate modes disabled due to insufficient PDP value.
     *
     * Note: This logic needs to be updated based if new states are added to cy_en_app_alt_modes_dis_state_t.
     */
    if (((CY_PDALTMODE_TBT_SVID == svid) && (CY_APP_ALT_MODES_DIS_NONE != altModesDisState)) ||
        ((CY_PDALTMODE_DP_SVID == svid) && (CY_APP_ALT_MODES_DIS_ALL_ALT_MODES == altModesDisState)))
    {
        isSvidSupported = false;
    }

    return isSvidSupported;
}
#endif /* (UFP_ALT_MODE_SUPP && BUMBLEBEE_DOCK) */

void Cy_App_Vdm_EvalVdmMsg(cy_stc_pdstack_context_t * context, const cy_stc_pdstack_pd_packet_t *vdm, cy_pdstack_vdm_resp_cbk_t vdm_resp_handler)
{
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
    cy_stc_pdaltmode_context_t* ptrAltModeContext = context->ptrAltModeContext;
#if BUMBLEBEE_DOCK
    bool isSvidAbsent = true;
#endif /* BUMBLEBEE_DOCK */
#endif /*  ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

#if BUMBLEBEE_DOCK
    cy_stc_app_status_t *appStatusPtr = Cy_App_GetStatus(context->port);
#endif /* BUMBLEBEE_DOCK */

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
            (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.vdmType == (uint32_t)CY_PDSTACK_VDM_TYPE_STRUCTURED) &&
            (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType == (uint32_t)CY_PDSTACK_CMD_TYPE_INITIATOR)
       )
    {

#if (defined(CY_DEVICE_SERIES_PMG1B1) || defined(CY_DEVICE_SERIES_PMG1B2))
        /* Do nothing */
#else
#if (!((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)))
        /* In PD 2.0, a DFP or UFP that does not support Structured VDMs shall ignore any Structured VDMs received. */
#if CY_PD_REV3_ENABLE
        if (pd3_live != true)
#endif /* CY_PD_REV3_ENABLE */
            pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_NOT_REQ;
#endif /* (!((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))) */
#endif /* (defined(CY_DEVICE_SERIES_PMG1B1) || defined(CY_DEVICE_SERIES_PMG1B2)) */

        if (pdAppStatusPtr->vdmResp.noResp != CY_PDSTACK_VDM_AMS_RESP_NOT_REQ)
        {
            /* Copies received VDM header data to VDM response header */
            pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].val = vdm->dat[CY_PD_VDM_HEADER_IDX].val;
#if CY_PD_REV3_ENABLE
            /* Uses the minimum VDM version from the partner's revision and the live revision */
            pdAppStatusPtr->vdmVersion = (uint8_t)CY_PDUTILS_GET_MIN(pdAppStatusPtr->vdmVersion,
                    vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stVer);
#if MINOR_SVDM_VER_SUPPORT
            pdAppStatusPtr->vdmMinorVersion = (uint8_t)CY_PDUTILS_GET_MIN(pdAppStatusPtr->vdmMinorVersion,
                    vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stMinVer);
#endif /* MINOR_SVDM_VER_SUPPORT */

#endif /* CY_PD_REV3_ENABLE */

            /* Sets a NAK response by default */
            pdAppStatusPtr->vdmResp.doCount = 1u;
            pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = (uint32_t)CY_PDSTACK_CMD_TYPE_RESP_NAK;

#if MUX_DELAY_EN
            /* Saves the pointer to VDM response handler and sets appropriate flag */
            pdAppStatusPtr->isVdmPending = true;
            pdAppStatusPtr->vdmRespCbk = vdm_resp_handler;
#endif /* MUX_DELAY_EN */


            if ((context->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP) || (pd3_live))
            {
                /* VDM commands (D_ID -- EXIT_MODE) should be NAKd if VDO count in VDM
                 * command is more than one. */
                if (vdm->len == 1u)
                {
                    switch((cy_en_pdstack_std_vdm_cmd_t)vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd)
                    {
                        case CY_PDSTACK_VDM_CMD_DSC_IDENTITY:
                            count = pdAppStatusPtr->vdmIdVdoCnt;
                            if((vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid == CY_PD_STD_SVID) && (count != 0u))
                            {
                                pdAppStatusPtr->vdmResp.doCount = count;
                                dobj = pdAppStatusPtr->vdmIdVdoResp;
                                for(i = 0u ; i < count; i++)
                                {
                                    pdAppStatusPtr->vdmResp.respBuf[i] = dobj[i];
#if BUMBLEBEE_DOCK
                                    /**
                                     * Update the Discover-ID response based on the power contract.
                                     * USB4, USB3, TBT and DP may not be supported at lower powers in Bumblebee dock.
                                     */
                                    switch (i)
                                    {
                                        case CY_PD_PRODUCT_TYPE_VDO_1_IDX:
                                        {
                                            switch (appStatusPtr->alt_modes_dis_state)
                                            {
                                                case CY_APP_ALT_MODES_DIS_ALL_ALT_MODES:
                                                {
                                                    /* Disable USB3, USB4 and alternate modes and set USB Highest Speed to USB 2.0 only. */
                                                    pdAppStatusPtr->vdmResp.respBuf[i].val &= UFP_VDO_DIS_USB3_ALT_MODES_MASK;
                                                    break;
                                                }
                                                case CY_APP_ALT_MODES_DIS_USB4_TBT_MODE:
                                                {
                                                    /* Disable USB4 and TBT mode and set USB Highest Speed to USB 3.2 Gen1. */
                                                    pdAppStatusPtr->vdmResp.respBuf[i].val &= UFP_VDO_DIS_USB4_TBT_MODES_MASK;
                                                    break;
                                                }
                                                default:
                                                {
                                                    /* No handling needed in other cases */
                                                    break;
                                                }
                                            }
                                            break;
                                        }
                                        case CY_PD_ID_HEADER_IDX:
                                        {
                                            /**
                                             * If USB4 and TBT modes are disabled then clear the Model
                                             * Operation Supported bit in Discover ID response.
                                             */
                                            if (CY_APP_ALT_MODES_DIS_ALL_ALT_MODES == appStatusPtr->alt_modes_dis_state)
                                            {
                                                /* Clearing Model Operation Supported bit. */
                                                pdAppStatusPtr->vdmResp.respBuf[i].val &= ID_HEADER_VDO_DISABLE_MODAL_OPERATION;
                                            }
                                            break;
                                        }
                                        default:
                                        {
                                            /* No handling needed in other cases */
                                            break;
                                        }
                                    }
#endif /* BUMBLEBEE_DOCK */
                                }
#if CY_PD_REV3_ENABLE
                                /* Masks product type (DFP) and connector type fields when VDM is v1.0 */
                                if(pdAppStatusPtr->vdmVersion == 0u)
                                {
                                    cy_pd_pd_do_t id_hdr = pdAppStatusPtr->vdmResp.respBuf[CY_APP_VDM_VDO_START_IDX];
                                    uint8_t max_do_cnt = 4;

                                    /* Ensure to clear fields reserved under PD 2.0 */
                                    id_hdr.std_id_hdr.prodTypeDfp = 0u;
                                    id_hdr.std_id_hdr.connType = 0u;

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
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
                                else if (((false == pd3_live) ||
                                            (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stVer < (uint32_t)CY_PDSTACK_STD_VDM_VER2) ||
                                            (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stMinVer < (uint32_t)CY_PDSTACK_STD_VDM_MINOR_VER1)) &&
                                        (true == Cy_App_Vdm_IsUfpUSB4Gen4Supp(ptrAltModeContext)))
                                {
                                    pdAppStatusPtr->vdmResp.respBuf[CY_PD_PRODUCT_TYPE_VDO_1_IDX].ufp_vdo_1.usbSig = (uint32_t)CY_PDSTACK_USB_GEN_3_SUPP;
                                }
                                else
                                {
                                    /* No action needed */
                                }
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */
#else /* !CY_PD_REV3_ENABLE */
                                pdAppStatusPtr->vdmResp.respBuf[CY_APP_VDM_VDO_START_IDX].std_id_hdr.rsvd1 = 0u;
#endif /* CY_PD_REV3_ENABLE */

                                /* Sets VDM response ACKed */
                                pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = (uint32_t)CY_PDSTACK_CMD_TYPE_RESP_ACK;
                            }
                            break;

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
                        case CY_PDSTACK_VDM_CMD_DSC_SVIDS:
#if BUMBLEBEE_DOCK
                            /**
                             * If TBT and DP modes are disabled then NAK the Discover-SVID
                             * response.
                             */
                            if (CY_APP_ALT_MODES_DIS_ALL_ALT_MODES == appStatusPtr->alt_modes_dis_state)
                            {
                                count = 0u;
                            }
                            else
#endif /* BUMBLEBEE_DOCK */
                            {
                                count = pdAppStatusPtr->vdmSvidVdoCnt;
                            }
                            if((vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid == CY_PD_STD_SVID) && (count != 0u))
                            {
                                pdAppStatusPtr->vdmResp.doCount = count;
                                dobj = pdAppStatusPtr->vdmSvidVdoResp;
                                for(i = 0u ; i < count; i++)
                                {
                                    pdAppStatusPtr->vdmResp.respBuf[i] = dobj[i];
#if BUMBLEBEE_DOCK
                                    /* Check for TBT mode disabled state. */
                                    if ((0u != i) && (CY_APP_ALT_MODES_DIS_USB4_TBT_MODE == appStatusPtr->alt_modes_dis_state))
                                    {
                                        /* Check for TBT VDO and remove from SVID list. */
                                        if (CY_PDALTMODE_TBT_SVID == dobj[i].std_svid_res.svidN1)
                                        {
                                            pdAppStatusPtr->vdmResp.respBuf[i].std_svid_res.svidN1 = CY_PDALTMODE_MNGR_EMPTY_VDO;
                                        }
                                        else if (CY_PDALTMODE_TBT_SVID == dobj[i].std_svid_res.svidN)
                                        {
                                            pdAppStatusPtr->vdmResp.respBuf[i].std_svid_res.svidN = CY_PDALTMODE_MNGR_EMPTY_VDO;
                                        }
                                        else
                                        {
                                            /* No action needed */
                                        }
                                    }
#endif /* BUMBLEBEE_DOCK */
                                }
                            }
#if BUMBLEBEE_DOCK
                            /* Check at least on SVID is present.  */
                            for (i = (CY_PD_VDM_HEADER_IDX + 1u); i < count; i++)
                            {
                                if (0u != pdAppStatusPtr->vdmResp.respBuf[i].val)
                                {
                                    isSvidAbsent = false;
                                    break;
                                }
                            }
#endif /* BUMBLEBEE_DOCK */
                            CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 14.3', 1, 'False positive, possibly triggered by complex conditionals.');
                            /*
                             * Check for SVID absent or count is zero.
                             * USBPD 3.2 Ver 1.0, expects NAK when SVID count is zero.
                             */
                            if (
#if BUMBLEBEE_DOCK
                                isSvidAbsent ||
#endif /* BUMBLEBEE_DOCK */
                                (count == 0u)
                            )
                            {
                                pdAppStatusPtr->vdmResp.doCount = 1u;
                                pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = (uint32_t)CY_PDSTACK_CMD_TYPE_RESP_NAK;
                            }
                            else
                            {
                                /* Sets the VDM Response ACKed */
                                pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = (uint32_t)CY_PDSTACK_CMD_TYPE_RESP_ACK;
                            }
                            CY_MISRA_BLOCK_END('MISRA C-2012 Rule 14.3');
                            break;
                        case CY_PDSTACK_VDM_CMD_DSC_MODES:
#if UFP_ALT_MODE_SUPP
#if BUMBLEBEE_DOCK
                            /* Check whether SVID is supported, otherwise NAK response will be sent. */
                            if (Cy_App_Vdm_IsSvidSupported((uint16_t)(vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid), appStatusPtr->alt_modes_dis_state))
#endif /* BUMBLEBEE_DOCK */
                            {
                                bool eval_rslt;
                                eval_rslt = Cy_PdAltMode_Mngr_GetModesVdoInfo (ptrAltModeContext, (uint16_t)(vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid), &dobj, &count);
                                if (eval_rslt == true)
                                {
                                    pdAppStatusPtr->vdmResp.doCount = count;
                                    for (i = 0u; i < count; i++)
                                    {
                                        pdAppStatusPtr->vdmResp.respBuf[i] = dobj[i];
                                    }

#if ((TBT_UFP_SUPP) && (VPRO_WITH_USB4_MODE))
                                    if (
                                            (ptrAltModeContext->tbtCfg->vpro_capable != 0u)
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

#if ((DP_UFP_SUPP) && (DP_2_1_ENABLE))
                                    /* Check for DP2.1 is supported and received SVDM version is 2.0 or earlier and PD Revision should not be Rev3. */
                                    if (
                                            (CY_PDALTMODE_DP_SVID == vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid) &&
                                            (0u != (ptrAltModeContext->dpCfg->dp_operation & CY_PDALTMODE_DP2_1_CFG_MASK)) &&
                                            ((false == pd3_live) ||
                                             (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stVer < (uint32_t)CY_PDSTACK_STD_VDM_VER2) ||
                                             (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stMinVer < (uint32_t)CY_PDSTACK_STD_VDM_MINOR_VER1))
                                       )
                                    {
                                        /* Reset DPAM version to 2.0 or earlier as received SVDM version is 2.0 or earlier.  */
                                        pdAppStatusPtr->vdmResp.respBuf[1].std_dp_vdo.dpAmVersion = 0u;
                                    }
#endif /* ((DP_UFP_SUPP) && (DP_2_1_ENABLE)) */

                                    /* Sets the VDM response ACKed */
                                    pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = (uint32_t)CY_PDSTACK_CMD_TYPE_RESP_ACK;
                                }
                            }
#endif /* UFP_ALT_MODE_SUPP */
                            break;

                        case CY_PDSTACK_VDM_CMD_ENTER_MODE:
                            break;

                        case CY_PDSTACK_VDM_CMD_EXIT_MODE:
                            break;
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
                        case CY_PDSTACK_VDM_CMD_ATTENTION:
                            /* Ignores attention VDM */
                            pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_NOT_REQ;
                            break;

                        default:
                            /* No handling needed in other cases */
                            break;
                    }
                }

                /* Under PD2, handle VDMs are based on VDM manager functionality */
                if (!pd3_live)
                {
                    if (
                            (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid != /*STD_SVID*/ 0xFF00u) &&
                            (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd != (uint8_t)CY_PDSTACK_VDM_CMD_ATTENTION)
                       )
                    {
#if UFP_ALT_MODE_SUPP
                        if (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd > (uint8_t)CY_PDSTACK_VDM_CMD_DSC_MODES)
                        {
                            pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = (uint32_t)Cy_PdAltMode_Mngr_EvalRecVdm(ptrAltModeContext, vdm);
                        }
#endif /* UFP_ALT_MODE_SUPP */
                    }
                }
#if CY_PD_REV3_ENABLE
                else
                {
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
                    if(context->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
                    {
                        if ((vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ENTER_MODE) ||
                                (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_EXIT_MODE))
                        {
                            pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_READY;
                        }
                        else
                        {
                            if (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ATTENTION)
                            {
                                pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_NOT_REQ;
                            }
                            (void)Cy_PdAltMode_Mngr_EvalRecVdm(ptrAltModeContext, vdm);
                        }
                    }
                    else
                    {
                        if (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd > (uint8_t)CY_PDSTACK_VDM_CMD_DSC_MODES)
                        {
#if (UFP_ALT_MODE_SUPP && BUMBLEBEE_DOCK)
                            bool isSvidSupported = true;
                            /* Check for Enter Mode command to verify whether it is supported in current power in Bumblebee. */
                            if (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ENTER_MODE)
                            {
                                isSvidSupported = Cy_App_Vdm_IsSvidSupported((uint16_t)(vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid), appStatusPtr->alt_modes_dis_state);
                            }
                            /* Check whether SVID is supported, otherwise NAK response will be sent. */
                            if (isSvidSupported)
#endif /* (UFP_ALT_MODE_SUPP && BUMBLEBEE_DOCK) */
                            {
                                pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType = (uint32_t)Cy_PdAltMode_Mngr_EvalRecVdm(ptrAltModeContext, vdm);
                            }
                        }
                    }
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */
                }
#endif /* CY_PD_REV3_ENABLE */
            }
            else
            {
                if (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd == (uint32_t)CY_PDSTACK_VDM_CMD_ATTENTION) 
                {
#if DFP_ALT_MODE_SUPP
                    if(ptrAltModeContext->altModeAppStatus->vdmTaskEn != 0u)
                    {
                        /* Evaluates attention VDM */
                        (void)Cy_PdAltMode_Mngr_EvalRecVdm(ptrAltModeContext, vdm);
                    }
#endif /* DFP_ALT_MODE_SUPP */
                    /* No response to VDMs received on PD 2.0 connection in the DFP state */
                    pdAppStatusPtr->vdmResp.noResp = CY_PDSTACK_VDM_AMS_RESP_NOT_REQ;
                }
                else
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
                            (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmd <= (uint32_t)CY_PDSTACK_VDM_CMD_EXIT_MODE) ||
                            (
#if DP_DFP_SUPP
                             ((vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid & 0xFFFEu) != 0xFF00u)
#else
                             (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid != 0xFF00u)
#endif /* DP_DFP_SUPP */
                             &&
#if TBT_DFP_SUPP
                             (vdm->dat[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.svid != 0x8087u)
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

            /* Sets the VDM version for the response */
            pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stVer = pdAppStatusPtr->vdmVersion;
#if MINOR_SVDM_VER_SUPPORT
            pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.stMinVer = pdAppStatusPtr->vdmMinorVersion;
#endif /* MINOR_SVDM_VER_SUPPORT */
        }
    }
    else
    {
#if UVDM_SUPP
        if (uvdm_cmd_handler (context, (uint32_t *)(&vdm->hdr), &dobj, &count) == false)
        {
            return;
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
            (pdAppStatusPtr->vdmResp.respBuf[CY_PD_VDM_HEADER_IDX].std_vdm_hdr.cmdType != (uint32_t)CY_PDSTACK_CMD_TYPE_RESP_ACK) ||
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
static void ridge_mux_delay_for_usb4_cbk (cy_timer_id_t id, void * ptrContext)
{
    CY_UNUSED_PARAMETER(id);
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t*) ptrContext;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = ptrPdStackContext->ptrAltModeContext;
#if (UFP_ALT_MODE_SUPP && (CCG_BB_ENABLE != 0))
    uint8_t port = ptrPdStackContext->port;
#endif /* UFP_ALT_MODE_SUPP */

    ptrAltModeContext->altModeAppStatus->isMuxBusy = false;
    /* Stops the delay timer */
    Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, (uint16_t)CY_PDALTMODE_GR_MUX_DELAY_TIMER);

#if ((CCG_BB_ENABLE != 0) && (UFP_ALT_MODE_SUPP != 0))
#if BILLBOARD_1_2_2_SUPPORT
        if(ptrAltModeContext->altModeAppStatus->usb4Supp)
        {
            Cy_PdAltMode_Mngr_BillboardUpdate(ptrAltModeContext, 0u, (Cy_App_GetRespBuffer(port)->reqStatus == CY_PDSTACK_REQ_ACCEPT ? true: false));
#if BUMBLEBEE_DOCK
            if(true == glAppBbWarnPwrFail)
            {
                /* Enable BB controller */
                 (void)Cy_PdAltMode_Billboard_Enable(ptrAltModeContext, CY_PDALTMODE_BILLBOARD_CAUSE_PWR_FAILURE);
            }
#endif /* BUMBLEBEE_DOCK */
        }
#endif /* BILLBOARD_1_2_2_SUPPORT */
#endif /* ((CCG_BB_ENABLE != 0) && (UFP_ALT_MODE_SUPP != 0)) */

    if (app_ridge_resp_callback != NULL)
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
void Cy_App_Vdm_EvalEnterUsb(cy_stc_pdstack_context_t * context, const cy_stc_pdstack_pd_packet_t *eudo, cy_pdstack_app_resp_cbk_t app_resp_handler)
{
    uint8_t port = context->port;
    cy_stc_pdaltmode_context_t * ptrAltModeContext = context->ptrAltModeContext;
    cy_stc_pdstack_app_status_t* pdAppStatusPtr = Cy_App_GetPdAppStatus(port);
#if BUMBLEBEE_DOCK
    cy_stc_app_status_t *appStatusPtr = Cy_App_GetStatus(port);
#endif /* BUMBLEBEE_DOCK */
    /* Basic validity checks by the PD stack includes message length */
    cy_pd_pd_do_t eudo_dat    = eudo->dat[0];
    cy_pd_pd_do_t ufp_vdo = pdAppStatusPtr->vdmIdVdoResp[CY_PD_PRODUCT_TYPE_VDO_1_IDX];

#if GATKEX_CREEK
    ptrAltModeContext->altModeAppStatus->isMuxBusy = false;

    if(ptrAltModeContext->devExchCapStatus->portType == CY_PD_PRT_TYPE_UFP)
    {
        /* Sets the ridge to disconnect between USB and TBT states when device is UFP. */
        ptrAltModeContext->altModeAppStatus->isMuxBusy = true;
        Cy_PdAltMode_Ridge_SetDisconnect(context->ptrAltModeContext);
        (void)Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_PDALTMODE_GET_TIMER_ID(context, CY_PDALTMODE_GR_MUX_DELAY_TIMER),
                CY_PDALTMODE_RIDGE_GR_MUX_VDM_DELAY_TIMER_PERIOD, &ridge_mux_delay_for_usb4_cbk);
    }
#endif

    /* Default response: REJECT */
    Cy_App_GetRespBuffer(port)->reqStatus = CY_PDSTACK_REQ_REJECT;

    /* Matches the configuration from the Enter USB DO against the UFP VDO1.*/
    if (pdAppStatusPtr->vdmIdVdoCnt > CY_PD_PRODUCT_TYPE_VDO_1_IDX)
    {
        if (
#if BUMBLEBEE_DOCK
                (appStatusPtr->alt_modes_dis_state == CY_APP_ALT_MODES_DIS_NONE) &&
#endif /* BUMBLEBEE_DOCK */
                (eudo_dat.enterusb_vdo.usbMode == CY_PD_USB_MODE_USB4) &&
                ((ufp_vdo.ufp_vdo_1.devCap & (uint32_t)CY_PDSTACK_DEV_CAP_USB_4_0) != 0u)  &&
                (eudo_dat.enterusb_vdo.cableSpeed >= (uint32_t)CY_PDSTACK_USB_GEN_1)
           )
        {
#if (STORE_DETAILS_OF_HOST)
            if(ptrAltModeContext->devExchCapStatus->portType == CY_PD_PRT_TYPE_UFP)
            {
                ptrAltModeContext->devExchCapStatus->hostInfo->hostEudo = eudo->dat[0].val;
            }
#endif /* STORE_DETAILS_OF_HOST */

            Cy_App_GetRespBuffer(port)->reqStatus = CY_PDSTACK_REQ_ACCEPT;
        }

        else if (
#if BUMBLEBEE_DOCK
                    (appStatusPtr->alt_modes_dis_state <= CY_APP_ALT_MODES_DIS_USB4_TBT_MODE) &&
#endif /* BUMBLEBEE_DOCK */
                    (eudo_dat.enterusb_vdo.usbMode == CY_PD_USB_MODE_USB3) &&
                    ((ufp_vdo.ufp_vdo_1.devCap & (uint32_t)CY_PDSTACK_DEV_CAP_USB_3_2) != 0u) &&
                    (eudo_dat.enterusb_vdo.cableSpeed >= (uint32_t)CY_PDSTACK_USB_GEN_1)
                )
        {
            Cy_App_GetRespBuffer(port)->reqStatus = CY_PDSTACK_REQ_ACCEPT;
        }
        else if (
                    (eudo_dat.enterusb_vdo.usbMode == CY_PD_USB_MODE_USB2) &&
                    ((ufp_vdo.ufp_vdo_1.devCap & (uint32_t)CY_PDSTACK_DEV_CAP_USB_2_0) != 0u)
                )
        {
            Cy_App_GetRespBuffer(port)->reqStatus = CY_PDSTACK_REQ_ACCEPT;
        }
        else
        {
            /* No action needed */
        }
    }

    ptrAltModeContext->altModeAppStatus->eudo.val = eudo->dat[0].val;

    CY_UNUSED_PARAMETER(ufp_vdo);

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
#if BUMBLEBEE_DOCK
            if(true == glAppBbWarnPwrFail)
            {
                /* Enable BB controller */
                 (void)Cy_PdAltMode_Billboard_Enable(ptrAltModeContext, CY_PDALTMODE_BILLBOARD_CAUSE_PWR_FAILURE);
            }
#endif /* BUMBLEBEE_DOCK */
        }
#endif /* (BILLBOARD_1_2_2_SUPPORT && UFP_ALT_MODE_SUPP) */
#endif /* (CCG_BB_ENABLE != 0) */

        app_resp_handler(context, Cy_App_GetRespBuffer(port));
    }
}
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */
#endif /* (!CY_PD_VDM_DISABLE) */

/* [] END OF FILE */
