/***************************************************************************//**
* \file cy_app_system.h
* \version 1.0
*
* \brief
* Support functions and definitions for bootloader and flash updates
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_SYSTEM_H_
#define _CY_APP_SYSTEM_H_

#include <stdint.h>

/*****************************************************************************
* MACRO definition
*****************************************************************************/
/**
* \addtogroup group_pmg_app_common_system
* \{
* <p>The system modules allows user to perform system specific operation. The following
* functionality are provided by this. </p>
* 
* * <b>System</b> \n
*   The system module provides APIs to set/get the firmware mode, version information, etc. \n 
*
* * <b>Bootloader</b> \n
*   The bootloader module provide APIs and data structures to validate and load the 
*   firmware image from flash. \n
*
* * <b>Flash</b> \n
*   The flash APIs allows user to read and write the device flash memory. \n
*
* * <b>Instrumentation</b> \n
*   The instrumentation module provides APIs to handle the device specific 
*   events (for example, watchdog timeout, hard fault). \n
*
********************************************************************************
* \section section_pmg_app_common_system_config configuration considerations
********************************************************************************
*
********************************************************************************
* \subsection section_pmg_app_common_system_system_config system
********************************************************************************
*
* 1. Include cy_app_system.h to get access to all system specific functions and 
* other declarations
*    \snippet snippet/system_sut.c snippet_system_include_system
* 
* 2. Invoke the APIs like the following to perform a system specific operations. Refer the
*    \ref group_pmg_app_common_system_system_functions section for detailed 
*    information about usages and functionality of all other APIs.
*    \snippet snippet/system_sut.c snippet_system_system_functions
*
********************************************************************************
* \subsection section_pmg_app_common_system_boot_config bootloader
********************************************************************************
*
* 1. Include cy_app_boot.h to get access to all bootloader specific functions and 
* other declarations
*    \snippet snippet/system_sut.c snippet_system_include_boot
*
* 2. Define the firmware metadata data structure and firmware image status structure 
*    required by the bootloader APIs. Metadata table is placed at a fixed location
*    for both firmware image-1 and image 2.
*    \snippet snippet/system_sut.c snippet_system_boot_structure
*
* 3. Invoke the APIs to perform firmware validation and load a valid image
*    \snippet snippet/system_sut.c snippet_system_boot_functions
*
* 4. Refer the \ref group_pmg_app_common_system_boot_functions section for detailed 
*    information about usages and functionality of all other APIs
*
********************************************************************************
* \subsection section_pmg_app_common_system_flash_config flash
********************************************************************************
*
* 1. Include cy_app_flash.h to get access to all flash specific functions and 
* other declarations
*    \snippet snippet/system_sut.c snippet_system_include_flash
*
* 2. Call the following API from main to set the flash access limit
*    \snippet snippet/system_sut.c snippet_system_flash_functions_set_limit
*
* 3. Call the enter mode API for enabling flash write operation
*    \snippet snippet/system_sut.c snippet_system_flash_functions_set_mode
*
* 4. Invoke the following API to write data into the flash memory
*    \snippet snippet/system_sut.c snippet_system_flash_functions_write
*
* 5. Refer the \ref group_pmg_app_common_system_flash_functions section for detailed 
*    information about usages and functionality of all other APIs
*
********************************************************************************
* \subsection section_pmg_app_common_system_instrumentation_config instrumentation
********************************************************************************
*
* 1. Include cy_app_instrumentation.h to get access to all instrumentation 
*    functions and other declarations in this library
*    \snippet snippet/system_sut.c snippet_system_include_instrumentation
*
* 2. Invoke the instrumentation init and start from main function
*    \snippet snippet/system_sut.c snippet_system_instrumentation_functions_init
*
* 4. Invoke the instrumentation task from main loop for handling the system faults
*    \snippet snippet/system_sut.c snippet_system_instrumentation_functions_task
*
* 5. Refer the \ref group_pmg_app_common_system_instrumentation_functions section
*    for detailed information about usages and functionality of all other APIs
*
********************************************************************************
*
* \defgroup group_pmg_app_common_system_macros macros
* \defgroup group_pmg_app_common_system_enums enumerated types
* \defgroup group_pmg_app_common_system_data_structures data structures
* \defgroup group_pmg_app_common_system_functions functions
*/
/** \} group_pmg_app_common_system */

