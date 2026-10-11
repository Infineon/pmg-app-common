/***************************************************************************//**
* \file cy_app_flash_config.h
* \version 3.0
*
* \brief
* Defines flash configuration
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

#ifndef CY_APP_FLASH_CONFIG_H
#define CY_APP_FLASH_CONFIG_H

#include "cy_flash.h"

/******************************************************************************
 * Constant definitions
 *****************************************************************************/

/** CCGx FLASH OPTIONS **/
/* Only the bootloader last row and the configuration table size is expected
 * to change to match the project requirement. With a fixed boot-loader, none
 * of this fields should be modified.
 */

/* Total size of flash */
#define CY_APP_SYS_FLASH_SIZE                          (CY_FLASH_SIZE)

/* Last row number of flash */
#define CY_APP_SYS_FLASH_LAST_ROW_NUM                  (CY_FLASH_NUMBER_ROWS - 1u)

/* Flash row size. This depends on the device type. Using PSoC&tm; Creator
 * provided MACRO. */
#define CY_APP_SYS_FLASH_ROW_SIZE                      (CY_FLASH_SIZEOF_ROW)

/* Shift value used for multiplying row number with flash row size to
 * get the actual flash address. This depends on the flash row size. Update this
 * field accordingly. */
#if (CY_FLASH_SIZEOF_ROW == 128u)
#define CY_APP_SYS_FLASH_ROW_SHIFT_NUM                 (7u)
#elif (CY_FLASH_SIZEOF_ROW == 256u)
#define CY_APP_SYS_FLASH_ROW_SHIFT_NUM                 (8u)
#else
#error "Selected device has unsupported flash row size."
#endif /* CY_FLASH_SIZEOF_ROW */ 

/* FW metadata table size in bytes */
#define CY_APP_SYS_METADATA_TABLE_SIZE                 (0x80u)

/* FW image 1 metadata row number and address */
#define CY_APP_SYS_IMG1_METADATA_ROW_NUM               (CY_APP_SYS_FLASH_LAST_ROW_NUM)
#define CY_APP_SYS_IMG1_FW_METADATA_ADDR               (((CY_APP_SYS_IMG1_METADATA_ROW_NUM + 1u) <<\
            CY_APP_SYS_FLASH_ROW_SHIFT_NUM) - CY_APP_SYS_METADATA_TABLE_SIZE)

/* FW image 2 metadata row number and address */
#define CY_APP_SYS_IMG2_METADATA_ROW_NUM               (CY_APP_SYS_FLASH_LAST_ROW_NUM - 1u)
#define CY_APP_SYS_IMG2_FW_METADATA_ADDR               (((CY_APP_SYS_IMG2_METADATA_ROW_NUM + 1u) <<\
            CY_APP_SYS_FLASH_ROW_SHIFT_NUM) - CY_APP_SYS_METADATA_TABLE_SIZE)

/* FW image 2 pseudo-metadata row number and address */
#define CY_APP_SYS_IMG2_PSEUDO_METADATA_ROW_NUM        (CY_APP_SYS_FLASH_LAST_ROW_NUM - 2u)
#define CY_APP_SYS_IMG2_FW_PSEUDO_METADATA_ADDR        (((CY_APP_SYS_IMG2_PSEUDO_METADATA_ROW_NUM + 1u) <<\
            CY_APP_SYS_FLASH_ROW_SHIFT_NUM) - CY_APP_SYS_METADATA_TABLE_SIZE)

/* APP priority row number */
#define CY_APP_SYS_APP_PRIORITY_ROW_NUM                (CY_FLASH_NUMBER_ROWS - 4u)

/* Location for storing flash logs */
#define CY_APP_SYS_FLASH_LOG_ADDR                          (((CY_APP_FLASH_LOG_ROW_NUM) << CY_APP_SYS_FLASH_ROW_SHIFT_NUM))

/* Location for storing backup flash logs */
#define CY_APP_SYS_FLASH_LOG_BACKUP_ADDR                          (((CY_APP_FLASH_LOG_BACKUP_ROW_NUM) << CY_APP_SYS_FLASH_ROW_SHIFT_NUM))

#if (CY_APP_DMC_ENABLE || DOXYGEN)

/* Dock metadata size = sizeof (cy_stc_app_dmc_dock_metadata_t) */
#define CY_APP_SYS_DMC_METADATA_SIZE                        0x300u

/* Number of dock metadata copies in flash */
#define CY_APP_SYS_DMC_METADATA_NUM                         (2u)

#define CY_APP_SYS_DMC_METADATA_START_ROW_ID                (0x3F0u)

#define CY_APP_SYS_DMC_METADATA_END_ROW_ID                  ((CY_APP_SYS_DMC_METADATA_START_ROW_ID + CY_APP_SYS_DMC_METADATA_NUM * (CY_APP_SYS_DMC_METADATA_SIZE / CY_FLASH_SIZEOF_ROW)) - 1u)

/* Flash row number for Serial Number data. */
#define CY_APP_DMC_SERIAL_NUMBER_ROW_NUM                    (CY_FLASH_NUMBER_ROWS - 3u)

#endif /* (CY_APP_DMC_ENABLE || DOXYGEN) */

#endif /* CY_APP_FLASH_CONFIG_H */

/* [] END OF FILE */
