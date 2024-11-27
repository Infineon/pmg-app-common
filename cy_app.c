/***************************************************************************//**
* \file cy_app.c
* \version 2.0
*
* \brief
* Implements functions for handling the PDStack event callbacks and
* power saving
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "cybsp.h"
#include "cy_pdstack_common.h"
#include "cy_pdstack_dpm.h"
#if (!CY_PD_SINK_ONLY)
#include "cy_app_source.h"
#endif /* CY_PD_SINK_ONLY */
#include "cy_app_sink.h"
#include "cy_app_swap.h"
#include "cy_app_vdm.h"
#include "cy_app.h"
#include "cy_gpio.h"
#include "cy_app_usb.h"

#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"

#include "cy_app_fault_handlers.h"

#if BATTERY_CHARGING_ENABLE
#include "cy_app_battery_charging.h"
#endif /* BATTERY_CHARGING_ENABLE */

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
#include "cy_pdaltmode_hw.h"
#include "cy_pdaltmode_vdm_task.h"
#include "cy_pdaltmode_mngr.h"
#include "cy_pdaltmode_timer_id.h"

#if RIDGE_SLAVE_ENABLE
#include "cy_pdaltmode_ridge_slave.h"
#include "cy_pdaltmode_intel_ridge_common.h"
#include "cy_pdaltmode_soc_dock.h"
#endif /* RIDGE_SLAVE_ENABLE */

#if STORE_DETAILS_OF_HOST
#include "cy_pdaltmode_host_details.h"
#endif /* STORE_DETAILS_OF_HOST */

#if DP_UFP_SUPP
#include "cy_usbpd_hpd.h"
#include "cy_pdaltmode_dp_sid.h"
#endif /* DP_UFP_SUPP */
#if (CCG_BB_ENABLE != 0)
#include "cy_app_billboard.h"
#include "cy_pdaltmode_billboard.h"
#endif /* (CCG_BB_ENABLE != 0) */

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
#include "cy_pdaltmode_usb4.h"
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */

#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */

#include "cy_app_timer_id.h"

#if CY_PD_DP_VCONN_SWAP_FEATURE
#include "cy_usbpd_typec.h"
#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */

#if CY_HPI_MASTER_ENABLE
#include "cy_hpi_master.h"
#endif /* CY_HPI_MASTER_ENABLE */

#if CY_USE_CONFIG_TABLE
#include "cy_usbpd_config_table.h"
#endif /* CY_USE_CONFIG_TABLE */

#if CY_HPI_ENABLED
#include "cy_hpi.h"
#if CY_HPI_RW_PD_RESP_MSG_DATA
#include "cy_app_hpi.h"
#endif /* CY_HPI_RW_PD_RESP_MSG_DATA */
#endif /* CY_HPI_ENABLED */

#if CY_APP_RTOS_ENABLED
#include "FreeRTOS.h"
#include "semphr.h"
#include "cyabs_rtos_internal.h"
#include "cyabs_rtos_impl.h"
#endif /* CY_APP_RTOS_ENABLED */

#if CY_CORROSION_MITIGATION_ENABLE
#include "cy_app_moisture_detect.h"
#endif /* CY_CORROSION_MITIGATION_ENABLE */

#if CCG_TYPE_A_PORT_ENABLE
cy_stc_app_status_t glAppStatus[2];
#else
cy_stc_app_status_t glAppStatus[NO_OF_TYPEC_PORTS];
#endif /* CCG_TYPE_A_PORT_ENABLE */

cy_stc_pdstack_app_status_t glAppPdStatus[NO_OF_TYPEC_PORTS];

/* Flag to indicate that activity timer timed out. */
static volatile bool ccg_activity_timer_flag[NO_OF_TYPEC_PORTS] = {false};

#if CY_APP_RTOS_ENABLED
SemaphoreHandle_t event_sema_handle[NO_OF_TYPEC_PORTS] = {NULL};
#endif /* CY_APP_RTOS_ENABLED */

static uint8_t glAppPrevPolarity[NO_OF_TYPEC_PORTS];
#if (CY_PD_EPR_ENABLE && (!CY_PD_SOURCE_ONLY))
static bool glAppResetEpr[NO_OF_TYPEC_PORTS];
#endif /* (CY_PD_EPR_ENABLE && (!CY_PD_SOURCE_ONLY)) */

#if CY_PD_REV3_ENABLE
void send_get_revision(cy_timer_id_t id, void *ptrContext);
#endif /* CY_PD_REV3_ENABLE */
#if CY_HPI_MASTER_ENABLE
extern cy_hpi_master_context_t *get_hpi_master_context(void);
#endif /* CY_HPI_MASTER_ENABLE */

cy_en_usbpd_adc_id_t glAppVbusPollAdcId[NO_OF_TYPEC_PORTS] = {CY_USBPD_ADC_ID_0};
cy_en_usbpd_adc_input_t glAppVbusPollAdcInput[NO_OF_TYPEC_PORTS] = {CY_USBPD_ADC_INPUT_AMUX_A};

#if ((CY_PD_REV3_ENABLE) && (CY_APP_GET_REVISION_ENABLE))
static bool glAppGetRevSendStatus[NO_OF_TYPEC_PORTS] = {
    false, 
#if PMG1_PD_DUALPORT_ENABLE
    false
#endif /* PMG1_PD_DUALPORT_ENABLE */
};

static uint8_t glAppGetRevRetry[NO_OF_TYPEC_PORTS] = {
    0x00u, 
#if PMG1_PD_DUALPORT_ENABLE
    0x00u
#endif /* PMG1_PD_DUALPORT_ENABLE */
};
#endif /* ((CY_PD_REV3_ENABLE) && (CY_APP_GET_REVISION_ENABLE)) */

#if CY_PD_REV3_ENABLE
/* Temporary storage for ongoing AMS type while handling chunked extended messages */
static cy_en_pdstack_ams_type_t glExtdAmsType[NO_OF_TYPEC_PORTS]; 

/* Global variable used as dummy data buffer to send chunk request messages */
static uint32_t glExtdData;

#if (CY_APP_HOST_ALERT_MSG_DISABLE != 1)
uint32_t get_bat_status[NO_OF_TYPEC_PORTS];
#endif /* (CY_APP_HOST_ALERT_MSG_DISABLE != 1) */
#endif /* CY_PD_REV3_ENABLE */

#if (CY_APP_ROLE_PREFERENCE_ENABLE)
/* Variable storing current preference for data role */
volatile uint8_t glAppPrefDataRole[NO_OF_TYPEC_PORTS];

#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
/* Variable storing current preference for power role */
volatile uint8_t glAppPrefPowerRole[NO_OF_TYPEC_PORTS];
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */

/* Forward declaration of function to trigger swap operations */
static void app_initiate_swap (cy_timer_id_t id, void *context);
#endif /* (CY_APP_ROLE_PREFERENCE_ENABLE) */

#if (!CY_PD_SINK_ONLY)
/* This flag holds whether there is discharge being applied from invalid VBUS state. */
static bool glAppInvalidVbusDisOn[NO_OF_TYPEC_PORTS];
#endif /* (!CY_PD_SINK_ONLY) */

/* Pointer to the structure holding the solution callback function. */
cy_app_sln_cbk_t *glPtrSlnCbk;

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
static bool app_is_vdm_task_ready(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    /* Assume cable discovery finished when device is UFP */
    bool retval = true;

#if DFP_ALT_MODE_SUPP
#if (!CCG_CBL_DISC_DISABLE) && (CY_APP_ROLE_PREFERENCE_ENABLE)
    uint8_t port = ptrPdStackContext->port;
#endif /* (!CCG_CBL_DISC_DISABLE) && (CY_APP_ROLE_PREFERENCE_ENABLE) */

    cy_stc_pdaltmode_context_t *ptrAltModeContext = ptrPdStackContext->ptrAltModeContext;

    /* This check only makes sense for DFP */
    if (ptrPdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_UFP)
    {
#if (CY_APP_ROLE_PREFERENCE_ENABLE)
        /* Do not proceed with alternate mode if DR_SWAP is pending */
        if ((glAppStatus[port].app_pending_swaps & CY_APP_DR_SWAP_PENDING) != 0)
        {
            return false;
        }
#endif /* (CY_APP_ROLE_PREFERENCE_ENABLE) */

#if (CY_PD_EPR_ENABLE)
        if(ptrPdStackContext->peStat.eprMultiMessageFlag)
        {
            return false;
        }
#endif

#if (!CCG_CBL_DISC_DISABLE)
        /*
         * Set the cable discovered flag if:
         * 1. Cable discovery is disabled
         * 2. EMCA present flag in DPM is set
         * 3. Cable discovery process not restarted
         */
        if (
                (ptrPdStackContext->dpmConfig.cblDsc == false) ||
                ((ptrPdStackContext->dpmConfig.emcaPresent != false) &&
                 (ptrAltModeContext->altModeAppStatus->discCblPending == false))
           )
        {
            ptrAltModeContext->altModeAppStatus->cblDiscIdFinished = true;
        }

        /* Return the status of cable discovered flag */
        retval = ptrAltModeContext->altModeAppStatus->cblDiscIdFinished;
#endif /* (!CCG_CBL_DISC_DISABLE) */
    }

#endif /* DFP_ALT_MODE_SUPP */

    return retval;
}
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

#if ((CY_PD_REV3_ENABLE) && (CY_APP_GET_REVISION_ENABLE))
void get_rev_cb(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t resp, const cy_stc_pdstack_pd_packet_t* pkt_ptr)
{
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
    cy_stc_pdaltmode_context_t *ptrAltModeCtx = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */

    /* Handle case for NOT_SUPPORTED and REVISION response */
    if(resp == CY_PDSTACK_RES_RCVD)
    {
        if(((1UL << pkt_ptr->hdr.hdr.msgType) & CY_PD_DATA_MSG_REVISION_MASK) != 0UL)
        {
            /* Revision message received */
            Cy_App_GetStatus(ptrPdStackContext->port)->pd_revision = (uint32_t)pkt_ptr->dat[0].val;
        }
        else
        {
            /* Not supported */
        }
        glAppGetRevSendStatus[ptrPdStackContext->port] = true;
    }
    else
    {
        if(resp != CY_PDSTACK_CMD_SENT)
        {
            if(resp == CY_PDSTACK_RES_TIMEOUT)
            {
                if(glAppGetRevRetry[ptrPdStackContext->port] < CY_APP_GET_REV_PD_CMD_RETRY_LIMIT)
                {
                    glAppGetRevRetry[ptrPdStackContext->port]++;
                }
                else
                {
                    glAppGetRevSendStatus[ptrPdStackContext->port] = true;
                }
            }
            
            if (glAppGetRevSendStatus[ptrPdStackContext->port] == false)
            {
                /* If the transmission was successful but response timed out, then attempt retrying the AMS. */
                Cy_PdUtils_SwTimer_Start (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                    CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_PD_GET_REVISION_COMMAND_RETRY_TIMER), CY_APP_GET_REV_PD_CMD_RETRY_TIMER_PERIOD, send_get_revision);
            }
        }
    }

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
    /* Enable VDM manager for DFP if the transmission of get revision message is completed */
    if(
            (ptrAltModeCtx->altModeAppStatus->vdmPrcsFailed == false) &&
            (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_DFP) &&
            (glAppGetRevSendStatus[ptrPdStackContext->port] == true)
      )
    {
        Cy_PdAltMode_VdmTask_Enable(ptrAltModeCtx);
    }
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
}


void send_get_revision(cy_timer_id_t id, void *ptrContext)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)ptrContext;

    /* Nothing to do if we are not in PD contract */
    if (!ptrPdStackContext->dpmConfig.contractExist)
    {
        return;
    }
    if(Cy_PdStack_Dpm_SendPdCommand(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_GET_REVISION, NULL, false, (cy_pdstack_dpm_pd_cmd_cbk_t)get_rev_cb) != CY_PDSTACK_STAT_SUCCESS)
    {
        Cy_PdUtils_SwTimer_Start (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                id, CY_APP_GET_REV_PD_CMD_RETRY_TIMER_PERIOD, send_get_revision);
    }
}
#endif /* ((CY_PD_REV3_ENABLE) && (CY_APP_GET_REVISION_ENABLE)) */

cy_en_pdstack_status_t Cy_App_Fault_DisablePort(cy_stc_pdstack_context_t *ptrPdStackContext, cy_pdstack_dpm_typec_cmd_cbk_t cbk)
{
    cy_en_pdstack_status_t retval = CY_PDSTACK_STAT_SUCCESS;
    uint8_t port = ptrPdStackContext->port;

    if (Cy_PdUtils_SwTimer_IsRunning (ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_FAULT_RECOVERY_TIMER)))
    {
        /* If the HPI master is asking us to disable the port, make sure all fault protection state is cleared. */
        glAppPdStatus[port].faultStatus &= ~(
                CY_APP_PORT_VBUS_DROP_WAIT_ACTIVE | CY_APP_PORT_SINK_FAULT_ACTIVE | CY_APP_PORT_DISABLE_IN_PROGRESS |
                CY_APP_PORT_VCONN_FAULT_ACTIVE | CY_APP_PORT_V5V_SUPPLY_LOST);

        Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_FAULT_RECOVERY_TIMER));

        Cy_USBPD_TypeC_DisableRd(ptrPdStackContext->ptrUsbPdContext, CY_PD_CC_CHANNEL_1);
        Cy_USBPD_TypeC_DisableRd(ptrPdStackContext->ptrUsbPdContext, CY_PD_CC_CHANNEL_2);
        cbk(ptrPdStackContext, CY_PDSTACK_DPM_RESP_SUCCESS);
    }
    else
    {
        /* Just pass the call on-to the stack */
        if (ptrPdStackContext->dpmConfig.dpmEnabled)
        {
            retval = Cy_PdStack_Dpm_SendTypecCommand(ptrPdStackContext,  CY_PDSTACK_DPM_CMD_PORT_DISABLE,  cbk);
        }
        else
        {
            cbk(ptrPdStackContext, CY_PDSTACK_DPM_RESP_SUCCESS);
        }
    }

    return retval;
}


#if (!CCG_CBL_DISC_DISABLE)
void app_cbl_dsc_callback (cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t resp,
        const cy_stc_pdstack_pd_packet_t *pkt_ptr)
{
    (void)pkt_ptr;

    /* Keep repeating the DPM command until we succeed */
    if (resp == CY_PDSTACK_SEQ_ABORTED)
    {
        Cy_PdUtils_SwTimer_Start (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_CBL_DISC_TRIGGER_TIMER), CY_APP_CBL_DISC_TIMER_PERIOD, Cy_App_CableDiscTimerCallback);
    }
}


void Cy_App_CableDiscTimerCallback (cy_timer_id_t id, void *callbackContext)
{
    cy_stc_pdstack_context_t *pdstack_context = callbackContext;

    if (Cy_PdStack_Dpm_SendPdCommand(pdstack_context, CY_PDSTACK_DPM_CMD_INITIATE_CBL_DISCOVERY, NULL, false, app_cbl_dsc_callback) != CY_PDSTACK_STAT_SUCCESS )
    {
        /* Start timer which will send initiate the DPM command after a delay */
        app_cbl_dsc_callback(pdstack_context, CY_PDSTACK_SEQ_ABORTED, 0);
    }

    (void) id;
}
#endif /* (!CCG_CBL_DISC_DISABLE) */

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
static void app_reset_disc_info (cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    ptrAltModeContext->altModeAppStatus->tbtCblVdo.val  = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->altModeAppStatus->tbtModeVdo.val = CY_PDALTMODE_MNGR_NO_DATA;
    ptrAltModeContext->altModeAppStatus->dpCblVdo.val   = CY_PDALTMODE_MNGR_NO_DATA;
}
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */

bool Cy_App_VdmLayerReset(cy_stc_pdstack_context_t *ptrPdStackContext)
{
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
    cy_stc_pdaltmode_context_t* ptrAltModeContext = ptrPdStackContext->ptrAltModeContext;

    bool stat = true;

    if ((ptrPdStackContext->dpmConfig.contractExist) && (  ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_DFP))
    {
        if(ptrAltModeContext->altModeAppStatus->altModeEntered == false)
        {
            /*
             * Reset the alternate mode state machine. The cable discovery complete flag is also cleared so
             * that alternate mode state machine can be started at the end of cable discovery.
             */
            Cy_PdAltMode_Mngr_LayerReset(ptrAltModeContext);
        }
        else
        {
            return stat;
        }
#if (!CCG_CBL_DISC_DISABLE)
        ptrAltModeContext->altModeAppStatus->cblDiscIdFinished = false;
        ptrAltModeContext->altModeAppStatus->discCblPending = true;

        /* Ask PD stack to trigger cable discovery */
        if (Cy_PdStack_Dpm_SendPdCommand(ptrPdStackContext, CY_PDSTACK_DPM_CMD_INITIATE_CBL_DISCOVERY, NULL, false, app_cbl_dsc_callback) !=  CY_PDSTACK_STAT_SUCCESS)
        {
            Cy_PdUtils_SwTimer_Start (ptrPdStackContext->ptrTimerContext, ptrPdStackContext ,
                    CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_CBL_DISC_TRIGGER_TIMER), CY_APP_CBL_DISC_TIMER_PERIOD, Cy_App_CableDiscTimerCallback);
        }
#endif /* (!CCG_CBL_DISC_DISABLE) */
    }
    else
    {
#if VCONN_OCP_ENABLE
        /* If there is no PD contract in place and we are VConn source, enable VConn and move on. */
        if ((ptrPdStackContext->dpmConfig.attach) && (ptrPdStackContext->dpmConfig.vconnLogical))
        {
            Cy_USBPD_Vconn_Enable(ptrPdStackContext->ptrUsbPdContext, ptrPdStackContext->dpmConfig.revPol);
        }
#endif /* VCONN_OCP_ENABLE */

        stat = false;
    }
    return stat;
#else
    (void) ptrPdStackContext;
    return false;
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */
}


bool Cy_App_ValidateCfgTableOffsets(cy_stc_usbpd_context_t *ptrUsbPdPortCtx)
{
#if CY_USE_CONFIG_TABLE

    cy_stc_usbpd_context_t *ptrUsbPdContext = ptrUsbPdPortCtx;
    const dock_config_t *ptrDockConfig = get_dock_config(ptrUsbPdContext);
#if (VBUS_OVP_ENABLE | VBUS_OCP_ENABLE | VBUS_RCP_ENABLE | VBUS_UVP_ENABLE | \
    VBUS_SCP_ENABLE | VCONN_OCP_ENABLE | OTP_ENABLE)
    const pd_port_config_t *portConf = get_pd_port_config(ptrUsbPdContext);
#endif /* (VBUS_OVP_ENABLE | VBUS_OCP_ENABLE | VBUS_RCP_ENABLE | VBUS_UVP_ENABLE | \
    VBUS_SCP_ENABLE | VCONN_OCP_ENABLE | OTP_ENABLE) */

#if (CY_CONFIG_TABLE_TYPE == CY_CONFIG_TABLE_DOCK)
    /* Check if the config table is for dock application or not */
    if ((CY_CONFIG_TABLE_DOCK != ptrDockConfig->table_type) ||
        (sizeof(dock_config_t) > ptrDockConfig->table_size))
    {
        return false;
    }
#endif /* (CY_CONFIG_TABLE_TYPE == CY_CONFIG_TABLE_DOCK) */

#if VBUS_OVP_ENABLE
    if ((portConf->port_n_ovp_table_offset == 0u) ||
        (portConf->port_n_ovp_table_len < sizeof(ovp_settings_t)))
    {
        return false;
    }
#endif /* VBUS_OVP_ENABLE */

#if VBUS_OCP_ENABLE
    if ((portConf->port_n_ocp_table_offset == 0u) ||
        (portConf->port_n_ocp_table_len < sizeof(ocp_settings_t)))
    {
        return false;
    }
#endif /* VBUS_OCP_ENABLE */

#if VBUS_RCP_ENABLE
    if ((portConf->port_n_rcp_table_offset == 0u) ||
        (portConf->port_n_rcp_table_len < sizeof(rcp_settings_t)))
    {
        return false;
    }
#endif /* VBUS_RCP_ENABLE */

#if VBUS_UVP_ENABLE
    if ((portConf->port_n_uvp_table_offset == 0u) ||
        (portConf->port_n_uvp_table_len < sizeof(uvp_settings_t)))
    {
        return false;
    }
#endif /* VBUS_UVP_ENABLE */

#if VBUS_SCP_ENABLE
    if ((portConf->port_n_scp_table_offset == 0u) ||
        (portConf->port_n_scp_table_len < sizeof(scp_settings_t)))
    {
        return false;
    }
#endif /* VBUS_SCP_ENABLE */

#if VCONN_OCP_ENABLE
    if ((portConf->port_n_vconn_ocp_table_offset == 0u) ||
        (portConf->port_n_vconn_ocp_table_len < sizeof(vconn_ocp_settings_t)))
    {
        return false;
    }

    /* No retries for VCONN OCP */
#endif /* VCONN_OCP_ENABLE */

#if OTP_ENABLE
    if ((portConf->port_n_otp_table_offset == 0u) ||
         (portConf->port_n_otp_table_len < sizeof(otp_settings_t)))

    {
        return false;
    }
#endif /* OTP_ENABLE */

    return true;
#else
    return false;
#endif /* CY_USE_CONFIG_TABLE */
}

static void ccg_activity_timer_cb(cy_timer_id_t id, void *callbackContext)
{
    (void)id;
    cy_stc_pdstack_context_t *context = callbackContext;
    /*
     * Activity timer expired. Generate an event so that PMG1 periodic checks
     * can run.
     */
    ccg_activity_timer_flag[0] = true;
    Cy_App_SendRtosEvent(Cy_PdStack_Dpm_GetContext(0));
#if PMG1_PD_DUALPORT_ENABLE
    ccg_activity_timer_flag[1] = true;
    Cy_App_SendRtosEvent(Cy_PdStack_Dpm_GetContext(1));
#endif /* PMG1_PD_DUALPORT_ENABLE */

    Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_PDUTILS_CCG_ACTIVITY_TIMER, CCG_ACTIVITY_TIMER_PERIOD,
            ccg_activity_timer_cb);
}

#if (!CY_PD_SINK_ONLY)
static void app_psrc_invalid_vbus_dischg_disable(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    if (glAppInvalidVbusDisOn[ptrPdStackContext->port] == true)
    {
        glAppInvalidVbusDisOn[ptrPdStackContext->port] = false;
        Cy_App_VbusDischargeOff(ptrPdStackContext);
        Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_PSOURCE_DIS_TIMER));
    }
}

/*
 * Timer callback function in case of timeout from VBUS discharge when trying to remove
 * invalid VBUS voltage from the bus.
 */
static void app_psrc_invalid_vbus_tmr_cbk(cy_timer_id_t id,  void * callbackCtx)
{
    (void)id;
    cy_stc_pdstack_context_t *ptrPdStackContext = callbackCtx;
    /* Disable the discharge if it is running. */
    app_psrc_invalid_vbus_dischg_disable(ptrPdStackContext);
}
#endif /* (!CY_PD_SINK_ONLY) */

uint8_t Cy_App_Task(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    Cy_App_Fault_Task (ptrPdStackContext);

#if BATTERY_CHARGING_ENABLE
    Cy_App_Bc_Task (ptrPdStackContext->ptrUsbPdContext);
#if CCG_TYPE_A_PORT_ENABLE
    Cy_App_Bc_Task (ptrPdStackContext->ptrUsbPdContext->altPortUsbPdCtx[0]);
#endif /* CCG_TYPE_A_PORT_ENABLE */
#endif /* BATTERY_CHARGING_ENABLE */

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
    cy_stc_pdaltmode_context_t* ptrAltModeContext = ptrPdStackContext->ptrAltModeContext;

    /* If VDM processing is allowed */
    if(ptrAltModeContext->altModeAppStatus->vdmTaskEn != false)
    {
        /* Wait for cable discovery completion before going to alt. modes */
        if (app_is_vdm_task_ready (ptrPdStackContext))
        {
            Cy_PdAltMode_VdmTask_Manager (ptrAltModeContext);
        }
    }

#if RIDGE_SLAVE_ENABLE
    Cy_PdAltMode_Ridge_Task(ptrAltModeContext);
    Cy_PdAltMode_SocDock_Task(ptrAltModeContext);
#endif /* RIDGE_SLAVE_ENABLE */

    Cy_PdAltMode_Mngr_Task(ptrAltModeContext);

#if (CCG_BB_ENABLE != 0)
    if (Cy_PdAltMode_Billboard_IsPresent(ptrAltModeContext) != false)
    {
        Cy_App_Usb_BbTask(ptrAltModeContext);
    }
#endif /* (CCG_BB_ENABLE != 0) */
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */

#if DFP_ALT_MODE_SUPP
    if ((!Cy_PdStack_Dpm_GetAutoVcsEnabled(ptrPdStackContext)) && (Cy_PdAltMode_VdmTask_IsIdle(ptrAltModeContext)))
    {
        /* We can re-enable auto VConn SWAP once the VDM task is idle */
        Cy_PdStack_Dpm_UpdateAutoVcsEnable(ptrPdStackContext, true);

        /* Revert the VConn retain setting to its original value */
        Cy_PdStack_Dpm_UpdateVconnRetain(ptrPdStackContext, ptrPdStackContext->ptrPortCfg->vconnRetain);
    }
#endif /* DFP_ALT_MODE_SUPP */

    if(ccg_activity_timer_flag[ptrPdStackContext->port] == true)
    {
#if CCG_TYPE_A_PORT_ENABLE
        glPtrSlnCbk->type_a_detect_disconnect();
#endif /* CCG_TYPE_A_PORT_ENABLE */

#if CY_CORROSION_MITIGATION_ENABLE
        if((ptrPdStackContext->dpmConfig.attach == false) ||
           (ptrPdStackContext->typecStat.moisturePresent == true))
        {
            Cy_App_MoistureDetect_Run(ptrPdStackContext);
        }
#endif /* CY_CORROSION_MITIGATION_ENABLE */
        ccg_activity_timer_flag[ptrPdStackContext->port] = false;
    }

    return true;
}

#if SYS_DEEPSLEEP_ENABLE

bool Cy_App_Sleep(void)
{
    bool stat = true;
    uint8_t port;

#if (((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) || ((DP_UFP_SUPP) && (PMG1_HPD_RX_ENABLE)))
    cy_stc_pdstack_context_t *ptrPdStackContext = NULL;
#endif /* (((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) || ((DP_UFP_SUPP) && (PMG1_HPD_RX_ENABLE))) */

#if CY_APP_USB_ENABLE
    if (Cy_App_Usb_IsIdle () == true)
    {
#if (CCG_BB_ENABLE != 0)
        if(Cy_App_Usb_BbIsIdle ((Cy_PdStack_Dpm_GetContext(TYPEC_PORT_0_IDX))->ptrAltModeContext) != true)
        {
            return false;
        }
#endif /* (CCG_BB_ENABLE != 0) */
    }
    else
    {
        return false;
    }
#endif /* CY_APP_USB_ENABLE */

    for (port = 0; port < NO_OF_TYPEC_PORTS; port++)
    {
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
        cy_stc_pdstack_context_t *ptrPdStackContext = Cy_PdStack_Dpm_GetContext(port);
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

        /* Do not go to sleep if activity timer timeout event is pending. */
        if(ccg_activity_timer_flag[port] == true)
        {
            stat = false;
            break;
        }
        /* Do not go to Sleep while the CC/SBU fault handling is pending */
        if ((Cy_App_GetPdAppStatus(port)->faultStatus & CY_APP_PORT_SINK_FAULT_ACTIVE) != 0)
        {
            stat = false;
            break;
        }

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
        if (Cy_PdAltMode_VdmTask_IsIdle(ptrPdStackContext->ptrAltModeContext) == false)
        {
            stat = false;
            break;
        }
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */

#if (DP_UFP_SUPP) && (PMG1_HPD_RX_ENABLE)
        /* CDT 245126 workaround: Check if HPD RX activity timer is running
         * If yes, don't enter Deep Sleep. */
        /* If timer is running, HPD RX module is busy. */
        if (Cy_PdUtils_SwTimer_IsRunning (ptrPdStackContext->ptrTimerContext, CY_PDSTACK_HPD_RX_ACTIVITY_TIMER_ID))
        {
            stat = false;
            break;
        }
#endif /* DP_UFP_SUPP && PMG1_HPD_RX_ENABLE */

    }

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
    if (stat)
    {
#if CY_APP_USB_ENABLE
        Cy_App_Usb_Sleep();
#endif /* CY_APP_USB_ENABLE */

        for (port = 0; port < NO_OF_TYPEC_PORTS; port++)
        {
            ptrPdStackContext = Cy_PdStack_Dpm_GetContext(port);
            /* Prepare for Deep-Sleep entry */
            Cy_PdAltMode_Mngr_Sleep(ptrPdStackContext->ptrAltModeContext);
        }
    }
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */

    return stat;
}

void Cy_App_Resume(void)
{
#if    (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)
#if CY_APP_USB_ENABLE
    Cy_App_Usb_Resume();
#endif /* CY_APP_USB_ENABLE */

    uint8_t port;
    cy_stc_pdstack_context_t *ptrPdStackContext = NULL;
    for (port = 0; port < NO_OF_TYPEC_PORTS; port++)
    {
        ptrPdStackContext = Cy_PdStack_Dpm_GetContext(port);
        Cy_PdAltMode_Mngr_Wakeup (ptrPdStackContext->ptrAltModeContext);
    }
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */
}
#endif /* SYS_DEEPSLEEP_ENABLE */

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
/* USB4 timeout callback function */
static void usb4_tmr_cbk(cy_timer_id_t id,  void *context)
{
    (void)id;

    cy_stc_pdstack_context_t * ptrPdStackContext = (cy_stc_pdstack_context_t *) context;
    cy_stc_pdaltmode_context_t * ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    /* Set MUX to safe/USB3 after USB4 timeout elapsed */
    (void)Cy_PdAltMode_HW_SetMux(ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);
}

static bool Cy_App_isUfpUsb4Supp (cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_stc_pdstack_app_status_t* appStat = ptrAltModeContext->appStatusContext;
    bool ret = false;

    /* Check if port is UFP and USB4 capable */
    if (
           (ALT_MODE_CALL_MAP(Cy_PdAltMode_VdmTask_IsUSB4Supp)(&appStat->vdmIdVdoResp[CY_PD_ID_HEADER_IDX], (appStat->vdmIdVdoCnt - CY_PD_ID_HEADER_IDX))) &&
           (ptrAltModeContext->pdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP)
       )
    {
        ret = true;
    }

    return ret;
}

#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */

#if (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)
#if (CCG_BB_ENABLE != 0)
/* Alternate mode entry timeout callback function */
void ame_tmr_cbk(cy_timer_id_t id, void *context)
{
    (void)id;
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
    /* Update datapath with USB SS in case of UFP USB4-capable port */
    if (Cy_App_isUfpUsb4Supp(ptrAltModeContext) != false)
    {
        usb4_tmr_cbk(CY_PDALTMODE_AME_TIMEOUT_TIMER, context);
    }
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */

    /* Alternate modes are reset in vdm_task_mngr_deinit() */
    Cy_PdAltMode_Billboard_Enable(ptrAltModeContext, CY_PDALTMODE_BILLBOARD_CAUSE_AME_TIMEOUT);
}
#endif /* (CCG_BB_ENABLE != 0) */
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */


#if CY_PD_REV3_ENABLE
void extd_msg_cb(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_resp_status_t resp,
        const cy_stc_pdstack_pd_packet_t *pkt_ptr)
{
    (void)pkt_ptr;

    if(resp == CY_PDSTACK_RES_RCVD)
    {
        ptrPdStackContext->dpmStat.nonIntrResponse = glExtdAmsType[ptrPdStackContext->port];
    }
    if(resp == CY_PDSTACK_CMD_SENT)
    {
        glExtdAmsType[ptrPdStackContext->port] = ptrPdStackContext->dpmStat.nonIntrResponse;
    }
}