#define ATTRIBUTES
#define CALL_OUT_FUNCTION(func_name) func_name
#define CALL_IN_FUNCTION(func_name) func_name
#define GET_IN_VARIABLE(var_name) var_name

/**
* \addtogroup group_pmg_app_common_system_macros
* \{
* \defgroup group_pmg_app_common_system_system_macros system
* \{
*/
/**
 * @brief Bootloader version address in FLASH
 */
#define CY_APP_SYS_BOOT_VERSION_ADDRESS                              (0x000000E0)
    
/**
 * @brief Offset of FW version from the start address of FW image in flash
 */
#define CY_APP_SYS_FW_VERSION_OFFSET                                 (0x000000E0)  

/**
 * @brief Offset of app version from the start of firmware version
 */
#define CY_APP_SYS_APP_VERSION_OFFSET                                 (0x00000004)

/**
 * @brief Offset of silicon ID stored in FW image in flash
 */
#define CY_APP_SYS_SILICON_ID_OFFSET                                 (0x000000EA)

/**
 * @brief Offset of bootloader type field in Bootloader flash region
 */
#define CY_APP_SYS_BOOT_TYPE_FIELD_OFFSET                            (0x000000EC)

/**
 * @brief Offset of Customer Specific Info in flash from FW start address
 */
#define CY_APP_SYS_FW_CUSTOM_INFO_OFFSET                             (0x000000C0)

/**
 * @brief Metadata table valid signature : "CY"
 */
#define CY_APP_SYS_METADATA_VALID_SIG                                (0x4359)

/**
 * @brief Pseudo-Metadata valid signature : "CP"
 */
#define CY_APP_SYS_PSEUDO_METADATA_VALID_SIG                         (0x4350)  

/**
 * @brief Boot mode request signature : "BL"
 */
#define CY_APP_SYS_BOOT_MODE_RQT_SIG                                 (0x424C)
    
/**
 * @brief Configuration table valid signature
 */
#define CY_APP_SYS_CONFIG_TABLE_SIGN                                 (0x4359u)
    
/**
 * @brief Invalid FW start address
 */
#define CY_APP_SYS_INVALID_FW_START_ADDR                             (0x00000000)

/**
 * @brief Bootloader type APP PRIORITY bit position
 */
#define CY_APP_SYS_BOOT_TYPE_APP_PRIORITY_POS                        (0x02)

/**
 * @brief Bootloader type FW update interface bit position
 */
#define CY_APP_SYS_BOOT_TYPE_FW_UPDATE_INTERFACE_POS                 (0x01)

/**
 * @brief Bootloader type secure Boot feature bit mask
 */
#define CY_APP_SYS_BOOT_TYPE_SECURE_BOOT_MASK                        (0x01)

/**
 * @brief Bootloader type FW update interface bit mask
 */
#define CY_APP_SYS_BOOT_TYPE_FW_UPDATE_INTERFACE_MASK                (0x02)

/**
 * @brief Silicon ID bit mask
 */
#define CY_APP_SYS_SILICON_ID_MASK                                   (0xFF00)

/** \} group_pmg_app_common_system_system_macros*/
/** \} group_pmg_app_common_system_macros */

/*****************************************************************************
* Enumerated data definition
*****************************************************************************/
/**
* \addtogroup group_pmg_app_common_system_enums
* \{
* \defgroup group_pmg_app_common_system_system_enums system
* \{
*/
/**
 * @typedef cy_en_sys_fw_mode_t
 * @brief List of firmware modes
 */
typedef enum
{
    CY_APP_SYS_FW_MODE_BOOTLOADER = 0,     /**< Bootloader mode */
    CY_APP_SYS_FW_MODE_FWIMAGE_1,          /**< Firmware image #1 */
    CY_APP_SYS_FW_MODE_FWIMAGE_2,          /**< Firmware image #2 */
    CY_APP_SYS_FW_MODE_INVALID             /**< Invalid value */
} cy_en_sys_fw_mode_t;

/** \} group_pmg_app_common_system_system_enums */
/** \} group_pmg_app_common_system_enums */

