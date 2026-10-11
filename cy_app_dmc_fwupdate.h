/***************************************************************************//**
* \file cy_app_dmc_fwupdate.h
* \version 3.0
*
* \brief
* DMC firmware update module
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

#ifndef CY_APP_DMC_FW_UPDATE_H
#define CY_APP_DMC_FW_UPDATE_H

#if (CY_APP_DMC_ENABLE || DOXYGEN)
/*******************************************************************************
 * Header files including
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "cy_usb_dev.h"
#include "cy_app_dmc_common.h"
#include "cy_app_status.h"
#include "cy_app_system.h"

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

/** Component ID of DMC. DMC should always be the first component.
 * @warning This value should not be changed
 */
#define CY_APP_DMC_COMPONENT_ID                        (0u)

/** Image 1 value is available in the lower nibble. To get image 1 status,
 *  mask with this value.
 * @warning Do not edit this value
 */
#define CY_APP_DMC_IMG1_STATUS_MASK                        (0x0Fu)

/** Image 2 value is available in the higher nibble. To get image 2 status,
 *  mask with this value.
 * @warning Do not edit this value
 */
#define CY_APP_DMC_IMG2_STATUS_MASK                        (0xF0u)

/** USB vendor BULK OUT endpoint */
#define CY_APP_DMC_USB_VENDOR_OUT_EP_INDEX                 (1u)

/** USB vendor interrupt IN endpoint. */
#define CY_APP_DMC_USB_VENDOR_INT_EP_INDEX                 (2u)

/**
 * @brief Largest packet size supported by the device. This is defined by
 * the firmware and should not be modified.
 */
#define CY_APP_DMC_USB_EP_MAX_PKT_SIZE                     (64u)

/** Length of data transferred over INT EP */
#define CY_APP_DMC_INT_EP_STATUS_LEN                       (3u)

/** @cond DOXYGEN_HIDE */
#if (CY_APP_USR_DEFINED_SN_SUPPORT)

/* DMC serial number (SN) bit mask in device_specific parameter */
#define CY_APP_DMC_METADATA_SN_BIT_MASK                    (0x0Eu)
#define CY_APP_DMC_METADATA_SN_BIT_POS                     (0x1u)

/* SN byte in the device specific parameter */
#define CY_APP_DMC_DEV_SPECIFIC_BYTE0                      (0x0u)

/*! Maximum length of serial number(in bytes) that can be stored in dock metadata */
#define CY_APP_DMC_METADATA_SN_MAX_LENGTH                  (128u)

#endif /* CY_APP_USR_DEFINED_SN_SUPPORT */
/** @endcond */

/** \} group_pmg_app_common_dmc_macros */

/*****************************************************************************
 **************************** Function prototypes ****************************
 *****************************************************************************/
/**
* \addtogroup group_pmg_app_common_dmc_functions
* \{
*/
/**
 * @brief This function is called before initiating a soft reset to DMC.
 * The reason for reset is updated in dock metadata as soft reset, so that the USB
 * enumeration happens.
 *
 * @param params Pointer to DMC parameters
 *
 */
void Cy_App_Dmc_PrepareSoftReset(cy_stc_dmc_params_t *params);

/** @cond DOXYGEN_HIDE */
/**
 * @brief Returns current device row number for the device being updated currently
 * @return current device row number for the device being updated currently
 */
uint16_t Cy_App_Dmc_GetCurRowId(void);

/**
 * @brief Returns current device row size for the device being updated currently
 *
 * @return current device row size for the device being updated currently
 */
uint16_t Cy_App_Dmc_GetCurRowSize(void);

/**
 * @brief Function to check if DMC is in Phase 2
 * @return True if DMC is in Phase 2; otherwise false.
 */
bool Cy_App_Dmc_IsPhase2 (void);

/**
 * @brief Function to check if factory update is done
 * @return True if update is completed; otherwise false.
 */
bool Cy_App_Dmc_IsFactoryUpdateDone (void);
/** @endcond */

