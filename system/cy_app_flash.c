/***************************************************************************//**
* \file cy_app_flash.c
* \version 2.0
*
* \brief
* Implements the flash module functions
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "cy_app_status.h"
#include "cy_app_system.h"
#include "cy_app_boot.h"
#include "cy_app_flash_config.h"
#include "cy_app_flash.h"
#include "cy_flash.h"
#include "cy_app_config.h"

/* 
 * Flags to indicate flashing mode. Flash read and write requests
 * are honoured only when any of these flags are set. Also, the
 * Cy_App_Flash_SetAccessLimits() function must be called before flash
 * access can be done. The default values prevent any write or
 * read.
 */
static uint8_t gl_flash_mode_en = 0;

/* Lowest flash row number that can be accessed */
static uint16_t gl_flash_access_first = CY_APP_SYS_FLASH_LAST_ROW_NUM + 1;

/* Highest flash row number that can be accessed */
static uint16_t gl_flash_access_last = CY_APP_SYS_FLASH_LAST_ROW_NUM + 1;

/* Flash row containing metadata for the alternate firmware image */
static uint16_t gl_flash_metadata_row = CY_APP_SYS_FLASH_LAST_ROW_NUM + 1;

/* Last boot loader flash row. Used for read protection */
static uint16_t gl_flash_bl_last_row = CY_APP_SYS_FLASH_LAST_ROW_NUM + 1;

#if (!CY_APP_BOOT_ENABLE)
/* Whether flash write data can be used in-place as SROM API parameter */
static volatile bool gl_flash_write_in_place = false;
#endif /* (!CY_APP_BOOT_ENABLE) */

/* MACROS for SROM APIs */

#define SROM_API_RETURN_VALUE                   \
    (((CY_FLASH_CPUSS_SYSARG_REG & 0xF0000000u) \
      == 0xA0000000u) ? CYRET_SUCCESS :         \
     (CY_FLASH_CPUSS_SYSARG_REG & 0x0000000Fu))

/* Keys used in SROM APIs */
#define SROM_FLASH_API_KEY_ONE          (0xB6)
#define SROM_FLASH_API_KEY_TWO(x)       (uint32_t)(0xD3u + x)
#define SROM_FLASH_KEY_TWO_OFFSET       (0x08)

/* Offset of argument 1 and 2 (words) for SROM APIs in SRAM buffer */
#define SROM_API_ARG0_OFFSET            (0x00)
#define SROM_API_ARG1_OFFSET            (0x01)

/* SROM LOAD FLASH API */
#define SROM_LOAD_FLASH_API_OPCODE              (0x04)
#define SROM_LOAD_FLASH_DATA_OFFSET             (0x02)
#define SROM_LOAD_FLASH_BYTE_ADDR               (0x00)
#define SROM_LOAD_FLASH_BYTE_ADDR_OFFSET        (0x10)
#define SROM_LOAD_FLASH_MACRO_OFFSET            (0x18)

/* FLASH ROW PROGRAM API */
#define SROM_FLASH_PROGRAM_API_OPCODE           (0x05)

/* Non-blocking write flash row API */
#define SROM_NB_FLASH_ROW_API_OPCODE    (0x07)
#define SROM_NB_FLASH_ROW_NUM_OFFSET    (0x10)

/* Resume non-blocking API */
#define SROM_RESUME_NB_API_OPCODE       (0x09)
    
/* Abort non-blocking flash row write API opcode */
#define SROM_ABORT_FLASH_WRITE_OPCODE   (0x1C)

/* CPUSS SYSARG return value mask */
#define CPUSS_SYSARG_RETURN_VALUE_MASK  (0xF0000000u)

/* CPUSS SYSARG success return value */
#define CPUSS_SYSARG_PASS_RETURN_VALUE  (0xA0000000u)

/* CPUSS SYSARG error return value */
#define CPUSS_SYSARG_ERROR_RETURN_VALUE (0xF0000000u)
    
#define CPUSS_FLASH_PARAM_SIZE          (8u)

#if (!CY_APP_BOOT_ENABLE)
/*
 * This function invokes the SROM API to do a flash row write.
 * This function is used instead of the CySysFlashWriteRow, so as to avoid
 * the clock trim updates that are done as part of that API.
 *
 * Note: This function expects that the data_p buffer has CPUSS_FLASH_PARAM_SIZE
 * bytes of prefix space which can be used for setting up the SROM write API
 * parameters.
 */
