 /***************************************************************************//**
* \file cy_app_dmc_common.h
* \version 1.0
*
* \brief
* Implements the common definitions and structures used in DMC
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_DMC_COMMON_H_
#define _CY_APP_DMC_COMMON_H_

#if (CY_APP_DMC_ENABLE || DOXYGEN)
/*******************************************************************************
 * Header files including
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include "cy_usbpd_config_table.h"
#include "cy_usb_dev.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_app_status.h"

/**
* \addtogroup group_pmg_app_common_dmc
* \{
* The Dock Management Controller (DMC) library allows users to implement firmware
* update support for variety of devices within a dock.
*
* The DMC library implements the general state machine needed to update the firmware
* of any device in the dock. This library uses the composite dock topology table (CDTT)
* for all the devices information.
*
* The firmware update comprises of 2 phases:
* - Phase 1: Downloading the composite image from host to the SPI flash.
* - Phase 2: Updating the firmware on the individual dock components from the image in 
* the SPI flash.
*
* The DMC library has the following features:
* - Supports unsigned and signed firmware update
* - Handles firmware update related vendor commands
* - Maintains the dock metadata (information related to dock components firmware status)
*
********************************************************************************
* \section section_pmg_app_common_dmc_config configuration considerations
********************************************************************************
*
* The following steps describe the simplest way of enabling the DMC library
* in the application.
*
* 1. Configure the CDTT parameters using EZ-PD&tm; Dock Configuration Utility.
*
* 2. Include cy_app_dmc_fwupdate.h and cy_app_dmc_metadata.h to get access to all
* the functions and other declarations in this library
*    \snippet snippet/dmc_snippet.c snippet_configuration_include
*
* 3. Define the following data structures required by the DMC library:
*    * DMC Configuration parameters
*    \snippet snippet/dmc_snippet.c snippet_configuration_dmc
*    * Register application callback functions
*    \snippet snippet/dmc_snippet.c snippet_configuration_app_cbk
* The DMC library uses this set of callbacks registered by the application
* to access USB and SPI flash hardware interfaces, write to or read from
* internal flash, etc. The library also provides notification of various events
* and DMC state changes.
*
* 4. Initialize the DMC library
*    \snippet snippet/dmc_snippet.c snippet_dmc_init
*
* 5. Invoke the Cy_App_Dmc_Task function from the main processing loop of the
* application to handle the DMC tasks
*    \snippet snippet/dmc_snippet.c snippet_dmc_task
*
********************************************************************************
*
* \defgroup group_pmg_app_common_dmc_macros macros
* \defgroup group_pmg_app_common_dmc_enums enumerated types
* \defgroup group_pmg_app_common_dmc_data_structures data structures
* \defgroup group_pmg_app_common_dmc_functions functions
*/

/**
* \addtogroup group_pmg_app_common_dmc_macros
* \{
*/


/*******************************************************************************
 * MACRO definition - Caution!! Do not edit the macro values in this section
 ******************************************************************************/
/** Maximum device components in the dock supported by DMC
 *  @warning Do not edit this value
 */
#define CY_APP_DMC_MAX_DEV_COUNT                   (8)

/** Multiplier for row size
 *  @warning Do not edit this value
 */
#define CY_APP_DMC_ROW_SIZE_MULT                   (64)

/** Image digest length (SHA-256 digest length)
 *  @warning Do not edit this value
 */
#define CY_APP_DMC_IMAGE_DIGEST_LEN                (32)

/** Size of the version information
 *  @warning Do not edit this value
 */
#define CY_APP_DMC_FW_VERSION_SIZE                 (8)

/** Complete size of all the version information  */
#define CY_APP_DMC_ALL_FW_VERSION_SIZE             (CY_APP_DMC_FW_VERSION_SIZE * 3)

/** Type of trigger - Existing trigger */
#define CY_APP_DMC_EXISTING_TRIGGER                (2u)

/** Type of trigger - New trigger */
#define CY_APP_DMC_NEW_TRIGGER                     (1u)

/** Type of trigger - No trigger */
#define CY_APP_DMC_NO_TRIGGER                      (0u)

/** Signature used to indicate valid CDTT */
#define CY_APP_DMC_CDTT_VALID_SIG                  (0x43)

/** Signature used to indicate DMC to enumerate USB */
#define CY_APP_DMC_USB_ENUM_RQT_SIG                (0x01)

/*
 * Digital signature algorithm specific macros
 */

/** SHA-256/ECDSA
 *  @warning Do not edit this value
 */
#define CY_APP_DMC_DIG_SIG_ALGO_SHA2_ECDSA         (1u)

/** SHA-256/RSA-1024
 *  @warning Do not edit this value
 */
#define CY_APP_DMC_DIG_SIG_ALGO_SHA2_RSA_1024      (2u)

/** SHA-256/RSA-2048
 *  @warning Do not edit this value
 */
#define CY_APP_DMC_DIG_SIG_ALGO_SHA2_RSA_2048      (3u)

/** Specifies the digital signature algorithm supported by the firmware */
#define CY_APP_DMC_DIG_SIG_ALGO_SUPPORTED                   (CY_APP_DMC_DIG_SIG_ALGO_SHA2_RSA_2048)

/** Specifies the signature length for the digital signature (in bytes)
 *  algorithm supported by the firmware
 */
#define CY_APP_DMC_DIG_SIG_ALGO_LEN                          (256)

/** Total number of signatures  */
#define CY_APP_DMC_NUM_OF_SIG_INSTANCE                       (2u)

/** Signature length */
#define CY_APP_DMC_SIG_LEN_SIZE_IN_BIN                       (sizeof(uint16_t))

