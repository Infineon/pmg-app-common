/***************************************************************************//**
* \file cy_app_battery_charging.h
* \version 2.0
*
* \brief
* Defines data structures and function prototypes associated with
* battery charging
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_BATTERY_CHARGING_H_
#define _CY_APP_BATTERY_CHARGING_H_

/*******************************************************************************
 * Header files including
 ******************************************************************************/

#include "cy_usbpd_common.h"
#include "cy_usbpd_bch.h"
#include "cy_pdstack_common.h"
#include "cy_pdutils_sw_timer.h"

/**
* \addtogroup group_pmg_app_common_bch
* \{
*
* The Battery Charging module implements the charger detection algorithms
* defined in the **Battery Charging Specification**.
* The module supports BC 1.2, QC, AFC and Apple charging. On a Type-C attach,
* both PD and battery charging state machines are run. If a PD message is received,
* then battery charging is disabled.
*
* <b>Source Operation:</b>
* * Only Apple charging is enabled, applies voltage on D+ and D- based on
* Apple brick ID and stays in Apple charging state.
* * Only BC 1.2 is enabled, applies DCP terminations and looks for BC 1.2 device,
* if BC 1.2 device is detected, looks for QC/AFC device.
* * Both Apple charging and BC 1.2 are enabled, starts the battery charging state
* in Apple charging mode. The D+ line is monitored for any voltage change.
* When a BC 1.2 based sink gets attached, it applies VDP_SRC to the D+ line causing
* the D+ voltage to drop below 2.2V. If this voltage drop is detected, removes
* Apple terminations and starts BC 1.2 detection.
*
* <b>Sink Operation:</b>
* * Only Apple charging is enabled, checks the voltages on D+/D- pin and detects
* the Apple brick ID.
* * Only BC 1.2 is enabled, starts primary charger detection. If DCP/CDP is detected,
* starts secondary charger detection.
* * Both Apple charging and BC 1.2 is enabled, starts with BC1.2 detection,
* if DCP is connected, starts QC/AFC charger detection. If QC/AFC charger is detected,
* requests for highest Vbus voltage and starts battery charging. If DCP/CDP is not
* detected in BC 1.2 primary detection state, starts Apple charger detection.
*
* The module provides a set of APIs through which the application can initialize,
* monitor, and configure the battery charging operation.
*
********************************************************************************
* \section section_pmg_app_common_bch_config Configuration considerations
********************************************************************************
*
* These steps describe the simplest way of enabling the Battery Charging module
* in the application.
*
* 1. Include cy_app_battery_charging.h to get access to all the functions and
* other declarations.
*    \snippet snippet/bch_snippet.c snippet_configuration_include
*
* 2. Configure the battery charging parameters using the Device Configurator
* available in the ModusToolbox&tm; quick panel.
*    \image html legacy_charging_config.png width=95%
*
* 3. Initialize the Battery Charging module.
*    \snippet snippet/bch_snippet.c snippet_bch_init
*
* 4. Start the Battery Charging operation on Type-C attach event.
*    \snippet snippet/bch_snippet.c snippet_bch_start
*
* 5. Invoke the Cy_App_Bc_Task function from the main processing loop of the
* application to handle the Battery Charging tasks.
*    \snippet snippet/bch_snippet.c snippet_bch_task
********************************************************************************
*
* \defgroup group_pmg_app_common_bch_macros Macros
* \defgroup group_pmg_app_common_bch_enums Enumerated Types
* \defgroup group_pmg_app_common_bch_data_structures Data Structures
* \defgroup group_pmg_app_common_bch_functions Functions
*/

/*******************************************************************************
 * MACRO definition
 ******************************************************************************/

/** @cond DOXYGEN_HIDE */

/* Combined mask for all legacy battery charging protocols in source mode. */
#define BATT_CHARGING_SRC_MASK  (BC_SRC_APPLE_MODE_ENABLE_MASK | \
        BC_SRC_1_2_MODE_ENABLE_MASK | BC_SRC_QC_MODE_ENABLE_MASK | \
        BC_SRC_AFC_MODE_ENABLE_MASK)
        
/* Combined mask for all legacy battery charging protocols in BCR2 sink mode. */
#define BATT_CHARGING_SINK_MASK  (BC_SINK_APPLE_MODE_ENABLE_MASK | \
        BC_SINK_1_2_MODE_ENABLE_MASK | BC_SINK_QC_MODE_ENABLE_MASK | \
        BC_SINK_AFC_MODE_ENABLE_MASK)

