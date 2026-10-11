/***************************************************************************//**
* \file cy_app_sink.c
* \version 3.0
*
* \brief
* Implements functions associated with the power
* consumer path control and fault detection.
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

#include <stdbool.h>
#include "cy_app_config.h"
#include "cy_app_sink.h"
#include "cy_app.h"
#include "cy_app_timer_id.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_usbpd_vbus_ctrl.h"
#include "cy_app_fault_handlers.h"

#if CY_HPI_ENABLED
#include "cy_hpi.h"
#endif /* CY_HPI_ENABLED */

#ifdef CY_APP_DEFER_SNK_VBUS_UVP_HANDLING
/* Delay time (in ms) for handling the UV fault */
#define APP_PSNK_VBUS_UVP_DEFER_TIMER_PERIOD        100
#endif /* CY_APP_DEFER_SNK_VBUS_UVP_HANDLING */

/* VBUS CONSUMER FET CONTROL REGISTER bit definitions.*/
#define VBUS_CFET_CTRL_EC_CTRL_EN_POS           (0u)
/**< VBUS consumer FET control register EC control enable bit position.*/
#define VBUS_CFET_CTRL_EC_CTRL_EN               (1u << VBUS_CFET_CTRL_EC_CTRL_EN_POS)
/**< VBUS consumer FET control register EC control enable.*/
#define VBUS_CFET_CTRL_EC_CFET_ON_POS           (1u)
/**< VBUS consumer FET control register EC CFET on bit position.*/
#define VBUS_CFET_CTRL_EC_CFET_ON               (1u << VBUS_CFET_CTRL_EC_CFET_ON_POS)

static bool glAppPsnkEnabled[NO_OF_TYPEC_PORTS] = {
    false
#if (NO_OF_TYPEC_PORTS > 1)
        ,
    false
#endif /* (NO_OF_TYPEC_PORTS > 1) */
};

#if CY_HPI_VBUS_C_CTRL_ENABLE
static uint8_t gl_vbus_cfet_on_ctrl[NO_OF_TYPEC_PORTS] = {
    0
#if PMG1_PD_DUALPORT_ENABLE
    ,
    0
#endif /* PMG1_PD_DUALPORT_ENABLE */
};
#endif /* CY_HPI_VBUS_C_CTRL_ENABLE */

__attribute__ ((weak)) void soln_sink_fet_off(cy_stc_pdstack_context_t * context)
{
    CY_UNUSED_PARAMETER(context);
}

__attribute__ ((weak)) void soln_sink_fet_on(cy_stc_pdstack_context_t * context)
{
    CY_UNUSED_PARAMETER(context);
}

#if ((CY_HPI_ENABLED) && (CY_HPI_VBUS_C_CTRL_ENABLE))
static void psnk_update_cfet_status (cy_stc_pdstack_context_t *ptrPdStackContext, bool cfet_on)
{
    uint8_t port = ptrPdStackContext->port;

    /* Update the FET status and write to HPI register. */
    gl_vbus_cfet_on_ctrl[port] &= (uint8_t)(~VBUS_CFET_CTRL_EC_CFET_ON);
    gl_vbus_cfet_on_ctrl[port] |= (uint8_t)((uint8_t)cfet_on << VBUS_CFET_CTRL_EC_CFET_ON_POS);
    Cy_Hpi_UpdateConsFetStatus((cy_stc_hpi_context_t *)ptrPdStackContext->ptrHpiContext, port, gl_vbus_cfet_on_ctrl[port]);
}
#endif /* ((CY_HPI_ENABLED) && (CY_HPI_VBUS_C_CTRL_ENABLE)) */

#if (defined(CY_DEVICE_PMG1S3) && (CY_PD_SINK_ONLY) && (!CY_APP_SINK_FET_CTRL_GPIO_EN))
static void sink_fet_off_cbk (cy_timer_id_t id,  void * context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;

    Cy_USBPD_Vbus_NgdoEqCtrl (ptrPdStackContext->ptrUsbPdContext, false);
    Cy_USBPD_Vbus_NgdoOff(ptrPdStackContext->ptrUsbPdContext, false);
}
#endif /* defined(CY_DEVICE_PMG1S3) && (CY_PD_SINK_ONLY) && (!CY_APP_SINK_FET_CTRL_GPIO_EN)) */

