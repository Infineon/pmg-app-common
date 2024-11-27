/***************************************************************************//**
* \file cy_app.h
* \version 2.0
*
* \brief
* Defines data structures and function prototypes associated with
* application level management of the USB-C port
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_H_
#define _CY_APP_H_

/*******************************************************************************
 * Header files including
 ******************************************************************************/
#include <stdint.h>
#include "cy_app_debug.h"
#include "cy_app_config.h"

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
#include "cy_pdaltmode_defines.h"
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */

#include "cy_usbpd_common.h"
#include "cy_usbpd_vbus_ctrl.h"
#include "cy_usbpd_typec.h"
#include "cy_pdstack_dpm.h"

/*****************************************************************************
 * Macros
 *****************************************************************************/
/**
* \addtogroup group_pmg_app_common_app
* \{
* See the \ref section_pmg_app_common_quick_start section
*
* \defgroup group_pmg_app_common_app_macros Macros
* \defgroup group_pmg_app_common_app_enums Enumerated types
* \defgroup group_pmg_app_common_app_data_structures Data structures
* \defgroup group_pmg_app_common_app_functions Functions
* \defgroup group_pmg_app_common_app_features Feature configuration
*/

/**
* \addtogroup group_pmg_app_common_app_macros
* \{
*/

/** PmgAppCommon middleware major version. */
#define CY_APP_COMMON_MW_VERSION_MAJOR               (2)

/** PmgAppCommon middleware minor version. */
#define CY_APP_COMMON_MW_VERSION_MINOR               (0)


/** The minimum wait time (in ms) before retrying Get_Revision PD command. */
#define CY_APP_GET_REV_PD_CMD_RETRY_TIMER_PERIOD     (5u)

/** Maximum number of retries for Get_Revision PD command. */
#define CY_APP_GET_REV_PD_CMD_RETRY_LIMIT            (2u)

/** Delay (in ms) to be used between the cable discovery init commands. */
#define CY_APP_CBL_DISC_TIMER_PERIOD                   (100u)

/** Timer period in milliseconds for providing delay for VCONN gate pull up enable. */
#define CY_APP_VCONN_TURN_ON_DELAY_PERIOD              (1u)

/** CDP state machine timeout period. */
#define CY_APP_BC_CDP_SM_TIMER_PERIOD                     (30000u)

/** VDP_SRC or VDM_SRC minimum turn on time (the minimum from BC 1.2 spec is 40 ms). */
#define CY_APP_BC_VDP_DM_SRC_ON_PERIOD                    (50u)

/** VDM_SRC enable/disable maximum period. */
#define CY_APP_BC_VDMSRC_EN_DIS_PERIOD                    (30u)

/** AFC SNK period between VI_BYTE sending. */
#define CY_APP_BC_AFC_SNK_VI_BYTE_PERIOD                  (80u)

/** T_GLITH_DONE time waiting for the portable device to complete detection.
    This is used by QC/AFC devices to proceed with subsequent detection. */
#define CY_APP_BC_DCP_DETECT_TIMER_PERIOD                 (1100u)

/** Debounce time to verify if the attached device is Apple or not. Apple devices
    create a glitch on DP line whereas BC devices continue to drive DP lower.
    This period is used when Apple and BC 1.2 source protocols are supported
    together. */
#define CY_APP_BC_APPLE_DETECT_TIMER_PERIOD               (5u)

/** Debounce time for identifying state change for DP and DM lines. */
#define CY_APP_BC_DP_DM_DEBOUNCE_TIMER_PERIOD             (40u)

/** AFC detection time. */
#define CY_APP_BC_AFC_DETECT_TIMER_PERIOD                 (100u)

/** TGLITCH_BC_DONE timer period. This timer is used in sink mode to detect QC charger. */
#define CY_APP_BC_GLITCH_BC_DONE_TIMER_PERIOD             (1500)

/** T_GLITCH_DM_HIGH timer period. After DCP opens D+/- short, the sink must wait for this time before requesting VBUS. */
#define CY_APP_BC_GLITCH_DM_HIGH_TIMER_PERIOD             (40)

