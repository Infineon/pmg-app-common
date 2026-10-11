/***************************************************************************//**
* \file cy_app_fault_handlers.c
* \version 3.0
*
* \brief
* Implements the application level fault handlers
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

#include "cy_app_fault_handlers.h"
#include "cy_app_config.h"
#if (!CY_PD_SINK_ONLY)
#include "cy_app_source.h"
#endif /* !CY_PD_SINK_ONLY */
#include "cy_app_sink.h"

#include "cy_app.h"
#include "cy_app_internal.h"
#include "cy_app_debug.h"
#include "cy_app_timer_id.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"
#include "cy_usbpd_vbus_ctrl.h"

#if BARLOW_DOCK
#include "cy_pdaltmode_intel_ridge_common.h"
#include "cy_pdaltmode_hw.h"
#endif /* BARLOW_DOCK */

#ifndef BB_GDRVO_1_TRIM_HSRCP_VAL_PMG1B1
/* Overriding RCP trim configuration value FCCM mode for PMG1B1 */
#define BB_GDRVO_1_TRIM_HSRCP_VAL_PMG1B1        (0x2Eu)
#endif /* BB_GDRVO_1_TRIM_HSRCP_VAL_PMG1B1 */

enum
{
    FAULT_TYPE_VBUS_OVP = 0u,    /* 0 */
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

#if ((VBUS_OCP_ENABLE) && (CY_APP_USE_EXTERNAL_CURRENT_MON))
void app_psrc_vbus_ocp_handler(void *context);
#endif /* ((VBUS_OCP_ENABLE) && (CY_APP_USE_EXTERNAL_CURRENT_MON) */

#endif /* (VBUS_OVP_ENABLE || VBUS_UVP_ENABLE || VBUS_OCP_ENABLE || VBUS_SCP_ENABLE || VBUS_RCP_ENABLE || VCONN_OCP_ENABLE) */

#if VBUS_OPP_ENABLE
/* Checks if Over power condition is present or not. */
bool soln_opp_present(void);

/* Over Power Protection fault handler. */
void app_psrc_vbus_opp_handler(void *context);
#endif /* VBUS_OPP_ENABLE */

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

    CY_UNUSED_PARAMETER(id);
}
#endif /* VCONN_OCP_ENABLE */


void Cy_App_VconnChangeHandler(cy_stc_pdstack_context_t *ptrPdStackContext, bool vconnOn)
{
    const cy_stc_pd_dpm_config_t *dpm_config= &ptrPdStackContext->dpmConfig;
#if VCONN_OCP_ENABLE
    uint8_t port = ptrPdStackContext->port;
#endif /* VCONN_OCP_ENABLE */

    if (!vconnOn)
    {
#if VCONN_OCP_ENABLE
        /* Store VConn fault status */
        glAppPdStatus[port].faultStatus |= (uint8_t)CY_APP_PORT_VCONN_FAULT_ACTIVE;
#endif /* VCONN_OCP_ENABLE */

        /* Ensure that the VConn switch is turned off */
        Cy_App_VconnDisable (ptrPdStackContext, dpm_config->revPol);
    }
    else
    {
#if VCONN_OCP_ENABLE
        glAppPdStatus[port].faultStatus &= ~(uint8_t)CY_APP_PORT_VCONN_FAULT_ACTIVE;
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
    for (i = 0u; i < (uint32_t)FAULT_TYPE_COUNT; i++)
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
        glAppPdStatus[port].faultStatus |= (uint8_t)CY_APP_PORT_SINK_FAULT_ACTIVE;

#if CY_APP_RTOS_ENABLED
        Cy_App_SendRtosEvent(context);
#endif /* CY_APP_RTOS_ENABLED*/
    }

    /* Stop PE */
    (void)Cy_PdStack_Dpm_PeStop(context);
}

