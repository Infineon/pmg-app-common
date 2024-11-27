/***************************************************************************//**
* \file cy_app_battery_charging.c
* \version 2.0
*
* \brief
* Implements the battery charging functions.
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
#include "cy_app_battery_charging.h"
#include "cy_pdstack_common.h"
#include "cy_pdstack_dpm.h"
#include "cy_pdutils.h"
#include "cy_usbpd_bch.h"
#include "cy_app_sink.h"
#include "cy_app_source.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_app_timer_id.h"
#include "cy_app.h"

#if CY_HPI_ENABLED
#include "cy_hpi.h"
#endif /* CY_HPI_ENABLED. */

#if (BATTERY_CHARGING_ENABLE)

cy_stc_bc_status_t gl_bc_status[NO_OF_BC_PORTS];

#if (!APPLE_SOURCE_DISABLE)
const uint16_t gl_apple_id_to_cur_map [] = {
    APPLE_AMP_1A,
    APPLE_AMP_2_1A,
    APPLE_AMP_2_4A,
    APPLE_AMP_3A
};
#endif /* (!APPLE_SOURCE_DISABLE) */

extern cy_app_sln_cbk_t *glPtrSlnCbk;

/* Callbacks from BC PHY. */
static void bc_phy_cbk_handler(void *context, uint32_t event);

/* BC FSM prototypes. */
static void bc_fsm_off(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);

#if (!CY_PD_SINK_ONLY)
/* Handlers for SRC mode operation. */
static void bc_fsm_src_look_for_connect(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_src_initial_connect(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_src_apple_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);

#if (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED))
static void bc_fsm_src_others_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_src_qc_or_afc(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_src_qc_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_src_afc_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
#endif /* (!QC_AFC_CHARGING_DISABLED) */
#endif /* (!CY_PD_SINK_ONLY) */

/* Handlers for Sink mode operation. */
#if (!(CY_PD_SOURCE_ONLY)) && (!BC_SOURCE_ONLY)
static void bc_fsm_sink_start(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_sink_apple_charger_detect(cy_stc_usbpd_context_t * context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_sink_apple_brick_id_detect(cy_stc_usbpd_context_t * context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_sink_primary_charger_detect(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_sink_type_c_only_source_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_sink_secondary_charger_detect(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_sink_dcp_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_sink_sdp_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_sink_cdp_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt);
#if QC_AFC_SNK_EN
static void bc_fsm_sink_afc_charger_detect(cy_stc_usbpd_context_t * context, cy_en_bc_fsm_evt_t evt);
static void bc_fsm_sink_qc_charger_detected(cy_stc_usbpd_context_t * context, cy_en_bc_fsm_evt_t evt);
#endif /* QC_AFC_SNK_EN */
#endif /* (!(CY_PD_SOURCE_ONLY)) && (!BC_SOURCE_ONLY) */

void (*const bc_fsm_table[BC_FSM_MAX_STATES]) (cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt) =
{
    bc_fsm_off,                                 /* 0: BC_FSM_OFF. */

#if !(CY_PD_SINK_ONLY)
    bc_fsm_src_look_for_connect,                /* 1: BC_FSM_SRC_LOOK_FOR_CONNECT. */
    bc_fsm_src_initial_connect,                 /* 2: BC_FSM_SRC_INITIAL_CONNECT. */
    bc_fsm_src_apple_connected,                 /* 3: BC_FSM_SRC_APPLE_CONNECTED. */
#else
    bc_fsm_off,                                 /* 1: BC_FSM_SRC_LOOK_FOR_CONNECT */
    bc_fsm_off,                                 /* 2: BC_FSM_SRC_INITIAL_CONNECT */
    bc_fsm_off,                                 /* 3: BC_FSM_SRC_APPLE_CONNECTED */
#endif /* (!(CY_PD_SINK_ONLY)) */

    bc_fsm_off,                                 /* 4: BC_FSM_SRC_CDP_CONNECTED */
#if !(CY_PD_SINK_ONLY)
#if (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED))
    bc_fsm_src_others_connected,                /*  5: BC_FSM_SRC_OTHERS_CONNECTED. */
    bc_fsm_src_qc_or_afc,                       /*  6: BC_FSM_SRC_QC_OR_AFC. */
    bc_fsm_src_qc_connected,                    /*  7: BC_FSM_SRC_QC_CONNECTED. */
    bc_fsm_src_afc_connected,                   /*  8: BC_FSM_SRC_AFC_CONNECTED. */
#else
    bc_fsm_off,                                 /*  5: BC_FSM_SRC_OTHERS_CONNECTED */
    bc_fsm_off,                                 /*  6: BC_FSM_SRC_QC_OR_AFC */
    bc_fsm_off,                                 /*  7: BC_FSM_SRC_QC_CONNECTED */
    bc_fsm_off,                                 /*  8: BC_FSM_SRC_AFC_CONNECTED */
#endif /* (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED)) */
#else
    bc_fsm_off,                                 /*  5: BC_FSM_SRC_OTHERS_CONNECTED */
    bc_fsm_off,                                 /*  6: BC_FSM_SRC_QC_OR_AFC */
    bc_fsm_off,                                 /*  7: BC_FSM_SRC_QC_CONNECTED */
    bc_fsm_off,                                 /*  8: BC_FSM_SRC_AFC_CONNECTED */
#endif /* (!(CY_PD_SINK_ONLY)) */

#if (!(CY_PD_SOURCE_ONLY)) && (!BC_SOURCE_ONLY)
    bc_fsm_sink_start,                          /* 9: BC_FSM_SINK_START. */
    bc_fsm_sink_apple_charger_detect,           /* 10: BC_FSM_SINK_APPLE_CHARGER_DETECT. */
    bc_fsm_sink_apple_brick_id_detect,          /* 11: BC_FSM_SINK_APPLE_BRICK_ID_DETECT. */
    bc_fsm_sink_primary_charger_detect,         /* 12: BC_FSM_SINK_PRIMARY_CHARGER_DETECT. */
    bc_fsm_sink_type_c_only_source_connected,   /* 13: BC_FSM_SINK_TYPE_C_ONLY_SOURCE_CONNECTED. */
    bc_fsm_sink_secondary_charger_detect,       /* 14: BC_FSM_SINK_SECONDARY_CHARGER_DETECT. */
    bc_fsm_sink_dcp_connected,                  /* 15: BC_FSM_SINK_DCP_CONNECTED. */
    bc_fsm_sink_sdp_connected,                  /* 16: BC_FSM_SINK_SDP_CONNECTED. */
    bc_fsm_sink_cdp_connected,                  /* 17: BC_FSM_SINK_CDP_CONNECTED. */
#if QC_AFC_SNK_EN
    bc_fsm_sink_afc_charger_detect,             /* 18: BC_FSM_SINK_AFC_CHARGER_DETECT. */
    bc_fsm_sink_qc_charger_detected             /* 19: BC_FSM_SINK_QC_CHARGER_DETECT. */
#else
    bc_fsm_off,                                 /* 18: BC_FSM_SINK_AFC_CHARGER_DETECT. */
    bc_fsm_off                                  /* 19: BC_FSM_SINK_QC_CHARGER_DETECT. */
#endif /* QC_AFC_SNK_EN */
#endif /* (!(CY_PD_SOURCE_ONLY)) && (!BC_SOURCE_ONLY) */
};

#if CY_HPI_ENABLED
static const uint8_t gl_bc_hpi_map[BC_CHARGE_CDP+1] =
{
    0x00,   /* BC_CHARGE_NONE. */
    0x04,    /* BC_CHARGE_DCP. */
    0x05,    /* BC_CHARGE_QC2. */
    0x06,   /* BC_CHARGE_QC3. */
    0x07,   /* BC_CHARGE_AFC. */
    0x08,   /* BC_CHARGE_APPLE. */
    0x03    /* BC_CHARGE_CDP. */
};
#endif /* CY_HPI_ENABLED */

static void bc_set_current_mode(cy_stc_usbpd_context_t *context, cy_en_bc_charge_mode_t mode)
{
    uint8_t port = context->port;
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[port];

    bc_stat->cur_mode = mode;

#if CY_HPI_ENABLED
#if CCG_TYPE_A_PORT_ENABLE
    if(port != TYPE_A_PORT_ID)
#endif /* CCG_TYPE_A_PORT_ENABLE */
    {
        Cy_Hpi_SetPortBcStatus((cy_stc_hpi_context_t *)Cy_PdStack_Dpm_GetContext(port)->ptrHpiContext, port, gl_bc_hpi_map[mode]);
    }
#endif /* CY_HPI_ENABLED */
}

#if (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD))
#if QC_AFC_SNK_EN
static bool bc_sink_mismatch_check(cy_stc_usbpd_context_t * context)
{
    uint8_t port = context->port;
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[port];

    /* Clears mismatch status from previously tried BC protocol. */ 
    bc_stat->mismatch = false;
    /* Read Sink voltage */
    uint16_t cur_volt = Cy_App_GetStatus(port)->psnk_volt;

    /* Mismatch check only for voltage range. Current can be any supported value. */
    if((cur_volt < bc_stat->min_volt) || (cur_volt > bc_stat->max_volt))
    {
        bc_stat->mismatch = true;
    } 

    return bc_stat->mismatch;
}

bool bc_psnk_enable(cy_stc_usbpd_context_t * context)
{
    uint8_t port = context->port;
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[port];
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(port);

    if(bc_sink_mismatch_check(context) == false)
    {
        bc_stat->connected = true;
        Cy_App_Sink_Enable(pdstack_ctx);
        Cy_App_EventHandler(pdstack_ctx, APP_EVT_BC_DETECTION_COMPLETED, NULL);
        return true;
    }
    else
    {
        Cy_App_Bc_Stop(context);
        return false;
    }
}
#endif /* QC_AFC_SNK_EN */

#if (!(CY_PD_SOURCE_ONLY)) && (!BC_SOURCE_ONLY)
#if (!APPLE_SINK_DISABLE)
static void bc_eval_apple_brick_id(cy_stc_usbpd_context_t * context, cy_en_bc_apple_brick_id_t brick_id)
{
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);
    /* Handles detected Apple Brick IDs. */
    switch (brick_id)
    {
        case APPLE_BRICK_ID_3:
            Cy_App_Sink_SetCurrent(pdstack_ctx, APPLE_AMP_1A);
            break;

        case APPLE_BRICK_ID_1:
            Cy_App_Sink_SetCurrent(pdstack_ctx, APPLE_AMP_2_1A);
            break;

        case APPLE_BRICK_ID_4:
            Cy_App_Sink_SetCurrent(pdstack_ctx, APPLE_AMP_2_4A);
            break;

        default:
            /* Rest of the Brick IDs reserved for future use. */
            break;
    }

    /* Ensures VBUS is set to 5 V. */
    Cy_App_Sink_SetVoltage(pdstack_ctx, CY_PD_VSAFE_5V);
#if QC_AFC_SNK_EN
    bc_psnk_enable(context);
#else
    Cy_App_Sink_Enable(pdstack_ctx);
#endif
}
#endif /* (!APPLE_SINK_DISABLE) */
#endif /* (!(CY_PD_SOURCE_ONLY)) */

static void bc_timer_cb(cy_timer_id_t id, void *cbContext)
{
    cy_stc_usbpd_context_t *context = (cy_stc_usbpd_context_t *)cbContext;

    if (context->port != 0u)
    {
        id = (id & 0x00FFU) + CY_PDUTILS_TIMER_APP_PORT0_START_ID;
    }

    if (id == CY_APP_BC_GENERIC_TIMER1)
    {
        Cy_App_Bc_FsmSetEvt(context, BC_EVT_TIMEOUT1);
    }

    if (id == CY_APP_BC_GENERIC_TIMER2)
    {
        Cy_App_Bc_FsmSetEvt(context, BC_EVT_TIMEOUT2);
    }
}

#if (!CY_PD_SINK_ONLY)
static void bc_pwr_ready_cbk(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    /* Do nothing. */
    CY_UNUSED_PARAMETER(ptrPdStackContext);
}

static void bc_psrc_enable(cy_stc_usbpd_context_t *context, uint16_t volt_mV)
{
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);
#if CCG_TYPE_A_PORT_ENABLE
    if(context->port == TYPE_A_PORT_ID)
    {
        glPtrSlnCbk->type_a_set_voltage(volt_mV);
        if(Cy_App_GetStatus(context->port)->is_vbus_on == false)
        {
            Cy_App_GetStatus(context->port)->is_vbus_on = true;
            glPtrSlnCbk->type_a_enable_disable_vbus(true);
        }
    }
    else
#endif /* CCG_TYPE_A_PORT_ENABLE */
    {
        Cy_App_Source_SetVoltage(pdstack_ctx, volt_mV);
        Cy_App_Source_Enable(pdstack_ctx, bc_pwr_ready_cbk);
    }
}
#endif /* (!CY_PD_SINK_ONLY) */

#if (LEGACY_APPLE_SRC_SLN_TERM_ENABLE)
void sln_apply_apple_src_term(cy_stc_usbpd_context_t *context, cy_en_usbpd_bch_src_term_t charger_term)
{
#if APPLE_SRC_EXT_TERM_ENABLE
    (void)Cy_USBPD_Bch_Phy_RemoveTerm(context);
    (void)Cy_USBPD_Bch_Phy_ConfigSrcTerm(context, CHGB_SRC_TERM_DCP);
#if (defined(CY_DEVICE_CCG6) || defined(CY_DEVICE_CCG3))
    Cy_USBPD_Bch_Apply_AppleTermDm(context, CHGB_SRC_TERM_APPLE_2_4A);
#endif /* (defined(CY_DEVICE_CCG6) || defined(CY_DEVICE_CCG3)) */
    (void)Cy_USBPD_Bch_ApplyDpPd(context);
    Cy_GPIO_Write(DP_APPLE_SRC_TERM_PORT, DP_APPLE_SRC_TERM_PIN, 1u);
    Cy_GPIO_SetDrivemode(DP_APPLE_SRC_TERM_PORT, DP_APPLE_SRC_TERM_PIN, CY_GPIO_DM_STRONG_IN_OFF);
#if defined(CY_DEVICE_CCG6)
    if(context->dpmGetConfig()->polarity != 0u)
    {
        Cy_USBPD_Mux_ConfigDpDm(context, CY_USBPD_DPDM_MUX_CONN_USB_BOT);
    }
    else
    {
        Cy_USBPD_Mux_ConfigDpDm(context, CY_USBPD_DPDM_MUX_CONN_USB_TOP);
    }
#endif /* defined(CY_DEVICE_CCG6) */
#else
    /* Uses the internal terminations. */
    (void)Cy_USBPD_Bch_Phy_ConfigSrcTerm(context, charger_term);
#endif /* APPLE_SRC_EXT_TERM_ENABLE */
}

void sln_remove_apple_src_term(cy_stc_usbpd_context_t *context)
{
    (void)Cy_USBPD_Bch_Phy_RemoveTerm(context);
#if APPLE_SRC_EXT_TERM_ENABLE
    Cy_GPIO_SetDrivemode(DP_APPLE_SRC_TERM_PORT, DP_APPLE_SRC_TERM_PIN, CY_GPIO_DM_ANALOG);
    Cy_GPIO_Write(DP_APPLE_SRC_TERM_PORT, DP_APPLE_SRC_TERM_PIN, 0u);
#if defined(CY_DEVICE_CCG6)
    Cy_USBPD_Mux_ConfigDpDm(context, CY_USBPD_DPDM_MUX_CONN_NONE);
#endif /* defined(CY_DEVICE_CCG6) */
#endif /* APPLE_SRC_EXT_TERM_ENABLE */
}
#endif /* (LEGACY_APPLE_SRC_SLN_TERM_ENABLE) */

static void bc_fsm_off(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    /* Nothing to do in this state. */
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(evt);
}

#if (!CY_PD_SINK_ONLY)
static void bc_fsm_src_look_for_connect(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);
#if (!APPLE_SOURCE_DISABLE)
    cy_stc_legacy_charging_cfg_t * chg_config = (cy_stc_legacy_charging_cfg_t *) context->usbpdConfig->legacyChargingConfig;
#endif /* (!APPLE_SOURCE_DISABLE)*/

#if  (ENABLE_APPLE_BC12_SUPPORT || (!APPLE_SOURCE_DISABLE))
    uint8_t chgb_dm_comp_ref = (uint8_t)CHGB_VREF_2_2V;
    (void)chgb_dm_comp_ref;
#if  (ENABLE_APPLE_BC12_SUPPORT)
    bool comp0_flag, comp1_flag;
#endif /* (ENABLE_APPLE_BC12_SUPPORT) */
#endif /* (ENABLE_APPLE_BC12_SUPPORT || (!APPLE_SOURCE_DISABLE)) */

    switch(evt)
    {
        case BC_FSM_EVT_ENTRY:
            /*
             * The detection logic varies based on the protocols selected.
             * If only Apple charging is selected, then different Apple source IDs
             * are supported. If Apple charging needs to be supported along with
             * BC 1.2, then only 2.4 A Apple charger ID is supported.
             *
             * If Apple charging is selected, keep presenting the Apple terminations.
             * In case of Apple only charging mode, keep presenting the terminations.
             * There is no further action after this.
             *
             * If Apple charging along with BC 1.2 based detection is enabled,
             * then first start with Apple 2.4 A termination. Additionally, enable D+ comparator
             * to look for < 2.2 V. If this is detected, then switch to BC termination
             * and proceed with BC 1.2-based connection detection logic.
             *
             * If Apple charging is not selected, then proceed directly to BC 1.2
             * terminations and subsequent detection logic.
             *
             * Detach detection for Apple and BC 1.2 DCP sink cannot be done as
             * sink terminations are not present after detection. Only re-attach
             * can be detected. So, in case of BC 1.2 DCP operation, this state
             * shall be re-entered to reapply Apple terminations as required.
             *
             * In case of QC and AFC mode of operation, detach can be detected.
             * When this happens, this state shall be re-entered. Detach handling
             * needs to be done for this.
             *
             * NOTE: There are two cases which are not currently dealt with:
             * 1. In case of Type-C port, when legacy state machine is entered for
             *    the first time, the VBUS may be already present and the
             *    sink may already be attached and completed its detection logic.
             *    May require power cycle VBUS to restart sink's detection
             *    logic. This is not currently done as PD and Legacy
             *    together (LEGACY_PD_PARALLEL_OPER) starts.
             *
             * 2. In case of Type-A port attached to a BC 1.2 sink or an Apple
             *    sink, there is no real detach detection. When Apple charging
             *    is enabled, there is also no re-attach detection.
             *
             *    The Type-A port current consumption is monitored to switch to
             *    low-power regulator when current drops under 300 mA. This same
             *    logic moves back to high-power regulator, but there is a polling
             *    delay as well as regulator turn on delay involved which can
             *    cause the 5 V regulator (which also feeds the CCG3PA device) to
             *    shut-off due to overcurrent.
             *
             *    There is a work around; when switched to low-power regulator,
             *    also switched to BC 1.2 DCP attach wait state and stay there until
             *    there is an attach. In event of attach, switch to Apple mode of
             *    detection and proceed as usual.
             *
             *    Since the current systems are able to withstand the switch
             *    without disconnecting, no implementation is done currently.
             */
            (void)Cy_USBPD_Bch_Phy_DisableComp(context, 0U);
            (void)Cy_USBPD_Bch_Phy_DisableComp(context, 1U);
            Cy_PdUtils_SwTimer_StopRange(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1),
                                         CY_APP_GET_TIMER_ID(context, CY_APP_BC_DP_DM_DEBOUNCE_TIMER));
            Cy_App_Bc_FsmClearEvt(context, BC_EVT_ALL_MASK);
            (void)Cy_USBPD_Bch_Phy_En(context);

#if (!APPLE_SOURCE_DISABLE)
            if ((chg_config->srcSel & BC_SRC_APPLE_MODE_ENABLE_MASK) != 0x00u)
            {
#if (LEGACY_APPLE_SRC_SLN_TERM_ENABLE)
                /*
                 * If Apple charging is enabled, then present Apple terminations.
                 * Since parallel operation requires external control, invoke
                 * the solution function instead of the HAL function. It is expected
                 * that the solution handler uses external termination for DP
                 * when parallel operation is required. The solution handler
                 * can choose to use internal termination when parallel operation
                 * is not required. This is useful when only one port requires
                 * parallel operation.
                 */
                sln_apply_apple_src_term(context, (cy_en_usbpd_bch_src_term_t)chg_config->appleSrcId);
                bc_stat->cur_amp = gl_apple_id_to_cur_map[chg_config->appleSrcId];
#else
                (void)Cy_USBPD_Bch_Phy_RemoveTerm(context);
                (void)Cy_USBPD_Bch_Phy_ConfigSrcTerm(context, CHGB_SRC_TERM_DCP);
                Cy_USBPD_Bch_Apply_AppleTermDp(context, CHGB_SRC_TERM_APPLE_2_4A);

                /* Sets the active supply current. */
                bc_stat->cur_amp = gl_apple_id_to_cur_map[chg_config->appleSrcId];

#endif /* LEGACY_APPLE_SRC_SLN_TERM_ENABLE */

#if ENABLE_APPLE_BC12_SUPPORT
                /*
                 * If parallel operation is expected, then setup CMP2 to
                 * detect D+ going under 2.2 V and setup CMP1 to detect D- going under 2.2 V as well.
                 * If it goes under this level, then it means that a BC 1.2 based sink is attached.
                 */
                if ((chg_config->srcSel & BC_SRC_1_2_MODE_ENABLE_MASK) != 0u)
                {
                    (void)Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_1_IDX, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                            CHGB_VREF_2_2V, CHGB_COMP_EDGE_FALLING);
#if (!defined(CY_DEVICE_CCG3))
                    (void)Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
                            (cy_en_usbpd_bch_vref_t)chgb_dm_comp_ref, CHGB_COMP_EDGE_FALLING);
#endif /* (!defined(CY_DEVICE_CCG3)) */
                }
#endif /* ENABLE_APPLE_BC12_SUPPORT */

                /* Indicates connectivity. */
                bc_stat->attach = true;
                bc_stat->connected = true;
                bc_set_current_mode(context, BC_CHARGE_APPLE);

#if CCG_TYPE_A_PORT_ENABLE
                if (context->port == TYPE_A_PORT_ID)
                {
                    glPtrSlnCbk->type_a_update_status(true, true);
                }
#endif /* CCG_TYPE_A_PORT_ENABLE */
            }
            else
#endif /* (!APPLE_SOURCE_DISABLE) */

            {
                /*
                 * If in DCP mode, do not change the setting, else this is
                 * a detach. Indicate the same.
                 */ 
                if (bc_stat->cur_mode != BC_CHARGE_DCP)
                {
                    bc_set_current_mode(context, BC_CHARGE_NONE);
                    bc_stat->attach = false;
#if CCG_TYPE_A_PORT_ENABLE
                    if (context->port == TYPE_A_PORT_ID)
                    {
                        glPtrSlnCbk->type_a_update_status(false, false);
                    }
#endif /* CCG_TYPE_A_PORT_ENABLE */
                }

                /* Ensures DCP terminations are on by default. */
                (void)Cy_USBPD_Bch_Phy_ConfigSrcTerm(context, CHGB_SRC_TERM_DCP);

                /* Sets Comp1 to look for > 0.4 V on D+. */
                (void)Cy_USBPD_Bch_Phy_Config_Comp(context, 0U, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                        CHGB_VREF_0_325V, CHGB_COMP_EDGE_RISING);
            }

            /* Ensures that VBUS is 5 V. But do this only if PD is disabled. */
#if CCG_TYPE_A_PORT_ENABLE
            if (context->port != TYPE_A_PORT_ID)
#endif /* CCG_TYPE_A_PORT_ENABLE */
            {
                if(pdstack_ctx->dpmStat.pdDisabled != false)
                {
                    if(Cy_App_GetStatus(context->port)->is_vbus_on != true)
                    {
                        bc_psrc_enable(context, CY_PD_VSAFE_5V);
                    }
                }
            }
            break;

        case BC_FSM_EVT_CMP1_FIRE:
#if CCG_TYPE_A_PORT_ENABLE
            if (context->port == TYPE_A_PORT_ID)
            {
                glPtrSlnCbk->type_a_update_status (true, false);
            }
#endif /* CCG_TYPE_A_PORT_ENABLE */
            if(bc_stat->cur_mode == BC_CHARGE_APPLE)
            {
                /*
                 * A BC 1.2 based sink has been attached. Switch the
                 * terminations to BC 1.2.
                 */
                (void)Cy_USBPD_Bch_Phy_DisableComp(context, 1U);
                (void)Cy_USBPD_Bch_Phy_DisableComp(context, 0U);
                Cy_App_Bc_FsmClearEvt(context, BC_EVT_ALL_MASK);

                /*
                 * This may be a glitch. Do a small debounce to ensure that
                 * it is attached to a BC device. Additionally, the latest iPhones are
                 * also causing a glitch on DP during connect. This need to
                 * be filtered out. There is no real debounce logic required.
                 * If a BC device is attached, the line would stay low beyond
                 * a fixed duration; if not the line can revert back to 2.2 V.
                 */
                (void)Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, (void*)context, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1),
                                               CY_APP_BC_APPLE_DETECT_TIMER_PERIOD, bc_timer_cb);
            }
            else
            {
                bc_stat->attach = true;
                bc_set_current_mode(context, BC_CHARGE_NONE);
                bc_stat->cur_amp = BC_AMP_LIMIT;
                (void)Cy_USBPD_Bch_Phy_DisableComp(context, 1U);
                (void)Cy_USBPD_Bch_Phy_DisableComp(context, 0U);
                bc_stat->bc_fsm_state = BC_FSM_SRC_INITIAL_CONNECT;
                Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            }
            break;