/** FWCT signature field size */
#define CY_APP_DMC_FWCT_SIG_FIELD_SIZE                       (CY_APP_DMC_SIG_LEN_SIZE_IN_BIN + (CY_APP_DMC_DIG_SIG_ALGO_LEN * CY_APP_DMC_NUM_OF_SIG_INSTANCE))

/*******************************************************************************
 * Other macro definition
 ******************************************************************************/
/** Mask to make number as multiple of 256 */
#define CY_APP_DMC_MUL_OF_256_MASK                           (0xFFFFFF00u)

/** \} group_pmg_app_common_dmc_macros */

/*****************************************************************************
 * Data struct definition
 ****************************************************************************/
/** 
 * \addtogroup group_pmg_app_common_dmc_enums
 * \{ 
 * */
/**
 * @typedef cy_en_dmc_state_t
 * @brief Internal DMC state enumeration.
 */
typedef enum {
    CY_APP_DMC_STATE_WAIT_FOR_START = 0x00,          /**< 0 - Idle state. Waits for the VDR_RQT_UPGRADE_START from host. */
    CY_APP_DMC_STATE_POWER_ON,                       /**< 1 - Default state at which DMC comes up after power on */
    CY_APP_DMC_STATE_PHASE2_FACTORY_BACKUP,          /**< 2 - Checks the validity of the images and initiate factory backup */
    CY_APP_DMC_STATE_PHASE2_FACTORY_BACKUP_COPY,     /**< 3 - Copy image from primary to factory area */
    CY_APP_DMC_STATE_PHASE2_CHECKS,                  /**< 4 - Checks whether Phase 2 update is needed or not */
    CY_APP_DMC_STATE_PHASE2_READ_FWCT,               /**< 5 - Reads the FWCT from SPI flash in Phase 2 */
    CY_APP_DMC_STATE_PHASE2_READ_SIGNATURE,          /**< 6 - Reads the signature from the SPI flash in Phase 2 */
    CY_APP_DMC_STATE_WAIT_FOR_FWCT,                  /**< 7 - Waits for FWCT table from the host - Phase 1 only */
    CY_APP_DMC_STATE_PHASE1_CHECKS,                  /**< 8 - Phase 1 update related checks */
    CY_APP_DMC_STATE_AUTHENTICATING,                 /**< 9 - Supported only for signed FW update - Phase 1 & Phase 2  */
    CY_APP_DMC_STATE_FWCT_ANALYSIS,                  /**< 10 - Compares FWCT in RAM (received from host) against info in dock metadata - Phase 1 & Phase 2 */
    CY_APP_DMC_STATE_VALIDATION_STATUS_UPDATE,       /**< 11 - For sending status due to FWCT analysis failure/Authentication failure - Phase 1 & Phase 2*/
    CY_APP_DMC_STATE_FW_UPDATE_ANALYSIS,             /**< 12 - Compares parsed FWCT in RAM if any pending update and initiate UPDATE_RQT - Phase 1 & Phase 2 */
    CY_APP_DMC_STATE_PREPARE_UPDATE,                 /**< 13 - Prepare the FW update to the particular device - Phase 1 & Phase 2 */
    CY_APP_DMC_STATE_INITIATE_FW_UPGRADE_RQT,        /**< 14 - DMC state to send FW_UPGRADE_RQT data over INT EP - Phase 1 only */
    CY_APP_DMC_STATE_WAIT_FOR_IMG_WRITE,             /**< 15 - Waits for IMG_WRITE vendor command from host - Phase 1 & Phase 2 */
    CY_APP_DMC_STATE_PHASE2_READ_IMG_SEG_INFO,       /**< 16 - Reads image related information like segment size, segment start address from SPI flash for the image - Phase 2 Only */
    CY_APP_DMC_STATE_WAIT_FOR_USB_DATA,              /**< 17 - Waits for one row worth data from the host over USB - Phase 1 only */
    CY_APP_DMC_STATE_PHASE2_READ_IMG_DATA,           /**< 18 - Reads one row of data from SPI flash for the particular image. Phase 2 only */
    CY_APP_DMC_STATE_IMG_WRITE_IN_PROGRESS,          /**< 19 - Image write in progress - Initiates flash write/I2C write as is appropriate - Phase 1 & Phase 2 */
    CY_APP_DMC_STATE_IMG_WRITE_STATUS,               /**< 20 - Sends previous (i.e. most recent) row write status over INT IN EP - Phase 1 & Phase 2 */
    CY_APP_DMC_STATE_WAIT_HPI_STATUS_OR_IDLE,        /**< 21 - Wait for HPI interrupt from CCGx device following the CCGx flash write over HPI, Or IDLE state (temporary waiting state) - Phase 1 only */
    CY_APP_DMC_STATE_PARTIAL_UPDATE_COMPLETE,        /**< 22 - FW update partially completed (all alternate images of all devices updated) - can be successful or failure - Phase 2 only */
    CY_APP_DMC_STATE_SEND_RENUM_NOTIFICATION,        /**< 23 - DMC state to send re-enumeration notification over INT EP - Not used for 2-phase update */
    CY_APP_DMC_STATE_JUMP_TO_ALTERNATE,              /**< 24 - DMC state to initiate jump to alternate image for the particular device - Phase 2 only */
    CY_APP_DMC_STATE_PRE_UPDATE_COMPLETE,            /**< 25 - DMC state to check if any other updates are pending other than DMC updates - Phase 2 only */
    CY_APP_DMC_STATE_FW_UPDATE_COMPLETE,             /**< 26 - FW update completed - can be successful or failure - Phase 1 & Phase 2 */
    CY_APP_DMC_STATE_ERROR_RECOVERY,                 /**< 27 - FW update failed for a particular image/device - Phase 1 & Phase 2 */
    CY_APP_DMC_STATE_FW_UPDATE_FAILED,               /**< 28 - Unexpected FW update failure. Control should not go to this state under normal scenarios - Phase 1 & Phase 2.  */
    CY_APP_DMC_STATE_WAIT_HPI_STATUS_OR_IDLE_2,      /**< 29 - Extra IDLE state to distinguish it from CY_APP_DMC_STATE_WAIT_HPI_STATUS_OR_IDLE - Phase 2 only */
    CY_APP_DMC_STATE_PHASE2_ERROR,                   /**< 30 - Error occurred during Phase 2 update - Phase 2 only */
    CY_APP_DMC_STATE_PHASE1_WAIT_FOR_TRIGGER,        /**< 31 - Wait for trigger vendor command. Phase 1 only. */
    CY_APP_DMC_STATE_WAIT_FOR_SPI_FLASH_OPR_COMPLETE /**< 32 - Wait for SPI flash operation to complete. */
}cy_en_dmc_state_t;

