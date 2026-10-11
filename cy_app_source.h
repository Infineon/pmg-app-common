/***************************************************************************//**
* \file cy_app_source.h
* \version 3.0
*
* \brief
* Defines function prototypes for power provided
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

#ifndef CY_APP_SOURCE_H
#define CY_APP_SOURCE_H

/*******************************************************************************
 * Header files including
 ******************************************************************************/
#include <stdint.h>
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
 */
void Cy_App_Source_Disable(cy_stc_pdstack_context_t * context, cy_pdstack_pwr_ready_cbk_t pwr_ready_handler);

/**
 * @brief Turns on Gate Driver of VBUS Provider FET.
 *
 * @param context Pointer to the PDStack context
 */
void Cy_App_Source_VbusFetOn(cy_stc_pdstack_context_t *context);
/** \} group_pmg_app_common_psrc_functions */

/* Weak function prototypes */
#if ((CY_APP_USE_EXTERNAL_CURRENT_MON) && (VBUS_OCP_ENABLE))
bool soln_set_current (cy_stc_pdstack_context_t * context, uint16_t cur_10mA);
#endif
void soln_vbus_fet_on (cy_stc_pdstack_context_t *context);
void soln_vbus_fet_off (cy_stc_pdstack_context_t *context);
void soln_src_set_voltage (cy_stc_pdstack_context_t *context);
bool soln_vbus_div_enabled (void);
bool soln_buck_boost_vbus_present(cy_stc_pdstack_context_t * context);
void soln_set_volt_port1 (uint16_t vol_in_mv);

#if PMG1_PD_DUALPORT_ENABLE
void soln_set_volt_port2 (uint16_t vol_in_mv);
#endif
#if CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE
void soln_vbus_div_control(cy_stc_pdstack_context_t *ctxt, uint32_t enable);
#endif

#if PMG1B1_BI_DIRECTION
/**
 * @brief Enable timer monitoring hysteresis for Buck-Boost output voltage.
 * That voltage is actual VBUS in PMG1B1 100W bidirection.
 *
 * @param context Pointer to the PDStack context
 * @param period Timer period in milliseconds.
 */
void Cy_App_Source_BB_En_Hys (cy_stc_pdstack_context_t *context, uint16_t period);
#endif /* PMG1B1_BI_DIRECTION */

#endif /* CY_APP_SOURCE_H */

/* [] END OF FILE */