static cy_en_app_status_t flash_trig_row_write_in_place(uint32_t row_num, uint8_t *data_p)
{
    volatile uint32_t *params = ((uint32_t *)data_p) - 2;
    cy_en_app_status_t status = CY_APP_STAT_SUCCESS;

    /* Connect the charge pump to IMO clock for flash write */
    SRSSLT->CLK_SELECT = (SRSSLT->CLK_SELECT & ~SRSSLT_CLK_SELECT_PUMP_SEL_Msk) | (1 << SRSSLT_CLK_SELECT_PUMP_SEL_Pos);

    /* Set the parameters for load data into latch operation */
    params[0] = SROM_FLASH_API_KEY_ONE |
        (SROM_FLASH_API_KEY_TWO(SROM_LOAD_FLASH_API_OPCODE) << SROM_FLASH_KEY_TWO_OFFSET);
    params[1] = CY_FLASH_SIZEOF_ROW - 1;
#if (CY_APP_IP_FLASH_MACROS > 1)
    if (CY_FLASH_GET_MACRO_FROM_ROW(row_num) != 0)
    {
        params[0] |= (1 << SROM_LOAD_FLASH_MACRO_OFFSET);
    }
#endif /* (CY_APP_IP_FLASH_MACROS > 1) */

    CPUSS->SYSARG = (uint32_t)(&params[0]);
    CPUSS->SYSREQ = (CPUSS_SYSREQ_SYSCALL_REQ_Msk | SROM_LOAD_FLASH_API_OPCODE);
    __asm(
            "NOP\n"
            "NOP\n"
            "NOP\n"
         );

    /* If load latch is successful */
    if ((CPUSS->SYSARG & CPUSS_SYSARG_RETURN_VALUE_MASK) == CPUSS_SYSARG_PASS_RETURN_VALUE)
    {
        /* Perform the flash write */
        params[0] = ((row_num << SROM_NB_FLASH_ROW_NUM_OFFSET) | SROM_FLASH_API_KEY_ONE |
            (SROM_FLASH_API_KEY_TWO(SROM_FLASH_PROGRAM_API_OPCODE) << SROM_FLASH_KEY_TWO_OFFSET));
        CPUSS->SYSARG = (uint32_t)(&params[0]);
        CPUSS->SYSREQ = (CPUSS_SYSREQ_SYSCALL_REQ_Msk | SROM_FLASH_PROGRAM_API_OPCODE);
        __asm(
                "NOP\n"
                "NOP\n"
                "NOP\n"
             );
        if ((CPUSS->SYSARG & CPUSS_SYSARG_RETURN_VALUE_MASK) != CPUSS_SYSARG_PASS_RETURN_VALUE)
        {
            status = CY_APP_STAT_FAILURE;
        }
    }
    else
    {
        status = CY_APP_STAT_FAILURE;
    }

    /* Disconnect the clock to the charge pump after flash write is complete */
    SRSSLT->CLK_SELECT = (SRSSLT->CLK_SELECT & ~SRSSLT_CLK_SELECT_PUMP_SEL_Msk);

    return status;
}
#endif /* (!CY_APP_BOOT_ENABLE) */

/*
 * This function invokes the SROM API to do a flash row write.
 * This function is used instead of the CySysFlashWriteRow, so as to avoid
 * the clock trim updates that are done as part of that API.
 */