/** T_V_NEW_REQUEST timer period. After entering QC mode, the sink must wait this much time before requesting next voltage. */
#define CY_APP_BC_V_NEW_REQUEST_TIMER_PERIOD              (200)

/** Delay VBUS_US of HX3 hub for 700 ms. */
#define CY_APP_VBUS_DET_TIMER_PERIOD                   (700u)

/*******************************************************************************
 * VBUS monitor configuration
 ******************************************************************************/
/** Allowed VBUS valid margin as percentage of expected voltage. */
#define CY_APP_VBUS_TURN_ON_MARGIN                         (-20)

/** Allowed VBUS valid margin (as percentage of expected voltage) before detach detection is triggered. */
#define CY_APP_VBUS_TURN_OFF_MARGIN                    (-20)

/** Allowed margin over expected voltage (as percentage) for negative VBUS
 * voltage transitions. */
#define CY_APP_VBUS_DISCHARGE_MARGIN                       (10)

/** Allowed margin over 5 V before the provider FET is turned OFF when
 * discharging to VSAFE0. */
#define CY_APP_VBUS_DISCHARGE_TO_5V_MARGIN                 (10)

/*******************************************************************************
 * Power source (PSOURCE) configuration
 ******************************************************************************/

/** Time (in ms) allowed for source voltage to become valid. */
#define CY_APP_PSOURCE_EN_TIMER_PERIOD                 (250u)

/** Period (in ms) of VBUS validity checks after enabling the power source. */
#define CY_APP_PSOURCE_EN_MONITOR_TIMER_PERIOD         (1u)

/** Time (in ms) between VBUS valid and triggering of PS_RDY. */
#define CY_APP_PSOURCE_EN_HYS_TIMER_PERIOD             (5u)

/** Time (in ms) between VBUS valid and triggering of PS_RDY. */
#define CY_APP_PSOURCE_AVS_EN_HYS_TIMER_PERIOD         (5u)

/** Time (in ms) for which the VBus_Discharge path will be enabled when turning
 * power source OFF. */
#define CY_APP_PSOURCE_DIS_TIMER_PERIOD                (600u)

/** Period (in ms) of VBUS drop to VSAFE0 checks after power source is turned OFF. */
#define CY_APP_PSOURCE_DIS_MONITOR_TIMER_PERIOD        (1u)

/** Period in ms for turning ON VBUS FET. */
#define CY_APP_VBUS_FET_ON_TIMER_PERIOD                (5u)

/** Period in ms for turning OFF VBUS FET. */
#define CY_APP_VBUS_FET_OFF_TIMER_PERIOD               (1u)

/** Time (in ms) for which the VBus_Discharge path will be enabled when turning
 * power source OFF. */
#define CY_APP_PSOURCE_EPR_DIS_TIMER_PERIOD            (1260u)

/** Time (in ms) allowed for source voltage to become valid. */
#define CY_APP_PSOURCE_EPR_EN_TIMER_PERIOD             (860u)

/** Time (in ms) after contract completion the sink will attempt to initiate EPR
 * entry. */
#define CY_APP_EPR_SNK_ENTRY_TIMER_PERIOD              (100u)

/** Minimum OVP detection voltage when ADC is used to implement OVP */
#define CY_APP_ADC_VBUS_MIN_OVP_LEVEL                          (6500u)

/** Number of swap attempts to be made when port partner is sending a WAIT response. */
#define CY_APP_MAX_SWAP_ATTEMPT_COUNT                      (10u)

/*************** Timer duration settings. Changes are not recommended. ********************/

/** Duration of power source current foldback timer (in ms). */
#define CY_APP_PSOURCE_CF_TIMER_PERIOD                     (100u)

/** Duration of extra VBUS discharge after voltage drops below desired level (in ms). */
#define CY_APP_PSOURCE_DIS_EXT_DIS_TIMER_PERIOD            (10u)

/** Maximum time allowed for power sink disable operation (in ms). */
#define CY_APP_PSINK_DIS_TIMER_PERIOD                      (250u)

/** Period (in ms) of VBUS voltage checks performed during power sink disable operation. */
#define CY_APP_PSINK_DIS_MONITOR_TIMER_PERIOD              (1u)