#if ENABLE_APPLE_BC12_SUPPORT
        case BC_FSM_EVT_CMP2_FIRE:
            if(bc_stat->cur_mode == BC_CHARGE_APPLE)
            {
                /*
                 * A BC 1.2 based sink has been attached. Switch the
                 * terminations to BC 1.2.
                 */
                (void)Cy_USBPD_Bch_Phy_DisableComp(context, 1U);
                (void)Cy_USBPD_Bch_Phy_DisableComp(context, 0U);
                Cy_App_Bc_FsmClearEvt(context, BC_EVT_ALL_MASK);

                /*
                 * This may be a glitch. Do a small debounce to ensure that
                 * it is attached to a BC device. Additionally, the latest iPhones are
                 * also causing a glitch on DP during connect. This need to
                 * be filtered out. There is no real debounce logic required.
                 * If a BC device is attached, the line would stay low beyond
                 * a fixed duration; if not the line shall revert back to 2.2 V.
                 */
                (void)Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, (void*)context, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1),
                                               CY_APP_BC_APPLE_DETECT_TIMER_PERIOD, bc_timer_cb);

            }
            break;

        case BC_FSM_EVT_TIMEOUT1:
            if(bc_stat->cur_mode == BC_CHARGE_APPLE)
            {
                /*
                 * If the DP/DM voltage continues under 2.2 V, it is attached to
                 * BC device; proceed with BC detection. If not, go back to Apple
                 * mode and wait for DP/DM to go down again.
                 */
                comp0_flag = (Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
                            (cy_en_usbpd_bch_vref_t)chgb_dm_comp_ref, CHGB_COMP_NO_INTR) == false);
                comp1_flag = (Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_1_IDX, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                            CHGB_VREF_2_2V, CHGB_COMP_NO_INTR) == false);

                if (comp0_flag || comp1_flag)
                {
#if LEGACY_APPLE_SRC_SLN_TERM_ENABLE
                    sln_remove_apple_src_term(context);
#endif /* LEGACY_APPLE_SRC_SLN_TERM_ENABLE */

                    /* Ensures DCP terminations are on by default. */
                    (void)Cy_USBPD_Bch_Phy_ConfigSrcTerm(context, CHGB_SRC_TERM_DCP);

                    bc_stat->attach = false;
                    bc_stat->connected = false;
                    bc_set_current_mode(context, BC_CHARGE_NONE);

                    /* Sets Comp1 to look for > 0.4 V on D+. */
                    (void)Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                            CHGB_VREF_0_325V, CHGB_COMP_EDGE_RISING);

#if CCG_TYPE_A_PORT_ENABLE
                    if (context->port == TYPE_A_PORT_ID)
                    {
                        glPtrSlnCbk->type_a_update_status (true, false);
                    }