/**
 * @typedef cy_en_dmc_fw_update_status_t
 * @brief Dock status type enumeration. Also used as indication of FW update status.
 * @warning Do not edit this enum value.
 */
typedef enum
{
    CY_APP_DMC_IDLE = 0,                                      /**< 0x00: Dock in idle state: "Idle". State after SWD programming. */
    CY_APP_DMC_PHASE1_UPDATE_IN_PROGRESS,                     /**< 0x01: Dock Phase 1 FW update in progress: "Update In progress" */
    CY_APP_DMC_PHASE1_UPDATE_PARTIAL,                         /**< 0x02: Dock Phase 1 FW update partially completed - in the middle of update cycle: "Update Partial" */
    CY_APP_DMC_RESERVED,                                      /**< 0x03: Reserved */
    CY_APP_DMC_PHASE1_UPDATE_COMPLETE_PARTIAL,                /**< 0x04: Dock Phase 1 FW update process completed, but not all images of all devices are valid. */
    CY_APP_DMC_PHASE1_UPDATE_COMPLETE_FULL_PHASE2_NOT_DONE = 5, /**< 0x05: Dock Phase 1 FW update process completed */

    CY_APP_DMC_PHASE2_UPDATE_IN_PROGRESS = 0x81,              /**< 0x81: Dock Phase 2 FW update in progress: "Update In progress" */
    CY_APP_DMC_PHASE2_UPDATE_PARTIAL,                         /**< 0x82: Dock Phase 2 FW update partially completed - in the middle of update cycle: "Update Partial" */
    CY_APP_DMC_PHASE2_UPDATE_FACTORY_BACKUP,                  /**< 0x83: Dock Phase 2 FW update factory backup started */
    CY_APP_DMC_PHASE2_UPDATE_COMPLETE_PARTIAL,                /**< 0x84: Dock Phase 2 FW update process completed, but not all images of all devices are valid" */
    CY_APP_DMC_PHASE2_UPDATE_COMPLETE_FULL,                   /**< 0x85: Dock Phase 2 FW update process completed */
    CY_APP_DMC_PHASE2_UPDATE_FAIL_INVALID_FWCT,               /**< 0x86: Dock Phase 2 FW update failed due to invalid FWCT */
    CY_APP_DMC_PHASE2_UPDATE_FAIL_INVALID_DOCK_IDENTITY,      /**< 0x87: Dock Phase 2 FW update failed due to invalid dock identity */
    CY_APP_DMC_PHASE2_UPDATE_FAIL_INVALID_COMPOSITE_VER,      /**< 0x88: Dock Phase 2 FW update failed due to invalid composite version */
    CY_APP_DMC_PHASE2_UPDATE_FAIL_AUTHENTICATION_FAILED,      /**< 0x89: Dock Phase 2 FW update failed due to authentication failure */
    CY_APP_DMC_PHASE2_UPDATE_FAIL_INVALID_ALGORITHM,          /**< 0x8A: Dock Phase 2 FW update failed due to invalid signature algorithm */
    CY_APP_DMC_PHASE2_UPDATE_FAIL_SPI_READ_FAILED,            /**< 0x8B: Dock Phase 2 FW update failed due to SPI flash read operation failure */
    CY_APP_DMC_PHASE2_UPDATE_FAIL_NO_VALID_KEY,               /**< 0x8C: Dock Phase 2 FW update failed due to invalid key */
    CY_APP_DMC_PHASE2_UPDATE_FAIL_NO_VALID_SPI_PACKAGE,       /**< 0x8D: Dock Phase 2 FW update failed due to no valid package in SPI flash */
    CY_APP_DMC_PHASE2_UPDATE_FAIL_RAM_INIT_FAILED,            /**< 0x8E: Dock Phase 2 FW update failed due to ram buffer initialization failure */
    CY_APP_DMC_PHASE2_UPDATE_FAIL_FACTORY_BACKUP_FAILED,      /**< 0x8F: Dock Phase 2 FW update failed due to factory backup failure */
    CY_APP_DMC_PHASE2_UPDATE_FAIL_NO_VALID_FACTORY_PACKAGE,   /**< 0x90: Dock Phase 2 FW update failed due to no valid factory package */
    CY_APP_DMC_PHASE1_UPDATE_FAIL = 0xFF                      /**< Dock Phase 1 FW update failed */
}cy_en_dmc_fw_update_status_t;

