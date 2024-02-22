/***************************************************************************//**
* \file cy_app_flash_log.h
* \version 1.0
*
* \brief
* Defines APIs and macros for flash logging
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_FLASH_LOG_H_
#define _CY_APP_FLASH_LOG_H_


#if (CY_APP_FLASH_LOG_ENABLE || DOXYGEN)

#include "cy_app_debug.h"
#include "cy_pdutils_sw_timer.h"

/**
* \addtogroup group_pmg_app_common_log
* \{
*/

/**
* \addtogroup group_pmg_app_common_log_macros
* \{
*/
/** Signature used in flash log header for authentication */
#define CY_APP_DEBUG_FLASH_LOG_SIGNATURE                                   (0x5943u)

/** Size occupied by flash logs in the internal flash */
#define CY_APP_DEBUG_FLASH_LOG_SIZE                                        (0x200u)

/** Version of flash log */
#define CY_APP_DEBUG_FLASH_LOG_VERSION                                     (0x00u)

/** Number of common faults supported by the flash log module */
#define CY_APP_DEBUG_FLASH_LOG_NUM_COMMON_FAULTS                           (4u)

/** Number of DMC faults supported by flash log module */
#define CY_APP_DEBUG_FLASH_LOG_NUM_DMC_FAULTS                              (3u)

/** Number of PD port specific faults supported by flash log module */
#define CY_APP_DEBUG_FLASH_LOG_NUM_PORT_FAULTS                             (13u)

/** Number of events after which the backup logs should be written */
#define CY_APP_DEBUG_FLASH_LOG_BKUP_UPDATE_THRESHOLD                     (10u)

/** Size of header used for the flash logs */
#define CY_APP_DEBUG_FLASH_LOG_HEADER_SIZE               (16u)

/** Offset of Common faults */
#define CY_APP_DEBUG_FLASH_LOG_COMMON_FAULTS_OFFSET      (CY_APP_DEBUG_FLASH_LOG_HEADER_SIZE)

/** Reserved size for storing logs of common faults in flash */
#define CY_APP_DEBUG_FLASH_LOG_COMMON_FAULTS_SIZE        (32u)

/** Offset of DMC faults */
#define CY_APP_DEBUG_FLASH_LOG_DMC_FAULTS_OFFSET      (CY_APP_DEBUG_FLASH_LOG_COMMON_FAULTS_OFFSET + CY_APP_DEBUG_FLASH_LOG_COMMON_FAULTS_SIZE)

/** Reserved size for storing logs of DMC faults in flash */
#define CY_APP_DEBUG_FLASH_LOG_DMC_FAULTS_SIZE        (64u)

/** Offset of PD port faults */
#define CY_APP_DEBUG_FLASH_LOG_PD_FAULTS_OFFSET      (CY_APP_DEBUG_FLASH_LOG_DMC_FAULTS_OFFSET + CY_APP_DEBUG_FLASH_LOG_DMC_FAULTS_SIZE)

/** Reserved size for storing logs of PD port faults in flash */
#define CY_APP_DEBUG_FLASH_LOG_PD_FAULTS_SIZE        (72u)

/** Maximum count of each fault */
#define CY_APP_DEBUG_FLASH_LOG_MAX_FAULT_COUNT                         ((uint16_t)0xFFFF)

