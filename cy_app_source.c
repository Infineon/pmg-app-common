/***************************************************************************//**
* \file cy_app_source.c
* \version 2.0
*
* \brief
* Implements functions associated with the power
* provider path control and fault detection.
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
#include "cy_app.h"
#include "cy_app_config.h"
#include "cy_app_sink.h"
#include "cy_app_source.h"
#include "cy_app_timer_id.h"
#include "cy_app_fault_handlers.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"
#include "cy_pdstack_dpm.h"
#include "cy_usbpd_vbus_ctrl.h"

/* Type-C current levels in 10 mA units */
#define CUR_LEVEL_3A    300
#define CUR_LEVEL_1_5A  150
#define CUR_LEVEL_DEF   90

/* VBUS absolute maximum voltage in mV units */
#if CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE
#define VBUS_MAX_VOLTAGE  (50000u)
#else
#define VBUS_MAX_VOLTAGE  (30000u)
#endif

#if (VBUS_OCP_ENABLE)
static const uint32_t cc_rp_to_cur_map[] = {
    CUR_LEVEL_DEF,
    CUR_LEVEL_1_5A,
    CUR_LEVEL_3A
};
#endif /* (VBUS_OCP_ENABLE) */

static void psrc_shutdown(cy_stc_pdstack_context_t * context, bool discharge_dis);

#if VBUS_SOFT_START_ENABLE

bool gl_fet_soft_start_en[NO_OF_TYPEC_PORTS] = {false};
void ocp_handler_wrapper(cy_timer_id_t id,  void *cbkContext);

#endif /* VBUS_SOFT_START_ENABLE */

void app_psrc_tmr_cbk(cy_timer_id_t id,  void * callbackCtx);
bool app_psrc_vbus_ovp_cbk(void *context, bool compOut);
bool app_psrc_vbus_ocp_cbk(void * cbkContext, bool comp_out);
bool app_psrc_vbus_scp_cbk(void * cbkContext, bool comp_out);
bool app_psrc_vbus_rcp_cbk(void * cbkContext, bool comp_out);

void psrc_select_voltage(cy_stc_pdstack_context_t *context);

#if (defined(CY_DEVICE_PMG1S3) && (!CY_PD_SINK_ONLY))
void vbus_fet_on_cbk (cy_timer_id_t id,  void * context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;

    /* Turn On the FET */
    Cy_USBPD_Vbus_NgdoG1Ctrl (ptrPdStackContext->ptrUsbPdContext, true);

    (void) id;
}
#endif /* (defined(CY_DEVICE_PMG1S3) && (!CY_PD_SINK_ONLY)) */

#if VBUS_SOFT_START_ENABLE
static void Vbus_NgdoSoftStartOn(cy_stc_pdstack_context_t *context)
{
    /* Disable the OCP fault detection module */
    Cy_USBPD_Fault_Vbus_OcpDisable(context->ptrUsbPdContext, true);

    /* Set the OCP threshold to 200 mA for soft start */
    Cy_USBPD_Fault_Vbus_OcpEnable(context->ptrUsbPdContext, 20, app_psrc_vbus_ocp_cbk);

    gl_fet_soft_start_en[context->port] = true;

    /* Sets the drive strength of the NGDO to HIGH (1.05 uA) */
    Cy_USBPD_Vbus_NgdoSetDriveStrength(context->ptrUsbPdContext, (uint8_t)0x07);

    /* Start the timer which will change drive strength and OCP settings to default after a timeout */
    Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_FET_SOFT_START_TIMER_ID), 50, ocp_handler_wrapper);
}
#endif /* VBUS_SOFT_START_ENABLE */

__attribute__ ((weak)) void soln_vbus_fet_on (cy_stc_pdstack_context_t *context)
{
    (void)context;
}

__attribute__ ((weak)) void soln_vbus_fet_off (cy_stc_pdstack_context_t *context)
{
    (void)context;
}

