/***************************************************************************//**
* \file cy_app_boot.h
* \version 2.0
*
* \brief
* Bootloader support header file
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_BOOT_H_
#define _CY_APP_BOOT_H_

#include <stdint.h>
#include <stdbool.h>
#include "cy_app_status.h"
#include "cy_app_system.h"
#include "cy_app_flash.h"
#include "cy_cryptolite_sha.h"

/*****************************************************************************
* MACRO definition
*****************************************************************************/
/**
* \addtogroup group_pmg_app_common_system_macros
* \{
* \defgroup group_pmg_app_common_system_boot_macros bootloader
* \{
*/
/**
 *  @brief Mask for image 1 FW status bit in boot mode reason byte
 */
#define CY_APP_SYS_IMG1_FW_STATUS_BIT_MASK                     (0x08)

/**
 *  @brief Signature used for firmware to indicate boot mode request
 */
#define CY_APP_SYS_BOOT_MODE_RQT_SIG                           (0x424C)

/**
 *  @brief Signature used to indicate boot FW1 request
 */
#define CY_APP_SYS_FW1_BOOT_RQT_SIG                            (0x4231)

/**
 *  @brief Signature used to indicate boot FW2 request
 */
#define CY_APP_SYS_FW2_BOOT_RQT_SIG                            (0x4232)

/**
 *  @brief Firmware boot sequence number offset
 */
#define CY_APP_SYS_FW_METADATA_BOOTSEQ_OFFSET                  (0x14)

/**
 *  @brief Default boot-wait window for bootloader: 50 ms
 */
#define CY_APP_SYS_BL_WAIT_DEFAULT                             (50)

/**
 *  @brief Minimum boot-wait window duration supported: 20 ms
 */
#define CY_APP_SYS_BL_WAIT_MINUMUM                             (20)

/**
 *  @brief Maximum boot-wait window duration supported: 1000 ms
 */
#define CY_APP_SYS_BL_WAIT_MAXIMUM                             (1000)

/**
 *  @brief FW metadata application ID value requesting default boot-wait window
 */
#define CY_APP_SYS_FWMETA_APPID_WAIT_DEF                       (0xFFFFu)

/**
 *  @brief FW metadata application ID value requesting a zero boot-wait window
 */
#define CY_APP_SYS_FWMETA_APPID_WAIT_0                         (0x4359)

/**
 * @brief Configuration table signature used for validation
 */
#define CY_APP_SYS_CONFIG_TABLE_SIGNATURE                      (0x4946)

/**
 * @brief Configuration table size parameter offset
 */ 
#define CY_APP_SYS_CONFIG_TABLE_SIZE_OFFSET                    (6)

/**
 * @brief Configuration table checksum offset
 */
#define CY_APP_SYS_CONFIG_TABLE_CHECKSUM_OFFSET                (8)

/**
 * @brief Configuration table data start offset for checksum calculation
 */
#define CY_APP_SYS_CONFIG_TABLE_CHECKSUM_START                 (12)

/**
 *  @brief Size of HASH in bytes
 */
#define CY_APP_SYS_CRYPTO_SHA_2_HASH_SIZE_BYTES                (0x20)

/** \} group_pmg_app_common_system_boot_macros*/
/** \} group_pmg_app_common_system_macros */

/*****************************************************************************
* Data struct definition
*****************************************************************************/
/**
* \addtogroup group_pmg_app_common_system_data_structures
* \{
* \defgroup group_pmg_app_common_system_boot_data_structures bootloader
* \{
*/
/**
 * @brief Boot mode reason structure
 *
 * This structure holds status of FW images and boot mode request.
 * If the device is running in bootloader mode, this register can be used
 * to identify the reason for this. The register will report the validity of
 * FW1 and FW2 binaries even in the case where the device is already running
 * in FW1 or FW2 mode.
 */