bool app_extd_msg_handler(cy_stc_pdstack_context_t *ptrPdStackContext, cy_stc_pd_packet_extd_t *pd_pkt_p)
{
#if ((CY_HPI_ENABLED) && (CY_HPI_RW_PD_RESP_MSG_DATA))
#if (CY_PD_BAT_STATUS_HANDLER_ENABLE)
    cy_stc_pdstack_dpm_pd_cmd_buf_t extd_dpm_buf;
    cy_stc_app_pd_resp_stored_t *ptrPdResp;

    if (pd_pkt_p->msg == CY_PDSTACK_EXTD_MSG_GET_BAT_STATUS)
    {
        /* Battery status response stored is 8 bytes wide including the header. */
        cy_stc_app_pd_resp_data_t batStatPdResp;
        batStatPdResp.respId      = CY_APP_PD_RESP_ID_BAT_STAT;
        batStatPdResp.respLen     = Cy_App_Hpi_GetResponseMinLen(CY_APP_PD_RESP_ID_BAT_STAT);
        /* Match All ports or per port response.*/
        batStatPdResp.cmdVal      = (uint8_t)(0x01u << ptrPdStackContext->port);
        /* Get the battery index from the pd packet. */
        batStatPdResp.respData[0] = pd_pkt_p->dat[0].val & 0xFF;
        batStatPdResp.respData[1] = 0x00u;
        extd_dpm_buf.cmdSop       = CY_PD_SOP;
        extd_dpm_buf.timeout      = 0;

        /* Retrieve the battery status response with minimum length.*/
        ptrPdResp = Cy_App_Hpi_RetrievePdRespData(&batStatPdResp,
                                          Cy_App_Hpi_GetResponseMatchLen(CY_APP_PD_RESP_ID_BAT_STAT));
        /* Check if no response then try with invalid battery slot ID. */
        if (NULL == ptrPdResp)
        {
            batStatPdResp.respData[0] = CY_APP_PD_RESP_INVLD_BAT_SLOT_ID;
            batStatPdResp.respData[1] = CY_APP_PD_RESP_INVLD_BATT_REF_FLAG;
            ptrPdResp = Cy_App_Hpi_RetrievePdRespData(&batStatPdResp, CY_APP_PD_RESP_MATCH_LENGTH);
        }
        if (NULL != ptrPdResp)
        {
            uint8_t * ptrData = (uint8_t*)&ptrPdResp->respData;
            /* Response is found. Hence send the response.*/
            memcpy(&extd_dpm_buf.cmdDo[0].val,
                   ((uint8_t*)ptrData + CY_APP_PD_RESP_DATA_HDR_SIZE),
                   (ptrPdResp->respLen - CY_APP_PD_RESP_DATA_HDR_SIZE));
            /* Adjust the length to multiple of DOs if not.*/
            extd_dpm_buf.noOfCmdDo = (ptrPdResp->respLen - CY_APP_PD_RESP_DATA_HDR_SIZE + 3) >> 2u;
            /* Send the battery status response stored by EC. */
            Cy_Pdstack_Dpm_SendPdCommandEc(ptrPdStackContext,
                                             CY_PDSTACK_DPM_CMD_SEND_BATT_STATUS,
                                             &extd_dpm_buf,
                                             NULL);
            return true;
        }
    }
#endif /* (CY_PD_BAT_STATUS_HANDLER_ENABLE) */
#if (CY_PD_BAT_CAPS_HANDLER_ENABLE)
    if (pd_pkt_p->msg == CY_PDSTACK_EXTD_MSG_GET_BAT_CAP)
    {
        /* Battery capability response stored is 13 bytes wide including the header. */
        cy_stc_app_pd_resp_data_t batCapPdResp;
        batCapPdResp.respId        = CY_APP_PD_RESP_ID_BAT_CAP;
        batCapPdResp.respLen       = Cy_App_Hpi_GetResponseMinLen(CY_APP_PD_RESP_ID_BAT_CAP);
        /* Match All ports or per port response.*/
        batCapPdResp.cmdVal        = (uint8_t)(0x01u << ptrPdStackContext->port);
        /* Get the battery index from the pd packet. */
        batCapPdResp.respData[0]   = pd_pkt_p->dat[0].val & 0xFF;
        batCapPdResp.respData[1]   = 0x00u;
        extd_dpm_buf.cmdSop        = CY_PD_SOP;
        extd_dpm_buf.extdType      = CY_PDSTACK_EXTD_MSG_BAT_CAP;
        extd_dpm_buf.extdHdr.val   = 0;
        extd_dpm_buf.timeout       = 0;

        ptrPdResp = Cy_App_Hpi_RetrievePdRespData(&batCapPdResp,
                                          Cy_App_Hpi_GetResponseMatchLen(CY_APP_PD_RESP_ID_BAT_CAP));
        /* Check if no response then try with invalid battery slot ID. */
        if (NULL == ptrPdResp)
        {
            batCapPdResp.respData[0] = CY_APP_PD_RESP_INVLD_BAT_SLOT_ID;
            batCapPdResp.respData[1] = CY_APP_PD_RESP_INVLD_BATT_REF_FLAG;
            ptrPdResp = Cy_App_Hpi_RetrievePdRespData(&batCapPdResp,
                                              Cy_App_Hpi_GetResponseMatchLen(CY_APP_PD_RESP_ID_BAT_CAP));
        }
        if (NULL != ptrPdResp)
        {
            uint8_t * ptrData = (uint8_t*)&ptrPdResp->respData;
            /* Update the response data and size. */
            extd_dpm_buf.extdHdr.extd.dataSize = ptrPdResp->respLen - CY_APP_PD_RESP_DATA_HDR_SIZE;
            extd_dpm_buf.datPtr        = (uint8_t*)ptrData + CY_APP_PD_RESP_DATA_HDR_SIZE;
            /* Send the battery capabilities response stored by EC. */
            Cy_PdStack_Dpm_SendPdCommand(ptrPdStackContext,
                                       CY_PDSTACK_DPM_CMD_SEND_EXTENDED,
                                       &extd_dpm_buf,
                                       true,
                                       NULL);
            return true;
        }
    }
#endif /* (CY_PD_BAT_CAPS_HANDLER_ENABLE) */
    {
#if (CY_HPI_PD_ENABLE)
        /* Don't do anything here if forwarding to EC is enabled. */
        if (HPI_CALL_MAP(Cy_Hpi_IsExtdMsgEcCtrlEnabled) (ptrPdStackContext->ptrHpiContext, ptrPdStackContext->port) != false)
        {
            return false;
        }
#endif /* (CY_HPI_PD_ENABLE) */
    }
#endif /* ((CY_HPI_ENABLED) && (CY_HPI_RW_PD_RESP_MSG_DATA)) */

    /* If this is a chunked message which is not complete, send another chunk request. */
    if ((pd_pkt_p->hdr.hdr.chunked == true) && (pd_pkt_p->hdr.hdr.dataSize >
                ((pd_pkt_p->hdr.hdr.chunkNum + 1u) * CY_PD_MAX_EXTD_MSG_LEGACY_LEN)))
    {
        cy_stc_pdstack_dpm_pd_cmd_buf_t extd_dpm_buf;

        extd_dpm_buf.cmdSop = (cy_en_pd_sop_t)pd_pkt_p->sop;
        extd_dpm_buf.extdType = (cy_en_pdstack_extd_msg_t) pd_pkt_p->msg;
        extd_dpm_buf.extdHdr.val = 0u;
        extd_dpm_buf.extdHdr.extd.chunked = true;
        extd_dpm_buf.extdHdr.extd.request = true;
        extd_dpm_buf.extdHdr.extd.chunkNum = pd_pkt_p->hdr.hdr.chunkNum + 1u;
        extd_dpm_buf.datPtr = (uint8_t*)&glExtdData;
        extd_dpm_buf.timeout = ptrPdStackContext->senderRspTimeout;

        /* Send next chunk request */
        Cy_PdStack_Dpm_SendPdCommand(ptrPdStackContext,CY_PDSTACK_DPM_CMD_SEND_EXTENDED,
                &extd_dpm_buf, true, extd_msg_cb); 
    }
    else if(pd_pkt_p->msg == CY_PDSTACK_EXTD_MSG_STATUS)
    {
        if (pd_pkt_p->sop == CY_PD_SOP)
        {
            /* Gets the power state change (6th byte) from extended status messages */
            uint8_t powerStateChange = (uint8_t)(pd_pkt_p->dat[1].val >> 16u) & CY_PD_EXTD_STATUS_PWR_STATE_CHANGE_MASK;
            ptrPdStackContext->dpmExtStat.pwrLed = ((powerStateChange & CY_PD_PD_EXTD_STATUS_PWR_LED_MASK ) >> CY_PD_PD_EXTD_STATUS_PWR_LED_POS);

            /* Return false for solution handling of power LED. */
            return false;
        }
    }
    else
    {
        /*
         * Don't send any response to response messages. Handling here instead of in the stack so that
         * these messages can be used for PD authentication implementation.
         */
        if ((pd_pkt_p->msg != CY_PDSTACK_EXTD_MSG_SECURITY_RESP) && (pd_pkt_p->msg != CY_PDSTACK_EXTD_MSG_FW_UPDATE_RESP))
        {
            /* Send not supported message */
            Cy_PdStack_Dpm_SendPdCommand(ptrPdStackContext,
                    CY_PDSTACK_DPM_CMD_SEND_NOT_SUPPORTED, NULL, true, NULL); 
        }
    }

    return true;
}
#endif /* CY_PD_REV3_ENABLE */

#if (CY_APP_ROLE_PREFERENCE_ENABLE)
static void app_role_swap_resp_cb (cy_stc_pdstack_context_t *ptrPdStackContext, 
        cy_en_pdstack_resp_status_t resp,
        const cy_stc_pdstack_pd_packet_t *pkt_ptr)
{
    uint8_t port = ptrPdStackContext->port;
    cy_stc_app_status_t *app_stat = &glAppStatus[port];

#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
    bool next_swap = false;
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */

    if (resp == CY_PDSTACK_RES_RCVD)
    {
        if (pkt_ptr->hdr.hdr.msgType == CY_PD_CTRL_MSG_WAIT)
        {
            app_stat->actv_swap_count++;
            if (app_stat->actv_swap_count < CY_APP_MAX_SWAP_ATTEMPT_COUNT)
            {
                Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                        CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_INITIATE_SWAP_TIMER), CY_APP_SWAP_WAIT_TIMER_PERIOD, app_initiate_swap);
            }
            else
            {
#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
                /* Swap attempts timed out. Proceed with next swap. */
                next_swap = true;
#else
                app_stat->app_pending_swaps = 0u;
                app_stat->actv_swap_type  = 0u;
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */
            }
        }
        else
        {
#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
            /* Swap succeeded or failed. Proceed with next swap. */
            next_swap = true;
#else
            app_stat->app_pending_swaps = 0u;
            app_stat->actv_swap_type  = 0u;
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */
        }
    }
    else if ((resp == CY_PDSTACK_CMD_FAILED) || (resp == CY_PDSTACK_SEQ_ABORTED) || (resp == CY_PDSTACK_RES_TIMEOUT))
    {
        Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_INITIATE_SWAP_TIMER), app_stat->actv_swap_delay, app_initiate_swap);
    }

#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
    if (next_swap)
    {
        /* Clear the LS bit in the app_pending_swaps flag as it has completed or failed. */
        app_stat->app_pending_swaps &= (app_stat->app_pending_swaps - 1);

        app_stat->actv_swap_type  = 0u;
        app_stat->actv_swap_count = 0u;
        Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_INITIATE_SWAP_TIMER),
                CY_APP_INITIATE_DR_SWAP_TIMER_PERIOD, app_initiate_swap);
    }
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */
}

static void app_initiate_swap (cy_timer_id_t id, void *context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;
    uint8_t port = ptrPdStackContext->port;
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
    cy_stc_pdaltmode_context_t *ptrAltModeContext = ptrPdStackContext->ptrAltModeContext;
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */

    cy_stc_app_status_t *app_stat_p = Cy_App_GetStatus(port);

    cy_stc_pdstack_dpm_pd_cmd_buf_t pd_cmd_buf;

    uint8_t actv_swap     = app_stat_p->actv_swap_type;
    uint8_t swaps_pending = app_stat_p->app_pending_swaps;

    (void)id;

    /* Stop the timer that triggers swap operation */
    Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_INITIATE_SWAP_TIMER));

    /* Nothing to do if we are not in PD contract */
    if (!ptrPdStackContext->dpmConfig.contractExist)
        return;

#if ((CY_PD_REV3_ENABLE) && (CY_APP_GET_REVISION_ENABLE))
    /* Defer swap initiation if get revision handling is not completed. */
    if((glAppGetRevSendStatus[ptrPdStackContext->port] != true) && ((ptrPdStackContext->dpmConfig.specRevSopLive >= CY_PD_REV3)))
    {
        Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_INITIATE_SWAP_TIMER),
                CY_APP_GET_REV_PD_CMD_RETRY_TIMER_PERIOD, app_initiate_swap);

        return;
    }
#endif /* ((CY_PD_REV3_ENABLE) && (CY_APP_GET_REVISION_ENABLE)) */

    if (actv_swap == 0)
    {
#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
        /* No ongoing swap operation. Pick the next pending swap from the list. */
        if ((swaps_pending & CY_APP_VCONN_SWAP_PENDING) != 0u)
        {
            actv_swap = CY_PDSTACK_DPM_CMD_SEND_VCONN_SWAP;
            app_stat_p->actv_swap_delay = CY_APP_INITIATE_DR_SWAP_TIMER_PERIOD;
        }
        else
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */
        {
            if ((swaps_pending & CY_APP_DR_SWAP_PENDING) != 0u)
            {
                actv_swap = CY_PDSTACK_DPM_CMD_SEND_DR_SWAP;
                app_stat_p->actv_swap_delay = CY_APP_INITIATE_DR_SWAP_TIMER_PERIOD;
            }
#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
            else
            {
                if (swaps_pending != 0u)
                {
                    actv_swap = CY_PDSTACK_DPM_CMD_SEND_PR_SWAP;
                    app_stat_p->actv_swap_delay = CY_APP_INITIATE_PR_SWAP_TIMER_PERIOD;
                }
            }
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */
        }

        app_stat_p->actv_swap_count = 0u;
    }

    if (actv_swap != 0u)
    {
        /* Check whether the selected swap is still valid */
        switch (actv_swap)
        {
#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
            case CY_PDSTACK_DPM_CMD_SEND_VCONN_SWAP:
                if (ptrPdStackContext->dpmConfig.vconnLogical)
                {
                    app_stat_p->app_pending_swaps &= ~CY_APP_VCONN_SWAP_PENDING;
                    actv_swap = 0u;
                }
                break;
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */

            case CY_PDSTACK_DPM_CMD_SEND_DR_SWAP:
                /* Stop sending DR_SWAP if any alternate mode has been entered */
                if
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
                (
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
                        (ptrPdStackContext->dpmConfig.curPortType == glAppPrefDataRole[port]) 
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
                        ||
                        (ptrAltModeContext->altModeAppStatus->altModeEntered != 0u)
                )
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
                {
                    app_stat_p->app_pending_swaps &= ~CY_APP_DR_SWAP_PENDING;
                    actv_swap = 0u;
                }
                break;

#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
            case CY_PDSTACK_DPM_CMD_SEND_PR_SWAP:
                if (ptrPdStackContext->dpmConfig.curPortRole == glAppPrefPowerRole[port])
                {
                    app_stat_p->app_pending_swaps &= ~CY_APP_PR_SWAP_PENDING;
                    actv_swap = 0u;
                }
                break;
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */

            default:
                actv_swap = 0u;
                break;
        }

        if (actv_swap == 0u)
        {
            /*
             * Currently selected SWAP is no longer relevant. Re-run function to identify the next swap to be
             * performed.
             */
            if (app_stat_p->app_pending_swaps != 0u)
            {
                app_stat_p->actv_swap_type = 0u;
                Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                        CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_INITIATE_SWAP_TIMER),
                        CY_APP_INITIATE_DR_SWAP_TIMER_PERIOD, app_initiate_swap);
            }
        }
        else
        {
            /* Store the swap command for use in the callback */
            app_stat_p->actv_swap_type = actv_swap;

            /* Only packet type needs to be set when initiating swap operations */
            pd_cmd_buf.cmdSop = CY_PD_SOP;

            /* Try to trigger the selected swap operation */
            if (Cy_PdStack_Dpm_SendPdCommand(ptrPdStackContext, (cy_en_pdstack_dpm_pd_cmd_t)actv_swap, &pd_cmd_buf,
                        false, app_role_swap_resp_cb) != CY_PDSTACK_STAT_SUCCESS)
            {
                /* Retries in case of AMS failure can always be done with a small delay */
                Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                        CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_INITIATE_SWAP_TIMER),
                        CY_APP_INITIATE_DR_SWAP_TIMER_PERIOD, app_initiate_swap);
            }
        }
    }
}

