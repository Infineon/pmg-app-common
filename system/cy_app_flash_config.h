/***************************************************************************//**
* \file cy_app_flash_config.h
* \version 2.0
*
* \brief
* Defines flash configuration
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_FLASH_CONFIG_H_
#define _CY_APP_FLASH_CONFIG_H_

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
#define CY_APP_SYS_FLASH_LAST_ROW_NUM                  (CY_FLASH_NUMBER_ROWS - 1)

/* Flash row size. This depends on the device type. Using PSoC&tm; Creator
 * provided MACRO. */
#define CY_APP_SYS_FLASH_ROW_SIZE                      (CY_FLASH_SIZEOF_ROW)

/* Shift value used for multiplying row number with flash row size to
 * get the actual flash address. This depends on the flash row size. Update this
 * field accordingly. */
#if (CY_FLASH_SIZEOF_ROW == 128)
#define CY_APP_SYS_FLASH_ROW_SHIFT_NUM                 (7u)
#elif (CY_FLASH_SIZEOF_ROW == 256)
#define CY_APP_SYS_FLASH_ROW_SHIFT_NUM                 (8u)
#else
#error "Selected device has unsupported flash row size."
#endif /* CY_FLASH_SIZEOF_ROW */ 

/* FW metadata table size in bytes */
#define CY_APP_SYS_METADATA_TABLE_SIZE                 (0x80)

/* FW image 1 metadata row number and address */
#define CY_APP_SYS_IMG1_METADATA_ROW_NUM               (CY_APP_SYS_FLASH_LAST_ROW_NUM)
#define CY_APP_SYS_IMG1_FW_METADATA_ADDR               (((CY_APP_SYS_IMG1_METADATA_ROW_NUM + 1) <<\
            CY_APP_SYS_FLASH_ROW_SHIFT_NUM) - CY_APP_SYS_METADATA_TABLE_SIZE)

/* FW image 2 metadata row number and address */
#define CY_APP_SYS_IMG2_METADATA_ROW_NUM               (CY_APP_SYS_FLASH_LAST_ROW_NUM - 1)
#define CY_APP_SYS_IMG2_FW_METADATA_ADDR               (((CY_APP_SYS_IMG2_METADATA_ROW_NUM + 1) <<\
            CY_APP_SYS_FLASH_ROW_SHIFT_NUM) - CY_APP_SYS_METADATA_TABLE_SIZE)

/* FW image 2 pseudo-metadata row number and address */
#define CY_APP_SYS_IMG2_PSEUDO_METADATA_ROW_NUM        (CY_APP_SYS_FLASH_LAST_ROW_NUM - 2)
#define CY_APP_SYS_IMG2_FW_PSEUDO_METADATA_ADDR        (((CY_APP_SYS_IMG2_PSEUDO_METADATA_ROW_NUM + 1) <<\
            CY_APP_SYS_FLASH_ROW_SHIFT_NUM) - CY_APP_SYS_METADATA_TABLE_SIZE)

/* APP priority row number */
#define CY_APP_SYS_APP_PRIORITY_ROW_NUM                (CY_FLASH_NUMBER_ROWS - 4)

/* Location for storing flash logs */
#define CY_APP_SYS_FLASH_LOG_ADDR                          (((CY_APP_FLASH_LOG_ROW_NUM) << CY_APP_SYS_FLASH_ROW_SHIFT_NUM))

/* Location for storing backup flash logs */
#define CY_APP_SYS_FLASH_LOG_BACKUP_ADDR                          (((CY_APP_FLASH_LOG_BACKUP_ROW_NUM) << CY_APP_SYS_FLASH_ROW_SHIFT_NUM))

#if (CY_APP_DMC_ENABLE || DOXYGEN)

/* Dock metadata size = sizeof (cy_stc_app_dmc_dock_metadata_t) */
#define CY_APP_SYS_DMC_METADATA_SIZE                        0x180

/* Number of dock metadata copies in flash */
#define CY_APP_SYS_DMC_METADATA_NUM                         (2u)

#define CY_APP_SYS_DMC_METADATA_START_ROW_ID                (0x3F0)

#define CY_APP_SYS_DMC_METADATA_END_ROW_ID                  ((CY_APP_SYS_DMC_METADATA_START_ROW_ID + CY_APP_SYS_DMC_METADATA_NUM * (CY_APP_SYS_DMC_METADATA_SIZE / CY_FLASH_SIZEOF_ROW)) - 1)
#endif /* (CY_APP_DMC_ENABLE || DOXYGEN) */

#endif /* _CY_APP_FLASH_CONFIG_H_ */

/* [] END OF FILE */