/* QC DP/DM macros.*/
#define QC_MODE_12V                                 ((((uint32_t)BC_D_0_6V) << 8) | (uint32_t)BC_D_0_6V)
#define QC_MODE_9V                                  ((((uint32_t)BC_D_0_6V) << 8) | (uint32_t)BC_D_3_3V)
#define QC_MODE_CONT                                ((((uint32_t)BC_D_3_3V) << 8) | (uint32_t)BC_D_0_6V)
#define QC_MODE_20V                                 ((((uint32_t)BC_D_3_3V) << 8) | (uint32_t)BC_D_3_3V)
#define QC_MODE_5V                                  ((((uint32_t)BC_D_GND) << 8) | (uint32_t)BC_D_0_6V)
#define QC_MODE_RSVD                                ((((uint32_t)BC_D_ERR) << 8) | (uint32_t)BC_D_ERR)

/** @endcond */

/**
* \addtogroup group_pmg_app_common_bch_macros
* \{
*/

#define AFC_DETECT_RETRY_COUNT          (6)     /**< AFC detect retry count. */
#define AFC_WAIT_DM_RETRY_COUNT         (10)    /**< AFC wait retry count. */

#ifndef BC_QC_AFC_SNK_MAX_VOLT
#define BC_QC_AFC_SNK_MAX_VOLT          (CY_PD_VSAFE_15V)  /**< BC QC AFC sink maximum voltage. */
#endif /* BC_QC_AFC_SNK_MAX_VOLT */
#ifndef BC_QC_AFC_SNK_MIN_VOLT
#define BC_QC_AFC_SNK_MIN_VOLT          (CY_PD_VSAFE_5V)  /**< BC QC AFC sink minimum voltage. */
#endif /* BC_QC_AFC_SNK_MIN_VOLT */
#ifndef BC_QC_AFC_SNK_MAX_CUR
#define BC_QC_AFC_SNK_MAX_CUR           (CY_PD_I_3A)  /**< BC QC AFC sink maximum current in 1 mA units. */
#endif /* BC_QC_AFC_SNK_MAX_CUR */
#ifndef BC_QC_AFC_SNK_MIN_CUR
#define BC_QC_AFC_SNK_MIN_CUR           (CY_PD_I_1P5A)  /**< BC QC AFC sink maximum current in 1 mA units. */
#endif /* BC_QC_AFC_SNK_MIN_CUR */

#define QC2_MAX_CURRENT                 (150u)  /**< Max allowed current per QC 2.0 protocol (1.5 A In 10 mA steps). */

/** \} group_pmg_app_common_bch_macros */

/*******************************************************************************
 * Data structure definition
 ******************************************************************************/

/**
 * \addtogroup group_pmg_app_common_bch_enums
 * \{
 * */

/**
 * @typedef cy_en_bc_charge_mode_t
 * @brief List of legacy battery charging schemes supported over a Type-C port.
 */
typedef enum{
    BC_CHARGE_NONE = 0,         /**< No active battery charging modes. */
    BC_CHARGE_DCP,              /**< Dedicated charging port as defined by BC 1.2 specification. */
    BC_CHARGE_QC2,              /**< QC 2.0 charger. */
    BC_CHARGE_QC3,              /**< QC 3.0 charger. */
    BC_CHARGE_AFC,              /**< Adaptive Fast Charging mode. */
    BC_CHARGE_APPLE,            /**< Apple power brick. */
    BC_CHARGE_CDP               /**< Charging Downstream Port as defined by BC 1.2 specification. */
} cy_en_bc_charge_mode_t;


/**
 * @typedef cy_en_bc_d_status_t
 * @brief Enumeration of the various DP/DM states.
 */
typedef enum
{
    BC_D_GND = 0,                       /**< DP/DM voltage < 0.325. */
    BC_D_0_6V,                          /**< 0.325 < DP/DPM voltage < 2 V.  */
    BC_D_3_3V,                          /**< DP/DM voltage > 2 V. */
    BC_D_ERR                            /**< Error state. */
} cy_en_bc_d_status_t;

/**
 * @typedef cy_en_bc_apple_term_t
 * @brief Enumeration of Apple terminations codes.
 */
typedef enum
{
    APPLE_TERM1 = 1,                    /**< Termination 1 code: 1 V - 2.22 V. */
    APPLE_TERM2 = 2,                    /**< Termination 2 code: 2.22 V - 2.89 V. */
    APPLE_TERM3 = 3                     /**< Termination 3 code: 2.89+ V. */
} cy_en_bc_apple_term_t;