/** Duration (in ms) of discharge sequence on the VBUS_IN supply in CCG3PA/CCG3PA2 designs. */
#define CY_APP_PSINK_DIS_VBUS_IN_DIS_PERIOD                (20u)

/** Period (in ms) of VBUS presence checks after a fault (overvoltage) detection while in a sink contract. */
#define CY_APP_FAULT_RECOVERY_TIMER_PERIOD                 (100u)

/** Time (in ms) for which VBUS will be monitored to ensure removal of VBUS by a faulty power source. */
#define CY_APP_FAULT_RECOVERY_MAX_WAIT                     (500u)

/** Billboard ON delay timer period (in ms). This should be long enough for a host to properly
  recognize the disconnection and reconnection of the USB Billboard device. */
#define CY_APP_BB_ON_TIMER_PERIOD                          (250u)

/** Time delay between successive DR_SWAP attempts (in ms). */
#define CY_APP_INITIATE_DR_SWAP_TIMER_PERIOD               (5u)

/** Time delay between successive PR_SWAP attempts (in ms). PR_SWAP attempts are kept slow to allow
  other sequences such as alternate mode negotiation to complete. */
#define CY_APP_INITIATE_PR_SWAP_TIMER_PERIOD               (450u)

/** The time before the next swap message, after a wait message has been received
  * in response to a swap message. */
#define CY_APP_SWAP_WAIT_TIMER_PERIOD                      (100u)

/** Power source safe FET ON monitor timer period in ms. */
#define CY_APP_PSOURCE_SAFE_FET_ON_MONITOR_TIMER_PERIOD    (1)

/** Reset VDM layer time period. */
#define CY_APP_RESET_VDM_TIMER_PERIOD                      (2u)

/** VCONN swap pending bit in the application-level pending swaps flag. */
#define CY_APP_VCONN_SWAP_PENDING                          (1u)

/** Data role swap pending bit in the application-level pending swaps flag. */
#define CY_APP_DR_SWAP_PENDING                             (2u)

/** Power role swap pending bit in the application-level pending swaps flag. */
#define CY_APP_PR_SWAP_PENDING                             (4u)

/** Wait time (in ms) between the DR_Swap tries. */
#define CY_APP_AUTO_DR_SWAP_TRY_PERIOD                     (10u)

/** Periodicity of checking if TBT mode should be exited (in ms). */
#define CY_APP_TBT_MODE_EXIT_CHECK_PERIOD                      (5u)

/** Delay between VCONN fault and recovery attempt (in ms). */
#define CY_APP_VCONN_RECOVERY_PERIOD                       (500u)

/** tDataReset period (in ms) used by DFP to delay MUX state updates. */
#define CY_APP_DATA_RESET_TIMER_PERIOD                     (205u)

/** tDataResetFail timeout period (in ms) used by the DFP after expiry of tDataReset timer. */
#define CY_APP_DATA_RESET_FAIL_TIMEOUT                     (295u)

/** Time (in ms) to start moisture detection after Type-C attach wait. */
#define CY_APP_MOISTURE_DETECT_TIMER_PERIOD                (70u)

/** CCG activity timer period in ms. */
#define CCG_ACTIVITY_TIMER_PERIOD                          (500u)

/** Minimum specification version of USB PD that supports extended alert
 * messages. */
#define CY_APP_MIN_PD_SPEC_VERSION_FOR_EXTD_ALERT_SUPPORT      (0x31110000UL)

/** \} group_pmg_app_common_app_macros */

/*****************************************************************************
 * Data struct definition
 ****************************************************************************/
/**
* \addtogroup group_pmg_app_common_app_enums
* \{
*/

/**
 * @typedef cy_en_app_port_fault_status_mask_t
 * @brief Fault detection and handling related status bits tracked in the faultStatus field
 */