/**
 * @typedef cy_en_dmc_fwct_status_t
 * @brief FWCT analysis status enumeration
 * @warning Do not edit this enum value
 */
typedef enum
{
    CY_APP_DMC_FWCT_INVALID_FWCT = 0,        /**< Invalid FWCT */
    CY_APP_DMC_FWCT_INVALID_DOCK_IDENTITY,   /**< Invalid dock identity */ 
    CY_APP_DMC_FWCT_INVALID_COMPOSITE_VER,   /**< Invalid composite version */
    CY_APP_DMC_AUTHENTICATION_FAILED,        /**< Authentication failure */
    CY_APP_DMC_INVALID_ALGORITHM             /**< Invalid authentication algorithm */

}cy_en_dmc_fwct_status_t;

/**
 * @typedef cy_en_dmc_img_status_t
 * @brief Dock image status enumeration
 * Values can range from 0 - 0x0F (as it represents only a nibble)
 * @warning Do not edit this enum value
 */
typedef enum
{
    CY_APP_DMC_IMG_STATUS_UNKNOWN = 0,     /**< Dock device image status is not known */
    CY_APP_DMC_IMG_STATUS_VALID,           /**< Dock device image status is valid */
    CY_APP_DMC_IMG_STATUS_INVALID,         /**< Dock device image status is Invalid */
    CY_APP_DMC_IMG_STATUS_RECOVERY,        /**< Dock device image is recovered */
    CY_APP_DMC_IMG_STATUS_RECOVERED_FROM_SECONDARY, /**< Dock device primary image status when it is recovered from secondary image. */
    CY_APP_DMC_IMG_STATUS_NOT_SUPPORTED = 0xF /**< Dock device image status is not supported. */
}cy_en_dmc_img_status_t;

/**
 * @typedef cy_en_dmc_dev_type_t
 * @brief Dock device type enumeration. Do not use values reserved below.
 * For any new device types, use values >= 0x40.
 */
typedef enum
{
    CY_APP_DMC_DEV_TYPE_INVALID = 0,       /**< 0x00: Dock controller can't recognize the device */
    CY_APP_DMC_DEV_TYPE_CCG3,              /**< 0x01: Dock device type is CCG3 */
    CY_APP_DMC_DEV_TYPE_DMC_CY7C65219,     /**< 0x02: Dock device type is CY7C65219 */
    CY_APP_DMC_DEV_TYPE_CCG4,              /**< 0x03: Dock device type is CCG4 */
    CY_APP_DMC_DEV_TYPE_CCG5,              /**< 0x04: Dock device type is CCG5 */
    CY_APP_DMC_DEV_TYPE_HX3,               /**< 0x05: Dock device type is HX3 */
    CY_APP_DMC_DEV_TYPE_RESERVED_06,       /**< 0x06: Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_07,       /**< 0x07: Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_08,       /**< 0x08: Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_09,       /**< 0x09: Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_HX3PD_HUB,         /**< 0x0A: Dock device type is HX3PD Hub controller */
    CY_APP_DMC_DEV_TYPE_HX3PD_PD,          /**< 0x0B: Dock device type is HX3PD PD controller */
    CY_APP_DMC_DEV_TYPE_RESERVED_0C,       /**< 0x0C: Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_CCG2,              /**< 0x0D: Dock device type is CCG2 */
    CY_APP_DMC_DEV_TYPE_TR_TBT_CTRL,       /**< 0x0E: Dock device type is Titan Ridge TBT controller */
    CY_APP_DMC_DEV_TYPE_RESERVED_0F,       /**< 0x0F: Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_10,       /**< 0x10: Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_11,       /**< 0x11: Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_CCG5C = 0x12,      /**< 0x12: Dock device type is CCG5C */
    CY_APP_DMC_DEV_TYPE_CCG6,              /**< 0x13: Dock device type is CCG6 */
    CY_APP_DMC_DEV_TYPE_RESERVED_14,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_15,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_16,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_CCG5_TBT,          /**< 0x17: Dock device type is CCG5 for TBT */
    CY_APP_DMC_DEV_TYPE_CCG5C_TBT,         /**< 0x18: Dock device type is CCG5C for TBT */
    CY_APP_DMC_DEV_TYPE_RESERVED_19,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_1A,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_1B,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_1C,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_GOSHEN_CTRLR,      /**< Dock device type is dock device type is Goshen Ridge controller */
    CY_APP_DMC_DEV_TYPE_FXVL,              /**< Dock device type is Foxville controller */
    CY_APP_DMC_DEV_TYPE_RESERVED_1F,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_20,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_21,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_22,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_23,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_24,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_25,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_26,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_27,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_28,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_29,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_2A,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_2B,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_2C,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_2D,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_2E,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_2F,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_30,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_31,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_32,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_33,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_34,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_35,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_36,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_37,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_38,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_39,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_3A,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_3B,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_3C,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_3D,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_3E,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_RESERVED_3F,       /**< Dock device type is reserved for future use */
    CY_APP_DMC_DEV_TYPE_DMC_PMG1S3 = 0xF0, /**< Dock device type is PMG1S3 */
    CY_APP_DMC_DEV_TYPE_CCG7SC,            /**< Dock device type is CCG7SC */
    CY_APP_DMC_DEV_TYPE_SPI_COMPONENT = 0xFF,       /**< 0xFF: Dock device type is SPI component */
}cy_en_dmc_dev_type_t;

/**
 * @typedef cy_en_dmc_img_mode_t
 * @brief Image type enumeration for DMC and devices connected to DMC
 */
