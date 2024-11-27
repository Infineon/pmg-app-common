/***************************************************************************//**
* \file cy_app_system.c
* \version 2.0
*
* \brief
* Support functions for bootloader and flash updates
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "cy_app_flash_config.h"
#include "cy_app_status.h"
#include "cy_app_boot.h"
#include "cy_app_system.h"
#include "cy_app_flash.h"
#include "cy_app_config.h"

#if (!CCG_SROM_CODE_ENABLE)

/* Invalid FW version */
uint8_t gl_invalid_version[8] = {0};

/* Variable representing the current firmware mode */
cy_en_sys_fw_mode_t gl_active_fw = CY_APP_SYS_FW_MODE_INVALID;

#endif /* (!CCG_SROM_CODE_ENABLE) */

ATTRIBUTES void Cy_App_Sys_SetDeviceMode(cy_en_sys_fw_mode_t fw_mode)
{
    GET_IN_VARIABLE(gl_active_fw) = fw_mode;
}    

ATTRIBUTES cy_en_sys_fw_mode_t Cy_App_Sys_GetDeviceMode(void)
{
#if (CY_APP_BOOT_ENABLE == 1)
    return CY_APP_SYS_FW_MODE_BOOTLOADER;
#else /* Firmware */
    return (GET_IN_VARIABLE(gl_active_fw));
#endif /* CY_APP_BOOT_ENABLE */
}

#if !CY_APP_FIRMWARE_APP_ONLY
ATTRIBUTES static uint8_t* sys_get_fw_version(cy_stc_sys_fw_metadata_t *fw_metadata)
{
    /*
     * FW stores its version at an offset of CY_PD_FW_VERSION_OFFSET
     * from its start address. Read the version from that location.
     * The start address of FW is determined from the boot_last_row
     * field of the FW metadata.
     */
    uint32_t addr = ((fw_metadata->fw_start) + CY_APP_SYS_FW_VERSION_OFFSET);
    return (uint8_t *)addr;
}
#endif /* CY_APP_FIRMWARE_APP_ONLY */

ATTRIBUTES uint8_t* Cy_App_Sys_GetBootVersion(void)
{
    return (uint8_t *)CY_APP_SYS_BOOT_VERSION_ADDRESS;
}

ATTRIBUTES uint8_t* Cy_App_Sys_GetImg1FwVersion(void)
{
#if !CY_APP_FIRMWARE_APP_ONLY
    /* Check if image 1 is valid */
    if ((CALL_OUT_FUNCTION(Cy_App_Boot_GetBootModeReason)()).status.fw1_invalid == 0)
    {
        return sys_get_fw_version (GET_IN_VARIABLE(gl_img1_fw_metadata));
    }
    else
    {
        return (GET_IN_VARIABLE(gl_invalid_version));
    }
#else
    return (GET_IN_VARIABLE(gl_invalid_version));
#endif /* CY_APP_FIRMWARE_APP_ONLY */
}

#if (!CY_APP_DUALAPP_DISABLE)    
ATTRIBUTES uint8_t* Cy_App_Sys_GetImg2FwVersion(void)
{
#if !CY_APP_FIRMWARE_APP_ONLY
    /* Check if image 2 is valid */
    if ((CALL_OUT_FUNCTION(Cy_App_Boot_GetBootModeReason)()).status.fw2_invalid == 0)
    {
        return sys_get_fw_version (GET_IN_VARIABLE(gl_img2_fw_metadata));
    }
    else
    {
        return (GET_IN_VARIABLE(gl_invalid_version));
    }
#else
    return (GET_IN_VARIABLE(gl_invalid_version));
#endif /* CY_APP_FIRMWARE_APP_ONLY */
}
#endif /* (!CY_APP_DUALAPP_DISABLE) */

ATTRIBUTES uint32_t Cy_App_Sys_GetFwImg1StartAddr(void)
{
#if !CY_APP_FIRMWARE_APP_ONLY
    /* Check if image 1 is valid */
    if (((CALL_OUT_FUNCTION(Cy_App_Boot_GetBootModeReason)()).status.fw1_invalid) != 1)
    {
        return (gl_img1_fw_metadata->fw_start);
    }
    else
    {
        /* Image 1 is invalid. Can't use MD row info. Use boot last row info
         * from flash configuration and response. */
        return ((CY_APP_BOOT_LOADER_LAST_ROW << CY_APP_SYS_FLASH_ROW_SHIFT_NUM)
                + CY_APP_SYS_FLASH_ROW_SIZE);
    }
#else
    return CY_APP_SYS_INVALID_FW_START_ADDR;
#endif /* CY_APP_FIRMWARE_APP_ONLY */
}