#endif /* CCG_TYPE_A_PORT_ENABLE */
                }
                else
                {
                    (void)Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_1_IDX, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                            CHGB_VREF_2_2V, CHGB_COMP_EDGE_FALLING);
#if (!defined(CY_DEVICE_CCG3))
                    (void)Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
                            (cy_en_usbpd_bch_vref_t)chgb_dm_comp_ref, CHGB_COMP_EDGE_FALLING);
#endif /* (!defined(CY_DEVICE_CCG3)) */
                }
            }
            break;

#endif /* (ENABLE_APPLE_BC12_SUPPORT) */
        default:
            /* No statement. */
            break;
    }
}

static void bc_fsm_src_initial_connect(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    const cy_stc_legacy_charging_cfg_t *chg_config = (cy_stc_legacy_charging_cfg_t *) context->usbpdConfig->legacyChargingConfig;

    switch(evt)
    {
        case BC_FSM_EVT_ENTRY:
            bc_stat->comp_rising = false;
            /* Sets Comp0 to look for < 0.4 V on D+ for 1 second. */
            (void)Cy_USBPD_Bch_Phy_Config_Comp(context, 0U,
                                               CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                                               CHGB_VREF_0_325V, CHGB_COMP_EDGE_FALLING);
#if (!defined(CY_DEVICE_CCG3))
            /* Sets Comp1 to ensure that DP does not go above 2.2 V. */
            (void)Cy_USBPD_Bch_Phy_Config_Comp(context, 1U,
                                                CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                                                CHGB_VREF_2V, CHGB_COMP_EDGE_RISING);
#endif /* (!defined(CY_DEVICE_CCG3)) */

            /* Starts TGLITCH_BC_DONE timer to ascertain Apple or others. */
            (void)Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, (void*)context, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1),
                                               CY_APP_BC_DCP_DETECT_TIMER_PERIOD, bc_timer_cb);
            break;

        case BC_FSM_EVT_CMP1_FIRE:
            if(bc_stat->comp_rising == false)
            {
                /*
                 * If AFC or BC 1.2 mode is enabled, then
                 * determine those devices.
                 */
                if((chg_config->srcSel & (BC_SRC_1_2_MODE_ENABLE_MASK
                        | BC_SRC_AFC_MODE_ENABLE_MASK)) != 0u)
                {
                    Cy_App_Bc_FsmClearEvt (context, BC_EVT_TIMEOUT2);
                    (void)Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, (void*)context,
                            CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER2),
                            150u, bc_timer_cb);
                }
                /*
                 * In QC only mode, stop DCP detect timer, and wait for DP to rise
                 * above 0.6 V again.
                 */
                else
                {
                    Cy_PdUtils_SwTimer_Stop(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1));
                    Cy_App_Bc_FsmClearEvt (context, BC_EVT_TIMEOUT1);
                }

                /* Sets Comp0 to look for > 0.4 V on D+. */
                (void)Cy_USBPD_Bch_Phy_Config_Comp(context, 0U,
                                                   CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                                                   CHGB_VREF_0_325V, CHGB_COMP_EDGE_RISING);
                bc_stat->comp_rising = true;
            }
            else
            {
                /* The DP line has gone above 0.6 V. Restart detection. */
                Cy_PdUtils_SwTimer_Stop(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER2));
                Cy_App_Bc_FsmClearEvt(context, BC_EVT_TIMEOUT2);
                (void)Cy_USBPD_Bch_Phy_DisableComp(context, 0U);
                Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            }
            break;

#if (!defined(CY_DEVICE_CCG3))
        case BC_FSM_EVT_CMP2_FIRE:
            /*
             * If TGLITCH_BC_DONE timer is running, it means that DP is above 2 V before
             * glitch filter timer expired. Wait for DP to come back in 0.4 V to 2 V range
             * and then start device detection again. Until then stay in DCP only mode.
             */
            if (Cy_PdUtils_SwTimer_IsRunning (bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1)))
            {
                /* DP is above 2 V, stop DCP detect timer. */
                Cy_PdUtils_SwTimer_Stop(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1));
                Cy_App_Bc_FsmClearEvt(context, BC_EVT_TIMEOUT1);
                /* Stop Apple device detect timer as well. */
                Cy_PdUtils_SwTimer_Stop(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER2));
                Cy_App_Bc_FsmClearEvt(context, BC_EVT_TIMEOUT2);

                /* Stop Comp0. */
                (void)Cy_USBPD_Bch_Phy_DisableComp(context, 0U);
                Cy_App_Bc_FsmClearEvt(context, BC_EVT_CMP1_FIRE);

                /*
                 * From this point, wait for DP to fall under 2 V. When it falls under
                 * 2 V, start device detection again.
                 */
                (void)Cy_USBPD_Bch_Phy_Config_Comp(context, 1U, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                          CHGB_VREF_2V, CHGB_COMP_EDGE_FALLING);
            }
            else
            {
                /* DP is now under 2 V. Start device detection again. */
                (void)Cy_USBPD_Bch_Phy_DisableComp(context, 1U);
                bc_stat->bc_fsm_state = BC_FSM_SRC_INITIAL_CONNECT;
                Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            }
            break;
#endif /* (!defined(CY_DEVICE_CCG3)) */

#if (!QC_AFC_CHARGING_DISABLED)
        case BC_FSM_EVT_TIMEOUT1:
            (void)Cy_USBPD_Bch_Phy_DisableComp(context, 0U);
            Cy_App_Bc_FsmClearEvt(context, BC_EVT_CMP1_FIRE);
            (void)Cy_USBPD_Bch_Phy_DisableComp(context, 1U);
            Cy_App_Bc_FsmClearEvt(context, BC_EVT_CMP2_FIRE);
            Cy_PdUtils_SwTimer_Stop(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER2));
            Cy_App_Bc_FsmClearEvt(context, BC_EVT_TIMEOUT2);
            bc_stat->bc_fsm_state = BC_FSM_SRC_OTHERS_CONNECTED;
            Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            break;
#endif /* (!QC_AFC_CHARGING_DISABLED) */

        case BC_FSM_EVT_TIMEOUT2:
            (void)Cy_USBPD_Bch_Phy_DisableComp(context, 0U);
            Cy_App_Bc_FsmClearEvt(context, BC_EVT_CMP1_FIRE);
            (void)Cy_USBPD_Bch_Phy_DisableComp(context, 1U);
            Cy_App_Bc_FsmClearEvt(context, BC_EVT_CMP2_FIRE);
            Cy_PdUtils_SwTimer_Stop(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1));
            Cy_App_Bc_FsmClearEvt(context, BC_EVT_TIMEOUT1);

            /* Treat this as a BC 1.2 device. */
            /* Indicates BC 1.2 device is connected so that current
             * monitoring can start. */
#if CCG_TYPE_A_PORT_ENABLE
            if (context->port == TYPE_A_PORT_ID)
            {
                glPtrSlnCbk->type_a_update_status (true, true);
            }
#endif /* CCG_TYPE_A_PORT_ENABLE */
            bc_stat->connected = true;
            bc_set_current_mode(context, BC_CHARGE_DCP);
            bc_stat->cur_amp = BC_AMP_LIMIT;
            /* Go back to init state and wait for DP attach event similar
             * to Apple mode. */
            bc_stat->bc_fsm_state = BC_FSM_SRC_LOOK_FOR_CONNECT;
            Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            break;

        default:
            /* No statement. */
            break;
    }
}

static void bc_fsm_src_apple_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    (void)context;
    (void)evt;
}

#if (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED))
static void bc_stop_src_cap_on_detect(cy_stc_usbpd_context_t *context)
{
#if CCG_TYPE_A_PORT_ENABLE
    if(context->port != TYPE_A_PORT_ID)
#endif /* CCG_TYPE_A_PORT_ENABLE */
    {
        cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);
        if (false == pdstack_ctx->dpmStat.pdDisabled)
        {
            (void)Cy_PdStack_Dpm_Disable(pdstack_ctx);
        }
    }
}

static void bc_fsm_src_others_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    const cy_stc_legacy_charging_cfg_t *chg_config = (cy_stc_legacy_charging_cfg_t *) context->usbpdConfig->legacyChargingConfig;

    switch(evt)
    {
        case BC_FSM_EVT_ENTRY:
            bc_stat->connected = true;

            if((chg_config->srcSel & (BC_SRC_AFC_MODE_ENABLE_MASK | BC_SRC_QC_MODE_ENABLE_MASK)) != 0u)
            {
                bc_stat->dp_dm_status.state = (uint16_t)QC_MODE_5V;
                bc_stat->old_dp_dm_status.state = (uint16_t)QC_MODE_5V;

                /* Try detecting QC or AFC. */
                (void)Cy_USBPD_Bch_Phy_ConfigSrcTerm(context, CHGB_SRC_TERM_QC);

                /* Lets the voltage settle. */
                Cy_SysLib_DelayUs(100);
            }
            /* Sets Comp1 to look for < 0.4 V on D- to ensure no short on DP/DM. */
            Cy_USBPD_Bch_Phy_Config_Comp(context, 0U, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
                CHGB_VREF_0_325V, CHGB_COMP_EDGE_FALLING);
            break;

        case BC_FSM_EVT_CMP1_FIRE:
            /* Moves to next state. */
            (void)Cy_USBPD_Bch_Phy_DisableComp(context, 0U);
            Cy_App_Bc_FsmClearEvt(context, BC_EVT_CMP1_FIRE);
            if((chg_config->srcSel & (BC_SRC_AFC_MODE_ENABLE_MASK | BC_SRC_QC_MODE_ENABLE_MASK)) != 0u)
            {
                bc_stat->bc_fsm_state = BC_FSM_SRC_QC_OR_AFC;
            }
            else
            {
                bc_stat->bc_fsm_state = BC_FSM_SRC_LOOK_FOR_CONNECT;
            }
            Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            break;

        default:
            /* Intentionally left empty. */
            break;
    }
}

static void bc_fsm_src_qc_or_afc(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    const cy_stc_legacy_charging_cfg_t *chg_config = (cy_stc_legacy_charging_cfg_t *) context->usbpdConfig->legacyChargingConfig;

    switch(evt)
    {
        case BC_FSM_EVT_ENTRY:
            if((chg_config->srcSel & BC_SRC_AFC_MODE_ENABLE_MASK) != 0u)
            {
                bc_stat->afc_src_msg_count = 0;
                bc_stat->afc_src_match_count = 0;
                bc_stat->afc_src_is_matched = false;
                (void)Cy_USBPD_Bch_AfcSrcInit(context);
                (void)Cy_USBPD_Bch_AfcSrcStart(context);
            }
            if((chg_config->srcSel & BC_SRC_QC_MODE_ENABLE_MASK) != 0u)
            {
                (void)Cy_USBPD_Bch_QcSrcInit(context);
            }
            break;

        case BC_FSM_EVT_DISCONNECT:
            /* Detached. */
            bc_stat->bc_fsm_state = BC_FSM_SRC_LOOK_FOR_CONNECT;
            Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            break;

        case BC_FSM_EVT_QC_CHANGE:
            /* Not AFC, move to QC detected. */
            (void)Cy_USBPD_Bch_AfcSrcStop(context);
            bc_set_current_mode(context, BC_CHARGE_QC2);
            bc_stat->bc_fsm_state = BC_FSM_SRC_QC_CONNECTED;
            Cy_App_Bc_FsmSetEvt(context, BC_EVT_QC_CHANGE);
            break;

        case BC_FSM_EVT_AFC_MSG_RCVD:
            /* Not QC, move to AFC detected. */
            (void)Cy_USBPD_Bch_QcSrcStop(context);
            bc_set_current_mode(context, BC_CHARGE_AFC);
            bc_stat->cur_amp = BC_AMP_LIMIT;
            bc_stat->bc_fsm_state = BC_FSM_SRC_AFC_CONNECTED;
            Cy_App_Bc_FsmSetEvt(context, BC_EVT_AFC_MSG_RCVD);
            bc_stop_src_cap_on_detect(context);
            break;

        case  BC_FSM_EVT_AFC_MSG_SEND_FAIL:
            (void)Cy_USBPD_Bch_AfcSrcStart(context);
            break;

        case BC_FSM_EVT_AFC_RESET_RCVD:
            (void)Cy_USBPD_Bch_AfcSrcStop(context);
            bc_stat->afc_src_msg_count = 0;
            bc_stat->afc_src_match_count = 0;
            bc_stat->afc_src_is_matched = false;
            bc_stat->afc_tx_active = (uint8_t)false;
            (void)Cy_USBPD_Bch_AfcSrcStart(context);
            break;

        default:
            /* Intentionally left empty. */
            break;
    }
}

