/***************************************************************************//**
* \file cy_app_sink.c
* \version 1.0
*
* \brief
* Implements functions associated with the power
* consumer path control and fault detection.
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
#include "cy_app_sink.h"
#include "cy_app.h"
#include "cy_app_timer_id.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"
#include "cy_usbpd_vbus_ctrl.h"
#include "cy_app_fault_handlers.h"

#ifdef CY_APP_DEFER_SNK_VBUS_UVP_HANDLING
/* Delay time (in ms) for handling the UV fault */
#define APP_PSNK_VBUS_UVP_DEFER_TIMER_PERIOD        100
#endif /* CY_APP_DEFER_SNK_VBUS_UVP_HANDLING */

bool glAppPsnkEnabled[NO_OF_TYPEC_PORTS] = {
    false
#if (NO_OF_TYPEC_PORTS > 1)
        ,
    false
#endif /* (NO_OF_TYPEC_PORTS > 1) */
};

bool app_psnk_vbus_ovp_cbk(void * cbkContext, bool comp_out);

__attribute__ ((weak)) void soln_sink_fet_off(cy_stc_pdstack_context_t * context)
{
    (void) context;
}

__attribute__ ((weak)) void soln_sink_fet_on(cy_stc_pdstack_context_t * context)
{
    (void)context;
}

void sink_fet_off(cy_stc_pdstack_context_t * context)
{
#if defined(CY_DEVICE_CCG3PA)
    Cy_USBPD_Vbus_GdrvCfetOff(context->ptrUsbPdContext, CY_APP_VBUS_FET_CTRL);
#else
    Cy_USBPD_Vbus_GdrvCfetOff(context->ptrUsbPdContext, false);
#endif /* defined (CY_DEVICE_CCG3PA) */
    soln_sink_fet_off (context);
}

void sink_fet_on(cy_stc_pdstack_context_t * context)
{
#if defined(CY_DEVICE_CCG3PA)
    Cy_USBPD_Vbus_GdrvCfetOn(context->ptrUsbPdContext, CY_APP_VBUS_FET_CTRL);
#else
    Cy_USBPD_Vbus_GdrvCfetOn(context->ptrUsbPdContext, false);
#endif /* defined (CY_DEVICE_CCG3PA) */

    soln_sink_fet_on (context);
}

#if VBUS_OVP_ENABLE
bool app_psnk_vbus_ovp_cbk(void * cbkContext, bool comp_out)
{
    /* Get the PDStack context from the USB PD context */
    cy_stc_usbpd_context_t * context = (cy_stc_usbpd_context_t *) cbkContext;
    cy_stc_pdstack_context_t * pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);

    /* OVP fault */
    sink_fet_off(pdstack_ctx);

    /* Set alert message */
    cy_pd_pd_do_t alert;
    alert.val = 0;
    alert.ado_alert.ovp = true;
    pdstack_ctx->dpmStat.alert = alert;

    /* Notify the application layer about the fault */
    Cy_App_EventHandler(pdstack_ctx, APP_EVT_VBUS_OVP_FAULT, NULL);

    (void)comp_out;

    return 0;
}
#endif /* VBUS_OVP_ENABLE */

#if VBUS_UVP_ENABLE
#if CY_APP_DEFER_SNK_VBUS_UVP_HANDLING
static void app_psnk_vbus_uvp_tmr_cbk(cy_timer_id_t id,  void *context)
{
    (void)id;

    /* Notify the application layer about the fault */
    Cy_App_EventHandler((cy_stc_pdstack_context_t*)context, APP_EVT_VBUS_UVP_FAULT, NULL);
}
#endif /* CY_APP_DEFER_SNK_VBUS_UVP_HANDLING */