/**
 * @typedef cy_en_bc_apple_brick_id_t
 * @brief List of possible Apple Brick IDs based on the terminations on DP and DM pins.
 */
typedef enum
{
    APPLE_BRICK_ID_0 = 0x11,            /**< APPLE_TERM1 on DM, APPLE_TERM1 on DP. */
    APPLE_BRICK_ID_1 = 0x12,            /**< APPLE_TERM1 on DM, APPLE_TERM2 on DP. */
    APPLE_BRICK_ID_2 = 0x13,            /**< APPLE_TERM1 on DM, APPLE_TERM3 on DP. */
    APPLE_BRICK_ID_3 = 0x21,            /**< APPLE_TERM2 on DM, APPLE_TERM1 on DP. */
    APPLE_BRICK_ID_4 = 0x22,            /**< APPLE_TERM2 on DM, APPLE_TERM2 on DP. */
    APPLE_BRICK_ID_5 = 0x23,            /**< APPLE_TERM2 on DM, APPLE_TERM3 on DP. */
    APPLE_BRICK_ID_6 = 0x31,            /**< APPLE_TERM3 on DM, APPLE_TERM1 on DP. */
    APPLE_BRICK_ID_7 = 0x32,            /**< APPLE_TERM3 on DM, APPLE_TERM2 on DP. */
    APPLE_BRICK_ID_8 = 0x33,            /**< APPLE_TERM3 on DM, APPLE_TERM3 on DP. */
} cy_en_bc_apple_brick_id_t;

/**
 * @typedef cy_en_bc_fsm_evt_t
 * @brief List of BC events notified by the PDL.
 */
typedef enum
{
    BC_FSM_EVT_ENTRY = 0,                   /**<  0: BC Event: State entry. */
    BC_FSM_EVT_CMP1_FIRE,                   /**<  1: BC Event: CMP1 interrupt. */
    BC_FSM_EVT_CMP2_FIRE,                   /**<  2: BC Event: CMP2 interrupt. */
    BC_FSM_EVT_QC_CHANGE,                   /**<  3: BC Event: QC state change. */
    BC_FSM_EVT_QC_CONT,                     /**<  4: BC Event: QC continuous mode entry. */
    BC_FSM_EVT_AFC_RESET_RCVD,              /**<  5: BC Event: AFC reset received. */
    BC_FSM_EVT_AFC_MSG_RCVD,                /**<  6: BC Event: AFC message received. */
    BC_FSM_EVT_AFC_MSG_SENT,                /**<  7: BC Event: AFC message sent. */
    BC_FSM_EVT_AFC_MSG_SEND_FAIL,           /**<  8: BC Event: AFC message sending failed. */
    BC_FSM_EVT_TIMEOUT1,                    /**<  9: BC Event: Timer1 expiry interrupt. */
    BC_FSM_EVT_TIMEOUT2,                    /**< 10: BC Event: Timer2 expiry interrupt. */
    BC_FSM_EVT_DISCONNECT,                  /**< 11: BC Event: Device disconnect. */
    BC_FSM_MAX_EVTS                         /**< 12: Number of events. */
} cy_en_bc_fsm_evt_t;

/**
 * @typedef cy_en_bc_state_t
 * @brief List of states in the legacy battery charging state machine.
 */
typedef enum{
    BC_FSM_OFF = 0,                                     /**< BC state machine inactive. */
    BC_FSM_SRC_LOOK_FOR_CONNECT,                        /**< Look for connection as a DCP. */
    BC_FSM_SRC_INITIAL_CONNECT,                         /**< Initial BC 1.2 sink connection detected. */
    BC_FSM_SRC_APPLE_CONNECTED,                         /**< Connected to sink as an Apple power brick. */
    BC_FSM_SRC_CDP_CONNECTED,                           /**< Port configured as a Charging Downstream Port (CDP). */
    BC_FSM_SRC_OTHERS_CONNECTED,                        /**< DCP failed Apple sink detection. */
    BC_FSM_SRC_QC_OR_AFC,                               /**< Port connected to a QC or AFC sink. */
    BC_FSM_SRC_QC_CONNECTED,                            /**< Connected to sink as a QC HVDCP. */
    BC_FSM_SRC_AFC_CONNECTED,                           /**< Connected to sink as an AFC charger. */
    BC_FSM_SINK_START,                                  /**< BC sink state machine start state. */
    BC_FSM_SINK_APPLE_CHARGER_DETECT,                   /**< Sink looking for an Apple charger. */
    BC_FSM_SINK_APPLE_BRICK_ID_DETECT,                  /**< Sink identified Apple charger, identifying brick ID. */
    BC_FSM_SINK_PRIMARY_CHARGER_DETECT,                 /**< BC 1.2 primary charger detect state. */
    BC_FSM_SINK_TYPE_C_ONLY_SOURCE_CONNECTED,           /**< BC 1.2 src detection failed, connected as Type-C sink. */
    BC_FSM_SINK_SECONDARY_CHARGER_DETECT,               /**< BC 1.2 secondary charger detect state. */
    BC_FSM_SINK_DCP_CONNECTED,                          /**< Sink connected to a BC 1.2 DCP. */
    BC_FSM_SINK_SDP_CONNECTED,                          /**< Sink connected to a Standard Downstream Port (SDP). */
    BC_FSM_SINK_CDP_CONNECTED,                          /**< Sink connected to a BC 1.2 CDP. */
    BC_FSM_SINK_AFC_CHARGER_DETECT,                     /**< AFC charger detect state. */
    BC_FSM_SINK_QC_CHARGER_DETECTED,                    /**< QC 2.0 charger detect state. */
    BC_FSM_MAX_STATES,                                  /**< Invalid state ID. */
} cy_en_bc_state_t;

