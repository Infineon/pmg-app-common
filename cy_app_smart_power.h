/***************************************************************************//**
* \file cy_app_smart_power.h
* \version 2.0
*
* \brief
* Defines data structures and function prototypes for smart power management
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_SMART_POWER_H_
#define _CY_APP_SMART_POWER_H_

#if (CY_APP_SMART_POWER_ENABLE || DOXYGEN)

#include <stdbool.h>
#include <stdint.h>
#include "cy_pdl.h"
#include "cy_pdstack_common.h"
#include "cy_usbpd_config_table.h"

/**
* \addtogroup group_pmg_app_common_smart_power
* \{
* The smart power library implements a power throttling algorithm. The power throttling
* aims to provide remainder of power consumed by all the down stream devices and
* the board to the UP stream (US) port.
*
********************************************************************************
* \section section_pmg_app_common_smart_power_config Configuration considerations
********************************************************************************
*
* The following steps describe the way for enabling the smart power library
* in an application.
*
* 1. Configure the smart power parameters using EZ-PD(TM) Dock Configuration utility.
*
* 2. Include cy_app_smart_power.h to get access to all the functions and
* other declarations in this library.
*    \snippet snippet/smart_power_snippet.c snippet_configuration_include
*
* 3. Define the following data structure required by the library.
*    \snippet snippet/smart_power_snippet.c snippet_configuration_smart_power
*
* 4. Initialize the smart power library.
*    \snippet snippet/smart_power_snippet.c snippet_smart_power_init
*
* 5. Invoke the Cy_App_SmartPower_Start function on US port PD contract negotiation
* complete event.
*    \snippet snippet/smart_power_snippet.c snippet_smart_power_start
*
* 6. Invoke the Cy_App_SmartPower_Task function from the main processing loop of the
* application to handle the smart power tasks.
*    \snippet snippet/smart_power_snippet.c snippet_smart_power_task
*
********************************************************************************
*
* \defgroup group_pmg_app_common_smart_power_data_structures Data structures
* \defgroup group_pmg_app_common_smart_power_functions Functions
*/
/** \} group_pmg_app_common_led*/

/**
 * \addtogroup group_pmg_app_common_smart_power_data_structures
 * \{
 * */
/*******************************************************************************
* Data structure definition
*******************************************************************************/
/* Forward declarations of structures */
struct cy_stc_app_smart_power_context;

/**
 * @brief Data structure to hold the smart power parameters
 */
typedef struct cy_stc_app_smart_power_context
{
    /** Adapter power in watts */
    uint16_t adapterPower;

    /** Adapter voltage in milli volts */
    uint16_t adapterVoltage;

    /** Smart power start delay timer ID */
    cy_timer_id_t smartPowerDelayTimerId;

    /** Source capability change retry timer ID */
    cy_timer_id_t capChangeRetryTimerId;

    /** Pointer to the upstream port PDStack context */
    cy_stc_pdstack_context_t *pdStackContext;

    /** Smart power configuration table pointer */
    const smart_power_config_t *ptrSmartPowerCfg;

    /** Callback function for Dock total current measurement */
    uint16_t (*dockTotalCurrentCbk)(struct cy_stc_app_smart_power_context *context);

    /** Callback function for Dock up-stream port current measurement */
    uint16_t (*upstreamCurrentCbk)(struct cy_stc_app_smart_power_context *context);
} cy_stc_app_smart_power_context_t;

/** \} group_pmg_app_common_smart_power_data_structures */

/**
* \addtogroup group_pmg_app_common_smart_power_functions
* \{
*/
/*******************************************************************************
* Function declaration
********************************************************************************/
/**
 * @brief Initializes the smart power required parameters
 *
 * @param context - Pointer to the cy_stc_app_smart_power_context_t context
 * @param ptrSmartPowerCfg - Pointer to the smart power configuration table
 * @param ptrPdStackContext - Upstream port PDStack pointer
 * @param adapterPower - Adapter power in watts
 * @param adapterVoltage - Adapter voltage in milli volts
 *
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_SmartPower_Init(cy_stc_app_smart_power_context_t *context,
                        const smart_power_config_t *ptrSmartPowerCfg,
                        cy_stc_pdstack_context_t *ptrPdStackContext,
                        uint16_t adapterPower,
                        uint16_t adapterVoltage);

/**
 * @brief Starts the smart power throttling
 *
 * @param context - Pointer to the cy_stc_app_smart_power_context_t context
 * @return Return true if operation is success otherwise, false.
 */
bool Cy_App_SmartPower_Start(cy_stc_app_smart_power_context_t *context);

/**
 * @brief Stops the smart power throttling
 *
 * @param context - Pointer to the cy_stc_app_smart_power_context_t context
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_SmartPower_Stop(cy_stc_app_smart_power_context_t *context);

/**
 * @brief Updates the Upstream port power-based on current consumed by the Dock.
 *
 * @param context - Pointer to the cy_stc_app_smart_power_context_t context
 * @return None
 */
void Cy_App_SmartPower_Task(cy_stc_app_smart_power_context_t *context);

/** \} group_pmg_app_common_smart_power_functions */

#endif /* (CY_APP_SMART_POWER_ENABLE || DOXYGEN) */

#endif /* _CY_APP_SMART_POWER_H_ */

/* [] End of file */