/**
 * @brief Function to set factory update status
 * @param status New status to be set  
 */
void Cy_App_Dmc_SetFactoryUpdateStatus (bool status);

/**
 * @brief Function to read factory update status
 * @param params Pointer to DMC Parameters
 * @return True if read is successful; otherwise false.
 */
bool Cy_App_Dmc_ReadFactoryUpdateStatus (cy_stc_dmc_params_t *params);

/**
 * @brief Returns if the DMC FW update state machine has not started and DMC
 *        is in idle state.
 *
 * @returns true if DMC is in idle state; otherwise false.
 */
bool Cy_App_Dmc_IsIdle(void);

/** @cond DOXYGEN_HIDE */
/**
 * @brief Returns if the particular image of the particular device, identified
 *        by the component ID needs to be updated or not based on the FW
 *        version check.
 *
 * @param comp_id Unique number for which the device topology requires
 *
 * @param img_type  This is cy_en_dmc_image_type_t enum
 *
 * @returns true if the image needs to be updated; otherwise false.
 */
bool Cy_App_Dmc_IsImageUpdateNeeded(uint8_t comp_id, cy_en_dmc_image_type_t img_type);
/** @endcond */

/**
 * @brief Returns the validity of the image identified by the image type and
 *        component ID of the device.
 *
 * @param comp_id Unique number of the device for which the device topology requires
 * @param img_type  Firmware image type \ref cy_en_dmc_image_type_t
 * @param img_update_count_ptr  pointer to contain the update attempt count for the image
 *
 * @returns the validity status of the image- \ref cy_en_dmc_img_status_t
 */
uint8_t Cy_App_Dmc_CheckImageValidity (uint8_t comp_id, cy_en_dmc_image_type_t img_type, uint8_t *img_update_count_ptr);

/** @cond DOXYGEN_HIDE */
/**
 * @brief Returns maximum FWCT size supported currently
 *
 * @return maximum FWCT size supported currently
 */
uint16_t Cy_App_Dmc_GetMaximumFwctSize(void);
/** @endcond */

/**
 * @brief Handler for DMC level asynchronous tasks
 *        This function primarily handles firmware update for the connected
 *        devices and DMC. This shall be called as part of the main loop.
 *
 * @param params Pointer to the DMC parameters
 *
 */
void Cy_App_Dmc_Task(cy_stc_dmc_params_t *params);

/**
 * @brief DMC pre initialization
 * Initializes the globals
 *
 * @param device_mode Device mode
 * @param ptrCdttCfg Pointer to CDTT structure
 * @param ptrSecCfg Pointer to security configuration structure
 * @param ptrAppCbk Pointer to application callback
 * @param params Pointer to DMC Parameters
 * @return cy_en_app_status_t
 */
cy_en_app_status_t Cy_App_Dmc_PreInit(cy_en_sys_fw_mode_t device_mode, const cdtt_config_t *ptrCdttCfg, const sec_config_t *ptrSecCfg,
                  const cy_stc_dmc_app_cbk_t *ptrAppCbk, cy_stc_dmc_params_t *params);

/**
 * @brief Sets current image type and image status for the particular device
 *        identified using component ID
 *
 * @param comp_id       ID for the connected device
 * @param cur_img_type  Firmware image type \ref cy_en_dmc_image_type_t
 * @param img_status    Firmware image status \ref cy_en_dmc_img_status_t
 *
 */
void Cy_App_Dmc_SetRamImageStatus(uint8_t comp_id, cy_en_dmc_image_type_t cur_img_type, cy_en_dmc_img_status_t img_status);

/**
 * @brief Updates the image type and image status for the particular device
 *        identified using component ID.
 *
 * @param comp_id       ID for the connected device
 * @param img_type      Firmware image type \ref cy_en_dmc_image_type_t
 * @param img_status    Firmware image status \ref cy_en_dmc_img_status_t
 *
 */