typedef enum
{
    CY_APP_DMC_IMG_MODE_SINGLE_IMG = 0,    /**< Indicates that the device has a single
                                     *   image
                                     */
    CY_APP_DMC_IMG_MODE_DUAL_IMG_SYM,      /**< The device supports symmetric boot.
                                     *   In symmetric mode the bootloader boots
                                     *   the image with higher version, when
                                     *   they are valid.
                                     *
                                     *   DMC uses this booting mechanism.
                                     */
    CY_APP_DMC_IMG_MODE_DUAL_IMG_ASYM      /**< The device supports asymmetric boot.
                                     *   Image 1 and 2 can be different or same.
                                     *   In this method bootloader is hard coded
                                     *   to boot the primary image.
                                     *   Secondary acts as recovery.
                                     */

}cy_en_dmc_img_mode_t;

/**
 * @typedef cy_en_dmc_access_type_t
 * @brief Dock device access type enumeration
 */
typedef enum
{
    CY_APP_DMC_ACCESS_TYPE_SELF = 0,          /**< The device connected to DMC does not have any control interface */
    CY_APP_DMC_ACCESS_TYPE_HPI_I2C,           /**< The device connected to DMC is accessed using HPI I2C */
    CY_APP_DMC_ACCESS_TYPE_HUB_I2C,           /**< The device connected to DMC is accessed using HUB I2C */
    CY_APP_DMC_ACCESS_TYPE_SPI,               /**< The device connected to DMC is accessed using SPI */
    CY_APP_DMC_ACCESS_TYPE_UART               /**< The device connected to DMC is accessed using UART */
}cy_en_dmc_access_type_t;

/**
 * @typedef cy_en_dmc_image_type_t
 * @brief List of CCG firmware modes
 * @warning Do not edit this enum value
 */
typedef enum
{
    CY_APP_DMC_IMAGE_TYPE_BOOTLOADER = 0,     /**< Bootloader mode */
    CY_APP_DMC_IMAGE_TYPE_FWIMAGE_1,          /**< Firmware image 1 */
    CY_APP_DMC_IMAGE_TYPE_FWIMAGE_2,          /**< Firmware image 2 */
    CY_APP_DMC_IMAGE_TYPE_INVALID             /**< Invalid value */
} cy_en_dmc_image_type_t;

/**
 * @typedef cy_en_dmc_status_t
 * @brief Internal INT EP STATUS enumeration.
 * Enumeration to hold status codes for all DMC INT EP communication.
 * This allows the success status to have a value of zero.
 * The operation represented by the enum values can be image write
 * (of particular row) or firmware Upgrade; depending on the INT EP opcode.
 * @warning Do not edit this enum value
 */
typedef enum
{
    CY_APP_DMC_STATUS_SUCCESS = 0,         /**< Status code indicating SUCCESS: no malfunctioning/no outstanding request or event */
    CY_APP_DMC_STATUS_IN_PROGRESS,         /**< Status code indicating operation in progress */
    CY_APP_DMC_STATUS_IDLE,                /**< Status code indicating DMC in IDLE state. No operation in progress.  */
    CY_APP_DMC_STATUS_FAIL = 0xFF,         /**< Status code indicating failure of the operation */

}cy_en_dmc_status_t;

/**
 * @typedef cy_en_dmc_fw_mode_t
 * @brief List of DMC firmware modes
 */
typedef enum
{
    CY_APP_DMC_FW_MODE_BOOTLOADER = 0,     /**< Bootloader mode */
    CY_APP_DMC_FW_MODE_FWIMAGE_1,          /**< Firmware image 1 */
    CY_APP_DMC_FW_MODE_FWIMAGE_2,          /**< Firmware image 2 */
    CY_APP_DMC_FW_MODE_INVALID             /**< Invalid value */
} cy_en_dmc_fw_mode_t;

/**
 * @typedef cy_en_dmc_led_mode_t
 * @brief List of LED control modes
 */
typedef enum
{
    CY_APP_DMC_LED_OFF = 0UL,   /**< LED is in OFF state */
    CY_APP_DMC_LED_ON,          /**< LED is in ON state */
    CY_APP_DMC_LED_BLINKING,    /**< LED is in blinking state */
    CY_APP_DMC_LED_BREATHING    /**< LED is in breathing state */
} cy_en_dmc_led_mode_t;

/**
 * @typedef cy_en_spi_flash_erase_t
 * @brief SPI Flash erase block size
 */
typedef enum
{
    CY_APP_DMC_SPI_FLASH_ERASE_4K = 0u, /**< Erase block size = 4K bytes */
    CY_APP_DMC_SPI_FLASH_ERASE_32K,     /**< Erase block size = 32K bytes */
    CY_APP_DMC_SPI_FLASH_ERASE_64K,     /**< Erase block size = 64K bytes */
    CY_APP_DMC_SPI_FLASH_ERASE_CHIP     /**< Erase block size = Entire chip */
} cy_en_spi_flash_erase_t;

/**
 * @typedef cy_en_dmc_app_evt_t
 * @brief Enum of events that are signaled to the application
 */