/* This function is called at the end of a PD contract to check whether any role swaps need to be triggered */
void Cy_App_ContractHandler (cy_stc_pdstack_context_t *ptrPdStackContext)
{
    cy_stc_app_status_t *app_stat = &glAppStatus[ptrPdStackContext->port];

    uint16_t delay_reqd = CY_APP_INITIATE_PR_SWAP_TIMER_PERIOD;

    uint8_t port = ptrPdStackContext->port;
#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
    /* Check if we need to go ahead with PR-SWAP */
    if (
            (glAppPrefPowerRole[port] == CY_PD_PRT_DUAL) ||
            (ptrPdStackContext->dpmConfig.curPortRole == glAppPrefPowerRole[port])
       )
    {
        app_stat->app_pending_swaps &= ~CY_APP_PR_SWAP_PENDING;
    }
    else
    {
        /* If we are about to swap to become source, ensure VConn Swap is done as required. */
        if ((glAppStatus[port].app_pending_swaps & CY_APP_PR_SWAP_PENDING) &&
                (glAppPrefPowerRole[port] == CY_PD_PRT_ROLE_SOURCE) &&
                (ptrPdStackContext->dpmConfig.vconnLogical == 0))
        {
            app_stat->app_pending_swaps |= CY_APP_VCONN_SWAP_PENDING;
        }
    }
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */

    /* Check if we need to go ahead with DR-SWAP. */
    if (
            (glAppPrefDataRole[port] == CY_PD_PRT_TYPE_DRP) ||
            (ptrPdStackContext->dpmConfig.curPortType == glAppPrefDataRole[port])
       )
    {
        app_stat->app_pending_swaps &= ~CY_APP_DR_SWAP_PENDING;
    }
    else
    {
        /* Delay DR_SWAP to finish EPR mode entry */
        /* DR-SWAPs need to be initiated as soon as possible. VConn swap will be triggered after DR_SWAP as needed. */
        delay_reqd = CY_APP_INITIATE_DR_SWAP_TIMER_PERIOD;
    }

    /* Start a timer that will kick off the swap state machine. */
    Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
            CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_INITIATE_SWAP_TIMER), delay_reqd, app_initiate_swap);
}

void Cy_App_ConnectChangeHandler (cy_stc_pdstack_context_t *ptrPdStackContext)
{
    /* Stop all timers used to trigger swap operations. */
    Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_INITIATE_SWAP_TIMER));
    uint8_t port = ptrPdStackContext->port;

#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
    /* Assume that PR_SWAP and DR_SWAP are pending. The actual status will be updated on contract completion. */
    glAppStatus[port].app_pending_swaps = CY_APP_PR_SWAP_PENDING | CY_APP_DR_SWAP_PENDING;
#else
    /* Assume that DR_SWAP is pending. The actual status will be updated on contract completion. */
    glAppStatus[port].app_pending_swaps = CY_APP_DR_SWAP_PENDING;
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */

    glAppStatus[port].actv_swap_type    = 0u;
    glAppStatus[port].actv_swap_count   = 0u;
}

#endif /* (CY_APP_ROLE_PREFERENCE_ENABLE) */

#if (CY_PD_EPR_ENABLE && (!CY_PD_SOURCE_ONLY))
void epr_enter_mode_timer_cb (
        cy_timer_id_t id,
        void *ptrContext)
{
    (void)id;
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)ptrContext;

    /* Check if the conditions for EPR entry are still valid. */
    if (ptrPdStackContext->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SINK)
    {
        if (Cy_PdStack_Dpm_SendPdCommand (ptrPdStackContext, CY_PDSTACK_DPM_CMD_SNK_EPR_MODE_ENTRY, NULL, false, NULL) != CY_PDSTACK_STAT_SUCCESS)
        {
            /* Retry the EPR Entry. */
            Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                    CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_EPR_MODE_TIMER), CY_APP_EPR_SNK_ENTRY_TIMER_PERIOD, epr_enter_mode_timer_cb);
        }
    }
}
#endif /* CY_PD_EPR_ENABLE && (!CY_PD_SOURCE_ONLY) */

#if ((!CY_PD_SINK_ONLY) && (!CY_PD_DEBUG_ACC_DISABLE))
/* Dummy callback used to ensure VBus discharge happens after debug accessory sink is disconnected */
static void debug_acc_src_disable_cbk(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    (void)ptrPdStackContext;
}

static void debug_acc_src_psrc_enable(cy_timer_id_t id, void * context)
{
    (void)id;

    cy_stc_pdstack_context_t *ptrPdStackContext =
        (cy_stc_pdstack_context_t*) context;

    ptrPdStackContext->ptrAppCbk->psrc_enable(ptrPdStackContext, NULL);
}
#endif /* ((!CY_PD_SINK_ONLY) && (!CY_PD_DEBUG_ACC_DISABLE)) */

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
#if RIDGE_SLAVE_ENABLE
static void app_check_usb_supp(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    const cy_stc_pdstack_dpm_status_t *dpm_stat = &ptrPdStackContext->dpmStat;

    cy_stc_pdaltmode_context_t* ptrAltModeContext = ptrPdStackContext->ptrAltModeContext;

    /* Check port partner's USB capabilities */
    if (
#if (!CY_PD_SINK_ONLY)
            ((dpm_stat->portRole == CY_PD_PRT_ROLE_SOURCE)                 &&
             ((dpm_stat->srcRdo.rdo_gen.usbCommCap == false)           ||
              (dpm_stat->curSrcPdo[0].fixed_src.usbCommCap == false))) ||
            ((dpm_stat->portRole == CY_PD_PRT_ROLE_SINK)                   &&
             ((dpm_stat->snkRdo.rdo_gen.usbCommCap == false)           ||
              (dpm_stat->srcCapP->dat[0].fixed_src.usbCommCap == false)))
#else
            ((dpm_stat->snkRdo.rdo_gen.usbCommCap == false)           ||
             (dpm_stat->srcCapP->dat[0].fixed_src.usbCommCap == false))
#endif /* (!CY_PD_SINK_ONLY) */
       )
    {
        ptrAltModeContext->altModeAppStatus->usb2Supp = false;
        ptrAltModeContext->altModeAppStatus->usb3Supp = false;
    }
}
#endif /* RIDGE_SLAVE_ENABLE */
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

#if (MUX_DELAY_EN)
void app_hard_rst_retimer_cbk (cy_timer_id_t id, void* ptrContext)
{
    cy_stc_pdstack_context_t * ptrPdStackContext = (cy_stc_pdstack_context_t *) ptrContext;

    Cy_App_EventHandler(ptrPdStackContext, APP_EVT_HR_SENT_RCVD_DEFERRED, NULL);

    (void)id;
}
#endif /* (MUX_DELAY_EN) */

#if (CY_CORROSION_MITIGATION_ENABLE && CY_APP_MOISTURE_DETECT_IN_ATTACH_ENABLE)
static void app_moisture_detect_tmr_cbk(cy_timer_id_t id,  void * callbackCtx)
{
    (void)id;
    cy_stc_pdstack_context_t *ptrPdStackContext = callbackCtx;
    Cy_App_MoistureDetect_Run(ptrPdStackContext);
}
#endif /* (CY_CORROSION_MITIGATION_ENABLE  && CY_APP_MOISTURE_DETECT_IN_ATTACH_ENABLE) */

void Cy_App_EventHandler(cy_stc_pdstack_context_t *ptrPdStackContext, 
        cy_en_pdstack_app_evt_t evt, const void* dat)
{
    uint8_t port = ptrPdStackContext->port;
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
    cy_stc_pdaltmode_context_t* ptrAltModeContext = ptrPdStackContext->ptrAltModeContext;
#endif

    const cy_en_pdstack_app_req_status_t* result;
    const cy_stc_pdstack_pd_contract_info_t* contract_status;
    bool  skip_soln_cb = false;
    bool  hardreset_cplt = false;
    bool  typec_only = false;

#if CY_PD_REV3_ENABLE
#if (CY_APP_HOST_ALERT_MSG_DISABLE != 1)
    cy_pd_pd_do_t alert_ado;
#endif /* (CY_APP_HOST_ALERT_MSG_DISABLE != 1) */
#endif /* CY_PD_REV3_ENABLE */

    switch(evt)
    {
        case APP_EVT_TYPEC_STARTED:

            CY_APP_DEBUG_LOG(ptrPdStackContext->port, (ptrPdStackContext->port == 0 ? CY_APP_DEBUG_PD_P0_PORT_ENABLE : CY_APP_DEBUG_PD_P1_PORT_ENABLE), NULL, 0, CY_APP_DEBUG_LOGLEVEL_INFO, true);
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
            /* Initialize the MUX to its default settings (isolate) */
            Cy_PdAltMode_HW_MuxCtrlInit (ptrAltModeContext);
            ptrAltModeContext->altModeAppStatus->vdmPrcsFailed = false;
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

#if PMG1_V5V_CHANGE_DETECT
            /* If V5V supply is not present, set flag to indicate it is not present. */
            if (!Cy_USBPD_V5V_IsSupplyOn (ptrPdStackContext->ptrUsbPdContext))
            {
                Cy_App_GetPdAppStatus(port)->faultStatus |= CY_APP_PORT_V5V_SUPPLY_LOST;
            }
#endif /* PMG1_V5V_CHANGE_DETECT */

            break;

        case APP_EVT_TYPEC_ATTACH:

            CY_APP_DEBUG_LOG(ptrPdStackContext->port, (ptrPdStackContext->port == 0 ? CY_APP_DEBUG_PD_P0_TYPEC_ATTACH : CY_APP_DEBUG_PD_P1_TYPEC_ATTACH), NULL, 0, CY_APP_DEBUG_LOGLEVEL_INFO, true);

#if (!CY_PD_SINK_ONLY)
            /*
             * Check and disable VBUS discharge if we had enabled discharge due to invalid VBUS voltage.
             * Since the check is already there inside the function, need not repeat it here.
             */
            app_psrc_invalid_vbus_dischg_disable(ptrPdStackContext);
#endif /* (!CY_PD_SINK_ONLY) */

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
#if RIDGE_SLAVE_ENABLE
            /* Set USB2/USB3 connection bits to 1 by default */
            ptrAltModeContext->altModeAppStatus->usb2Supp = true;
            ptrAltModeContext->altModeAppStatus->usb3Supp = true;
#endif /* RIDGE_SLAVE_ENABLE */

            /* Update the MUX configuration for DFP; For UFPs, it will be done later at CONNECT event. */
            if (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_DFP)
            {
                /* This will also enable the USB (DP/DM) MUX where required */
                Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, 0, 0);
            }
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

            /* Clear all fault counters if we have seen a change in polarity from previous connection */
            if (ptrPdStackContext->dpmConfig.polarity != glAppPrevPolarity[port])
            {
                Cy_App_Fault_ClearCounts (ptrPdStackContext->port);
            }
            glAppPrevPolarity[port] = ptrPdStackContext->dpmConfig.polarity ;
#if (CY_PD_EPR_ENABLE && (!CY_PD_SOURCE_ONLY))
            glAppResetEpr[port] = false;
#endif /* CY_PD_EPR_ENABLE && (!CY_PD__SOURCE_ONLY) */
            break;

        case APP_EVT_CONNECT:

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
            ptrAltModeContext->altModeAppStatus->vdmPrcsFailed = false;

#if (!CCG_CBL_DISC_DISABLE)
            ptrAltModeContext->altModeAppStatus->cblDiscIdFinished = false;
            ptrAltModeContext->altModeAppStatus->discCblPending = false;
#endif /* (!CCG_CBL_DISC_DISABLE) */
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

#if ((!CY_PD_SINK_ONLY) && (!CY_PD_DEBUG_ACC_DISABLE))
            if (ptrPdStackContext->dpmConfig.attachedDev == CY_PD_DEV_DBG_ACC)
            {
                if(ptrPdStackContext->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SOURCE)
                {
                    uint16_t mux_wait_delay = ptrPdStackContext->ptrDpmParams->muxEnableDelayPeriod;

                    if (mux_wait_delay == 0u)
                    {
                        /* If the delay is 0 then changing it to 1, so that after 1 ms cb is called to apply VBus.*/
                        mux_wait_delay = 1u;
                    }
                    Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                            CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_PSOURCE_EN_TIMER),
                            mux_wait_delay, debug_acc_src_psrc_enable);
                }

                glAppStatus[port].debug_acc_attached = true;

#if ((DEBUG_ACCESSORY_SNK_ENABLE) || (DEBUG_ACCESSORY_SRC_ENABLE))
                if (
#if !DEBUG_ACCESSORY_SRC_ENABLE
                        (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_ROLE_SINK)
#elif !DEBUG_ACCESSORY_SNK_ENABLE
                        (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_ROLE_SOURCE)
#else /* DEBUG_ACCESSORY_SRC_ENABLE == 1 && DEBUG_ACCESSORY_SNK_ENABLE == 1 */
                        (1)
#endif /* !DEBUG_ACCESSORY_SRC_ENABLE */
                   )
                {
#if (RIDGE_SLAVE_ENABLE)
                    /* Set USB2/USB3 connection bits to 1 in case of debug accessory by default.
                     * Added here because APP_EVT_TYPEC_ATTACH event is not occurring for debug accessory.
                     */
                    ptrAltModeContext->altModeAppStatus->usb2Supp = true;
                    ptrAltModeContext->altModeAppStatus->usb3Supp = true;

#endif /* (RIDGE_SLAVE_ENABLE) */

                    Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, 0, 0);

                }
#endif /* ((DEBUG_ACCESSORY_SNK_ENABLE) || (DEBUG_ACCESSORY_SRC_ENABLE)) */
            }
            else
#endif /* ((!CY_PD_SINK_ONLY) && (!CY_PD_DEBUG_ACC_DISABLE)) */
            {
#if ((UFP_ALT_MODE_SUPP) || (DFP_ALT_MODE_SUPP))
#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
                if (Cy_App_isUfpUsb4Supp(ptrAltModeContext) != false)
                {
                    /* Set Isolate state for UFP connection. */
                    Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_ISOLATE, 0, 0);
                }
                else
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */
                {
                    /* Update the data path for UFP mode */
                    if(ptrPdStackContext->dpmConfig.curPortType != CY_PD_PRT_TYPE_DFP)
                    {
                        /* This will also enable the USB (DP/DM) MUX where required. */
                        Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, 0, 0);
                    }
                }
#endif /* ((UFP_ALT_MODE_SUPP) || (DFP_ALT_MODE_SUPP)) */
            }

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)

#if (CCG_BB_ENABLE != 0)
            /* Enable the AME timer on attach if in sink mode */
            if (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP)
            {
                if (Cy_App_Usb_BbBindToPort(ptrAltModeContext) == CY_PDALTMODE_BILLBOARD_STAT_SUCCESS)
                {
                    /* Start the AME timer in any case as the MUX needs to be setup in the callback in some cases */
                    Cy_PdUtils_SwTimer_Start (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                            CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_AME_TIMEOUT_TIMER), CY_PDALTMODE_APP_AME_TIMEOUT_TIMER_PERIOD, ame_tmr_cbk);

                    /* Leave self power status flag cleared as we do not have a PD contract in place */
                    Cy_App_Usb_BbUpdateSelfPwrStatus (ptrAltModeContext, 0u);
                }
            }
#endif /* (CCG_BB_ENABLE != 0) */

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
            if (
                   (TIMER_CALL_MAP(Cy_PdUtils_SwTimer_IsRunning)(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_AME_TIMEOUT_TIMER)) == false) &&
                   (Cy_App_isUfpUsb4Supp(ptrAltModeContext) != false)
               )
            {
                /* Start the USB4 Timeout timer to postpone USB2/3 exposing. */
                TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                    CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_AME_TIMEOUT_TIMER), CY_PDALTMODE_APP_USB4_TIMEOUT_TIMER_PERIOD, usb4_tmr_cbk);
            }
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */

#if (CY_APP_ROLE_PREFERENCE_ENABLE)
            Cy_App_ConnectChangeHandler (ptrPdStackContext);
#endif /* (CY_APP_ROLE_PREFERENCE_ENABLE) */