cy_en_app_status_t flash_trig_row_write(uint32_t row_num, uint8_t *data_p)
{
    volatile uint32_t params[(CY_FLASH_SIZEOF_ROW + CPUSS_FLASH_PARAM_SIZE) / sizeof(uint32_t)];
    cy_en_app_status_t status = CY_APP_STAT_SUCCESS;

    /* Connect the charge pump to IMO clock for flash write */
    SRSSLT->CLK_SELECT = (SRSSLT->CLK_SELECT & ~SRSSLT_CLK_SELECT_PUMP_SEL_Msk) | (1 << SRSSLT_CLK_SELECT_PUMP_SEL_Pos);

    /* Copy the data into the parameter buffer */
    memcpy ((uint8_t *)(&params[2]), data_p, CY_FLASH_SIZEOF_ROW);

    /* Set the parameters for load data into latch operation */
    params[0] = SROM_FLASH_API_KEY_ONE |
        (SROM_FLASH_API_KEY_TWO(SROM_LOAD_FLASH_API_OPCODE) << SROM_FLASH_KEY_TWO_OFFSET);
    params[1] = CY_FLASH_SIZEOF_ROW - 1;
#if (CY_APP_IP_FLASH_MACROS > 1)
    if (CY_FLASH_GET_MACRO_FROM_ROW(row_num) != 0)
    {
        params[0] |= (1 << SROM_LOAD_FLASH_MACRO_OFFSET);
    }
#endif /* (CY_APP_IP_FLASH_MACROS > 1) */

    CPUSS->SYSARG = (uint32_t)(&params[0]);
    CPUSS->SYSREQ = (CPUSS_SYSREQ_SYSCALL_REQ_Msk | SROM_LOAD_FLASH_API_OPCODE);
    __asm(
            "NOP\n"
            "NOP\n"
            "NOP\n"
         );

    /* If load latch is successful */
    if ((CPUSS->SYSARG & CPUSS_SYSARG_RETURN_VALUE_MASK) == CPUSS_SYSARG_PASS_RETURN_VALUE)
    {
        /* Perform the flash write */
        params[0] = ((row_num << SROM_NB_FLASH_ROW_NUM_OFFSET) | SROM_FLASH_API_KEY_ONE |
            (SROM_FLASH_API_KEY_TWO(SROM_FLASH_PROGRAM_API_OPCODE) << SROM_FLASH_KEY_TWO_OFFSET));
        CPUSS->SYSARG = (uint32_t)(&params[0]);
        CPUSS->SYSREQ = (CPUSS_SYSREQ_SYSCALL_REQ_Msk | SROM_FLASH_PROGRAM_API_OPCODE);
        __asm(
                "NOP\n"
                "NOP\n"
                "NOP\n"
             );
        if ((CPUSS->SYSARG & CPUSS_SYSARG_RETURN_VALUE_MASK) != CPUSS_SYSARG_PASS_RETURN_VALUE)
        {
            status = CY_APP_STAT_FAILURE;
        }
    }
    else
    {
        status = CY_APP_STAT_FAILURE;
    }

    /* Disconnect the clock to the charge pump after flash write is complete */
    SRSSLT->CLK_SELECT = (SRSSLT->CLK_SELECT & ~SRSSLT_CLK_SELECT_PUMP_SEL_Msk);

    return status;
}

void Cy_App_Flash_EnterMode(bool is_enable, cy_en_flash_interface_t mode, bool data_in_place)
{
    /* Enter or exit the flashing mode. Only one mode will be active at a time. */
    if (is_enable)
    {
        gl_flash_mode_en = (1 << mode);
#if (!CY_APP_BOOT_ENABLE)
        gl_flash_write_in_place = data_in_place;
#endif /* (!CY_APP_BOOT_ENABLE) */
    }
    else
    {
        gl_flash_mode_en = 0;
#if (!CY_APP_BOOT_ENABLE)
        gl_flash_write_in_place = false;
#endif /* (!CY_APP_BOOT_ENABLE) */
    }
#if (CY_APP_BOOT_ENABLE)
    (void)data_in_place;
#endif /* (CY_APP_BOOT_ENABLE) */
}

bool Cy_App_Flash_AccessGetStatus (uint8_t modes)
{
    return ((bool)((gl_flash_mode_en & modes) != 0));
}

void Cy_App_Flash_SetAccessLimits (uint16_t start_row, uint16_t last_row, uint16_t md_row,
        uint16_t bl_last_row)
{
    /* 
     * Caller is expected to provide valid parameters. No error checking
     * is expected to be done by this function. Store the flash write and
     * flash read access area information.
     */
    gl_flash_access_first = start_row;
    gl_flash_access_last  = last_row;
    gl_flash_metadata_row = md_row;
    gl_flash_bl_last_row = bl_last_row;
}

cy_en_app_status_t flash_blocking_row_write(uint16_t row_num, uint8_t *data)
{
    cy_en_app_status_t stat = CY_APP_STAT_SUCCESS;

#if (!CY_APP_BOOT_ENABLE)
    /* Invoke flash write API */
    if (gl_flash_write_in_place)
    {
        stat = flash_trig_row_write_in_place (row_num, data);
    }
    else
#endif /* (!CY_APP_BOOT_ENABLE) */
    {
#if ((CY_HPI_ENABLE) && (!CY_APP_BOOT_ENABLE))
        /* Assume that only in-place writes are enabled in HPI based binaries */
        stat = CY_APP_STAT_FAILURE;
#else
        stat = flash_trig_row_write (row_num, data);
#endif /* ((CY_HPI_ENABLE) && (!CY_APP_BOOT_ENABLE)) */
    }

    if (stat != CY_APP_STAT_SUCCESS)
    {
        stat = CY_APP_STAT_FLASH_UPDATE_FAILED;
    }

    return stat;
}

