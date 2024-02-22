/***************************************************************************//**
* \file cy_app_uart_debug.c
* \version 1.0
*
* \brief
* Implements UART-based debug APIs
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "cy_app_uart_debug.h"

#if CY_APP_UART_DEBUG_ENABLE

cy_en_scb_uart_status_t Cy_Debug_UART_Init(
        CySCB_Type *base,
        cy_stc_scb_uart_config_t const *config,
        cy_stc_scb_uart_context_t *context)
{
    cy_en_scb_uart_status_t status = CY_SCB_UART_SUCCESS;
    uint8_t version = CY_APP_DEBUG_UART_STRUCTURE_VERSION;

    /* Configure and enable the UART peripheral */
    status = Cy_SCB_UART_Init(base, config, context);
    if(status != CY_SCB_UART_SUCCESS)
    {
        return status;
    }

    Cy_SCB_UART_Enable(base);

    Cy_SCB_UART_PutString(base, "UART_INIT\r\n");

    /* Send the UART structure version information if the UART is initialized. */
    CY_APP_DEBUG_LOG(0, CY_APP_DEBUG_STRUCTURE_VERSION, &version, 1u, CY_APP_DEBUG_LOGLEVEL_CRITICAL, true);

    return status;
}

cy_en_debug_status_t Cy_Debug_UART_Log(
        CySCB_Type *base,
        bool is_std,
        cy_en_debug_opcodes_t opCode,
        uint8_t *data,
        uint8_t size,
        cy_en_uart_debug_log_level_t log_level)
{
    uint8_t uart_debug_buffer[CY_APP_DEBUG_UART_BUFFER_SIZE];
    bool send_uart_logs = false;
    uint8_t uart_variable_idx = 0;

    if (NULL == base)
    {
        return CY_APP_DEBUG_STAT_BAD_PARAM;
    }

    /* These opcodes are specifically used for tracking the count of P1 and
     * P2 authentication failure counts. So, they need not be sent out through
     * UART.
     */
    if((opCode == CY_APP_DEBUG_FW_UPD_P1_AUTH_FAILURE) || (opCode == CY_APP_DEBUG_FW_UPD_P2_AUTH_FAILURE))
    {
        return CY_APP_DEBUG_STAT_SUCCESS;
    }

    /* To send the logs through UART is based on the debug level.  */
#if (CY_APP_DEBUG_LEVEL == CY_APP_DEBUG_LEVEL_RELEASE)
    if(log_level > CY_APP_DEBUG_LOGLEVEL_ERROR)
    {
        send_uart_logs = true;
    }
#elif (CY_APP_DEBUG_LEVEL == CY_APP_DEBUG_LEVEL_1)
    if(log_level > CY_APP_DEBUG_LOGLEVEL_WARNING)
    {
        send_uart_logs = true;
    }
#elif (CY_APP_DEBUG_LEVEL == CY_APP_DEBUG_LEVEL_2)
    if(log_level > CY_APP_DEBUG_LOGLEVEL_INFO)
    {
        send_uart_logs = true;
    }
#else
    send_uart_logs = true;
#endif /* CY_APP_DEBUG_LEVEL */

    if(send_uart_logs)
    {
        /* Generate the UART log structure */
        uart_debug_buffer[CY_APP_DEBUG_UART_HEADER_OFFSET] = ((is_std << CY_APP_DEBUG_UART_DATA_IS_STD_POS) | (size & CY_APP_DEBUG_UART_DATA_SIZE_MASK));
        uart_debug_buffer[CY_APP_DEBUG_UART_DATA_OPCODE_HIGH_BYTE_OFFSET] = (uint8_t)(opCode >> 8u);
        uart_debug_buffer[CY_APP_DEBUG_UART_DATA_OPCODE_LOW_BYTE_OFFSET] = (uint8_t)opCode;

        /* Boundary condition to ensure that the variable data size does not cross the
         * maximum UART log size. */
        if((size > (CY_APP_DEBUG_UART_BUFFER_SIZE-CY_APP_DEBUG_UART_FIXED_DATA_SIZE-CY_APP_DEBUG_UART_DELIMITER_SIZE)))
        {
            size = CY_APP_DEBUG_UART_BUFFER_SIZE-CY_APP_DEBUG_UART_FIXED_DATA_SIZE-CY_APP_DEBUG_UART_DELIMITER_SIZE;
        }
        if((size > 0u) && (data != NULL))
        {
            for(uart_variable_idx = 0; uart_variable_idx < size; uart_variable_idx++)
            {
                uart_debug_buffer[CY_APP_DEBUG_UART_DATA_VARIABLE_BYTE_OFFSET + uart_variable_idx] = data[uart_variable_idx];
            }
        }
        uart_debug_buffer[CY_APP_DEBUG_UART_DATA_VARIABLE_BYTE_OFFSET + uart_variable_idx] = '\r';
        uart_variable_idx++;
        uart_debug_buffer[CY_APP_DEBUG_UART_DATA_VARIABLE_BYTE_OFFSET + uart_variable_idx] = '\n';
        /* Send the generated UART log structure out through UART */
        Cy_SCB_UART_PutArrayBlocking(base, (void *)uart_debug_buffer, (size + CY_APP_DEBUG_UART_FIXED_DATA_SIZE + CY_APP_DEBUG_UART_DELIMITER_SIZE));
    }
   (void)uart_debug_buffer;

   return CY_APP_DEBUG_STAT_SUCCESS;
}
#endif /* CY_APP_UART_DEBUG_ENABLE */

/* [] End of file */