/*****************************************************************************
* Global function declaration
*****************************************************************************/
/**
* \addtogroup group_pmg_app_common_system_functions
* \{
* \defgroup group_pmg_app_common_system_system_functions system
* \{
*/    
/**
 * @brief Set the current firmware mode
 *
 * This function is used by the start-up logic to store the current firmware
 * mode for the CCGx device.
 *
 * This should not be used outside of the default start-up logic for the CCGx
 * bootloader and firmware applications.
 *
 * @param fw_mode The active firmware mode to be set
 *
 * @return None
 */
void Cy_App_Sys_SetDeviceMode(cy_en_sys_fw_mode_t fw_mode);

/**
 * @brief Get the current firmware mode
 *
 * This function retrieves the current firmware mode of the CCG device
 *
 * @return The current firmware mode
 */
cy_en_sys_fw_mode_t Cy_App_Sys_GetDeviceMode(void);

/**
 * @brief Get bootloader version
 *
 * The bootloader version is stored at absolute address SYS_CCG_BOOT_VERSION_ADDRESS
 * in device FLASH. This function returns a pointer to this version information.
 * 
 * @return Pointer to the bootloader version information
 */
uint8_t* Cy_App_Sys_GetBootVersion(void);

/**
 * @brief Get version for firmware image 1.
 *
 * This function returns a pointer to the version information for firmware image 1 (FW1).
 * The version is located at a fixed offset of CY_PD_FW_VERSION_OFFSET bytes
 * from the start of the firmware binary.
 *
 * @return Pointer to the firmware image-1 version information
 */
uint8_t* Cy_App_Sys_GetImg1FwVersion(void);

/**
 * @brief Get version for firmware image 2.
 *
 * This function returns a pointer to the version information for firmware image-2 (FW2).
 * The version is located at a fixed offset of CY_PD_FW_VERSION_OFFSET bytes
 * from the start of the firmware binary.
 *
 * @return Pointer to the firmware image 2 version information
 */
uint8_t* Cy_App_Sys_GetImg2FwVersion(void);

/**
 * @brief Get the flash start address of firmware image 1
 *
 * This function returns the flash address from where firmware image 1 (FW1) has
 * been stored.
 *
 * @return Start address of firmware image 1
 */
uint32_t Cy_App_Sys_GetFwImg1StartAddr(void);

/**
 * @brief Get the flash start address of firmware image 2
 *
 * This function returns the flash address from where firmware image 2 (FW2) has
 * been stored
 *
 * @return Start address of firmware image 2
 */
uint32_t Cy_App_Sys_GetFwImg2StartAddr(void);

/**
 * @brief Determines the more recently update firmware image
 *
 * The CCG bootloader uses this function to determine the more recently updated
 * firmware image (from among FW1 and FW2) by comparing the sequence numbers of
 * images which are stored in the firmware metadata table. The bootloader loads
 * the most recently updated binary by default (even if its version is older 
 * than that of the other firmware binary).
 *
 * @return Firmware id: 1 for image 1 and 2 for image 2.
 */
uint8_t Cy_App_Sys_GetRecentFwImage(void);

/**
 * @brief Get silicon ID of device
 *
 * This function retrieves the silicon ID of the CCG device
 * 
 * @param silicon_id Pointer to buffer to hold the silicon ID
 *
 * @return None
 */
void Cy_App_Sys_GetSiliconId(uint32_t *silicon_id);

/**
 * @brief Returns silicon revision
 *
 * @return Silicon revision
 *      B[7:4] - Major rev
 *      B[3:0] - Minor rev
 */
uint8_t Cy_App_Sys_SetSiliconRevision(void);

/**
 * @brief Get start address of customer info section
 *
 * This function returns the start address of customer info section
 *
 * @return Address of customer info section
 */
uint32_t Cy_App_Sys_GetCustomInfoAddr(void);

/**
 * @brief Get bcdDevice version of the device
 *
 * This function returns bcdDevice version for the device which can be used
 * as a part of the D_ID response, secure boot checks etc. Format of bcdDevice version
 * is documented in the function body.
 * 
 * @param ver_addr Offset of version in flash memory
 *
 * @return 16 bits bcdDevice version
 */
uint16_t Cy_App_Sys_GetBcdDeviceVersion(uint32_t ver_addr);

/** \} group_pmg_app_common_system_system_functions */
/** \} group_pmg_app_common_system_functions */

#endif /* _CY_APP_SYSTEM_H_ */
/* [] END OF FILE */