static void vbus_fet_on(cy_stc_pdstack_context_t *context)
{
    /* If FET is already on then no need to enable it again */    
    if(Cy_App_GetStatus(context->port)->is_vbus_on == false)
    {
        Cy_App_GetStatus(context->port)->is_vbus_on = true;

        /*
         * In case of CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME, the regulator is
         * already turned on. 
         * Turn off sink FET causes the regulator to get wrongly
         * shutdown and disables the sink.
         */
#if (!(CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME))
        Cy_USBPD_Vbus_GdrvCfetOff(context->ptrUsbPdContext, false);
        Cy_SysLib_DelayUs(10);
#endif /* (!(CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME)) */

#if (defined(CY_DEVICE_PMG1S3) && (!CY_PD_SINK_ONLY))
        Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, CY_APP_GET_TIMER_ID(context, CY_APP_VBUS_FET_OFF_TIMER));
#if VBUS_SOFT_START_ENABLE
        Vbus_NgdoSoftStartOn(context);
#endif /* VBUS_SOFT_START_ENABLE */
#endif /* (defined(CY_DEVICE_PMG1S3) && (!CY_PD_SINK_ONLY)) */

#if defined(CY_DEVICE_CCG3PA)
        Cy_USBPD_Vbus_GdrvPfetOn(context->ptrUsbPdContext, CY_APP_VBUS_P_FET_CTRL);
#else
        Cy_USBPD_Vbus_GdrvPfetOn(context->ptrUsbPdContext, true);
#endif /* defined(CY_DEVICE_CCG3PA) */

#if (defined(CY_DEVICE_PMG1S3) && (!CY_PD_SINK_ONLY))
        Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_VBUS_FET_ON_TIMER),
                CY_APP_VBUS_FET_ON_TIMER_PERIOD, vbus_fet_on_cbk);
#endif /* (defined(CY_DEVICE_PMG1S3) && (!CY_PD_SINK_ONLY)) */
    }

    soln_vbus_fet_on (context);
}

#if (defined(CY_DEVICE_PMG1S3) && (!CY_PD_SINK_ONLY))
void vbus_fet_off_cbk (cy_timer_id_t id,  void * context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;

    Cy_USBPD_Vbus_NgdoEqCtrl (ptrPdStackContext->ptrUsbPdContext, false);

    Cy_USBPD_Vbus_GdrvPfetOff(ptrPdStackContext->ptrUsbPdContext, true);

    (void) id;
}
#endif /* (defined(CY_DEVICE_PMG1S3) && (!CY_PD_SINK_ONLY))*/

static void vbus_fet_off(cy_stc_pdstack_context_t *context)
{
    Cy_App_GetStatus(context->port)->is_vbus_on = false;

#if (defined(CY_DEVICE_PMG1S3) && (!CY_PD_SINK_ONLY))
    /* Stop the VBUS_FET_ON_TIMER */
    Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, CY_APP_GET_TIMER_ID(context, CY_APP_VBUS_FET_ON_TIMER));

    Cy_USBPD_Vbus_NgdoG1Ctrl (context->ptrUsbPdContext, false);
    Cy_USBPD_Vbus_NgdoEqCtrl (context->ptrUsbPdContext, true);

    Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_VBUS_FET_OFF_TIMER),
            CY_APP_VBUS_FET_OFF_TIMER_PERIOD, vbus_fet_off_cbk);
#elif defined(CY_DEVICE_CCG3PA)
    Cy_USBPD_Vbus_GdrvPfetOff(context->ptrUsbPdContext, CY_APP_VBUS_P_FET_CTRL);
#else
    Cy_USBPD_Vbus_GdrvPfetOff(context->ptrUsbPdContext, true);
#endif /* (defined(CY_DEVICE_PMG1S3) && (!CY_PD_SINK_ONLY)) */

    soln_vbus_fet_off (context);
}

static void call_psrc_ready_cbk(cy_stc_pdstack_context_t * context)
{
    cy_stc_app_status_t* app_stat = Cy_App_GetStatus(context->port);

    if (app_stat->pwr_ready_cbk != NULL)
    {
        app_stat->pwr_ready_cbk (context);
        app_stat->pwr_ready_cbk = NULL;
    }
}

/*Timer callback*/
void app_psrc_tmr_cbk(cy_timer_id_t id,  void * callbackCtx)
{
    cy_stc_pdstack_context_t* context = callbackCtx;
    cy_stc_app_status_t* app_stat = Cy_App_GetStatus(context->port);

    if (context->port != 0u)
    {
        id = (cy_timer_id_t)((id & 0x00FFU) + CY_PDUTILS_TIMER_APP_PORT0_START_ID);
    }

    switch(id)
    {
        case CY_APP_PSOURCE_EN_TIMER:
            /* If the Supply did not reach expected level, turn off the power and do error recovery. */
            Cy_PdUtils_SwTimer_StopRange(context->ptrTimerContext, 
                                         CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_MONITOR_TIMER), 
                                         CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_HYS_TIMER));

            app_stat->psrc_volt_old = CY_PD_VSAFE_0V;
            psrc_shutdown(context, true);

#if (VBUS_UVP_ENABLE)
            /*
             * In an undervoltage condition, if the VBUS does not reach VSAFE5V.
             * Since the UVP hardware detection cannot be
             * enabled when turning on the VBUS, this has to be manually triggered
             * from here by invoking the callback directly. Do this only if UVP is
             * enabled from the configuration table.
             */
            if (context->ptrUsbPdContext->usbpdConfig->vbusOvpConfig->enable)
            {
                app_psrc_vbus_ovp_cbk(context, false);
            }
