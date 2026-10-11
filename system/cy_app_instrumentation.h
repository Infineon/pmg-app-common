/***************************************************************************//**
* \file cy_app_instrumentation.h
* \version 3.0
*
* \brief
* Defines data structures and function prototypes to monitor
* CPU resource (execution time and stack) usage
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

#ifndef CY_APP_INSTRUMENTATION_H
#define CY_APP_INSTRUMENTATION_H

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
typedef enum 
{
    CY_APP_INST_EVT_WDT_RESET = 0u,                 /**< 0x00: Instrumentation fault event for watchdog reset */
    CY_APP_INST_EVT_HARD_FAULT = 1u,                /**< 0x01: Instrumentation fault event for hard fault */
    CY_APP_INST_EVT_POWER_CYCLE = 2u                /**< 0x02: Power cycle event */
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
 */
void Cy_App_Instrumentation_Init(cy_stc_pdutils_sw_timer_t *ptrTimerContext);

/**
 * @brief Start any timers or tasks associated with application instrumentation
 */
void Cy_App_Instrumentation_Start(void);

/**
 * @brief Perform tasks associated with application instrumentation. The specific
 * functionality implemented is user defined and can vary.
 */
void Cy_App_Instrumentation_Task(void);

/**
 * @brief Register solution level callback function to be executed when instrumentation fault occurs
 *
 * @param cb Callback function from solution layer to handle the instrumentation faults
 *
 */
void Cy_App_Instrumentation_RegisterCb(cy_app_instrumentation_cb_t cb);

/**
 * @brief Function to get the number of times the system got reset due to watch dog timeout
 * @return reset count
 */
uint32_t Cy_App_Instrumentation_GetWdtResetCount(void);

/** \} group_pmg_app_common_system_instrumentation_functions */
/** \} group_pmg_app_common_system_functions */

#endif /* CY_APP_INSTRUMENTATION_H */
/* [] END OF FILE */