bool app_psnk_vbus_uvp_cbk (void * context, bool comp_out)
{
    cy_stc_usbpd_context_t *ptrUsbPdContext = (cy_stc_usbpd_context_t *)context;
    /* Get the PDStack context from the USB PD context */
    cy_stc_pdstack_context_t * pdstack_ctx = Cy_PdStack_Dpm_GetContext(ptrUsbPdContext->port);

    /* UVP fault */
    sink_fet_off(pdstack_ctx);

#if CY_APP_DEFER_SNK_VBUS_UVP_HANDLING
    Cy_PdUtils_SwTimer_Start(pdstack_ctx->ptrTimerContext, pdstack_ctx,
            CY_APP_GET_TIMER_ID(pdstack_ctx, CY_APP_PSINK_VBUS_UVP_DEFER_TIMER),
            APP_PSNK_VBUS_UVP_DEFER_TIMER_PERIOD, app_psnk_vbus_uvp_tmr_cbk);
#else
    /* Notify the application layer about the fault */
    Cy_App_EventHandler(pdstack_ctx, APP_EVT_VBUS_UVP_FAULT, NULL);
#endif /* DEFER_VBUS_UVP_HANDLING */

    (void)comp_out;

    return 0;
}
#endif /* VBUS_UVP_ENABLE */

void Cy_App_Sink_SetVoltage (cy_stc_pdstack_context_t * context, uint16_t volt_mV)
{
    cy_stc_app_status_t* app_stat = Cy_App_GetStatus(context->port);
    app_stat->psnk_volt = volt_mV;

    /* Disable VBus discharge when starting off as a SINK device. */
    Cy_USBPD_Vbus_DischargeOff(context->ptrUsbPdContext);

#if VBUS_OVP_ENABLE
#if defined(CY_DEVICE_CCG3PA)
    Cy_App_Fault_OvpEnable(context, volt_mV, CY_APP_VBUS_FET_CTRL, app_psnk_vbus_ovp_cbk);
#else
    Cy_App_Fault_OvpEnable(context, volt_mV, false, app_psnk_vbus_ovp_cbk);
#endif /* defined(CY_DEVICE_CCG3PA) */
#endif /* VBUS_OVP_ENABLE */
}

void Cy_App_Sink_SetCurrent (cy_stc_pdstack_context_t * context, uint16_t cur_10mA)
{
    cy_stc_app_status_t* app_stat = Cy_App_GetStatus(context->port);

    /*
     * There is no implementation to update the current settings at present.
     * We are just storing the current value into a variable. This implementation
     * needs to be updated when the PMG1 solution has capability to control the
     * sink current capability.
     */
    app_stat->psnk_cur = cur_10mA;
    if (cur_10mA <= CY_PD_ISAFE_DEF)
    {
        /* Notify the application layer to reduce current consumption to standby current */
        Cy_App_EventHandler(context, APP_EVT_STANDBY_CURRENT, NULL);

#if CY_APP_SNK_STANDBY_FET_SHUTDOWN_ENABLE
        /* Turn off the Sink FET if not in dead battery condition */
        if (context->dpmStat.deadBat == false)
        {
            sink_fet_off (context);
            glAppPsnkEnabled[context->port] = false;
        }
#endif /* CY_APP_SNK_STANDBY_FET_SHUTDOWN_ENABLE */
    }
}

void Cy_App_Sink_Enable (cy_stc_pdstack_context_t * context)
{
    uint32_t intr_state;

#if VBUS_UVP_ENABLE
    cy_stc_app_status_t* app_stat = Cy_App_GetStatus(context->port);
#if defined(CY_DEVICE_CCG3PA)
    Cy_App_Fault_UvpEnable(context, app_stat->psnk_volt, CY_APP_VBUS_FET_CTRL, app_psnk_vbus_uvp_cbk);
#else
    Cy_App_Fault_UvpEnable(context, app_stat->psnk_volt, false, app_psnk_vbus_uvp_cbk);
#endif /* defined(CY_DEVICE_CCG3PA) */
#endif /* VBUS_UVP_ENABLE */

    intr_state = Cy_SysLib_EnterCriticalSection();

    /* Make sure discharge path is disabled at this stage */
    Cy_USBPD_Vbus_DischargeOff(context->ptrUsbPdContext);

    /* Turn on FETs only if dpm is enabled and there is no active fault condition. */
    if ((context->dpmConfig.dpmEnabled) && (context->dpmStat.faultActive == false))
    {
#if (CY_APP_SNK_STANDBY_FET_SHUTDOWN_ENABLE)
        /* Enable the sink path only if we are allowed to draw more than 0.5A of current */
        if (Cy_App_GetStatus(context->port)->psnk_cur > CY_PD_ISAFE_DEF)
#endif /* (CY_APP_SNK_STANDBY_FET_SHUTDOWN_ENABLE) */
        {
            if (!glAppPsnkEnabled[context->port])
            {
                glAppPsnkEnabled[context->port] = true;
                sink_fet_on(context);
            }
        }
    }

    Cy_SysLib_ExitCriticalSection(intr_state);
}

