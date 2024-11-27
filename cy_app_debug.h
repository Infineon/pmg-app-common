/***************************************************************************//**
* \file cy_app_debug.h
* \version 2.0
*
* \brief
* Defines APIs and macros for the debug module
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_DEBUG_H_
#define _CY_APP_DEBUG_H_

#include "cy_pdutils_sw_timer.h"

/** Enable the debug module if either UART or flash log module is enabled */
#if (CY_APP_UART_DEBUG_ENABLE || CY_APP_FLASH_LOG_ENABLE)
#define CY_APP_DEBUG_ENABLE                              (1u)
#else
#define CY_APP_DEBUG_ENABLE                              (0u)
#endif /* (CY_APP_UART_DEBUG_ENABLE || CY_APP_FLASH_LOG_ENABLE) */

#if (CY_APP_DEBUG_ENABLE || DOXYGEN)
/**
* \addtogroup group_pmg_app_common_log
* \{
* Include cy_app_debug.h, cy_app_uart_debug.h, and cy_app_flash_log.h to get access to all the
* functions and declarations used by the debug module. See the Configuration Considerations section
* to start using the debug module.
* 
* The debug module is used for logging the failures and events while the
* firmware is running on the PMG1 device. The debug module supports two modes
* of logging. \n 
* a. Logging through UART: \n 
* In this mode, the log messages are transmitted over the UART interface. The
* events and failures are prioritized based on different log levels. 
* The log level can be changed at compile time using CY_APP_DEBUG_LEVEL macro. \n 
* b. Storing into internal flash: \n 
* This feature will enable logging the failure details into internal flash. Two
* types of information are stored into the internal flash. They are static
* information and dynamic information.
*
* Static information stores the number of occurrences of predefined failures.
* Dynamic information stores last few failure details into internal flash.
*
********************************************************************************
* \section section_pmg_app_common_log_config configuration considerations
********************************************************************************
*
* Do the following steps to enable debug module for the application.
* 
* Step 1: Header file inclusion \n
* Include the required header files to get access to the debug module.
* \snippet snippet/debug_snippet.c snippet_debug_include
*
* Step 2: Initialize the debug module \n 
* The debug module can be initialized by calling the API Cy_App_Debug_Init().
* This API will internally initialize UART and flash logging. 
* \snippet snippet/debug_snippet.c snippet_debug_initialize
*
* Step 3: Logging events/failures \n 
* Use the Macro CY_APP_DEBUG_LOG to log an event or failure. The following piece of code 
* is used to log power cycle event.
* \snippet snippet/debug_snippet.c snippet_debug_handle
*
* Step 4: Run the flash log task \n 
* Run the flash log task in the main loop by calling the API
* Cy_App_Debug_FlashTask(). This API will internally check for pending flash
* writes, defer the writes and finally write the logs to internal flash. It
* also handles the backup flash log writes.
* \snippet snippet/debug_snippet.c snippet_debug_task
*
* Step 5: Retrieving flash logs \n 
* The logs stored in internal flash can be retrieved by using the API
* Cy_App_Debug_FlashReadLogs().
* \snippet snippet/debug_snippet.c snippet_debug_retrieve
* 
* \defgroup group_pmg_app_common_log_macros Macros
* \defgroup group_pmg_app_common_log_enums Enumerated Types
* \defgroup group_pmg_app_common_log_data_structures Data Structures
* \defgroup group_pmg_app_common_log_functions Functions
*/

/**
* \addtogroup group_pmg_app_common_log_macros
* \{
*/

/** Macro for logging debug messages */
#define CY_APP_DEBUG_LOG(port, evt, data, data_size, log_level, is_std)   Cy_App_Debug_LogData(port, evt, data, data_size, log_level, is_std)

/** Mask for identifying SCB Index in additional information corresponding to
 * SCB errors */
#define CY_APP_DEBUG_SCB_ERR_SCB_IDX_MASK                           (0x07U)

/** Position of SCB Index in additional information corresponding to SCB errors
 * */
#define CY_APP_DEBUG_SCB_ERR_SCB_IDX_POS                            (0x05U)

/** Position of SCB Interface in additional information corresponding to SCB
 * errors*/
#define CY_APP_DEBUG_SCB_ERR_SCB_INTF_POS                           (0x03U)

/** Position of SCB operation in additional information corresponding to SCB
 * errors*/
#define CY_APP_DEBUG_SCB_ERR_SCB_OPERN_POS                          (0x01U)