#endif /* (VBUS_UVP_ENABLE) */
            break;

        case CY_APP_PSOURCE_EN_MONITOR_TIMER:
            if (((app_stat->psrc_rising == true) &&
                        (Cy_App_VbusIsPresent(context, app_stat->psrc_volt, CY_APP_VBUS_TURN_ON_MARGIN) == true)) ||
                    ((app_stat->psrc_rising == false) &&
                     (Cy_App_VbusIsPresent(context, app_stat->psrc_volt, CY_APP_VBUS_DISCHARGE_MARGIN) == false))
               )
            {
#if CY_PD_EPR_AVS_ENABLE
                if(context->dpmExtStat.eprAvsMode == CY_PDSTACK_EPR_AVS_SMALL)
                {
                    /* Start source enable AVS hysteresis timer */
                    Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_HYS_TIMER),
                            CY_APP_PSOURCE_AVS_EN_HYS_TIMER_PERIOD, app_psrc_tmr_cbk);
                }
                else
#endif /* CY_PD_EPR_AVS_ENABLE */
                {
                    /* Start source enable hysteresis timer */
                    Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_HYS_TIMER),
                            CY_APP_PSOURCE_EN_HYS_TIMER_PERIOD, app_psrc_tmr_cbk);
                }

                break;
            }

            /* Start monitor timer again */
            Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_MONITOR_TIMER),
                    CY_APP_PSOURCE_EN_MONITOR_TIMER_PERIOD, app_psrc_tmr_cbk);
            break;

        case CY_APP_PSOURCE_EN_HYS_TIMER:
#if CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME
            if (Cy_PdUtils_SwTimer_IsRunning(context->ptrTimerContext, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_MONITOR_TIMER)))
            {
                return;
            }
#endif /* CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME */           
            
            if((app_stat->psrc_rising == false) &&
                (Cy_App_VbusIsPresent(context, app_stat->psrc_volt, CY_APP_VBUS_NEW_VALID_MARGIN) == true))
            {
                Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_HYS_TIMER),
                        CY_APP_PSOURCE_EN_HYS_TIMER_PERIOD, app_psrc_tmr_cbk);
                return;
            }
            Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_TIMER));
            app_stat->psrc_volt_old = app_stat->psrc_volt;
            Cy_App_VbusDischargeOff(context);

            if(app_stat->psrc_rising == false)
            {
#if VBUS_OVP_ENABLE
                /* VBUS voltage has stabilized at the new lower level. Update the OVP and RCP limits. */
                Cy_App_Fault_OvpEnable(context, app_stat->psrc_volt,
                                       CCG_SRC_FET, app_psrc_vbus_ovp_cbk);
#endif /* VBUS_OVP_ENABLE */

#if VBUS_RCP_ENABLE
                Cy_App_Fault_RcpEnable(context, app_stat->psrc_volt, 
                                       app_psrc_vbus_rcp_cbk);
#endif /* VBUS_RCP_ENABLE */
            }
            else
            {
#if VBUS_UVP_ENABLE
                Cy_App_Fault_UvpEnable(context, 
                                       app_stat->psrc_volt, 
                                       CCG_SRC_FET, app_psrc_vbus_ovp_cbk);
#endif /* VBUS_UVP_ENABLE */
            }

            call_psrc_ready_cbk(context);
            break;

        case CY_APP_PSOURCE_DIS_TIMER:
            /* Discharge operation timed out */
            Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_DIS_MONITOR_TIMER));
            psrc_shutdown(context, true);
            break;

        case CY_APP_PSOURCE_DIS_MONITOR_TIMER:
            if (Cy_App_VbusIsPresent(context, CY_PD_VSAFE_5V, CY_APP_VBUS_DISCHARGE_TO_5V_MARGIN) == false)
            {
                /* If the voltage drops below 5 V turn off the FET and continue discharge. */
                psrc_shutdown(context, false);
            }

            if (Cy_App_VbusIsPresent(context, CY_PD_VSAFE_0V, CY_APP_VBUS_TURN_ON_MARGIN) == false)
            {
                /* Start extra discharge to allow proper discharge below Vsafe0V */
                Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_DIS_EXT_DIS_TIMER),
                        CY_APP_PSOURCE_DIS_EXT_DIS_TIMER_PERIOD, app_psrc_tmr_cbk);
            }
            else
            {
                /* Start monitor timer again */
                Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_DIS_MONITOR_TIMER),
                        CY_APP_PSOURCE_DIS_MONITOR_TIMER_PERIOD, app_psrc_tmr_cbk);
            }
            break;

        case CY_APP_PSOURCE_DIS_EXT_DIS_TIMER:
            Cy_PdUtils_SwTimer_Stop(context->ptrTimerContext, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_DIS_TIMER));
            Cy_App_VbusDischargeOff(context);

            /* Notify the caller that Cy_App_Source_Disable is complete */
            call_psrc_ready_cbk(context);
            break;
        default:
            break;
    }
}