typedef enum
{
    CY_APP_DMC_EVT_FW_UPDATE_FAIL = 0x00,          /**< Firmware update fail event */
    CY_APP_DMC_EVT_PHASE2_UPDATE_STARTS,           /**< Firmware update start event */
    CY_APP_DMC_EVT_PHASE2_AUTHENTICATION_SUCCESS,  /**< Firmware update authentication success event */
    CY_APP_DMC_EVT_PHASE2_IMAGE_WRITE_SUCCESS,     /**< Firmware update image write success event */
    CY_APP_DMC_EVT_PHASE2_IMAGE_WRITE_FAIL,        /**< Firmware update image write fail event */
    CY_APP_DMC_EVT_PENDING_UPDATES,                /**< Firmware update pending event */
    CY_APP_DMC_EVT_PHASE2_IMG_UPDATE_START,        /**< Firmware update Phase 2 image update start event */
    CY_APP_DMC_EVT_PHASE2_UPDATE_ENDS,             /**< Firmware update Phase 2 update end event */
    CY_APP_DMC_EVT_PHASE2_FACTORY_BACKUP_STARTED,  /**< Firmware update factory backup start event */
    CY_APP_DMC_EVT_PHASE2_DOCK_RESET,              /**< Firmware update dock reset event */
    CY_APP_DMC_EVT_PHASE2_FACTORY_BACKUP_NOT_DONE, /**< Firmware update factory backup incomplete event */
    CY_APP_DMC_EVT_PHASE2_FACTORY_UPDATE_STATUS_FAILED, /**< Firmware update factory backup update fail event */
    CY_APP_DMC_EVT_DMC_STATE_CHANGE,               /**< Firmware update state change event */
}cy_en_dmc_app_evt_t;

/** \} group_pmg_app_common_dmc_enums */
/** 
 * \addtogroup group_pmg_app_common_dmc_data_structures 
 * \{ 
 * */
/**
 * @brief Internal DMC structure for storing device's firmware version (bootloader, image 1, image 2).
 */
typedef struct
{
    uint8_t bl_version[CY_APP_DMC_FW_VERSION_SIZE];      /**< FW version of bootloader for the device */
    uint8_t img1_version[CY_APP_DMC_FW_VERSION_SIZE];    /**< FW version of image1 for the device */
    uint8_t img2_version[CY_APP_DMC_FW_VERSION_SIZE];    /**< FW version of image2 for the device */

}cy_stc_app_dmc_fw_version_t;

/**
 * @brief Internal DMC structure for device status within the dock metadata
 */
typedef struct
{
    uint8_t             dev_type;              /**< Device type of the device */
    uint8_t             comp_id;               /**< Component ID of the device */
    uint8_t             image_mode;            /**< Image mode of the device - single image/dual symmetric/dual asymmetric image */
    cy_en_dmc_image_type_t        cur_img;               /**< Current running image */
    uint8_t             img_status;            /**< Image status
                                                    * B7:B4 => Image 2 status
                                                    * B3:B0 => Image 1 status
                                                    * Refer cy_en_dmc_img_status_t for values.
                                                    */
    uint8_t             update_attempt_count;  /**< Status indicating if the update is the first attempt in this update sequence or
                                                    * if it was attempted before
                                                    * B7:B4 => Image 2 update attempt counts
                                                    * B3:B0 => Image 1 status attempt counts
                                                    *  0 = Fresh update/first attempt
                                                    *  2-0xF = Count of attempts done to update the image prior to this
                                                    */
    uint8_t             dev_specific[2];       /**< Contains device-specific information
                                                    * If the device type is DMC or HX3, then byte 0 can be decoded as below.
                                                    * For other device these bits are don't care.
                                                    *   B2:B1 : SN Validation bit
                                                    *       0x00 => SN not implemented
                                                    *       0x01 => SN Valid
                                                    *       0x02 => SN Invalid
                                                    * If the device type is DMC_DEV_TYPE_MST_HUB_TYPE1,
                                                    * dev_specific [0] is used to indicate update failure, if any as below. This is needed to identify if any of the image update (out of the 2) failed
                                                    * as one of the images will always be valid.
                                                    * 0x00 = UPDATE_SUCCESSFUL
                                                    * 0xFF = UPDATE_FAILED
                                                    */
    cy_stc_app_dmc_fw_version_t    fw_version;            /**< Complete firmware version */
}cy_stc_app_dmc_devx_status_t;

/**
 * @brief Internal DMC structure for dock status stored in DMC flash. This structure is used to report dock status to
 * the host.
 */
typedef struct
{
    cy_en_dmc_fw_update_status_t       dock_status;                /**< Overall status of dock */
    uint8_t             dev_count;                  /**< Device count */
    uint16_t            status_length;              /**< Length of status bytes including dock_status,
                                                          devx_status for each device. */
    uint32_t            composite_ver;              /**< Dock composite version info */
    cy_stc_app_dmc_devx_status_t   devx_status[CY_APP_DMC_MAX_DEV_COUNT]; /**< FW Status of device of interest */
}cy_stc_app_dmc_dock_status_t;

/**
 * @brief Custom information structure
 */
typedef union
{
    /** misc_status miscellaneous status */
    struct
    {
        uint8_t     trigger_phase2              :   2; /**< Trigger Phase 2 */
        uint8_t     consider_critical_update    :   1; /**< This shall be set only when the primary package is updated by Phase 1 update */
        uint8_t     candidate_package           :   2; /**< Candidate package */
        uint8_t     force_update_flag           :   1; /**< Force update flag */
        uint8_t     read_dock_id                :   1; /**< Read dock ID */
        uint8_t     reserved                    :   1; /**< Reserved byte */
        uint8_t     reserved_byte               :   8; /**< Reserved bytes */
    } misc_status;

    uint16_t data;  /**< Miscellaneous status value in unsigned integer */

} cy_stc_app_dmc_cust_info_t;

/**
 * @brief Internal DMC structure for dock metadata stored in DMC flash
 */