/** \} group_pmg_app_common_log_macros */

/**
* \addtogroup group_pmg_app_common_log_enums
* \{
*/
/** This enum defines the opcodes used by the debug module */
typedef enum
{
    /** Opcode for logging UART structure version information */
    CY_APP_DEBUG_STRUCTURE_VERSION = 0x00,

    /** Opcode for logging watchdog reset */
    CY_APP_DEBUG_WDT,
    
    /** Opcode for logging hard fault */
    CY_APP_DEBUG_HARD_FAULT,
    
    /** Opcode for logging power cycle */
    CY_APP_DEBUG_POWER_CYCLE,
    
    /** Opcode for logging VDDD brown out fault */
    CY_APP_DEBUG_VDDD_BROWN_OUT_FAULT,

    /** Opcode for logging firmware update failures */
    CY_APP_DEBUG_FW_UPD_FAILURE = 0x20,
    
    /** Opcode for logging count of Phase 1 authentication failures */
    CY_APP_DEBUG_FW_UPD_P1_AUTH_FAILURE,
    
    /** Opcode for logging count of Phase 2 authentication failures */
    CY_APP_DEBUG_FW_UPD_P2_AUTH_FAILURE,
    
    /** Opcode for logging SCB failures */
    CY_APP_DEBUG_SCB_FAILURES,
    
    /** Opcode for logging HPI events */
    CY_APP_DEBUG_HPI_EVENT,

    /** Opcode for logging VBUS OV corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_VBUS_OV = 0x80,

    /** Opcode for logging VBUS UV corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_VBUS_UV,

    /** Opcode for logging VBUS SC corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_VBUS_SC,
    
    /** Opcode for logging VBUS OC corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_VBUS_OC,
    
    /** Opcode for logging VBUS RC corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_VBUS_RC,
    
    /** Opcode for logging VCONN OC corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_VCONN_OC,
    
    /** Opcode for logging VBUS_IN_OV corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_VBUS_IN_OV,
    
    /** Opcode for logging VBUS_IN_UV corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_VBUS_IN_UV,
    
    /** Opcode for logging system over-temperature corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_SYSTEM_OT,
    
    /** Opcode for logging CRC error corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_CRC_ERROR,
    
    /** Opcode for logging CC OV corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_CC_OV,
    
    /** Opcode for logging CC SC corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_CC_SC,
    
    /** Opcode for logging SBU OV corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_SBU_OV,
    
    /** Opcode for logging Type C detach events corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_TYPEC_DETACH,
    
    /** Opcode for logging Type C attach corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_TYPEC_ATTACH,
    
    /** Opcode for logging hard reset TX corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_HARD_RST_TX,
    
    /** Opcode for logging hard reset RX corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_HARD_RST_RX,
    
    /** Opcode for logging Port Enable corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_PORT_ENABLE,
    
    /** Opcode for logging port disable corresponding to port 0 */
    CY_APP_DEBUG_PD_P0_PORT_DISABLE,

    /** Opcode for logging VBUS OV corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_VBUS_OV = 0xC0,
    
    /** Opcode for logging VBUS UV corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_VBUS_UV,
    
    /** Opcode for logging VBUS SC corresponding to Port 1 */
    CY_APP_DEBUG_PD_P1_VBUS_SC,
    
    /** Opcode for logging VBUS OC corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_VBUS_OC,
    
    /** Opcode for logging VBUS RC corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_VBUS_RC,
    
    /** Opcode for logging VCONN OC corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_VCONN_OC,
    
    /** Opcode for logging VBUS_IN_OV corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_VBUS_IN_OV,
    
    /** Opcode for logging VBUS_IN_UV corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_VBUS_IN_UV,
    
    /** Opcode for logging system over-temperature corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_SYSTEM_OT,
    
    /** Opcode for logging CRC Error corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_CRC_ERROR,
    
    /** Opcode for logging CC OV corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_CC_OV,

    /** Opcode for logging CC SC corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_CC_SC,
    
    /** Opcode for logging SBU OV corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_SBU_OV,
    
    /** Opcode for logging Type-C detach corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_TYPEC_DETACH,
    
    /** Opcode for logging Type-C attach corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_TYPEC_ATTACH,
    
    /** Opcode for logging hard reset TX corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_HARD_RST_TX,
    
    /** Opcode for logging hard reset RX corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_HARD_RST_RX,
    
    /** Opcode for logging port enable corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_PORT_ENABLE,
    
    /** Opcode for logging port disable corresponding to port 1 */
    CY_APP_DEBUG_PD_P1_PORT_DISABLE,

    /** Opcode to trace start of Phase 2 update */
    CY_APP_DEBUG_FW_UPD_PHASE2_START = 0x100,
    
    /** Opcode to trace authentication success */
    CY_APP_DEBUG_FW_UPD_PHASE2_AUTH_SUCCESS,
    
    /** Opcode to trace image write success */
    CY_APP_DEBUG_FW_UPD_PHASE2_IMG_WRITE_SUCCESS,
    
    /** Opcode to trace image write fail */
    CY_APP_DEBUG_FW_UPD_PHASE2_IMG_WRITE_FAIL,
    
    /** Opcode to trace pending updates */
    CY_APP_DEBUG_FW_UPD_PENDING_UPDATES,
    
    /** Opcode to trace start of image update */
    CY_APP_DEBUG_FW_UPD_PHASE2_IMG_UPDATE_START,
    
    /** Opcode to trace end of Phase 2 update */
    CY_APP_DEBUG_FW_UPD_PHASE2_UPDATE_END,
    
    /** Opcode to trace start of factory backup */
    CY_APP_DEBUG_FW_UPD_PHASE2_FACTORY_BKUP_START,
    
    /** Opcode to trace dock reset */
    CY_APP_DEBUG_FW_UPD_PHASE2_DOCK_RESET,
    
    /** Opcode to trace factory backup not done */
    CY_APP_DEBUG_FW_UPD_PHASE2_FACTORY_BKUP_NOT_DONE,
    
    /** Opcode to trace failure of factory update status */
    CY_APP_DEBUG_FW_UPD_PHASE2_FACTORY_UPDATE_FAIL,
    
    /** Opcode to trace DMC state */
    CY_APP_DEBUG_OPCODE_DMC_STATE,
    
    /** Opcode to trace reception of HPI master event */
    CY_APP_DEBUG_HPI_MASTER_EVT_RECEIVED,
    
    /** Opcode to trace HPI master queue push error */
    CY_APP_DEBUG_HPI_MASTER_QUEUE_PUSH_ERROR,
    
    /** Opcode for sending out line number or any 2 byte value for debug */
    CY_APP_DEBUG_INFO_TRACE,
    
    /** Opcode for sending power adapter size */
    CY_APP_DEBUG_PA_SIZE,
    
    /** Opcode for sending currently running App ID */
    CY_APP_DEBUG_CURRENT_RUNNING_APP,

}cy_en_debug_opcodes_t;

