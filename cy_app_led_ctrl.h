/***************************************************************************//**
* \file cy_app_led_ctrl.h
* \version 1.0
*
* \brief
* Defines data structures and function prototypes
* associated with the application level management of the led control
* module.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_LED_CTRL_H_
#define _CY_APP_LED_CTRL_H_

#if (CY_APP_LED_CONTROL_ENABLE || DOXYGEN)

#include "cy_pdl.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_app_status.h"

/**
* \addtogroup group_pmg_app_common_led
* \{
* The LED controller provides APIs and data structures to control a user LED
* connected to a GPIO. This module uses a software timer to control the LED in
* various modes.
*
* <b>Features:</b>
* * ON/OFF mode
* * Blink mode
* * Breathing mode
*
********************************************************************************
* \section section_pmg_app_common_led_config configuration considerations
********************************************************************************
* The following steps describe the method for enabling the LED controller in an application.
*
* 1. Include cy_app_led_ctrl.h to get access to all functions and other declarations
*    \snippet led_control_sut.c snippet_led_control_include
* 
* 2. Configure the GPIO as output pin using Device Configurator link available 
*    in the ModusToolBox&tm; quick panel.
*
* 3. Define the LED controller context data structure
*    \snippet led_control_sut.c snippet_led_control_structure
*
* 4. Initialize the context structure members
*    \snippet led_control_sut.c snippet_led_control_init_data_struct
*
* 5. Invoke the following API from main to initialize the LED controller
*    \snippet led_control_sut.c snippet_led_control_init
*
* 6. Use the following API to change the LED control mode
*    \snippet led_control_sut.c snippet_led_control_switch_mode
*
* \defgroup group_pmg_app_common_led_macros macros
* \defgroup group_pmg_app_common_led_enums enumerated types
* \defgroup group_pmg_app_common_led_data_structures data structures
* \defgroup group_pmg_app_common_led_functions functions
*/
/** \} group_pmg_app_common_led*/

/**
* \addtogroup group_pmg_app_common_led_macros
* \{
*/
/** Period of the PWM generated on the GPIO for controlling the LED */
#define CY_APP_LED_CTRL_BREATHING_PERIOD          (20u)

/** The duty cycle of PWM generated on the GPIO is changed in steps of 1 ms. This
    macro defines the maximum number of steps used to change the duty cycle 
    from 0 to 100% and vice versa. */
#define CY_APP_LED_CTRL_BREATHING_COUNT           (10u)

/** The time for which the LED stays at peak period i.e., HIGH level. */
#define CY_APP_LED_CTRL_PEAK_PERIOD               (500u)

/** Time (in ms) for which the LED toggles between ON and OFF state in blinking mode */
#define CY_APP_LED_CTRL_BLINKING_PERIOD           (1000)

/** \} group_pmg_app_common_app_macros */

/**
* \addtogroup group_pmg_app_common_led_enums
* \{
*/
/**
 * @typedef cy_en_led_ctrl_mode_t
 * @brief Enum defines the LED control modes
 */
typedef enum
{
    /** LED is OFF */
    CY_APP_LED_CTRL_LED_OFF = 0UL,

    /** LED is ON */
    CY_APP_LED_CTRL_LED_ON,

    /** LED is in blinking mode */
    CY_APP_LED_CTRL_LED_BLINKING,

    /** LED is in breathing mode */
    CY_APP_LED_CTRL_LED_BREATHING
} cy_en_led_ctrl_mode_t;

/**
 * @typedef cy_en_led_ctrl_breathing_state_t
 * @brief Enum defines breathing mode LED states
 */
