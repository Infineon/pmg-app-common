/***************************************************************************//**
* \file cy_app_fault_handlers.c
* \version 2.0
*
* \brief
* Implements the application level fault handlers
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
#include "cy_app_config.h"
#if (!CY_PD_SINK_ONLY)
#include "cy_app_source.h"
#endif /* !CY_PD_SINK_ONLY */
#include "cy_app_sink.h"

#include "cy_app.h"
#include "cy_app_debug.h"
#include "cy_app_timer_id.h"

#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"
#include "cy_usbpd_vbus_ctrl.h"

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
#include "cy_pdaltmode_timer_id.h"
#endif

enum
{
    FAULT_TYPE_VBUS_OVP = 0,    /* 0 */
    FAULT_TYPE_VBUS_UVP,        /* 1 */
    FAULT_TYPE_VBUS_OCP,        /* 2 */
    FAULT_TYPE_VBUS_SCP,        /* 3 */
    FAULT_TYPE_CC_OVP,          /* 4 */
    FAULT_TYPE_VCONN_OCP,       /* 5 */
    FAULT_TYPE_SBU_OVP,         /* 6 */
    FAULT_TYPE_OTP,             /* 7 */
    FAULT_TYPE_VBUS_RCP,        /* 8 */
    FAULT_TYPE_COUNT            /* 9 */
};

#if (VBUS_OVP_ENABLE || VBUS_UVP_ENABLE || VBUS_OCP_ENABLE || VBUS_SCP_ENABLE || VBUS_RCP_ENABLE || VCONN_OCP_ENABLE)
/* Variable defined in app.c */
extern cy_stc_pdstack_app_status_t glAppPdStatus[];

/* Shorthand for any faults enabled */
#define CY_APP_FAULT_HANDLER_ENABLE            (1)

/* 
 * If fault retry count in the configuration table is set to this value, then
 * faults are not counted. That is, infinite fault recovery is enabled.
 */
#define FAULT_COUNTER_SKIP_VALUE        (255u)

/* Number of retries defined by user for each fault type */
static uint8_t glAppFaultRetryLimit[NO_OF_TYPEC_PORTS][FAULT_TYPE_COUNT] =
{
    {0}
};

/* Number of times each fault condition has been detected during the current connection */
static volatile uint8_t glAppFaultCount[NO_OF_TYPEC_PORTS][FAULT_TYPE_COUNT] =
{
    {0}
};

#endif /* (VBUS_OVP_ENABLE || VBUS_UVP_ENABLE || VBUS_OCP_ENABLE || VBUS_SCP_ENABLE || VBUS_RCP_ENABLE || VCONN_OCP_ENABLE) */

#if ((VCONN_OCP_ENABLE) || (PMG1_V5V_CHANGE_DETECT))

#if VCONN_OCP_ENABLE
static void vconn_restore_timer_cb (cy_timer_id_t id, void *context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;
    const cy_stc_pd_dpm_config_t *dpm_config= &ptrPdStackContext->dpmConfig;

    if ((dpm_config->attach) && (dpm_config->vconnLogical))
    {
        /* If CCG is still the VConn source, start recovery actions. */
        Cy_App_VconnChangeHandler (ptrPdStackContext, true);
    }

    (void)id;
}
#endif /* VCONN_OCP_ENABLE */

void Cy_App_VconnChangeHandler(cy_stc_pdstack_context_t *ptrPdStackContext, bool vconn_on)
{
    const cy_stc_pd_dpm_config_t *dpm_config= &ptrPdStackContext->dpmConfig;
#if VCONN_OCP_ENABLE
    uint8_t port = ptrPdStackContext->port;
#endif /* VCONN_OCP_ENABLE */

    if (!vconn_on)
    {
#if VCONN_OCP_ENABLE
        /* Store VConn fault status */
        glAppPdStatus[port].faultStatus |= CY_APP_PORT_VCONN_FAULT_ACTIVE;
#endif /* VCONN_OCP_ENABLE */

        /* Ensure that the VConn switch is turned off */
        Cy_App_VconnDisable (ptrPdStackContext, dpm_config->revPol);
    }
    else
    {
#if VCONN_OCP_ENABLE
        glAppPdStatus[port].faultStatus &= ~CY_APP_PORT_VCONN_FAULT_ACTIVE;
#endif /* VCONN_OCP_ENABLE */
    }
}

