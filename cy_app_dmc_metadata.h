/***************************************************************************//**
* \file cy_app_dmc_metadata.h
* \version 3.0
*
* \brief
* Dock metadata handler function prototypes
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

#ifndef CY_APP_DMC_METADATA_H
#define CY_APP_DMC_METADATA_H

#if (CY_APP_DMC_ENABLE || DOXYGEN)

/*******************************************************************************
 * Header files including
 ******************************************************************************/
#include <stdbool.h>
#include "cy_app_dmc_common.h"
#if CY_APP_USR_DEFINED_SN_SUPPORT
#include "cy_app_dmc_vendor.h"
#endif /* CY_APP_USR_DEFINED_SN_SUPPORT */

/**
* \addtogroup group_pmg_app_common_dmc
* \{
*/

/**
* \addtogroup group_pmg_app_common_dmc_macros
* \{
*/
/*******************************************************************************
 * MACRO definition
 ******************************************************************************/

/** Length of dock status header: sizeof(cy_stc_app_dmc_dock_status_t) - sizeof(cy_stc_app_dmc_devx_status_t) */
#define CY_APP_DMC_DMC_TATUS_HDR_LEN     (sizeof(cy_stc_app_dmc_dock_status_t) - (sizeof(cy_stc_app_dmc_devx_status_t) * CY_APP_DMC_MAX_DEV_COUNT))

/** \} group_pmg_app_common_dmc_macros */

/**
* \addtogroup group_pmg_app_common_dmc_functions
* \{
*/
/*****************************************************************************
 **************************** Function prototypes ****************************
 *****************************************************************************/
/**
 * @brief Initialize dock metadata.
 * When dock metadata is empty in DMC flash, form the dock metadata by querying
 * various devices.
 * When dock metadata is valid, copy the dock metadata contents from flash to
 * RAM copy. Update the dock metadata in RAM from CDTT parameters and write back
 * RAM copy into flash.
 *
 * @param params Pointer to DMC parameters structure
 *
 */
void Cy_App_Dmc_InitMetadata(cy_stc_dmc_params_t *params);

/**
 * @brief Write dock metadata from RAM to flash
 *
 * @param init set true to reset metadata update counter; otherwise false.
 * @param params pointer to dmc parameters
 */
void Cy_App_Dmc_WriteMetadata(bool init, cy_stc_dmc_params_t *params);

/**
 * @brief Returns the dock metadata.
 * @return Pointer to the dock metadata structure
 */
cy_stc_app_dmc_dock_metadata_t* Cy_App_Dmc_GetDockMetadata(void);

/**
 * @brief This function returns current status of the dock
 * @return Returns pointer to the dock status structure
 */
cy_stc_app_dmc_dock_status_t* Cy_App_Dmc_GetDockStatus(void);

#if (CY_APP_USR_DEFINED_SN_SUPPORT)
/**
 * @brief This function sets the Serial number status flag.
 * @param comp_id Component ID associated with serial number update.
 * @param state Serial number state to updated.
 * @param params pointer to dmc parameters.
 */
void Cy_App_Dmc_SetSnFlag(uint8_t comp_id, cy_en_dmc_sn_state_t state, cy_stc_dmc_params_t *params);

/**
 * @brief This function returns the DMC Serial number validity status.
 * @return Returns true if serial number is valid, false if serial number is invalid.
 */
bool Cy_App_Dmc_IsDmcSnValid(void);
#endif /* (CY_APP_USR_DEFINED_SN_SUPPORT) */

/** \} group_pmg_app_common_dmc_functions */
/** \} group_pmg_app_common_dmc */
#endif /* (CY_APP_DMC_ENABLE || DOXYGEN) */

#endif /* CY_APP_DMC_METADATA_H */

/* [] END OF FILE */