#if CY_APP_USB_ENABLE && (CCG_BB_ENABLE == 0)
            Cy_App_Usb_Enable();
#endif /* CY_APP_USB_ENABLE && (CCG_BB_ENABLE == 0) */
            break;

        case APP_EVT_HARD_RESET_COMPLETE:
            hardreset_cplt = true;
            /* Intentional fall through to the next case */
            // fall through
        case APP_EVT_HR_SENT_RCVD_DEFERRED:
        case APP_EVT_HARD_RESET_SENT:
        case APP_EVT_PE_DISABLED:
            if(evt == APP_EVT_HARD_RESET_SENT)
            {
                CY_APP_DEBUG_LOG(ptrPdStackContext->port, (ptrPdStackContext->port == 0 ? CY_APP_DEBUG_PD_P0_HARD_RST_TX : CY_APP_DEBUG_PD_P1_HARD_RST_TX), NULL, 0, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            }
            typec_only = ((ptrPdStackContext->dpmStat.pdConnected == false) || (evt == APP_EVT_PE_DISABLED));
            /* Intentional fall through to the next case */
            // fall through
        case APP_EVT_HARD_RESET_RCVD:
        case APP_EVT_VBUS_PORT_DISABLE:
        case APP_EVT_DISCONNECT:
        case APP_EVT_TYPE_C_ERROR_RECOVERY:

#if (CY_PD_EPR_ENABLE && (!CY_PD_SOURCE_ONLY))
            Cy_PdUtils_SwTimer_Stop (ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_EPR_MODE_TIMER));
#endif /* (CY_PD_EPR_ENABLE && (!CY_PD_SOURCE_ONLY)) */
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))

#if (!CCG_CBL_DISC_DISABLE)
            ptrAltModeContext->altModeAppStatus->cblDiscIdFinished = false;
            ptrAltModeContext->altModeAppStatus->discCblPending = false;
#endif /* (!CCG_CBL_DISC_DISABLE) */
            /* Reset discovery related info */
            app_reset_disc_info(ptrAltModeContext);
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */
            if(evt == APP_EVT_DISCONNECT)
            {
#if ((CY_PD_REV3_ENABLE) && (CY_APP_GET_REVISION_ENABLE))
                /* Reset the retry count for PD Get_Revision AMS */
                glAppGetRevRetry[ptrPdStackContext->port] = 0x00u;

                glAppGetRevSendStatus[ptrPdStackContext->port] = false;
#endif /* ((CY_PD_REV3_ENABLE) && (CY_APP_GET_REVISION_ENABLE)) */
#if CY_APP_USB_ENABLE && (CCG_BB_ENABLE == 0)
                Cy_App_Usb_Disable();
#endif /* ((CY_PD_REV3_ENABLE) && (CY_APP_GET_REVISION_ENABLE)) */
                CY_APP_DEBUG_LOG(ptrPdStackContext->port, (ptrPdStackContext->port == 0 ? CY_APP_DEBUG_PD_P0_TYPEC_DETACH : CY_APP_DEBUG_PD_P1_TYPEC_DETACH), NULL, 0, CY_APP_DEBUG_LOGLEVEL_INFO, true);
            }

            if(evt == APP_EVT_HARD_RESET_RCVD)
            {
                CY_APP_DEBUG_LOG(ptrPdStackContext->port, (ptrPdStackContext->port == 0 ? CY_APP_DEBUG_PD_P0_HARD_RST_RX : CY_APP_DEBUG_PD_P1_HARD_RST_RX), NULL, 0, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            }
            else if(evt == APP_EVT_VBUS_PORT_DISABLE)
            {
                CY_APP_DEBUG_LOG(ptrPdStackContext->port, (ptrPdStackContext->port == 0 ? CY_APP_DEBUG_PD_P0_PORT_DISABLE : CY_APP_DEBUG_PD_P1_PORT_DISABLE), NULL, 0, CY_APP_DEBUG_LOGLEVEL_INFO, true);
            }
            else
            {
                /* Do nothing */
            }

            if (
                    (evt == APP_EVT_HARD_RESET_SENT) ||
                    (evt == APP_EVT_HARD_RESET_RCVD) ||
                    (evt == APP_EVT_HR_SENT_RCVD_DEFERRED)
               )
            {
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
#if MUX_DELAY_EN
                if(ptrAltModeContext->altModeAppStatus->isMuxBusy == false)
#endif /* MUX_DELAY_EN */
                {
#if RIDGE_SLAVE_ENABLE
                    /* Drop all previous MUX status updates and update the status as 0 (no data connection) */
                    Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_DEINIT, 0, 0);
#endif /* RIDGE_SLAVE_ENABLE */
                }
#if MUX_DELAY_EN && ICL_SLAVE_ENABLE
                else
                {
                    /* Run MUX delay timer */
                    Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext, CY_PDALTMODE_ICL_HARD_RST_TIMER),
                            Cy_PdUtils_SwTimer_GetCount(ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_MUX_DELAY_TIMER)) + 2,
                            app_hard_rst_retimer_cbk);
                    break;
                }
#endif /* MUX_DELAY_EN */
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */
            }

            if (evt == APP_EVT_DISCONNECT)
            {
#if ((!CY_PD_SINK_ONLY) && (!CY_PD_DEBUG_ACC_DISABLE))
                if(
                        (false != glAppStatus[port].debug_acc_attached)
                        &&
                        (ptrPdStackContext->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SOURCE)
                  )
                {
                    ptrPdStackContext->ptrAppCbk->psrc_disable(ptrPdStackContext, debug_acc_src_disable_cbk);
                }

                /* Mark debug accessory detached */
                glAppStatus[port].debug_acc_attached = false;
#endif /* ((!CY_PD_SINK_ONLY) && (!CY_PD_DEBUG_ACC_DISABLE)) */
#if (RIDGE_SLAVE_ENABLE)

                /* Set USB2/USB3 connection bits to 1 by default */
                ptrAltModeContext->altModeAppStatus->usb2Supp = false;
                ptrAltModeContext->altModeAppStatus->usb3Supp = false;

#endif /* (RIDGE_SLAVE_ENABLE) */
            }

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
#if (STORE_DETAILS_OF_HOST)
            if (port == TYPEC_PORT_0_IDX)
            {
                ptrAltModeContext->hostDetails->isHostDetailsAvailable = false;
            }
#endif

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
            ptrAltModeContext->altModeAppStatus->usb4Active = false;
            ptrAltModeContext->altModeAppStatus->usb4DataRstCnt = 0u;
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */

            Cy_PdAltMode_VdmTask_MngrDeInit (ptrAltModeContext);

#if CY_PD_DP_VCONN_SWAP_FEATURE
            Cy_PdUtils_SwTimer_Stop (ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_CBL_DISC_TRIGGER_TIMER));
#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */

#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */

            if (hardreset_cplt)
            {
#if (CY_PD_EPR_ENABLE && (!CY_PD_SOURCE_ONLY))
                glAppResetEpr[port] = false;
#endif /* CY_PD_EPR_ENABLE && (!CY_PD__SOURCE_ONLY) */
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
#if GATKEX_CREEK
                /* Update US port with disconnect status upon receiving hard reset */
                if ((port == TYPEC_PORT_0_IDX) && (evt != APP_EVT_PE_DISABLED)) /* Disconnect on port A */
                {
                    Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_ISOLATE, 0u, 0u);
                }
#endif /* GATKEX_CREEK */

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)

                if (Cy_App_isUfpUsb4Supp(ptrAltModeContext) != false)
                {
                    /* Set Isolate state for UFP connection. */
                    Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_ISOLATE, 0, 0);

                    /* Start the USB4 Timeout timer to postpone USB2/3 exposing. */
                    TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Start) (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                            CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_AME_TIMEOUT_TIMER),
                            CY_PDALTMODE_APP_USB4_TIMEOUT_TIMER_PERIOD, usb4_tmr_cbk);
                }
                else
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */
                {
                    Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, 0, 0);
                }
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
            }
            else
            {
                /*
                 * Isolate the data lines if this is a PD connection
                 */
                if (!typec_only)
                {
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
#if !RIDGE_SLAVE_ENABLE
                    Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_ISOLATE, 0u, 0u);
#endif /* !RIDGE_SLAVE_ENABLE */

#if (CCG_BB_ENABLE != 0)
                    Cy_PdUtils_SwTimer_Stop (ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_AME_TIMEOUT_TIMER));
#endif /* (CCG_BB_ENABLE != 0) */

#if RIDGE_SLAVE_ENABLE
                    Cy_PdAltMode_HW_SetMux(ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_RIDGE_CUSTOM, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);
#endif /* RIDGE_SLAVE_ENABLE */
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
                }
            }
#if (VBUS_OCP_ENABLE || VBUS_SCP_ENABLE || VBUS_OVP_ENABLE || VBUS_UVP_ENABLE)
            if(evt == APP_EVT_TYPE_C_ERROR_RECOVERY)
            {
                /* Clear port-in-fault flag if all fault counts are within limits */
                if (!Cy_App_Fault_IsCountExceeded(ptrPdStackContext))
                {
                    if ((Cy_App_GetPdAppStatus(port)->faultStatus & CY_APP_PORT_DISABLE_IN_PROGRESS) == 0)
                    {
                        ptrPdStackContext->dpmStat.faultActive = false;
                    }
                }
            }
#endif /* VBUS_OCP_ENABLE | VBUS_SCP_ENABLE | VBUS_OVP_ENABLE| VBUS_UVP_ENABLE */

            if ((evt == APP_EVT_DISCONNECT) || (evt == APP_EVT_VBUS_PORT_DISABLE))
            {
#if CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME
                /* Disable the regulator on port disconnect */
                REGULATOR_DISABLE(port);
#endif /* CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME */

#if VCONN_OCP_ENABLE
                /* Clear the VConn fault status */
                glAppPdStatus[port].faultStatus &= ~CY_APP_PORT_VCONN_FAULT_ACTIVE;
#endif /* VCONN_OCP_ENABLE */

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
#if CCG_BB_ENABLE
                if (evt == APP_EVT_DISCONNECT)
                {
                    Cy_PdAltMode_Billboard_Disable (ptrAltModeContext, true);

                    /* Clear self power status flag cleared on disconnect */
                    Cy_App_Usb_BbUpdateSelfPwrStatus (ptrAltModeContext, 0u);
                }
#endif /* CCG_BB_ENABLE */
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
            }

#if ((!CY_PD_SINK_ONLY) || (CY_APP_ROLE_PREFERENCE_ENABLE))
            if (
                    (evt == APP_EVT_HARD_RESET_COMPLETE) ||
                    (evt == APP_EVT_TYPE_C_ERROR_RECOVERY) ||
                    (evt == APP_EVT_DISCONNECT)
               )
            {
#if (CY_APP_ROLE_PREFERENCE_ENABLE)
                /* Stop the DR-Swap and PR-Swap trigger timers.  Assume that
                 * PR_SWAP and DR_SWAP are pending. The actual status will be
                 * updated on contract completion.
                 */
                Cy_App_ConnectChangeHandler (ptrPdStackContext);
#endif /* (CY_APP_ROLE_PREFERENCE_ENABLE) */
            }
#endif /* ((!CY_PD_SINK_ONLY) || (CY_APP_ROLE_PREFERENCE_ENABLE)) */

            break;

#if (!CCG_CBL_DISC_DISABLE)
        case APP_EVT_EMCA_NOT_DETECTED:
        case APP_EVT_EMCA_DETECTED:
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
            ptrAltModeContext->altModeAppStatus->cblDiscIdFinished = true;
            ptrAltModeContext->altModeAppStatus->discCblPending = false;
            ptrAltModeContext->altModeAppStatus->vdmPrcsFailed = false;
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */

#if DFP_ALT_MODE_SUPP
            /* Notify VDM discovery layer that it can proceed */
            Cy_PdAltMode_VdmTask_UpdateVcsStatus (ptrAltModeContext);
#endif /* DFP_ALT_MODE_SUPP */

            break;
#endif /* (!CCG_CBL_DISC_DISABLE) */

        case APP_EVT_DR_SWAP_COMPLETE:
            result = (const cy_en_pdstack_app_req_status_t*)dat ;
            if(*result == CY_PDSTACK_REQ_ACCEPT )
            {
#if (CY_APP_ROLE_PREFERENCE_ENABLE)
                glAppStatus[port].app_pending_swaps &= ~CY_APP_DR_SWAP_PENDING;
                if (glAppStatus[port].actv_swap_type == CY_PDSTACK_DPM_CMD_SEND_DR_SWAP)
                {
                    Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_INITIATE_SWAP_TIMER));
                    Cy_App_ContractHandler (ptrPdStackContext);
                }
#endif /* (CY_APP_ROLE_PREFERENCE_ENABLE) */

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))

                /* Reset discovery related info */
                app_reset_disc_info(ptrAltModeContext);

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)

                TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop) (ptrPdStackContext->ptrTimerContext,
                                        CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_AME_TIMEOUT_TIMER));

                ptrAltModeContext->altModeAppStatus->usb4Active = false;
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */

#if (RIDGE_SLAVE_ENABLE)
                /* Check if source and sink ports supports USB communication */
                app_check_usb_supp(ptrPdStackContext);
#endif /* (RIDGE_SLAVE_ENABLE)*/

                (void)Cy_PdAltMode_HW_SetMux(ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, CY_PDALTMODE_MNGR_NO_DATA, CY_PDALTMODE_MNGR_NO_DATA);

                /* Device data role changed. Reset alternate mode layer. */
                Cy_PdAltMode_Mngr_LayerReset(ptrAltModeContext);

#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */
            }
            break;

        case APP_EVT_VENDOR_RESPONSE_TIMEOUT:
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
            /* If the APP layer is going to retry the VDM, do not send the event. */
            if (ptrAltModeContext->altModeAppStatus->vdmRetryPending)
            {
                skip_soln_cb = true;
            }
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
            break;
        case APP_EVT_BAD_SINK_APDO_SEL:
            break;

        case APP_EVT_PD_CONTRACT_NEGOTIATION_COMPLETE:
            contract_status = (cy_stc_pdstack_pd_contract_info_t*)dat;

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
            /* Revise SVDM version only before start VDM processing */
            if (ptrAltModeContext->altModeAppStatus->vdmTaskEn == false)
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
            {
                /* Set VDM version based on active PD revision */
#if CY_PD_REV3_ENABLE
                if (ptrPdStackContext->dpmConfig.specRevSopLive >= CY_PD_REV3)
                {
                    glAppPdStatus[port].vdmVersion = CY_PDSTACK_STD_VDM_VER2;
                    /* Set minor VDM version if required */
#if MINOR_SVDM_VER_SUPPORT
                    glAppPdStatus[port].vdmMinorVersion = CY_PDSTACK_STD_VDM_MINOR_VER1;
#endif /* MINOR_SVDM_VER_SUPPORT */
                }
                else
#endif /* CY_PD_REV3_ENABLE */
                {
                    glAppPdStatus[port].vdmVersion = CY_PDSTACK_STD_VDM_VER1;
                    glAppPdStatus[port].vdmMinorVersion = CY_PDSTACK_STD_VDM_MINOR_VER0;
                }
            }

            if ((contract_status->status ==CY_PDSTACK_CONTRACT_NEGOTIATION_SUCCESSFUL) ||
                    (contract_status->status == CY_PDSTACK_CONTRACT_CAP_MISMATCH_DETECTED))
            {
#if (CY_APP_ROLE_PREFERENCE_ENABLE)
                Cy_App_ContractHandler (ptrPdStackContext);
#endif /* (CY_APP_ROLE_PREFERENCE_ENABLE) */

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
                /*
                 * Contract established. Enable VDM task manager for alt. mode support.
                 * This function will have no effect if the alt. modes are already running.
                 */
                if (
                        (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP) ||
#if (CY_APP_GET_REVISION_ENABLE)
                        /* Allow alternate mode tasks to run for DFP if in Rev2 */
                        ((ptrAltModeContext->altModeAppStatus->vdmPrcsFailed == false) &&
                                ((ptrPdStackContext->dpmConfig.specRevSopLive <= CY_PD_REV2) || (glAppGetRevSendStatus[ptrPdStackContext->port] == true)))
#else
                        /* Defer the alternate mode tasks for DFP until the get revision process is completed */
                        (ptrAltModeContext->altModeAppStatus->vdmPrcsFailed == false)
#endif /* (!CY_APP_GET_REVISION_ENABLE) */
                   )
                {
                    Cy_PdAltMode_VdmTask_Enable(ptrAltModeContext);
                }
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */
            }
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
#if (CCG_BB_ENABLE != 0)
            /* Now that there is a PD contract in place, we can set the self powered status flag. */
            Cy_App_Usb_BbUpdateSelfPwrStatus (ptrAltModeContext, 1);

            if (
                    (contract_status->status != CY_PDSTACK_CONTRACT_NEGOTIATION_SUCCESSFUL) &&
                    (ptrPdStackContext->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SINK) &&
                    (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP)
               )
            {
                Cy_PdAltMode_Billboard_Enable(ptrAltModeContext, CY_PDALTMODE_BILLBOARD_CAUSE_PWR_FAILURE);
            }
