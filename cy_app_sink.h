/***************************************************************************//**
* \file cy_app_sink.h
* \version 1.0
*
* \brief
* Defines function prototypes for power consumer
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

#ifndef _CY_APP_SINK_H_
#define _CY_APP_SINK_H_

#include "cy_pdstack_common.h"

/**
* \addtogroup group_pmg_app_common_psnk
* \{
* The consumer path handler provides APIs and data structures to control the power 
* consumer path and enables the fault detection.
* These APIs are registered as application callbacks and used by the PDStack middleware library.
* 
* <b>Features:</b>
* * Set fault threshold voltage
* * Enable/Disable consumer path
*
*********************************************************************************
* \section section_pmg_app_common_psnk_config Configuration Considerations
********************************************************************************
*
* The following steps describe the usage of these APIs.
*
* 1. Include psink.h to get access to all functions and other declarations.
*    \snippet psink_sut.c snippet_psnk_include
*    \n
* 2. Initialize the functions to the application callback structure.
*    \snippet psink_sut.c snippet_psnk_cbk_structure
*    \n
* 3. Register the application callback to the PDStack middleware library.
*    See the \ref section_pmg_app_common_quick_start section.
*
* \defgroup group_pmg_app_common_psnk_functions Functions
*/

/** \} group_pmg_app_common_psnk */


/**
* \addtogroup group_pmg_app_common_psnk_functions
* \{
*/

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
 * @brief Sets the expected VBus voltage when
 * USB PD port is functioning as a sink. The voltage level is used
 * to configure the overvoltage protection on the device.
 *
 * @param context Pointer to the PDStack context
 * @param volt_mV Expected VBus voltage level in mV unit
 *
 * @return None
 */
void Cy_App_Sink_SetVoltage (cy_stc_pdstack_context_t * context, uint16_t volt_mV);

/**
 * @brief Notifies the application code about
 * the amount of current the system is allowed to take from
 * the VBUS power supply. The application logic should configure its
 * load and battery charging circuits based on this value so that
 * the power source does not see any overload condition.
 *
 * @param context Pointer to the PDStack context
 * @param cur_10mA Maximum allowed current in 10 mA unit
 *
 * @return None
 */
void Cy_App_Sink_SetCurrent (cy_stc_pdstack_context_t * context, uint16_t cur_10mA);

/**
 * @brief Enables the power consumer path to that the system
 * can received power from the Type-C VBUS. The expected voltage and maximum
 * allowed current is notified through the Cy_App_Sink_SetVoltage()
 * and Cy_App_Sink_SetCurrent() functions.
 *
 * @param context Pointer to the PDStack context
 * @return None
 */
void Cy_App_Sink_Enable (cy_stc_pdstack_context_t * context);

/**
 * @brief Disables the VBUS power sink path and discharge VBUS supply down
 * to a safe level. This function is called by the PDstack at times when
 * the system is not allowed to draw power from the VBUS supply. The application
 * can use this call to initiate a VBUS discharge operation so that a subsequent
 * Type-C connection is speeded up. The snk_discharge_off_handler callback
 * function is called once VBUS discharges down to vSafe0V.
 *
 * @param context Pointer to the PDStack context
 * @param snk_discharge_off_handler Sink Discharge fet off callback pointer
 * @return None
 */
void Cy_App_Sink_Disable (cy_stc_pdstack_context_t * context, cy_pdstack_sink_discharge_off_cbk_t snk_discharge_off_handler);

/** \} group_pmg_app_common_psnk_functions */
#endif /* _CY_APP_SINK_H_ */

/* [] END OF FILE */

