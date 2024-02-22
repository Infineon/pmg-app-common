/***************************************************************************//**
* \file cy_app_dmc_metadata.h
* \version 1.0
*
* \brief
* Dock metadata handler function prototypes
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_DMC_METADATA_H_
#define _CY_APP_DMC_METADATA_H_

#if (CY_APP_DMC_ENABLE || DOXYGEN)

/*******************************************************************************
 * Header files including
 ******************************************************************************/
#include <stdbool.h>
#include "cy_app_dmc_common.h"

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
 * @return None
 */
void Cy_App_Dmc_InitMetadata(cy_stc_dmc_params_t *params);

/**
 * @brief Write dock metadata from RAM to flash
 *
 * @param init set true to reset metadata update counter; otherwise false.
 * @param params pointer to dmc parameters
 * @return None
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
void Cy_App_Dmc_SetSnFlag(uint8_t comp_id, cy_en_dmc_sn_state_t state, cy_stc_dmc_params_t *params);
#endif /* (CY_APP_USR_DEFINED_SN_SUPPORT) */

/** \} group_pmg_app_common_dmc_functions */
/** \} group_pmg_app_common_dmc */
#endif /* (CY_APP_DMC_ENABLE || DOXYGEN) */

#endif /* _CY_APP_DMC_METADATA_H_ */

/* [] END OF FILE */