#endif /* ((VCONN_OCP_ENABLE) || (PMG1_V5V_CHANGE_DETECT)) */

#if CY_APP_FAULT_HANDLER_ENABLE

/* Check whether any fault count has exceeded limit for the specified PD port */
bool Cy_App_Fault_IsCountExceeded(cy_stc_pdstack_context_t * context)
{
    uint32_t i;
    bool     retval = false;
    uint8_t port = context->port;
    /*
     * Check whether the count for any fault type has exceeded the limit specified
     */
    for (i = 0; i < FAULT_TYPE_COUNT; i++)
    {
        if (glAppFaultCount[port][i] > glAppFaultRetryLimit[port][i])
        {
            retval = true;
            break;
        }
    }

    return (retval);
}

/* This function stops PD operation and configures Type-C to look for detach of faulty device */
void Cy_App_Fault_ConfigureForDetach(cy_stc_pdstack_context_t * context)
{
    cy_stc_pd_dpm_config_t *dpm_config = &(context->dpmConfig);
    uint8_t port = context->port;

    if ((!dpm_config->attach) || (dpm_config->curPortRole == CY_PD_PRT_ROLE_SINK))
    {
        /* Set flag to trigger port disable sequence */
        glAppPdStatus[port].faultStatus |= CY_APP_PORT_SINK_FAULT_ACTIVE;

#if CY_APP_RTOS_ENABLED
        Cy_App_SendRtosEvent(context);
#endif /* CY_APP_RTOS_ENABLED*/
    }

    /* Stop PE */
    Cy_PdStack_Dpm_PeStop(context);
}

/* Generic routine that notifies the stack about recovery actions for a fault */
static void app_handle_fault(cy_stc_pdstack_context_t * context, uint32_t fault_type)
{
    uint8_t port = context->ptrUsbPdContext->port;

    if (fault_type != FAULT_TYPE_VCONN_OCP)
    {
        context->dpmStat.faultActive = true;
    }

    /* Update the fault count */
    if(glAppFaultRetryLimit[port][fault_type] == FAULT_COUNTER_SKIP_VALUE)
    {
        /* Do not count faults if infinite fault retry is set */    
    }
    else
    {
        glAppFaultCount[port][fault_type]++;
    }

    if (glAppFaultCount[port][fault_type] < (glAppFaultRetryLimit[port][fault_type] + 1))
    {
#if VCONN_OCP_ENABLE
        if (fault_type == FAULT_TYPE_VCONN_OCP)
        {
            /* Start VConn turn OFF procedure and start a timer to restore VConn after a delay */
            Cy_App_VconnChangeHandler (context, false);
            Cy_PdUtils_SwTimer_Start (context->ptrTimerContext, context, CY_PDSTACK_GET_PD_TIMER_ID(context, CY_PDSTACK_PD_VCONN_RECOVERY_TIMER),
                    CY_APP_VCONN_RECOVERY_PERIOD, vconn_restore_timer_cb);
        }
        else
#endif /* VCONN_OCP_ENABLE */
        {
            context->peStat.hardResetCount = 0;
            /*
             * Try a hard reset to recover from fault
             * If not successful (not in PD contract), try Type-C error recovery.
             */
            if (Cy_PdStack_Dpm_SendPdCommand(context, CY_PDSTACK_DPM_CMD_SEND_HARD_RESET, NULL, false, NULL) != CY_PDSTACK_STAT_SUCCESS)
            {
                Cy_PdStack_Dpm_SendTypecCommand(context, CY_PDSTACK_DPM_CMD_TYPEC_ERR_RECOVERY, NULL);
            }
        }
    }
    else
    {
#if VCONN_OCP_ENABLE
        if (fault_type == FAULT_TYPE_VCONN_OCP)
        {
            Cy_App_VconnChangeHandler (context, false);
        }
        else
#endif /* VCONN_OCP_ENABLE */
        {
            Cy_App_Fault_ConfigureForDetach(context);
        }
    }
}

