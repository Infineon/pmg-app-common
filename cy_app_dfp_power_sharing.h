/***************************************************************************//**
* \file cy_app_dfp_power_sharing.h
* \version 3.0
*
* \brief
* Defines data structures and function prototypes for DFP Port(s) power sharing feature.
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

#ifndef CY_APP_DFP_POWER_SHARING_H
#define CY_APP_DFP_POWER_SHARING_H

#if (CY_APP_DFP_POWER_SHARING_ENABLE || DOXYGEN)
#include <stdint.h>
#include "cy_pdstack_common.h"
#include "cy_hpi_master.h"

/* Source PDO count, same as maximum source PDOs supported in config table.
 * Maximum value is 7. */
#define CY_APP_DPS_MAX_SRC_PDO_CNT              (7u)

/* Maximum number of DFP ports supported for DFP power sharing algorithm. */
#define CY_MAX_DFP_PORT_COUNT_DOCK              (10u)

/**
* \addtogroup group_pmg_app_common_dfp_power_sharing
* \{
* The DFP power sharing implements a power sharing algorithm for DFP port(s) present on DMC.
* The power sharing algorithm aims to provide higher power to dedicated DFP port.
*
********************************************************************************
* \section section_pmg_app_common_dfp_power_sharing_config Configuration considerations
********************************************************************************
*
* The following steps describe the way for enabling the DFP power sharing feature
* in an application.
*
* 1. Configure the DFP power sharing parameters in main.c file.
*
* 2. Include cy_app_dfp_power_sharing.h to get access to all the functions and
* other declarations related to this feature.
*    \snippet snippet/dfp_power_sharing_snippet.c snippet_configuration_include
*
* 3. Define the following data structure required by application.
*    \snippet snippet/dfp_power_sharing_snippet.c snippet_configuration_dfp_power_sharing
*
* 4. Initialize the DFP power sharing algorithm.
*    \snippet snippet/dfp_power_sharing_snippet.c snippet_dfp_power_sharing_init
*
* 5. Invoke the Cy_App_DfpPowerSharing_Task function from the main processing loop of the
* application to handle the DFP power sharing tasks.
*    \snippet snippet/dfp_power_sharing_snippet.c snippet_dfp_power_sharing_task
*
********************************************************************************
*
* \defgroup group_pmg_app_common_dfp_power_sharing_data_structures Data structures
* \defgroup group_pmg_app_common_dfp_power_sharing_functions Functions
*/
/** \} group_pmg_app_common_dfp_power_sharing */

/**
 * \addtogroup group_pmg_app_common_dfp_power_sharing_data_structures
 * \{
 * */
/*******************************************************************************
* Data structure definition
*******************************************************************************/
/**
 * @typedef cy_en_app_dps_alg_t
 * @brief DFP power sharing algorithm.
 */
typedef enum
{
    CY_APP_DPS_ALG_FCFS = 0u,              /**< First come, first serve algorithm. */
    CY_APP_DPS_ALG_FIXED_PRIORITY,        /**< Fixed priority algorithm. */
    CY_APP_DPS_ALG_EQUAL_PRIORITY,        /**< Equal priority algorithm. */
}cy_en_app_dps_alg_t;

/* Forward declarations of structures */
struct dps_config;
struct cy_stc_app_dps_port_info;
struct cy_stc_app_dps_context;

/**
 * @brief Data structure to hold the port related parameters
 */
typedef struct cy_stc_app_dps_port_info
{
    /** PD stack / HPI context of the port. */
    void *portContext;

    /** Timer ID to track updates on the port. */
    cy_timer_id_t timerId;

    /** Flag to indicate HPI event received. */
    uint8_t eventRcvd           : 1;

    /** Flag to indicate port is ready for communication. */
    uint8_t isPortReady         : 1;

    /** Flag to indicate capability mismatch in current RDO. */
    uint8_t isCapMismatchSet    : 1;

    /** Reserved bits. */
    uint8_t reserved1           : 5;

    /** HPI event code associated with the device. */
    uint8_t eventCode;

    /** Port address linked to the DFP port.
     *  -> For DMC port, port address should be 0.\n
     *  -> For CCGx devices, port address should be I2C address.
     */
    uint8_t portAddr;

    /** DFP power sharing port state to track the device ready/source PDO update status. */
    uint8_t portState;

    /** Port retry to track the source PDO update. */
    uint8_t retryCount;

    /* Reserved. */
    uint8_t reserved2;
}cy_stc_app_dps_port_info_t;