typedef enum {
    CY_APP_PORT_FAULT_NONE                 = 0x00, /**< System functioning without any fault. */
    CY_APP_PORT_VCONN_FAULT_ACTIVE         = 0x01, /**< Status bit that indicates VCONN fault is active. */
    CY_APP_PORT_SINK_FAULT_ACTIVE          = 0x02, /**< Status bit that indicates sink fault handling is pending. */
    CY_APP_PORT_SRC_FAULT_ACTIVE           = 0x04, /**< Status bit that indicates source fault handling is pending. */
    CY_APP_PORT_VBUS_DROP_WAIT_ACTIVE      = 0x08, /**< Status bit that indicates wait for VBUS drop is pending. */
    CY_APP_PORT_V5V_SUPPLY_LOST            = 0x10, /**< Status bit that indicates that V5V supply (for VCONN) has been lost. */
    CY_APP_PORT_DISABLE_IN_PROGRESS        = 0x80  /**< Port disable operation is in progress. */
} cy_en_app_port_fault_status_mask_t;

/**
 * @typedef cy_en_app_nb_sys_pwr_state_t
 * @brief List of system power states for notebook/desktop designs. These states will be reported by
 * the EC to CCG device through HPI registers.
 */
typedef enum
{
    CY_APP_NB_SYS_PWR_STATE_S0 = 0,            /**< Notebook/desktop is in Active (S0) state. */
    CY_APP_NB_SYS_PWR_STATE_S3,                /**< Notebook/desktop is in Sleep (S3) state. */
    CY_APP_NB_SYS_PWR_STATE_S4,                /**< Notebook/desktop is in Hibernate (S4) state. */
    CY_APP_NB_SYS_PWR_STATE_S5,                /**< Notebook/desktop is in OFF (S5) state. */
    CY_APP_NB_SYS_PWR_STATE_MOD_STBY,          /**< Notebook/desktop is in Modern Standby state. */
    CY_APP_NB_SYS_PWR_STATE_G3                 /**< Notebook/desktop is in G3 state. */
} cy_en_app_nb_sys_pwr_state_t;

/** \} group_pmg_app_common_app_enums */

/**
* \addtogroup group_pmg_app_common_app_data_structures
* \{
*/
/**
  @brief This structure holds the application-specific parameters.
  */
typedef struct app_params
{
    /** VBUS ADC ID. */
    cy_en_usbpd_adc_id_t appVbusPollAdcId;

    /** VBUS ADC input. */
    cy_en_usbpd_adc_input_t appVbusPollAdcInput;

    /** Preferred power role. */
    uint8_t prefPowerRole;

    /** Preferred data role. */
    uint8_t prefDataRole;

    /** Pointer to the discover identity response. */
    cy_pd_pd_do_t *discIdResp;

    /** Length of discover identity response in bytes including the 4 bytes VDM
     * header. */
    uint8_t discIdLen;

    /** Swap response. */
    uint8_t swapResponse;

} cy_stc_app_params_t;

/**
  @brief This structure holds all variables related to the application layer functionality.
  */
typedef struct
{
    cy_pdstack_pwr_ready_cbk_t pwr_ready_cbk;        /**< Registered power source callback. */
    cy_pdstack_sink_discharge_off_cbk_t snk_dis_cbk; /**< Registered power sink callback. */
    app_resp_t appResp;                   /**< Buffer for APP responses */
    uint16_t psrc_volt;                   /**< Current power source voltage in mV. */
    uint16_t psrc_volt_old;               /**< Old power source voltage in mV. */
    uint16_t psnk_volt;                   /**< Current PSink voltage in mV units. */
    uint16_t psnk_cur;                    /**< Current PSink current in 10 mA units. */
    bool is_vbus_on;                      /**< Is supplying VBUS flag. */
    bool is_vconn_on;                     /**< Is supplying VCONN flag. */
    bool psrc_rising;                     /**< Voltage ramp up/down. */
    bool cur_fb_enabled;                  /**< Indicates that current foldback is enabled. */
    bool ld_sw_ctrl;                      /**< Indicates whether the VBUS load switch control is active or not. */
    bool bc_12_src_disabled;              /**< BC 1.2 source disabled flag. */
    bool bc_12_snk_disabled;              /**< BC 1.2 sink disabled flag. */
    bool bc_snk_disabled;                 /**< Legacy battery charging sink disabled flag. */
    uint8_t       app_pending_swaps;      /**< Variable denoting the types of swap operation that are pending. */
    uint8_t       actv_swap_type;         /**< Denotes the active swap operation. */
    uint16_t      actv_swap_delay;        /**< Delay to be applied between repeated swap attempts. */
    uint8_t       actv_swap_count;        /**< Denotes number of active swap attempts completed. */
    uint8_t       turn_on_temp_limit;     /**< Temperature threshold to turn on internal FET after turn off condition. */
    uint8_t       turn_off_temp_limit;    /**< Temperature threshold to turn off internal FET. */
    bool          is_hot_shutdown;        /**< Indicates that hot shutdown detected. */
    bool          debug_acc_attached;     /**< Debug accessory attach status. */
    bool          apu_reset_pending;      /**< Flag to indicate that APU reset is in progress. */
    uint32_t      pd_revision;         /**< PD spec revision of the port partner. */
    uint16_t user_custom_pid;               /**< Custom PID value defined for the port by the EC. */
    bool user_custom_pid_valid;             /**< Custom PID value is valid or not. */
    cy_pd_pd_do_t source_info;              /**< Source info response to be sent by the device. */
    cy_pd_pd_do_t spec_revision;            /**< PD spec revision response to be sent by the device. */
} cy_stc_app_status_t;