typedef union
{
    uint8_t val;                            /**< Integer field used for direct manipulation of reason code */

    /** status Firmware status bit */
    struct cy_stc_fw_mode_reason_t
    {
        uint8_t boot_mode_request : 1;      /**< Boot mode request made by FW */
        uint8_t reserved          : 1;      /**< Reserved field: Will be zero */
        uint8_t fw1_invalid       : 1;      /**< FW1 image invalid: 0 = Valid, 1 = Invalid */
        uint8_t fw2_invalid       : 1;      /**< FW2 image invalid: 0 = Valid, 1 = Invalid */
        uint8_t reserved1         : 4;      /**< Reserved for later use */
    } status;                               /**< Union containing the status fields in the boot mode reason value */

} cy_stc_fw_img_status_t;

/**
 * @brief Firmware metadata structure
 *
 * This structure defines the format of the firmware metadata that is stored
 * on device flash. The bootloader uses the metadata to identify the firmware validity.
 * location, size, start address etc. The metadata for the two runtime firmware images
 * (FW1 and FW2) are located at fixed addresses, allowing the bootloader
 * to precisely locate and validate the flash content during boot-up.
 */
typedef struct /** @cond DOXYGEN_HIDE */ __attribute__((__packed__)) /** @endcond */
{
    uint32_t fw_start;                  /**< Offset 00: App Fw start address */
    uint32_t fw_size;                   /**< Offset 04: App Fw size */
    uint16_t boot_app_id;               /**< Offset 08: Boot-wait time */
    uint16_t boot_last_row;             /**< Offset 0A: Last flash row of bootloader or previous firmware */
    uint32_t config_fw_start;           /**< Offset 0C: Verify start address */
    uint32_t config_fw_size;            /**< Offset 10: Verify size */
    uint32_t boot_seq;                  /**< Offset 14: Boot sequence number field. Bootloader will load the valid
                                             FW copy that has the higher sequence number associated with it. */
    uint32_t reserved2[15];             /**< Offset 18: Reserved */
    uint16_t metadata_version;          /**< Offset 54: Version of the metadata structure */
    uint16_t metadata_valid;            /**< Offset 56: Metadata valid field. Valid if contains "IF" */
    uint32_t fw_crc32;                  /**< Offset 58: App Fw CRC32 checksum */
    uint32_t reserved3[8];              /**< Offset 5C: Reserved */
    uint32_t md_crc32;                  /**< Offset 7C: Metadata CRC32 checksum */
} cy_stc_sys_fw_metadata_t;

/** \} group_pmg_app_common_system_boot_data_structures */
/** \} group_pmg_app_common_system_data_structures */

/*****************************************************************************
* Global variable declaration
*****************************************************************************/

/**
 *  @brief Pointer to metadata associated with the image 1 FW binary
 */
extern cy_stc_sys_fw_metadata_t *gl_img1_fw_metadata;

#if (!CY_APP_DUALAPP_DISABLE)
/**
 *  @brief Pointer to metadata associated with the image 2 FW binary
 */
extern cy_stc_sys_fw_metadata_t *gl_img2_fw_metadata;
#endif /* (!CY_APP_DUALAPP_DISABLE) */

/*****************************************************************************
* Global function declaration
*****************************************************************************/
/**
* \addtogroup group_pmg_app_common_system_functions
* \{
* \defgroup group_pmg_app_common_system_boot_functions bootloader
* \{
*/ 
/**
 * @brief Validate the configuration table specified
 *
 * Each copy of firmware on the device flash contains an embedded
 * configuration table that defines the runtime behaviour of the device. This
 * function checks whether the configuration table located at the specified location
 * is valid (has valid signature and checksum).
 *
 * @param table_p Pointer to the configuration table to be validated
 *
 * @return CY_APP_STAT_SUCCESS if the table is valid, CY_APP_STAT_FAILURE otherwise
 */
cy_en_app_status_t Cy_App_Boot_ValidateCfgtable(uint8_t *table_p);

/**
 * @brief Validate the firmware image associated with the given metadata
 *
 * This function validates the firmware binary associated with the
 * metadata specified in the fw_metadata parameter. The validity check includes
 * checks for signature, location, size and checksum. This function internally
 * performs validation of the embedded configuration table using the
 * Cy_App_Boot_ValidateCfgtable function.
 *
 * @param fw_metadata Pointer to metadata table of the FW which has to be validated

 * @return CY_APP_STAT_SUCCESS if the firmware is valid, CY_APP_STAT_FAILURE otherwise.
 */