#if VBUS_OCP_ENABLE
void app_psrc_vbus_ocp_tmr_cbk(cy_timer_id_t id,  void *context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;

    /* 
     * Stop all psource transition timers and notify the stack about voltage
     * transition complete to process the SCP hard reset sequence. 
     * Also disable all other fault detection by calling the psource shutdown.
     */
    Cy_PdUtils_SwTimer_StopRange(ptrPdStackContext->ptrTimerContext,
            CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_PSOURCE_EN_TIMER),
            CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_PSOURCE_EN_HYS_TIMER));
    call_psrc_ready_cbk(ptrPdStackContext);

    /* OCP fault */
    psrc_shutdown(ptrPdStackContext, true);

    /* Set alert message */
    cy_pd_pd_do_t alert;
    alert.val = 0;
    alert.ado_alert.ocp = true;
    ptrPdStackContext->dpmStat.alert = alert;

    /* Enqueue OCP fault event */
    Cy_App_EventHandler(ptrPdStackContext, APP_EVT_VBUS_OCP_FAULT, NULL);

    (void)id;
}

#if VBUS_SOFT_START_ENABLE
void ocp_handler_wrapper(cy_timer_id_t id,  void *cbkContext)
{
    cy_stc_pdstack_context_t *context = (cy_stc_pdstack_context_t *)cbkContext;

    if (gl_fet_soft_start_en[context->port])
    {
        Cy_USBPD_Vbus_NgdoSetDriveStrength(context->ptrUsbPdContext, 0x0E);

        gl_fet_soft_start_en[context->port] = false;

        Cy_App_Source_SetCurrent(context, CUR_LEVEL_3A);
    }

    (void)id;
}
#endif /* VBUS_SOFT_START_ENABLE */

bool app_psrc_vbus_ocp_cbk(void *cbkContext, bool comp_out)
{
    cy_stc_usbpd_context_t * context = (cy_stc_usbpd_context_t *)cbkContext;
    cy_stc_pdstack_context_t * ptrPdStackContext = Cy_PdStack_Dpm_GetContext(context->port);
    bool retval = false;

    if (comp_out)
    {
#if VBUS_SOFT_START_ENABLE
        if(gl_fet_soft_start_en[context->port])
        {
             /* Disable the OCP fault detection module */
            Cy_USBPD_Fault_Vbus_OcpDisable(context, true);

            Cy_USBPD_Vbus_NgdoSetDriveStrength(context, 0x02); 
            
            /* Schedule a timer which will increase the drive strength after a delay. */
            Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext, 
                            CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_FET_SOFT_START_TIMER_ID), 5, ocp_handler_wrapper);
        }
        else
#endif /* VBUS_SOFT_START_ENABLE */
        {
            
            /* Start a OCP debounce timer */
            Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                    CY_PDSTACK_GET_PD_TIMER_ID(ptrPdStackContext, CY_PDSTACK_PD_OCP_DEBOUNCE_TIMER),
                    ptrPdStackContext->ptrUsbPdContext->usbpdConfig->vbusOcpConfig->debounce,
                    app_psrc_vbus_ocp_tmr_cbk);
        }
    }
    else
    {
        /* Check if the timer is still running, in an negative edge. */
        retval = Cy_PdUtils_SwTimer_IsRunning(ptrPdStackContext->ptrTimerContext, CY_PDSTACK_GET_PD_TIMER_ID(ptrPdStackContext, CY_PDSTACK_PD_OCP_DEBOUNCE_TIMER));
        if (retval)
        {
            Cy_PdUtils_SwTimer_Stop(ptrPdStackContext->ptrTimerContext, CY_PDSTACK_GET_PD_TIMER_ID(ptrPdStackContext, CY_PDSTACK_PD_OCP_DEBOUNCE_TIMER));
        }
    }

    return retval;
}
#endif /* VBUS_OCP_ENABLE */