static void bc_fsm_src_qc_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    int pulse_count;
    uint32_t new_volt;
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    const cy_stc_legacy_charging_cfg_t *chg_config = (cy_stc_legacy_charging_cfg_t *) context->usbpdConfig->legacyChargingConfig;

    switch(evt)
    {
        case BC_FSM_EVT_DISCONNECT:
            /* Detached. */
            (void)Cy_USBPD_Bch_QcSrcContModeStop(context);
            bc_stat->bc_fsm_state = BC_FSM_SRC_LOOK_FOR_CONNECT;

            Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            if(Cy_App_GetStatus(context->port)->psrc_volt != CY_PD_VSAFE_5V)
            {
                /* Updates the voltage to VSAFE_5V on exit from QC state. */
                bc_psrc_enable(context, CY_PD_VSAFE_5V);
            }
            break;

        case BC_FSM_EVT_QC_CHANGE:
            if(bc_stat->dp_dm_status.state != QC_MODE_CONT)
            {
                bc_set_current_mode(context, BC_CHARGE_QC2);
                /* Disables Continuous mode operation. */
                (void)Cy_USBPD_Bch_QcSrcContModeStop(context);
                Cy_App_Bc_FsmClearEvt(context, BC_EVT_QC_CONT);
            }

            if(bc_stat->dp_dm_status.state != QC_MODE_CONT)
            {
                /*
                 * Here if voltage is detected other than 5 V in QC mode, stop sending source caps.
                 */
                bc_stop_src_cap_on_detect(context);
            }

            switch(bc_stat->dp_dm_status.state)
            {
                case (uint16_t)QC_MODE_5V:
                    bc_stat->cur_amp = QC_AMP_5V;
                    bc_psrc_enable(context, CY_PD_VSAFE_5V);
                    break;

                case (uint16_t)QC_MODE_9V:
                    bc_stat->cur_amp = QC_AMP_9V;
                    bc_psrc_enable(context, CY_PD_VSAFE_9V);
                    break;

                case (uint16_t)QC_MODE_12V:
                    bc_stat->cur_amp = QC_AMP_12V;

                    bc_psrc_enable(context, CY_PD_VSAFE_12V);
                    break;

                case (uint16_t)QC_MODE_20V:
                    if((chg_config->qcSrcType == BC_SRC_QC_VER_2_CLASS_B_VAL) ||
                       (chg_config->qcSrcType == BC_SRC_QC_VER_3_CLASS_B_VAL)
                       )
                    {
                        bc_stat->cur_amp = QC_AMP_20V;
                        bc_psrc_enable(context, CY_PD_VSAFE_20V);
                    }
                    break;

                case (uint16_t)QC_MODE_CONT:
                    if(chg_config->qcSrcType >= BC_SRC_QC_VER_3_CLASS_A_VAL)
                    {
                        bc_stat->cur_amp = QC_AMP_CONT;
                        bc_set_current_mode(context, BC_CHARGE_QC3);
                        /* Enables Continuous mode operation. */
                        (void)Cy_USBPD_Bch_QcSrcContModeStart(context);

#if (CCG_CABLE_COMP_ENABLE) && (CCG_CABLE_COMP_IN_QC_3_0_DISABLE)
                        /*
                         * When entering into QC3 continuous mode, a new explicit
                         * voltage request call is needed to remove cable
                         * compensation voltage from the existing VBUS voltage.
                         */
                        bc_psrc_enable(context, Cy_App_GetStatus(context->port)->psrc_volt);
#endif /* (CCG_CABLE_COMP_ENABLE) && (CCG_CABLE_COMP_IN_QC_3_0_DISABLE) */
                    }
                    break;

                default:
                    /* Intentionally left empty. */
                    break;
            }
            break;

        case BC_FSM_EVT_QC_CONT:
            pulse_count = Cy_USBPD_Bch_Get_QcPulseCount(context);
            if(pulse_count > 0)
            {
                /* Voltage change in mV units. Each pulse cause 200 mV change. */
                new_volt = (uint32_t)pulse_count * QC_CONT_VOLT_CHANGE_PER_PULSE;
                new_volt = new_volt + Cy_App_GetStatus(context->port)->psrc_volt;
            }
            else
            {
                new_volt = 0u - pulse_count;
                /* Voltage change in mV units. Each pulse cause 200 mV change. */
                new_volt = new_volt * QC_CONT_VOLT_CHANGE_PER_PULSE;
                if(new_volt <= ((uint32_t)Cy_App_GetStatus(context->port)->psrc_volt - QC3_MIN_VOLT))
                {
                    new_volt = Cy_App_GetStatus(context->port)->psrc_volt - new_volt;
                }
                else
                {
                    new_volt = QC3_MIN_VOLT;
                }
            }

            /* Checks for minimum and maximum voltage levels and limit to allowed range. */
            if(new_volt < QC3_MIN_VOLT)
            {
                new_volt = QC3_MIN_VOLT;
            }
            if ((chg_config->qcSrcType  == BC_SRC_QC_VER_3_CLASS_A_VAL) && (new_volt > CY_PD_VSAFE_12V))
            {
                new_volt = CY_PD_VSAFE_12V;
            }
            if ((chg_config->qcSrcType == BC_SRC_QC_VER_3_CLASS_B_VAL) && (new_volt > CY_PD_VSAFE_20V))
            {
                new_volt = CY_PD_VSAFE_20V;
            }
            /* If the voltage is higher than 5 V, then stop PD. */
            if(new_volt > CY_PD_VSAFE_5V)
            {
                bc_stop_src_cap_on_detect(context);
            }
            /* Updates voltage only if there is a difference. */
            if(Cy_App_GetStatus(context->port)->psrc_volt != new_volt)
            {
                bc_stat->cur_amp = QC_AMP_CONT;
                bc_psrc_enable(context, (uint16_t)new_volt);
            }
            /* Clears out the handled pulse count. */
            Cy_USBPD_Bch_Update_QcPulseCount(context, pulse_count);
            break;

        default:
            /* Intentionally left empty. */
            break;
    }
}

static void bc_afc_src_evaluate_match(cy_stc_usbpd_context_t *context)
{
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    if(bc_stat->afc_src_msg_count >= 3u)
    {
        if(bc_stat->afc_src_match_count >= 2u)
        {
            bc_stat->afc_src_msg_count = 0;
            bc_stat->afc_src_match_count = 0;
            bc_stat->cur_amp = ((uint16_t)AFC_BASE_AMP + (CY_PDUTILS_BYTE_GET_LOWER_NIBBLE((uint16_t)bc_stat->afc_src_matched_byte) * (uint16_t)AFC_AMP_STEP));
            bc_psrc_enable(context, (AFC_BASE_VOLT + (CY_PDUTILS_BYTE_GET_UPPER_NIBBLE(bc_stat->afc_src_matched_byte) * AFC_VOLT_STEP)));
            (void)Cy_USBPD_Bch_AfcSrcStart(context);
        }
        else
        {
            /* Enters default operation and clears AFC counters if three attempts fail.  */
            bc_stat->afc_src_msg_count = 0;
            bc_stat->afc_src_match_count = 0;
            bc_stat->cur_volt = CY_PD_VSAFE_5V;
            bc_stat->cur_amp = BC_AMP_LIMIT;
            bc_psrc_enable(context, CY_PD_VSAFE_5V);
        }
    }
    else
    {
        (void)Cy_USBPD_Bch_AfcSrcStart(context);
    }
}

static uint8_t bc_afc_src_get_vi_count(cy_stc_usbpd_context_t *context)
{
    return ((uint8_t)context->usbpdConfig->legacyChargingConfig->afcSrcCapCnt);
}

static uint8_t* bc_afc_src_get_vi_ptr(cy_stc_usbpd_context_t *context)
{
    return ((uint8_t *)context->usbpdConfig->legacyChargingConfig->afcSrcCaps);
}

static void bc_afc_src_handle_rcvd_msg(cy_stc_usbpd_context_t *context)
{
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    /* Matches data received and sends proper response same byte if match else all. */
    uint8_t *rcvd_vi = Cy_USBPD_Bch_Get_AfcDataPtr(context);
    uint8_t i;
    uint8_t *src_vi = (uint8_t *)bc_afc_src_get_vi_ptr(context);
    uint8_t src_count = bc_afc_src_get_vi_count(context);
    /* Sets Tx active flag. */
    bc_stat->afc_tx_active = (uint8_t)true;
    for(i = 0; i < src_count; i++)
    {
        if(((rcvd_vi[0] & 0xF0u) == (src_vi[i] & 0xF0u) ) &&
           ((rcvd_vi[0] & 0xFu) <= (src_vi[i] & 0xFu)))
        {
            bc_stat->afc_src_cur_match_byte = rcvd_vi[0];
            bc_stat->afc_src_is_matched = true;
            Cy_USBPD_Bch_Afc_Set_Tx_Data(context, &rcvd_vi[0], 1);
            return;
        }
    }
    bc_stat->afc_src_is_matched = false;
    /* If there is no match, clears both the AFC counters. */
    bc_stat->afc_src_msg_count = 0;
    bc_stat->afc_src_match_count = 0;
    Cy_USBPD_Bch_Afc_Set_Tx_Data(context, src_vi, src_count);
}

static void bc_fsm_src_afc_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];

    switch(evt)
    {
        case BC_FSM_EVT_DISCONNECT:
            /* Detached. */
            (void)Cy_USBPD_Bch_AfcSrcStop(context);
            bc_stat->bc_fsm_state = BC_FSM_SRC_LOOK_FOR_CONNECT;
            Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            if(Cy_App_GetStatus(context->port)->psrc_volt != CY_PD_VSAFE_5V)
            {
                /* Updates the voltage to CY_PD_VSAFE_5V on exit from AFC state. */
                bc_psrc_enable(context, CY_PD_VSAFE_5V);
            }
            break;

        case BC_FSM_EVT_AFC_MSG_RCVD:
            bc_afc_src_handle_rcvd_msg(context);
            break;

        case BC_FSM_EVT_AFC_RESET_RCVD:
            (void)Cy_USBPD_Bch_AfcSrcStop(context);
            bc_stat->cur_volt = CY_PD_VSAFE_5V;
            bc_psrc_enable(context, CY_PD_VSAFE_5V);
            bc_stat->afc_src_msg_count = 0;
            bc_stat->afc_src_match_count = 0;
            bc_stat->afc_src_is_matched = false;
            bc_stat->afc_tx_active = (uint8_t)false;
            (void)Cy_USBPD_Bch_AfcSrcStart(context);
            break;

        case BC_FSM_EVT_AFC_MSG_SENT:
            if(bc_stat->afc_src_is_matched == true)
            {
                /* Increments AFC counters on match only. */
                bc_stat->afc_src_msg_count++;
                if(bc_stat->afc_src_cur_match_byte == bc_stat->afc_src_last_match_byte)
                {
                    bc_stat->afc_src_match_count++;
                    if(bc_stat->afc_src_match_count == 2u)
                    {
                        bc_stat->afc_src_matched_byte = bc_stat->afc_src_cur_match_byte;
                    }
                }
            }
            bc_stat->afc_tx_active = (uint8_t)false;
            bc_stat->afc_src_last_match_byte = bc_stat->afc_src_cur_match_byte;
            bc_afc_src_evaluate_match(context);
            break;

        case BC_FSM_EVT_AFC_MSG_SEND_FAIL:

            /* If transmission is active, increment the message count. */
            if (bc_stat->afc_tx_active == (uint8_t)true)
            {
                bc_stat->afc_tx_active = (uint8_t)false;
                bc_stat->afc_src_msg_count++;
                bc_afc_src_evaluate_match(context);
            }
            else
            {
                /* This is a timeout event. Restart hardware state machine. */
                (void)Cy_USBPD_Bch_AfcSrcStart(context);
            }
            break;

        default:
            /* Intentionally left empty. */
            break;
    }
}
#endif /* (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED)) */
#endif /* (!CY_PD_SINK_ONLY) */

#if (!(CY_PD_SOURCE_ONLY)) && (!BC_SOURCE_ONLY)

#if QC_AFC_SNK_EN
#if (LEGACY_PD_PARALLEL_OPER)
static void pd_stop_on_bc_hv_detect(cy_stc_usbpd_context_t * context)
{
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);
    cy_stc_pdstack_dpm_status_t* dpm_stat = &(pdstack_ctx->dpmStat);
    if(false == dpm_stat->pdDisabled)
    {
        Cy_PdStack_Dpm_Disable(pdstack_ctx);
    }
}
#endif /* LEGACY_PD_PARALLEL_OPER */

void bc_snk_try_next_protocol(cy_stc_usbpd_context_t * context)
{
    uint8_t c_port=context->port;
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[c_port];
    const cy_stc_legacy_charging_cfg_t *chg_cfg = (cy_stc_legacy_charging_cfg_t *) context->usbpdConfig->legacyChargingConfig;
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);

    /* Tries AFC protocol if enabled. */
    if (chg_cfg->snkSel & BC_SINK_AFC_MODE_ENABLE_MASK)
    {
        if(bc_stat->cur_mode == BC_CHARGE_DCP)
        {
            bc_stat->bc_fsm_state = BC_FSM_SINK_AFC_CHARGER_DETECT;
            Cy_App_Bc_FsmSetEvt (context, BC_EVT_ENTRY);
            bc_set_current_mode(context, BC_CHARGE_AFC);
#if (LEGACY_PD_PARALLEL_OPER)
            /* QC/AFC detection starts so stop DPM. */
            pd_stop_on_bc_hv_detect(context);
#endif /* LEGACY_PD_PARALLEL_OPER */
            return;
        }
    }

    /* Tries QC 2.0 protocol if enabled. */
    if (chg_cfg->snkSel & BC_SINK_QC_MODE_ENABLE_MASK)
    {
        if((bc_stat->cur_mode == BC_CHARGE_DCP)||(bc_stat->cur_mode == BC_CHARGE_AFC))
        {
            /* Tries QC 2.0 protocol if enabled. */
            bc_stat->bc_fsm_state = BC_FSM_SINK_QC_CHARGER_DETECTED;
            bc_set_current_mode(context, BC_CHARGE_QC2);

            Cy_App_Bc_FsmSetEvt (context, BC_EVT_ENTRY);
#if (LEGACY_PD_PARALLEL_OPER)
            /* QC/AFC detection starts so stop DPM. */
            pd_stop_on_bc_hv_detect(context);
#endif
            return;
        }
    }

    /* Ensures VBUS is set to 5 V. */
    Cy_App_Sink_SetVoltage(pdstack_ctx, CY_PD_VSAFE_5V);

    /* Tries Apple protocol if enabled and BC 1.2 current mismatch is still present. */
    if (
            (chg_cfg->snkSel & BC_SINK_APPLE_MODE_ENABLE_MASK) &&
            (bc_stat->max_amp > CY_PD_I_1P5A)
       )
    {
        bc_stat->bc_fsm_state = BC_FSM_SINK_APPLE_CHARGER_DETECT;
        Cy_App_Bc_FsmSetEvt (context, BC_EVT_ENTRY);
    }
    else
    {
        /* Checks BC 1.2 DCP mode mismatch. */
        if (
                (bc_stat->min_volt > CY_PD_VSAFE_5V) ||
                (bc_stat->max_amp > CY_PD_I_1P5A)
           )
        {
            Cy_App_Bc_Stop(context);
        }
        else
        {
            /* QC/AFC/Apple detections are disabled, stay in DCP mode. */
            /* DCP is connected. */
            bc_set_current_mode(context, BC_CHARGE_DCP);
            bc_psnk_enable(context);
            Cy_USBPD_Bch_Phy_Dis(context);
        }
    }
}
#endif /* QC_AFC_SNK_EN */