#if (!CY_APP_DUALAPP_DISABLE) 
ATTRIBUTES uint32_t Cy_App_Sys_GetFwImg2StartAddr(void)
{
#if !CY_APP_FIRMWARE_APP_ONLY
    /* Check if image 2 is valid */
    if (((CALL_OUT_FUNCTION(Cy_App_Boot_GetBootModeReason)()).status.fw2_invalid) != 1)
    {
        return (gl_img2_fw_metadata->fw_start);
    }
    else
    {
        /* Image 2 is invalid. Can't use MD row info. Use last row FW1 info
         * from flash configuration and response. */
        return ((CY_APP_IMG1_LAST_FLASH_ROW_NUM << CY_APP_SYS_FLASH_ROW_SHIFT_NUM)
                + CY_APP_SYS_FLASH_ROW_SIZE);
    }
#else
    return CY_APP_SYS_INVALID_FW_START_ADDR;
#endif /* CY_APP_FIRMWARE_APP_ONLY */
}

ATTRIBUTES uint8_t Cy_App_Sys_GetRecentFwImage(void)
{
#if (CY_APP_PRIORITY_FEATURE_ENABLE == 1)
    /* Read APP priority field to determine which image gets the priority */
    uint8_t app_priority;

    app_priority = *((uint8_t *)(CY_APP_SYS_APP_PRIORITY_ROW_NUM << CY_APP_SYS_FLASH_ROW_SHIFT_NUM));
    if (app_priority != 0)
    {
        if (app_priority == 0x01)
            return CY_APP_SYS_FW_MODE_FWIMAGE_1;
        else
            return CY_APP_SYS_FW_MODE_FWIMAGE_2;
    }
    else
#endif /* !CY_APP_PRIORITY_FEATURE_ENABLE */
    {
        /* Prioritize FW2 over FW1 if no priority is defined */
        if ((GET_IN_VARIABLE(gl_img2_fw_metadata))->boot_seq >= (GET_IN_VARIABLE(gl_img1_fw_metadata))->boot_seq)
            return CY_APP_SYS_FW_MODE_FWIMAGE_2;
        else
            return CY_APP_SYS_FW_MODE_FWIMAGE_1;
    }
}
#endif /* (!CY_APP_DUALAPP_DISABLE) */

ATTRIBUTES void Cy_App_Sys_GetSiliconId(uint32_t *silicon_id)
{
#if CY_APP_SILICON_ID
    /* Read silicon ID. */
    *silicon_id = SFLASH_SILICON_ID;
#else
    *silicon_id = 0;
#endif /* CY_APP_SILICON_ID */
}

ATTRIBUTES uint8_t Cy_App_Sys_SetSiliconRevision(void)
{
#if CY_APP_SILICON_ID
    uint8_t srev;

    /* Silicon revision is stored in core-sight tables.
     * Major revision = ROMTABLE_PID2(7:4)
     * Minor revision = ROMTABLE_PID3(7:4) */
    srev = ROMTABLE_PID2 & 0xF0;
    srev |= (ROMTABLE_PID3 & 0xF0) >> 4;
    return srev;
#else
    return 0;
#endif /* CY_APP_SILICON_ID */
}

ATTRIBUTES uint32_t Cy_App_Sys_GetCustomInfoAddr(void)
{
    uint32_t addr;

    /* Get the FW start address based on the FW image */
#if (CY_APP_DUALAPP_DISABLE)
    addr = CALL_IN_FUNCTION(Cy_App_Sys_GetFwImg1StartAddr)();
#else /* (!CY_APP_DUALAPP_DISABLE) */
    if (CALL_IN_FUNCTION(Cy_App_Sys_GetDeviceMode)() == CY_APP_SYS_FW_MODE_FWIMAGE_1)
    {
        addr = CALL_IN_FUNCTION(Cy_App_Sys_GetFwImg1StartAddr)();
    }
    else
    {
        addr = CALL_IN_FUNCTION(Cy_App_Sys_GetFwImg2StartAddr)();
    }
#endif /* (!CY_APP_DUALAPP_DISABLE) */

    return (addr + CY_APP_SYS_FW_CUSTOM_INFO_OFFSET);
}

ATTRIBUTES uint16_t Cy_App_Sys_GetBcdDeviceVersion(uint32_t ver_addr)
{
    /*     
     * bcdDevice version is defined and derived from the FW version as follows:
     * Bit 0:3   = FW_MINOR_VERSION
     * Bit 4:6   = FW_MAJOR_VERSION (lower 3 bits)
     * Bit 7:13  = APP_EXT_CIR_NUM (lower 7 bits)
     * Bit 14:15 = APP_MAJOR_VERSION (lower 2 bits)
     * FW version is stored at a fixed offset in FLASH in this format:
     * 4 Byte BASE_VERSION, 4 Byte APP_VERSION
     */
    
    uint8_t base_major_minor = *((uint8_t *)(ver_addr + 3));
    uint8_t app_cir_num = *((uint8_t *)(ver_addr + 6));
    uint8_t app_major_minor = *((uint8_t *)(ver_addr + 7));
    
    return ((base_major_minor & 0x7F) | ((app_cir_num & 0x7F) << 7)
        | ((app_major_minor & 0x30) << 10));
}

/* [] END OF FILE */
