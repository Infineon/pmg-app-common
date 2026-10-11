/***************************************************************************//**
* \file cy_app_internal.h
* \version 3.0
*
* \brief
* Defines internal data structures associated with application level management
* of the USB-C port. This file shall not be included into files outside of
* the pmg-app-common module.
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

/** @cond DOXYGEN_HIDE */

#ifndef CY_APP_INTERNAL_H
#define CY_APP_INTERNAL_H

/*******************************************************************************
 * Header files including
 ******************************************************************************/
#include <stdint.h>
#include "cy_app.h"
#if CY_HPI_MASTER_ENABLE
#include "cy_hpi_master.h"
#endif /* CY_HPI_MASTER_ENABLE */

#if CY_APP_USB_ENABLE
#include "cy_app_usb.h"
#endif /* CY_APP_USB_ENABLE */

/*******************************************************************************
* Global variables for pmg-app-common internal use
********************************************************************************/
extern cy_stc_pdstack_app_status_t glAppPdStatus[];

/* Pointer to the structure holding the solution callback function. */
extern cy_app_sln_cbk_t *glPtrSlnCbk;

#if CY_APP_USB_ENABLE
extern cy_stc_usb_init_ctxt_t *glUsbInitCtx;
#endif /* CY_APP_USB_ENABLE */

#if (CY_APP_ROLE_PREFERENCE_ENABLE)
#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
/* Variable storing current preference for power role. */
extern volatile uint8_t glAppPrefPowerRole[NO_OF_TYPEC_PORTS];
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */

/* Variable storing current preference for data role */
extern volatile uint8_t glAppPrefDataRole[NO_OF_TYPEC_PORTS];
#endif /* (CY_APP_ROLE_PREFERENCE_ENABLE) */

#if BUMBLEBEE_DOCK
/* Billboard Warning Power Fail indicator. */
extern bool glAppBbWarnPwrFail;
#endif /* BUMBLEBEE_DOCK */

#if CY_HPI_MASTER_ENABLE
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 8.6', 1, 'used global variable from main application');
extern cy_hpi_master_context_t *get_hpi_master_context(void);
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 8.6');
#endif /* CY_HPI_MASTER_ENABLE */

#if (CY_APP_BOOT_ENABLE != 0)
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 8.6', 1, 'used global variable from main application');
extern uint32_t cyBtldrRunType;
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 8.6');
#endif /* CY_APP_BOOT_ENABLE */

#endif /* CY_APP_INTERNAL_H */

/** @endcond */

/* [] END OF FILE */