void Cy_App_Dmc_UpdateRamImageStatus (uint8_t comp_id, cy_en_dmc_image_type_t img_type, cy_en_dmc_img_status_t img_status);

/**
 * @brief The firmware version for a given dock device is updated in the RAM Copy of
 * the dock metadata
 * It contains version information for bootloader, image 1, and image 2.
 *
 * @param comp_id       This is a unique value used to identify the device
 *                      that is connected to the DMC
 * @param bl_version    Bootloader version for the dock device
 * @param fw1_version   Firmware 1 version for the dock device
 * @param fw2_version   Firmware 2 version for the dock device
 *
 */
void Cy_App_Dmc_UpdateRamVersions(uint8_t comp_id, uint8_t *bl_version, uint8_t *fw1_version, uint8_t *fw2_version);

/**
 * @brief Resets DMC to known state
 *
 * @param params Pointer to DMC Parameters
 *
 */
void Cy_App_Dmc_ResetState (cy_stc_dmc_params_t *params);


/**
 * @brief Initiates soft reset to the DMC device
 *
 */
void Cy_App_Dmc_SoftReset(void);

/**
 * @brief Gets device topology information from CDTT based on comp_id.
 * This function returns the pointer to the dev_topology_t when the comp_id
 * is less than total device count connected to the DMC.
 *
 * @param   comp_id Unique number for which the device topology requires
 * @return  dev_topology_t *    When comp_id is less than device count
 * @return  NULL                When comp_id is greater than or equals to
 *                              device count
 */
const dev_topology_t *Cy_App_Dmc_GetDeviceTopology(uint8_t comp_id);

/**
 * @brief Fetches the current DMC state
 * @return Current DMC state
 */
cy_en_dmc_state_t Cy_App_Dmc_GetCurState (void);

/**
 * @brief Changes the state of the DMC to the given state
 * @param state New DMC state
 */
void Cy_App_Dmc_SetCurState (cy_en_dmc_state_t state);

/** @cond DOXYGEN_HIDE */
/**
 * @brief To know whether DMC supports signed firmware or not
 * @return True when signed firmware update is supported; otherwise false.
 */
bool Cy_App_Dmc_SignedFwUpdateSupported(void);
/** @endcond */

/**
 * @brief Returns whether DMC version check is enabled or not
 * @return True when version check is enabled; otherwise false.
 */
bool Cy_App_Dmc_VersionCheckEnabled(void);

/**
 * @brief This function returns pointer to the structure holding function pointers of various device modules
 * @return Pointer to the structure holding
 * function pointers of various device modules
 */
const cy_stc_app_dmc_update_opern_t** Cy_App_Dmc_GetDevUpdateOpern(void);

/**
 * @brief Returns the alternate image metadata row number
 * @return uint16_t Alternate image metadata row number
 */
uint16_t Cy_App_Dmc_GetAltImgMdRowNum(void);

#if CY_APP_USR_DEFINED_SN_SUPPORT
/**
 * @brief This function returns pointer to the DMC parameters
 * @return Pointer to the DMC parameters
 */
cy_stc_dmc_params_t* Cy_App_Dmc_GetDmcParamPtr(void);
#endif /* CY_APP_USR_DEFINED_SN_SUPPORT */

/**
 * @brief This function used to set the port enabled status in DMC parameters
 * @param val port enabled status value to be updated
 */
void Cy_App_Dmc_SetPortEnabledStatus(uint8_t val);

/**
 * @brief This function used to set SPI flash metadata erase flag.
 */
void Cy_App_Dmc_SetSpiMdEraseFlag(void);

/**
 * @brief This function used to check whether FW update is in progress.
 * @return bool FW update progress status.
 */
bool Cy_App_Dmc_IsFwuInprogress(void);
/** \} group_pmg_app_common_dmc_functions */
/** \} group_pmg_app_common_dmc */

#endif /* (CY_APP_DMC_ENABLE || DOXYGEN) */
#endif /* CY_APP_DMC_FW_UPDATE_H */

/* [] END OF FILE */