/* Timer callback */
static void app_psnk_tmr_cbk(cy_timer_id_t id,  void * callbackCtx)
{
    cy_stc_pdstack_context_t * context = callbackCtx;
    uint8_t port = context->port;
    cy_stc_app_status_t* app_stat = Cy_App_GetStatus(port);

    if (context->port != 0u)
    {
        id = (cy_timer_id_t)((id & 0x00FFU) + CY_PDUTILS_TIMER_APP_PORT0_START_ID);
    }

    switch((cy_timer_id_t)id)
    {
        case CY_APP_PSINK_DIS_TIMER:
            Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext,
                    CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_DIS_MONITOR_TIMER));
            Cy_USBPD_Vbus_DischargeOff(context->ptrUsbPdContext);
            break;

        case CY_APP_PSINK_DIS_MONITOR_TIMER:
            if(Cy_App_VbusIsPresent(context, CY_PD_VSAFE_5V, 0) == false)
            {
                Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext,
                        CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_DIS_TIMER));
                Cy_USBPD_Vbus_DischargeOff(context->ptrUsbPdContext);
                app_stat->snk_dis_cbk(context);
            }
            else
            {
                /*Start monitor timer again*/
                Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context,
                        CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_DIS_MONITOR_TIMER),
                        CY_APP_PSINK_DIS_MONITOR_TIMER_PERIOD, app_psnk_tmr_cbk);
            }
            break;

        default:
            break;
    }
}

void Cy_App_Sink_Disable (cy_stc_pdstack_context_t * context, cy_pdstack_sink_discharge_off_cbk_t snk_discharge_off_handler)
{
    uint32_t intr_state;
    uint8_t port = context->port;
    cy_stc_app_status_t* app_stat = Cy_App_GetStatus(port);

    intr_state = Cy_SysLib_EnterCriticalSection();

#if VBUS_OVP_ENABLE
    Cy_App_Fault_OvpDisable (context, false);
#endif /* VBUS_OVP_ENABLE */

#if VBUS_UVP_ENABLE
    Cy_App_Fault_UvpDisable (context, false);
#endif /* VBUS_UVP_ENABLE */

    sink_fet_off(context);
    glAppPsnkEnabled[port] = false;

    Cy_USBPD_Vbus_DischargeOff(context->ptrUsbPdContext);
    Cy_PdUtils_SwTimer_StopRange(context->ptrTimerContext,
            CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_DIS_TIMER),
            CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_VBUS_UVP_DEFER_TIMER));

    if ((snk_discharge_off_handler != NULL) && (context->dpmConfig.dpmEnabled))
    {
        Cy_USBPD_Vbus_DischargeOn(context->ptrUsbPdContext);

        app_stat->snk_dis_cbk = snk_discharge_off_handler;

        /* Start power source enable and monitor timers */
        Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context,
                CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_DIS_TIMER),
                CY_APP_PSINK_DIS_TIMER_PERIOD, app_psnk_tmr_cbk);
        Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context,
                CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_DIS_MONITOR_TIMER),
                CY_APP_PSINK_DIS_MONITOR_TIMER_PERIOD, app_psnk_tmr_cbk);
    }

    /* Update the psnk_volt data structure so that we do not have stale value till the next sink attach. */
    app_stat->psnk_volt = CY_PD_VSAFE_5V;

    Cy_SysLib_ExitCriticalSection(intr_state);
}

/* [] END OF FILE */