/**
   @brief This structure hold all solution level functions used as a part of application layer.
 */
typedef struct
{
    /** Checks if Type-A port is connected or not. */
    bool (*type_a_is_idle)(void);

    /** Updates the Type-A port connection status. */
    void (*type_a_update_status)(bool is_connect, bool detach_det);

    /** Sets the voltage on Type-A port. */
    void (*type_a_set_voltage)(uint16_t volt_mV);

    /** Enables or disables VBUS on Type-A port. */
    void (*type_a_enable_disable_vbus)(bool on_off);

    /** Enables or disables Type-A port. */
    void (*type_a_port_enable_disable)(bool en_dis);

    /** Detects Type-A port disconnect. */
    void (*type_a_detect_disconnect)(void);

}cy_app_sln_cbk_t;

/** \} group_pmg_app_common_app_data_structures */

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/
/**
* \addtogroup group_pmg_app_common_app_functions
* \{
*/

/**
 * @brief Application-level initialization function
 *
 * This function performs application-level initialization required for the PMG
 * solution. This should be called before calling the Cy_PdStack_Dpm_Init
 * function of PDStack middleware.
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @param appParams Pointer to the application-specific parameters.
 *
 * @return None.
 *
 */
void Cy_App_Init(cy_stc_pdstack_context_t *ptrPdStackContext, const cy_stc_app_params_t *appParams);

/**
 * @brief Handler for application-level asynchronous tasks.
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @return 1 in case of success, 0 in case of task handling error.
 */
uint8_t Cy_App_Task(cy_stc_pdstack_context_t *ptrPdStackContext);

/**
 * @brief Handler for events notified from the PDStack middleware.
 * @param ptrPdStackContext Pointer to the PDStack context on which events are to be handled.
 * @param evt Type of event to be handled.
 * @param dat Data associated with the event.
 * @return None.
 */
void Cy_App_EventHandler(cy_stc_pdstack_context_t *ptrPdStackContext, 
        cy_en_pdstack_app_evt_t evt, const void* dat);

/**
 * @brief Function to return pointer PD command response buffer.
 * @param port PD port corresponding to the command and response.
 * @return Pointer to the response buffer.
 */
app_resp_t* Cy_App_GetRespBuffer(uint8_t port);

/**
 * @brief Function to return pointer to the application status structure
 * consisting of all variables related to application layer functionality.
 * @param port PD port to be queried.
 * @return Pointer to the application status information structure.
 */
cy_stc_app_status_t* Cy_App_GetStatus(uint8_t port);

/**
 * @brief Function to return pointer to the structure holding common altmode
 * and application layer information.
 * @param port PD port to be queried.
 * @return Pointer to the altmode and application-related information structure.
 */
cy_stc_pdstack_app_status_t* Cy_App_GetPdAppStatus(uint8_t port);

/**
 * @brief Check whether the application layer is ready to allow device to enter into
 * Deep Sleep mode. This API also configures the various modules in the application
 * layer to go into Deep Sleep mode if entry into Deep Sleep is possible.
 * @return True if Deep Sleep entry is possible; false otherwise.
 */
bool Cy_App_Sleep (void);

