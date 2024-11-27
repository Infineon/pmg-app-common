/***************************************************************************//**
* \file cy_app_flash.h
* \version 2.0
*
* \brief
* Defines data structures and function prototypes
* for the flash module
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_FLASH_H_
#define _CY_APP_FLASH_H_

#include <stdint.h>
#include <stdbool.h>
#include "cy_app_status.h"

/*****************************************************************************
* Enumerated data definition
*****************************************************************************/
/**
* \addtogroup group_pmg_app_common_system_enums
* \{
* \defgroup group_pmg_app_common_system_flash_enums flash
* \{
*/
/**
 * @typedef cy_en_flash_app_priority_t
 * @brief Enumeration of app priority values
 *
 * App priority is a debug feature that allows the PMG1 bootloader to prioritize one
 * copy of the firmware over the other. The default behaviour is for both firmware
 * binaries to have the same priority so that the more recently updated firmware
 * binary gets loaded. The priority scheme can be updated to allow FW1 or FW2 to
 * always be loaded for debugging purposes.
 */
typedef enum
{
    CY_APP_FLASH_APP_PRIORITY_DEFAULT = 0,      /**< Default. Latest image gets priority */
    CY_APP_FLASH_APP_PRIORITY_IMAGE_1,          /**< Image 1 gets priority over image 2 */
    CY_APP_FLASH_APP_PRIORITY_IMAGE_2           /**< Image 2 gets priority over image 1 */
} cy_en_flash_app_priority_t;

/**
 * @typedef cy_en_flash_interface_t
 * @brief List of supported flash update interfaces
 *
 * PMG1 supports multiple flash update interfaces such as HPI, UVDM, USB, and so on, 
 * depending on the application. This enumerated type lists the supported flash
 * update interfaces.
 */
typedef enum
{
    CY_APP_FLASH_IF_HPI = 0,                   /**< HPI based flash update interface */
    CY_APP_FLASH_IF_UVDM,                      /**< UVDM based flash update interface */
    CY_APP_FLASH_IF_USB_HID,                   /**< USB flash update interface */
    CY_APP_FLASH_IF_IECS_UART,                 /**< IECS (UART) flash update interface */
} cy_en_flash_interface_t;

/**
 * @typedef cy_en_flash_write_status_t
 * @brief List of possible status codes for non blocking flash write operation
 */
typedef enum
{
    CY_APP_FLASH_WRITE_COMPLETE,               /**< Flash Write successfully completed */
    CY_APP_FLASH_WRITE_ABORTED,                /**< Flash Write aborted */
    CY_APP_FLASH_WRITE_COMPLETE_AND_ABORTED,   /**< Flash Write completed with an abort request */
    CY_APP_FLASH_WRITE_IN_PROGRESS             /**< Flash Write is active */
} cy_en_flash_write_status_t;

/** \} group_pmg_app_common_system_flash_enums */
/** \} group_pmg_app_common_system_enums */

/*****************************************************************************
* Macro definition
*****************************************************************************/


/*****************************************************************************
* Data struct definition
*****************************************************************************/
/**
* \addtogroup group_pmg_app_common_system_data_structures
* \{
* \defgroup group_pmg_app_common_system_flash_data_structures flash
* \{
*/
/**
 * @typedef cy_app_flash_cbk_t
 * @brief Non-blocking flash write row callback function type
 *
 * The PMG1 device supports non-blocking flash update operations, so that the
 * firmware can perform other operations while a flash row write is in progress.
 * The completion of the flash row write is notified through a callback function.
 * This type represents the function type which can be registered as a callback
 * for notification of non-blocking flash operations.
 */
typedef void (*cy_app_flash_cbk_t)(cy_en_flash_write_status_t status);

/** \} group_pmg_app_common_system_flash_data_structures */
/** \} group_pmg_app_common_system_data_structures */

/*****************************************************************************
* Global function declaration
*****************************************************************************/
/**
* \addtogroup group_pmg_app_common_system_functions
* \{
* \defgroup group_pmg_app_common_system_flash_functions flash
* \{
*/

/**
 * @brief Handle ENTER_FLASHING_MODE command
 *
 * This function notifies the PMG1 stack that flash read/write is being enabled
 * by the application. By default, PMG1 firmware disallows all flash read/write
 * operations. Flash access is only allowed after flashing mode has been
 * explicitly enabled through user command.
 *
 * Note: FW update interface is allowed to enable flashing mode only once in one
 * session. This is to ensure that multiple flashing interfaces are not active
 * simultaneously. Each FW update interface is expected to take care of this.
 * Once flash access is complete, this API should be used to exit FW Update
 * interface.
 *
 * @param is_enable Enable/disable flashing mode
 * @param mode Flash update interface to be used
 * @param data_in_place Specifies whether the flash write data buffer can be
 * used in place as SROM API parameter buffer
 *
 * @return None
 */