static void sink_fet_off(cy_stc_pdstack_context_t * context)
{
#if (defined(CY_DEVICE_PMG1S3) && (CY_PD_SINK_ONLY) && (!CY_APP_SINK_FET_CTRL_GPIO_EN))
    Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, CY_APP_GET_TIMER_ID(context, CY_APP_VBUS_FET_ON_TIMER));

    Cy_USBPD_Vbus_NgdoG1Ctrl (context->ptrUsbPdContext, false);
    Cy_USBPD_Vbus_NgdoEqCtrl (context->ptrUsbPdContext, true);

    (void)Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context,
                    CY_APP_GET_TIMER_ID(context, CY_APP_VBUS_FET_OFF_TIMER),
                    CY_APP_VBUS_FET_OFF_TIMER_PERIOD, &sink_fet_off_cbk);
#elif defined(CY_DEVICE_CCG3PA)
    Cy_USBPD_Vbus_GdrvCfetOff(context->ptrUsbPdContext, CY_APP_VBUS_C_FET_CTRL);
#elif defined(CY_DEVICE_SERIES_PMG1B1)
    Cy_USBPD_Vbus_GdrvPfetOff(context->ptrUsbPdContext, false);
#else
    Cy_USBPD_Vbus_GdrvCfetOff(context->ptrUsbPdContext, false);
#endif /* (defined(CY_DEVICE_PMG1S3) && (CY_PD_SINK_ONLY) && (!CY_APP_SINK_FET_CTRL_GPIO_EN)) */

#if ((CY_HPI_ENABLED) && (CY_HPI_VBUS_C_CTRL_ENABLE))
    psnk_update_cfet_status (context, false);
#endif /* ((CY_HPI_ENABLED) && (CY_HPI_VBUS_C_CTRL_ENABLE)) */

    soln_sink_fet_off (context);
}

#if (defined(CY_DEVICE_PMG1S3) && (CY_PD_SINK_ONLY) && (!CY_APP_SINK_FET_CTRL_GPIO_EN))
void sink_fet_on_cbk (cy_timer_id_t id,  void * context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;
    /* Turn On the FET. */
    Cy_USBPD_Vbus_NgdoG1Ctrl (ptrPdStackContext->ptrUsbPdContext, true);
}
#endif /* (defined(CY_DEVICE_PMG1S3) && (CY_PD_SINK_ONLY) && (!CY_APP_SINK_FET_CTRL_GPIO_EN)) */

void Cy_App_Sink_FetOn(cy_stc_pdstack_context_t * context)
{
#if (defined(CY_DEVICE_PMG1S3) && (CY_PD_SINK_ONLY) && (!CY_APP_SINK_FET_CTRL_GPIO_EN))
    Cy_USBPD_Vbus_NgdoOn(context->ptrUsbPdContext, false);
    (void)Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context,
                        CY_APP_GET_TIMER_ID(context, CY_APP_VBUS_FET_ON_TIMER),
                        CY_APP_VBUS_FET_ON_TIMER_PERIOD, &sink_fet_on_cbk);
#elif defined(CY_DEVICE_CCG3PA)
    Cy_USBPD_Vbus_GdrvCfetOn(context->ptrUsbPdContext, CY_APP_VBUS_C_FET_CTRL);
#elif defined(CY_DEVICE_SERIES_PMG1B1)
    Cy_USBPD_Vbus_GdrvPfetOn(context->ptrUsbPdContext, false);
#else
    Cy_USBPD_Vbus_GdrvCfetOn(context->ptrUsbPdContext, false);
#endif /* (defined(CY_DEVICE_PMG1S3) && (CY_PD_SINK_ONLY) && (!CY_APP_SINK_FET_CTRL_GPIO_EN)) */

#if ((CY_HPI_ENABLED) && (CY_HPI_VBUS_C_CTRL_ENABLE))
    psnk_update_cfet_status (context, true);
#endif /* ((CY_HPI_ENABLED) && (CY_HPI_VBUS_C_CTRL_ENABLE)) */

    soln_sink_fet_on (context);
}

#if VBUS_OVP_ENABLE
static bool app_psnk_vbus_ovp_cbk(void * cbkContext, bool compOut)
{
    /* Get the PDStack context from the USB PD context */
    cy_stc_usbpd_context_t * context = (cy_stc_usbpd_context_t *) cbkContext;
    cy_stc_pdstack_context_t * pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);

    /* OVP fault */
    sink_fet_off(pdstack_ctx);

    /* Set alert message */
    cy_pd_pd_do_t alert;
    alert.val = 0u;
    alert.ado_alert.ovp = 1u;
    pdstack_ctx->dpmStat.alert = alert;

    /* Notify the application layer about the fault */
    Cy_App_EventHandler(pdstack_ctx, APP_EVT_VBUS_OVP_FAULT, NULL);

    CY_UNUSED_PARAMETER(compOut);

    return false;
}
#endif /* VBUS_OVP_ENABLE */