/**
 * @typedef cy_en_bc_sink_timer_t
 * @brief List of soft timers used by the battery charging state machine.
 */
typedef enum
{
    BC_SINK_TIMER_NONE = 0,                             /**< No timers running. */
    BC_SINK_DCD_DEBOUNCE_TIMER = 1,                      /**< DCD Debounce timer. */
    BC_SINK_TIMER_BC_DONE = 2,                          /**< T_GLITCH_BC_DONE timer. */
    BC_SINK_TIMER_DM_HIGH = 3
} cy_en_bc_sink_timer_t;

/** \} group_pmg_app_common_bch_enums */

/**
 * \addtogroup group_pmg_app_common_bch_data_structures
 * \{
 * */

/*******************************************************************************
 * Data structure definition
 ******************************************************************************/

/**
 * @brief Union to hold DP/DM status.
 */
typedef union
{
    uint16_t state;                                     /**< Combined status of DP and DM. */
    uint8_t  d[2];                                      /**< Individual status of Dp(d[0]) and Dm(d[1]). */
} cy_stc_bc_dp_dm_state_t;

/**
 * @brief Struct to define battery charger status.
 */
typedef struct {
    cy_stc_pdutils_sw_timer_t *ptr_timer_ctx;   /**< Pointer to software timer context. */
    cy_en_bc_charge_mode_t cur_mode;            /**< Active charging scheme. */
    cy_en_bc_state_t bc_fsm_state;              /**< Current state of the BC state machine. */
    uint32_t bc_evt;                            /**< Bitmap representing event notifications to the state machine. */
    uint16_t cur_volt;                          /**< Active VBUS voltage in mV units. */
    uint16_t cur_amp;                           /**< Active supply current in 10 mA units. */
    uint16_t max_volt;                          /**< Maximum Sink VBUS voltage in mV units. */
    uint16_t min_volt;                          /**< Minimum Sink VBUS voltage in mV units. */
    uint16_t max_amp;                           /**< Maximum Sink current in 10 mA units. */
    uint16_t min_amp;                           /**< Minimum Sink current in 10 mA units. */
    bool volatile connected;                    /**< Whether BC connection is detected. */
    bool volatile mismatch;                     /**< Whether BC voltage/current mismatch is detected. */

    bool volatile comp_rising;                  /**< Whether comparator is looking for a rising edge or falling edge. */
    cy_stc_bc_dp_dm_state_t dp_dm_status;       /**< Debounced status of the DP/DM pins. */
    cy_stc_bc_dp_dm_state_t old_dp_dm_status;   /**< Previous status of the DP/DM pins. */
    cy_en_bc_sink_timer_t       cur_timer;      /**< Identifies the timer that is running. */

    uint16_t requested_qc_volt;                 /**< Recently requested voltage from QC source in mV units. */

    uint8_t afc_src_msg_count;                  /**< Number of successful AFC message transfers. Three transfers have
                                                     to be successful before making any VI changes. */
    bool volatile afc_src_is_matched;           /**< Status of VI match from the last received AFC byte. */
    uint8_t afc_src_cur_match_byte;             /**< The currently matched AFC VI value. */
    uint8_t afc_src_last_match_byte;            /**< Previously matched AFC VI value. */
    uint8_t afc_src_matched_byte;               /**< Holds the VI byte that is matched in two out of the last three
                                                     messages. */
    uint8_t afc_src_match_count;                /**< Number of AFC VI byte matches detected. */
    uint8_t afc_tx_active;                      /**< Whether AFC message transmission is active. */

    uint8_t afc_snk_cur_vi_byte;                /**< AFC VI value recently sent by Sink. */
    uint8_t afc_retry_count;                    /**< Sink Tx retries counter. */

    bool attach;                                /**< Whether charger attach has been detected. */
} cy_stc_bc_status_t;