typedef enum
{
    /** Breathing init state. LED is at zero brightness. */
    CY_APP_LED_CTRL_BREATHING_STATE_INIT = 0,

    /** Breathing rise state. Brightness of LED starts increasing from zero. */
    CY_APP_LED_CTRL_BREATHING_STATE_RISE,

    /** Breathing peak state. The brightness of LED is at its peak. */
    CY_APP_LED_CTRL_BREATHING_STATE_PEAK,

    /** Breathing fall state. The brightness of LED starts decreasing from maximum brightness. */
    CY_APP_LED_CTRL_BREATHING_STATE_FALL
} cy_en_led_ctrl_breathing_state_t;
/** \} group_pmg_app_common_app_macros */

/**
* \addtogroup group_pmg_app_common_led_data_structures
* \{
*/
/**
  @brief This structure holds all variables related to the LED controller
  */
typedef struct cy_stc_led_ctrl_context
{
    /** The pointer to the GPIO's port register base address */
    GPIO_PRT_Type* port;

    /** Timer context used for LED control */
    cy_stc_pdutils_sw_timer_t *timerContext;

    /** Position of the pin bit-field within the port register */
    uint32_t pinNum;

    /** Unique timer ID */
    cy_timer_id_t timerId;

    /** Previous state of the LED control module */
    cy_en_led_ctrl_mode_t prev_state;

    /** Previous state of the LED in blinking mode of LED control module */
    bool blink_state;

    /** Blink rate in milliseconds */
    uint16_t blink_rate;

    /** Previous state of the LED in breathing mode of LED control module */
    bool breath_state;

    /** This parameter is used by the module to understand how the LED is connected in hardware and
     * in turn drives the LED HIGH/LOW
     *
     * | ledOrientation | GPIO output needed to turn ON the LED | GPIO output needed to turn OFF the LED |
     * | -------------- | ------------------------------------- | -------------------------------------- |
     * | true           | HIGH                                  | LOW                                    |
     * | false          | LOW                                   | HIGH                                   |
     */
    bool ledOrientation;

    /** Period of LED in breathing mode of LED control module */
    uint16_t led_period;

    /** Breath count of LED in breathing mode */
    uint16_t breath_count;

    /** Breath value of LED in breathing mode */
    uint16_t breath_value;

    /** Previous breathing state of LED */
    cy_en_led_ctrl_breathing_state_t led_breathing_state;

}cy_stc_led_ctrl_context_t;
/** \} group_pmg_app_common_led_data_structures */

/**
* \addtogroup group_pmg_app_common_led_functions
* \{
*/
/*******************************************************************************
* Function name: Cy_App_LedCtrl_Init
****************************************************************************//**
*
* This function is used to initialize the LED control module. During initialization
* of the module, the LED connected to the GPIO will be turned OFF. The LED control context
* should be initialized before calling this function.
*
* \param ledCtrlCtx
* Pointer to the LED control context
*
* \return
* CY_APP_STAT_BAD_PARAM if a bad parameter is passed as argument
* CY_APP_STAT_SUCCESS otherwise
*
*******************************************************************************/
cy_en_app_status_t Cy_App_LedCtrl_Init(cy_stc_led_ctrl_context_t *ledCtrlCtx);

/*******************************************************************************
* Function name: Cy_App_LedCtrl_SwitchMode
****************************************************************************//**
*
* This function is used to change the mode of LED
*
* \param ledMode
* The desired mode of operation
*
* \param ledCtrlCtx
* Pointer to the LED control context
*
* \return
* CY_APP_STAT_NO_STATE_CHANGE if the current mode of LED operation is same as the new mode requested
* CY_APP_STAT_BAD_PARAM if a bad parameter is passed as argument
* CY_APP_STAT_SUCCESS if the mode change was successful
*
*******************************************************************************/
cy_en_app_status_t Cy_App_LedCtrl_SwitchMode(cy_en_led_ctrl_mode_t ledMode, cy_stc_led_ctrl_context_t *ledCtrlCtx);

/** \} group_pmg_app_common_led_functions */

#endif /* (CY_APP_LED_CONTROL_ENABLE || DOXYGEN) */

#endif /* _CY_APP_LED_CTRL_H_ */

/* [] END OF FILE */