#if VBUS_SCP_ENABLE
bool app_psrc_vbus_scp_cbk(void * cbkContext, bool comp_out)
{
    /* Get the PDStack context from the USBPD context */
    cy_stc_usbpd_context_t * context = (cy_stc_usbpd_context_t *) cbkContext;
    cy_stc_pdstack_context_t * pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);
    /* 
     * Stop all psource transition timers and notify stack about voltage
     * transition complete to process the SCP hard reset sequence. 
     * Also disable all other fault detection by calling psource shutdown.
     */ 
    Cy_PdUtils_SwTimer_StopRange(pdstack_ctx->ptrTimerContext,
            CY_APP_GET_TIMER_ID(pdstack_ctx, CY_APP_PSOURCE_EN_TIMER),
            CY_APP_GET_TIMER_ID(pdstack_ctx, CY_APP_PSOURCE_EN_HYS_TIMER));
    call_psrc_ready_cbk(pdstack_ctx);

    /* SCP fault */
    psrc_shutdown(pdstack_ctx, true);

    /* Set alert message */
    cy_pd_pd_do_t alert;
    alert.val = 0;
    alert.ado_alert.ocp = true;
    pdstack_ctx->dpmStat.alert = alert;

    /* Enqueue SCP fault event */
    Cy_App_EventHandler(pdstack_ctx, APP_EVT_VBUS_SCP_FAULT, NULL);

    (void)comp_out;

    return 0;
}
#endif /* VBUS_SCP_ENABLE */

#if VBUS_RCP_ENABLE
bool app_psrc_vbus_rcp_cbk(void * cbkContext, bool comp_out)
{
    /* Get the PDStack context from the USB PD context */
    cy_stc_usbpd_context_t * context = (cy_stc_usbpd_context_t *) cbkContext;
    cy_stc_pdstack_context_t * pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);

    /* RCP fault */
    psrc_shutdown(pdstack_ctx, true);

    cy_pd_pd_do_t alert;
    alert.val = 0;
    /* Treat RCP as equivalent to OVP and send an alert post fault recovery */
    alert.ado_alert.ovp = true;
    pdstack_ctx->dpmStat.alert = alert;

    /* Notify the solution layer about the fault. */
    Cy_App_EventHandler(pdstack_ctx, APP_EVT_VBUS_RCP_FAULT, NULL);

    (void)comp_out;

    return 0;
}
#endif /* VBUS_RCP_ENABLE */

#if ((VBUS_OVP_ENABLE) || (VBUS_UVP_ENABLE))

static void ovp_pwr_ready_cbk(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    /* Dummy callback to allow VBUS discharge */
    (void)ptrPdStackContext;
}

bool app_psrc_vbus_ovp_cbk(void *cbkContext, bool compOut)
{
    /* Get the PDStack context from the USB PD context */
    cy_stc_usbpd_context_t * context = (cy_stc_usbpd_context_t *) cbkContext;
    cy_stc_pdstack_context_t *ptrPdStackContext = Cy_PdStack_Dpm_GetContext(context->port);
    uint8_t port = ptrPdStackContext->port;
    cy_stc_app_status_t *app_stat = Cy_App_GetStatus(port);

    app_stat->psrc_volt = CY_PD_VSAFE_0V;
    psrc_select_voltage(ptrPdStackContext);

    /*OVP fault */
    psrc_shutdown(ptrPdStackContext, true);

    if (compOut == true)
    {
        /* OVP fault condition */

        /* Set alert message to be sent after fault recovery */
        cy_pd_pd_do_t alert;
        alert.val = 0;
        alert.ado_alert.ovp = true;
        ptrPdStackContext->dpmStat.alert = alert;

        Cy_App_EventHandler(ptrPdStackContext, APP_EVT_VBUS_OVP_FAULT, NULL);
        Cy_App_Source_Disable(ptrPdStackContext, ovp_pwr_ready_cbk);
    }
#if VBUS_UVP_ENABLE
    else
    {
        /* UVP fault condition */

        /* 
         * UVP is a hardware cutoff in microseconds and OCP is a software
         * debounce and cutoff in milliseconds. When there is a sudden change
         * in VBUS current, VBUS voltage dips and causes UVP to react first
         * rather than OCP. Sink expects an alert message for OCP, that will be
         * missed if UVP is received. Hence, mimic OCP alert in case of UVP as
         * well. This will be taken care only for non-PPS contracts.
         */
        if(ptrPdStackContext->dpmStat.srcSelPdo.src_gen.supplyType != CY_PDSTACK_PDO_AUGMENTED)
        {
            /* Set alert message */
            cy_pd_pd_do_t alert;
            alert.val = 0;
            alert.ado_alert.ocp = true;
            ptrPdStackContext->dpmStat.alert = alert;
        }

        Cy_App_EventHandler(ptrPdStackContext, APP_EVT_VBUS_UVP_FAULT, NULL);
    }
#endif /* VBUS_UVP_ENABLE */

    return 0;
}
#endif /* ((VBUS_OVP_ENABLE) || (VBUS_UVP_ENABLE)) */

__attribute__ ((weak)) void soln_set_volt_port1 (uint16_t vol_in_mv)
{
    (void)vol_in_mv;
}

#if PMG1_PD_DUALPORT_ENABLE
__attribute__ ((weak)) void soln_set_volt_port2 (uint16_t vol_in_mv)
{
    (void)vol_in_mv;
}
#endif /* PMG1_PD_DUALPORT_ENABLE */

