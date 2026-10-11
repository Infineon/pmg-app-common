/***************************************************************************//**
* \file cy_app_boot.c
* \version 3.0
*
* \brief
* Bootloader support functions
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

#include <stdint.h>
#include <stdbool.h>
#include "cy_app_status.h"
#include "cy_app_system.h"
#include "cy_app_flash.h"
#include "cy_app_boot.h"
#include "cy_flash.h"
#include "cy_cryptolite_sha.h"
#include "cy_pdutils.h"
#include "cy_app_flash_config.h"
#include "cy_app_config.h"
#include "cy_app_internal.h"

#define Bootloader_1_MD_BTLDB_ACTIVE_0         0U
#define Bootloader_1_MD_BTLDB_ACTIVE_1         1U

/* Structure to hold reason for boot mode */
#if defined(__ARMCC_VERSION)
CY_SECTION(".bss.cy_boot_img_status") __USED
#else
CY_SECTION(".cy_boot_img_status") __USED
#endif /* defined(__ARMCC_VERSION) */
static volatile cy_stc_fw_img_status_t gl_img_status;

#if (CY_APP_BOOT_ENABLE != 0)
/* If dual-app bootloading is disabled, provide stub variable to keep the compiler happy. */
static volatile uint8_t Bootloader_1_activeApp = 0u;
#endif /* (CY_APP_BOOT_ENABLE != 0) */

#if (CY_APP_BOOT_WAIT_WINDOW_DISABLE == 0)
/* Boot-wait duration specified by firmware metadata */
static volatile uint16_t gl_boot_wait_delay = CY_APP_SYS_BL_WAIT_DEFAULT;
#endif /* (CY_APP_BOOT_WAIT_WINDOW_DISABLE != 0) */
 
/* Pointer to image 1 FW metadata table */
cy_stc_sys_fw_metadata_t *gl_img1_fw_metadata = 
        (cy_stc_sys_fw_metadata_t *)(CY_APP_SYS_IMG1_FW_METADATA_ADDR);

#if (!CY_APP_DUALAPP_DISABLE)
/* Pointer to image 2 FW metadata table */
cy_stc_sys_fw_metadata_t *gl_img2_fw_metadata = 
        (cy_stc_sys_fw_metadata_t *)(CY_APP_SYS_IMG2_FW_METADATA_ADDR);
#endif /* (!CY_APP_DUALAPP_DISABLE) */

cy_stc_fw_img_status_t Cy_App_Boot_GetBootModeReason(void)
{
    /* Return the reason for boot mode */
    return gl_img_status;
}

#define CRC_TABLE_SIZE                      (16U)           /* A number of uint32_t elements in the CRC32 table */
#define CRC_INIT                            (0xFFFFFFFFU)
#define NIBBLE_POS                          (4U)
#define NIBBLE_MSK                          (0xFU)

static uint32_t calculate_crc32(const uint8_t *address, uint32_t length)
{
    /* Contains generated values to calculate CRC-32C by 4 bits per iteration */
    static const uint32_t crcTable[CRC_TABLE_SIZE] =
    {
        0x00000000U, 0x105ec76fU, 0x20bd8edeU, 0x30e349b1U,
        0x417b1dbcU, 0x5125dad3U, 0x61c69362U, 0x7198540dU,
        0x82f63b78U, 0x92a8fc17U, 0xa24bb5a6U, 0xb21572c9U,
        0xc38d26c4U, 0xd3d3e1abU, 0xe330a81aU, 0xf36e6f75U,
    };

    uint32_t crc = CRC_INIT;
    if (length != 0U)
    {
        do
        {
            crc = crc ^ *address;
            crc = (crc >> NIBBLE_POS) ^ crcTable[crc & NIBBLE_MSK];
            crc = (crc >> NIBBLE_POS) ^ crcTable[crc & NIBBLE_MSK];
            --length;
            ++address;
        } while (length != 0U);
    }
    return (~crc);
}

