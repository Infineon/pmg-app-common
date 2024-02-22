/***************************************************************************//**
* \file cy_app_uart_debug.h
* \version 1.0
*
* \brief
* Defines the UART-based debug APIs and macros
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_UART_DEBUG_H_
#define _CY_APP_UART_DEBUG_H_

#include "cy_app_debug.h"

#if CY_APP_UART_DEBUG_ENABLE

/* Macro to indicate the external pullup status on the hardware.
 * This is needed for Rev 01 of PMG1S3 USBC Dock RDK board. */
#define CY_APP_DEBUG_PULLUP_ON_UART                              (0u)

/* Position of is_std field in UART header */
#define CY_APP_DEBUG_UART_DATA_IS_STD_POS                          (7u)

/* Mask for identifying variable data size field in UART header */
#define CY_APP_DEBUG_UART_DATA_SIZE_MASK                        (0x07u)

/* Offset of UART header in UART log structure */
#define CY_APP_DEBUG_UART_HEADER_OFFSET                         (0x00u)
/* Offset of High-byte of opcode in UART log structure */
#define CY_APP_DEBUG_UART_DATA_OPCODE_HIGH_BYTE_OFFSET          (0x01u)
/* Offset of Low-byte of opcode in UART log structure */
#define CY_APP_DEBUG_UART_DATA_OPCODE_LOW_BYTE_OFFSET           (0x02u)
/* Offset of Variable data in UART log structure */
#define CY_APP_DEBUG_UART_DATA_VARIABLE_BYTE_OFFSET             (0x03u)

/* Value to be used in is_std field of UART header to indicate a std UART log */
#define CY_APP_DEBUG_STD_MESSAGE                                (0x01u)
/* Value to be used in is_std field of UART header to indicate a custom UART log */
#define CY_APP_DEBUG_CUSTOM_MESSAGE                             (0x00u)

/* Total size of UART log message */
#define CY_APP_DEBUG_UART_BUFFER_SIZE                       (12u)
/* Fixed size of UART log message. This includes the header and opcode */
#define CY_APP_DEBUG_UART_FIXED_DATA_SIZE                    (3u)
/* Size of delimiter that is sent after each UART log */
#define CY_APP_DEBUG_UART_DELIMITER_SIZE                     (2u)

/* Version of UART structure used */
#define CY_APP_DEBUG_UART_STRUCTURE_VERSION               (0x00u)

/* Enables the transmission of critical log messages over UART */
#define CY_APP_DEBUG_LEVEL_RELEASE                        (0x0u)
/* Enables the transmission of critical and error log messages over UART */
#define CY_APP_DEBUG_LEVEL_1                              (0x1u)
/* Enables the transmission of critical, error and warning log messages over UART */
#define CY_APP_DEBUG_LEVEL_2                              (0x2u)
/* Enables the transmission of all log messages over UART */
#define CY_APP_DEBUG_LEVEL_3                              (0x3u)

/*******************************************************************************
* Function name: Cy_Debug_UART_Init
****************************************************************************//**
* Initializes the UART Module for transferring log messages over 
* UART.
*
* \param base
* Pointer to the UART SCB instance
*
* \param config
* Pointer to configuration structure \ref cy_stc_scb_uart_config_t
*
* \param context
* Pointer to the context structure \ref cy_stc_scb_uart_context_t allocated
* by the user. The structure is used during the UART operation for internal
* configuration and data retention. The user must not modify anything
* in this structure.
* If only UART \ref group_scb_uart_ll will be used pass NULL as pointer to
* context.
*
* \return
* \ref cy_en_scb_uart_status_t
*
*******************************************************************************/

cy_en_scb_uart_status_t Cy_Debug_UART_Init(CySCB_Type *base, cy_stc_scb_uart_config_t const *config,
        cy_stc_scb_uart_context_t *context);
        
/*******************************************************************************
* Function name: Cy_Debug_UART_Log
****************************************************************************//**
*
*  Transmits the log messages over UART.
*
* \param base
* Pointer to the UART SCB instance
*
* \param is_std
* specifies the is_std field in the header of UART
* structure
*
* \param opCode
* The opcode corresponding to the error \ref cy_en_debug_opcodes_t
*
* \param data
* Pointer to the variable data which is logged along with the error
*
* \param size
* Size of variable data which is logged along with the error
*
* \param log_level
* This parameter is used to decide the priority of the prints sent out through 
* the UART line \ref cy_en_uart_debug_log_level_t
*
* \return
* CY_APP_DEBUG_STAT_BAD_PARAM if an invalid parameter is passed
* CY_APP_DEBUG_STAT_SUCCESS if the operation is successful
*
*******************************************************************************/

cy_en_debug_status_t Cy_Debug_UART_Log(CySCB_Type *base, bool is_std, cy_en_debug_opcodes_t opCode, uint8_t *data, uint8_t size,
        cy_en_uart_debug_log_level_t log_level);

#endif /* CY_APP_UART_DEBUG_ENABLE */

#endif /* _CY_APP_UART_DEBUG_H_ */

/* [] End of file */