/** Enumeration defining the UART debug log levels */
typedef enum
{
    /** Info messages */
    CY_APP_DEBUG_LOGLEVEL_INFO = 0x00u,

    /** Warning messages */
    CY_APP_DEBUG_LOGLEVEL_WARNING,
    
    /** Error messages */
    CY_APP_DEBUG_LOGLEVEL_ERROR,
    
    /** Critical messages */
    CY_APP_DEBUG_LOGLEVEL_CRITICAL

}cy_en_uart_debug_log_level_t;

/** Enumeration defining the interfaces causing the SCB failure */
typedef enum
{
    /** Failure is caused by the I2C interface */
    CY_APP_DEBUG_SCB_I2C = 0x00,

    /** Failure is caused by the SPI Interface */
    CY_APP_DEBUG_SCB_SPI,
    
    /** Failure is caused by the UART interface */
    CY_APP_DEBUG_SCB_UART,
    
    /** Failure is caused by IO */
    CY_APP_DEBUG_SCB_IO

}cy_en_debug_scb_intf_t;

/** Enumeration defining the operation responsible for SCB failure */
typedef enum
{
    /** SCB failure occurs during read operation */
    CY_APP_DEBUG_SCB_READ = 0x00,

    /** SCB failure occurs during write operation */
    CY_APP_DEBUG_SCB_WRITE,
    
    /** SCB failure occurs during erase operation */
    CY_APP_DEBUG_SCB_ERASE,

}cy_en_debug_scb_operation_t;

/** Enumeration defining debug module status */
typedef enum
{
    /** Operation successful */
    CY_APP_DEBUG_STAT_SUCCESS = 0x00,

    /** Not ready for operation */
    CY_APP_DEBUG_STAT_NOT_READY,

    /** Bad parameter passed */
    CY_APP_DEBUG_STAT_BAD_PARAM,

    /** Flash update failed */
    CY_APP_DEBUG_STAT_FLASH_UPDATE_FAILED,

    /** UART initialization failed */
    CY_APP_DEBUG_STAT_UART_INIT_FAILED

}cy_en_debug_status_t;