static void bc_fsm_sink_start(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    const cy_stc_legacy_charging_cfg_t *chg_config = (cy_stc_legacy_charging_cfg_t *) context->usbpdConfig->legacyChargingConfig;

    if (evt == BC_FSM_EVT_ENTRY)
    {
#if QC_AFC_SNK_EN
        Cy_USBPD_Bch_Phy_DisableComp(context, BC_CMP_0_IDX);
        Cy_PdUtils_SwTimer_StopRange(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1),
                CY_APP_GET_TIMER_ID(context, CY_APP_BC_DP_DM_DEBOUNCE_TIMER));
        Cy_App_Bc_FsmClearEvt(context, BC_EVT_ALL_MASK);
#endif /* QC_AFC_SNK_EN */
        /* Resets the charger detect hardware block by disabling and enabling it. */
        Cy_USBPD_Bch_Phy_Dis(context);
        Cy_USBPD_Bch_Phy_En(context);

#if QC_AFC_SNK_EN
        Cy_App_Bc_FsmSetEvt(context,BC_CHARGE_NONE);

        /* Starts BC 1.2, if enabled. */
        if (chg_config->snkSel & BC_SINK_1_2_MODE_ENABLE_MASK)
        {
            bc_stat->bc_fsm_state = BC_FSM_SINK_PRIMARY_CHARGER_DETECT;
            Cy_App_Bc_FsmSetEvt (context, BC_EVT_ENTRY);
        }
        /* Moves to Apple charger detection state, if enabled. */
        else if (chg_config->snkSel & BC_SINK_APPLE_MODE_ENABLE_MASK)
        {
            bc_stat->bc_fsm_state = BC_FSM_SINK_APPLE_CHARGER_DETECT;
            Cy_App_Bc_FsmSetEvt (context, BC_EVT_ENTRY);
        }
        /* No legacy charging mode is enabled. Assume Type-C only source. Stop BC FSM. */
        else
        {
            Cy_App_Bc_Stop(context);
        }
#else
        bc_set_current_mode(context, BC_CHARGE_NONE);

#if (!APPLE_SINK_DISABLE)
        /* Moves to Apple charger detection state, if enabled. */
        if (chg_config->snkSel & BC_SINK_APPLE_MODE_ENABLE_MASK)
        {
            bc_stat->bc_fsm_state = BC_FSM_SINK_APPLE_CHARGER_DETECT;
            Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
        }
        else
#endif /* (!APPLE_SINK_DISABLE) */
        {
            /* Starts BC 1.2, if enabled. */
            if (chg_config->snkSel & BC_SINK_1_2_MODE_ENABLE_MASK)
            {
                bc_stat->bc_fsm_state = BC_FSM_SINK_PRIMARY_CHARGER_DETECT;
                Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            }
            /* No legacy charging mode is enabled. Assume Type-C only source. */
            else
            {
                bc_stat->bc_fsm_state = BC_FSM_SINK_TYPE_C_ONLY_SOURCE_CONNECTED;
                Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            }
        }
#endif /* QC_AFC_SNK_EN */
    }
}

static void bc_fsm_sink_apple_charger_detect(cy_stc_usbpd_context_t * context, cy_en_bc_fsm_evt_t evt)
{
#if (!APPLE_SINK_DISABLE)
    bool apple_charger_detected = false;
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
#if (!QC_AFC_SNK_EN)
    const cy_stc_legacy_charging_cfg_t *chg_cfg = (cy_stc_legacy_charging_cfg_t *) context->usbpdConfig->legacyChargingConfig;
#endif /* (!QC_AFC_SNK_EN) */
    /*
     * CCG sink needs to detect if it is connected to Apple charger or not.
     * Apple charger is expected to drive >1 V on both D+/-. So measure D+/-
     * voltage and determine the type of charger.
     */
    if (evt == BC_FSM_EVT_ENTRY)
    {
        /* Apple RDAT_LKG resistors on D+ and D-. */
        Cy_USBPD_Bch_ApplyRdatLkgDp(context);
        Cy_USBPD_Bch_ApplyRdatLkgDm(context);

        /*
         * Need to review if a timer can be used here instead of just checking current
         * voltage on D+/-.
         */

        /* Checks if D+ > 1 V. */
        if (Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
            CHGB_VREF_0_85V, CHGB_COMP_NO_INTR) == true)
        {
            /* Checks if D- > 1 V. */
            if (Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
            CHGB_VREF_0_85V, CHGB_COMP_NO_INTR) == true)
            {
                /* Apple charger detected. */
                apple_charger_detected = true;
            }
        }

        if (apple_charger_detected == true)
        {
            /* Known that CCG is connected to Apple charger. Detect Brick ID. */
            bc_set_current_mode(context, BC_CHARGE_APPLE);

            bc_stat->bc_fsm_state = BC_FSM_SINK_APPLE_BRICK_ID_DETECT;
            Cy_App_Bc_FsmSetEvt (context, BC_EVT_ENTRY);
        }
        /* Apple charger is not detected. */
#if QC_AFC_SNK_EN
        /* No legacy charging mode is enabled. Assume Type-C only source. */
        else
        {
            Cy_App_Bc_Stop(context);
        }
#else
        else
        {
            /* Starts BC 1.2, if enabled. */
            if (chg_cfg->snkSel & BC_SINK_1_2_MODE_ENABLE_MASK)
            {
                bc_stat->bc_fsm_state = BC_FSM_SINK_PRIMARY_CHARGER_DETECT;
                Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            }
            /* No legacy charging mode is enabled. Assume Type-C only source. */
            else
            {
                bc_stat->bc_fsm_state = BC_FSM_SINK_TYPE_C_ONLY_SOURCE_CONNECTED;
                Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            }
        }
#endif /* QC_AFC_SNK_EN */
    }
#else
    (void)evt;
    (void)context;
#endif /* (!APPLE_SINK_DISABLE) */
}

static void bc_fsm_sink_apple_brick_id_detect(cy_stc_usbpd_context_t * context, cy_en_bc_fsm_evt_t evt)
{
#if (!APPLE_SINK_DISABLE)
    /* Detects Apple Brick ID here as required by Apple Brick ID spec. */

    /*
     * DP and DM can be connected to three terminations:
     * TERM1: 1 - 2.22 V
     * TERM2: 2.22 - 2.89 V
     * TERM3: 2.89+ V
     * Encoding user here is: TERM1 : 1, TERM2: 2, TERM3: 3.
     */
    cy_en_bc_apple_term_t dp_term = APPLE_TERM1, dm_term = APPLE_TERM1;
    if (evt == BC_FSM_EVT_ENTRY)
    {
        /*
         * It is known that DP is greater than 1 V. Check if DP is greater than
         * 2.9 V. If yes, term3 on DP exists. If not, check if DP is greater than 2.2 V.
         * If yes, term2 on DP exists. Else, DP has term1.
         */

        /* Enable 2.9 V detection for Apple Brick ID. */
        Cy_USBPD_Bch_Enable_AppleDet(context);

        if (Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
            CHGB_VREF_2_9V, CHGB_COMP_NO_INTR) == true)
        {
            dp_term = APPLE_TERM3;
        }
        else
        {
            if (Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                CHGB_VREF_2_2V, CHGB_COMP_NO_INTR) == true)
            {
                dp_term = APPLE_TERM2;
            }
        }

        /* Similar test for DM. */
        if (Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
            CHGB_VREF_2_9V, CHGB_COMP_NO_INTR) == true)
        {
            dm_term = APPLE_TERM3;
        }
        else
        {
            if (Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
                CHGB_VREF_2_2V, CHGB_COMP_NO_INTR) == true)
            {
                dm_term = APPLE_TERM2;
            }
        }
        /* Disables 2.9 V detection for Apple Brick ID. */
        Cy_USBPD_Bch_Disable_AppleDet(context);

        /* Evaluates Apple termination detected. */
        bc_eval_apple_brick_id (context, (cy_en_bc_apple_brick_id_t)((uint8_t)dp_term | ((uint8_t)dm_term << 0x04)));

    }
#else
    (void)context;
    (void)evt;
#endif /* (!APPLE_SINK_DISABLE) */
}

/* This state is for primary charger detect. See BC 1.2 specification for details. */
static void bc_fsm_sink_primary_charger_detect(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
#if QC_AFC_SNK_EN
    const cy_stc_legacy_charging_cfg_t *chg_cfg = (cy_stc_legacy_charging_cfg_t *) context->usbpdConfig->legacyChargingConfig;
#endif /* QC_AFC_SNK_EN */
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];

    switch (evt)
    {
        case BC_FSM_EVT_ENTRY:
            /* Apply terminations on D+/- and start VDP_DM_SRC_ON timer to schedule the next step. */
            Cy_USBPD_Bch_Phy_ConfigSnkTerm (context, CHGB_SINK_TERM_PCD);
            Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, (void *)context,
                    CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1),
                    CY_APP_BC_VDP_DM_SRC_ON_PERIOD, bc_timer_cb);
            break;

        case BC_FSM_EVT_TIMEOUT1:
            /* Now measure D- and see if D- is pulled up to VDP_SRC. */
            if (Cy_USBPD_Bch_Phy_Config_Comp (context, 0, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
                        CHGB_VREF_0_325V, CHGB_COMP_NO_INTR) == true)
            {
#if QC_AFC_SNK_EN
                if (Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                    CHGB_VREF_0_85V, CHGB_COMP_NO_INTR) == true)
                {
                    /* Checks if D- > 1 V. */
                    if (Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
                    CHGB_VREF_0_85V, CHGB_COMP_NO_INTR) == true)
                    {
                        /* Apple charger detected. */
                        /* Move to Apple charger detection state, if enabled. */
                        if (chg_cfg->snkSel & BC_SINK_APPLE_MODE_ENABLE_MASK)
                        {
                            bc_stat->bc_fsm_state = BC_FSM_SINK_APPLE_CHARGER_DETECT;
                            Cy_App_Bc_FsmSetEvt (context, BC_EVT_ENTRY);
                        }
                        else
                        {
                            /* Type-C only source connected. */
                            Cy_App_Bc_Stop(context);
                        }
                    }
                }
                else
#endif /* QC_AFC_SNK_EN */
                {
                    /* Starts timer for source to differentiate between primary and secondary detection. */
                    Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, context, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER2), CY_APP_BC_VDMSRC_EN_DIS_PERIOD, bc_timer_cb);
                }
            }
#if QC_AFC_SNK_EN
            /* Apple pull-ups are not seen now but anyway move to Apple charger detection state, if enabled. */
            else if (chg_cfg->snkSel & BC_SINK_APPLE_MODE_ENABLE_MASK)
            {
                bc_stat->bc_fsm_state = BC_FSM_SINK_APPLE_CHARGER_DETECT;
                Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            }
#endif /* QC_AFC_SNK_EN */
            else
            {
                /* Type-C only source connected. */
#if QC_AFC_SNK_EN
                Cy_App_Bc_Stop(context);
#else           
                bc_stat->bc_fsm_state = BC_FSM_SINK_TYPE_C_ONLY_SOURCE_CONNECTED;
                Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
#endif /* QC_AFC_SNK_EN */
            }

            /* Removes applied terminations */
            Cy_USBPD_Bch_Phy_RemoveTerm(context);
            break;

        case BC_FSM_EVT_TIMEOUT2:
            /* Proceeds to secondary detection for CDP/DCP detection. */
            bc_stat->bc_fsm_state = BC_FSM_SINK_SECONDARY_CHARGER_DETECT;
            Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);
            break;

        default:
            break;
    }
}

static void bc_fsm_sink_type_c_only_source_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    if (evt == BC_FSM_EVT_ENTRY)
    {
        /* Disables charger detect block operation. */
        Cy_USBPD_Bch_Phy_Dis(context);

#if defined(CY_DEVICE_CCG6)
        /* TBD: Enable DP/DM Mux. */
#endif /* defined(CY_DEVICE_CCG6) */
    }
}

/* This state is used to perform secondary charger detect. See BC 1.2 specification for details. */
static void bc_fsm_sink_secondary_charger_detect(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];

    switch (evt)
    {
        case BC_FSM_EVT_ENTRY:
            /* Applies terminations on D+/-. */
            Cy_USBPD_Bch_Phy_ConfigSnkTerm(context, CHGB_SINK_TERM_SCD);

            /* Starts timer to apply VDM_SRC for TVDM_SRC_ON. */
            Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, (void *)context,
                    CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1),
                    CY_APP_BC_VDP_DM_SRC_ON_PERIOD, bc_timer_cb);
            break;

        case BC_FSM_EVT_TIMEOUT1:
            /* Measure D+ and see if D- is pulled-up to VDM_SRC. */
            if (Cy_USBPD_Bch_Phy_Config_Comp(context, 0, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                        CHGB_VREF_0_325V, CHGB_COMP_NO_INTR) == true)
            {
                /* DCP connected. */
                bc_set_current_mode(context, BC_CHARGE_DCP);
                bc_stat->bc_fsm_state = BC_FSM_SINK_DCP_CONNECTED;
            }
            else
            {
                /* CDP connected. */
                bc_set_current_mode(context, BC_CHARGE_CDP);
                bc_stat->bc_fsm_state = BC_FSM_SINK_CDP_CONNECTED;
            }

            Cy_App_Bc_FsmSetEvt(context, BC_EVT_ENTRY);

            /* Removes applied terminations. */
            Cy_USBPD_Bch_Phy_RemoveTerm(context);
            break;

        default:
            break;
    }
}