/* Check whether configuration table checksum is good */
cy_en_app_status_t Cy_App_Boot_ValidateCfgtable(uint8_t *table_p)
{
    uint16_t size = CY_PDUTILS_MAKE_WORD (table_p[CY_APP_SYS_CONFIG_TABLE_SIZE_OFFSET + 1u], table_p[CY_APP_SYS_CONFIG_TABLE_SIZE_OFFSET]);

    if (((uint32_t)table_p >= CY_FLASH_SIZE) ||
            (CY_PDUTILS_MAKE_WORD (table_p[1], table_p[0]) != CY_APP_SYS_CONFIG_TABLE_SIGNATURE))
    {
        return CY_APP_STAT_INVALID_FW;
    }

    uint32_t table_crc = CY_PDUTILS_MAKE_DWORD(table_p[CY_APP_SYS_CONFIG_TABLE_CHECKSUM_OFFSET+3u],table_p[CY_APP_SYS_CONFIG_TABLE_CHECKSUM_OFFSET+2u],table_p[CY_APP_SYS_CONFIG_TABLE_CHECKSUM_OFFSET+1u],table_p[CY_APP_SYS_CONFIG_TABLE_CHECKSUM_OFFSET]);

    if (table_crc != calculate_crc32 (
            table_p + CY_APP_SYS_CONFIG_TABLE_CHECKSUM_START, (uint32_t)size - CY_APP_SYS_CONFIG_TABLE_CHECKSUM_START))
    {
        return CY_APP_STAT_INVALID_FW;
    }

    return CY_APP_STAT_SUCCESS;
}

cy_en_app_status_t Cy_App_Boot_ValidateFw(cy_stc_sys_fw_metadata_t *fw_metadata)
{
    cy_en_app_status_t status = CY_APP_STAT_SUCCESS;
    /* Address of FW image start */
    uint32_t fw_start;
    /* Size of FW image in bytes */
    uint32_t fw_size;
   
    fw_start = fw_metadata->fw_start;
    fw_size = fw_metadata->fw_size;
    
    /*
     * Validate:
     * 1) FW size
     * 2) FW checksum
     * 3) FW entry
     */

    if (
            (fw_size == 0u) ||
            ((fw_start + fw_size) >= CY_FLASH_SIZE) ||
             (fw_metadata->fw_crc32 != calculate_crc32((uint8_t *)fw_start, fw_size))
       )
    {
        status = CY_APP_STAT_INVALID_FW;
    }
    else
    {
        /* In an application architecture where the configuration table is not available,
         * the fw_start and config_fw_start addresses will be the same. In this case the 
         * configuration table validation is not required.
         */
        if(fw_metadata->fw_start != fw_metadata->config_fw_start)
        {
            status = Cy_App_Boot_ValidateCfgtable ((uint8_t *)(fw_metadata->config_fw_start));
        }
    }

    return status;
}

cy_en_app_status_t Cy_App_Boot_HandleValidateFwCmd(cy_en_sys_fw_mode_t fw_mode)
{
    cy_stc_sys_fw_metadata_t *md_p = NULL;
    cy_en_app_status_t code = CY_APP_STAT_NO_RESPONSE;

#if (CY_APP_BOOT_ENABLE != 0)
    switch (fw_mode)
    {
        case CY_APP_SYS_FW_MODE_FWIMAGE_1:
            md_p = GET_IN_VARIABLE(gl_img1_fw_metadata);
            break;

#if (!CY_APP_DUALAPP_DISABLE)
        case CY_APP_SYS_FW_MODE_FWIMAGE_2:
            md_p = GET_IN_VARIABLE(gl_img2_fw_metadata);
            break;
#endif /* (!CY_APP_DUALAPP_DISABLE) */

        default:
            code = CY_APP_STAT_INVALID_ARGUMENT;
            break;
    }
#else /* (CY_APP_BOOT_ENABLE != 0) */
    if (fw_mode == CALL_IN_FUNCTION(Cy_App_Sys_GetDeviceMode)())
    {
        /* There is no need to validate the currently running image */
        code = CY_APP_STAT_SUCCESS;
    }
    else
    {
        switch (fw_mode)
        {
            case CY_APP_SYS_FW_MODE_FWIMAGE_1:
                {
                    md_p = GET_IN_VARIABLE(gl_img1_fw_metadata);
                }
                break;

#if (!CY_APP_DUALAPP_DISABLE)
            case CY_APP_SYS_FW_MODE_FWIMAGE_2:
                {
                    md_p = GET_IN_VARIABLE(gl_img2_fw_metadata);
                }
                break;
#endif /* (!CY_APP_DUALAPP_DISABLE) */

            default:
                code = CY_APP_STAT_INVALID_ARGUMENT;
                break;
        }
    }
#endif /* (CY_APP_BOOT_ENABLE != 0) */

    if (md_p != NULL)
    {
        if (Cy_App_Boot_ValidateFw (md_p) == CY_APP_STAT_SUCCESS)
        {
            code = CY_APP_STAT_SUCCESS;
            if(fw_mode == CY_APP_SYS_FW_MODE_FWIMAGE_1)
            {
                gl_img_status.status.fw1_invalid = 0u;
            }
#if (!CY_APP_DUALAPP_DISABLE)
            else if(fw_mode == CY_APP_SYS_FW_MODE_FWIMAGE_2)
            {
                gl_img_status.status.fw2_invalid = 0u;
            }
            else 
            {
                /* No action needed */
            }
#endif /* !CY_APP_DUALAPP_DISABLE */
        }
        else
        {
            code = CY_APP_STAT_INVALID_FW;
            if(fw_mode == CY_APP_SYS_FW_MODE_FWIMAGE_1)
            {
                gl_img_status.status.fw1_invalid = 1u;
            }
#if (!CY_APP_DUALAPP_DISABLE)
            else if(fw_mode == CY_APP_SYS_FW_MODE_FWIMAGE_2)
            {
                gl_img_status.status.fw2_invalid = 1u;
            }
            else
            {
                /* No action needed */
            }
#endif /* !CY_APP_DUALAPP_DISABLE */
        }
    }
    return code;
}

