/***************************************************************************//**
* \file cy_app_sink.h
* \version 3.0
*
* \brief
* Defines function prototypes for power consumer
* path control and fault detection.
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

#ifndef CY_APP_SINK_H
#define CY_APP_SINK_H

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
 */
void Cy_App_Sink_SetCurrent (cy_stc_pdstack_context_t * context, uint16_t cur_10mA);

/**
 * @brief Enables the power consumer path to that the system
 * can received power from the Type-C VBUS. The expected voltage and maximum
 * allowed current is notified through the Cy_App_Sink_SetVoltage()
 * and Cy_App_Sink_SetCurrent() functions.
 *
 * @param context Pointer to the PDStack context
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
 */
void Cy_App_Sink_Disable (cy_stc_pdstack_context_t * context, cy_pdstack_sink_discharge_off_cbk_t snk_discharge_off_handler);

/**
 * @brief Turns on Gate Driver of Sink FET (Consumer FET).
 *
 * @param context Pointer to the PDStack context
 */
void Cy_App_Sink_FetOn(cy_stc_pdstack_context_t * context);

/**
 * @brief Function allows VBus consumer FET control over the HPI interface.
 *
 * @param context Pointer to the PDStack context
 * @param ctrl_p Pointer to the control command
 * @return true if the operation is success otherwise false.
 */
bool Cy_App_Sink_VbusCFetOnCtrl(cy_stc_pdstack_context_t *context, uint8_t *ctrl_p);

/** \} group_pmg_app_common_psnk_functions */

/* Weak function prototypes for solution-specific sink FET control */
void soln_sink_fet_off(cy_stc_pdstack_context_t * context);
void soln_sink_fet_on(cy_stc_pdstack_context_t * context);

#endif /* CY_APP_SINK_H */

/* [] END OF FILE */