#if QC_AFC_SNK_EN
static void bc_fsm_sink_dcp_connected(cy_stc_usbpd_context_t * context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_bc_status_t* bc_stat = &gl_bc_status[context->port];
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);

    switch (evt)
    {
        case BC_FSM_EVT_ENTRY:
            /*
             * It is known that DCP is connected. If charger supports QC, it will open
             * the D+/- short due to this D- will fall below VDP_SRC.
             */

            /* Puts back VDP_SRC and IDM_SINK to measure D-. */
            Cy_USBPD_Bch_Phy_ConfigSnkTerm(context, CHGB_SINK_TERM_PCD);
            /* Sets up the comparator to monitor D- and see if it is pulled down. */
            Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
                    CHGB_VREF_0_325V, CHGB_COMP_EDGE_FALLING);
            /*
             * Starts TGLITCH_BC_DONE (1.5 s). If D- is pulled low within this time,
             * then charger is QC/AFC. Otherwise it is DCP.
             */
            Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, context, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1),
                CY_APP_BC_GLITCH_BC_DONE_TIMER_PERIOD, bc_timer_cb);
            bc_stat->cur_timer = BC_SINK_TIMER_BC_DONE;

            break;

        case BC_FSM_EVT_TIMEOUT1:
            if (bc_stat->cur_timer == BC_SINK_TIMER_BC_DONE)
            {
                /* DCP did not remove D+/- short. So the charger does not support QC. Stay in DCP mode. */

                /* Ensures VBUS is set to 5 V. */
                Cy_App_Sink_SetVoltage(pdstack_ctx, CY_PD_VSAFE_5V);

                /* Sets the battery charging current to 1.5 A. */
                Cy_App_Sink_SetCurrent(pdstack_ctx, CY_PD_I_1P5A);
                bc_psnk_enable(context);
            }
            if (bc_stat->cur_timer == BC_SINK_TIMER_DM_HIGH)
            {
                /*
                 * D+/- short removed and D- sampled for T_GLITCH_DM_HIGH
                 * timer. QC/AFC charger detected.
                 */
                bc_snk_try_next_protocol(context);
            }
            Cy_USBPD_Bch_Phy_DisableComp(context, BC_CMP_0_IDX);
            break;

        case BC_FSM_EVT_CMP1_FIRE:
            if (bc_stat->cur_timer == BC_SINK_TIMER_BC_DONE)
            {
                /*
                 * DCP removed D+/- short. Sink is expected to debounce D- for
                 * T_GLITCH_DM_HIGH (40 ms) before making VBUS request. Set up the
                 * comparator and timer.
                 */
                Cy_PdUtils_SwTimer_Stop(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1));
                Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
                    CHGB_VREF_0_325V, CHGB_COMP_EDGE_RISING);
                Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, context, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1),
                    CY_APP_BC_GLITCH_DM_HIGH_TIMER_PERIOD, bc_timer_cb);
                bc_stat->cur_timer = BC_SINK_TIMER_DM_HIGH;
            }
            else if (bc_stat->cur_timer == BC_SINK_TIMER_DM_HIGH)
            {
                /* D- does not held low for T_GLITCH_DM_HIGH. */
                Cy_PdUtils_SwTimer_Stop(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1));
            }
            break;

        default:
            break;
    }
}
#else
/* The Type-C source is a Downstream Charging Port (DCP). */
static void bc_fsm_sink_dcp_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);

    if (evt == BC_FSM_EVT_ENTRY)
    {
        Cy_USBPD_Bch_Phy_Dis(context);

#if defined(CY_DEVICE_CCG6)
        /* TBD: Enable DP/DM Mux if not done so far. */
#endif /* defined(CY_DEVICE_CCG6) */

        /* Ensures VBUS is set to 5 V. */
        Cy_App_Sink_SetVoltage(pdstack_ctx, CY_PD_VSAFE_5V);

        /* Sets current limit to 1.5 A (DCP) and enable sink FET if not already turned ON. */
        Cy_App_Sink_SetCurrent(pdstack_ctx, CY_PD_I_1P5A);
        Cy_App_Sink_Enable(pdstack_ctx);
    }
}
#endif /* QC_AFC_SNK_EN */

/* The Type-C source is a Standard Downstream Port (SDP). */
static void bc_fsm_sink_sdp_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);

    if (evt == BC_FSM_EVT_ENTRY)
    {
        /* SDP is connected. */
#if QC_AFC_SNK_EN
#if !CY_PD_SINK_ONLY
        Cy_App_Sink_SetCurrent(pdstack_ctx, CY_PD_ISAFE_DEF);
        Cy_App_Sink_Enable(pdstack_ctx);
#else
        /* In case of BCR2, use higher Type-C current limit. */
        Cy_App_Bc_Stop(context);
        (void)pdstack_ctx;
#endif /* !CY_PD_SINK_ONLY  */
#else
        Cy_App_Sink_SetCurrent(pdstack_ctx, CY_PD_ISAFE_DEF);
        Cy_App_Sink_Enable(pdstack_ctx);
#endif /* QC_AFC_SNK_EN */
        Cy_USBPD_Bch_Phy_Dis(context);

#if defined(CY_DEVICE_CCG6)
        /* TBD: Enable DP/DM Mux if not done so far. */
#endif /* defined(CY_DEVICE_CCG6) */
    }
}

/* The Type-C source is a Charging Downstream Port (CDP). */
static void bc_fsm_sink_cdp_connected(cy_stc_usbpd_context_t *context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);

    if (evt == BC_FSM_EVT_ENTRY)
    {
        Cy_USBPD_Bch_Phy_Dis(context);

#if defined(CY_DEVICE_CCG6)
        /* TBD: Enable DP/DM Mux if not done so far. */
#endif /* defined(CY_DEVICE_CCG6) */

        /* Sets current limit to 1.5 A (CDP) and enable sink FET if not already turned ON. */
        Cy_App_Sink_SetCurrent(pdstack_ctx, CY_PD_I_1P5A);
        Cy_App_Sink_Enable(pdstack_ctx);
    }
}

#if QC_AFC_SNK_EN
static uint8_t bc_sink_calculate_required_current(cy_stc_usbpd_context_t* context, uint16_t cur10mA)
{
    (void)context;
    uint32_t new_current;
    /* Converts current to 10 mA units. */
    new_current = cur10mA;

    if(new_current < AFC_BASE_AMP)
    {
        /* AFC min. current is 750 mA. */
        new_current = AFC_BASE_AMP;
    }
    /* AFC protocol defines max. charging current 3 A. */
    return ((CY_PDUTILS_GET_MIN(((new_current - AFC_BASE_AMP)/AFC_AMP_STEP), AFC_MAX_AMP))  & 0x0F);
}

static void bc_fsm_sink_afc_charger_detect(cy_stc_usbpd_context_t* context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_bc_status_t* bc_stat = &gl_bc_status[context->port];
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);
    uint16_t vbus = 0;
    uint8_t i;
    uint8_t rcvd_vi;
    bool new_VI_BYTE_flag = false;

    switch(evt)
    {
        case BC_FSM_EVT_ENTRY:
            /* Sets AFC sink termination. */
            Cy_USBPD_Bch_Phy_ConfigSnkTerm(context,CHGB_SINK_TERM_AFC);

            Cy_USBPD_Bch_AfcSinkInit(context);
            /* Forms initial request. */
            /* Sets Vmax from rotary switch at firs.t */
            bc_stat->afc_snk_cur_vi_byte = (((((bc_stat->max_volt - AFC_BASE_VOLT)/AFC_VOLT_STEP) << 4 ) & 0xF0) |
                                    bc_sink_calculate_required_current(context, bc_stat->max_amp));

            Cy_App_Sink_SetVoltage(pdstack_ctx, bc_stat->max_volt);
            bc_stat->cur_amp = CY_PDUTILS_GET_MIN(bc_stat->max_amp, CY_PD_I_3A);
            Cy_App_Sink_SetCurrent(pdstack_ctx, CY_PDUTILS_GET_MIN(bc_stat->max_amp, CY_PD_I_3A));

            Cy_USBPD_Bch_Afc_Set_Tx_Data(context, &bc_stat->afc_snk_cur_vi_byte, 1);
            /* Enables AFC interrupt, init Rx buffer, and start transaction. */
            Cy_USBPD_Bch_AfcSinkStart(context);

            bc_stat->afc_retry_count = 0;
            Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, context, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER2), CY_APP_BC_AFC_SNK_VI_BYTE_PERIOD, bc_timer_cb);
            break;

        case BC_FSM_EVT_TIMEOUT2:
            /* Checks received data in buffer. */
            if(Cy_USBPD_Bch_AfcGetRxDataCount(context) > 0)
            {
                /* Checks if sent and received VI_BYTE are equal. */
                rcvd_vi = *Cy_USBPD_Bch_AfcGetRxDataPtr(context);
                if(rcvd_vi == bc_stat->afc_snk_cur_vi_byte)
                {
                    vbus = Cy_App_VbusGetValue(pdstack_ctx);
                    uint16_t vbus_vi = ((rcvd_vi >> 4) * AFC_VOLT_STEP) + AFC_BASE_VOLT;
                    /* Checks if required VBUS voltage is set with +/- 5% tolerance. */
                    if((vbus > (vbus_vi - CY_PDUTILS_DIV_ROUND_UP(vbus_vi, 20))) && (vbus < (vbus_vi + CY_PDUTILS_DIV_ROUND_UP(vbus_vi, 20))))
                    {
                        /* VBUS is set as required, send ping with the same VI_BYTE. */

                        if(bc_sink_mismatch_check(context) == false)
                        {
                            if(bc_stat->connected == false)
                            {
                                bc_stat->cur_volt = vbus_vi;
                                bc_set_current_mode(context, BC_CHARGE_AFC);
                                bc_psnk_enable(context);
                            }
                        }
                        else
                        {
                            Cy_USBPD_Bch_AfcSinkStop(context);
                            bc_stat->connected = false;
                            bc_set_current_mode(context, BC_CHARGE_AFC);
                            /* No suitable capabilities were found. Try QC mode. */
                            bc_snk_try_next_protocol(context);
                            break;
                        }
                    }
                    else
                    {
                        /* Do nothing. SRC awaits on three equal VI_BYTEs, so send VI_BYTE again.  */
                    }
                }
                else
                {
                    /* AFC SRC capabilities are received. Choose a new VI_BYTE. */
                    /* Do reverse scan as AFC capabilities are ordered from the lowest to the highest voltages. */
                    for( i= (Cy_USBPD_Bch_AfcGetRxDataCount(context)); i--; )
                    {
                        /* At first check if provided voltage is suitable. */
                        if((bc_stat->afc_snk_cur_vi_byte & 0xF0) >= (*(Cy_USBPD_Bch_AfcGetRxDataPtr(context) + (i-1)) & 0xF0))
                        {
                            /* If voltages are equal, check if provided current is suitable. */
                            if(bc_sink_calculate_required_current(context, bc_stat->min_amp) <= (*(Cy_USBPD_Bch_AfcGetRxDataPtr(context) + (i-1)) & 0x0F))
                            {
                                uint16_t vbus_vi = ((*(Cy_USBPD_Bch_AfcGetRxDataPtr(context) + (i-1)) >> 4) * AFC_VOLT_STEP) + AFC_BASE_VOLT;

                                /* Gets the next AFC SRC capability byte if provided voltage is less than minimally required. */
                                if(vbus_vi < bc_stat->min_volt)
                                {
                                    continue;
                                }

                                Cy_App_Sink_SetVoltage(pdstack_ctx, vbus_vi);
                                bc_stat->afc_snk_cur_vi_byte = *(Cy_USBPD_Bch_AfcGetRxDataPtr(context) + (i-1));
                                new_VI_BYTE_flag = true;
                                break;
                            }
                        }
                        else
                        {
                            /* Tries the next capability from the list. */
                        }
                    }
                    if(new_VI_BYTE_flag == false)
                    {
                        Cy_USBPD_Bch_AfcSinkStop(context);
                        bc_stat->connected = false;
                        /* No suitable capabilities were found. Try QC mode. */
                        bc_set_current_mode(context, BC_CHARGE_AFC);
                        /* Tries Apple protocol in case of mismatch or stay in DCP. */
                        bc_snk_try_next_protocol(context);
                        break;
                    }
                }
                /* Sends old or BYTE_VI. */
                Cy_USBPD_Bch_Afc_Set_Tx_Data(context, &bc_stat->afc_snk_cur_vi_byte, 1);
                Cy_USBPD_Bch_AfcSinkStart(context);
                Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, context,
                           CY_APP_GET_TIMER_ID(context,CY_USBPD_APP_BC_GENERIC_TIMER2), CY_APP_BC_AFC_SNK_VI_BYTE_PERIOD, bc_timer_cb);
            }
            else
            {
                /* Nothing is received from SRC. Do retry. */
                bc_stat->afc_retry_count ++;
                if(bc_stat->afc_retry_count < AFC_DETECT_RETRY_COUNT)
                {
                    Cy_USBPD_Bch_Afc_Set_Tx_Data(context, &bc_stat->afc_snk_cur_vi_byte, 1);
                    Cy_USBPD_Bch_AfcSinkStart(context);
                    Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, context, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER2), CY_APP_BC_AFC_SNK_VI_BYTE_PERIOD, bc_timer_cb);
                }
                else
                {
                    Cy_USBPD_Bch_AfcSinkStop(context);
                    bc_stat->connected = false;
                    /* AFC charger is not found. Try QC mode. */
                    bc_set_current_mode(context, BC_CHARGE_AFC);
                    /* Tries Apple protocol in case of mismatch or stay in DCP. */
                    bc_snk_try_next_protocol(context);
                    break;
                }
            }
            break;

        /* Nothing to handle. Wait on timeout event. */
        case BC_FSM_EVT_AFC_MSG_SENT:
        case BC_FSM_EVT_AFC_MSG_RCVD:
            break;

        case BC_FSM_EVT_DISCONNECT:
            /* Detached. */
            Cy_USBPD_Bch_AfcSinkStop(context);
            Cy_App_Bc_Stop(context);

            break;

        case BC_FSM_EVT_AFC_RESET_RCVD:
            /* AFC reset. Treat it as a failure and try the next BC protocol. */
            Cy_USBPD_Bch_AfcSinkStop(context);
            Cy_App_Sink_SetVoltage(pdstack_ctx, CY_PD_VSAFE_5V);
            bc_stat->cur_volt = CY_PD_VSAFE_5V;

            bc_set_current_mode(context, BC_CHARGE_AFC);

            bc_snk_try_next_protocol(context);
            break;

        default:
            break;

    }
}

static void bc_apply_sink_term(cy_stc_usbpd_context_t * context, uint16_t voltage)
{
    switch (voltage)
    {
        case CY_PD_VSAFE_9V:
            Cy_USBPD_Bch_Phy_ConfigSnkTerm(context, CHGB_SINK_TERM_QC_9V);
            break;
        case CY_PD_VSAFE_12V:
            Cy_USBPD_Bch_Phy_ConfigSnkTerm(context, CHGB_SINK_TERM_QC_12V);
            break;
        case CY_PD_VSAFE_20V:
            Cy_USBPD_Bch_Phy_ConfigSnkTerm(context, CHGB_SINK_TERM_QC_20V);
            break;
        default:
            Cy_USBPD_Bch_Phy_ConfigSnkTerm(context, CHGB_SINK_TERM_QC_5V);
            break;
    }
}

