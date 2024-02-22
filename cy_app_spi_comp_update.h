/***************************************************************************//**
* \file cy_app_spi_comp_update.h
* \version 1.0
*
* \brief
* Implements the SPI component firmware update interfaces
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_SPI_COMP_UPDATE_H_
#define _CY_APP_SPI_COMP_UPDATE_H_

#if (CY_APP_DMC_ENABLE || DOXYGEN)
#include <stdbool.h>
#include "cy_app_dmc_fwupdate.h"

/**
* \addtogroup group_pmg_app_common_dmc
* \{
*/

/**
* \addtogroup group_pmg_app_common_dmc_macros
* \{
*/
    
/*****************************************************************************
 ********************************* Macros ************************************
 *****************************************************************************/
/** Type of candidate package - primary */
#define CY_APP_DMC_CANDIDATE_PACKAGE_PRIMARY                  (CY_APP_DMC_IMAGE_TYPE_FWIMAGE_1)

/** Type of candidate package - factory */
#define CY_APP_DMC_CANDIDATE_PACKAGE_FACTORY                  (CY_APP_DMC_IMAGE_TYPE_FWIMAGE_2)
    
/** Candidate package area size in bytes */
#define CY_APP_DMC_PACKAGE_AREA_MAX_SIZE                       ((1024 << 10) << 1)    /* 2MB */

/** Candidate package 64K size */
#define CY_APP_DMC_PACKAGE_64K_BLOCK_SIZE                      (64 << 10)          /* 64 KB */
    
/** SPI flash page program size */
#define CY_APP_DMC_SPI_PAGE_PROG_SIZE                          (0x100)

/** Page boundary bit mask */
#define CY_APP_DMC_PAGE_BOUNDARY_BIT_MASK                      (CY_APP_DMC_MUL_OF_256_MASK)
    
/** SPI flash page size */
#define CY_APP_DMC_SPI_PAGE_SIZE                               (256u)

/** SPI flash page size mask */
#define CY_APP_DMC_SPI_PAGE_SIZE_MASK                          (CY_APP_DMC_SPI_PAGE_SIZE - 1u)

/** Metadata address in SPI flash */
#define CY_APP_DMC_SPI_METADATA_ADDRESS                        (0x20000u)

/** Metadata size in SPI flash */
#define CY_APP_DMC_SPI_METADATA_ADDRESS_SIZE                   (1u)

/** Time in milliseconds for periodic verification of SPI Erase operation status. */
#define CY_APP_DMC_SPI_EEPROM_ERASE_DELAY     (10u)

/** Time in milliseconds for periodic verification of SPI Write operation status. */
#define CY_APP_DMC_SPI_EEPROM_WRITE_DELAY     (1u)

/** \} group_pmg_app_common_dmc_macros */

/**
* \addtogroup group_pmg_app_common_dmc_enums
* \{
*/
/**
 * @typedef cy_en_spi_comp_state_t
 * @brief SPI Component status enumeration
 */
typedef enum
{
    CY_APP_SPI_COMP_STATE_FACTORY_PACKAGE_ERASE = 0,  /**< Factory package erase */
    CY_APP_SPI_COMP_STATE_PRIMARY_PACKAGE_ERASE,      /**< Primary package erase */
    CY_APP_SPI_COMP_STATE_PRIMARY_PACKAGE_FWCT_WRITE, /**< Primary package FWCT write */
    CY_APP_SPI_COMP_STATE_PRIMARY_PACKAGE_SIG_WRITE,  /**< Primary package signature write */
    CY_APP_SPI_COMP_STATE_PHASE1_ROW_WRITE,           /**< Phase 1 write */
    CY_APP_SPI_COMP_STATE_FACTORY_COPY                /**< Factory copy */
    
} cy_en_spi_comp_state_t;
/** \} group_pmg_app_common_dmc_enums */

/**
* \addtogroup group_pmg_app_common_dmc_functions
* \{
*/
/*****************************************************************************
 **************************** Function prototypes ****************************
 *****************************************************************************/

/**
 * @brief Pointer to the structure of function pointers related to
 * various SPI_COMP operation.
 * @return cy_stc_app_dmc_update_opern_t Pointer to the cy_stc_app_dmc_update_opern_t
 */
const cy_stc_app_dmc_update_opern_t *Cy_App_Dmc_SpiGetCompOperation(void);

/**
 * @brief Updates SPI flash component ID in RAM buffer
 * @param comp_id Component ID of the SPI flash
 * @return None
 */
void Cy_App_Dmc_SpiSetCompId (uint8_t comp_id);

/**
 * @brief Gets SPI flash component ID
 * @return Component ID
 */
uint8_t Cy_App_Dmc_SpiGetCompId (void);

/** @cond DOXYGEN_HIDE */
/**
 * @brief Erases the given package area in the SPI flash
 * @param package Package area corresponding to package to be erased
 * @param package_size Size of package in bytes
 * @param cb Callback to be invoked after erase completion
 * @param params Pointer to the DMC parameters
 * @return One of cy_en_app_status_t
 */
cy_en_app_status_t Cy_App_Dmc_SpiErasePackageArea (uint8_t package, uint32_t package_size, cy_stc_app_dmc_completion_cbk_t cb, cy_stc_dmc_params_t *params);

/**
 * @brief Copies the firmware images from the primary area to the factory area
 * @param package_size Size of package in bytes
 * @param dest_ptr Pointer to the destination where the image is copied
 * @param cb Invokes callback after completion of the copy
 * @param params Pointer to the DMC parameters
 * @return One of cy_en_app_status_t
 */
cy_en_app_status_t Cy_App_Dmc_SpiCopyPrimaryToFactory (uint32_t package_size, uint8_t *dest_ptr, cy_stc_app_dmc_completion_cbk_t cb, cy_stc_dmc_params_t *params);

/**
 * @brief Continues the SPI flash operation
 * @param params Pointer to the DMC parameters
 * @return None
 */
void Cy_App_Dmc_SpiFlashContinueOperation(cy_stc_dmc_params_t *params);
/** @endcond */

/** \} group_pmg_app_common_dmc_functions */
/** \} group_pmg_app_common_dmc */

#endif /* (CY_APP_DMC_ENABLE || DOXYGEN) */
#endif /* _CY_APP_SPI_COMP_UPDATE_H_ */

/* [] End of file */