/** \} group_pmg_app_common_bch_data_structures */

/*******************************************************************************
 * Global function declaration
 ******************************************************************************/

/**
* \addtogroup group_pmg_app_common_bch_functions
* \{
*/

/**
 * @brief This function initializes the battery charging block. This should be
 * called one time only at system startup for each port on which the charger detect
 * state machine needs to run.
 *
 * @param context Pointer to USBPD context.
 * @param timerCtx Pointer to software timer context.
 * @return cy_en_usbpd_status_t
 */
cy_en_usbpd_status_t Cy_App_Bc_Init(cy_stc_usbpd_context_t *context, cy_stc_pdutils_sw_timer_t *timerCtx);

/**
 * @brief This function starts the battery charging block operation after Type-C
 * connection has been detected.
 *
 * @param context Pointer to USBPD context.
 * @return cy_en_usbpd_status_t
 */
cy_en_usbpd_status_t Cy_App_Bc_Start(cy_stc_usbpd_context_t *context);

/**
 * @brief This function stops the battery charging block operation once Type-C
 * disconnect has been detected.
 *
 * @param context Pointer to USBPD context.
 * @return cy_en_usbpd_status_t
 */
cy_en_usbpd_status_t Cy_App_Bc_Stop(cy_stc_usbpd_context_t *context);

/**
 * @brief This function returns whether the battery charging module is active or not.
 *
 * @param context Pointer to USBPD context.
 * @return True if the charger detect module is running, false otherwise.
 */
bool Cy_App_Bc_IsActive(cy_stc_usbpd_context_t *context);

/**
 * @brief This function performs actions associated with the battery charging state
 * machine and is expected to be called from the application main.
 *
 * @param context Pointer to USBPD context.
 * @return cy_en_usbpd_status_t
 */
cy_en_usbpd_status_t Cy_App_Bc_Task(cy_stc_usbpd_context_t *context);

/**
 * @brief This function prepares the battery charging block for device entry into deep sleep
 * state.
 *
 * @param context Pointer to USBPD context.
 * @return Returns true if the deep sleep mode can be entered, false otherwise.
 */
bool Cy_App_Bc_PrepareDeepSleep(cy_stc_usbpd_context_t *context);

/**
 * @brief This function restores the battery charging block into functional state after the
 * PMG1 device wakes from deep sleep mode.
 *
 * @param context Pointer to USBPD context.
 * @return Void.
 */
void Cy_App_Bc_Resume(cy_stc_usbpd_context_t *context);

/**
 * @brief This function retrieves the current status of the battery charging state machine.
 *
 * @param context Pointer to USBPD context.
 * @return Pointer to charger detect status. The structure must not be modified by the caller.
 */
const cy_stc_bc_status_t* Cy_App_Bc_GetStatus(cy_stc_usbpd_context_t *context);

/**
 * @brief This function updates the battery charging state machine based on event notifications
 * from the USB PDStack. This event handler calls the Cy_App_Bc_Start and Cy_App_Bc_Stop functions
 * as required.
 *
 * @param context Pointer to USBPD context.
 * @param evt USB PD event ID.
 * @param dat Optional data associated with the event.
 * @return Void.
 */
void Cy_App_Bc_PdEventHandler(cy_stc_usbpd_context_t *context, cy_en_pdstack_app_evt_t evt, const void* dat);

/**
 * @brief This function sets an event status for the battery charging state machine to process.
 *
 * @param context Pointer to USBPD context.
 * @param evt_mask Mask specifying events to be set.
 *
 * @return Void.
 */
void Cy_App_Bc_FsmSetEvt(cy_stc_usbpd_context_t *context, uint32_t evt_mask);

/**
 * @brief This function clears one or more events after the battery charging state machine has
 * dealt with them.
 *
 * @param context Pointer to USBPD context.
 * @param evt_mask Event Mask to be cleared.
 * @return Void.
 */
void Cy_App_Bc_FsmClearEvt(cy_stc_usbpd_context_t *context, uint32_t evt_mask);

/** \} group_pmg_app_common_bch_functions */
/** \} group_pmg_app_common_bch */

#endif /* _CY_APP_BATTERY_CHARGING_H_ */

/* End of File */