#endif /* (CCG_BB_ENABLE != 0) */

#if RIDGE_SLAVE_ENABLE
            /* Set USB2/USB3 connection bits to 1 by default*/
            ptrAltModeContext->altModeAppStatus->usb2Supp = true;
            ptrAltModeContext->altModeAppStatus->usb3Supp = true;

            /* Check if source and sink ports supports USB communication */
            app_check_usb_supp(ptrPdStackContext);

#endif /* RIDGE_SLAVE_ENABLE */
#endif /* (DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP) */

#if ((CY_PD_REV3_ENABLE) && (CY_APP_GET_REVISION_ENABLE))
            if((!(glAppGetRevSendStatus[ptrPdStackContext->port])) && (ptrPdStackContext->dpmConfig.specRevSopLive >= CY_PD_REV3))
            {
                if((Cy_PdUtils_SwTimer_IsRunning(ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_PD_GET_REVISION_COMMAND_RETRY_TIMER)) == false))
                {
                    /* Start the timer for sending the PD Get_Revision AMS */
                    Cy_PdUtils_SwTimer_Start (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                            CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_PD_GET_REVISION_COMMAND_RETRY_TIMER), CY_APP_GET_REV_PD_CMD_RETRY_TIMER_PERIOD, send_get_revision);
                }
            }
#endif /* ((CY_PD_REV3_ENABLE) && (CY_APP_GET_REVISION_ENABLE)) */

#if (CY_PD_EPR_ENABLE && (!CY_PD_SOURCE_ONLY))
            Cy_PdUtils_SwTimer_Stop (ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_EPR_MODE_TIMER));

            if((!glAppResetEpr[port]) && (ptrPdStackContext->dpmConfig.specRevSopLive >= CY_PD_REV3))
            {
                /* Start a timer to attempt EPR entry if the current role is sink */
                if ( (ptrPdStackContext->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SINK) &&
                        (ptrPdStackContext->dpmStat.srcCapP->dat[0].fixed_src.eprModeCapable == true) && 
                            (ptrPdStackContext->dpmExtStat.epr.snkEnable == true))
                {
                    Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                            CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_EPR_MODE_TIMER), CY_APP_EPR_SNK_ENTRY_TIMER_PERIOD, epr_enter_mode_timer_cb);

                    glAppResetEpr[port] = true;
                }
            }
#endif /* CY_PD_EPR_ENABLE && (!CY_PD_SOURCE_ONLY) */
        break;

#if CY_PD_REV3_ENABLE
        case APP_EVT_HANDLE_EXTENDED_MSG:
            {
                if(!(app_extd_msg_handler(ptrPdStackContext, (cy_stc_pd_packet_extd_t *)dat)))
                {
                    skip_soln_cb  = false;
                }
                else
                {
                    skip_soln_cb  = true;
                }
            }
            break;
#if (CY_APP_HOST_ALERT_MSG_DISABLE != 1)
        case APP_EVT_ALERT_RECEIVED:
            /* Respond to ALERT message only if there is the number of object is one */
            if (((cy_stc_pdstack_pd_packet_t*)dat)->len == 1u)
            {
                alert_ado = ((cy_stc_pdstack_pd_packet_t*)dat)->dat[0];
                if(alert_ado.ado_alert.batStatusChange == false)
                {
                    Cy_PdStack_Dpm_SendPdCommand(ptrPdStackContext, CY_PDSTACK_DPM_CMD_GET_STATUS,  NULL, true, NULL);
                }
                else
                {
                    uint8_t i = alert_ado.ado_alert.fixedBats |
                        (alert_ado.ado_alert.hotSwapBats << 4u);
                    cy_stc_pdstack_dpm_pd_cmd_buf_t cmd;

                    /* Identify the first battery for which the change is intended */
                    get_bat_status[port] = 0u;
                    while ((i != 0) && ((i & 0x01) == 0))
                    {
                        get_bat_status[port]++;
                        i >>= 1;
                    }

                    cmd.cmdSop = CY_PD_SOP;
                    cmd.extdHdr.val = 0x01u;
                    cmd.timeout = ptrPdStackContext->senderRspTimeout;
                    cmd.extdType = CY_PDSTACK_EXTD_MSG_GET_BAT_STATUS;
                    cmd.datPtr = (uint8_t*)&get_bat_status[port];
                    Cy_PdStack_Dpm_SendPdCommand(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SEND_EXTENDED,  &cmd, true, NULL);
                }
            }
            break;
#endif /* (CY_APP_HOST_ALERT_MSG_DISABLE != 1) */
#endif /* CY_PD_REV3_ENABLE */

        case APP_EVT_TYPEC_ATTACH_WAIT:
#if (CY_CORROSION_MITIGATION_ENABLE && CY_APP_MOISTURE_DETECT_IN_ATTACH_ENABLE)
            if(ptrPdStackContext->dpmStat.deadBat != true)
            {
                Cy_App_MoistureDetect_Run(ptrPdStackContext);
                Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_MOISTURE_DETECT_TIMER_ID),
                                         CY_APP_MOISTURE_DETECT_TIMER_PERIOD, app_moisture_detect_tmr_cbk);
            }
#endif /* (CY_CORROSION_MITIGATION_ENABLE && CY_APP_MOISTURE_DETECT_IN_ATTACH_ENABLE) */
#if CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME
            REGULATOR_ENABLE(port);
#endif /* CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME */
            break;

        case APP_EVT_TYPEC_ATTACH_WAIT_TO_UNATTACHED:
#if (!CY_PD_SINK_ONLY)
            /*
             * Check and disable VBUS discharge if we had enabled discharge due to invalid VBUS voltage.
             * Since the check is already there inside the function, need not repeat it here.
             */
            app_psrc_invalid_vbus_dischg_disable(ptrPdStackContext);
#endif /* (!CY_PD_SINK_ONLY) */

#if (CY_CORROSION_MITIGATION_ENABLE && CY_APP_MOISTURE_DETECT_IN_ATTACH_ENABLE)
            Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_MOISTURE_DETECT_TIMER_ID));
#endif /* (CY_CORROSION_MITIGATION_ENABLE && CY_APP_MOISTURE_DETECT_IN_ATTACH_ENABLE) */
#if CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME
            REGULATOR_DISABLE(port);
#endif /* CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME */
            break;

#if ((CY_APP_POWER_ROLE_PREFERENCE_ENABLE))
        case APP_EVT_PR_SWAP_COMPLETE:
#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
            glAppStatus[port].app_pending_swaps &= ~CY_APP_PR_SWAP_PENDING;
            if (glAppStatus[port].actv_swap_type == CY_PDSTACK_DPM_CMD_SEND_PR_SWAP)
            {
                Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_INITIATE_SWAP_TIMER));
                Cy_App_ContractHandler (ptrPdStackContext);
            }
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */
            break;
#endif /* ((CY_APP_POWER_ROLE_PREFERENCE_ENABLE)) */

        case APP_EVT_DATA_RESET_ACCEPTED:
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
            /* Need to reset alternate modes as part of Data_Reset. No status update is to be provided. */
            Cy_PdAltMode_Mngr_LayerReset (ptrAltModeContext);

#if (!CCG_CBL_DISC_DISABLE)
            ptrAltModeContext->altModeAppStatus->cblDiscIdFinished = false;
#endif /* (!CCG_CBL_DISC_DISABLE) */

#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
            /* Mark USB4 not active as data reset has been accepted */
            ptrAltModeContext->altModeAppStatus->usb4Active = false;

            if (Cy_App_isUfpUsb4Supp(ptrAltModeContext) != false)
            {
                /* Set Isolate state for UFP connection. */
                Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_ISOLATE, 0, 0);
            }
            /* Switch to USB only configuration once data reset has been accepted. */
            else
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */
            {
                /* Switch to USB only configuration once data reset has been accepted */
                if (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP)
                {
                    Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, 0u, 0u);
                }
                else
                {
                    Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_ISOLATE, 0u, 0u);
                }
            }

#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */
            break;

        case APP_EVT_DATA_RESET_CPLT:
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
            /* DFP needs to re-enable USBx connections at completion of Data_Reset */
            if (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_DFP)
            {
                Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, 0u, 0u);
            }
#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
            /* Enable the USB4 timeout timer on attach if in sink mode */
            if (Cy_App_isUfpUsb4Supp(ptrAltModeContext) != false)
            {
                /* Start the USB4 timeout timer to postpone USB2/3 exposing */
                Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                                     CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_AME_TIMEOUT_TIMER), CY_PDALTMODE_APP_USB4_TIMEOUT_TIMER_PERIOD, usb4_tmr_cbk);
            }
            else
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */
            {
                TIMER_CALL_MAP(Cy_PdUtils_SwTimer_Stop) (ptrPdStackContext->ptrTimerContext, CY_PDALTMODE_GET_TIMER_ID(ptrPdStackContext, CY_PDALTMODE_AME_TIMEOUT_TIMER));
            }
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */

            break;

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
        case APP_EVT_USB_ENTRY_CPLT:
            if (ptrPdStackContext->dpmConfig.curPortType == CY_PD_PRT_TYPE_UFP)
            {
                /* If enter USB is being accepted, compute the correct SoC status value and update the MUX. */
                cy_pd_pd_do_t eudo = *(cy_pd_pd_do_t *)dat;
                
                /* Disable USB4 timeout timer. */
                Cy_PdUtils_SwTimer_Stop(ptrAltModeContext->pdStackContext->ptrTimerContext,
                                    CY_PDALTMODE_GET_TIMER_ID(ptrAltModeContext->pdStackContext, CY_PDALTMODE_AME_TIMEOUT_TIMER));

                Cy_PdAltMode_Usb4_UpdateDataStatus (ptrAltModeContext, eudo, 0x00);
            }
            ptrAltModeContext->altModeAppStatus->usb4DataRstCnt = 0;
            ptrAltModeContext->altModeAppStatus->usb4Active = true;
            break;
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */

        case APP_EVT_PD_SINK_DEVICE_CONNECTED:
            break;

        case APP_EVT_PKT_RCVD:
            break;

        case APP_EVT_CBL_RESET_SENT:
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
            ptrAltModeContext->altModeAppStatus->cblRstDone = true;
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
            break;

#if (!CY_PD_SINK_ONLY)
        case APP_EVT_HR_PSRC_ENABLE:
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
            Cy_PdAltMode_HW_SetMux (ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SS_ONLY, 0u, 0u);
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
            break;
#endif /* (!CY_PD_SINK_ONLY) */

        case APP_EVT_TYPEC_RP_DETACH:
            break;

        case APP_EVT_PR_SWAP_ACCEPTED:
            break;

#if CY_PD_EPR_ENABLE
        case APP_EVT_EPR_MODE_ENTER_FAILED:
            break;
        case APP_EVT_EPR_MODE_ENTER_RECEIVED:
#if CY_APP_ROLE_PREFERENCE_ENABLE
            Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_INITIATE_SWAP_TIMER));
#endif /* CY_APP_ROLE_PREFERENCE_ENABLE */
            break;
        case APP_EVT_EPR_MODE_ENTER_SUCCESS:
            break;
#endif /* CY_PD_EPR_ENABLE */

        case APP_EVT_UNEXPECTED_VOLTAGE_ON_VBUS:
#if (!CY_PD_SINK_ONLY)
            /*
             * There could be stale voltage on VBUS. Try to discharge this voltage. In case
             * this is a quick reconnect, the previous discharge may already be running.
             * In this case, do not try to discharge. If not, then start a discharge cycle.
             * This should only be done for finite duration to avoid any damage in case this
             * was externally driven voltage. Re-use the psource timer implementation.
             *
             * NOTE: This may need to be re-implemented if the APP_PSOURCE_DIS_TIMER timer
             * is being implemented differently.
             */
            if (Cy_PdUtils_SwTimer_IsRunning(ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_PSOURCE_DIS_TIMER)) == false)
            {
                (void)Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_PSOURCE_DIS_TIMER),
                        CY_APP_PSOURCE_DIS_TIMER_PERIOD, app_psrc_invalid_vbus_tmr_cbk);
                glAppInvalidVbusDisOn[port] = true;
                Cy_App_VbusDischargeOn(ptrPdStackContext);
            }
#endif /* (!CY_PD_SINK_ONLY) */
            break;

        default:
            /* Nothing to do */
            break;
    }

    /* Pass the event notification to the fault handler module */
    if (Cy_App_Fault_EventHandler (ptrPdStackContext, evt, dat))
    {
        skip_soln_cb = true;
    }

#if BATTERY_CHARGING_ENABLE
    Cy_App_Bc_PdEventHandler (ptrPdStackContext->ptrUsbPdContext, evt, dat);
#endif /* BATTERY_CHARGING_ENABLE */

    if (!skip_soln_cb)
    {
        /* Send notifications to the solution */
        sln_pd_event_handler(ptrPdStackContext, evt, dat) ;
    }
}

app_resp_t* Cy_App_GetRespBuffer(uint8_t port)
{
    return &glAppStatus[port].appResp;
}

cy_stc_app_status_t* Cy_App_GetStatus(uint8_t port)
{
    return &glAppStatus[port];
}

cy_stc_pdstack_app_status_t* Cy_App_GetPdAppStatus(uint8_t port)
{
     return &glAppPdStatus[port];
}

#if CY_PD_DP_VCONN_SWAP_FEATURE
/* Callback that will be called when there is any change to the V5V or VSYS supplies */
void Cy_App_SupplyChangeCallback(void *context, cy_en_usbpd_supply_t supply_id, bool present)
{
#if PMG1_V5V_CHANGE_DETECT
    cy_stc_usbpd_context_t* ptrUsbPdContext = (cy_stc_usbpd_context_t *)context;
    cy_stc_pdstack_context_t* ptrPdStackContext = (cy_stc_pdstack_context_t *)ptrUsbPdContext->pdStackContext;
    /*
     * Currently we only handle V5V changes:
     * If V5V is removed, we exit active alternate modes if there is a cable which requires VConn
     * If V5V is re-applied after being removed, we restart the alternate mode state machine
     */
    if (supply_id == CY_USBPD_SUPPLY_V5V)
    {
        if (!present)
        {
            glAppPdStatus[ptrPdStackContext->port].faultStatus |= CY_APP_PORT_V5V_SUPPLY_LOST;

#if (!CY_PD_VCONN_DISABLE)
            if (Cy_App_VconnIsPresent (ptrPdStackContext))
            {
                Cy_App_VconnChangeHandler (ptrPdStackContext, false);
            }
#endif /* (!CCG_VCONN_DISABLE) */
        }
        else
        {
            if ((Cy_App_GetPdAppStatus(ptrPdStackContext->port)->faultStatus & CY_APP_PORT_V5V_SUPPLY_LOST) != 0u)
            {
                glAppPdStatus[ptrPdStackContext->port].faultStatus &= ~CY_APP_PORT_V5V_SUPPLY_LOST;
                Cy_App_VconnChangeHandler (ptrPdStackContext, true);
            }
        }
    }
#else
    (void)context;
    (void)supply_id;
    (void)present;
#endif /* PMG1_V5V_CHANGE_DETECT */
}
#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */

void Cy_App_Init(cy_stc_pdstack_context_t *ptrPdStackContext, const cy_stc_app_params_t *appParams)
{
    uint8_t port = ptrPdStackContext->port;
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
    cy_stc_pdaltmode_context_t *ptrAltModeContext = ptrPdStackContext->ptrAltModeContext;
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

    if (appParams == NULL)
    {
        return;
    }

    uint8_t swap_response = 0u;

    glAppVbusPollAdcId[port] = appParams->appVbusPollAdcId;
    glAppVbusPollAdcInput[port] = appParams->appVbusPollAdcInput;

#if CY_USE_CONFIG_TABLE
    /* Update the swap response from config table */
    app_config_t *ptrAppConfig = pd_get_ptr_app_tbl(ptrPdStackContext->ptrUsbPdContext);
    swap_response = ptrAppConfig->swap_response;
#else
    /* Non-config table use case */
    swap_response = appParams->swapResponse;
#endif /* CY_USE_CONFIG_TABLE */

#if CY_APP_RTOS_ENABLED
    event_sema_handle[port] = xSemaphoreCreateCounting(10,0);
    if(NULL == event_sema_handle[port])
    {
        CY_ASSERT(0);
    }
#endif /* CY_APP_RTOS_ENABLED */

    Cy_PdStack_Dpm_UpdateSwapResponse(ptrPdStackContext, swap_response);

    /* For now, only the VDM handlers require an init call. */
#if (!CY_PD_VDM_DISABLE)
    /* Initialize the VDM responses */
    Cy_App_Vdm_Init(ptrPdStackContext, appParams);
#endif /* (!CY_PD_VDM_DISABLE) */

#if BATTERY_CHARGING_ENABLE
    Cy_App_Bc_Init(ptrPdStackContext->ptrUsbPdContext, ptrPdStackContext->ptrTimerContext);
#endif /* BATTERY_CHARGING_ENABLE */

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
    /* Copy AM configuration info */
    memcpy ((uint16_t *)ptrAltModeContext->baseAmInfo, 
            (const uint16_t *)&ptrAltModeContext->altModeCfg->base_am_info,
            (uint8_t)(ptrAltModeContext->altModeCfgLen - (CY_PDALTMODE_MNGR_AM_SVID_CONFIG_OFFSET_START_IDX << 1u)));

#if DFP_ALT_MODE_SUPP
    /* Save mask to enable DFP alt modes */
    ptrAltModeContext->dfpAltModeMask = ptrAltModeContext->altModeCfg->dfp_alt_mode_mask;
#endif /* DFP_ALT_MODE_SUPP */
#if UFP_ALT_MODE_SUPP
    /* Save mask to enable UFP alt modes */
    ptrAltModeContext->ufpAltModeMask = ptrAltModeContext->altModeCfg->ufp_alt_mode_mask;
#endif /* DFP_ALT_MODE_SUPP */
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
#if (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE)
#if RIDGE_SLAVE_ENABLE
    /* Save host capabilities configuration */
    ptrAltModeContext->altModeAppStatus->customHpiHostCapControl = ptrAltModeContext->tbtCfg->host_supp;
    if (ptrAltModeContext->tbtCfg->vpro_capable)
    {
        ptrAltModeContext->altModeAppStatus->customHpiHostCapControl |= CY_PDALTMODE_APP_HPI_VPRO_SUPP_MASK;
    }
    if (
            (ptrAltModeContext->tbtCfg->usb4_supp == CY_PDSTACK_USB_ROLE_HOST) ||
            (ptrAltModeContext->tbtCfg->usb4_supp == CY_PDSTACK_USB_ROLE_DRD)
       )
    {
        ptrAltModeContext->altModeAppStatus->customHpiHostCapControl |= CY_PDALTMODE_USB4_EN_HOST_PARAM_MASK;
    }
#else
    if (
            (ptrAltModeContext->tbtCfg->usb4_supp == CY_PDSTACK_USB_ROLE_HOST) ||
            (ptrAltModeContext->tbtCfg->usb4_supp == CY_PDSTACK_USB_ROLE_DRD)
       )
    {
        ptrAltModeContext->altModeAppStatus->customHpiHostCapControl |= CY_PDALTMODE_USB4_EN_HOST_PARAM_MASK;
    }
#endif /* RIDGE_SLAVE_ENABLE */
#endif /* (CY_PD_USB4_SUPPORT_ENABLE && CY_APP_PD_USB4_SUPPORT_ENABLE) */
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
        if (
                (Cy_PdAltMode_Mngr_GetAltModesConfigSvidIdx(ptrAltModeContext, CY_PD_PRT_TYPE_UFP, CY_PDALTMODE_TBT_SVID) != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED) ||
                (Cy_PdAltMode_Mngr_GetAltModesConfigSvidIdx(ptrAltModeContext, CY_PD_PRT_TYPE_DFP, CY_PDALTMODE_TBT_SVID) != CY_PDALTMODE_MNGR_MODE_NOT_SUPPORTED)
            )
        {
            ptrAltModeContext->altModeAppStatus->customHpiHostCapControl |=  CY_PDALTMODE_TBT_EN_HOST_PARAM_MASK;
        }

#if (CCG_BB_ENABLE != 0)
    /*
     * Initialize the billboard interface. The billboard
     * interface shall not get initialized if it is not
     * enabled in configuration table.
     */
    Cy_App_Usb_BbInit(ptrAltModeContext);
#endif /* (CCG_BB_ENABLE != 0) */
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

#if (CY_APP_ROLE_PREFERENCE_ENABLE)
#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
#if CY_USE_CONFIG_TABLE
    glAppPrefPowerRole[port] = ptrAppConfig->pref_power_role;
#else
    glAppPrefPowerRole[port] = appParams->prefPowerRole;
#endif /* CY_USE_CONFIG_TABLE */
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */

#if CY_USE_CONFIG_TABLE
    glAppPrefDataRole[port] = ptrAppConfig->pref_data_role;
#else
    glAppPrefDataRole[port] = appParams->prefDataRole;
#endif /* CY_USE_CONFIG_TABLE */
#endif /* (CY_APP_ROLE_PREFERENCE_ENABLE) */

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
    if (port == 0)
    {
        if((ptrAltModeContext->appStatusContext->vdmIdVdoResp[CY_PD_PRODUCT_TYPE_VDO_1_IDX].ufp_vdo_1.devCap & (uint32_t)CY_PDSTACK_DEV_CAP_USB_4_0))
        {
            ptrAltModeContext->altModeAppStatus->usb4Supp = true;
        }
        else
        {
            ptrAltModeContext->altModeAppStatus->usb4Supp = false;
        }
    }
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */

    /* Update custom host config settings to the stack */
    ptrPdStackContext->dpmStat.typecAccessorySuppDisabled = !(ptrPdStackContext->ptrPortCfg->accessoryEn);
    ptrPdStackContext->dpmStat.typecRpDetachDisabled = !(ptrPdStackContext->ptrPortCfg->rpDetachEn);

#if CY_PD_DP_VCONN_SWAP_FEATURE
#if (PMG1_V5V_CHANGE_DETECT)
    /* Register a handler that will be notified when there is any change in V5V or VSYS state */
    Cy_USBPD_SetSupplyChange_EvtCb(ptrPdStackContext->ptrUsbPdContext, Cy_App_SupplyChangeCallback);
#endif /* (PMG1_V5V_CHANGE_DETECT) */
#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */

    if(true != Cy_PdUtils_SwTimer_IsRunning(ptrPdStackContext->ptrTimerContext, CY_PDUTILS_CCG_ACTIVITY_TIMER))
    {
        Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext, CY_PDUTILS_CCG_ACTIVITY_TIMER,
                                 CCG_ACTIVITY_TIMER_PERIOD, ccg_activity_timer_cb);
    }

#if CCG_TYPE_A_PORT_ENABLE
    glPtrSlnCbk->type_a_port_enable_disable(true);
#endif /* CCG_TYPE_A_PORT_ENABLE */

#if CY_CORROSION_MITIGATION_ENABLE
    Cy_App_MoistureDetect_Init(ptrPdStackContext);
#endif /* CY_CORROSION_MITIGATION_ENABLE */
}

#if SYS_DEEPSLEEP_ENABLE
__attribute__ ((weak)) bool soln_sleep ()
{
    return true;
}

__attribute__ ((weak)) void soln_resume ()
{
    return;
}

/* Implements Deep Sleep functionality for power saving */
bool Cy_App_SystemSleep(cy_stc_pdstack_context_t *ptrPdStack0Context, cy_stc_pdstack_context_t *ptrPdStack1Context)
{
    uint32_t intr_state;
    bool dpm_slept = false;
    bool retval = false;
    bool app_slept = false;
#if RIDGE_SLAVE_ENABLE
    bool ridge_intf_slept = false;
#endif /* RIDGE_SLAVE_ENABLE */
#if PMG1_PD_DUALPORT_ENABLE
    bool dpm_port1_slept = false;
#endif /* PMG1_PD_DUALPORT_ENABLE */
#if BATTERY_CHARGING_ENABLE
    bool bc_slept = false;
#endif /* BATTERY_CHARGING_ENABLE */

    if (soln_sleep () == false)
    {
        return retval;
    }

#if CY_CORROSION_MITIGATION_ENABLE
    if((ptrPdStack0Context->typecStat.moisturePresent != false)
#if PMG1_PD_DUALPORT_ENABLE
      || (ptrPdStack1Context->typecStat.moisturePresent != false)
#endif /* PMG1_PD_DUALPORT_ENABLE */
      )
    {
        return retval;
    }
#endif /* CY_CORROSION_MITIGATION_ENABLE */

    /* Do one DPM Sleep capability check before locking interrupts out */
    if (
            (Cy_PdStack_Dpm_IsIdle (ptrPdStack0Context, &dpm_slept) != CY_PDSTACK_STAT_SUCCESS) ||
            (!dpm_slept)
       )
    {
        return retval;
    }

#if PMG1_PD_DUALPORT_ENABLE
    if (
            (Cy_PdStack_Dpm_IsIdle (ptrPdStack1Context, &dpm_port1_slept) != CY_PDSTACK_STAT_SUCCESS) ||
            (!dpm_port1_slept)
       )
    {
        return retval;
    }
#endif /* PMG1_PD_DUALPORT_ENABLE */

#if CY_HPI_ENABLED
    if (Cy_Hpi_SleepAllowed((cy_stc_hpi_context_t *)ptrPdStack0Context->ptrHpiContext) != true)
    {
        return retval;
    }
#endif /* CY_HPI_ENABLED */

#if CY_HPI_MASTER_ENABLE
    if (Cy_HPI_Master_SleepAllowed(get_hpi_master_context()) != true)
    {
        return retval;
    }
#endif /* CY_HPI_MASTER_ENABLE */

#if CCG_TYPE_A_PORT_ENABLE
    if(glPtrSlnCbk->type_a_is_idle() == true)
    {
        return retval;
    }
#endif /* CCG_TYPE_A_PORT_ENABLE */

    intr_state = Cy_SysLib_EnterCriticalSection();

    if (Cy_App_Sleep())
    {
        app_slept = true;
#if BATTERY_CHARGING_ENABLE
        if(Cy_App_Bc_PrepareDeepSleep(ptrPdStack0Context->ptrUsbPdContext))
        {
            bc_slept = true;
#endif /* BATTERY_CHARGING_ENABLE */

#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
#if RIDGE_SLAVE_ENABLE
            if (Cy_PdAltMode_SocDock_Sleep(ptrPdStack0Context->ptrAltModeContext) &&
                Cy_PdAltMode_SocDock_Sleep(ptrPdStack1Context->ptrAltModeContext))
#endif /* RIDGE_SLAVE_ENABLE */
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */
            {
#if RIDGE_SLAVE_ENABLE
                ridge_intf_slept = true;
#endif /* RIDGE_SLAVE_ENABLE */
                if (((Cy_PdStack_Dpm_PrepareDeepSleep(ptrPdStack0Context,
                                    &dpm_slept) == CY_PDSTACK_STAT_SUCCESS) && (dpm_slept))
#if PMG1_PD_DUALPORT_ENABLE
                        && ((Cy_PdStack_Dpm_PrepareDeepSleep(ptrPdStack1Context,
                                    &dpm_port1_slept) == CY_PDSTACK_STAT_SUCCESS)
                            && (dpm_port1_slept))
#endif /* PMG1_PD_DUALPORT_ENABLE */
                    )
                {
                    Cy_PdUtils_SwTimer_EnterSleep(ptrPdStack0Context->ptrTimerContext);

#if CY_HPI_ENABLED
                    if (Cy_Hpi_Sleep((cy_stc_hpi_context_t *)ptrPdStack0Context->ptrHpiContext))
#endif /* CY_HPI_ENABLED */
                    {

                        Cy_USBPD_SetReference(ptrPdStack0Context->ptrUsbPdContext, true);
                    
#if PMG1_PD_DUALPORT_ENABLE
                        Cy_USBPD_SetReference(ptrPdStack1Context->ptrUsbPdContext, true);
#endif /* PMG1_PD_DUALPORT_ENABLE */

#if ((!CY_APP_DEBUG_PULLUP_ON_UART) && (CY_APP_UART_DEBUG_ENABLE))
                        /* Set the HSIOM to GPIO since the UART can't drive the pin in Deep Sleep mode */
                        Cy_GPIO_SetHSIOM(CYBSP_DEBUG_UART_TX_PORT, CYBSP_DEBUG_UART_TX_PIN, HSIOM_SEL_GPIO);
#endif /* ((!CY_APP_DEBUG_PULLUP_ON_UART) && (CY_APP_UART_DEBUG_ENABLE)) */

                        /*
                         * The UART check needs to be done to confirm there is not any transition in progress
                         */
#if CY_APP_DEBUG_ENABLE
                        if(Cy_SCB_UART_IsTxComplete(Cy_App_Debug_GetScbBaseAddr()))
#endif /* CY_APP_DEBUG_ENABLE */
                        {
                            /*
                             * The I2C IDLE check needs to be done as the last step
                             * before device enters into Sleep. Otherwise, the device may fail
                             * to wake up when there is an address match on the I2C interface.
                             */
#if CY_HPI_ENABLED
                            if((CY_APP_HPI_I2C_HW->I2C_STATUS & SCB_I2C_STATUS_BUS_BUSY_Msk) == 0u)
#endif /* CY_HPI_ENABLED */
                            {
                                /* Device Sleep entry */
                                Cy_SysPm_CpuEnterDeepSleep();
                            }
                        }
#if ((!CY_APP_DEBUG_PULLUP_ON_UART) && (CY_APP_UART_DEBUG_ENABLE))
                        /* Change the HSIOM back to UART_TX */
                        Cy_GPIO_SetHSIOM(CYBSP_DEBUG_UART_TX_PORT, CYBSP_DEBUG_UART_TX_PIN, CYBSP_DEBUG_UART_TX_HSIOM);
#endif /* ((!CY_APP_DEBUG_PULLUP_ON_UART) && (CY_APP_UART_DEBUG_ENABLE)) */

                        Cy_USBPD_SetReference(ptrPdStack0Context->ptrUsbPdContext, false);
#if PMG1_PD_DUALPORT_ENABLE
                        Cy_USBPD_SetReference(ptrPdStack1Context->ptrUsbPdContext, false);
#endif /* PMG1_PD_DUALPORT_ENABLE */
                        retval = true;
                    }
                }
            }
#if BATTERY_CHARGING_ENABLE
        }
#endif /* BATTERY_CHARGING_ENABLE */
    }

    /* Call dpm_wakeup() if dpm_sleep() had returned true */
    if (dpm_slept)
    {
        Cy_PdStack_Dpm_Resume(ptrPdStack0Context, &dpm_slept);
    }

#if PMG1_PD_DUALPORT_ENABLE
    if (dpm_port1_slept)
    {
        Cy_PdStack_Dpm_Resume(ptrPdStack1Context, &dpm_port1_slept);
    }
#endif /* PMG1_PD_DUALPORT_ENABLE */

#if BATTERY_CHARGING_ENABLE
    if (bc_slept)
    {
        Cy_App_Bc_Resume (ptrPdStack0Context->ptrUsbPdContext);
    }
#endif /* BATTERY_CHARGING_ENABLE */

    /* Call Cy_App_Resume() if Cy_App_Sleep() had returned true */
    if(app_slept)
    {
#if ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP))
        Cy_App_Resume();
#endif /* ((DFP_ALT_MODE_SUPP) || (UFP_ALT_MODE_SUPP)) */
    }
#if RIDGE_SLAVE_ENABLE
    if(ridge_intf_slept)
    {
        Cy_PdAltMode_SocDock_Wakeup();
    }
#endif /* RIDGE_SLAVE_ENABLE */

    soln_resume ();

    Cy_SysLib_ExitCriticalSection(intr_state);

#if (PMG1_PD_DUALPORT_ENABLE != 1)
    (void)ptrPdStack1Context;
#endif

    return retval;
}
#endif /* SYS_DEEPSLEEP_ENABLE */