void psrc_select_voltage(cy_stc_pdstack_context_t *context)
{
    uint8_t port = context->port;
    cy_stc_app_status_t *app_stat = Cy_App_GetStatus(port);
    uint16_t select_volt = app_stat->psrc_volt;

    /* Voltage should not drop below 5 V */
    if (app_stat->psrc_volt == CY_PD_VSAFE_0V)
    {
        app_stat->psrc_volt = CY_PD_VSAFE_5V;
    }

    /* 
     * Cap the selected voltage to the absolute maximum voltage that can be
     * applied to the cable. */ 
    if (select_volt > VBUS_MAX_VOLTAGE)
    {
        select_volt = VBUS_MAX_VOLTAGE;
    }

    if(port == TYPEC_PORT_0_IDX)
    {
        soln_set_volt_port1 (select_volt);
    }
#if PMG1_PD_DUALPORT_ENABLE
    else
    {
        soln_set_volt_port2 (select_volt);
    }
#endif /* PMG1_PD_DUALPORT_ENABLE */
}

void Cy_App_Source_SetVoltage(cy_stc_pdstack_context_t * context, uint16_t volt_mV)
{
    uint8_t port = context->port;
    cy_stc_app_status_t *app_stat = Cy_App_GetStatus(port);
    const cy_stc_pd_dpm_config_t *dpm_stat = &context->dpmConfig;
    app_stat->psrc_volt = volt_mV;

#if VBUS_OCP_ENABLE
    /* Leave OCP detection disabled while doing the voltage transition. */
    Cy_App_Fault_OcpDisable(context, false);
#endif /* VBUS_OCP_ENABLE */

    if ((app_stat->psrc_volt >= app_stat->psrc_volt_old) && (volt_mV != CY_PD_VSAFE_0V))
    {
#if VBUS_OVP_ENABLE
        /* If voltage is increased, ensure that OVP and RCP limits are moved up. */
        Cy_App_Fault_OvpEnable(context, volt_mV, CCG_SRC_FET, app_psrc_vbus_ovp_cbk);
#endif /* VBUS_OVP_ENABLE */

#if VBUS_RCP_ENABLE
        Cy_App_Fault_RcpEnable(context, volt_mV, app_psrc_vbus_rcp_cbk);
#endif /* VBUS_RCP_ENABLE */
    }
    else if ((app_stat->psrc_volt < app_stat->psrc_volt_old) && (volt_mV != CY_PD_VSAFE_0V))
    {
        /*
         * Enable UVP only if port partner is attached. Ensure that
         * UVP does not get enabled if VBUS is not applied, like in case of
         * HARD_RESET.
         */
        if ((dpm_stat->attach == true) && (app_stat->is_vbus_on == true))
        {
#if VBUS_UVP_ENABLE
            Cy_App_Fault_UvpEnable(context, volt_mV, CCG_SRC_FET, app_psrc_vbus_ovp_cbk);
#endif /* VBUS_UVP_ENABLE */
        }
    }

    psrc_select_voltage(context);
}

uint32_t Cy_App_Source_GetVoltage (cy_stc_pdstack_context_t *context)
{
    return Cy_App_GetStatus(context->port)->psrc_volt;
}

void Cy_App_Source_SetCurrent (cy_stc_pdstack_context_t *context, uint16_t cur_10mA)
{
#if (VBUS_OCP_ENABLE || VBUS_SCP_ENABLE)

#if VBUS_OCP_ENABLE
    /* Update the OCP/SCP thresholds when required. */
    const cy_stc_pdstack_dpm_status_t *dpm_stat = &context->dpmStat;
    cy_stc_pd_dpm_config_t *dpm_config = &context->dpmConfig;
    uint32_t ocp_cur;

    if (dpm_config->contractExist)
    {
        switch(dpm_stat->srcSelPdo.src_gen.supplyType)
        {
            case CY_PDSTACK_PDO_FIXED_SUPPLY:
            case CY_PDSTACK_PDO_VARIABLE_SUPPLY:
                ocp_cur = dpm_stat->srcSelPdo.src_gen.maxCurPower;
                break;
            case CY_PDSTACK_PDO_AUGMENTED:
                if(dpm_stat->srcSelPdo.pps_src.apdoType == CY_PDSTACK_APDO_AVS)
                {
                    /* PDP value is in 1W units and the max volt is in 100 mV units. Convert pdp in 100 mW units
                     * and divide by voltage gives the current in amps then multiplied by 100 to convert in 10 mA units. */  
                    ocp_cur = ((dpm_stat->srcSelPdo.epr_avs_src.pdp * 10) / dpm_stat->srcSelPdo.epr_avs_src.maxVolt) * 100;
                }
                else if(dpm_stat->srcSelPdo.spr_avs_src.apdoType == CY_PDSTACK_APDO_SPR_AVS)
                {
                    /* Set the current limit based on the contract voltage. */
                    if(dpm_stat->srcRdo.rdo_spr_avs.outVolt <= (CY_PD_VSAFE_15V / 25U))
                    {
                        ocp_cur = dpm_stat->srcSelPdo.spr_avs_src.maxCur1;
                    }
                    else
                    {
                        ocp_cur = dpm_stat->srcSelPdo.spr_avs_src.maxCur2;
                    }
                }
                else
                {
                    /* Max current in PPS PDO is in 50 mA units, multiplied by 5 to convert in 10 mA units. */
                    ocp_cur = dpm_stat->srcSelPdo.pps_src.maxCur * 5;
                }
                break;
            default:
                ocp_cur = dpm_stat->srcSelPdo.src_gen.maxCurPower;
                break;
        }
    }
    else
    {
        ocp_cur = cc_rp_to_cur_map[dpm_stat->srcCurLevel];
    }

    Cy_App_Fault_OcpEnable(context, ocp_cur, app_psrc_vbus_ocp_cbk);
#endif /* VBUS_OCP_ENABLE */

#if VBUS_SCP_ENABLE
    Cy_App_Fault_ScpEnable(context, 1000, app_psrc_vbus_scp_cbk);
#endif /* VBUS_SCP_ENABLE */

    (void) cur_10mA;
#else
    (void) context;
    (void) cur_10mA;
#endif /* (VBUS_OCP_ENABLE || VBUS_SCP_ENABLE) */
}