/*
 * @brief Handle clear flash row operation.
 *
 * Description
 * This function clears specified flash row
 *
 * @param row_num Flash row number
 * @return cy_en_app_status_t status code
 */
cy_en_app_status_t Cy_App_Flash_RowClear(uint16_t row_num)
{
    uint32_t row_address = row_num << CY_APP_SYS_FLASH_ROW_SHIFT_NUM;

    uint8_t buffer[CY_APP_SYS_FLASH_ROW_SIZE + CPUSS_FLASH_PARAM_SIZE] = {0};
    return (cy_en_app_status_t)Cy_Flash_WriteRow(row_address, (uint32_t *)buffer);
}

cy_en_app_status_t Cy_App_Flash_RowWrite(uint16_t row_num, uint8_t *data, cy_app_flash_cbk_t cbk)
{
    /* Initialize return status value */
    cy_en_app_status_t status = CY_APP_STAT_NO_RESPONSE;
    uint32_t row_address = row_num << CY_APP_SYS_FLASH_ROW_SHIFT_NUM;

#if (!CY_APP_DUALAPP_DISABLE)
#if ((CY_APP_BOOT_ENABLE != 0) || (CY_APP_PSEUDO_METADATA_DISABLE != 0))
    uint32_t seq_num;
    uint16_t offset;
#else /* ((CY_APP_BOOT_ENABLE != 0) || (CY_APP_PSEUDO_METADATA_DISABLE != 0)) */
    cy_stc_sys_fw_metadata_t *fw_metadata;
#endif /* ((CY_APP_BOOT_ENABLE != 0) || (CY_APP_PSEUDO_METADATA_DISABLE != 0)) */
#endif /* CY_APP_DUALAPP_DISABLE */

    /* Can't handle flash update request if flashing mode is not active */
    if (gl_flash_mode_en == 0)
    {
        return CY_APP_STAT_NOT_READY;
    }

#if !CY_APP_DMC_ENABLE
    if ((data == NULL) || (row_num < gl_flash_access_first) ||
            ((row_num > gl_flash_access_last) && (row_num != gl_flash_metadata_row)) ||
            (row_num > CY_APP_SYS_FLASH_LAST_ROW_NUM))
#else
    if ((data == NULL) || (row_num < gl_flash_access_first) ||
            ((row_num > gl_flash_access_last) && (row_num != gl_flash_metadata_row) &&
            ((row_num < CY_APP_SYS_DMC_METADATA_START_ROW_ID) || 
#if (CY_APP_USR_DEFINED_SN_SUPPORT)            
            (row_num > DOCK_METADATA_END_ROW_ID_WITH_SN))))
#else
            (row_num > CY_APP_SYS_DMC_METADATA_END_ROW_ID))))
#endif /* CY_APP_USR_DEFINED_SN_SUPPORT */    
#endif /* CY_APP_DMC_ENABLE */
    {
        return CY_APP_STAT_INVALID_ARGUMENT;
    }

#if (!CY_APP_DUALAPP_DISABLE)     
#if CY_APP_BOOT_ENABLE
    /*
     * Ensure boot loader is not allowed to write to reserved rows (if any) in FW2 image area.
     * Certain applications use FW image 2 area to store APP priority, customer info etc.
     * These rows are sandwiched between last row of image 2 and image 2's metadata table row.
     */
    if ((row_num > CY_APP_IMG2_LAST_FLASH_ROW_NUM) && (row_num < CY_APP_SYS_IMG2_METADATA_ROW_NUM))
    {
        return CY_APP_STAT_INVALID_ARGUMENT;
    }
#endif /* CY_APP_BOOT_ENABLE */

#if ((CY_APP_BOOT_ENABLE != 0) || (CY_APP_PSEUDO_METADATA_DISABLE != 0))
    /* Byte offset to the sequence number field in metadata */
    offset  = (CY_APP_SYS_FLASH_ROW_SIZE - CY_APP_SYS_METADATA_TABLE_SIZE + CY_APP_SYS_FW_METADATA_BOOTSEQ_OFFSET);
    if (row_num == CY_APP_SYS_IMG1_METADATA_ROW_NUM)
    {
#if CY_APP_PRIORITIZE_FW2
        /* Set sequence number to 0 */
        seq_num = 0;
#else /* !CY_APP_PRIORITIZE_FW2 */
        /* Set sequence number to 1 + that of FW2. */
        seq_num = Cy_App_Boot_GetBootSeq (CY_APP_SYS_FW_MODE_FWIMAGE_2) + 1;
#endif /* CY_APP_PRIORITIZE_FW2 */

        ((uint32_t *)data)[offset / 4] = seq_num;
    }
    if (row_num == CY_APP_SYS_IMG2_METADATA_ROW_NUM)
    {
#if CY_APP_PRIORITIZE_FW1
        /* Set sequence number to 0 */
        seq_num = 0;
#else /* !CY_APP_PRIORITIZE_FW1 */
        /* Set sequence number to 1 + that of FW1 */
        seq_num = Cy_App_Boot_GetBootSeq (CY_APP_SYS_FW_MODE_FWIMAGE_1) + 1;
#endif /* CY_APP_PRIORITIZE_FW1 */

        ((uint32_t *)data)[offset / 4] = seq_num;
    }