/* Generic routine that notifies the stack about recovery actions for a fault */
static void app_handle_fault(cy_stc_pdstack_context_t * context, uint32_t fault_type)
{
    uint8_t port = context->ptrUsbPdContext->port;

    if (fault_type != (uint32_t)FAULT_TYPE_VCONN_OCP)
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

    if (glAppFaultCount[port][fault_type] < (glAppFaultRetryLimit[port][fault_type] + 1u))
    {
#if VCONN_OCP_ENABLE
        if (fault_type == (uint32_t)FAULT_TYPE_VCONN_OCP)
        {
            /* Start VConn turn OFF procedure and start a timer to restore VConn after a delay */
            Cy_App_VconnChangeHandler (context, false);
            (void)Cy_PdUtils_SwTimer_Start (context->ptrTimerContext, context, CY_PDSTACK_GET_PD_TIMER_ID(context, CY_PDSTACK_PD_VCONN_RECOVERY_TIMER),
                    CY_APP_VCONN_RECOVERY_PERIOD, &vconn_restore_timer_cb);
        }
        else
#endif /* VCONN_OCP_ENABLE */
        {
            context->peStat.hardResetCount = 0u;
            /*
             * Try a hard reset to recover from fault
             * If not successful (not in PD contract), try Type-C error recovery.
             */
            if (Cy_PdStack_Dpm_SendPdCommand(context, CY_PDSTACK_DPM_CMD_SEND_HARD_RESET, NULL, false, NULL) != CY_PDSTACK_STAT_SUCCESS)
            {
                (void)Cy_PdStack_Dpm_SendTypecCommand(context, CY_PDSTACK_DPM_CMD_TYPEC_ERR_RECOVERY, NULL);
            }
        }
    }
    else
    {
#if VCONN_OCP_ENABLE
        if (fault_type == (uint32_t)FAULT_TYPE_VCONN_OCP)
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
        if ((Cy_App_GetPdAppStatus(port)->faultStatus & (uint8_t)CY_APP_PORT_VBUS_DROP_WAIT_ACTIVE) != 0u)
        {
            glAppPdStatus[port].faultStatus &= ~(uint8_t)CY_APP_PORT_VBUS_DROP_WAIT_ACTIVE;

            /* VBus has already been removed. Enable the Rd termination to check for physical detach. */
            Cy_USBPD_TypeC_RdEnable (ptrPdStackContext->ptrUsbPdContext);
            period = CY_APP_FAULT_RECOVERY_MAX_WAIT;
        }
        else
        {
            /*
             * If VBus is not detected, we can re-enable the PD port.
             */
            glAppPdStatus[port].faultStatus &= ~(uint8_t)CY_APP_PORT_DISABLE_IN_PROGRESS;
            ptrPdStackContext->dpmStat.faultActive = false;

            Cy_USBPD_TypeC_DisableRd(ptrPdStackContext->ptrUsbPdContext, CY_PD_CC_CHANNEL_1);
            Cy_USBPD_TypeC_DisableRd(ptrPdStackContext->ptrUsbPdContext, CY_PD_CC_CHANNEL_2);
            (void)Cy_PdStack_Dpm_Start(ptrPdStackContext);

            /* Return without restarting the timer */
            return;
        }
    }

    /* Restart the timer to check VBus and Rp status again.*/
    (void)Cy_PdUtils_SwTimer_Start (ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                              CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_FAULT_RECOVERY_TIMER),
                              period, &fault_recovery_timer_cb);

    CY_UNUSED_PARAMETER(id);
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
        glAppPdStatus[port].faultStatus |= (uint8_t)CY_APP_PORT_VBUS_DROP_WAIT_ACTIVE;

#if CY_APP_RTOS_ENABLED
        Cy_App_SendRtosEvent(context);
#endif /* CY_APP_RTOS_ENABLED */
    }

    /* Provide a delay to allow VBus turn ON by port partner and then enable the port. */
    (void)Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context,
                             CY_APP_GET_TIMER_ID(context, CY_APP_FAULT_RECOVERY_TIMER),
                             period, &fault_recovery_timer_cb);

    CY_UNUSED_PARAMETER(resp);
}
#endif /* CY_APP_FAULT_HANDLER_ENABLE */

/* Clear all fault counters associated with the specified port */
void Cy_App_Fault_ClearCounts (uint8_t port)
{
#if CY_APP_FAULT_HANDLER_ENABLE
    /* Clear all fault counters on disconnect */
    (void)memset ((uint8_t *)glAppFaultCount[port], 0, FAULT_TYPE_COUNT);
#endif /* CY_APP_FAULT_HANDLER_ENABLE */

    CY_UNUSED_PARAMETER(port);
}