void Cy_App_Source_Enable (cy_stc_pdstack_context_t * context,
        cy_pdstack_pwr_ready_cbk_t pwr_ready_handler)
{
#if CY_PD_EPR_AVS_ENABLE
    cy_stc_pdstack_dpm_ext_status_t *dpmExtStat = &(context->dpmExtStat);
#endif /* CY_PD_EPR_AVS_ENABLE */

#if CY_PD_EPR_ENABLE
    uint32_t powerEnTimerPeriod = 0u;
#endif

    cy_stc_app_status_t* app_stat = Cy_App_GetStatus(context->port);
    const cy_stc_pdstack_dpm_status_t *dpm_stat = &(context->dpmStat);
    uint32_t intr_state;

    intr_state = Cy_SysLib_EnterCriticalSection();

    Cy_PdUtils_SwTimer_StopRange(context->ptrTimerContext, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_TIMER),
            CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_DIS_EXT_DIS_TIMER));

    /* Turn on FETs only if dpm is enabled and there is no active fault condition. */
    if ((context->dpmConfig.dpmEnabled) && (dpm_stat->faultActive == false))
    {
#if (VBUS_OCP_ENABLE || VBUS_SCP_ENABLE)
        /* Ensure that the OCP/SCP is enabled where required. The current parameter is not used. */
        Cy_App_Source_SetCurrent (context, CUR_LEVEL_3A);
#endif /* (VBUS_OCP_ENABLE || VBUS_SCP_ENABLE) */

#if CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME  
        if(REGULATOR_STATUS(port) == false)
        {
            /* Enable the regulator before turning on the FET */
            REGULATOR_ENABLE(port);
            Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_MONITOR_TIMER),
                    REGULATOR_TURN_ON_DELAY, app_psrc_tmr_cbk);
        }
#endif /* CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME */    

        /* Turn off VBUS Discharge by default */
        Cy_App_VbusDischargeOff(context);

        /* Turn on PSource FET */
        vbus_fet_on(context);

        if (pwr_ready_handler != NULL)
        {
            app_stat->psrc_rising = true;

            /* If the VBUS voltage is dropping, turn the discharge path on. */
            if(app_stat->psrc_volt_old > app_stat->psrc_volt)
            {
                app_stat->psrc_rising = false;
                Cy_App_VbusDischargeOn(context);
            }
            app_stat->pwr_ready_cbk = pwr_ready_handler;

            /* Start power source enable and monitor timers */
#if CY_PD_EPR_ENABLE

            bool isActive = false;
            (void)Cy_PdStack_Dpm_IsEprModeActive(context, &isActive);

            if (isActive == false)
            {
                powerEnTimerPeriod = CY_APP_PSOURCE_EN_TIMER_PERIOD;
            }
            else
            {
#if CY_PD_EPR_AVS_ENABLE
                (void)Cy_PdStack_Dpm_IsEprAvsModeActive(context, &isActive);

                if (isActive == true)
                {
                    /* After sending the accept message, the adjustable voltage Supply starts to
                       decrease its output voltage. The adjustable voltage supply new voltage set-point
                       (corresponding to vAvsNew) shall be reached by tAvsSrcTransLarge for steps larger than
                       vAvsSmallStep or else by tAvsSrcTransSmall. The power supply informs
                       the device policy manager that it is has reached the new level.
                       The power supply status is passed to the policy engine.
                       */
                    if(dpmExtStat->eprAvsMode == CY_PDSTACK_EPR_AVS_SMALL)
                    {
                        powerEnTimerPeriod = CY_PD_PSOURCE_AVS_TRANS_SMALL_PERIOD;
                    }
                    else if(dpmExtStat->eprAvsMode == CY_PDSTACK_EPR_AVS_LARGE)
                    {
                        powerEnTimerPeriod = CY_PD_PSOURCE_AVS_TRANS_LARGE_PERIOD;
                    }
                    else
                    {
                        powerEnTimerPeriod = CY_APP_PSOURCE_EN_TIMER_PERIOD;
                    }
                }
                else
#endif /* CY_PD_EPR_AVS_ENABLE */
                {
                    powerEnTimerPeriod = CY_APP_PSOURCE_EPR_EN_TIMER_PERIOD;
                }
            }
            Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_TIMER),
                    powerEnTimerPeriod, app_psrc_tmr_cbk);
