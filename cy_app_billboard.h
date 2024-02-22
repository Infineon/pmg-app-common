/***************************************************************************//**
* \file cy_app_billboard.h
* \version 1.0
*
* \brief
* Billboard control interface header file
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_BILLBOARD_H_
#define _CY_APP_BILLBOARD_H_

#if (CCG_BB_ENABLE || DOXYGEN)

#include "cy_pdaltmode_defines.h"
#include "cy_pdaltmode_billboard.h"

/*****************************************************************************
 **************************** Function prototypes ****************************
 *****************************************************************************/
/**
* \addtogroup group_pmg_app_common_usb
* \{
* Include cy_app_usb.h and cy_app_billboard.h to get access to all the functions and declarations used by the USB module. See the configuration considerations section to start using the USB module.
*
* The USB module operates on top of the USB device middleware library. It provides support for vendor, billboard and HID interfaces.
*
* \defgroup group_pmg_app_common_usb_enums enumerated types
* \defgroup group_pmg_app_common_usb_data_structures data structures
* \defgroup group_pmg_app_common_usb_functions functions
*/

/**
* \addtogroup group_pmg_app_common_usb_functions
* \{
*/

/**
 * @brief Initialize the Billboard module
 *
 * The API initializes the Billboard module. This is mainly a software state
 * machine initialization. The USB device is not enabled at this point. The API
 * helps to cleanup previous state information.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 *
 * @return Status of the call
 */
cy_en_pdaltmode_billboard_status_t Cy_App_Usb_BbInit(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/**
 * @brief Billboard module task function
 *
 * The function implements the billboard state machine and needs to be invoked
 * in the main task loop. The task handler allows deferring interrupts and
 * improves interrupt latency of the system.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 *
 * @return None
 */
void Cy_App_Usb_BbTask(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/**
 * @brief Function requests for flashing mode operation
 *
 * The function enables/disables the billboard interface in USB flashing mode.
 * This should be invoked only if the solution requires to start
 * the flashing mode. This request shall prevent the billboard expiry on enable
 * and restart the billboard expiry on disable as required.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 * @param enable Enable/disable control. true = Enable, false = Disable.
 *
 * @return Status of the call
 */
cy_en_pdaltmode_billboard_status_t Cy_App_Usb_BbFlashingCtrl(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool enable);

/**
 * @brief Function returns whether the billboard module is idle or not
 *
 * This function indicates whether there are any pending tasks or not. This
 * function can be invoked before device enters Sleep mode to check if it is
 * allowed. But idle condition does not allow Deep Sleep entry. For this,
 * the application is expected to use the Cy_App_Usb_Sleep() function.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 *
 * @return true = billboard module is idle; false = billboard module is busy.
 */
bool Cy_App_Usb_BbIsIdle(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/**
 * @brief Function that returns the billboard firmware version information
 *
 * @return Pointer to buffer containing billboard firmware version
 */
uint8_t *Cy_App_Usb_BbGetVersion(void);

/**
 * @brief Bind the billboard function to a specific port. This will be required in
 * cases where a single billboard device is used across multiple PD ports. The binding
 * will be vacated when bb_disable is called.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 *
 * @return CCG_STAT_SUCCESS if the binding is successful, error code otherwise.
 */
cy_en_pdaltmode_billboard_status_t Cy_App_Usb_BbBindToPort (cy_stc_pdaltmode_context_t *ptrAltModeContext);

/**
 * @brief Check whether the billboard is enabled for the given Altmode context
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 *
 * @return True if billboard is currently enabled
 */
bool Cy_App_Usb_BbIsEnabled(cy_stc_pdaltmode_context_t *ptrAltModeContext);

/**
 * @brief Update the self powered status bit reported in the billboard device's
 * configuration descriptor.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 * @param self_pwrd The power-up status (0 = not self powered, other values mean self powered).
 *
 * @return None
 */
void Cy_App_Usb_BbUpdateSelfPwrStatus (cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t self_pwrd);

/** \} group_pmg_app_common_usb_functions */
/** \} group_pmg_app_common_usb */

#endif /* (CCG_BB_ENABLE || DOXYGEN) */
#endif /* _CY_APP_BILLBOARD_H_ */

/* [] END OF FILE */