void Cy_App_Flash_EnterMode(bool is_enable, cy_en_flash_interface_t mode, bool data_in_place);

/**
 * @brief Check whether flashing mode has been entered
 *
 * This function checks whether flashing mode has currently been entered by a
 * different interface
 *
 * @param modes Bitmap containing flashing interfaces to be checked
 *
 * @return Returns true if flashing mode has been entered using any of the
 * interfaces listed in modes, false otherwise.
 */
bool Cy_App_Flash_AccessGetStatus(uint8_t modes);

/**
 * @brief Set limits to the flash rows that can be accessed.
 *
 * The PMG1 stack has been designed to support fail-safe firmware upgrades using
 * a pair of firmware binaries that can mutually update each other. This scheme
 * can only be robust if the currently active firmware binary can effectively
 * protect itself by not allowing access to any of its own flash rows.
 *
 * This function is used to specify the list of flash rows that can safely be
 * accessed by the currently active firmware binary. This function should only
 * be used with parameters derived based on the binary locations identified from
 * firmware metadata. Incorrect usage of this API can cause the device to stop
 * responding during a flash update operation.
 *
 * This API must be invoked as part of initialization before the Cy_App_Flash_RowWrite()
 * and Cy_App_Flash_RowRead() functions can be called. By default, no flash row can be
 * read or written to.
 *
 * @param start_row The lowest row number that can be written
 * @param last_row The highest row number that can be written
 * @param md_row Row containing metadata for the alternate firmware
 * @param bl_last_row Last bootloader row. Rows above this can be read
 *
 * @return None
 */
void Cy_App_Flash_SetAccessLimits (uint16_t start_row, uint16_t last_row, uint16_t md_row, uint16_t bl_last_row);

#if (CY_APP_FLASH_ENABLE_NB_MODE == 1)
/**
 * @brief Handle flash write abort request
 *
 * This API is used by FW update interface to request abort of the ongoing non-blocking 
 * flash write request. This API sets a flag which is sampled in the next SPCIF interrupt
 * and abort sequence is started.
 *
 * @param None
 *
 * @return None
 */
void Cy_App_Flash_NonBlockingWriteAbort(void);
#endif /* CY_APP_FLASH_ENABLE_NB_MODE */

/**
 * @brief Erase the contents of the specified flash row
 *
 * This API erases the contents of the specified flash row by filling it with zeroes.
 * Please note that this API will only work if flashing mode is enabled and the selected
 * row is within the range of write enabled rows.
 *
 * @param row_num Row number to be erased
 *
 * @return Status of row erase operation
 */
cy_en_app_status_t Cy_App_Flash_RowClear(uint16_t row_num);

/**
 * @brief Write the given data to the specified flash row.
 *
 * This API handles the flash write row operation. The contents from the data buffer
 * is written to the row_num flash row. The access rules for the flash row is as
 * same as it is for the Cy_App_Flash_RowClear API.
 *
 * For non-blocking write row operation, the API returns as soon as the row update
 * is started. The stack takes care of executing all of the steps across multiple
 * resume interrupts; and the callback is called at the end of the process.
 *
 * @param row_num Flash row to be updated
 * @param data Buffer containing data to be written to the flash row
 * @param cbk Callback function to be called at the end of non-blocking flash write
 *
 * @return Status of the flash write. CY_APP_STAT_SUCCESS or appropriate error code
 */
cy_en_app_status_t Cy_App_Flash_RowWrite(uint16_t row_num, uint8_t *data, cy_app_flash_cbk_t cbk);

/**
 * @brief Read the contents of the specified flash row
 *
 * This API handles the flash read row operation. The contents of the flash row are
 * copied into the specified data buffer, if flashing mode has been entered and the
 * row_num is part of the readable range of memory.
 *
 * @param row_num Flash row to be read
 * @param data Buffer to read the flash row content into
 *
 * @return Status of the flash read. CY_APP_STAT_SUCCESS or appropriate error code
 */
cy_en_app_status_t Cy_App_Flash_RowRead(uint16_t row_num, uint8_t* data);

#if (CY_APP_BOOT_ENABLE == 0)

/**
 * @brief Updates the app boot priority flag
 *
 * This function is used to set the app priority field to override the default FW selection
 * algorithm
 *
 * @param app_priority Desired boot priority setting
 *
 * @return Status code of the priority update
 */
cy_en_app_status_t Cy_App_Flash_SetAppPriority(cy_en_flash_app_priority_t app_priority);
#endif /*CY_APP_BOOT_ENABLE*/

/** \} group_pmg_app_common_system_flash_functions */
/** \} group_pmg_app_common_system_functions */

#endif /* _CY_APP_FLASH_H_ */

/* [] END OF FILE */