cy_en_app_status_t Cy_App_Boot_ValidateFw(cy_stc_sys_fw_metadata_t *fw_metadata);

/**
 * @brief Handles the VALIDATE_FW command from HPI or UVDM
 *
 * This API handles the VALIDATE_FW command received through the HPI or UVDM interfaces
 *
 * @param fw_mode Firmware binary id: 1 for FW1 and 2 for FW2

 * @return Status code indicating the validity of the firmware
 */
cy_en_app_status_t Cy_App_Boot_HandleValidateFwCmd(cy_en_sys_fw_mode_t fw_mode);

/**
 * @brief Returns the boot-wait delay configured for the application
 *
 * This function identifies the boot-wait delay required by checking the firmware metadata
 *
 * @return Boot-wait delay in milliseconds
 */
uint16_t Cy_App_Boot_GetWaitTime(void);

/**
 * @brief Identify the firmware binary to be loaded
 *
 * This function is only used in the bootloader, and
 * implements the main start-up logic of the bootloader. The function
 * validates the two firmware binaries in device flash, and identifies
 * the binary to be loaded. If neither binary is valid, the function returns
 * false notifying the caller to continue in bootloader mode.
 *
 * @return true if firmware load is allowed; false otherwise.
 */
bool Cy_App_Boot_Start(void);

/**
 * @brief Returns a bitmap containing the reason for boot mode
 *
 * This function returns the bitmap value that is to be stored in the
 * BOOT_MODE_REASON HPI register, which identifies the validity of the
 * two firmware binaries. The validation of the firmware is expected to
 * have been completed earlier through the Cy_App_Boot_Start function. This
 * function only retrieves the status stored during the validation procedure.
 *
 * @see cy_stc_fw_img_status_t
 *
 * @return Boot mode reason bitmap
 */
cy_stc_fw_img_status_t Cy_App_Boot_GetBootModeReason(void);

/**
 * @brief Transfer control to the firmware binary identified by Cy_App_Boot_Start
 *
 * This function is only used by the bootloader. This transfers control to
 * the firmware binary selected as the boot target by the Cy_App_Boot_Start function.
 * This is expected to be called after the boot-wait window has elapsed.
 *
 * @return None
 */
void Cy_App_Boot_JumpToFw(void);

/**
 * @brief Get the boot sequence number value for the specified firmware image.
 *
 * A boot sequence number field stored in the firmware metadata is used by the
 * bootloader to identify the firmware binary to be loaded. This function
 * retrieves the sequence number associated with the specified firmware binary.
 *
 * @param fwid Firmware id whose sequence number is to be retrieved. 1 for FW1
 * and 2 for FW2.
 *
 * @return Boot sequence number value if the firmware is valid; 0 otherwise.
 */
uint32_t Cy_App_Boot_GetBootSeq(uint8_t fwid);

/**
 * @brief Function to validate firmware images and update image status
 *
 * This function is used to validate the firmware images in the device flash
 * and update their status in the image status/boot-mode reason field.
 *
 * @return None
 */
void Cy_App_Boot_UpdateFwStatus(void);

#if (defined (CY_IP_M0S8CRYPTOLITE))
/**
 * @brief Function calculates firmware image hash value using cryptolite APIs
 *
 * @param fw_metadata Pointer to metadata table of the FW which has to be validated
 * @param final_hash Pointer to calculated hash buffer
 * @param sha_ctx Pointer to the SHA context
 *
 * @return CY_APP_STAT_SUCCESS if the hash calculation success; CY_APP_STAT_FAILURE otherwise.
 */
cy_en_app_status_t Cy_App_Boot_CalculateFwImageHash(cy_stc_sys_fw_metadata_t *fw_metadata,
                                                    uint8_t *final_hash,
                                                    cy_stc_cryptolite_sha_context_t *sha_ctx);
#endif /* (defined (CY_IP_M0S8CRYPTOLITE)) */

/** \} group_pmg_app_common_system_boot_functions */
/** \} group_pmg_app_common_system_functions */

#endif /* _CY_APP_BOOT_H_ */

/* [] END OF FILE */
