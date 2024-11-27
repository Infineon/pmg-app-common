/***************************************************************************//**
* \file cy_app_status.h
* \version 2.0
*
* \brief
* Defines the return status definitions
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_STATUS_H_
#define _CY_APP_STATUS_H_

/*****************************************************************************
 * MACRO definition
 *****************************************************************************/

/*****************************************************************************
 * Enumerated data definition
 *****************************************************************************/
/**
 * @brief Interface status codes
 *
 * The enumeration hold the status codes for all PMG1 interfaces. These values
 * are pre-defined for each interface and should not be modified. To make
 * interface usage easier, the enumeration starts at '-2' and the 
 * success status is '0'. The response code should be 
 * incremented by two before sending to the individual interfaces.
 */
typedef enum ccg_status
{
    CY_APP_STAT_NO_RESPONSE = -2,          /**< Special status code indicating no response */
    CY_APP_STAT_SUCCESS = 0,               /**< Success status */
    CY_APP_STAT_FLASH_DATA_AVAILABLE,      /**< Special status code indicating flash data availability. */
    CY_APP_STAT_BAD_PARAM,                 /**< Bad input parameter */
    CY_APP_STAT_INVALID_COMMAND = 3,       /**< Operation failed due to invalid command */
    CY_APP_STAT_FLASH_UPDATE_FAILED = 5,   /**< Flash write operation failed */
    CY_APP_STAT_INVALID_FW,                /**< Special status code indicating invalid firmware */
    CY_APP_STAT_INVALID_ARGUMENT,          /**< Operation failed due to invalid argument */
    CY_APP_STAT_NOT_SUPPORTED,             /**< Feature not supported */
    CY_APP_STAT_INVALID_SIGNATURE,         /**< Invalid signature parameter identified */
    CY_APP_STAT_TRANS_FAILURE,             /**< Transaction failure status */
    CY_APP_STAT_CMD_FAILURE,               /**< Command failure status */
    CY_APP_STAT_FAILURE,                   /**< Generic failure status */
    CY_APP_STAT_READ_DATA,                 /**< Special status code indicating read data
                                             availability */
    CY_APP_STAT_NOT_READY,                 /**< Operation failed due to device/stack not ready. */
    CY_APP_STAT_BUSY,                      /**< Operation failed due to device/stack busy status. */
    CY_APP_STAT_TIMEOUT,                   /**< Operation timed out */
    CY_APP_STAT_INVALID_PORT,              /**< Invalid port number */
    CY_APP_STAT_NO_STATE_CHANGE            /**< Previous state and current state are same */
} cy_en_app_status_t;

#endif /* _CY_APP_STATUS_H_ */

/* [] End of file */