#else /* ((CY_APP_BOOT_ENABLE != 0) || (CY_APP_PSEUDO_METADATA_DISABLE != 0)) */
    /*
     * FW image updates the corresponding pseudo metadata
     * row instead of actual metadata row
     */
    if (row_num == CY_APP_SYS_IMG1_METADATA_ROW_NUM)
    {
        row_num = gl_img2_fw_metadata->boot_last_row;
        /*
         * Mark the METADATA_VALID as "CP" which indicates that FW flashing is
         * now complete. After RESET, this will signal the current FW to
         * validate the other image and then jump to it.
         */
        fw_metadata= (cy_stc_sys_fw_metadata_t *)(data + (CY_APP_SYS_FLASH_ROW_SIZE
                - CY_APP_SYS_METADATA_TABLE_SIZE));
        fw_metadata->metadata_valid = CY_APP_SYS_PSEUDO_METADATA_VALID_SIG;
    }
    else if (row_num == CY_APP_SYS_IMG2_METADATA_ROW_NUM)
    {
        row_num = CY_APP_SYS_IMG2_PSEUDO_METADATA_ROW_NUM;
        /* 
         * Mark the METADATA_VALID as "CP" which indicates that FW flashing is
         * now complete. After RESET, this will signal the current FW to validate
         * the other image and then jump to it.
         */
        fw_metadata= (cy_stc_sys_fw_metadata_t *)(data + (CY_APP_SYS_FLASH_ROW_SIZE
                - CY_APP_SYS_METADATA_TABLE_SIZE));
        fw_metadata->metadata_valid = CY_APP_SYS_PSEUDO_METADATA_VALID_SIG;
    }
#endif /* ((CY_APP_BOOT_ENABLE != 0) || (CY_APP_PSEUDO_METADATA_DISABLE != 0)) */
#endif /* (CY_APP_DUALAPP_DISABLE) */

    /* Blocking flash row write in bootloader mode */
    /* Handle only if flashing mode is active */
    status = (cy_en_app_status_t)Cy_Flash_WriteRow(row_address, (uint32_t *)data);

    (void)cbk;
    return status;
}

cy_en_app_status_t Cy_App_Flash_RowRead(uint16_t row_num, uint8_t* data)
{
    /* Can't handle flash update request if flashing mode is not active */
    if (gl_flash_mode_en == 0)
    {
        return CY_APP_STAT_NOT_READY;
    }

    /* We allow any row outside of the bootloader to be read */
    if ((data == NULL) || (row_num <= gl_flash_bl_last_row) ||
            (row_num > CY_APP_SYS_FLASH_LAST_ROW_NUM))
    {
        return CY_APP_STAT_INVALID_ARGUMENT;
    }

    memcpy (data, (void *)(row_num << CY_APP_SYS_FLASH_ROW_SHIFT_NUM), CY_APP_SYS_FLASH_ROW_SIZE);
    return CY_APP_STAT_SUCCESS;
}

#if CY_APP_PRIORITY_FEATURE_ENABLE
cy_en_app_status_t Cy_App_Flash_SetAppPriority(cy_en_flash_app_priority_t app_priority)
{
    uint8_t temp_buf[CY_APP_SYS_FLASH_ROW_SIZE + CPUSS_FLASH_PARAM_SIZE] = {0};

    /* Ensure APP priority value is valid */
    if (app_priority > CY_APP_FLASH_APP_PRIORITY_IMAGE_2)
    {
        return CY_APP_STAT_INVALID_ARGUMENT;
    }
    else
    {
        /* Set APP priority field */
        temp_buf[CPUSS_FLASH_PARAM_SIZE] = app_priority;
        return flash_blocking_row_write (CY_APP_SYS_APP_PRIORITY_ROW_NUM, temp_buf + CPUSS_FLASH_PARAM_SIZE);
    }
}
#endif /* CY_APP_PRIORITY_FEATURE_ENABLE */

/* [] END OF FILE */