typedef struct
{
    uint16_t            signature;                  /**< Dock metadata signature */
    uint16_t            length;                     /**< Dock metadata length */
    uint32_t            checksum;                   /**< Dock metadata checksum */
    bool                secured_only;               /**< Indicates DMC firmware supports only signed update */
    uint8_t             update_counter;             /**< Dock metadata update counter. Reset to 0 on power up */
    cy_stc_app_dmc_cust_info_t         app_status;                 /**< Application specific status info to be stored in the flash */
    uint8_t             phase2_update_counter;      /**< Phase 2 update counter. Reset to 0 when Phase 2 update when Phase 2 update completes. */
    uint8_t             reserved_0[6];              /**< Padding */
    uint8_t             soft_reset_enum;            /**< Flag to set soft reset */
    uint32_t            cdtt_checksum;              /**< Checksum for CDTT data (config space information) */
    cy_stc_app_dmc_dock_status_t   status;                     /**< Dock status */
    uint8_t             reserved_1[96];             /**< Reserved */
}cy_stc_app_dmc_dock_metadata_t;

/**
 *  @brief Hardware interface configuration data structure for specific devices
 */
typedef struct
{
    CySCB_Type *base;                   /**< Base address of the SCB instance */
    void const *config;                 /**< Pointer to the SCB configuration */
    void *context;                      /**< Pointer to the SCB context */
    IRQn_Type intrSrc;                  /**< Interrupt source */
    uint8_t reserved[3];                /**< Reserved for future use */
}cy_stc_app_dmc_dev_hw_cfg_t;

/**
 *  @brief Holds the parameters such as I2C slave address, GPIO's information
 *         for reset, interrupt and write protect purpose.
 */
typedef union
{
    uint8_t val[8];                     /**< Access parameter value in unsigned integer */

    /** HPI I2C Specific Parameters */
    struct cy_stc_app_hpi_i2c_param
    {
        uint8_t slave_addr;            /**< Type-C upstream device slave
                                        *   address for HPI communication
                                        */
        uint8_t hpi_intr_gpio;         /**< GPIO pin that is being used in DMC
                                        *   for receiving HPI interrupts from type-C
                                        *   device
                                        */
        uint8_t reserved[6];            /**< Reserved for future usage */
    } hpi_i2c_param;                    /**< Access param interpreted as HPI */

    /** HUB I2C Specific parameters */
    struct cy_stc_app_hub_i2c_param
    {
        uint8_t slave_addr;             /**< HX3 HUB slave address for I2C communication */
        uint8_t reset_gpio;             /**< GPIO being used in DMC to reset HX3 Hub */
        uint8_t wp_gpio;                /**< GPIO being used in DMC for write protecting I2C EEPROM */
        uint8_t reserved[5];           /**< Reserved for future usage */
    } hub_i2c_param;                   /**< Access param interpreted as Hub EEPROM I2C access */

    /** HX3PD HUB SPI Specific parameters */
    struct cy_stc_app_hx3pd_hub_spi_param
    {
        uint8_t reset_gpio;             /**< GPIO being used in DMC to reset HX3PD Hub */
        uint8_t wp_gpio;                /**< GPIO being used in DMC for write protecting SPI EEPROM */
        uint8_t reserved[6];            /**< Reserved for future usage */
    } hx3pd_hub_spi_param;              /**< Access param interpreted as HX3PD Hub EEPROM SPI access */
}cy_stc_app_dmc_dev_access_param_t;

/* Forward declarations of structures */
struct cy_stc_dmc_params;

/** @brief Function pointer for usb setup callback
 * @param transfer Pointer to the USB transfer structure
 * @return None
 */
typedef cy_en_usb_dev_status_t (*cy_app_usb_setup_cbk_t) (cy_stc_usb_dev_control_transfer_t *transfer);

/**
 * @brief Structure to hold the application interface. The application is expected to
 * fill the structure with pointers to functions that use the on board circuitry to
 * accomplish tasks.
 *
 * @warning The application must check that the callback pointer passed by the
 * stack is not NULL.
 */
typedef struct
{
    /** Function to perform dock reset */
    void (*init_dock_reset) (void);

    /** Function to check if DMC is in factory condition */
    bool (*is_dmc_in_factory_condition) (void);

    /** Flash enter mode */
    void (*flash_enter_mode) (bool is_enable, bool data_in_place);

    /** Flash row write */
    cy_en_app_status_t (*flash_row_write) (uint16_t row_num, uint8_t *data);

    /** Flash row read */
    cy_en_app_status_t (*flash_row_read) (uint16_t row_num, uint8_t* data);

    /** SPI flash write enable */
    cy_en_app_status_t (*spi_flash_write_enable) (bool enable);

    /** SPI flash write */
    cy_en_app_status_t (*spi_flash_write) (uint8_t *buffer, uint16_t size, uint32_t page_addr, bool retry);

    /** SPI flash read */
    cy_en_app_status_t (*spi_flash_read) (uint8_t *buffer, uint16_t size, uint32_t page_addr, bool retry);

    /** SPI flash erase */
    cy_en_app_status_t (*spi_flash_erase) (uint32_t flash_addr, cy_en_spi_flash_erase_t size);

    /** SPI flash write progress check function */
    bool (*spi_flash_is_write_in_progress) (void);

    /** RSA signature verification function */
    bool (*rsa_verify_signature) (uint8_t* hash, uint16_t hash_len, uint8_t* signature, uint16_t sign_len, uint8_t* publicKey, uint16_t key_len);

    /** USB send status */
    cy_en_usb_dev_status_t (*usb_send_status) (uint8_t ep_index, uint8_t *data, uint8_t size);

    /** USB receive data */
    cy_en_usb_dev_status_t (*usb_receive_data) (uint8_t ep_index, uint8_t *buffer_p, uint8_t *recd_bytes);

    /** EP0 set up read */
    cy_en_usb_dev_status_t (*usb_ep0_setup_read) (uint8_t *data, uint16_t length, cy_app_usb_setup_cbk_t cb);

    /** EP0 set up write */
    cy_en_usb_dev_status_t (*usb_ep0_setup_write) (uint8_t *data, uint16_t length, bool last, cy_app_usb_setup_cbk_t cb);

    /** Function to get EP0 buffer */
    uint8_t* (*usb_get_ep0_buffer) (void);

    /** Firmware update complete handler */
    void (*app_fw_update_complete_handler) (void);

    /** Set led mode */
    void (*led_set_mode) (cy_en_dmc_led_mode_t mode);

    /** Application event handler */
    void (*app_event_handler) (cy_en_dmc_app_evt_t evt, uint8_t *data, uint8_t size);

} cy_stc_dmc_app_cbk_t;