/** WDT sequential offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_WDT_RESET_OFFSET                        (CY_APP_DEBUG_FLASH_LOG_COMMON_FAULTS_OFFSET)

/** Hard fault sequential offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_HARD_FAULT_OFFSET                        (CY_APP_DEBUG_FLASH_WDT_RESET_OFFSET + sizeof(uint16_t))

/** Power cycle sequential offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_POWER_CYCLE_OFFSET                        (CY_APP_DEBUG_FLASH_HARD_FAULT_OFFSET + sizeof(uint16_t))

/** VDDD BOD sequential offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_VDDD_BOD_FAULT_OFFSET                   (CY_APP_DEBUG_FLASH_LOG_POWER_CYCLE_OFFSET + sizeof(uint16_t))

/** FW update failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_FW_UPD_FAIL_OFFSET                      (CY_APP_DEBUG_FLASH_LOG_DMC_FAULTS_OFFSET)

/** FW update P1 authentication failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_FW_UPD_P1_AUTH_FAIL_OFFSET                      (CY_APP_DEBUG_FLASH_LOG_FW_UPD_FAIL_OFFSET + sizeof(uint16_t))

/** FW update P2 authentication failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_FW_UPD_P2_AUTH_FAIL_OFFSET                      (CY_APP_DEBUG_FLASH_LOG_FW_UPD_P1_AUTH_FAIL_OFFSET + sizeof(uint16_t))

/** VBUS OV failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_VBUS_OV_OFFSET                               (CY_APP_DEBUG_FLASH_LOG_PD_FAULTS_OFFSET)

/** VBUS UV failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_VBUS_UV_OFFSET                               (CY_APP_DEBUG_FLASH_LOG_VBUS_OV_OFFSET + sizeof(uint16_t))

/** VBUS SC failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_VBUS_SC_OFFSET                               (CY_APP_DEBUG_FLASH_LOG_VBUS_UV_OFFSET + sizeof(uint16_t))

/** VBUS OC failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_VBUS_OC_OFFSET                               (CY_APP_DEBUG_FLASH_LOG_VBUS_SC_OFFSET + sizeof(uint16_t))

/** VBUS RC failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_VBUS_RC_OFFSET                               (CY_APP_DEBUG_FLASH_LOG_VBUS_OC_OFFSET + sizeof(uint16_t))

/** VCONN OC failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_VCONN_OC_OFFSET                               (CY_APP_DEBUG_FLASH_LOG_VBUS_RC_OFFSET + sizeof(uint16_t))

/** VBUS IN OV failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_VBUS_IN_OV_OFFSET                               (CY_APP_DEBUG_FLASH_LOG_VCONN_OC_OFFSET + sizeof(uint16_t))

/** VBUS IN UV failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_VBUS_IN_UV_OFFSET                               (CY_APP_DEBUG_FLASH_LOG_VBUS_IN_OV_OFFSET + sizeof(uint16_t))

/** System OT failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_SYSTEM_OT_OFFSET                                 (CY_APP_DEBUG_FLASH_LOG_VBUS_IN_UV_OFFSET + sizeof(uint16_t))

/** PD CRC failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_PD_CRC_OFFSET                                 (CY_APP_DEBUG_FLASH_LOG_SYSTEM_OT_OFFSET + sizeof(uint16_t))

/** CC OV failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_CC_OV_OFFSET                                 (CY_APP_DEBUG_FLASH_LOG_PD_CRC_OFFSET + sizeof(uint16_t))

/** CC SC failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_CC_SC_OFFSET                                 (CY_APP_DEBUG_FLASH_LOG_CC_OV_OFFSET + sizeof(uint16_t))

/** SBU OV failure offset as per the flash log row definition */
#define CY_APP_DEBUG_FLASH_LOG_SBU_OV_OFFSET                                 (CY_APP_DEBUG_FLASH_LOG_CC_SC_OFFSET + sizeof(uint16_t))

/** Invalid OFFSET */
#define CY_APP_DEBUG_FLASH_LOG_INVALID_OFFSET                                (0x0u)

/** Flash page size */
#define CY_APP_DEBUG_FLASH_LOG_FLASH_PAGE_SIZE                              (256u)

/** Initial offset for first and last failure index */
#define CY_APP_DEBUG_FLASH_LOG_OFFSETS_INITIAL_INDEX              (0xFFu)

/** Start offset for first and last failure index */
#define CY_APP_DEBUG_FLASH_LOG_OFFSETS_START_INDEX              (0x00u)

