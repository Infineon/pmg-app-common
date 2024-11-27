/***************************************************************************//**
* \file cy_app_fault_handlers.h
* \version 2.0
*
* \brief
* Defines data structures and function prototypes associated with
* fault handling.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_FAULT_HANDLERS_H_
#define _CY_APP_FAULT_HANDLERS_H_

/*******************************************************************************
 * Header files including
 ******************************************************************************/
#include <stdint.h>
#include "cy_app_debug.h"
#include "cy_app_config.h"

#include "cy_usbpd_common.h"
#include "cy_usbpd_vbus_ctrl.h"
#include "cy_usbpd_typec.h"
#include "cy_pdstack_dpm.h"

/**
* \addtogroup group_pmg_app_common_fault
* \{
* Fault handler provides API to monitor and handle the fault situations on the 
* VBus or VConn voltage rails, and trip the supply to avoid damage to any of the
* system components. Fault handler continuously monitors the voltages and 
* compare with threshold values. Fault handler is built over the USB PD driver.
*
* <b>Features:</b>
* * VBus Reverse Current protection
* * VBus overvoltage protection
* * VBus undervoltage protection
* * VBus short-circuit protection
* * VBus overcurrent protection
* * VConn overcurrent protection
*
********************************************************************************
* \section section_pmg_app_common_fault_config Configuration considerations
********************************************************************************
*
* The following steps describe the way to enable fault handler in an application.
*
* 1. Include fault_handlers.h and cy_pdl.h to get access to all functions and 
*    other declarations.
*    \snippet fault_handler_sut.c snippet_fault_handler_include
*    \n
* 2. Defines the USB PD and PDStack data structures.
*    \snippet fault_handler_sut.c snippet_fault_handler_usbpd_config
*    \n
* 3. Configure the fault parameters using the Device Configurator available in the 
*    ModusToolbox&tm; quick panel.
*    \image html usbpd_fault_config.png width=90%
*
*    \n Or, if the application uses a configuration table, configure the fault 
*    parameters using the Configuration Utility. Then initialize the fault configuration 
*    in the USB PD configuration structure.
*    \snippet fault_handler_sut.c snippet_fault_handler_data_structure
*    \n
*
* 4. Use the following make file defines to enable/disable faults
*    * VBUS_RCP_ENABLE 
*    * VBUS_SCP_ENABLE 
*    * VBUS_OCP_ENABLE
*    * VBUS_OVP_ENABLE
*    * VBUS_UVP_ENABLE
*    * VCONN_OCP_ENABLE
*    \n \n
*
* 5. Call the fault enable API to enable the desired fault detection
*    \snippet fault_handler_sut.c snippet_fault_handler_init
*    \n
*
* 6. Invoke fault event handler from the application event handler to handle the events
*    \snippet fault_handler_sut.c snippet_fault_handler_event
*    \n
*
* 7. Invoke the fault handler task form the main loop to handle the fault conditions
*    \snippet fault_handler_sut.c snippet_fault_handler_task
*    \n
*
* \defgroup group_pmg_app_common_fault_functions Functions
*/
/** \} group_pmg_app_common_fault */


/*******************************************************************************
 * Functions
 ******************************************************************************/
/**
* \addtogroup group_pmg_app_common_fault_functions
* \{
*/

/**
 * @brief Enable and configure the VBus overvoltage protection circuitry
 *
 * @param context Pointer to the PDStack context
 * @param volt_mV Expected VBus voltage
 * @param pfet Whether PFET is used for the power supply control
 * @param ovp_cb Callback function to be triggered when there is an OV event
 *
 * @return None
 */
void Cy_App_Fault_OvpEnable(cy_stc_pdstack_context_t *context, uint16_t volt_mV,
                            bool pfet, cy_cb_vbus_fault_t ovp_cb);

/**
 * @brief Disable the VBus OverVoltage protection circuitry
 *
 * @param context Pointer to the PDStack context
 * @param pfet Whether PFET is used for the power supply control
 *
 * @return None
 */
void Cy_App_Fault_OvpDisable(cy_stc_pdstack_context_t *context, bool pfet);

/**
 * @brief Enable and configure the VBus UnderVoltage protection circuitry
 *
 * @param context Pointer to the pdstack context
 * @param volt_mV Expected VBus voltage
 * @param pfet Whether PFET is used for the power supply control
 * @param uvp_cb Callback function to be triggered when there is an UV event
 *
 * @return None
 */
void Cy_App_Fault_UvpEnable(cy_stc_pdstack_context_t *context, uint16_t volt_mV,
                            bool pfet, cy_cb_vbus_fault_t uvp_cb);

/**
 * @brief Disable the VBus UnderVoltage protection circuitry.
 *
 * @param context Pointer to the PDStack context
 * @param pfet Whether PFET is used for the power supply control.
 *
 * @return None
 */
void Cy_App_Fault_UvpDisable(cy_stc_pdstack_context_t *context, bool pfet);