#if VCONN_OCP_ENABLE
void app_vconn_ocp_tmr_cbk(cy_timer_id_t id,  void *context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;

    /* Disable VConn since we hit a fault */
    Cy_App_VconnDisable(ptrPdStackContext, ptrPdStackContext->dpmConfig.revPol);

    /* Notify application layer about fault */
    Cy_App_EventHandler(ptrPdStackContext, APP_EVT_VCONN_OCP_FAULT, NULL);

    (void)id;
}

bool app_vconn_ocp_cbk(void *context, bool comp_out)
{
    cy_stc_usbpd_context_t * ptrUsbPdContext = (cy_stc_usbpd_context_t *)context;
    cy_stc_pdstack_context_t * ptrPdStackContext = Cy_PdStack_Dpm_GetContext(ptrUsbPdContext->port);
    bool retval = false;

    if (comp_out)
    {
        /* Start a VConn OCP debounce timer */
        Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_PDSTACK_GET_PD_TIMER_ID(ptrPdStackContext, CY_PDSTACK_PD_VCONN_OCP_DEBOUNCE_TIMER),
                ptrPdStackContext->ptrUsbPdContext->usbpdConfig->vconnOcpConfig->debounce,
                app_vconn_ocp_tmr_cbk);
    }
    else
    {
        /* Negative edge. Check if the timer is still running. */
        retval = Cy_PdUtils_SwTimer_IsRunning(ptrPdStackContext->ptrTimerContext, CY_PDSTACK_GET_PD_TIMER_ID(ptrPdStackContext, CY_PDSTACK_PD_VCONN_OCP_DEBOUNCE_TIMER));
        if (retval)
        {
            Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, CY_PDSTACK_GET_PD_TIMER_ID(ptrPdStackContext, CY_PDSTACK_PD_VCONN_OCP_DEBOUNCE_TIMER));
        }
    }

    return retval;
}

#endif /* VCONN_OCP_ENABLE */

void app_vconn_tmr_cbk(cy_timer_id_t id,  void *context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;
    Cy_USBPD_Vconn_GatePullUp_Enable(ptrPdStackContext->ptrUsbPdContext);

    (void) id;
}

bool Cy_App_VconnEnable(cy_stc_pdstack_context_t *ptrPdStackContext, uint8_t channel)
{
#if VCONN_OCP_ENABLE
    /* Do not attempt to enable VConn if fault was detected in present connection */
    if ((Cy_App_GetPdAppStatus(ptrPdStackContext->port)->faultStatus & CY_APP_PORT_VCONN_FAULT_ACTIVE) != 0)
    {
        return false;
    }

    Cy_App_Fault_Vconn_OcpEnable(ptrPdStackContext, app_vconn_ocp_cbk);
#endif /* VCONN_OCP_ENABLE */

    /* Reset RX protocol for cable */
    Cy_PdStack_Dpm_ProtResetRx(ptrPdStackContext, CY_PD_SOP_PRIME);
    Cy_PdStack_Dpm_ProtResetRx(ptrPdStackContext, CY_PD_SOP_DPRIME);

    if (Cy_USBPD_Vconn_Enable(ptrPdStackContext->ptrUsbPdContext, channel) != CY_USBPD_STAT_SUCCESS)
    {
#if ((defined(CY_DEVICE_CCG6)) || (defined(CY_DEVICE_PMG1S3)))
        glAppPdStatus[ptrPdStackContext->port].faultStatus |= CY_APP_PORT_V5V_SUPPLY_LOST;
#endif /* PMG1 */
        return false;
    }
#if defined(CY_DEVICE_PMG1S3)
    /* Start a timer for providing required delay before Vconn gate pull up enable */
    Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
            CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_VCONN_TURN_ON_DELAY_TIMER),
            CY_APP_VCONN_TURN_ON_DELAY_PERIOD, app_vconn_tmr_cbk);
#endif /* CY_DEVICE_PMG1S3 */
    return true;
}

void Cy_App_VconnDisable(cy_stc_pdstack_context_t *ptrPdStackContext, uint8_t channel)
{
    Cy_USBPD_Vconn_Disable(ptrPdStackContext->ptrUsbPdContext, channel);
#if VCONN_OCP_ENABLE
    Cy_App_Fault_Vconn_OcpDisable(ptrPdStackContext);
#endif /* VCONN_OCP_ENABLE */
}

bool Cy_App_VconnIsPresent(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    return Cy_USBPD_Vconn_IsPresent(ptrPdStackContext->ptrUsbPdContext, ptrPdStackContext->dpmConfig.revPol);
}

bool Cy_App_VbusIsPresent(cy_stc_pdstack_context_t *ptrPdStackContext, uint16_t volt, int8_t per)
{
    uint8_t level;
    uint8_t retVal;
    uint8_t port = ptrPdStackContext->port;
    /*
     * Re-run calibration every time to ensure that VDDD or the measurement
     * does not break
     */
    Cy_USBPD_Adc_Calibrate(ptrPdStackContext->ptrUsbPdContext, glAppVbusPollAdcId[port]);
    level =  Cy_USBPD_Adc_GetVbusLevel(ptrPdStackContext->ptrUsbPdContext, 
            glAppVbusPollAdcId[port], 
            volt, per);

    retVal = Cy_USBPD_Adc_CompSample(ptrPdStackContext->ptrUsbPdContext,
            glAppVbusPollAdcId[port], 
            glAppVbusPollAdcInput[port], level);

    return retVal;
}

void Cy_App_VbusDischargeOn(cy_stc_pdstack_context_t* context)
{
    Cy_USBPD_Vbus_DischargeOn(context->ptrUsbPdContext);
}

void Cy_App_VbusDischargeOff(cy_stc_pdstack_context_t* context)
{
    Cy_USBPD_Vbus_DischargeOff(context->ptrUsbPdContext);
}

uint16_t Cy_App_VbusGetValue(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    uint16_t retVal;
    uint8_t port = ptrPdStackContext->port;

    /* Measure the actual VBUS voltage */
    retVal = Cy_USBPD_Adc_MeasureVbus(ptrPdStackContext->ptrUsbPdContext,
            glAppVbusPollAdcId[port],
            glAppVbusPollAdcInput[port]);

    return retVal;
}

#if ((CY_PD_EPR_ENABLE) && (!CY_PD_SINK_ONLY))
bool Cy_App_EvalEprMode(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_eprmdo_action_t eprModeState, cy_pdstack_app_resp_cbk_t app_resp_handler)
{
    (void) ptrPdStackContext;
    (void) eprModeState;
    (void) app_resp_handler;

    /* Place holder for customer specific EPR source preparation.
     * It is possible to provide additional checking before sends EPR mode
     * enter acknowledged  */

    return true;
}

bool Cy_App_SendEprCap(cy_stc_pdstack_context_t *ptrPdStackContext, cy_pdstack_app_resp_cbk_t app_resp_handler)
{
    (void) ptrPdStackContext;
    (void) app_resp_handler;

    /* Place holder for customer specific EPR source preparation.
     * It is possible to provide additional checking before sends EPR SRC CAP.
     * */

    app_resp_handler(ptrPdStackContext, NULL);

    return true;
}
#endif /* ((CY_PD_EPR_ENABLE) && (!CY_PD_SINK_ONLY)) */

#if (!CY_PD_SINK_ONLY)
#if (!CY_APP_SMART_POWER_ENABLE)
static uint8_t app_get_current_pdp(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    cy_pd_pd_do_t maxPdo, curPdo;
    cy_stc_pdstack_dpm_status_t *dpm_stat = &(ptrPdStackContext->dpmStat);
    uint8_t index;

    maxPdo = dpm_stat->curSrcPdo[0];

    /* Loop through all PDOs to update power. */
    for (index = 1u; index < dpm_stat->srcPdoCount; index++)
    {
        curPdo = dpm_stat->curSrcPdo[index];

        /* Checks if the PDO's mask is enabled for this PDO. */
        if ((dpm_stat->srcPdoMask & (0x01U << index)) != 0U)
        {
            if (curPdo.fixed_src.supplyType != (uint32_t)CY_PDSTACK_PDO_FIXED_SUPPLY)
            {
                /* Non fixed supply PDO encountered, exit */
                break;
            }
            maxPdo = dpm_stat->curSrcPdo[index];
        }
    }
    return ((((maxPdo.fixed_src.voltage * CY_PD_VOLT_PER_UNIT)/1000) * (maxPdo.fixed_src.maxCurrent * 10))/1000);
}
#endif /* (!CY_APP_SMART_POWER_ENABLE) */

bool Cy_App_SendSrcInfo (struct cy_stc_pdstack_context *ptrPdStackContext)
{
    bool ret = true;
#if ((CY_APP_RW_PD_RESP_MSG_DATA) && (CY_PD_SRC_INFO_HANDLER_ENABLE))
    cy_stc_app_pd_resp_stored_t *ptrPdResp;
    /* Source info stored in the memory is 4 bytes in size.*/
    cy_stc_app_pd_resp_data_t srcInfoPdResp;
    srcInfoPdResp.respId = CY_APP_PD_RESP_ID_SRC_INFO;
    srcInfoPdResp.respLen = CY_APP_APP_SRC_INFO_LENGTH;
    /* Match port specific response only.*/
    srcInfoPdResp.cmdVal = (uint8_t)(CY_APP_PD_RESP_DATA_PORT_FLAG_MSK << ptrPdStackContext->port);
    /* Retrieve the source info response with 4 bytes as length.*/
    ptrPdResp = Cy_App_Hpi_RetrievePdRespData(&srcInfoPdResp, 0x00u);

    if(NULL != ptrPdResp)
    {
        /* Update the source info */
        ptrPdStackContext->dpmExtStat.srcInfo.val = ((cy_pd_pd_do_t *)(&ptrPdResp->respData))->val;
    }
    else
    {
        ret = false;
    }
#else
#if (!CY_APP_SMART_POWER_ENABLE)
    cy_stc_pdstack_dpm_ext_status_t* ptrDpmExtStat = &(ptrPdStackContext->dpmExtStat);
#if CY_PD_EPR_ENABLE
    cy_stc_pdstack_dpm_status_t* dpm_stat = &(ptrPdStackContext->dpmStat);
#endif /* CY_PD_EPR_ENABLE */

    if(ptrPdStackContext->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SOURCE)
    {
        if(ptrPdStackContext->dpmConfig.emcaPresent != false)
        {
            /* EMCA present, updated Port Preset PDP configuration based on cable capabilities. */
            ptrDpmExtStat->srcInfo.src_info.portPresPdp =  ptrDpmExtStat->extSrcCap[CY_PD_EXT_SPR_SRCCAP_PDP_INDEX];

#if CY_PD_EPR_ENABLE
            if (ptrDpmExtStat->epr.srcEnable == 1)
            {
                /* Both Passive VDO and ACT VDO1 have EPR support, Vbus_max and Imax fields at the same positions.
                   Check if EPR support is set and Max cable voltage is 50V. */
                if((dpm_stat->cblVdo.pas_cbl_vdo.eprModeCapable == true)
                        && (dpm_stat->cblVdo.pas_cbl_vdo.maxVbusVolt == CY_PD_MAX_CBL_VBUS_50V)
                        && (dpm_stat->cblVdo.pas_cbl_vdo.vbusCur == CY_PDSTACK_CBL_VBUS_CUR_5A))
                {
                    ptrDpmExtStat->srcInfo.src_info.portPresPdp =  ptrDpmExtStat->extSrcCap[CY_PD_EXT_EPR_SRCCAP_PDP_INDEX];
                }
            }
#endif /* CY_PD_EPR_ENABLE */
        }
        else
        {
            /* EMCA not present, update PDP to match same. */
            ptrDpmExtStat->srcInfo.src_info.portPresPdp = app_get_current_pdp(ptrPdStackContext);
        }

#if CY_PD_EPR_ENABLE
        if(ptrDpmExtStat->eprActive == true)
        {
            ptrDpmExtStat->srcInfo.src_info.portRepPdp =  ptrDpmExtStat->extSrcCap[CY_PD_EXT_EPR_SRCCAP_PDP_INDEX];
        }
        else
#endif /* CY_PD_EPR_ENABLE */
        {
            ptrDpmExtStat->srcInfo.src_info.portRepPdp =  app_get_current_pdp(ptrPdStackContext);
        }
    }
#else
    (void)ptrPdStackContext;
#endif /* (!CY_APP_SMART_POWER_ENABLE) */
#endif /* ((CY_APP_RW_PD_RESP_MSG_DATA) && (CY_PD_SRC_INFO_HANDLER_ENABLE)) */
    return ret;
}
#endif /* (!CY_PD_SINK_ONLY) */

cy_en_pdstack_status_t Cy_App_DisablePdPort(cy_stc_pdstack_context_t *ptrPdStackContext, cy_pdstack_dpm_typec_cmd_cbk_t cbk)
{
    cy_en_pdstack_status_t retval = CY_PDSTACK_STAT_SUCCESS;
    uint8_t port = ptrPdStackContext->port;

    if (Cy_PdUtils_SwTimer_IsRunning (ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_FAULT_RECOVERY_TIMER)))
    {
        /* If the HPI Master is asking us to disable the port, make sure all fault protection state is cleared. */
        glAppPdStatus[port].faultStatus &= ~(
                CY_APP_PORT_VBUS_DROP_WAIT_ACTIVE | CY_APP_PORT_SINK_FAULT_ACTIVE | CY_APP_PORT_DISABLE_IN_PROGRESS |
                CY_APP_PORT_VCONN_FAULT_ACTIVE | CY_APP_PORT_V5V_SUPPLY_LOST);

        Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_FAULT_RECOVERY_TIMER));

    }
    else
    {
        /* Just pass the call on-to the stack. */
        if (ptrPdStackContext->dpmConfig.dpmEnabled)
        {
            retval = Cy_PdStack_Dpm_SendTypecCommand(ptrPdStackContext,  CY_PDSTACK_DPM_CMD_PORT_DISABLE,  cbk);
            return retval;
        }
    }

    /* Make sure any dead battery Rd terminations are removed at this stage. */
    Cy_USBPD_TypeC_DisableRd(ptrPdStackContext->ptrUsbPdContext, CY_PD_CC_CHANNEL_1);
    Cy_USBPD_TypeC_DisableRd(ptrPdStackContext->ptrUsbPdContext, CY_PD_CC_CHANNEL_2);

    /* Send success response to the caller. */
    cbk(ptrPdStackContext, CY_PDSTACK_DPM_RESP_SUCCESS);

    return retval;
}

bool Cy_App_GetRtosEvent(cy_stc_pdstack_context_t *context, uint32_t waitTime)
{
#if CY_APP_RTOS_ENABLED
    if(NULL == event_sema_handle[context->port])
        return false;


    return (pdTRUE == xSemaphoreTake(event_sema_handle[context->port], convert_ms_to_ticks(waitTime)));
#else
    (void)context;
    (void)waitTime;
    return false;
#endif /* CY_APP_RTOS_ENABLED */
}

void Cy_App_SendRtosEvent(cy_stc_pdstack_context_t *context)
{
#if CY_APP_RTOS_ENABLED
    if(NULL == event_sema_handle[context->port])
        return;

    if(is_in_isr() == true)
    {
        BaseType_t xTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(event_sema_handle[context->port], &xTaskWoken);
        portYIELD_FROM_ISR(xTaskWoken);
    }
    else
    {
        xSemaphoreGive(event_sema_handle[context->port]);
    }
#else
    (void)context;
#endif /* CY_APP_RTOS_ENABLED */
}

void Cy_App_RegisterSlnCallback(cy_stc_pdstack_context_t *ptrPdStackcontext, cy_app_sln_cbk_t *callback)
{
    (void)ptrPdStackcontext;
    /* Register the solution callback functions */
    glPtrSlnCbk = callback;
}

__attribute__ ((weak)) void sln_pd_event_handler(cy_stc_pdstack_context_t *ptrPdStackContext,
        cy_en_pdstack_app_evt_t evt, const void *data)
{
    (void) ptrPdStackContext;
    (void) evt;
    (void) data;
    return;
}

/* [] END OF FILE */