/** \} group_pmg_app_common_log_enums */
/**
* \addtogroup group_pmg_app_common_log_functions
* \{
*/
/*******************************************************************************
* Function name: Cy_App_Debug_Init
****************************************************************************//**
*
* This function is used for the initialization of the debug module. Internally, 
* it calls functions to initialize UART and flash logging.
*
* \param base
* The pointer to the UART SCB instance
*
* \param config
* The pointer to SCB UART configuration structure
*
* \param flash_addr
* This parameter specifies the address of internal flash that is used for
* flash logging.
*
* \param backup_flash_addr
* This parameter specifies the address of internal flash that is used for
* storing backup of flash logs.
*
* \param timerID
* Timer ID used for flash logging
*
* \param deferTime
* Defer time for flash writes
*
* \param timerCtx
* Pointer to timer context used for flash logging
*
* \return
* CY_APP_DEBUG_STAT_BAD_PARAM if a bad parameter is passed.
* CY_APP_DEBUG_STAT_UART_INIT_FAILED if UART initialization failed.
* CY_APP_DEBUG_STAT_SUCCESS if the operation is successful.
*
*******************************************************************************/

cy_en_debug_status_t Cy_App_Debug_Init(CySCB_Type *base, cy_stc_scb_uart_config_t const *config,
        uint32_t flash_addr, uint32_t backup_flash_addr, uint16_t timerID, uint16_t deferTime, cy_stc_pdutils_sw_timer_t *timerCtx);
        
/*******************************************************************************
* Function name: Cy_App_Debug_LogData
****************************************************************************//**
*
* This function will internally call UART and flash logging functions for 
* logging the failures.
*
* \param port
* This port specifies the port on which the error occurred
*
* \param evt
* The opcode corresponding to the error \ref cy_en_debug_opcodes_t
*
* \param data
* Pointer to the variable data which is logged along with the error
*
* \param data_size
* Size of variable data which is logged along with the error
*
* \param log_level
* This parameter is used to decide the priority of the prints sent out through 
* the UART line
*
* \param is_std
* This parameter is used to specify the is_std field in the header of UART
* structure
*
* \return
* CY_APP_DEBUG_STAT_SUCCESS if the operation is successful
* CY_APP_DEBUG_STAT_NOT_READY if flash log module or UART module is not initialized
* CY_APP_DEBUG_STAT_BAD_PARAM if an invalid evt is used
* CY_APP_DEBUG_STAT_FLASH_UPDATE_FAILED if flash write failed
*
*******************************************************************************/

cy_en_debug_status_t Cy_App_Debug_LogData(uint8_t port,
        cy_en_debug_opcodes_t evt,
        uint8_t *data,
        uint8_t data_size,
        cy_en_uart_debug_log_level_t log_level,
        bool is_std
        );
        
/*******************************************************************************
* Function name: Cy_App_Debug_AppendScbInfo
****************************************************************************//**
*
* This function will generate the additional information corresponding to an 
* SCB failure.
*
* \param scb_idx
* The index of SCB that triggered the SCB error
*
* \param intf
* The interface that triggered the SCB error \ref cy_en_debug_scb_intf_t
*
* \param operation
* The operation that triggered the SCB error \ref cy_en_debug_scb_operation_t
*
* \return
* The additional information corresponding to the SCB error
*
*******************************************************************************/

uint8_t Cy_App_Debug_AppendScbInfo(uint8_t scb_idx,
        cy_en_debug_scb_intf_t intf,
        cy_en_debug_scb_operation_t operation);

/*******************************************************************************
* Function name: Cy_App_Debug_GetScbBaseAddr
****************************************************************************//**
*
* This function returns the SCB Base used by the logging/debug module
*
* \return
* SCB-base used by the logging/debug module
*
*******************************************************************************/
CySCB_Type * Cy_App_Debug_GetScbBaseAddr(void);

/** \} group_pmg_app_common_log_functions */
/** \} group_pmg_app_common_log */

#else
#define CY_APP_DEBUG_LOG(port, evt, data, data_size, log_level, is_std)   NULL
#endif /* (CY_APP_DEBUG_ENABLE || DOXYGEN) */

#endif /* _CY_APP_DEBUG_H_ */

/* [] END OF FILE */