#if (CY_APP_BOOT_ENABLE != 0)
#if (CY_APP_BOOT_WAIT_WINDOW_DISABLE == 0)
/* Return the boot-wait setting to the user code */
uint16_t Cy_App_Boot_GetWaitTime(void)
{
    return (gl_boot_wait_delay);
}

static void boot_set_wait_timeout(cy_stc_sys_fw_metadata_t *md_p)
{
    /* Check for boot-wait option */
    if (md_p->boot_app_id == CY_APP_SYS_FWMETA_APPID_WAIT_0)
    {
        gl_boot_wait_delay = 0u;
    }
    else
    {
        if (md_p->boot_app_id != CY_APP_SYS_FWMETA_APPID_WAIT_DEF)
        {
            /* Get the boot-wait delay from metadata, applying the MIN and MAX limits. */
            gl_boot_wait_delay = CY_PDUTILS_GET_MAX (CY_APP_SYS_BL_WAIT_MAXIMUM, 
                                                     CY_PDUTILS_GET_MIN (CY_APP_SYS_BL_WAIT_MINUMUM,md_p->boot_app_id));
        }
    }
}
#endif /* CY_APP_BOOT_WAIT_WINDOW_DISABLE */

/*This function schedules recent and valid FW. It sets boot mode reason. */
bool Cy_App_Boot_Start(void)
{
    cy_stc_sys_fw_metadata_t *md_p;
    uint8_t img;

    bool boot_fw1 = false;
    bool boot_fw2 = false;

#if (CY_APP_DUALAPP_DISABLE)
    CY_UNUSED_PARAMETER(boot_fw1);
    CY_UNUSED_PARAMETER(boot_fw2);
#endif /* CY_APP_DUALAPP_DISABLE */

    md_p = NULL;
    gl_img_status.val = 0u;
   
    /* Check the two firmware binaries for validity */
    if (Cy_App_Boot_ValidateFw ((cy_stc_sys_fw_metadata_t *)CY_APP_SYS_IMG1_FW_METADATA_ADDR) != CY_APP_STAT_SUCCESS)
    {
        gl_img_status.status.fw1_invalid  = 1u;
    }

#if (!CY_APP_DUALAPP_DISABLE)
    if (Cy_App_Boot_ValidateFw ((cy_stc_sys_fw_metadata_t *)CY_APP_SYS_IMG2_FW_METADATA_ADDR) != CY_APP_STAT_SUCCESS)
#endif /* CY_APP_DUALAPP_DISABLE */
    {
        gl_img_status.status.fw2_invalid = 1u;
    }
    
    /* Check for the boot mode request */
    /*
     * NOTE: cyBtldrRunType is bootloader component provided variable.
     * It is used to store the jump signature. Check the lower two bytes
     * for signature.
     */
    if ((cyBtldrRunType & 0xFFFFu) == CY_APP_SYS_BOOT_MODE_RQT_SIG)
    {
        /*
         * FW has made a request to stay in the boot mode. Return
         * from here after clearing the variable.
         */
        cyBtldrRunType = 0u;
        /* Set the reason for boot mode. */
        gl_img_status.status.boot_mode_request = 1u;
        return false;
    }
    
    /* Check if we have been asked to boot FW1 or FW2 specifically. */
    if ((cyBtldrRunType & 0xFFFFu) == CY_APP_SYS_FW1_BOOT_RQT_SIG)
    { 
        boot_fw1 = true;
    }    

#if (!CY_APP_DUALAPP_DISABLE)
    if ((cyBtldrRunType & 0xFFFFu) == CY_APP_SYS_FW2_BOOT_RQT_SIG)
    {
        boot_fw2 = true;
    }        
#endif /* CY_APP_DUALAPP_DISABLE */

#if (!CY_APP_DUALAPP_DISABLE)
    /*
     * If we have been specifically asked to boot FW2, do that;
     * otherwise, choose the binary with greater sequence number.
     */
    if (gl_img_status.status.fw2_invalid == 0u)
    {
        /* 
         * FW2 is valid
         * We can boot this if:
         * 1. We have been asked to boot FW2
         * 2. FW1 is not valid
         * 3. FW2 is newer than FW1, and we have not been asked to boot FW1.
         */
        if ((gl_img_status.status.fw1_invalid != 0u) || (boot_fw2) ||
                ((!boot_fw1) && (CALL_IN_FUNCTION(Cy_App_Sys_GetRecentFwImage)() == CY_APP_SYS_FW_MODE_FWIMAGE_2)))
        {
            md_p = GET_IN_VARIABLE(gl_img2_fw_metadata);
            img  = Bootloader_1_MD_BTLDB_ACTIVE_1;
        }
        else
        {
            md_p = GET_IN_VARIABLE(gl_img1_fw_metadata);
            img  = Bootloader_1_MD_BTLDB_ACTIVE_0;
        }
    }
    else
#endif /* CY_APP_DUALAPP_DISABLE */
    {
        /* FW2 is invalid */
        /* Load FW1 if it is valid */
        if (gl_img_status.status.fw1_invalid == 0u)
        {
            md_p = GET_IN_VARIABLE(gl_img1_fw_metadata);
            img  = Bootloader_1_MD_BTLDB_ACTIVE_0;
        }
    }
    
    if (md_p != NULL)
    {
#if (CY_APP_BOOT_WAIT_WINDOW_DISABLE == 0)
        /*
         * If we are in the middle of a jump-to-alt-fw command, do not provide
         * the boot wait window.
         */
        if ((boot_fw1) || (boot_fw2))
        {
            gl_boot_wait_delay = 0u;
        }
        else
        {
            boot_set_wait_timeout (md_p);
        }
#endif /* CY_APP_BOOT_WAIT_WINDOW_DISABLE */

        Bootloader_1_activeApp = img;

        return true;
    }

    /* Stay in bootloader */
    return false;
}

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.11', 1,'This function will not return');
void Cy_App_Boot_JumpToFw(void)
{
    /* Schedule the FW and undergo a reset */
    cyBtldrRunType = 0x80u;
    CySoftwareReset ();
}
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.11');
#else /* !CY_APP_BOOT_ENABLE */

