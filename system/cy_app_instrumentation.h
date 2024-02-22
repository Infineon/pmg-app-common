/***************************************************************************//**
* \file cy_app_instrumentation.h
* \version 1.0
*
* \brief
* Defines data structures and function prototypes to monitor
* CPU resource (execution time and stack) usage
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_INSTRUMENTATION_H_
#define _CY_APP_INSTRUMENTATION_H_

#include <stdint.h>
#include "cy_pdutils_sw_timer.h"

/**
* \addtogroup group_pmg_app_common_system_enums
* \{
* \defgroup group_pmg_app_common_system_instrumentation_enums instrumentation
* \{
*/
/**
 * @brief Enumeration of all instrumentation fault events
 */
typedef enum instrumentation_events
{
    CY_APP_INST_EVT_WDT_RESET = 0,                 /**< 0x00: Instrumentation fault event for watchdog reset */
    CY_APP_INST_EVT_HARD_FAULT = 1,                /**< 0x01: Instrumentation fault event for hard fault */
    CY_APP_INST_EVT_POWER_CYCLE = 2                /**< 0x02: Power cycle event */
} cy_en_inst_evt_t;

/** \} group_pmg_app_common_system_instrumentation_enums */
/** \} group_pmg_app_common_system_enums */

/**
* \addtogroup group_pmg_app_common_system_data_structures
* \{
* \defgroup group_pmg_app_common_system_instrumentation_data_structures instrumentation
* \{
*/
/**
 * @brief Callback function to solution level handler for instrumentation faults
 */
typedef void (*cy_app_instrumentation_cb_t)(uint8_t port, uint8_t evt);

/** \} group_pmg_app_common_system_instrumentation_data_structures */
/** \} group_pmg_app_common_system_data_structures */

/**
* \addtogroup group_pmg_app_common_system_functions
* \{
* \defgroup group_pmg_app_common_system_instrumentation_functions instrumentation
* \{
*/
/**
 * @brief Initialize data structures associated with application instrumentation
 * @param ptrTimerContext Pointer to the timer context
 * @return None
 */
void Cy_App_Instrumentation_Init(cy_stc_pdutils_sw_timer_t *ptrTimerContext);

/**
 * @brief Start any timers or tasks associated with application instrumentation
 * @return None
 */
void Cy_App_Instrumentation_Start(void);

/**
 * @brief Perform tasks associated with application instrumentation. The specific
 * functionality implemented is user defined and can vary.
 * @return None
 */
void Cy_App_Instrumentation_Task(void);

/**
 * @brief Register solution level callback function to be executed when instrumentation fault occurs
 *
 * @param cb Callback function from solution layer to handle the instrumentation faults
 *
 * @return None
 */
void Cy_App_Instrumentation_RegisterCb(cy_app_instrumentation_cb_t cb);

/**
 * @brief Function to get the number of times the system got reset due to watch dog timeout
 * @return reset count
 */
uint32_t Cy_App_Instrumentation_GetWdtResetCount(void);

/** \} group_pmg_app_common_system_instrumentation_functions */
/** \} group_pmg_app_common_system_functions */

#endif /* _CY_APP_INSTRUMENTATION_H_ */
/* [] END OF FILE */