#else
            Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_TIMER),
                                CY_APP_PSOURCE_EN_TIMER_PERIOD, app_psrc_tmr_cbk);
#endif /* CY_PD_EPR_ENABLE */

            Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_MONITOR_TIMER),
                    CY_APP_PSOURCE_EN_MONITOR_TIMER_PERIOD, app_psrc_tmr_cbk);
        }
    }

    Cy_SysLib_ExitCriticalSection(intr_state);
}

void Cy_App_Source_Disable(cy_stc_pdstack_context_t * context, cy_pdstack_pwr_ready_cbk_t pwr_ready_handler)
{
    uint8_t port = context->port;
    cy_stc_app_status_t* app_stat = Cy_App_GetStatus(port);
    uint8_t intr_state;

    intr_state = Cy_SysLib_EnterCriticalSection();

    Cy_PdUtils_SwTimer_StopRange(context->ptrTimerContext, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_EN_TIMER),
            CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_DIS_EXT_DIS_TIMER));

#if VBUS_UVP_ENABLE
    Cy_App_Fault_UvpDisable(context, CCG_SRC_FET);
#endif /* VBUS_UVP_ENABLE */

    if((app_stat->psrc_volt_old <= CY_PD_VSAFE_5V))
    {
        psrc_shutdown(context, false);
        Cy_SysLib_DelayUs(20);
    }
    else
    {
        Cy_App_Source_SetVoltage(context, CY_PD_VSAFE_5V);
    }

    app_stat->psrc_volt_old = CY_PD_VSAFE_0V;

    if ((pwr_ready_handler != NULL) && (context->dpmConfig.dpmEnabled))
    {
        /* Turn on discharge to get the voltage to drop faster */
        Cy_App_VbusDischargeOn(context);
        app_stat->pwr_ready_cbk = pwr_ready_handler;
#if(CY_PD_EPR_ENABLE)
        bool isActive = true;
        Cy_PdStack_Dpm_IsEprModeActive(context, &isActive);
        Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_DIS_TIMER),
                isActive ?  CY_APP_PSOURCE_EPR_DIS_TIMER_PERIOD : CY_APP_PSOURCE_DIS_TIMER_PERIOD,
                app_psrc_tmr_cbk);
#else
        /* Start Power source enable and monitor timer */
        Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_DIS_TIMER),
                CY_APP_PSOURCE_DIS_TIMER_PERIOD, app_psrc_tmr_cbk);
#endif /* CY_PD_EPR_ENABLE */
        Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context, CY_APP_GET_TIMER_ID(context, CY_APP_PSOURCE_DIS_MONITOR_TIMER),
                CY_APP_PSOURCE_DIS_MONITOR_TIMER_PERIOD, app_psrc_tmr_cbk);
    }
    else
    {
        psrc_shutdown(context, true);
    }

    Cy_SysLib_ExitCriticalSection(intr_state);
}


static void psrc_shutdown(cy_stc_pdstack_context_t * context, bool discharge_dis)
{
    uint8_t port = context->port;

    if(Cy_App_GetStatus(context->port)->is_vbus_on != false)
    {
        /* Turn off source FET */
        vbus_fet_off(context);
    }

    if(discharge_dis == true)
    {
        Cy_App_VbusDischargeOff(context);
    }

    /* Disable OVP/OCP/UVP */
    Cy_App_Fault_OvpDisable (context, CCG_SRC_FET);
    Cy_App_Fault_UvpDisable (context, CCG_SRC_FET);
    Cy_App_Fault_OcpDisable(context, false);
    Cy_App_Fault_ScpDisable(context);
    Cy_App_Fault_RcpDisable(context);

    (void) port;
}

/* [] END OF FILE */