/** Last offset for first and last failure index */
#define CY_APP_DEBUG_FLASH_LOG_OFFSETS_LAST_INDEX              (0xF3u)

/** Size of each failure details */
#define CY_APP_DEBUG_FLASH_LOG_FAILURE_DETAILS_SIZE                      (0x03u)

/** Offset of MSB byte of opcode in failure details */
#define CY_APP_DEBUG_FLASH_LOG_OPCODE_MSB_OFFSET                         (0x00u)

/** Offset of LSB byte of opcode in failure details */
#define CY_APP_DEBUG_FLASH_LOG_OPCODE_LSB_OFFSET                         (0x01u)

/** Offset of additional information in failure details */
#define CY_APP_DEBUG_FLASH_LOG_ADDITIONAL_INFO_OFFSET                         (0x02u)

/** Delay to write into row 2 after row 1 write */
#define CY_APP_DEBUG_FLASH_LOG_ROW2_WRITE_DELAY                                    (100u)

/** \} group_pmg_app_common_log_macros */

/**
* \addtogroup group_pmg_app_common_log_data_structures
* \{
*/
/** Common faults for each port.This structure defines the format of the
 * firmware system faults in the static information row that are Type-C port
 * independent. */
typedef struct /** @cond DOXYGEN_HIDE */ __attribute__((__packed__)) /** @endcond */
{
    /** WDT reset counter */
    uint16_t wdt_reset;

    /** system hard fault counter */
    uint16_t hard_fault;

    /** Power cycle counter for various resets */
    uint16_t power_cycle;

    /** VDDD brown-out fault counter */
    uint16_t vddd_bod_fault;

    /** Reserved */
    uint16_t reserved1[12];

} cy_stc_app_common_faults_t; 

/** DMC faults.This structure defines the format of the DMC faults in the
 * static information row. */
typedef struct /** @cond DOXYGEN_HIDE */ __attribute__((__packed__)) /** @endcond */
{
    /** FW update failure count */
    uint16_t fw_upd_fail_count;

    /** Phase 1 FW update authentication failure count */
    uint16_t p1_fw_upd_auth_fail_count;

    /** Phase 2 FW update authentication failure count */
    uint16_t p2_fw_upd_auth_fail_count;

    /** Reserved */
    uint16_t reserved2[29];

} cy_stc_app_dmc_faults_t;

/** PD Port faults.This structure defines the format of the PD port faults in
 * the static information row. */
typedef struct /** @cond DOXYGEN_HIDE */ __attribute__((__packed__)) /** @endcond */
{
    /** Vbus overvoltage counter */
    uint16_t vbus_ov;

    /** Vbus undervoltage counter */
    uint16_t vbus_uv;

    /** Vbus short-circuit counter */
    uint16_t vbus_sc;

    /** Vbus overcurrent counter */
    uint16_t vbus_oc;

    /** Vbus reverse current counter */
    uint16_t vbus_rc;

    /** Vconn overcurrent counter */
    uint16_t vconn_oc;

    /** Vbus-in overvoltage counter */
    uint16_t vbus_in_ov;

    /** Vbus-in undervoltage counter */
    uint16_t vbus_in_uv;

    /** System over-temperature counter */
    uint16_t system_ot;

    /** CRC error */
    uint16_t crc_err;

    /** CC overvoltage counter */
    uint16_t cc_ov;

    /** CC short-circuit counter */
    uint16_t cc_sc;

    /** SBU overvoltage counter */
    uint16_t sbu_ov;

    /** Reserved */
    uint16_t reserved3[23];

} cy_stc_app_port_faults_t;