/* Fault-handling specific actions to be performed for various event callbacks */
bool Cy_App_Fault_EventHandler(cy_stc_pdstack_context_t * context, cy_en_pdstack_app_evt_t evt, const void *dat)
{
    bool skip_soln_cb = false;
    uint8_t port = context->port;
#if CY_APP_FAULT_HANDLER_ENABLE
    CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 16.1', 1, 'intentionally used fall through between cases.');
    CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 16.3', 1, 'intentionally used fall through between cases.');
    switch (evt)
    {
        case APP_EVT_TYPE_C_ERROR_RECOVERY:
            if (Cy_App_Fault_IsCountExceeded(context))
            {
                break;
            }
            /* Fall-through to the below case when fault counts are within limits */
            /* fall through */
        case APP_EVT_DISCONNECT:
        case APP_EVT_VBUS_PORT_DISABLE:
        case APP_EVT_HARD_RESET_SENT:
            /* Clear the port-in-fault status */
            if ((glAppPdStatus[port].faultStatus & (uint8_t)CY_APP_PORT_DISABLE_IN_PROGRESS) == 0u)
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
            (void)CY_APP_DEBUG_LOG(port, (port == 0u ? CY_APP_DEBUG_PD_P0_VBUS_OC : CY_APP_DEBUG_PD_P1_VBUS_OC), NULL, 0u, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            app_handle_fault(context, FAULT_TYPE_VBUS_OCP);
#if BARLOW_DOCK
            (void)Cy_PdAltMode_HW_SetMux(context->ptrAltModeContext, CY_PDALTMODE_MUX_CONFIG_SAFE, 0u, CY_PDALTMODE_RIDGE_STATUS_OCP_MASK);
#endif /* BARLOW_DOCK */
            break;
#endif /* VBUS_OCP_ENABLE */

#if VBUS_SCP_ENABLE
        case APP_EVT_VBUS_SCP_FAULT:
            (void)CY_APP_DEBUG_LOG(port, (port == 0u ? CY_APP_DEBUG_PD_P0_VBUS_SC : CY_APP_DEBUG_PD_P1_VBUS_SC), NULL, 0u, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            app_handle_fault(context, FAULT_TYPE_VBUS_SCP);
            break;
#endif /* VBUS_SCP_ENABLE */

#if VBUS_RCP_ENABLE
        case APP_EVT_VBUS_RCP_FAULT:
            (void)CY_APP_DEBUG_LOG(port, (port == 0u ? CY_APP_DEBUG_PD_P0_VBUS_RC : CY_APP_DEBUG_PD_P1_VBUS_RC), NULL, 0u, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            app_handle_fault(context, FAULT_TYPE_VBUS_RCP);
            break;
#endif /* VBUS_RCP_ENABLE */

#if VBUS_OVP_ENABLE
        case APP_EVT_VBUS_OVP_FAULT:
            (void)CY_APP_DEBUG_LOG(port, (port == 0u ? CY_APP_DEBUG_PD_P0_VBUS_OV : CY_APP_DEBUG_PD_P1_VBUS_OV), NULL, 0u, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            app_handle_fault(context, FAULT_TYPE_VBUS_OVP);
            break;
#endif /* VBUS_OVP_ENABLE */

#if VBUS_UVP_ENABLE
        case APP_EVT_VBUS_UVP_FAULT:
            (void)CY_APP_DEBUG_LOG(port, (port == 0u ? CY_APP_DEBUG_PD_P0_VBUS_UV : CY_APP_DEBUG_PD_P1_VBUS_UV), NULL, 0u, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            app_handle_fault(context, FAULT_TYPE_VBUS_UVP);
            break;
#endif /* VBUS_UVP_ENABLE */

#if VCONN_OCP_ENABLE
        case APP_EVT_VCONN_OCP_FAULT:
            (void)CY_APP_DEBUG_LOG(port, (port == 0u ? CY_APP_DEBUG_PD_P0_VCONN_OC : CY_APP_DEBUG_PD_P1_VCONN_OC), NULL, 0u, CY_APP_DEBUG_LOGLEVEL_ERROR, true);
            app_handle_fault(context, FAULT_TYPE_VCONN_OCP);
            break;
#endif /* VCONN_OCP_ENABLE */

        default:
            /* No Operation Needed */
            break;
    }
    CY_MISRA_BLOCK_END('MISRA C-2012 Rule 16.3');
    CY_MISRA_BLOCK_END('MISRA C-2012 Rule 16.1');
#endif /* CY_APP_FAULT_HANDLER_ENABLE */
    CY_UNUSED_PARAMETER(port);
    CY_UNUSED_PARAMETER(evt);
    CY_UNUSED_PARAMETER(dat);

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
#if CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE
        glAppFaultRetryLimit[port][FAULT_TYPE_VBUS_OVP] = 0u;
#endif /* CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE */
    }
#endif /* VBUS_OVP_ENABLE */

#if VBUS_OCP_ENABLE
    if (fault_config->vbusOcpConfig != NULL)
    {
        glAppFaultRetryLimit[port][FAULT_TYPE_VBUS_OCP] = fault_config->vbusOcpConfig->retryCount;

#if CY_APP_USE_EXTERNAL_CURRENT_MON
        glAppFaultRetryLimit[port][FAULT_TYPE_VBUS_OCP] = 0u;
#endif /* CY_APP_USE_EXTERNAL_CURRENT_MON */
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
#if CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE
        glAppFaultRetryLimit[port][FAULT_TYPE_VBUS_UVP] = 0u;
#endif /* CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE */
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

    CY_UNUSED_PARAMETER(context);
    return true;
}

#if ((CY_APP_USE_EXTERNAL_CURRENT_MON) && (VBUS_OCP_ENABLE))
__attribute__ ((weak)) bool soln_read_pm_ocp_active (cy_stc_pdstack_context_t * context)
{
    CY_UNUSED_PARAMETER(context);

    return false;
}
#endif /* ((CY_APP_USE_EXTERNAL_CURRENT_MON) && (VBUS_OCP_ENABLE)) */

void Cy_App_Fault_Task(cy_stc_pdstack_context_t * context)
{
#if CY_APP_FAULT_HANDLER_ENABLE
    uint8_t port = context->port;

#if ((CY_APP_USE_EXTERNAL_CURRENT_MON) && (VBUS_OCP_ENABLE))
    if(soln_read_pm_ocp_active (context))
    {
        app_psrc_vbus_ocp_handler(context);
    }
#endif /* (CY_APP_USE_EXTERNAL_CURRENT_MON) && (VBUS_OCP_ENABLE) */

    /*
     * If sink fault handling is pending, queue a port disable command.
     */
    if((Cy_App_GetPdAppStatus(port)->faultStatus & (uint8_t)CY_APP_PORT_SINK_FAULT_ACTIVE) != 0u)
    {
        if (Cy_PdStack_Dpm_SendTypecCommand (context, CY_PDSTACK_DPM_CMD_PORT_DISABLE, &app_port_disable_cb) != CY_PDSTACK_STAT_BUSY)
        {
            glAppPdStatus[port].faultStatus &= ~(uint8_t)CY_APP_PORT_SINK_FAULT_ACTIVE;
            glAppPdStatus[port].faultStatus |= (uint8_t)CY_APP_PORT_DISABLE_IN_PROGRESS;
        }
    }
#else
    CY_UNUSED_PARAMETER(context);
#endif /* CY_APP_FAULT_HANDLER_ENABLE */

#if VBUS_OPP_ENABLE
    /* Over-power-protection handling
     * For Port-0,
     * current port role is source,
     * PD contract is SPR
     * check if OPP occurred.
     */
    if((TYPEC_PORT_0_IDX == context->port) &&
       (false != context->dpmConfig.contractExist) &&
       (CY_PD_PRT_ROLE_SOURCE == context->dpmConfig.curPortRole) &&
       (true != context->dpmExtStat.eprActive) &&
       soln_opp_present())
    {
        app_psrc_vbus_opp_handler(context);

        /* Stop PE. */
        Cy_PdStack_Dpm_PeStop(context);
    }
#endif /* VBUS_OPP_ENABLE */
}

#if VBUS_OVP_ENABLE
#define MAX_OVP_DEBOUNCE_CYCLES         (0x20u)

__attribute__ ((weak)) void soln_fault_vbus_ovp_enable(cy_stc_pdstack_context_t * context, uint16_t volt_mV, 
                                                        bool pfet, cy_cb_vbus_fault_t ovp_cb)
{
    /* This function should be implemented by the solution to enable OVP */
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(volt_mV);
    CY_UNUSED_PARAMETER(pfet);
    CY_UNUSED_PARAMETER(&ovp_cb);
}

__attribute__ ((weak)) void soln_fault_vbus_ovp_disable(cy_stc_pdstack_context_t * context, bool pfet)
{
    /* This function should be implemented by the solution to disable OVP */
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(pfet);
}
#endif /* VBUS_OVP_ENABLE */

/* Configure overvoltage protection checks based on the parameters in config table */
void Cy_App_Fault_OvpEnable(cy_stc_pdstack_context_t * context, uint16_t volt_mV, bool pfet, cy_cb_vbus_fault_t ovp_cb)
{
#if VBUS_OVP_ENABLE
    uint32_t intr_state;
    cy_stc_fault_vbus_ovp_cfg_t * ovp_config = (cy_stc_fault_vbus_ovp_cfg_t *) context->ptrUsbPdContext->usbpdConfig->vbusOvpConfig;

    if (ovp_config->enable != 0u)
    {
        intr_state = Cy_SysLib_EnterCriticalSection();

        if (ovp_config->mode != (uint8_t)CY_USBPD_VBUS_OVP_MODE_ADC)
        {
#if CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE
            if(volt_mV > CY_PD_VSAFE_20V)
            {
                volt_mV = ((volt_mV * EPR_MAX_VOLTAGE_DIV_RATIO) / EPR_PERCENTAGE) - EPR_MAX_VOLTAGE_FET_VGS_MIN_THRESHOLD_IN_MILLI_VOLT;
            }
#endif /* CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE */
            Cy_USBPD_Fault_Vbus_OvpEnable(context->ptrUsbPdContext, volt_mV, ovp_cb, pfet);
        }
        else
        {
            soln_fault_vbus_ovp_enable(context, volt_mV, pfet, ovp_cb);
        }

        Cy_SysLib_ExitCriticalSection(intr_state);
    }
#else
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(volt_mV);
    CY_UNUSED_PARAMETER(pfet);
    CY_UNUSED_PARAMETER(&ovp_cb);
#endif /* VBUS_OVP_ENABLE */
}

void Cy_App_Fault_OvpDisable(cy_stc_pdstack_context_t * context, bool pfet)
{
#if VBUS_OVP_ENABLE
    cy_stc_fault_vbus_ovp_cfg_t * ovp_config = (cy_stc_fault_vbus_ovp_cfg_t *) context->ptrUsbPdContext->usbpdConfig->vbusOvpConfig;

    if (ovp_config->enable != 0u)
    {
        /* Disable OVP */
        if (ovp_config->mode != (uint8_t)CY_USBPD_VBUS_OVP_MODE_ADC)
        {
            Cy_USBPD_Fault_Vbus_OvpDisable(context->ptrUsbPdContext, pfet);
        }
        else
        {
            soln_fault_vbus_ovp_disable(context, pfet);
        }
    }
#else
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(pfet);
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

    if (uvp_config->enable != 0u)
    {
        intr_state = Cy_SysLib_EnterCriticalSection ();

#if CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE
        if(volt_mV > CY_PD_VSAFE_20V)
        {
            volt_mV = ((volt_mV * EPR_MAX_VOLTAGE_DIV_RATIO) / EPR_PERCENTAGE) - EPR_MAX_VOLTAGE_FET_VGS_MIN_THRESHOLD_IN_MILLI_VOLT;
        }
#endif /* CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE */
        Cy_USBPD_Fault_Vbus_UvpEnable (context->ptrUsbPdContext, volt_mV, uvp_cb, pfet);

        Cy_SysLib_ExitCriticalSection (intr_state);
    }
#else
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(volt_mV);
    CY_UNUSED_PARAMETER(pfet);
    CY_UNUSED_PARAMETER(&uvp_cb);
#endif /* VBUS_UVP_ENABLE */
}

void Cy_App_Fault_UvpDisable(cy_stc_pdstack_context_t * context, bool pfet)
{
#if VBUS_UVP_ENABLE
    const cy_stc_fault_vbus_uvp_cfg_t * uvp_config = context->ptrUsbPdContext->usbpdConfig->vbusUvpConfig;

    /* Disable UVP */
    if (uvp_config->enable != 0u)
    {
        Cy_USBPD_Fault_Vbus_UvpDisable (context->ptrUsbPdContext, pfet);
    }
#else
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(pfet);
#endif /* VBUS_UVP_ENABLE */
}

void Cy_App_Fault_RcpEnable(cy_stc_pdstack_context_t *context, uint16_t volt_mV, cy_cb_vbus_fault_t rcp_cb)
{
#if VBUS_RCP_ENABLE
    const cy_stc_fault_vbus_rcp_cfg_t * rcp_config = context->ptrUsbPdContext->usbpdConfig->vbusRcpConfig;
    
    if (rcp_config->enable != 0u)
    {
        Cy_USBPD_Fault_Vbus_RcpEnable(context->ptrUsbPdContext, volt_mV, rcp_cb);
    }
#else
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(volt_mV);
    CY_UNUSED_PARAMETER(&rcp_cb);
#endif /* VBUS_RCP_ENABLE */
}

void Cy_App_Fault_RcpDisable(cy_stc_pdstack_context_t * context)
{
#if VBUS_RCP_ENABLE
    const cy_stc_fault_vbus_rcp_cfg_t * rcp_config = context->ptrUsbPdContext->usbpdConfig->vbusRcpConfig;
    
    if (rcp_config->enable != 0u)
    {
        Cy_USBPD_Fault_Vbus_RcpDisable (context->ptrUsbPdContext);
    }
#else
    CY_UNUSED_PARAMETER(context);
#endif /* VBUS_RCP_ENABLE */
}

void Cy_App_Fault_Voutbb_RcpEnable(cy_stc_pdstack_context_t *context, cy_cb_vbus_fault_t rcp_cb)
{
#if VBUS_RCP_ENABLE && PMG1B1_BI_DIRECTION
    const cy_stc_fault_vbus_rcp_cfg_t * rcp_config = context->ptrUsbPdContext->usbpdConfig->vbusRcpConfig;
    PPDSS_TRIMS_REGS_T pd_trims = (PPDSS_TRIMS_REGS_T)context->ptrUsbPdContext->trimsBase;

    if (rcp_config->enable)
    {
        CY_USBPD_REG_FIELD_UPDATE(pd_trims->trim_bb_gdrvo_1,
                                  PDSS_TRIM_BB_GDRVO_1_TRIM_HSRCP,
                                  BB_GDRVO_1_TRIM_HSRCP_VAL_PMG1B1);
        Cy_USBPD_Fault_Voutbb_RcpEnable(context->ptrUsbPdContext, rcp_cb);
    }
#else
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(&rcp_cb);
#endif /* VBUS_RCP_ENABLE */
}

void Cy_App_Fault_Voutbb_RcpDisable(cy_stc_pdstack_context_t * context)
{
#if VBUS_RCP_ENABLE && PMG1B1_BI_DIRECTION
    const cy_stc_fault_vbus_rcp_cfg_t * rcp_config = context->ptrUsbPdContext->usbpdConfig->vbusRcpConfig;

    if (rcp_config->enable)
    {
        Cy_USBPD_Fault_Voutbb_RcpDisable(context->ptrUsbPdContext);
    }
#else
    CY_UNUSED_PARAMETER(context);
#endif /* VBUS_RCP_ENABLE */
}

void Cy_App_Fault_OcpEnable(cy_stc_pdstack_context_t *context, uint32_t current, cy_cb_vbus_fault_t ocp_cb)
{
#if VBUS_OCP_ENABLE
    const cy_stc_fault_vbus_ocp_cfg_t * ocp_config = context->ptrUsbPdContext->usbpdConfig->vbusOcpConfig;
    
    if (ocp_config->enable != 0u)
    {
        Cy_USBPD_Fault_Vbus_OcpEnable (context->ptrUsbPdContext, current, ocp_cb);
    }
#else
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(current);
    CY_UNUSED_PARAMETER(&ocp_cb);
#endif /* VBUS_OCP_ENABLE */
}

void Cy_App_Fault_OcpDisable(cy_stc_pdstack_context_t *context, bool pctrl)
{
#if VBUS_OCP_ENABLE
    const cy_stc_fault_vbus_ocp_cfg_t * ocp_config = context->ptrUsbPdContext->usbpdConfig->vbusOcpConfig;
    
    if (ocp_config->enable != 0u)
    {
        /* Make sure the OCP debounce timer has been stopped */
        Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, 
                                CY_PDSTACK_GET_PD_TIMER_ID(context, CY_PDSTACK_PD_OCP_DEBOUNCE_TIMER));
        Cy_USBPD_Fault_Vbus_OcpDisable (context->ptrUsbPdContext, pctrl);
    }
#else
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(pctrl);
#endif /* VBUS_OCP_ENABLE */
}


void Cy_App_Fault_ScpEnable(cy_stc_pdstack_context_t *context, uint32_t current, cy_cb_vbus_fault_t scp_cb)
{
#if VBUS_SCP_ENABLE
    const cy_stc_fault_vbus_scp_cfg_t * scp_config = context->ptrUsbPdContext->usbpdConfig->vbusScpConfig;
    
    /* Enable SCP. */
    if (scp_config->enable != 0u)
    {
        Cy_USBPD_Fault_Vbus_ScpEnable (context->ptrUsbPdContext, current, scp_cb);
    }
#else
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(current);
    CY_UNUSED_PARAMETER(scp_cb);
#endif /* VBUS_SCP_ENABLE */
}

void Cy_App_Fault_ScpDisable(cy_stc_pdstack_context_t * context)
{
#if VBUS_SCP_ENABLE
    const cy_stc_fault_vbus_scp_cfg_t * scp_config = context->ptrUsbPdContext->usbpdConfig->vbusScpConfig;
    
    if (scp_config->enable != 0u)
    {
        Cy_USBPD_Fault_Vbus_ScpDisable(context->ptrUsbPdContext);
    }
#else
    CY_UNUSED_PARAMETER(context);
#endif /* VBUS_SCP_ENABLE */
}

void Cy_App_Fault_Vconn_OcpEnable(cy_stc_pdstack_context_t *context, cy_cb_vbus_fault_t vconn_ocp_cb)
{
#if VCONN_OCP_ENABLE
    const cy_stc_fault_vconn_ocp_cfg_t * vconn_ocp_config = context->ptrUsbPdContext->usbpdConfig->vconnOcpConfig;

    if(vconn_ocp_config->enable != 0u)
    {
        Cy_USBPD_Fault_Vconn_OcpEnable (context->ptrUsbPdContext, vconn_ocp_cb);
    }
#else
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(vconn_ocp_cb);
#endif /* VCONN_OCP_ENABLE */
}

void Cy_App_Fault_Vconn_OcpDisable(cy_stc_pdstack_context_t *context)
{
#if VCONN_OCP_ENABLE
    const cy_stc_fault_vconn_ocp_cfg_t * vconn_ocp_config = context->ptrUsbPdContext->usbpdConfig->vconnOcpConfig;

    if(vconn_ocp_config->enable != 0u)
    {
        Cy_USBPD_Fault_Vconn_OcpDisable(context->ptrUsbPdContext);
    }
#else
    CY_UNUSED_PARAMETER(context);
#endif /* VCONN_OCP_ENABLE */
}

cy_en_pdstack_status_t Cy_App_Fault_DisablePort(cy_stc_pdstack_context_t* ptrPdStackContext, cy_pdstack_dpm_typec_cmd_cbk_t cbk)
{
    cy_en_pdstack_status_t retval = CY_PDSTACK_STAT_SUCCESS;
    uint8_t port = ptrPdStackContext->port;

    if (Cy_PdUtils_SwTimer_IsRunning(ptrPdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_FAULT_RECOVERY_TIMER)))
    {
        /* If the HPI master is asking us to disable the port, make sure all fault protection state is cleared. */
        glAppPdStatus[port].faultStatus &= (uint8_t)(~(
                (uint8_t)CY_APP_PORT_VBUS_DROP_WAIT_ACTIVE | (uint8_t)CY_APP_PORT_SINK_FAULT_ACTIVE | (uint8_t)CY_APP_PORT_DISABLE_IN_PROGRESS |
                (uint8_t)CY_APP_PORT_VCONN_FAULT_ACTIVE | (uint8_t)CY_APP_PORT_V5V_SUPPLY_LOST));

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
            retval = Cy_PdStack_Dpm_SendTypecCommand(ptrPdStackContext, CY_PDSTACK_DPM_CMD_PORT_DISABLE, cbk);
        }
        else
        {
            cbk(ptrPdStackContext, CY_PDSTACK_DPM_RESP_SUCCESS);
        }
    }

    return retval;
}

/* [] End of file */
