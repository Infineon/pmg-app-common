/***************************************************************************//**
* \file cy_app_billboard.h
* \version 3.0
*
* \brief
* Billboard control interface header file
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

#ifndef CY_APP_BILLBOARD_H
#define CY_APP_BILLBOARD_H

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
* \defgroup group_pmg_app_common_usb_enums Enumerated Types
* \defgroup group_pmg_app_common_usb_data_structures Data Structures
* \defgroup group_pmg_app_common_usb_functions Functions
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
 * @brief Update the self powered status bit reported in the billboard device's
 * configuration descriptor.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 * @param self_pwrd The power-up status (0 = not self powered, other values mean self powered).
 *
 */
void Cy_App_Usb_BbUpdateSelfPwrStatus (cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t self_pwrd);

/** \} group_pmg_app_common_usb_functions */
/** \} group_pmg_app_common_usb */

#endif /* (CCG_BB_ENABLE || DOXYGEN) */
#endif /* CY_APP_BILLBOARD_H */

/* [] END OF FILE */