void Cy_App_Boot_UpdateFwStatus(void)
{
    gl_img_status.val = 0u;

    /* Check the two firmware binaries for validity */
    if (Cy_App_Boot_HandleValidateFwCmd (CY_APP_SYS_FW_MODE_FWIMAGE_1) != CY_APP_STAT_SUCCESS)
    {
        gl_img_status.status.fw1_invalid = 1u;
    }

#if (!CY_APP_DUALAPP_DISABLE)
    if (Cy_App_Boot_HandleValidateFwCmd (CY_APP_SYS_FW_MODE_FWIMAGE_2) != CY_APP_STAT_SUCCESS)
#endif /* !CY_APP_DUALAPP_DISABLE */
    {
        gl_img_status.status.fw2_invalid = 1u;
    }

#if CY_APP_PRIORITY_FEATURE_ENABLE
    /* Update the app-priority field if the feature is enabled */
    gl_img_status.status.reserved1 = ((*(uint8_t *)(CY_APP_SYS_APP_PRIORITY_ROW_NUM << CY_APP_SYS_FLASH_ROW_SHIFT_NUM)) << 4u);
#endif /* CY_APP_PRIORITY_FEATURE_ENABLE */
}
#endif /* CY_APP_BOOT_ENABLE */

uint32_t Cy_App_Boot_GetBootSeq(uint8_t fwid)
{
    cy_stc_sys_fw_metadata_t *md_p;

    if (fwid == (uint8_t)CY_APP_SYS_FW_MODE_FWIMAGE_1)
    {
        md_p = (cy_stc_sys_fw_metadata_t *)CY_APP_SYS_IMG1_FW_METADATA_ADDR;
    }    
#if (!CY_APP_DUALAPP_DISABLE)
    else
    {
        md_p = (cy_stc_sys_fw_metadata_t *)CY_APP_SYS_IMG2_FW_METADATA_ADDR;
    }
#endif /* (!CY_APP_DUALAPP_DISABLE) */

#if (CY_APP_BOOT_ENABLE != 0)
    if (Cy_App_Boot_ValidateFw (md_p) == CY_APP_STAT_SUCCESS)
    {
        return (md_p->boot_seq);
    }
#else /* (CY_APP_BOOT_ENABLE != 0) */
    /* We only need to validate if the target is not the active firmware */
    if ((CALL_IN_FUNCTION(Cy_App_Sys_GetDeviceMode)() == fwid) || (Cy_App_Boot_ValidateFw (md_p) == CY_APP_STAT_SUCCESS))
    {
        return (md_p->boot_seq);
    }
#endif /* (CY_APP_BOOT_ENABLE != 0) */

    return 0u;
}