#if VBUS_UVP_ENABLE
#if CY_APP_DEFER_SNK_VBUS_UVP_HANDLING
static void app_psnk_vbus_uvp_tmr_cbk(cy_timer_id_t id,  void *context)
{
    CY_UNUSED_PARAMETER(id);

    /* Notify the application layer about the fault */
    Cy_App_EventHandler((cy_stc_pdstack_context_t*)context, APP_EVT_VBUS_UVP_FAULT, NULL);
}
#endif /* CY_APP_DEFER_SNK_VBUS_UVP_HANDLING */

#if(PMG1B1_USB_CHARGER == 0)
static bool app_psnk_vbus_uvp_cbk (void * context, bool compOut)
{
    cy_stc_usbpd_context_t *ptrUsbPdContext = (cy_stc_usbpd_context_t *)context;
    /* Get the PDStack context from the USB PD context */
    cy_stc_pdstack_context_t * pdstack_ctx = Cy_PdStack_Dpm_GetContext(ptrUsbPdContext->port);

    /* UVP fault */
    sink_fet_off(pdstack_ctx);

#if CY_APP_DEFER_SNK_VBUS_UVP_HANDLING
    (void)Cy_PdUtils_SwTimer_Start(pdstack_ctx->ptrTimerContext, pdstack_ctx,
            CY_APP_GET_TIMER_ID(pdstack_ctx, CY_APP_PSINK_VBUS_UVP_DEFER_TIMER),
            APP_PSNK_VBUS_UVP_DEFER_TIMER_PERIOD, &app_psnk_vbus_uvp_tmr_cbk);
#else
    /* Notify the application layer about the fault */
    Cy_App_EventHandler(pdstack_ctx, APP_EVT_VBUS_UVP_FAULT, NULL);
#endif /* DEFER_VBUS_UVP_HANDLING */

    CY_UNUSED_PARAMETER(compOut);

    return false;
}
#endif /* PMG1B1_USB_CHARGER == 0 */
#endif /* VBUS_UVP_ENABLE */

void Cy_App_Sink_SetVoltage (cy_stc_pdstack_context_t * context, uint16_t volt_mV)
{
    cy_stc_app_status_t* app_stat = Cy_App_GetStatus(context->port);
    app_stat->psnk_volt = volt_mV;

    /* Disable VBus discharge when starting off as a SINK device. */
    Cy_App_VbusDischargeOff(context);

#if VBUS_OVP_ENABLE
#if defined(CY_DEVICE_CCG3PA)
    Cy_App_Fault_OvpEnable(context, volt_mV, CY_APP_VBUS_C_FET_CTRL, &app_psnk_vbus_ovp_cbk);
#else
    Cy_App_Fault_OvpEnable(context, volt_mV, false, &app_psnk_vbus_ovp_cbk);
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
#if(PMG1B1_USB_CHARGER == 0)
    cy_stc_app_status_t* app_stat = Cy_App_GetStatus(context->port);
#if defined(CY_DEVICE_CCG3PA)
    Cy_App_Fault_UvpEnable(context, app_stat->psnk_volt, CY_APP_VBUS_C_FET_CTRL, &app_psnk_vbus_uvp_cbk);
#else
    Cy_App_Fault_UvpEnable(context, app_stat->psnk_volt, false, &app_psnk_vbus_uvp_cbk);
#endif /* defined(CY_DEVICE_CCG3PA) */
#endif /* (PMG1B1_USB_CHARGER == 0) */
#endif /* VBUS_UVP_ENABLE */

    intr_state = Cy_SysLib_EnterCriticalSection();

    /* Make sure discharge path is disabled at this stage */
    Cy_App_VbusDischargeOff(context);

    /* Turn on FETs only if dpm is enabled and there is no active fault condition. */
    if ((context->dpmConfig.dpmEnabled) && (context->dpmStat.faultActive == false))
    {
        if( (!glAppPsnkEnabled[context->port])
#if (CY_APP_SNK_STANDBY_FET_SHUTDOWN_ENABLE)
        /* Enable the sink path only if we are allowed to draw more than 0.5A of current */
            && (Cy_App_GetStatus(context->port)->psnk_cur > CY_PD_ISAFE_DEF)
#endif /* (CY_APP_SNK_STANDBY_FET_SHUTDOWN_ENABLE) */
#if (CY_HPI_VBUS_C_CTRL_ENABLE)
            /* Do not enable sink path if EC control is enabled.*/
            && (0u == (gl_vbus_cfet_on_ctrl[context->port] & VBUS_CFET_CTRL_EC_CTRL_EN))
#endif /* CY_HPI_VBUS_C_CTRL_ENABLE */
        )
        {
            glAppPsnkEnabled[context->port] = true;
#if(PMG1B1_USB_CHARGER == 0)
            Cy_App_Sink_FetOn(context);
#endif /* (PMG1B1_USB_CHARGER == 0) */
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

    switch((cy_en_timer_id_t)id)
    {
        case CY_APP_PSINK_DIS_TIMER:
            Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext,
                    CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_DIS_MONITOR_TIMER));
            Cy_App_VbusDischargeOff(context);
            break;

        case CY_APP_PSINK_DIS_MONITOR_TIMER:
            if(Cy_App_VbusIsPresent(context, CY_PD_VSAFE_5V, 0) == false)
            {
                Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext,
                        CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_DIS_TIMER));
                Cy_App_VbusDischargeOff(context);
                app_stat->snk_dis_cbk(context);
            }
            else
            {
                /*Start monitor timer again*/
                (void)Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context,
                        CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_DIS_MONITOR_TIMER),
                        CY_APP_PSINK_DIS_MONITOR_TIMER_PERIOD, &app_psnk_tmr_cbk);
            }
            break;

        default:
            /* No Operation Needed */
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

    Cy_App_VbusDischargeOff(context);
    Cy_PdUtils_SwTimer_StopRange(context->ptrTimerContext,
            CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_DIS_TIMER),
            CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_VBUS_UVP_DEFER_TIMER));

    if ((snk_discharge_off_handler != NULL) && (context->dpmConfig.dpmEnabled))
    {
        Cy_App_VbusDischargeOn(context);

        app_stat->snk_dis_cbk = snk_discharge_off_handler;

        /* Start power source enable and monitor timers */
        (void)Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context,
                CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_DIS_TIMER),
                CY_APP_PSINK_DIS_TIMER_PERIOD, &app_psnk_tmr_cbk);
        (void)Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context,
                CY_APP_GET_TIMER_ID(context, CY_APP_PSINK_DIS_MONITOR_TIMER),
                CY_APP_PSINK_DIS_MONITOR_TIMER_PERIOD, &app_psnk_tmr_cbk);
    }

    /* Update the psnk_volt data structure so that we do not have stale value till the next sink attach. */
    app_stat->psnk_volt = CY_PD_VSAFE_5V;

    Cy_SysLib_ExitCriticalSection(intr_state);
}