static void bc_fsm_sink_qc_charger_detected(cy_stc_usbpd_context_t * context, cy_en_bc_fsm_evt_t evt)
{
    cy_stc_bc_status_t* bc_stat = &gl_bc_status[context->port];
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);
    uint16_t vbus = 0;

    switch (evt)
    {
        case BC_FSM_EVT_ENTRY:
            /* QC charger detected. Ask for VBUS maximum. */
            /* QC does not support the 15 V request. */
            if(bc_stat->max_volt == CY_PD_VSAFE_15V)
            {
                if(bc_stat->min_volt < CY_PD_VSAFE_15V)
                {
                    bc_stat->requested_qc_volt = CY_PD_VSAFE_12V;
                }
                else
                {
                    bc_stat->connected = false;
                    /* Tries Apple protocol in case of mismatch or stay in DCP. */
                    bc_snk_try_next_protocol(context);
                }
            }
            else
            {
                bc_stat->requested_qc_volt = bc_stat->max_volt;
            }

            bc_apply_sink_term(context, bc_stat->requested_qc_volt);
            Cy_App_Sink_SetVoltage(pdstack_ctx, bc_stat->requested_qc_volt);
            /* Wait for T_V_NEW_REQUEST and then check the new voltage. */
            Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, context, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1),
                CY_APP_BC_V_NEW_REQUEST_TIMER_PERIOD, bc_timer_cb);
            break;

        case BC_FSM_EVT_TIMEOUT1:
            /*
             * T_V_NEW_REQUEST timer timed out. Ask for higher VBUS if current
             * VBUS is 5 V.
             */
            vbus = Cy_App_VbusGetValue(pdstack_ctx);

            /* Checks if required VBUS voltage is set with +/- 5% tolerance. */
            if((vbus > (bc_stat->requested_qc_volt - CY_PDUTILS_DIV_ROUND_UP(bc_stat->requested_qc_volt, 20))) &&
                vbus < (bc_stat->requested_qc_volt + CY_PDUTILS_DIV_ROUND_UP(bc_stat->requested_qc_volt, 20)))
            {
                /* VBUS is present. Enable PSink. */
                Cy_App_Sink_SetCurrent (pdstack_ctx, CY_PDUTILS_GET_MIN(bc_stat->max_amp, CY_PD_I_1P5A));
                bc_set_current_mode(context, BC_CHARGE_QC2);

                if(bc_sink_mismatch_check(context) == false)
                {
                    if(bc_stat->connected == false)
                    {
                        bc_psnk_enable(context);
                    }
                }
                else
                {
                    bc_stat->connected = false;
                    /* Tries Apple protocol in case of mismatch or stay in DCP. */
                    bc_snk_try_next_protocol(context);
                }
            }
            else
            {
                /* No requested VBUS from charger. Try to request a lower VBUS. */
                switch (bc_stat->requested_qc_volt)
                {
                    case CY_PD_VSAFE_20V:
                        bc_stat->requested_qc_volt = CY_PD_VSAFE_12V;
                        break;
                    case CY_PD_VSAFE_12V:
                        bc_stat->requested_qc_volt = CY_PD_VSAFE_9V;
                        break;
                    case CY_PD_VSAFE_9V:
                        bc_stat->requested_qc_volt = CY_PD_VSAFE_5V;
                        break;
                    default:
                        bc_stat->requested_qc_volt = CY_PD_VSAFE_5V;
                        break;
                }

                if(bc_stat->min_volt > bc_stat->requested_qc_volt)
                {
                    bc_stat->connected = false;
                    /* Tries Apple protocol in case of mismatch or stay in DCP. */
                    bc_snk_try_next_protocol(context);
                    break;
                }

                /* Tries to request lower VBUS from the charger. */
                Cy_App_Sink_SetVoltage(pdstack_ctx, bc_stat->requested_qc_volt);
                bc_apply_sink_term(context, bc_stat->requested_qc_volt);
                /* Starts timer again. */
                /* Wait for T_V_NEW_REQUEST and then check the new voltage. */
                Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, context, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1),
                    CY_APP_BC_V_NEW_REQUEST_TIMER_PERIOD, bc_timer_cb);
            }
            break;
        default:
            break;
    }
}
#endif /* QC_AFC_SNK_EN */
#endif /* (!(CCG_SOURCE_ONLY)) && (!BC_SOURCE_ONLY) */

/* Callbacks from the PDL driver. */
static void bc_phy_cbk_handler(void *callbackCtx, uint32_t event)
{
    cy_stc_usbpd_context_t *usbpdcontext = (cy_stc_usbpd_context_t *)callbackCtx;
    Cy_App_Bc_FsmSetEvt(usbpdcontext, event);
}

#endif /* (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD)) */

#if (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED))
static void bc_debounce(cy_stc_usbpd_context_t *context)
{
    uint32_t i;
    cy_stc_bc_dp_dm_state_t new_state;
    cy_en_usbpd_bch_comp_pinput_t pinput = CHGB_COMP_P_DP;
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    const cy_stc_legacy_charging_cfg_t *chg_config = (cy_stc_legacy_charging_cfg_t *) context->usbpdConfig->legacyChargingConfig;

    new_state.state = (uint16_t)QC_MODE_RSVD;

    /* Gets current status. */
    for(i = 0 ; i < 2u ; i++)
    {
        if(i == 1u)
        {
            pinput = CHGB_COMP_P_DM;
        }

        if (Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, pinput, CHGB_COMP_N_VREF,
                CHGB_VREF_0_325V, CHGB_COMP_NO_INTR) == false)
        {
            new_state.d[i] = (uint8_t)BC_D_GND;
        }
        else if(Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, pinput, CHGB_COMP_N_VREF,
                          CHGB_VREF_2V, CHGB_COMP_NO_INTR) == false)
        {
            new_state.d[i] = (uint8_t)BC_D_0_6V;
        }
        else
        {
            new_state.d[i] = (uint8_t)BC_D_3_3V;
        }
    }

    /* Does debounce. */
    if(bc_stat->dp_dm_status.state == new_state.state)
    {
        bc_stat->old_dp_dm_status.state = bc_stat->dp_dm_status.state;
        Cy_PdUtils_SwTimer_Stop(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_DP_DM_DEBOUNCE_TIMER));
        return;
    }

    if(bc_stat->old_dp_dm_status.state != new_state.state)
    {
        /*
         * Do not debounce DP/DM state if current mode is QC Continuous mode and new state
         * translates to non-5 V Fixed mode. Only transition out of continuous mode is either
         * 5 V fixed mode or disconnect.
         */
        /* QAC suppression 2996: new_state is a union object. Even though state member is not explicitly assigned,
         * its value may change as other members of the unions are assigned prior to this check. Therefore, it is
         * not a redundant check. */
        if ((bc_stat->dp_dm_status.state != QC_MODE_CONT) || (new_state.state == QC_MODE_5V) || /* PRQA S 2996 */
            (new_state.state == 0u)) /* PRQA S 2996 */
        {
            Cy_PdUtils_SwTimer_Start(bc_stat->ptr_timer_ctx, (void *)context,
                                CY_APP_GET_TIMER_ID(context, CY_APP_BC_DP_DM_DEBOUNCE_TIMER),
                                CY_APP_BC_DP_DM_DEBOUNCE_TIMER_PERIOD, NULL);
            bc_stat->old_dp_dm_status.state = new_state.state;
            return;
        }
        if(bc_stat->dp_dm_status.state == QC_MODE_CONT)
        {
            return;
        }
    }

    if(Cy_PdUtils_SwTimer_IsRunning(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_DP_DM_DEBOUNCE_TIMER)) == false)
    {
        bc_stat->dp_dm_status.state = bc_stat->old_dp_dm_status.state;
        /*
         * If both DP and DM are High-Z, generate CMP1_FIRE interrupt which translates
         * to device disconnect event. Otherwise, generate QC mode change interrupt.
         */
        if (bc_stat->dp_dm_status.state == 0u)
        {
            Cy_App_Bc_FsmSetEvt(context, BC_EVT_DISCONNECT);
        }
        else
        {
            /* Proceeds with QC state change only if QC is enabled. */
            if ((chg_config->srcSel & BC_SRC_QC_MODE_ENABLE_MASK) != 0u)
            {
                Cy_App_Bc_FsmSetEvt(context, BC_EVT_QC_CHANGE);
            }
        }
    }
}
#endif /* (!QC_AFC_CHARGING_DISABLED) */

#if QC_AFC_SNK_EN
bool bc_is_qc_afc_charging_active(cy_stc_usbpd_context_t * context)
{
    cy_stc_bc_status_t* bc_stat = &gl_bc_status[context->port];
    return ((bc_stat->cur_mode == BC_CHARGE_QC2) || (bc_stat->cur_mode == BC_CHARGE_AFC));
}
#endif /* QC_AFC_SNK_EN */

cy_en_usbpd_status_t Cy_App_Bc_Init(cy_stc_usbpd_context_t *context, cy_stc_pdutils_sw_timer_t *timerCtx)
{
    cy_en_usbpd_status_t stat = CY_USBPD_STAT_BAD_PARAM;
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    bc_stat->ptr_timer_ctx = timerCtx;

#if (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD))
    if ((context != NULL) && (context->port < NO_OF_BC_PORTS))
    {
        if (Cy_USBPD_Bch_Phy_Init (context, bc_phy_cbk_handler) == CY_USBPD_STAT_SUCCESS)
        {
            stat = CY_USBPD_STAT_SUCCESS;
            bc_stat->bc_fsm_state = BC_FSM_OFF;
            bc_stat->bc_evt       = 0u;
        }
#if QC_AFC_SNK_EN
        bc_stat->max_volt = BC_QC_AFC_SNK_MAX_VOLT;
        bc_stat->min_volt = BC_QC_AFC_SNK_MIN_VOLT;
        bc_stat->max_amp = BC_QC_AFC_SNK_MAX_CUR;
        bc_stat->min_amp = BC_QC_AFC_SNK_MIN_CUR;
#endif /* QC_AFC_SNK_EN */
    }
#else
    CY_UNUSED_PARAMETER(context);
#endif /* (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD)) */

    return stat;
}

cy_en_usbpd_status_t Cy_App_Bc_Start(cy_stc_usbpd_context_t *context)
{
#if (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD))
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];

#if CCG_TYPE_A_PORT_ENABLE
    if(context->port == TYPE_A_PORT_ID)
    {
        bc_stat->bc_fsm_state = BC_FSM_SRC_LOOK_FOR_CONNECT;
        bc_stat->bc_evt = BC_EVT_ENTRY;
#if CY_APP_RTOS_ENABLED
        /* USBPD context is typecasted to PDStack context because Type-A port
         * does not have PDStack context and Cy_App_SendRtosEvent uses only the port
         * variable from the context and it is the same in both PDStack and USBPD context. */
        Cy_App_SendRtosEvent((cy_stc_pdstack_context_t *)context);
#endif /* CY_APP_RTOS_ENABLED */
        return CY_USBPD_STAT_SUCCESS;
    }
#endif /* CCG_TYPE_A_PORT_ENABLE */

#if ((!CY_PD_SOURCE_ONLY) && (!BC_SOURCE_ONLY))
    if(context->dpmGetConfig()->curPortRole == CY_PD_PRT_ROLE_SINK)
    {
        /* Moves to start state for sink mode operation. */
        bc_stat->bc_fsm_state = BC_FSM_SINK_START;
    }
    else
#endif /* ((!CY_PD_SOURCE_ONLY) && (!BC_SOURCE_ONLY)) */
    {
        bc_stat->bc_fsm_state = BC_FSM_SRC_LOOK_FOR_CONNECT;
    }
    bc_stat->bc_evt = BC_EVT_ENTRY;
    bc_stat->connected = false;

#if CY_APP_RTOS_ENABLED
    /* Sends RTOS event. */
    Cy_App_SendRtosEvent((cy_stc_pdstack_context_t *)context);
#endif /* CY_APP_RTOS_ENABLED */
#else
    CY_UNUSED_PARAMETER(context);
#endif /* (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD)) */

    return CY_USBPD_STAT_SUCCESS;
}

cy_en_usbpd_status_t Cy_App_Bc_Stop(cy_stc_usbpd_context_t *context)
{
#if (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD))
    if (context->port >= NO_OF_BC_PORTS)
    {
        return CY_USBPD_STAT_BAD_PARAM;
    }

    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];

    /* Do nothing if it is already off. */
    if (bc_stat->bc_fsm_state == BC_FSM_OFF)
    {
        return CY_USBPD_STAT_SUCCESS;
    }

#if (LEGACY_APPLE_SRC_SLN_TERM_ENABLE)
    /* Disables external Apple source termination. */
    sln_remove_apple_src_term(context);
#endif /* (LEGACY_APPLE_SRC_SLN_TERM_ENABLE) */

    Cy_USBPD_Bch_Phy_DisableComp(context, 0u);
    Cy_PdUtils_SwTimer_StopRange(bc_stat->ptr_timer_ctx,
            CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1),
            CY_APP_GET_TIMER_ID(context, CY_APP_CDP_DP_DM_POLL_TIMER));
    Cy_USBPD_Bch_Phy_Dis(context);

    bc_stat->bc_fsm_state = BC_FSM_OFF;
    bc_stat->bc_evt       = 0u;
    bc_stat->connected    = false;
    bc_stat->attach       = false;
#if QC_AFC_SNK_EN
    bc_stat->mismatch = false;
#endif
    bc_stat->cur_volt = CY_PD_VSAFE_0V;
    bc_set_current_mode(context, BC_CHARGE_NONE);

#if CCG_TYPE_A_PORT_ENABLE
    if(context->port != TYPE_A_PORT_ID)
#endif /* CCG_TYPE_A_PORT_ENABLE */
    {
        cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);

        /* If there is no PD contract, ensure current limit is set to minimum. */
        if (pdstack_ctx->dpmConfig.contractExist == 0u)
        {
            Cy_App_Sink_SetCurrent (pdstack_ctx, CY_PD_ISAFE_0A);
        }
#if defined(CY_DEVICE_CCG6)
        if (pdstack_ctx->dpmConfig.attach != 0u)
        {
            /* TBD: Enable the DP/DM MUX for USB 2.0 data connection. */
        }
#endif /* defined(CY_DEVICE_CCG6) */
    }
#else
    CY_UNUSED_PARAMETER(context);
#endif /* (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD)) */

    return CY_USBPD_STAT_SUCCESS;
}