/**
 * @brief Data structure to hold the DFP power sharing parameters
 */
typedef struct cy_stc_app_dps_context
{
    /** Maximum source PDOs count. */
    uint8_t maxPdpSrcPdoCount;

    /** Source capabilities mask to be used to update maximum PDP source capabilities. */
    uint8_t maxPdpSrcCapMask;

    /** Source capabilities mask to be used to update minimum PDP source capabilities. */
    uint8_t minPdpSrcCapMask;

    /** Minimum source PDOs count. */
    uint8_t minPdpSrcPdoCount;

    /** Port number participating in DFP power sharing in case of
     * FCFS (or) Fixed priority algorithm.
     */
    uint8_t portNum;

    /** Port number advertised with maximum PDP earlier in FCFS algorithm. */
    uint8_t restorePortNum;

    /** Pointer to timer context. */
    cy_stc_pdutils_sw_timer_t *ptrTimerContext;

    /** DFP power sharing configuration table pointer. */
    const dps_config_t *ptrDfpPowerSharingCfg;

    /** Minimum PDP source PDO list. */
    cy_pd_pd_do_t minPdpSrcPdo[CY_APP_DPS_MAX_SRC_PDO_CNT];

    /** Maximum PDP source PDO list. */
    cy_pd_pd_do_t maxPdpSrcPdo[CY_APP_DPS_MAX_SRC_PDO_CNT];

    /** Pointer to port related information. */
    cy_stc_app_dps_port_info_t portInfo[CY_MAX_DFP_PORT_COUNT_DOCK];

    /** Timer ID used for checking PD contract status of the DFP port in FCFS algorithm. */
    cy_timer_id_t idleTimerId;
} cy_stc_app_dps_context_t;

/** \} group_pmg_app_common_dfp_power_sharing_data_structures */

/**
* \addtogroup group_pmg_app_common_dfp_power_sharing_functions
* \{
*/
/*******************************************************************************
* Function declaration
********************************************************************************/
/**
 * @brief Initializes the DFP power sharing algorithm required parameters
 *
 * @param context - Pointer to the cy_stc_app_dps_context_t context
 * @param ptrDfpPowerSharingCfg - Pointer to the DFP power sharing configuration table
 * @param ptrCdttCfg - Pointer to the CDTT configuration table
 * @param ptrPort1PdStackContext - Pointer to the Port-1 PD stack context
 * @param ptrHpiMasterContext - Pointer to the global HPI master context
 * @param timerId - Starting timer ID used by DFP power sharing algorithm
 *
 * @return Return True if initialization is success and DFP power sharing is enabled \n
 * False otherwise.
 */
bool Cy_App_DfpPowerSharing_Init(cy_stc_app_dps_context_t *context,
    const dps_config_t *ptrDfpPowerSharingCfg,
    const cdtt_config_t *ptrCdttCfg,
    cy_stc_pdstack_context_t *ptrPort1PdStackContext,
    cy_hpi_master_context_t *ptrHpiMasterContext,
    uint16_t timerId);

/**
 * @brief Implements a power sharing algorithm based on DFP power sharing configuration.
 *
 * @param context - Pointer to the cy_stc_app_dps_context_t context
 *
 */
void Cy_App_DfpPowerSharing_Task(cy_stc_app_dps_context_t *context);

/**
 * @brief Implements CCGx HPI event handler.
 *
 * @param context - Pointer to the cy_hpi_master_context_t context.
 * @param event - Pointer to the cy_hpi_master_event_t structure received from CCGx device.
 *
 */
void Cy_App_DfpPowerSharing_HpiEventHandler(cy_hpi_master_context_t *context, cy_hpi_master_event_t *event);

/**
 * @brief Set Capability mismatch bit for DMC (PMG1-S3) Port-1 DFP port.
 *
 * @param isCapMismatchSet - Capability mismatch status of the PD port.
 *
 */
void Cy_App_DfpPowerSharing_SetDmcPortCapMismatch(bool isCapMismatchSet);

/** \} group_pmg_app_common_dfp_power_sharing_functions */

#endif /* (CY_APP_DFP_POWER_SHARING_ENABLE || DOXYGEN) */

#endif /* CY_APP_DFP_POWER_SHARING_H */

/* [] End of file */
