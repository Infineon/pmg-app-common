/***************************************************************************//**
* \file cy_app_debug.c
* \version 1.0
*
* \brief
* Implements the debug functionality
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "cy_app_debug.h"
#include "cy_app_uart_debug.h"

#if CY_APP_FLASH_LOG_ENABLE
#include "cy_app_flash_log.h"
#endif /* CY_APP_FLASH_LOG_ENABLE */

#if CY_APP_DEBUG_ENABLE

static CySCB_Type *gl_base;

cy_en_debug_status_t Cy_App_Debug_Init(
        CySCB_Type *base,
        cy_stc_scb_uart_config_t const *config,
        uint32_t flash_addr,
        uint32_t backup_flash_addr,
        uint16_t timerID,
        uint16_t deferTime,
        cy_stc_pdutils_sw_timer_t *timerCtx)
{
#if CY_APP_UART_DEBUG_ENABLE
    cy_en_scb_uart_status_t status = CY_SCB_UART_SUCCESS;    
#endif /* CY_APP_UART_DEBUG_ENABLE */

#if CY_APP_FLASH_LOG_ENABLE
    if(timerCtx == NULL || base == NULL)
    {
        return CY_APP_DEBUG_STAT_BAD_PARAM;
    }
#endif /* CY_APP_FLASH_LOG_ENABLE */

#if CY_APP_UART_DEBUG_ENABLE
    gl_base = base;
    status = Cy_Debug_UART_Init(gl_base, config, NULL);
    if(status != CY_SCB_UART_SUCCESS)
    {
        return CY_APP_DEBUG_STAT_UART_INIT_FAILED;
    }
#endif /* CY_APP_UART_DEBUG_ENABLE */

#if CY_APP_FLASH_LOG_ENABLE
    Cy_App_Debug_FlashInit(flash_addr, backup_flash_addr, timerID, deferTime, timerCtx);
#endif /* CY_APP_FLASH_LOG_ENABLE */

    (void)flash_addr;
    return CY_APP_DEBUG_STAT_SUCCESS;
}

cy_en_debug_status_t Cy_App_Debug_LogData(
        uint8_t port,
        cy_en_debug_opcodes_t evt,
        uint8_t *data,
        uint8_t data_size,
        cy_en_uart_debug_log_level_t log_level,
        bool is_std)
{
    cy_en_debug_status_t status = CY_APP_DEBUG_STAT_SUCCESS;
#if CY_APP_UART_DEBUG_ENABLE
    if(gl_base == NULL)
    {
        return CY_APP_DEBUG_STAT_NOT_READY;
    }
    status = Cy_Debug_UART_Log(gl_base, is_std, (uint16_t)evt, data, data_size, log_level);
    if(status != CY_APP_DEBUG_STAT_SUCCESS)
    {
        return status;
    }
#endif /* CY_APP_UART_DEBUG_ENABLE */

    /* These opcodes need not be stored in flash. Only UART logging is needed for these
     * opcodes. */
#if CY_APP_FLASH_LOG_ENABLE

    if(
    (evt != CY_APP_DEBUG_STRUCTURE_VERSION) && (evt != CY_APP_DEBUG_PD_P0_TYPEC_ATTACH) &&
            (evt != CY_APP_DEBUG_PD_P1_TYPEC_ATTACH) && ((evt < CY_APP_DEBUG_FW_UPD_PHASE2_START)
            || (evt > CY_APP_DEBUG_INFO_TRACE))
#if (!CY_APP_LOG_DISCONNECT_EVT_ENABLE)
            && (evt != CY_APP_DEBUG_PD_P0_TYPEC_DETACH) && (evt != CY_APP_DEBUG_PD_P1_TYPEC_DETACH)
#endif /* (!CY_APP_LOG_DISCONNECT_EVT_ENABLE) */            
            )
    {

        status = Cy_App_Debug_FlashLog(port, evt, data[0]);
    }
#endif /* CY_APP_FLASH_LOG_ENABLE */

    return status;
}

uint8_t Cy_App_Debug_AppendScbInfo(
        uint8_t scb_idx,
        cy_en_debug_scb_intf_t intf,
        cy_en_debug_scb_operation_t operation)
{
    uint8_t scb_error_additional_info = 0x00U;

    scb_error_additional_info = (((scb_idx & CY_APP_DEBUG_SCB_ERR_SCB_IDX_MASK) << CY_APP_DEBUG_SCB_ERR_SCB_IDX_POS) | \
            (intf << CY_APP_DEBUG_SCB_ERR_SCB_INTF_POS) | (operation << CY_APP_DEBUG_SCB_ERR_SCB_OPERN_POS));

    return scb_error_additional_info;
}

CySCB_Type * Cy_App_Debug_GetScbBaseAddr(void)
{
    return gl_base;
}
#endif /* CY_APP_DEBUG_ENABLE */

/* [] END OF FILE */