/**
 * @brief Enable and configure the VBus reverse-current protection circuitry
 *
 * @param context Pointer to the PDStack context
 * @param volt_mV Expected VBus voltage
 * @param rcp_cb Callback function to be triggered when there is an RCP event
 *
 * @return None
 */
void Cy_App_Fault_RcpEnable(cy_stc_pdstack_context_t *context, uint16_t volt_mV,
                            cy_cb_vbus_fault_t rcp_cb);

/**
 * @brief Disable the VBus reverse-current protection circuitry
 *
 * @param context Pointer to the PDStack context 
 *
 * @return None
 */
void Cy_App_Fault_RcpDisable(cy_stc_pdstack_context_t * context);

/**
 * @brief Enable and configure the VBus overcurrent protection circuitry
 *
 * @param context Pointer to the PDStack context
 * @param current Expected VBus current
 * @param ocp_cb Callback function to be triggered when there is an OCP event
 *
 * @return None
 */
void Cy_App_Fault_OcpEnable(cy_stc_pdstack_context_t *context, uint32_t current,
                            cy_cb_vbus_fault_t ocp_cb);
                            
/**
 * @brief Disable the VBus overcurrent protection circuitry
 *
 * @param context Pointer to the PDStack context
 * @param pctrl Whether PFET is used for the power supply control
 *
 * @return None
 */                            
void Cy_App_Fault_OcpDisable(cy_stc_pdstack_context_t *context, bool pctrl);

/**
 * @brief Enable and configure the VBus short-circuit protection circuitry.
 *
 * @param context Pointer to the PDStack context
 * @param current Expected VBus current
 * @param scp_cb Callback function to be triggered when there is an SCP event
 *
 * @return None
 */
void Cy_App_Fault_ScpEnable(cy_stc_pdstack_context_t *context, uint32_t current,
                            cy_cb_vbus_fault_t scp_cb);
                            
/**
 * @brief Disable the VBus short-circuit protection circuitry
 *
 * @param context Pointer to the PDStack context
 *
 * @return None
 */                            
void Cy_App_Fault_ScpDisable(cy_stc_pdstack_context_t * context);

/**
 * @brief Enable and configure the VConn overcurrent protection circuitry
 *
 * @param context Pointer to the PDStack context
 * @param vconn_ocp_cb Callback function to be triggered when there is a VCONN OCP event
 *
 * @return None
 */
void Cy_App_Fault_Vconn_OcpEnable(cy_stc_pdstack_context_t *context, 
                                  cy_cb_vbus_fault_t vconn_ocp_cb);
  
  /**
 * @brief Disable the VConn overcurrent protection circuitry
 *
 * @param context Pointer to the PDStack context
 *
 * @return None
 */   
void Cy_App_Fault_Vconn_OcpDisable(cy_stc_pdstack_context_t *context);

/**
 * @brief Wrapper function for PD port disable. This function is used to ensure that
 * any application level state associated with a faulty connection are cleared when a PD port is disableD.
 *
 * @param context Pointer to the PDStack context
 * @param cbk Callback to be called after operation is complete
 *
 * @return CCG_STAT_SUCCESS on success, appropriate error code otherwise.
 */
cy_en_pdstack_status_t Cy_App_Fault_DisablePort(cy_stc_pdstack_context_t *context,
                                                cy_pdstack_dpm_typec_cmd_cbk_t cbk);

/**
 * @brief Initialize fault-handling related variables from the configuration table
 *
 * @param context Pointer to the pdstack context
 *
 * @return false if the configuration table is invalid; otherwise true.
 */
bool Cy_App_Fault_InitVars (cy_stc_pdstack_context_t *context);

/**
 * @brief Clear the fault occurrence counts after a Type-C detach is detected
 *
 * @param port port on which the faults need to be cleared
 *
 * @return None
 */
void Cy_App_Fault_ClearCounts (uint8_t port);

/**
 * @brief Perform any fault handler related tasks
 *
 * @param context Pointer to the PDStack context
 *
 * @return None
 */
void Cy_App_Fault_Task (cy_stc_pdstack_context_t *context);

/**
 * @brief Handle any application events associated with fault handling logic
 *
 * @param context Pointer to the pdstack context
 * @param evt Event code.
 * @param dat Data associated with the event code
 *
 * @return true if the event does not need to be passed up to the solution layer
 */
bool Cy_App_Fault_EventHandler(cy_stc_pdstack_context_t *context, 
                               cy_en_pdstack_app_evt_t evt, const void *dat);

/**
 * @brief Prepare the CCG to wait for physical detach of faulty port partner
 *
 * @param context Pointer to the PDStack context
 *
 * @return None
 */
void Cy_App_Fault_ConfigureForDetach(cy_stc_pdstack_context_t * context);

/**
 * @brief Check whether any fault count has exceeded limit for the specified PD port
 *
 * @param context Pointer to the pdstack context
 *
 * @return true if fault count has exceeded limit; false otherwise.
 */    
bool Cy_App_Fault_IsCountExceeded(cy_stc_pdstack_context_t * context);

/** \} group_pmg_app_common_fault_functions */

#endif /* _CY_APP_FAULT_HANDLERS_H_ */

/* [] END OF FILE */