/* Timer used to re-enable the PD port after a fault */
static void fault_recovery_timer_cb(cy_timer_id_t id, void *context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *) context;
    uint16_t period = CY_APP_FAULT_RECOVERY_TIMER_PERIOD;
    uint8_t port = ptrPdStackContext->port;

    if (
            (Cy_App_VbusIsPresent(ptrPdStackContext, CY_PD_VSAFE_0V, 0) == false)
       )
    {
        if ((Cy_App_GetPdAppStatus(port)->faultStatus & CY_APP_PORT_VBUS_DROP_WAIT_ACTIVE) != 0)
        {
            glAppPdStatus[port].faultStatus &= ~CY_APP_PORT_VBUS_DROP_WAIT_ACTIVE;

            /* VBus has already been removed. Enable the Rd termination to check for physical detach. */
            Cy_USBPD_TypeC_RdEnable (ptrPdStackContext->ptrUsbPdContext);
            period = CY_APP_FAULT_RECOVERY_MAX_WAIT;
        }
        else
        {
            /*
             * If VBus is not detected, we can re-enable the PD port.
             */
            glAppPdStatus[port].faultStatus &= ~CY_APP_PORT_DISABLE_IN_PROGRESS;
            ptrPdStackContext->dpmStat.faultActive = false;

            Cy_USBPD_TypeC_DisableRd(ptrPdStackContext->ptrUsbPdContext, CY_PD_CC_CHANNEL_1);
            Cy_USBPD_TypeC_DisableRd(ptrPdStackContext->ptrUsbPdContext, CY_PD_CC_CHANNEL_2);
            Cy_PdStack_Dpm_Start(ptrPdStackContext);

            /* Return without restarting the timer */
            return;
        }
    }

    /* Restart the timer to check VBus and Rp status again.*/
    Cy_PdUtils_SwTimer_Start (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                              CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_FAULT_RECOVERY_TIMER),
                              period, fault_recovery_timer_cb);

    (void)id;
}

/* Callback used to get notification that PD port disable has been completed */
static void app_port_disable_cb(cy_stc_pdstack_context_t * context, cy_en_pdstack_dpm_typec_cmd_resp_t resp)
{
    uint16_t period = CY_APP_FAULT_RECOVERY_TIMER_PERIOD;
    uint8_t port = context->port;

    if (
            (Cy_App_VbusIsPresent(context, CY_PD_VSAFE_0V, 0) == false)
       )
    {
        /* VBus has already been removed. Enable the Rd termination to check for physical detach. */
        Cy_USBPD_TypeC_RdEnable (context->ptrUsbPdContext);
        period = CY_APP_FAULT_RECOVERY_MAX_WAIT;
    }
    else
    {
        /* VBus has not been removed. Start a task which waits for VBus removal. */
        glAppPdStatus[port].faultStatus |= CY_APP_PORT_VBUS_DROP_WAIT_ACTIVE;

#if CY_APP_RTOS_ENABLED
        Cy_App_SendRtosEvent(context);
#endif /* CY_APP_RTOS_ENABLED */
    }

    /* Provide a delay to allow VBus turn ON by port partner and then enable the port. */
    Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context,
                             CY_APP_GET_TIMER_ID(context, CY_APP_FAULT_RECOVERY_TIMER),
                             period, fault_recovery_timer_cb);

    (void)resp;
}
#endif /* CY_APP_FAULT_HANDLER_ENABLE */

/* Clear all fault counters associated with the specified port */
void Cy_App_Fault_ClearCounts (uint8_t port)
{
#if CY_APP_FAULT_HANDLER_ENABLE
    /* Clear all fault counters on disconnect */
    memset ((uint8_t *)glAppFaultCount[port], 0, FAULT_TYPE_COUNT);
#endif /* CY_APP_FAULT_HANDLER_ENABLE */

    (void)port;
}