/**
 * @brief Restores the application layer state after PMG device wakes from Deep Sleep mode.
 * @return None.
 */
void Cy_App_Resume (void);

/**
 * @brief Function to place PMG device in power saving mode if possible.
 *
 * This function places the PMG device in power saving Deep Sleep mode
 * if possible. The function checks for each interface (PD, HPI, etc.)
 * being idle and then enters into Sleep mode with the appropriate wakeup
 * triggers. If the device enters Sleep mode, the function will only
 * return after the device has woken up.
 *
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @param ptrPdStack1Context Pointer to the PDStack context of port 1.
 * @return True if the device went into Sleep; false otherwise.
 */
bool Cy_App_SystemSleep(cy_stc_pdstack_context_t *ptrPdStackContext, cy_stc_pdstack_context_t *ptrPdStack1Context);

/*****************************************************************************
  Functions related to power
 *****************************************************************************/

/**
 * @brief This function enables VCONN power on the given CC channel.
 *
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @param channel Selected CC line.
 *
 * @return True if VCONN is turned ON; false otherwise.
 */

bool Cy_App_VconnEnable(cy_stc_pdstack_context_t *ptrPdStackContext, uint8_t channel);

/**
 * @brief This function disables VCONN power on the given CC channel.
 *
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @param channel Selected CC line.
 *
 * @return None.
 */
void Cy_App_VconnDisable(cy_stc_pdstack_context_t *ptrPdStackContext, uint8_t channel);

/**
 * @brief This function checks if power is present on VCONN.
 *
 * @param ptrPdStackContext Pointer to the PDStack context.
 *
 * @return True if power is present on VCONN; false otherwise.
 */
bool Cy_App_VconnIsPresent(cy_stc_pdstack_context_t *ptrPdStackContext);

/**
 * @brief This function checks if power is present on VBUS.
 *
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @param volt Voltage in mV units.
 * @param per  Threshold margin.
 *
 * @return True if power is present on VBUS; else, otherwise false.
 */
bool Cy_App_VbusIsPresent(cy_stc_pdstack_context_t *ptrPdStackContext, uint16_t volt, int8_t per);

/**
 * @brief This function returns current VBUS voltage in mV.
 *
 * @param ptrPdStackContext Pointer to the PDStack context.
 *
 * @return VBUS voltage in mV.
 */
uint16_t Cy_App_VbusGetValue(cy_stc_pdstack_context_t *ptrPdStackContext);

/**
 * @brief This function turns on discharge FET on selected port.
 *
 * @param context Pointer to the PDStack context.
 *
 * @return None.
 */
void Cy_App_VbusDischargeOn(cy_stc_pdstack_context_t* context);

/**
 * @brief This function turns off discharge FET on selected port.
 *
 * @param context Pointer to the PDStack context.
 *
 * @return None.
 */
void Cy_App_VbusDischargeOff(cy_stc_pdstack_context_t* context);

/**
 * @brief Restarts alternate mode layer.
 *
 * @param ptrPdStackcontext Pointer to the PDStack context.
 *
 * @return True if the alternate mode is reset; false otherwise.
 */
bool Cy_App_VdmLayerReset(cy_stc_pdstack_context_t * ptrPdStackcontext);

/**
 * @brief Function to handle VCONN supply state change due to OCP condition or
 * V5V supply change.
 *
 * @param ptrPdStackcontext Pointer to the PDStack context.
 * @param vconn_on Whether VCONN has just turned ON or OFF.
 *
 * @return None.
 */
void Cy_App_VconnChangeHandler(cy_stc_pdstack_context_t * ptrPdStackcontext, bool vconn_on);

/**
 * @brief This function is called at the end of a PD contract to check whether any
 * role swaps need to be triggered.
 *
 * @param ptrPdStackContext Pointer to the PDStack context.
 *
 * @return Void.
 */
void Cy_App_ContractHandler (cy_stc_pdstack_context_t *ptrPdStackContext);

/**
 * @brief This function is called whenever there is a change in the connection.
 *
 * @param ptrPdStackContext Pointer to the PDStack context.
 * 
 * @return Void.
 */
void Cy_App_ConnectChangeHandler (cy_stc_pdstack_context_t *ptrPdStackContext);