bool Cy_App_Sink_VbusCFetOnCtrl(cy_stc_pdstack_context_t *context, uint8_t *ctrl_p)
{
   bool ret = true;

#if CY_HPI_VBUS_C_CTRL_ENABLE
    const cy_stc_pdstack_dpm_status_t *dpm_stat = &context->dpmStat;
    const cy_stc_pd_dpm_config_t *dpm_cfg = &context->dpmConfig;
    uint8_t ctrl_val = ctrl_p[0];

    if (dpm_stat->deadBat == false)
    {
        if ((ctrl_val & VBUS_CFET_CTRL_EC_CTRL_EN) != 0u)
        {
            if (
                    (dpm_cfg->attach) &&
                    (dpm_cfg->curPortRole == CY_PD_PRT_ROLE_SINK) &&
                    (dpm_stat->faultActive == false)
               )
            {
                /* EC controlled mode while in sink connection: Turn FET ON/OFF based on command. */
                if ((ctrl_val & VBUS_CFET_CTRL_EC_CFET_ON) != 0u)
                {
                    Cy_App_Sink_FetOn(context);
                    glAppPsnkEnabled[context->port] = true;
                }
                else
                {
                    sink_fet_off (context);
                    glAppPsnkEnabled[context->port] = false;
                }
            }
            else
            {
                if ((ctrl_val & VBUS_CFET_CTRL_EC_CFET_ON) != 0u)
                {
                    /* Clear the FET ON bit to indicate that FET could not be turned ON. */
                    ctrl_val &= (uint8_t)(~VBUS_CFET_CTRL_EC_CFET_ON);
                    ret = false;
                }
            }
        }

        gl_vbus_cfet_on_ctrl[context->port] = ctrl_val;
    }
    else
    {
        /* Continue to hold previous value in the HPI register. */
        ctrl_val = gl_vbus_cfet_on_ctrl[context->port];
        ret = false;
    }

    ctrl_p[0] = ctrl_val;
#else
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(ctrl_p);
#endif /* CY_HPI_VBUS_C_CTRL_ENABLE */
    return ret;
}

/* [] END OF FILE */