bool Cy_App_Bc_IsActive(cy_stc_usbpd_context_t *context)
{
#if (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD))
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    bool ret = false;

    if (bc_stat->bc_fsm_state != BC_FSM_OFF)
    {
        ret = true;
    }

    return ret;
#else
    CY_UNUSED_PARAMETER(context);
    return false;
#endif /* (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD)) */
}

cy_en_usbpd_status_t Cy_App_Bc_Task(cy_stc_usbpd_context_t *context)
{
#if (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD))
    cy_stc_bc_status_t* bc_stat = &gl_bc_status[context->port];
    uint8_t evt;

#if CCG_TYPE_A_PORT_ENABLE
    if(context->port != TYPE_A_PORT_ID)
#endif /* CCG_TYPE_A_PORT_ENABLE */
    {
        if (context->dpmGetConfig()->connect == 0u)
        {
            Cy_App_Bc_Stop(context);
            return CY_USBPD_STAT_SUCCESS;
        }
    }

#if (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED))
    if((bc_stat->bc_fsm_state == BC_FSM_SRC_QC_OR_AFC) || (bc_stat->bc_fsm_state == BC_FSM_SRC_QC_CONNECTED)
        || (bc_stat->bc_fsm_state == BC_FSM_SRC_AFC_CONNECTED))
    {
        bc_debounce(context);
    }
#endif /* (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED)) */

    /* Gets the next event to be processed. */
    evt = Cy_PdUtils_EventGroup_GetEvent((uint32_t *)&(bc_stat->bc_evt), true);

    if (evt < BC_FSM_MAX_EVTS)
    {
        /* Calls the FSM handler function if a valid event exists. */
        bc_fsm_table[bc_stat->bc_fsm_state](context, (cy_en_bc_fsm_evt_t)evt);
    }
#else
    CY_UNUSED_PARAMETER(context);
#endif /* (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD)) */

    return CY_USBPD_STAT_SUCCESS;
}

bool Cy_App_Bc_PrepareDeepSleep(cy_stc_usbpd_context_t *context)
{
#if (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD))
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
#if (!QC_AFC_CHARGING_DISABLED)
    bool chgb_dp_status_flag, chgb_dm_status_flag;
#endif /* ((!QC_AFC_CHARGING_DISABLED)) */

    if(context->dpmGetConfig()->connect == false)
    {
        return true;
    }

#if (defined(CY_IP_M0S8USBPD) || defined(CY_DEVICE_PMG1S3))
    if (bc_stat->bc_fsm_state != BC_FSM_OFF)
    {
        /* Do not go to deep sleep if legacy charging is active. */
        return false;
    }
#endif /* (defined(CY_IP_M0S8USBPD) || defined(CY_DEVICE_PMG1S3)) */

#if (!QC_AFC_SNK_EN)
    if (
            (bc_stat->bc_evt != 0u) ||
            (Cy_PdUtils_SwTimer_RangeEnabled (bc_stat->ptr_timer_ctx,
                    CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1), CY_APP_GET_TIMER_ID(context, CY_APP_BC_DP_DM_DEBOUNCE_TIMER)))
       )
#else
    if ((bc_stat->bc_evt != 0) ||
        (Cy_PdUtils_SwTimer_IsRunning(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER1)) == true) ||
        (Cy_PdUtils_SwTimer_IsRunning(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_GENERIC_TIMER2)) == true) ||
        (Cy_PdUtils_SwTimer_IsRunning(bc_stat->ptr_timer_ctx, CY_APP_GET_TIMER_ID(context, CY_APP_BC_DP_DM_DEBOUNCE_TIMER)) == true))
#endif /* (!QC_AFC_SNK_EN)) */
    {
        return false;
    }

#if (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED))
    if (bc_stat->bc_fsm_state == BC_FSM_SRC_AFC_CONNECTED)
    {
        return false;
    }

    if (bc_stat->bc_fsm_state == BC_FSM_SRC_QC_CONNECTED)
    {
        /* Enables master DP/DM sense. */
        Cy_USBPD_Bch_QcSrcMasterSenseEn(context);
    }
#endif /* (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED)) */

    Cy_USBPD_Bch_Phy_Config_DeepSleep (context);

#if (!QC_AFC_CHARGING_DISABLED)
    /*
     * Checks the DP/DM status before going to Sleep mode.
     * This is to make sure DP/DM debounce edges are not missed between
     * Cy_App_Bc_Task() and Cy_APP_Bc_PrepareDeepSleep() functions.
     */
    chgb_dp_status_flag = Cy_USBPD_Bch_Phy_DpStat(context);
    chgb_dm_status_flag = Cy_USBPD_Bch_Phy_DmStat(context);
    if((((uint8_t)(chgb_dp_status_flag == true) ^ (uint8_t)(bc_stat->dp_dm_status.d[0] == (uint8_t)BC_D_3_3V)) != 0u) ||
      (((uint8_t)(chgb_dm_status_flag == true) ^ (uint8_t)(bc_stat->dp_dm_status.d[1] == (uint8_t)BC_D_3_3V)) != 0u))
    {
        return false;
    }
#endif /* (!QC_AFC_CHARGING_DISABLED) */

#if (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED))
        /*
         * Configure DP/DM comparators to enable device wakeup in QC 2.0 mode.
         * When a QC device updates QC 2.0 mode, these interrupts will wake up the device.
         * QC 2.0 mode request is then debounced in bc_debounce() routine.
         * Setting comparators based on the current QC 2.0 mode.
         */
        if ((bc_stat->bc_fsm_state == BC_FSM_SRC_QC_OR_AFC) ||
            (bc_stat->bc_fsm_state == BC_FSM_SRC_QC_CONNECTED))
        {
            switch (bc_stat->dp_dm_status.state)
            {
                case (uint16_t)QC_MODE_5V:
                    /* For < 0.6 V transition on DP. */
                    Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                          CHGB_VREF_0_325V, CHGB_COMP_EDGE_FALLING);
                    /* For > 0 V transition on DM. */
                    Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_1_IDX, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
                          CHGB_VREF_0_325V, CHGB_COMP_EDGE_RISING);
                    /* QCOM RCVR interrupt will be used for > 0.6 V transition on DP. */
                    break;

                case (uint16_t)QC_MODE_9V:
                    /* For < 0.6 V transition on DM. */
                    Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_1_IDX, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
                              CHGB_VREF_0_325V, CHGB_COMP_EDGE_FALLING);
                    /*
                     * QCOM RCVR interrupts will be used for < 3.3 V transition on DP
                     * and > 0.6 V transition on DM.
                     */
                    break;

                case (uint16_t)QC_MODE_12V:
                    /* For < 0.6 V transition on DP. */
                    Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                              CHGB_VREF_0_325V, CHGB_COMP_EDGE_FALLING);
                     /* For < 0.6 V transition on DM. */
                    Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_1_IDX, CHGB_COMP_P_DM, CHGB_COMP_N_VREF,
                              CHGB_VREF_0_325V, CHGB_COMP_EDGE_FALLING);
                    /* QCOM RCVR interrupts will be used for > 0.6 V transition on DP and DM. */
                    break;

                case (uint16_t)QC_MODE_20V:
                    /* Nothing to do here because QC RCVR interrupt will detect < 3.3 V
                     * transition on DP and DM. */
                    break;

                case (uint16_t)QC_MODE_CONT:
                    /* For < 0.6 V transition on DP. */
                    Cy_USBPD_Bch_Phy_Config_Comp(context,BC_CMP_0_IDX, CHGB_COMP_P_DP, CHGB_COMP_N_VREF,
                              CHGB_VREF_0_325V, CHGB_COMP_EDGE_FALLING);
                    /*
                     * QCOM RCVR interrupt will be used for < 3.3 V transition on DM and > 0.6 V
                     * transition on DP.
                     */
                    break;

                default:
                    /* Intentionally left empty. */
                    break;
            }
        }
#endif /* (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED)) */

   return true;
#else
    CY_UNUSED_PARAMETER(context);
    return true;
#endif /* (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD)) */
}

void Cy_App_Bc_Resume(cy_stc_usbpd_context_t *context)
{
#if (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD))
#if (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED))
    /*
     * If in QC or AFC mode, you might have configured comparators to wakeup device
     * on DP/DM activity. Disable the comparators.
     */
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];

    if ((bc_stat->bc_fsm_state == BC_FSM_SRC_QC_OR_AFC) ||
        (bc_stat->bc_fsm_state == BC_FSM_SRC_QC_CONNECTED))
    {
        Cy_USBPD_Bch_Phy_DisableComp(context, BC_CMP_0_IDX);
        Cy_App_Bc_FsmClearEvt(context, BC_EVT_CMP1_FIRE);
        Cy_USBPD_Bch_Phy_DisableComp(context, BC_CMP_1_IDX);
        Cy_App_Bc_FsmClearEvt(context, BC_EVT_CMP2_FIRE);
    }
#endif /* (!(QC_SRC_AFC_CHARGING_DISABLED || QC_AFC_CHARGING_DISABLED)) */
    Cy_USBPD_Bch_Phy_Config_Wakeup(context);
#endif /* (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD)) */
}

const cy_stc_bc_status_t* Cy_App_Bc_GetStatus(cy_stc_usbpd_context_t *context)
{
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    return ((const cy_stc_bc_status_t *)bc_stat);
}

void Cy_App_Bc_PdEventHandler(cy_stc_usbpd_context_t *context, cy_en_pdstack_app_evt_t evt, const void* dat)
{
    CY_UNUSED_PARAMETER(dat);
#if (LEGACY_PD_PARALLEL_OPER)
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
#endif /* (LEGACY_PD_PARALLEL_OPER) */
    const cy_stc_legacy_charging_cfg_t *chg_cfg = (cy_stc_legacy_charging_cfg_t *) context->usbpdConfig->legacyChargingConfig;
    cy_stc_pdstack_context_t *pdstack_ctx = Cy_PdStack_Dpm_GetContext(context->port);
#if (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD))
    switch (evt)
    {
        case APP_EVT_DISCONNECT:
        case APP_EVT_VBUS_PORT_DISABLE:
        case APP_EVT_TYPE_C_ERROR_RECOVERY:
            Cy_App_Bc_Stop(context);
            break;

        case APP_EVT_PE_DISABLED:
#if (!LEGACY_PD_PARALLEL_OPER)
            /* Starts legacy state machine once PE is disabled. */
            if (pdstack_ctx->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SOURCE)
            {
                /* Already started in case of parallel legacy and PD source operation. */
                if ((chg_cfg->srcSel & BATT_CHARGING_SRC_MASK) != 0u)
                {
                    Cy_App_Bc_Start(context);
                }
            }

#if ((!CY_PD_SOURCE_ONLY) && (!BC_SOURCE_ONLY))
            if (pdstack_ctx->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SINK)
            {
                if ((chg_cfg->snkSel & BATT_CHARGING_SINK_MASK) != 0u)
                {
                    Cy_App_Bc_Start(context);
                }
            }
#endif /* ((!CY_PD_SOURCE_ONLY) && (!BC_SOURCE_ONLY)) */
#endif /* (!LEGACY_PD_PARALLEL_OPER) */
            break;

#if (LEGACY_PD_PARALLEL_OPER)
        case APP_EVT_TYPEC_ATTACH:
        case APP_EVT_CONNECT:
            if(bc_stat->bc_fsm_state != BC_FSM_OFF)
            {
                return;
            }
#if ((!CY_PD_SOURCE_ONLY) && (!BC_SOURCE_ONLY))
            if (pdstack_ctx->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SINK)
            {
                if ((chg_cfg->snkSel & BATT_CHARGING_SINK_MASK) != 0)
                {
                    /* Starts battery charging state machine once in the connected state if Rp detected is default Rp. */
                    if (pdstack_ctx->dpmConfig.snkCurLevel == CY_PD_RD_USB)
                    {
                        Cy_App_Bc_Start(context);
                    }
                }
            }
            else
#endif /* ((!CY_PD_SOURCE_ONLY) && (!BC_SOURCE_ONLY)) */
            {
                if ((chg_cfg->srcSel & BATT_CHARGING_SRC_MASK) != 0)
                {
                    Cy_App_Bc_Start(context);
                }
            }
            break;
#endif /* (LEGACY_PD_PARALLEL_OPER) */

#if (!BC_SOURCE_ONLY)
#if (!QC_AFC_SNK_EN)
       case APP_EVT_PD_CONTRACT_NEGOTIATION_COMPLETE:
            if(
                    (pdstack_ctx->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SINK) &&
                    ((chg_cfg->snkSel & BATT_CHARGING_SINK_MASK) != 0u)
              )
            {
                /* Stops battery charging state machine once PD contract has been established. */
                Cy_App_Bc_Stop(context);
            }
            break;
#else
       case APP_EVT_PKT_RCVD:
            if (
                    (pdstack_ctx->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SINK) &&
                    ((chg_cfg->snkSel & BATT_CHARGING_SINK_MASK) != 0u)
               )
            {
                /* PD message received.
                 * Stops BC state machine if not in QC/AFC mode. */
                if(bc_is_qc_afc_charging_active(context)== false)
                {
                    Cy_App_Bc_Stop(context);
                }
            }
            break;
#endif /* (!QC_AFC_SNK_EN) */
#endif /* (!BC_SOURCE_ONLY) */

        default:
            break;
    }
#else
    CY_UNUSED_PARAMETER(context);
    CY_UNUSED_PARAMETER(evt);
#endif /* (defined(CY_IP_MXUSBPD) || defined(CY_IP_M0S8USBPD)) */
}

void Cy_App_Bc_FsmSetEvt(cy_stc_usbpd_context_t *context, uint32_t evt_mask)
{
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    bc_stat->bc_evt |= evt_mask;

#if CY_APP_RTOS_ENABLED
    /* USBPD context is typecasted to PDStack context because Type-A port
     * does not have PDStack context and Cy_App_SendRtosEvent uses only the port
     * variable from the context and it is the same in both PDStack and USBPD context. */
    Cy_App_SendRtosEvent((cy_stc_pdstack_context_t *)context);
#endif /* CY_APP_RTOS_ENABLED */
}

void Cy_App_Bc_FsmClearEvt(cy_stc_usbpd_context_t *context, uint32_t evt_mask)
{
    cy_stc_bc_status_t *bc_stat = &gl_bc_status[context->port];
    bc_stat->bc_evt &= ~evt_mask;
}

#endif /* (BATTERY_CHARGING_ENABLE) */

/* End of file. */
