/***************************************************************************//**
* \file cy_app_source.h
* \version 1.0
*
* \brief
* Defines function prototypes for power provided
* path control and fault detection.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_SOURCE_H_
#define _CY_APP_SOURCE_H_

/*******************************************************************************
 * Header files including
 ******************************************************************************/
#include <stdint.h>
#include "cybsp.h"
#include "cy_app_sink.h"
#include "cy_app.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"
#include "cy_usbpd_vbus_ctrl.h"

/**
* \addtogroup group_pmg_app_common_psrc
* \{
* Provider path handler provides APIs and data structures to control the power 
* provider path and enables the fault detection.
* These APIs are registered as application callbacks and used by the PDStack middleware library.
* 
* <b>Features:</b>
* * Set fault threshold voltage
* * Enable/Disable provider path
* * Set VBus voltage and current
*
*********************************************************************************
* \section section_pmg_app_common_psrc_config Configuration considerations
********************************************************************************
*
* The following steps describe the usage of these APIs:
*
* 1. Include cy_app_source.h to get access to all functions and other declarations.
*    \snippet psource_sut.c snippet_psrc_include
*    \n
* 2. Initialize the functions to the application callback structure.
*    \snippet psource_sut.c snippet_psrc_cbk_structure
*    \n
* 3. Register the application callback to the PDStack middleware library.
*    See the \ref section_pmg_app_common_quick_start section.
*
* \defgroup group_pmg_app_common_psrc_functions Functions
*/

/** \} group_pmg_app_common_psrc */

/**
* \addtogroup group_pmg_app_common_psrc_functions
* \{
*/

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
 * @brief Sets the VBUS source voltage to the desired value. It
 * also updates the voltage thresholds associated with protection schemes such
 * as OVP and UVP.
 *
 * @param context Pointer to the PDStack context
 * @param volt_mV Voltage in mV units
 *
 * @return None
 */
void Cy_App_Source_SetVoltage(cy_stc_pdstack_context_t * context, uint16_t volt_mV);

/**
 * @brief Gets the VBUS source voltage that is currently
 * configured. This is different from the Cy_App_VbusGetValue() function which
 * measures the actual VBUS voltage.
 *
 * @param context Pointer to the PDStack context
 * @return Voltage in mV units
 */
uint32_t Cy_App_Source_GetVoltage (cy_stc_pdstack_context_t *context);

/**
 * @brief Sets the VBUS source current limit. The current limits
 * are used to configure the current sensing circuits to trigger fault indication
 * in case of overload.
 *
 * @param context Pointer to the PDStack context
 * @param cur_10mA Current in 10 mA units
 * @return None
 */
void Cy_App_Source_SetCurrent (cy_stc_pdstack_context_t *context, uint16_t cur_10mA);

/**
 * @brief Enables the VBUS power supply. The voltage and current
 * to be supplied would have been specified through the Cy_App_Source_SetVoltage() and
 * Cy_App_Source_SetCurrent() calls before the supply is enabled. The function returns
 * as soon as the supply enable operation is started. The pwr_ready_handler
 * is expected to be called once VBUS voltage is stabilized at the desired level.
 *
 * @param context Pointer to the PDStack context
 * @param pwr_ready_handler Application handler callback function
 * @return None
 */
void Cy_App_Source_Enable (cy_stc_pdstack_context_t * context,
        cy_pdstack_pwr_ready_cbk_t pwr_ready_handler);

/**
 * @brief Disables the VBUS power supply. If a non-NULL
 * pwr_ready_handler callback is specified, the function can return after
 * starting the VBUS disable operation. The callback will be called once
 * the VBUS voltage has been safely brought to vSafe0V. If the callback
 * is NULL, the function is expected to return after shutting down the supply
 * without initiating any VBUS discharge sequence.
 *
 * @param context Pointer to the PDStack context
 * @param pwr_ready_handler Application handler callback function
 *
 * @return None
 */
void Cy_App_Source_Disable(cy_stc_pdstack_context_t * context, cy_pdstack_pwr_ready_cbk_t pwr_ready_handler);

/** \} group_pmg_app_common_psrc_functions */

#endif /* _CY_APP_SOURCE_H_ */

/* [] END OF FILE */