/**
 * @brief Callback function for cable discovery process.
 * @param id Timer ID responsible for the callback.
 * @param callbackContext Callback context.
 * @return None.
 * */
void Cy_App_CableDiscTimerCallback (cy_timer_id_t id, void *callbackContext);

/**
 * @brief Evaluates the EPR mode request. The application needs to invoke
 * app_resp_handler with the evaluation response.
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @param eprModeState EPR mode.
 * @param app_resp_handler Application response handler callback.
 * @return True if EPR evaluation is successful.
 * */
bool Cy_App_EvalEprMode(cy_stc_pdstack_context_t *ptrPdStackContext, cy_en_pdstack_eprmdo_action_t eprModeState, cy_pdstack_app_resp_cbk_t app_resp_handler);

/**
 * @brief Callback function invoked by the PDStack middleware to allow
 * updating the EPR source capabilities before they are sent by the PDStack
 * middleware.
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @param app_resp_handler Application response handler callback.
 * @return True if EPR capabilities can be sent.
 * */
bool Cy_App_SendEprCap(cy_stc_pdstack_context_t *ptrPdStackContext, cy_pdstack_app_resp_cbk_t app_resp_handler);

/**
 * @brief Callback function to decide whether to send source info message,
 * PDStack middleware sends src_info message if this function returns true
 * otherwise sends reject message.
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @return True if source info can be sent.
 */
bool Cy_App_SendSrcInfo (struct cy_stc_pdstack_context *ptrPdStackContext);

/**
 * @brief This function validates the configuration table offsets and size.
 * @param ptrUsbPdPortCtx USB PD port context.
 * @return True if the table offset and size are valid; false otherwise.
 * */
bool Cy_App_ValidateCfgTableOffsets(cy_stc_usbpd_context_t *ptrUsbPdPortCtx);

/**
 * @brief Callback function for notifying supply change event.
 * @param context Callback context.
 * @param supply_id Supply identifier.
 * @param present True if the supply is present.
 * @return None.
 * */
void Cy_App_SupplyChangeCallback(void* context, cy_en_usbpd_supply_t supply_id, bool present);

/**
 * @brief Function disable the PD port.
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @param cbk Port disabled callback handler.
 * @return Port disabled success or failure status.
 * */
cy_en_pdstack_status_t Cy_App_DisablePdPort(cy_stc_pdstack_context_t *ptrPdStackContext,
                                            cy_pdstack_dpm_typec_cmd_cbk_t cbk);

/**
 * @brief Function waits for a RTOS event.
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @param waitTime Event wait timeout in ms.
 * @return True if the event received success, otherwise false.
 * */
bool Cy_App_GetRtosEvent(cy_stc_pdstack_context_t *ptrPdStackContext, uint32_t waitTime);

/**
 * @brief Function sends a RTOS event.
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @return Void.
 * */
void Cy_App_SendRtosEvent(cy_stc_pdstack_context_t *ptrPdStackContext);

/**
 * @brief Function registers callback structure of solution level functions.
 * @param ptrPdStackcontext PD stack context.
 * @param callback pointer to solution callback structure.
 * @return None.
 */
void Cy_App_RegisterSlnCallback(cy_stc_pdstack_context_t *ptrPdStackcontext, cy_app_sln_cbk_t *callback);

/*****************************************************************************
  Functions to be provided at the solution level
 *****************************************************************************/
/**
 * @brief Solution handler for PD events reported from the stack.
 *
 * The function provides all PD events to the solution. For a solution
 * supporting HPI, the solution function should redirect the calls to
 * hpi_pd_event_handler.
 *
 * @param ptrPdStackContext Pointer to the PDStack context.
 * @param evt Event that is being notified.
 * @param data Data associated with the event. This is an opaque pointer that
 * needs to be de-referenced based on event type.
 *
 * @return None.
 */
void sln_pd_event_handler(cy_stc_pdstack_context_t *ptrPdStackContext, 
        cy_en_pdstack_app_evt_t evt, const void *data);

/** \} group_pmg_app_common_app_functions */
/** \} group_pmg_app_common_app */

#endif /* _CY_APP_H_ */

/* [] END OF FILE */