#if (defined (CY_IP_M0S8CRYPTOLITE))
cy_en_app_status_t Cy_App_Boot_CalculateFwImageHash(cy_stc_sys_fw_metadata_t *fw_metadata, uint8_t *final_hash, cy_stc_cryptolite_sha_context_t *sha_ctx)
{
    cy_en_app_status_t status = CY_APP_STAT_SUCCESS;

    /*
     * PMG1S3 includes CRYPTOLITE IP which includes the SHA-256 block. Use the CRYPTOLITE IP
     * through Cryptolite layer.
     */

    /* Pointer to FW image start address */
    uint32_t fw_start = 0u;
    /* Pointer to FW image end address */
    uint32_t fw_end = 0u;
    /* Size of FW image */
    uint32_t fw_size_bytes = 0u;

    /* Init SHA-2 */
    if((Cy_Cryptolite_Sha_Init(CRYPTOLITE, sha_ctx) == CY_CRYPTOLITE_SUCCESS) &&
            (Cy_Cryptolite_Sha_Start(CRYPTOLITE, sha_ctx) == CY_CRYPTOLITE_SUCCESS))
    {
        /* Get the start address of the FW image */
        fw_start = fw_metadata->fw_start;
        /* Get the size of FW image */
        fw_size_bytes = fw_metadata->fw_size;
        fw_end = fw_start + fw_size_bytes;

        /*
         * Ensure fw_start and fw_size are valid.
         * FW_START and FW_SIZE parameters are derived from metadata table.
         * Expectation is that FW START and FW SIZE are valid.
         * FW_START: Should be a multiple of 0x40 (We don't have to check this
         * as FW_START is calculated as a multiple of ROW_SIZE (0x80), hence it is always
         * 64 Bytes aligned.
         * FW_SIZE: Should be multiple of 0x40.
         * FW image should not cross FLASH_SIZE boundary.
         */
        if (fw_end > CY_APP_SYS_FLASH_SIZE)
        {
            return CY_APP_STAT_FAILURE;
        }
        if(Cy_Cryptolite_Sha_Update(CRYPTOLITE, (uint8_t *)fw_start, fw_size_bytes, sha_ctx) != CY_CRYPTOLITE_SUCCESS)
        {
            /* HASH calculation failed */
            status = CY_APP_STAT_FAILURE;
        }
        if((Cy_Cryptolite_Sha_Finish(CRYPTOLITE, final_hash, sha_ctx) != CY_CRYPTOLITE_SUCCESS) &&
                (Cy_Cryptolite_Sha_Free(CRYPTOLITE, sha_ctx) != CY_CRYPTOLITE_SUCCESS))
        {
            /* HASH calculation failed */
            status = CY_APP_STAT_FAILURE;
        }
    }
    else
    {
        /* Initialization of SHA context failed */
        status = CY_APP_STAT_FAILURE;
    }

    return status;
}
#endif /* (defined (CY_IP_M0S8CRYPTOLITE)) */

/* [] END OF FILE */