/**
 * @brief Structure to hold DMC parameters
 */
typedef struct cy_stc_dmc_params
{
    /** Pointer to the CDTT config structure */
    const cdtt_config_t *ptrCdttCfg;

    /** Pointer to the security config structure */
    const sec_config_t *ptrSecCfg;

    /** Pointer to USBFS device driver context */
    cy_stc_usbfs_dev_drv_context_t *ptrUsbdrvContext;

    /** Pointer to USBFS device context */
    cy_stc_usb_dev_context_t *ptrUsbdevContext;

    /** Pointer to software timer context */
    cy_stc_pdutils_sw_timer_t *ptrTimerContext;

    /** Pointer to the application callback structure */
    const cy_stc_dmc_app_cbk_t *ptrAppCbk;

    /** SPI flash primary package address */
    uint32_t spiPrimaryPkgAddr;

    /** SPI flash factory package address */
    uint32_t spiFactoryPkgAddr;

    /** Dock composite version */
    uint32_t compsiteVer;

    /** SPI flash operation status check timer ID */
    cy_timer_id_t flashOprTimerId;

    /** Dock reset delay timer ID */
    cy_timer_id_t ResetDelayTimerId;

    /** Hardware interface configuration */
    cy_stc_app_dmc_dev_hw_cfg_t devHwCfg[CY_APP_DMC_MAX_DEV_COUNT];

}cy_stc_dmc_params_t;

/** @brief Function pointer for handling DMC Completion callback
 * @param status Status of completion
 * @return None 
 * */
typedef void (*cy_stc_app_dmc_completion_cbk_t)(cy_en_app_status_t status);

/**
 * @brief Structure holding function pointers of various device modules.
 * This will be filled with comp_id as index - same order as devices are in CDTT.
 * @warning Below listed function pointers should not be left NULL.
 * when new device module is being added else the code will brick or may
 * function in an unpredicted manner.
 * init_dev_param
 * flash_row
 * dev_update_logic
 * skip_jump_to_alt_request
 */
typedef struct
{
    /** Mandatory: Device parameters init handler */
    void (*init_dev_param) (const dev_topology_t *topology, cy_stc_dmc_params_t *params);
    /** Firmware update context init handler */
    void (*init)(const dev_topology_t *topology, cy_stc_dmc_params_t *params);
    /** Firmware update context deinit handler */
    void (*deinit)(cy_stc_dmc_params_t *params);
    /** Firmware update prepare handler */
    cy_en_app_status_t (*prepare_update)(cy_en_dmc_image_type_t img_type, bool *deferred, cy_stc_app_dmc_completion_cbk_t cb, cy_stc_dmc_params_t *params);
    /** Mandatory: Flash row write handler */
    cy_en_app_status_t (*flash_row)(cy_en_dmc_image_type_t img_type, uint16_t row_id, uint8_t *buffer, uint16_t size, bool *deferred, cy_stc_app_dmc_completion_cbk_t cb, cy_stc_dmc_params_t *params);
    /** FW update process finish handler */
    cy_en_app_status_t (*finish_update)(cy_en_dmc_image_type_t img_type, bool flashing_status, bool *deferred, cy_stc_app_dmc_completion_cbk_t cb, cy_stc_dmc_params_t *params);
    /** Mandatory FW version check function */
    bool (*check_fw_version)(uint8_t *img_version, cy_en_dmc_image_type_t img_type, uint8_t comp_id, bool check_last_valid);
    /** Jump to alternate image handler */
    void (*jump_to_alternate)(const dev_topology_t *topology, cy_stc_app_dmc_completion_cbk_t cb, cy_stc_dmc_params_t *params);
    /** API to defer device status query during initialization sequence */
    bool (*is_dev_query_deferred)(void);
    /** API to determine the update logic specific for device module based on the image mode of the device and current running image */
    bool (*dev_update_logic)(uint8_t comp_id, cy_en_dmc_image_type_t img_type, bool *jump_to_alt_request);
    /** API to determine the jump to alternate image request specific for device module based on the image mode of the device and current running image */
    bool (*skip_jump_to_alt_request)(uint8_t comp_id, cy_en_dmc_image_type_t cur_img);

    /** This function should configure the hardware specific for the device
     *  module (like the SCB and GPIOs needed for the device firmware update)
     */
    void (*dev_config_hw_interface)(const dev_topology_t *topology, cy_stc_dmc_params_t *params);
}cy_stc_app_dmc_update_opern_t;
/** \} group_pmg_app_common_dmc_data_structures */

/** \} group_pmg_app_common_dmc */
#endif /* (CY_APP_DMC_ENABLE || DOXYGEN) */
#endif /* _CY_APP_DMC_COMMON_H_ */

/* [] END OF FILE */