/* Fault-handling specific actions to be performed for various event callbacks */
bool Cy_App_Fault_EventHandler(cy_stc_pdstack_context_t * context, cy_en_pdstack_app_evt_t evt, const void *dat)
{
    bool skip_soln_cb = false;
    uint8_t port = context->port;
#if CY_APP_FAULT_HANDLER_ENABLE
    switch (evt)
    {
        case APP_EVT_TYPE_C_ERROR_RECOVERY:
            if (Cy_App_Fault_IsCountExceeded(context))
            {
                break;
            }
            /* Fall-through to the below case when fault counts are within limits */
            // fall through
        case APP_EVT_DISCONNECT:
        case APP_EVT_VBUS_PORT_DISABLE:
        case APP_EVT_HARD_RESET_SENT:
            /* Clear the port-in-fault status */
            if ((glAppPdStatus[port].faultStatus & CY_APP_PORT_DISABLE_IN_PROGRESS) == 0)
            {
                context->dpmStat.faultActive = false;
            }

            if ((evt == APP_EVT_DISCONNECT) || (evt == APP_EVT_VBUS_PORT_DISABLE))
            {
                /* Clear fault counters in cases where an actual disconnect has been detected */
                Cy_App_Fault_ClearCounts (port);
            }
            break;

        case APP_EVT_CONNECT:
        case APP_EVT_HARD_RESET_COMPLETE:
            break;

        case APP_EVT_PD_CONTRACT_NEGOTIATION_COMPLETE:
            break;

#if VBUS_OCP_ENABLE
        case APP_EVT_VBUS_OCP_FAULT:
            CY_APP_DEBUG_LOG(port, (port == 0 ? CY_APP_DEBUG_PD_P0_VBUS_OC : CY_APP_DEBUG_PD_P1_VBUS_OC), NULL, 0, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            app_handle_fault(context, FAULT_TYPE_VBUS_OCP);
            break;
#endif /* VBUS_OCP_ENABLE */

#if VBUS_SCP_ENABLE
        case APP_EVT_VBUS_SCP_FAULT:
            CY_APP_DEBUG_LOG(port, (port == 0 ? CY_APP_DEBUG_PD_P0_VBUS_SC : CY_APP_DEBUG_PD_P1_VBUS_SC), NULL, 0, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            app_handle_fault(context, FAULT_TYPE_VBUS_SCP);            
            break;
#endif /* VBUS_SCP_ENABLE */

#if VBUS_RCP_ENABLE
        case APP_EVT_VBUS_RCP_FAULT:
            CY_APP_DEBUG_LOG(port, (port == 0 ? CY_APP_DEBUG_PD_P0_VBUS_RC : CY_APP_DEBUG_PD_P1_VBUS_RC), NULL, 0, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            app_handle_fault(context, FAULT_TYPE_VBUS_RCP);           
            break;
#endif /* VBUS_RCP_ENABLE */

#if VBUS_OVP_ENABLE
        case APP_EVT_VBUS_OVP_FAULT:
            CY_APP_DEBUG_LOG(port, (port == 0 ? CY_APP_DEBUG_PD_P0_VBUS_OV : CY_APP_DEBUG_PD_P1_VBUS_OV), NULL, 0, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            app_handle_fault(context, FAULT_TYPE_VBUS_OVP);
            break;
#endif /* VBUS_OVP_ENABLE */

#if VBUS_UVP_ENABLE
        case APP_EVT_VBUS_UVP_FAULT:
            CY_APP_DEBUG_LOG(port, (port == 0 ? CY_APP_DEBUG_PD_P0_VBUS_UV : CY_APP_DEBUG_PD_P1_VBUS_UV), NULL, 0, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            app_handle_fault(context, FAULT_TYPE_VBUS_UVP);
            break;
#endif /* VBUS_UVP_ENABLE */

#if VCONN_OCP_ENABLE
        case APP_EVT_VCONN_OCP_FAULT:
            CY_APP_DEBUG_LOG(port, (port == 0 ? CY_APP_DEBUG_PD_P0_VCONN_OC : CY_APP_DEBUG_PD_P1_VCONN_OC), NULL, 0, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            app_handle_fault(context, FAULT_TYPE_VCONN_OCP);
            break;
#endif /* VCONN_OCP_ENABLE */

        default:
            break;
    }
#endif /* CY_APP_FAULT_HANDLER_ENABLE */
    (void) port;
    (void) evt;
    (void) dat;

    return skip_soln_cb;
}

bool Cy_App_Fault_InitVars (cy_stc_pdstack_context_t * context)
{
#if (VBUS_OVP_ENABLE || VBUS_UVP_ENABLE || VBUS_OCP_ENABLE || VBUS_SCP_ENABLE || VBUS_RCP_ENABLE || VCONN_OCP_ENABLE)
    cy_stc_usbpd_config_t * fault_config = context->ptrUsbPdContext->usbpdConfig;
    uint8_t port = context->port;
#endif /* (VBUS_OVP_ENABLE || VBUS_UVP_ENABLE || VBUS_OCP_ENABLE || VBUS_SCP_ENABLE || VBUS_RCP_ENABLE) */

#if VBUS_OVP_ENABLE
    if (fault_config->vbusOvpConfig != NULL)
    {
        glAppFaultRetryLimit[port][FAULT_TYPE_VBUS_OVP] = fault_config->vbusOvpConfig->retryCount;
    }
#endif /* VBUS_OVP_ENABLE */

#if VBUS_OCP_ENABLE
    if (fault_config->vbusOcpConfig != NULL)
    {
        glAppFaultRetryLimit[port][FAULT_TYPE_VBUS_OCP] = fault_config->vbusOcpConfig->retryCount;
    }
#endif /* VBUS_OCP_ENABLE */

#if VBUS_RCP_ENABLE
    if (fault_config->vbusRcpConfig != NULL)
    {
        glAppFaultRetryLimit[port][FAULT_TYPE_VBUS_RCP] = fault_config->vbusRcpConfig->retryCount;
    }
#endif /* VBUS_RCP_ENABLE */

#if VBUS_UVP_ENABLE
    if (fault_config->vbusUvpConfig != NULL)
    {
        glAppFaultRetryLimit[port][FAULT_TYPE_VBUS_UVP] = fault_config->vbusUvpConfig->retryCount;
    }
#endif /* VBUS_UVP_ENABLE */

#if VBUS_SCP_ENABLE
    if (fault_config->vbusScpConfig != NULL)
    {
        glAppFaultRetryLimit[port][FAULT_TYPE_VBUS_SCP] = fault_config->vbusScpConfig->retryCount;
    }
#endif /* VBUS_SCP_ENABLE */

#if VCONN_OCP_ENABLE
    if (fault_config->vconnOcpConfig != NULL)
    {
        glAppFaultRetryLimit[port][FAULT_TYPE_VCONN_OCP] = fault_config->vconnOcpConfig->retryCount;
    }
#endif /* VCONN_OCP_ENABLE */

    (void) context;
    return true;
}

void Cy_App_Fault_Task(cy_stc_pdstack_context_t * context)
{
#if CY_APP_FAULT_HANDLER_ENABLE
    uint8_t port = context->port;
    /*
     * If sink fault handling is pending, queue a port disable command.
     */
    if((Cy_App_GetPdAppStatus(port)->faultStatus & CY_APP_PORT_SINK_FAULT_ACTIVE) != 0)
    {
        if (Cy_PdStack_Dpm_SendTypecCommand (context, CY_PDSTACK_DPM_CMD_PORT_DISABLE, app_port_disable_cb) != CY_PDSTACK_STAT_BUSY)
        {
            glAppPdStatus[port].faultStatus &= ~CY_APP_PORT_SINK_FAULT_ACTIVE;
            glAppPdStatus[port].faultStatus |= CY_APP_PORT_DISABLE_IN_PROGRESS;
        }
    }
#else
    (void)context;
#endif /* CY_APP_FAULT_HANDLER_ENABLE */
}

#if VBUS_OVP_ENABLE
#define MAX_OVP_DEBOUNCE_CYCLES         (0x20u)
#endif /* VBUS_OVP_ENABLE */

/* Configure overvoltage protection checks based on the parameters in config table */
void Cy_App_Fault_OvpEnable(cy_stc_pdstack_context_t * context, uint16_t volt_mV, bool pfet, cy_cb_vbus_fault_t ovp_cb)
{
#if VBUS_OVP_ENABLE
    uint32_t intr_state;
    cy_stc_fault_vbus_ovp_cfg_t * ovp_config = (cy_stc_fault_vbus_ovp_cfg_t *) context->ptrUsbPdContext->usbpdConfig->vbusOvpConfig;

    if (ovp_config->enable)
    {
        intr_state = Cy_SysLib_EnterCriticalSection();

        if (ovp_config->mode != CY_USBPD_VBUS_OVP_MODE_ADC)
        {
            Cy_USBPD_Fault_Vbus_OvpEnable(context->ptrUsbPdContext, volt_mV, ovp_cb, pfet);
        }

        Cy_SysLib_ExitCriticalSection(intr_state);
    }
#else
    (void)context;
    (void)volt_mV;
    (void)pfet;
    (void)ovp_cb;
#endif /* VBUS_OVP_ENABLE */
}

void Cy_App_Fault_OvpDisable(cy_stc_pdstack_context_t * context, bool pfet)
{
#if VBUS_OVP_ENABLE
    cy_stc_fault_vbus_ovp_cfg_t * ovp_config = (cy_stc_fault_vbus_ovp_cfg_t *) context->ptrUsbPdContext->usbpdConfig->vbusOvpConfig;

    if (ovp_config->enable)
    {
        /* Disable OVP */
        if (ovp_config->mode != CY_USBPD_VBUS_OVP_MODE_ADC)
        {
            Cy_USBPD_Fault_Vbus_OvpDisable(context->ptrUsbPdContext, pfet);
        }
    }
#else
    (void)context;
    (void)pfet;
#endif /* VBUS_OVP_ENABLE */
}

#if VBUS_UVP_ENABLE
#define MAX_UVP_DEBOUNCE_CYCLES         (0x20u)
#endif /* VBUS_UVP_ENABLE */

/* Configure undervoltage protection checks based on the parameters in config table */
void Cy_App_Fault_UvpEnable(cy_stc_pdstack_context_t * context, uint16_t volt_mV, bool pfet, cy_cb_vbus_fault_t uvp_cb)
{
#if VBUS_UVP_ENABLE
    uint32_t intr_state;
    const cy_stc_fault_vbus_uvp_cfg_t * uvp_config = context->ptrUsbPdContext->usbpdConfig->vbusUvpConfig;

    if (uvp_config->enable)
    {
        intr_state = Cy_SysLib_EnterCriticalSection ();

        Cy_USBPD_Fault_Vbus_UvpEnable (context->ptrUsbPdContext, volt_mV, uvp_cb, pfet);

        Cy_SysLib_ExitCriticalSection (intr_state);
    }
#else
    (void)context;
    (void)volt_mV;
    (void)pfet;
    (void)uvp_cb;
#endif /* VBUS_UVP_ENABLE */
}

void Cy_App_Fault_UvpDisable(cy_stc_pdstack_context_t * context, bool pfet)
{
#if VBUS_UVP_ENABLE
    const cy_stc_fault_vbus_uvp_cfg_t * uvp_config = context->ptrUsbPdContext->usbpdConfig->vbusUvpConfig;

    /* Disable UVP */
    if (uvp_config->enable)
    {
        Cy_USBPD_Fault_Vbus_UvpDisable (context->ptrUsbPdContext, pfet);
    }
#else
    (void)context;
    (void)pfet;
#endif /* VBUS_UVP_ENABLE */
}

void Cy_App_Fault_RcpEnable(cy_stc_pdstack_context_t *context, uint16_t volt_mV, cy_cb_vbus_fault_t rcp_cb)
{
#if VBUS_RCP_ENABLE
    const cy_stc_fault_vbus_rcp_cfg_t * rcp_config = context->ptrUsbPdContext->usbpdConfig->vbusRcpConfig;
    
    if (rcp_config->enable)
    {
        Cy_USBPD_Fault_Vbus_RcpEnable(context->ptrUsbPdContext, volt_mV, rcp_cb);
    }
#else
    (void) context;
    (void)volt_mV;
    (void)rcp_cb;
#endif /* VBUS_RCP_ENABLE */
}

void Cy_App_Fault_RcpDisable(cy_stc_pdstack_context_t * context)
{
#if VBUS_RCP_ENABLE
    const cy_stc_fault_vbus_rcp_cfg_t * rcp_config = context->ptrUsbPdContext->usbpdConfig->vbusRcpConfig;
    
    if (rcp_config->enable)
    {
        Cy_USBPD_Fault_Vbus_RcpDisable (context->ptrUsbPdContext);
    }
#else
    (void) context;
#endif /* VBUS_RCP_ENABLE */
}

void Cy_App_Fault_OcpEnable(cy_stc_pdstack_context_t *context, uint32_t current, cy_cb_vbus_fault_t ocp_cb)
{
#if VBUS_OCP_ENABLE
    const cy_stc_fault_vbus_ocp_cfg_t * ocp_config = context->ptrUsbPdContext->usbpdConfig->vbusOcpConfig;
    
    if (ocp_config->enable)
    {
        Cy_USBPD_Fault_Vbus_OcpEnable (context->ptrUsbPdContext, current, ocp_cb);
    }
#else
    (void)context;
    (void)current;
    (void)ocp_cb;
#endif /* VBUS_OCP_ENABLE */
}

void Cy_App_Fault_OcpDisable(cy_stc_pdstack_context_t *context, bool pctrl)
{
#if VBUS_OCP_ENABLE
    const cy_stc_fault_vbus_ocp_cfg_t * ocp_config = context->ptrUsbPdContext->usbpdConfig->vbusOcpConfig;
    
    if (ocp_config->enable)
    {
        /* Make sure the OCP debounce timer has been stopped */
        Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, 
                                CY_PDSTACK_GET_PD_TIMER_ID(context, CY_PDSTACK_PD_OCP_DEBOUNCE_TIMER));
        Cy_USBPD_Fault_Vbus_OcpDisable (context->ptrUsbPdContext, pctrl);
    }
#else
    (void)context;
    (void)pctrl;
#endif /* VBUS_OCP_ENABLE */
}


void Cy_App_Fault_ScpEnable(cy_stc_pdstack_context_t *context, uint32_t current, cy_cb_vbus_fault_t scp_cb)
{
#if VBUS_SCP_ENABLE
    const cy_stc_fault_vbus_scp_cfg_t * scp_config = context->ptrUsbPdContext->usbpdConfig->vbusScpConfig;
    
    /* Enable SCP. */
    if (scp_config->enable)
    {
        Cy_USBPD_Fault_Vbus_ScpEnable (context->ptrUsbPdContext, current, scp_cb);
    }
#else
    (void)context;
    (void)current;
    (void)scp_cb;
#endif /* VBUS_SCP_ENABLE */
}

void Cy_App_Fault_ScpDisable(cy_stc_pdstack_context_t * context)
{
#if VBUS_SCP_ENABLE
    const cy_stc_fault_vbus_scp_cfg_t * scp_config = context->ptrUsbPdContext->usbpdConfig->vbusScpConfig;
    
    if (scp_config->enable)
    {
        Cy_USBPD_Fault_Vbus_ScpDisable(context->ptrUsbPdContext);
    }
#else
    (void) context;
#endif /* VBUS_SCP_ENABLE */
}

void Cy_App_Fault_Vconn_OcpEnable(cy_stc_pdstack_context_t *context, cy_cb_vbus_fault_t vconn_ocp_cb)
{
#if VCONN_OCP_ENABLE
    Cy_USBPD_Fault_Vconn_OcpEnable (context->ptrUsbPdContext, vconn_ocp_cb);
#else
    (void)context;
    (void)vconn_ocp_cb;
#endif /* VCONN_OCP_ENABLE */    
}

void Cy_App_Fault_Vconn_OcpDisable(cy_stc_pdstack_context_t *context)
{
#if VCONN_OCP_ENABLE
    Cy_USBPD_Fault_Vconn_OcpDisable(context->ptrUsbPdContext);
#else
    (void)context;
#endif /* VCONN_OCP_ENABLE */    
}

/* [] End of file */