/** Flash logging structure */
typedef struct
{
    /** Signature 'CY' */
    uint16_t signature;

    /** Version of black box */
    uint8_t version;

    /** Number of Type-C ports */
    uint8_t no_of_ports;

    /** Number of common faults */
    uint8_t no_of_common_faults;

    /** Number of DMC faults */
    uint8_t no_of_dmc_faults;

    /** Number of port specific faults */
    uint8_t no_of_port_faults;

    /** Reserved */
    uint8_t reserved[7];

    /** Data/faults common for all ports */
    cy_stc_app_common_faults_t common_faults;

    /** DMC faults */
    cy_stc_app_dmc_faults_t dmc_faults;

    /** PD port faults for 2 ports */
    cy_stc_app_port_faults_t port_faults[2];
    
    /** Reserved */
    uint8_t reserved1;
    
    /** Checksum for first row */
    uint8_t checksum_static;
    
    /** Offset of first failure */
    uint8_t first_failure_offset;

    /** Offset of last failure */
    uint8_t last_failure_offset;
    
    /** Reserved */
    uint8_t reserved2[7];

    /** Buffer for storing dynamic information */
    uint8_t failure_details[246];

    /** Checksum for second row */
    uint8_t checksum_dynamic;

} cy_stc_app_flash_log_t;

/** \} group_pmg_app_common_log_data_structures */
/**
* \addtogroup group_pmg_app_common_log_functions
* \{
*/
/*******************************************************************************
* Function name: Cy_App_Debug_FlashInit
****************************************************************************//**
*
* This function will initialize the flash log module. It checks if the existing 
* logs are valid or not. If the existing logs are not valid, then the flash logs 
* are re-initialized.
*
* \param flash_addr
* This parameter specifies the address of internal flash that is used for
* flash logging
*
* \param backup_flash_addr
* This parameter specifies the address of internal flash that is used for
* storing backup of flash logs
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
* None
*
*******************************************************************************/
void Cy_App_Debug_FlashInit(uint32_t flash_addr, uint32_t backup_flash_addr, uint16_t timerID, uint16_t deferTime, cy_stc_pdutils_sw_timer_t *timerCtx);

/*******************************************************************************
* Function name: Cy_App_Debug_FlashLog
****************************************************************************//**
*
* This function handles flash log events. It determines the offsets within the flash 
* where the logs are to be updated and modifies the RAM copy of flash logs. The modified
* RAM copy of flash logs will be used to update the flash logs.
*
* \param port
* This port specifies the port on which the error occurred
*
* \param evt
* The opcode corresponding to the error cy_en_debug_opcodes_t
*
* \param failure_info
* The 1 byte data which is logged along with the opcode in internal flash
*
* \return
* CY_APP_DEBUG_STAT_NOT_READY if flash log module is not initialized
* CY_APP_DEBUG_STAT_FLASH_UPDATE_FAILED if flash write failed
* CY_APP_DEBUG_STAT_SUCCESS if the operation is successful
*
*******************************************************************************/
cy_en_debug_status_t Cy_App_Debug_FlashLog(uint8_t port, cy_en_debug_opcodes_t evt, uint8_t failure_info);

/*******************************************************************************
* Function name: Cy_App_Debug_FlashTask
****************************************************************************//**
*
* This function performs tasks associated with flash logging. It checks for pending flash writes, defer
* the writes and finally write the logs to the flash.
*
* \return
* None
*
*******************************************************************************/
void Cy_App_Debug_FlashTask();

/*******************************************************************************
* Function name: Cy_App_Debug_FlashReadLogs
****************************************************************************//**
*
* This function will retrieve the logs stored in internal flash
*
* \return flash_log
* Pointer to the logs stored in internal flash
*
*******************************************************************************/
cy_stc_app_flash_log_t * Cy_App_Debug_FlashReadLogs(void);
/** \} group_pmg_app_common_log_functions */
/** \} group_pmg_app_common_log */

#endif /* (CY_APP_FLASH_LOG_ENABLE || DOXYGEN) */

#endif /* _CY_APP_FLASH_LOG_H_ */

/* [] END OF FILE */
