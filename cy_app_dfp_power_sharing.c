/***************************************************************************//**
* \file cy_app_dfp_power_sharing.c
* \version 3.0
*
* \brief
* Source code for the DFP power sharing feature implementation.
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
#if CY_APP_DFP_POWER_SHARING_ENABLE
#include "cy_pdl.h"
#include "cy_app_dfp_power_sharing.h"
#include "cy_pdstack_dpm.h"
#include "cy_hpi_master.h"
#include "cy_app_dmc_common.h"
#include "cy_usbpd_config_table.h"

/*******************************************************************************
 * MACRO definition
 ******************************************************************************/

/* Value for PDO index not found */
#define DPS_PDO_NOT_FOUND                     (0xFFu)

/* Voltage value for 15V in mV */
#define DPS_VOLTAGE_15V                       (15000u)

/* Voltage value for 9V in mV */
#define DPS_VOLTAGE_9V                        (9000u)

/* Current cap set to 3A */
#define DPS_CURRENT_CAP_3A                    (300u)

/* Default source PDO size in bytes. */
#define DPS_SRC_PDO_DEFAULT_SIZE              (0x04u)

/* Invalid DFP power sharing port number. */
#define DPS_INVALID_PORT_NUM                  (0xFFu)

/* Value used for dividing power value in PDO to calculate power in watts.
 * Voltage in PDO is stored as 50 mV unit. Current in PDO is stored as 10 mA unit.
 * Dividing this value by 2000 gives divide by (1000 * 1000). */
#define DPS_PDO_POWER_DIV_FACTOR              (2000u)

/* Value used for dividing power value in PPS PDO to calculate power in watts.
 * Voltage in PDO is stored as 100 mV unit. Current in PDO is stored as 50 mA unit. */
#define DPS_PPS_PDO_POWER_DIV_FACTOR          (200u)

/* Time period in milliseconds to retry source capabilities if it failed. */
#define DPS_PMG1_RETRY_TIMER_PERIOD           (5u)

/* HPI response time period in milliseconds. */
#define DPS_HPI_RESPONSE_TIMER_PERIOD         (100u)

/* CCGx retry period in milliseconds. */
#define DPS_CCGX_RETRY_TIMER_PERIOD           (1000u)

/* Idle timer period check in milliseconds. */
#define DPS_IDLE_TIMER_PERIOD                 (1000u)

/* PD contract status bit mask in HPI register. */
#define DPS_PD_CONTRACT_STATUS_BIT_MASK       (1u << 10u)

/* Signature value to be used to send source PDO update command over HPI interface. */
#define DPS_SPR_SRC_PDO_SIGNATURE             (0x53524350u)

/* Size of the array (DWORDS) to be used to update source PDO over HPI interface. */
#define DPS_SPR_SRC_PDO_UPDATE_BUF_SIZE       (8u)

/* Default PD event mask value to be updated to CCGx device to receive the following PD events.
 * -> b4: Type-C disconnect
 * -> b5: PD Contract negotiation complete
 * -> b6: PD Control message received (for Hard reset event)
 * -> b11: Error and Timeout related events
 */
#define DPS_HPI_PD_EVENT_MASK                 (0x0870u)

/* Capability mismatch bit mask in RDO (b26) */
#define DPS_CAP_MISMATCH_BIT_MASK_IN_MSB      (0x04u)

/*******************************************************************************
 * Data types
 ******************************************************************************/
/**
 * @typedef cy_en_dps_src_pdo_index_t
 * @brief Source PDOs index for maximum PDP support based on the macros defined above.
 */
typedef enum
{
    DPS_PORT_SRC_PDO_5V_FIXED = 0u,                 /**< Index for 5V fixed source PDO. */
    DPS_PORT_SRC_PDO_9V_FIXED,                      /**< Index for 9V fixed source PDO. */
    DPS_PORT_SRC_PDO_15V_FIXED,                     /**< Index for 15V fixed source PDO. */
    DPS_PORT_SRC_PDO_AVS,                           /**< Index for AVS source PDO. */
}cy_en_dps_src_pdo_idx_t;

/**
 * @typedef cy_en_dps_port_state_t
 * @brief List of port states associated with DFP power sharing algorithm.
 */
typedef enum
{
    DPS_PORT_STATE_IDLE = 0u,                       /**< 0x00u - Idle state. */
    DPS_PORT_STATE_PD_EVT_MASK_UPDATE,              /**< 0x01u - PD event mask update state. */
    DPS_PORT_STATE_WAIT_PD_EVT_MASK_UPDATE,         /**< 0x02u - Waiting for response for PD event mask update state. */
    DPS_PORT_STATE_SRC_PDO_UPDATE,                  /**< 0x03u - Source PDO update state. */
    DPS_PORT_STATE_WAIT_SRC_PDO_UPDATE,             /**< 0x04u - Waiting for response for source PDO update state. */
    DPS_PORT_STATE_WAIT_FOR_PD_CONTRACT,            /**< 0x05u - Waiting for PD contract complete state. */
    DPS_PORT_STATE_SRC_PDO_RESTORE,                 /**< 0x06u - Source PDO restore state. */
    DPS_PORT_STATE_WAIT_SRC_PDO_RESTORE,            /**< 0x07u - Waiting for response for source PDO restore state. */
    DPS_PORT_STATE_MAX_PDP_ADVERTISED,              /**< 0x08u - Maximum PDP advertised state. */
}cy_en_dps_port_state_t;

/*******************************************************************************
 * Global variables
 ******************************************************************************/

/**
 * @brief Global pointer to structure to hold the global DFP power sharing context.
 */
static cy_stc_app_dps_context_t *gl_ptrDfpPowerSharingCtx;

/**
 * @brief Global variable to store the current DFP port number for DFP power sharing.
 */
static uint8_t gl_dfpPortNum = 0u;

/*******************************************************************************
 * Function definition
 ******************************************************************************/
/**
 * @brief This function updates the PDO current value based on power value.
 */
static void Cy_App_DfpPowerSharing_UpdatePdo(uint16_t power, cy_pd_pd_do_t *pdo)
{
    cy_en_pdstack_pdo_t supplyType = (cy_en_pdstack_pdo_t)pdo->fixed_src.supplyType;
    cy_en_pdstack_apdo_t apdoType = (cy_en_pdstack_apdo_t)pdo->pps_src.apdoType;

    /* Check for fixed supply PDO. */
    if (CY_PDSTACK_PDO_FIXED_SUPPLY == supplyType)
    {
            pdo->fixed_src.maxCurrent = ((uint32_t)power * DPS_PDO_POWER_DIV_FACTOR) / pdo->fixed_src.voltage;
    }
    else if (supplyType == CY_PDSTACK_PDO_AUGMENTED)
    {
        if(apdoType == CY_PDSTACK_APDO_PPS)
        {
            pdo->pps_src.maxCur = ((uint32_t)power * DPS_PPS_PDO_POWER_DIV_FACTOR) / pdo->pps_src.maxVolt;
        }
        else if (apdoType == CY_PDSTACK_APDO_SPR_AVS)
        {
            /* Assume AVS voltage = 15V */
            pdo->spr_avs_src.maxCur1 = ((uint32_t)power * DPS_PDO_POWER_DIV_FACTOR * CY_PD_VOLT_PER_UNIT) / DPS_VOLTAGE_15V;
        }
        else
        {
            /*No Operation Needed*/
        }    
    }
    else
    {
        /*No Operation Needed*/
    }
}

/**
 * @brief This function configures the list of source PDOs based on maximum PDP value.
 */
static void Cy_App_DfpPowerSharing_UpdateMaxPdpSrcPdo(
    cy_stc_app_dps_context_t *context, 
    const dps_config_t *ptrDfpPowerSharingCfg)
{
    uint8_t idx9vPdo = DPS_PDO_NOT_FOUND;
    uint8_t idx15vPdo = DPS_PDO_NOT_FOUND;
    uint8_t idxAvsPdo = DPS_PDO_NOT_FOUND;
    
    uint8_t pdoCount = context->minPdpSrcPdoCount;
    uint8_t maxPdp = ptrDfpPowerSharingCfg->max_pdp_watts;

    context->maxPdpSrcCapMask = context->minPdpSrcCapMask ;
    context->maxPdpSrcPdoCount = pdoCount;

    /* Identify index of 9V,15V and AVS PDOs */
    for (uint8_t pdoIndex = 0u; pdoIndex < pdoCount; ++pdoIndex)
    {
        const cy_pd_pd_do_t *pdo = &context->maxPdpSrcPdo[pdoIndex];
        cy_en_pdstack_pdo_t supplyType = (cy_en_pdstack_pdo_t)pdo->fixed_src.supplyType;
        cy_en_pdstack_apdo_t apdoType = (cy_en_pdstack_apdo_t)pdo->spr_avs_src.apdoType;

        if (supplyType == CY_PDSTACK_PDO_FIXED_SUPPLY)
        {
            uint16_t voltage_mv = (uint16_t)pdo->fixed_src.voltage * CY_PD_VOLT_PER_UNIT;
            if ((voltage_mv == DPS_VOLTAGE_9V) && (idx9vPdo == DPS_PDO_NOT_FOUND))
            {
                idx9vPdo = pdoIndex;
            }    
            else if ((voltage_mv == DPS_VOLTAGE_15V) && (idx15vPdo == DPS_PDO_NOT_FOUND))
            {
                idx15vPdo = pdoIndex;
            }    
            else
            {
                /* No Operation Needed */
            }
        }
        else if ((supplyType == CY_PDSTACK_PDO_AUGMENTED) &&
                 (apdoType == CY_PDSTACK_APDO_SPR_AVS) &&
                 (idxAvsPdo == DPS_PDO_NOT_FOUND))
        {
            idxAvsPdo = pdoIndex;
        }
        else
        {
            /* No Operation Needed */
        }
    }

    /* Update 9V PDOs */
    if (idx9vPdo != DPS_PDO_NOT_FOUND)
    {
        /* Calculate current based on max PDP but cap at 3A per spec */
        Cy_App_DfpPowerSharing_UpdatePdo(maxPdp, &context->maxPdpSrcPdo[idx9vPdo]);
        
        /* Cap 9V PDO at 3A according to spec */
        if (context->maxPdpSrcPdo[idx9vPdo].fixed_src.maxCurrent > DPS_CURRENT_CAP_3A)
        {
            context->maxPdpSrcPdo[idx9vPdo].fixed_src.maxCurrent = DPS_CURRENT_CAP_3A;
        }
    }
    
    /* Update 15V PDOs */
    if (idx15vPdo != DPS_PDO_NOT_FOUND)
    {
        Cy_App_DfpPowerSharing_UpdatePdo(maxPdp, &context->maxPdpSrcPdo[idx15vPdo]);
        
        /* Cap 15V PDO at 3A according to spec */
        if (context->maxPdpSrcPdo[idx15vPdo].fixed_src.maxCurrent > DPS_CURRENT_CAP_3A)
        {
            context->maxPdpSrcPdo[idx15vPdo].fixed_src.maxCurrent = DPS_CURRENT_CAP_3A;
        }
    }

    /* Update AVS PDO assuming 15V max voltage and 20V max voltage */
    if (idxAvsPdo != DPS_PDO_NOT_FOUND)
    {
        Cy_App_DfpPowerSharing_UpdatePdo(maxPdp, &context->maxPdpSrcPdo[idxAvsPdo]);
    }
}

/**
 * @brief Initializes the DFP power sharing algorithm with required parameters.
 */
bool Cy_App_DfpPowerSharing_Init(cy_stc_app_dps_context_t *context,
                                    const dps_config_t *ptrDfpPowerSharingCfg,
                                    const cdtt_config_t *ptrCdttCfg,
                                    cy_stc_pdstack_context_t *ptrPort1PdStackContext,
                                    cy_hpi_master_context_t *ptrHpiMasterContext,
                                    uint16_t timerId)
{
    uint8_t portNum;

    /* Check for NULL pointers and DFP power sharing enable in DMC configuration table. */
    if ((NULL == context) || (NULL == ptrDfpPowerSharingCfg) || (NULL == ptrCdttCfg) ||
        (NULL == ptrHpiMasterContext) ||
        (0u == ptrDfpPowerSharingCfg->dfp_power_sharing_enable))
    {
        return false;
    }

    /* Initialize minimum PDP and maximum PDP source PDOs. */
    (void)memset((void *)&(context->minPdpSrcPdo), 0, CY_APP_DPS_MAX_SRC_PDO_CNT * DPS_SRC_PDO_DEFAULT_SIZE);
    (void)memset((void *)&(context->maxPdpSrcPdo), 0, CY_APP_DPS_MAX_SRC_PDO_CNT * DPS_SRC_PDO_DEFAULT_SIZE);

#if PMG1_PD_DUALPORT_ENABLE
    if (NULL == ptrPort1PdStackContext)
    {
        return false;
    }

    /* Copy minimum PDP source PDOs from PMG1 Port-1 configuration table. */
    const cy_stc_pdstack_port_cfg_t *ptrPortCfg = pd_get_ptr_pdstack_tbl(ptrPort1PdStackContext->ptrUsbPdContext);

    (void)memcpy ((void *)&(context->minPdpSrcPdo), (const void *)&(ptrPortCfg->srcPdo), ((uint32_t)ptrPortCfg->srcPdoCount * DPS_SRC_PDO_DEFAULT_SIZE));
    (void)memcpy((void *)&(context->maxPdpSrcPdo),(const void *)&(ptrPortCfg->srcPdo),((uint32_t)ptrPortCfg->srcPdoCount * DPS_SRC_PDO_DEFAULT_SIZE));

    /* Update minimum source PDO mask with unconstrained power bit set. */
    context->minPdpSrcCapMask  = (ptrPortCfg->defSrcPdoMask | CY_PD_PDO_UNCONSTRAINED_BIT_MASK);
    /* Update minimum source PDO count. */
    context->minPdpSrcPdoCount = ptrPortCfg->srcPdoCount;
#endif /* PMG1_PD_DUALPORT_ENABLE */

    /* Update DFP power sharing context parameters. */
    gl_ptrDfpPowerSharingCtx       = context;
    context->ptrDfpPowerSharingCfg = ptrDfpPowerSharingCfg;
    context->ptrTimerContext       = ptrPort1PdStackContext->ptrTimerContext;

    for (portNum = 0u; portNum < ptrDfpPowerSharingCfg->num_dfp_ports; portNum++)
    {
        /* If the Device Type is PMG1S3, then assign the device context as Port-1 PD Stack context. */
        if ((uint8_t)CY_APP_DMC_DEV_TYPE_DMC_PMG1S3 == ptrCdttCfg->dev_info[ptrDfpPowerSharingCfg->dfp_port_config[portNum].device_idx].device_type)
        {
            /* Update DMC controller port information. */
            context->portInfo[portNum].portContext  = ptrPort1PdStackContext;
            context->portInfo[portNum].portAddr     = 0u;
            context->portInfo[portNum].isPortReady  = 1u;
            context->portInfo[portNum].portState    = (uint8_t)DPS_PORT_STATE_PD_EVT_MASK_UPDATE;
        }
        else
        {
            /* Update CCGx controller port information. */
            context->portInfo[portNum].portContext  = ptrHpiMasterContext;
            context->portInfo[portNum].portAddr     = ptrCdttCfg->dev_info[ptrDfpPowerSharingCfg->dfp_port_config[portNum].device_idx].access_param[0];
            context->portInfo[portNum].isPortReady  = 0u;
            context->portInfo[portNum].portState    = (uint8_t)DPS_PORT_STATE_IDLE;
        }

        /* Initialize timer ID for each port. */
        context->portInfo[portNum].timerId         = timerId + portNum;
        context->portInfo[portNum].retryCount      = 0u;
    }

    /* Allocate the timer ID after the timer IDs of all DFP ports. */
    context->idleTimerId = timerId + portNum;

    /* Update the source PDO count. */
    context->maxPdpSrcPdoCount = CY_APP_DPS_MAX_SRC_PDO_CNT;

    /* Update source PDOs based on maximum PDP configuration. */
    Cy_App_DfpPowerSharing_UpdateMaxPdpSrcPdo(context, ptrDfpPowerSharingCfg);

    /* Set the port number and restore port number to invalid. */
    context->portNum        = DPS_INVALID_PORT_NUM;
    context->restorePortNum = DPS_INVALID_PORT_NUM;

    return true;
}

/**
 * @brief Retrieves the port number in DFP power sharing structure for received HPI master event.
 */
static uint8_t Cy_App_DfpPowerSharing_GetPortNum(cy_stc_app_dps_context_t *ptrDfpPowerSharingContext, cy_hpi_master_event_t *event)
{
    uint8_t portNum, devPortIdx;
    cy_stc_app_dps_port_info_t *ptrPortInfo = &ptrDfpPowerSharingContext->portInfo[0];
    uint8_t numDfpPorts = ptrDfpPowerSharingContext->ptrDfpPowerSharingCfg->num_dfp_ports;
    const dfp_port_config_t *ptrDfpPortCfg = &ptrDfpPowerSharingContext->ptrDfpPowerSharingCfg->dfp_port_config[0];

    /* Parse through all DFP ports to identify the actual DFP port associated with the HPI event. */
    for(portNum = 0u; portNum < numDfpPorts; portNum++)
    {
        devPortIdx = ptrDfpPortCfg[portNum].device_port_idx;

        /* Check for the device details associated with the event. */
        if ((event->slaveDev->slaveAddr == ptrPortInfo[portNum].portAddr) &&
            (event->port == devPortIdx))
        {
            /* Device details are matched with  */
            break;
        }
    }

    /* If port number is not matched, return Invalid Port number. */
    if (portNum == numDfpPorts)
    {
        portNum = DPS_INVALID_PORT_NUM;
    }

    return portNum;
}

/**
 * @brief Handles the HPI events received from the CCGx device.
 */
void Cy_App_DfpPowerSharing_HpiEventHandler(cy_hpi_master_context_t *context, cy_hpi_master_event_t *event)
{
    cy_stc_app_dps_context_t *ptrDfpPowerSharingContext = gl_ptrDfpPowerSharingCtx;
    cy_stc_app_dps_port_info_t *ptrPortInfo = &ptrDfpPowerSharingContext->portInfo[0];
    uint8_t portNum = DPS_INVALID_PORT_NUM;
    uint8_t numDfpPorts = ptrDfpPowerSharingContext->ptrDfpPowerSharingCfg->num_dfp_ports;
    CY_UNUSED_PARAMETER(context);

    /* Check whether port-level event is received. */
    if (DPS_INVALID_PORT_NUM != event->port)
    {
        /* Get the port number associated with received HPI master event. */
        portNum = Cy_App_DfpPowerSharing_GetPortNum(ptrDfpPowerSharingContext, event);
    }

    CY_MISRA_DEVIATE_BLOCK_START('OVERRUN', 1, 'False positive');        
    CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 18.1', 1, 'False positive');        
    /* Handle selection HPI events for processing. Other event codes are not processed.
     * If any other HPI event is received apart from the handled list, the DFP port state is updated
     * in timer callback. */
    switch ((cy_hpi_master_response_t)event->eventCode)
    {
        case CY_HPI_MASTER_EVENT_RESET_COMPLETE:
        {
            /* As Reset event is associated with one CCGx device, it is required to update
             * DFP port(s) based on device type (single/dual port device).
             * Hence, it is required to check all configured DFP ports. */
            for(portNum = 0u; portNum < numDfpPorts; portNum++)
            {
                if (event->slaveDev->slaveAddr == ptrPortInfo[portNum].portAddr)
                {
                    /* Reset the port state to update PD event mask. */
                    ptrPortInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_PD_EVT_MASK_UPDATE;

                    /* Reset the port retry count. */
                    ptrPortInfo[portNum].retryCount = 0u;

                    /* Stop the timer. */
                    Cy_PdUtils_SwTimer_Stop(ptrDfpPowerSharingContext->ptrTimerContext, ptrPortInfo[portNum].timerId);

                    /* If the port associated with FCFS/Fixed algorithm,
                     * mark the power sharing port number and restore port number as invalid. */
                    if (ptrDfpPowerSharingContext->portNum == portNum)
                    {
                        ptrDfpPowerSharingContext->portNum = DPS_INVALID_PORT_NUM;
                    }
                    ptrDfpPowerSharingContext->restorePortNum = DPS_INVALID_PORT_NUM;

                    /* Reset the port ready flag. */
                    ptrPortInfo[portNum].isPortReady = 0u;

                    /* Reset capability mismatch flag. */
                    ptrPortInfo[portNum].isCapMismatchSet = 0u;
                }
            }
            break;
        }
        case CY_HPI_MASTER_RESPONSE_SUCCESS:
        case CY_HPI_MASTER_EVENT_NEGOTIATION_COMPLETE:
        case CY_HPI_MASTER_EVENT_PS_RDY_RECEIVED:
        {
            /* Check whether port number is valid. */
            if (DPS_INVALID_PORT_NUM != portNum)
            {
                /* Set HPI event received flag and event code. */
                ptrPortInfo[portNum].eventRcvd = 1u;
                ptrPortInfo[portNum].eventCode = event->eventCode;
                /* Stop the running timer associated with the port. */
                Cy_PdUtils_SwTimer_Stop(ptrDfpPowerSharingContext->ptrTimerContext, ptrPortInfo[portNum].timerId);

                /* Set the Capability mismatch bit after PD contract negotiation is complete.
                 * It shall be set only if the power sharing port (FCFS) is not set. */
                if (((uint8_t)CY_APP_DPS_ALG_FCFS == ptrDfpPowerSharingContext->ptrDfpPowerSharingCfg->power_sharing_method) &&
                    ((uint8_t)CY_HPI_MASTER_EVENT_NEGOTIATION_COMPLETE == event->eventCode) &&
                    (8u == event->dataLen) &&
                    (0u != (event->eventData[7u] & DPS_CAP_MISMATCH_BIT_MASK_IN_MSB)))
                {
                    /* Set capability mismatch flag. */
                    ptrPortInfo[portNum].isCapMismatchSet = 1u;
                }
            }
            break;
        }
        /* Intentional fall-through. */
        case CY_HPI_MASTER_EVENT_DISCONNECT_DETECT:
        case CY_HPI_MASTER_EVENT_HARD_RESET_RECEIVED:
        case CY_HPI_MASTER_EVENT_HARD_RESET_SENT:
        case CY_HPI_MASTER_EVENT_SOURCE_DISABLED:
        case CY_HPI_MASTER_EVENT_PORT_DISABLED:
        case CY_HPI_MASTER_EVENT_SENDER_TIMEOUT:
        {
            /* These HPI events are applicable only for the PD port level command updates.
             * If these events are received, the register update is not completed, hence the
             * port state is updated accordingly. */

            /* Check whether port number is valid. */
            if (DPS_INVALID_PORT_NUM != portNum)
            {
                /* Reset capability mismatch flag. */
                ptrPortInfo[portNum].isCapMismatchSet = 0u;
                /* Check for FCFS algorithm and Maximum power shared port is set. */
                if (((uint8_t)CY_APP_DPS_ALG_FCFS == ptrDfpPowerSharingContext->ptrDfpPowerSharingCfg->power_sharing_method) &&
                    (ptrDfpPowerSharingContext->portNum == portNum))
                {
                    /* Restore the PDOs back to minimum source PDOs. */
                    ptrDfpPowerSharingContext->restorePortNum = portNum;
                    ptrDfpPowerSharingContext->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_SRC_PDO_RESTORE;
                    ptrDfpPowerSharingContext->portNum = DPS_INVALID_PORT_NUM;
                }
            }
            break;
        }
        /* Intentional fall-through. */
        case CY_HPI_MASTER_RESPONSE_INVALID_COMMAND:
        case CY_HPI_MASTER_RESPONSE_INVALID_ARGUMENT:
        case CY_HPI_MASTER_RESPONSE_NOT_SUPPORTED:
        case CY_HPI_MASTER_RESPONSE_UNDEFINED_ERROR:
        case CY_HPI_MASTER_RESPONSE_PD_COMMAND_FAILED:
        case CY_HPI_MASTER_RESPONSE_PORT_BUSY:
        {
            /* These HPI events are applicable only for the PD port level command updates.
             * If these events are received, the register update is not completed, hence the
             * port state is updated accordingly. */

            /* Check whether port number is valid. */
            if (DPS_INVALID_PORT_NUM != portNum)
            {
                /* Set HPI event received flag and event code. */
                ptrPortInfo[portNum].eventRcvd = 1u;
                ptrPortInfo[portNum].eventCode = event->eventCode;
                /* Stop the running timer associated with the port. */
                Cy_PdUtils_SwTimer_Stop(ptrDfpPowerSharingContext->ptrTimerContext, ptrPortInfo[portNum].timerId);
            }
            break;
        }
        default:
            /* Nothing to do */
            break;
    }
    CY_MISRA_BLOCK_END('MISRA C-2012 Rule 18.1'); 
    CY_MISRA_BLOCK_END('OVERRUN'); 
}

/**
 * @brief Handles the timeout functionality during source PDO restore.
 */
static void Cy_App_DfpPowerSharing_SrcPdoRestoreTimerCbk(cy_timer_id_t id, void *ptrContext)
{
    cy_stc_app_dps_context_t *context = (cy_stc_app_dps_context_t *)ptrContext;
    uint16_t portNum = id - context->portInfo[0].timerId;

    /* Retry restoring source PDO command for 2 times, other the device is non-responsive
     * and update the port state to idle and reset the restore port number. */
    if (context->portInfo[portNum].retryCount < 2u)
    {
        context->portInfo[portNum].retryCount++;
        context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_SRC_PDO_RESTORE;
    }
    else
    {
        context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_IDLE;
        context->restorePortNum = DPS_INVALID_PORT_NUM;
    }
}

/**
 * @brief Handles the timeout functionality during source PDOs update.
 */
static void Cy_App_DfpPowerSharing_SrcPdoUpdateTimerCbk(cy_timer_id_t id, void *ptrContext)
{
    cy_stc_app_dps_context_t *context = (cy_stc_app_dps_context_t *)ptrContext;
    uint16_t portNum = id - context->portInfo[0].timerId;

    /* Check for DFP power sharing algorithm. */
    switch ((cy_en_app_dps_alg_t)context->ptrDfpPowerSharingCfg->power_sharing_method)
    {
        case CY_APP_DPS_ALG_FCFS:
        case CY_APP_DPS_ALG_FIXED_PRIORITY:
        {
            if (portNum == context->portNum)
            {
                /* Check for DMC device and maximum retries. */
                if ((0u == context->portInfo[portNum].portAddr) && (context->portInfo[portNum].retryCount < 3u))
                {
                    context->portInfo[portNum].retryCount++;
                    context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_SRC_PDO_UPDATE;
                }
                else
                {
                    /* Update the port state to restore source PDO if the algorithm is FCFS. */
                    if ((uint8_t)CY_APP_DPS_ALG_FCFS == context->ptrDfpPowerSharingCfg->power_sharing_method)
                    {
                        context->restorePortNum = context->portNum;
                        context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_SRC_PDO_RESTORE;

                        /* If the source PDO update is not completed in the given time,
                         * reset port number which participated in DFP power sharing. */
                        context->portNum = DPS_INVALID_PORT_NUM;
                    }
                    else
                    {
                        /* Update the port state to maximum PDP advertised for Fixed priority algorithm
                         * as there is no need to try again and only updated source PDOs will be sent. */
                        context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_MAX_PDP_ADVERTISED;
                    }
                }
            }
            break;
        }
        case CY_APP_DPS_ALG_EQUAL_PRIORITY:
        {
            /* Retry updating source PDO command for 3 times, other the device is non-responsive
             * and update the port state to maximum PDP advertised. */
            if (context->portInfo[portNum].retryCount < 3u)
            {
                context->portInfo[portNum].retryCount++;
                context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_SRC_PDO_UPDATE;
            }
            else
            {
                context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_MAX_PDP_ADVERTISED;
            }
            break;
        }
        default:
            /* Nothing to do */
            break;
    }
}

/**
 * @brief Restores the source PDO of the given port.
 */
static void Cy_App_DfpPowerSharing_RestoreSrcPdo(cy_stc_app_dps_context_t *context, uint8_t portNum, uint8_t devPortIdx)
{
    cy_stc_app_dps_port_info_t *ptrPortInfo = &context->portInfo[portNum];
    uint32_t srcPdoSetData[DPS_SPR_SRC_PDO_UPDATE_BUF_SIZE] = {0u};
    uint8_t defSrcCapMask = context->minPdpSrcCapMask;

    /* Copy minimum PDP source PDPs from context buffer to local buffer. */
    (void)memcpy ((void *)&srcPdoSetData[1], (void *)&(context->minPdpSrcPdo),
                        (uint8_t)(context->minPdpSrcPdoCount * DPS_SRC_PDO_DEFAULT_SIZE));

    /* Check for DMC port. */
    if (0u == ptrPortInfo->portAddr)
    {
        cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t*)ptrPortInfo->portContext;

        /* Update source PDO using PD stack APIs and reset restore port number. */
        CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 11.3', 1, 'Intentional pointer type conversion.');
        if ((CY_PDSTACK_STAT_SUCCESS == Cy_PdStack_Dpm_UpdateSrcCapMask(ptrPdStackContext, defSrcCapMask)) &&
            (CY_PDSTACK_STAT_SUCCESS == Cy_PdStack_Dpm_UpdateSrcCap(ptrPdStackContext,
                                                                context->minPdpSrcPdoCount,
                                                                (cy_pd_pd_do_t*)&srcPdoSetData[1])))
        {
            context->restorePortNum = DPS_INVALID_PORT_NUM;
            ptrPortInfo->portState  = (uint8_t)DPS_PORT_STATE_IDLE;
        }
        else
        {
            /* Update port state to Idle to retry updating source capabilities. */
            ptrPortInfo->portState = (uint8_t)DPS_PORT_STATE_IDLE;

            /* Start timer with HPI response time. */
            (void)Cy_PdUtils_SwTimer_Start (context->ptrTimerContext,
                                        context,
                                        ptrPortInfo->timerId,
                                        DPS_PMG1_RETRY_TIMER_PERIOD,
                                        &Cy_App_DfpPowerSharing_SrcPdoRestoreTimerCbk);
        }
        CY_MISRA_BLOCK_END('MISRA C-2012 Rule 11.3');
    }
    else
    {
        cy_hpi_master_context_t *ptrHpiMasterContext = (cy_hpi_master_context_t *)ptrPortInfo->portContext;

        /* Signature to update source PDOs. */
        srcPdoSetData[0] = DPS_SPR_SRC_PDO_SIGNATURE;

        /* Write source PDO information to PD data memory. */
        if (CY_HPI_MASTER_SUCCESS == Cy_HPI_Master_PdResponseRegWrite(ptrHpiMasterContext,
                                                    ptrPortInfo->portAddr,
                                                    devPortIdx,
                                                    0u,
                                                    &srcPdoSetData,
                                                    (uint16_t)sizeof(srcPdoSetData)))
        {
            /* Update the source PDO mask selection register. */
            if(CY_HPI_MASTER_SUCCESS == Cy_HPI_Master_PortRegWrite(ptrHpiMasterContext,
                    ptrPortInfo->portAddr,
                    devPortIdx,
                    (uint8_t)CY_HPI_MASTER_PORT_REG_SOURCE_PDO_ADDR,
                    &defSrcCapMask,
                    (uint16_t)sizeof(defSrcCapMask)))
            {
                /* Clear pending HPI event received flag. */
                ptrPortInfo->eventRcvd = 0u;
            }
        }
        /* Update port state to wait for restore PDO response state. */
        ptrPortInfo->portState = (uint8_t)DPS_PORT_STATE_WAIT_SRC_PDO_RESTORE;

        /* Start timer with HPI response time. */
        (void)Cy_PdUtils_SwTimer_Start (context->ptrTimerContext,
                                    context,
                                    ptrPortInfo->timerId,
                                    DPS_HPI_RESPONSE_TIMER_PERIOD,
                                    &Cy_App_DfpPowerSharing_SrcPdoRestoreTimerCbk);
    }
}

/**
 * @brief Updates the source PDOs of the given port.
 */
static void Cy_App_DfpPowerSharing_UpdateSrcPdo(cy_stc_app_dps_context_t *context, uint8_t portNum, uint8_t devPortIdx)
{
    bool isStartTimer   = false;
    uint32_t timePeriod = DPS_PMG1_RETRY_TIMER_PERIOD;
    cy_stc_app_dps_port_info_t *ptrPortInfo = &context->portInfo[portNum];

    /* Check for DMC port. */
    if (0u == ptrPortInfo->portAddr)
    {
        cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t*)ptrPortInfo->portContext;

        /* Update source PDO using PD stack APIs and reset restore port number. */
        if ((CY_PDSTACK_STAT_SUCCESS == Cy_PdStack_Dpm_UpdateSrcCapMask(ptrPdStackContext, context->maxPdpSrcCapMask)) &&
            (CY_PDSTACK_STAT_SUCCESS == Cy_PdStack_Dpm_UpdateSrcCap(ptrPdStackContext, context->maxPdpSrcPdoCount, context->maxPdpSrcPdo)))
        {
            /* Send DPM command after updating source PDOs. */
            if (CY_PDSTACK_STAT_SUCCESS != Cy_PdStack_Dpm_SendPdCommand(ptrPdStackContext, CY_PDSTACK_DPM_CMD_SRC_CAP_CHNG,
                                                        NULL, false, NULL))
            {
                /* Set timer start parameters to handle source PDO update failure. */
                isStartTimer = true;
                timePeriod   = DPS_PMG1_RETRY_TIMER_PERIOD;

                /* Change the port state to idle.
                 * In case of DPM send command fail, the port state is updated in timer callback. */
                ptrPortInfo->portState  = (uint8_t)DPS_PORT_STATE_IDLE;
            }
            else
            {
                /* Update the port state to indicate Maximum PDP capabilities are advertised. */
                ptrPortInfo->portState = (uint8_t)DPS_PORT_STATE_MAX_PDP_ADVERTISED;
            }
        }
        else
        {
            /* Set timer start parameters to handle source PDO update failure. */
            isStartTimer = true;
            timePeriod   = DPS_PMG1_RETRY_TIMER_PERIOD;

            /* Change the port state to idle.
             * In case of DPM send command fail, the port state is updated in timer callback. */
            ptrPortInfo->portState  = (uint8_t)DPS_PORT_STATE_IDLE;
        }
    }
    else
    {
        uint32_t srcPdoSetData[DPS_SPR_SRC_PDO_UPDATE_BUF_SIZE] = {0u};
        cy_hpi_master_context_t *ptrHpiMasterContext = (cy_hpi_master_context_t *)ptrPortInfo->portContext;
        srcPdoSetData[0] = DPS_SPR_SRC_PDO_SIGNATURE;

        /* Copy source PDOs data from DFP power sharing context. */
        (void)memcpy((uint8_t *)&srcPdoSetData[1],(uint8_t *)&context->maxPdpSrcPdo[0], 
                                sizeof(uint32_t) * (size_t)context->maxPdpSrcPdoCount);

        /* Write source PDO information to PD data memory. */
        if (CY_HPI_MASTER_SUCCESS == Cy_HPI_Master_PdResponseRegWrite(ptrHpiMasterContext,
                                                    ptrPortInfo->portAddr,
                                                    devPortIdx,
                                                    0u,
                                                    &srcPdoSetData,
                                                    (uint16_t)sizeof(srcPdoSetData)))
        {
            /* Update the source PDO mask selection register. */
            if(CY_HPI_MASTER_SUCCESS == Cy_HPI_Master_PortRegWrite(ptrHpiMasterContext,
                    ptrPortInfo->portAddr,
                    devPortIdx,
                    (uint8_t)CY_HPI_MASTER_PORT_REG_SOURCE_PDO_ADDR,
                    &context->maxPdpSrcCapMask,
                    1u))
            {
                /* Clear pending HPI event received flag. */
                ptrPortInfo->eventRcvd = 0u;
            }
        }

        /* Update the port state to wait for source PDO response state. */
        ptrPortInfo->portState = (uint8_t)DPS_PORT_STATE_WAIT_SRC_PDO_UPDATE;

        /* Set timer start parameters for waiting HPI response or command failure. */
        timePeriod   = DPS_HPI_RESPONSE_TIMER_PERIOD;
        isStartTimer = true;
    }

    /* Start timer if the flag is set. */
    if (isStartTimer)
    {
        (void)Cy_PdUtils_SwTimer_Start (context->ptrTimerContext,
                        context,
                        ptrPortInfo->timerId,
                        (uint16_t)timePeriod,
                        &Cy_App_DfpPowerSharing_SrcPdoUpdateTimerCbk);
    }
}

/**
 * @brief Handles source PDOs update state machine.
 */
static void Cy_App_DfpPowerSharing_HandleSrcPdoUpdateFsm(cy_stc_app_dps_context_t *context, uint8_t portNum)
{
    cy_stc_app_dps_port_info_t *ptrPortInfo = &context->portInfo[portNum];
    uint8_t devPortIdx = context->ptrDfpPowerSharingCfg->dfp_port_config[portNum].device_port_idx;

    /* FSM to handle source PDO updates. */
    switch((cy_en_dps_port_state_t)context->portInfo[portNum].portState)
    {
        case DPS_PORT_STATE_SRC_PDO_UPDATE:
        {
            /* Handle update source PDO function. */
            Cy_App_DfpPowerSharing_UpdateSrcPdo(context, portNum, devPortIdx);
            break;
        }
        case DPS_PORT_STATE_WAIT_SRC_PDO_UPDATE:
        {
            /* Check for Non-DMC port and HPI event received flag. */
            if ((0u != ptrPortInfo->portAddr) && (0u != ptrPortInfo->eventRcvd))
            {
                /* Check for success response. */
                if ((uint8_t)CY_HPI_MASTER_RESPONSE_SUCCESS == ptrPortInfo->eventCode)
                {
                    /* Check for FCFS algorithm. */
                    if ((uint8_t)CY_APP_DPS_ALG_FCFS == context->ptrDfpPowerSharingCfg->power_sharing_method)
                    {
                        /* Update the port state to wait for PD contract in case of FCFS algorithm. */
                        context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_WAIT_FOR_PD_CONTRACT;
                        (void)Cy_PdUtils_SwTimer_Start (context->ptrTimerContext,
                                        context,
                                        context->portInfo[portNum].timerId,
                                        DPS_CCGX_RETRY_TIMER_PERIOD,
                                        &Cy_App_DfpPowerSharing_SrcPdoUpdateTimerCbk);
                    }
                    else
                    {
                        /* Update the port state to maximum PDP advertised state in case of other algorithms,
                         * because it does not matter when the PD contract happens. */
                        context->portInfo[portNum].portState  = (uint8_t)DPS_PORT_STATE_MAX_PDP_ADVERTISED;
                    }
                }
                else
                {
                    if (((uint8_t)CY_APP_DPS_ALG_FCFS == context->ptrDfpPowerSharingCfg->power_sharing_method) &&
                        (portNum == context->portNum))
                    {
                        /* In case of FCFS algorithm,
                         * -> Update the restore port number as it is required to update the
                         *    port with default PDO after failing to update with maximum PDP source PDOs.
                         * -> Update the port state to restore source PDO and
                         * -> Reset the port number associated with maximum PDP advertisement. */
                        context->restorePortNum = portNum;
                        context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_SRC_PDO_RESTORE;
                        context->portNum = DPS_INVALID_PORT_NUM;
                    }
                    else
                    {
                        /* For other algorithms, update port state update source PDO update again.  */
                        context->portInfo[portNum].portState  = (uint8_t)DPS_PORT_STATE_SRC_PDO_UPDATE;
                    }
                }

                /* Clear HPI event received flag. */
                ptrPortInfo->eventRcvd = 0u;
            }
            break;
        }
        case DPS_PORT_STATE_WAIT_FOR_PD_CONTRACT:
        {
            /* Check for Non-DMC port and HPI event received flag. */
            if ((0u != ptrPortInfo->portAddr) && (0u != ptrPortInfo->eventRcvd))
            {
                /* Check for PD contract negotiation complete event. */
                if ((uint8_t)CY_HPI_MASTER_EVENT_NEGOTIATION_COMPLETE == ptrPortInfo->eventCode)
                {
                    /* Update the port state to maximum PDP advertised. */
                    context->portInfo[portNum].portState  = (uint8_t)DPS_PORT_STATE_MAX_PDP_ADVERTISED;
                }
                else
                {
                    /* As the PD contract failed during maximum PDP update, reset the flags to
                     * start FCFS algorithm from the start. */
                    context->restorePortNum = portNum;
                    context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_SRC_PDO_RESTORE;
                    context->portNum = DPS_INVALID_PORT_NUM;
                }

                /* Clear HPI event received flag. */
                ptrPortInfo->eventRcvd = 0u;
            }
            break;
        }
        case DPS_PORT_STATE_SRC_PDO_RESTORE:
        {
            /* Handle restore source PDO function. */
            Cy_App_DfpPowerSharing_RestoreSrcPdo(context, portNum, devPortIdx);
            break;
        }
        case DPS_PORT_STATE_WAIT_SRC_PDO_RESTORE:
        {
            /* Check for Non-DMC port and HPI event received flag. */
            if ((0u != ptrPortInfo->portAddr) && (0u != ptrPortInfo->eventRcvd))
            {
                /* Check for success event. */
                if ((uint8_t)CY_HPI_MASTER_RESPONSE_SUCCESS == ptrPortInfo->eventCode)
                {
                    /* Reset the restore port number. */
                    context->restorePortNum = DPS_INVALID_PORT_NUM;
                }
                else
                {
                    /* As HPI response is not success, Start timer with HPI response time to retry. */
                    (void)Cy_PdUtils_SwTimer_Start (context->ptrTimerContext,
                                                context,
                                                ptrPortInfo->timerId,
                                                DPS_HPI_RESPONSE_TIMER_PERIOD,
                                                &Cy_App_DfpPowerSharing_SrcPdoRestoreTimerCbk);
                }

                /* Update port state to idle. */
                context->portInfo[portNum].portState  = (uint8_t)DPS_PORT_STATE_IDLE;

                /* Clear HPI event received flag. */
                ptrPortInfo->eventRcvd = 0u;
            }
            break;
        }
        default:
        {
            /* No operation. */
            break;
        }
    }
}

/**
 * @brief Set Capability mismatch bit for DMC (PMG1s#) Port-1 DFP port.
 */
void Cy_App_DfpPowerSharing_SetDmcPortCapMismatch(bool isCapMismatchSet)
{
    uint8_t portNum;
    cy_stc_app_dps_context_t *ptrDfpPowerSharingContext = gl_ptrDfpPowerSharingCtx;
    cy_stc_app_dps_port_info_t *ptrPortInfo = &ptrDfpPowerSharingContext->portInfo[0];
    uint8_t numDfpPorts = ptrDfpPowerSharingContext->ptrDfpPowerSharingCfg->num_dfp_ports;

    /* Parse through all DFP ports to identify the DMC DFP port. */
    if ((uint8_t)CY_APP_DPS_ALG_FCFS == ptrDfpPowerSharingContext->ptrDfpPowerSharingCfg->power_sharing_method)
    {
        for(portNum = 0u; portNum < numDfpPorts; portNum++)
        {
            if (0u == ptrPortInfo[portNum].portAddr)
            {
                if (isCapMismatchSet)
                {
                    ptrPortInfo[portNum].isCapMismatchSet = 1u;
                }
                else
                {
                    ptrPortInfo[portNum].isCapMismatchSet = 0u;
                    if (portNum == ptrDfpPowerSharingContext->portNum)
                    {
                        ptrDfpPowerSharingContext->restorePortNum = portNum;
                        ptrDfpPowerSharingContext->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_SRC_PDO_RESTORE;
                        ptrDfpPowerSharingContext->portNum = DPS_INVALID_PORT_NUM;
                    }
                }
                break;
            }
        }
    }
}

/**
 * @brief Check whether the port is active.
 */
static bool Cy_App_DfpPowerSharing_IsPortActive(cy_stc_app_dps_port_info_t *ptrPortInfo)
{
    bool isPortActive = false;
    cy_hpi_master_context_t *ptrHpiMasterContext;
    uint8_t readBuf;
    /* Check only for CCGx devices. */
    if (0u != ptrPortInfo->portAddr)
    {
        ptrHpiMasterContext = (cy_hpi_master_context_t *)ptrPortInfo->portContext;

        /* Read device mode register from CCGx device and check whether the device is not in boot mode. */
        if ((CY_HPI_MASTER_SUCCESS == Cy_HPI_Master_DevRegRead(ptrHpiMasterContext,
                                                          ptrPortInfo->portAddr,
                                                          (uint8_t)CY_HPI_MASTER_DEV_REG_DEVICE_MODE,
                                                          &readBuf,
                                                          (uint16_t)sizeof(readBuf))) &&
            (0u != (readBuf & 0x03u)))
        {
            isPortActive = true;
        }
    }

    return isPortActive;
}

/**
 * @brief PD event mask update callback handler.
 */
static void Cy_App_DfpPowerSharing_EventMaskUpdateCbk(cy_timer_id_t id, void *ptrContext)
{
    cy_stc_app_dps_context_t *context = (cy_stc_app_dps_context_t *)ptrContext;
    uint16_t portNum = id - context->portInfo[0].timerId;

    /* Set port state to update event mask state. */
    context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_PD_EVT_MASK_UPDATE;
}

/**
 * @brief Updates PD event mask of the CCGx device.
 */
static void Cy_App_DfpPowerSharing_UpdatePdEventMask(cy_stc_app_dps_context_t *context, uint8_t portNum)
{
    cy_hpi_master_context_t *ptrHpiMasterContext = (cy_hpi_master_context_t *)context->portInfo[portNum].portContext;
    uint8_t devPortIdx = context->ptrDfpPowerSharingCfg->dfp_port_config[portNum].device_port_idx;
    /* As the event mask bits which need update are available in first 2 bytes of Event mask,
     * only 2B is updated. */
    uint16_t hpiPdEventMask = DPS_HPI_PD_EVENT_MASK;

    /* Write to PD Event register mask. */
    (void)Cy_HPI_Master_PortRegWrite (ptrHpiMasterContext,
                                        context->portInfo[portNum].portAddr,
                                        devPortIdx,
                                        (uint8_t)CY_HPI_MASTER_PORT_REG_EVENT_MASK,
                                        &hpiPdEventMask,
                                        (uint16_t)sizeof(hpiPdEventMask));

    /* Set port state to wait for update event mask state. */
    context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_WAIT_PD_EVT_MASK_UPDATE;
    context->portInfo[portNum].eventRcvd = 0u;

    /* Start timer with HPI response time. */
    (void)Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context,
                                context->portInfo[portNum].timerId,
                                DPS_HPI_RESPONSE_TIMER_PERIOD,
                                &Cy_App_DfpPowerSharing_EventMaskUpdateCbk);
}

/**
 * @brief Handles device ready FSM.
 */
static void Cy_App_DfpPowerSharing_DeviceReadyFsm(cy_stc_app_dps_context_t *context, uint8_t portNum)
{
    /* Reset the port number to 0 after reaching maximum ports. */
    cy_stc_app_dps_port_info_t *ptrPortInfo = &context->portInfo[portNum];
    uint8_t portState = ptrPortInfo->portState;

    /* Port Ready states handling. */
    switch ((cy_en_dps_port_state_t)portState)
    {
        case DPS_PORT_STATE_PD_EVT_MASK_UPDATE:
        {
            /* Check for port ready flag. */
            if (0u == ptrPortInfo->isPortReady)
            {
                /* Check for port active status. */
                if (Cy_App_DfpPowerSharing_IsPortActive(ptrPortInfo))
                {
                    /* Handle PD event mask update if port is active. */
                    Cy_App_DfpPowerSharing_UpdatePdEventMask(context, portNum);

                    /* Set port state to wait for PD event mask update. */
                    portState = (uint8_t)DPS_PORT_STATE_WAIT_PD_EVT_MASK_UPDATE;
                }
                else
                {
                    /* Start timer to check port active status again. */
                    (void)Cy_PdUtils_SwTimer_Start(context->ptrTimerContext, context,
                                                context->portInfo[portNum].timerId,
                                                DPS_CCGX_RETRY_TIMER_PERIOD,
                                                &Cy_App_DfpPowerSharing_EventMaskUpdateCbk);

                    /* Set port state to idle. */
                    portState = (uint8_t)DPS_PORT_STATE_IDLE;
                }
            }
            else
            {
                /* Set port state to update source PDO. */
                portState = (uint8_t)DPS_PORT_STATE_SRC_PDO_UPDATE;
            }
            break;
        }

        case DPS_PORT_STATE_WAIT_PD_EVT_MASK_UPDATE:
        {
            /* Check for HPI event received status. */
            if (0u != ptrPortInfo->eventRcvd)
            {
                /* Check for success event. */
                if ((uint8_t)CY_HPI_MASTER_RESPONSE_SUCCESS == ptrPortInfo->eventCode)
                {
                    /* Set port ready flag. */
                    ptrPortInfo->isPortReady = 1u;
                    /* Set port state to update source PDO. */
                    portState = (uint8_t)DPS_PORT_STATE_SRC_PDO_UPDATE;
                }
                else
                {
                    /* Set port state to PD event mask update. */
                    portState = (uint8_t)DPS_PORT_STATE_PD_EVT_MASK_UPDATE;
                }

                /* Clear HPI event received flag. */
                ptrPortInfo->eventRcvd = 0u;
            }
            break;
        }
        default:
        {
            /* Nothing to do */
            break;
        }
    }

    /* Restore the port state in global port information. */
    ptrPortInfo->portState = portState;
}

/**
 * @brief Implements a power sharing algorithm based on DFP power sharing configuration.
 */
void Cy_App_DfpPowerSharing_Task(cy_stc_app_dps_context_t *context)
{
    uint8_t portNum;

    /* Reset the global port number after reaching maximum DFP ports configured. */
    if (gl_dfpPortNum >= context->ptrDfpPowerSharingCfg->num_dfp_ports)
    {
        gl_dfpPortNum = 0u;
    }

    /* Handle device ready FSM. */
    Cy_App_DfpPowerSharing_DeviceReadyFsm(context, gl_dfpPortNum);

    /* Handle power sharing algorithm. */
    switch ((cy_en_app_dps_alg_t)context->ptrDfpPowerSharingCfg->power_sharing_method)
    {
        case CY_APP_DPS_ALG_FIXED_PRIORITY:
        {
            if (DPS_INVALID_PORT_NUM == context->portNum)
            {
                /* For fixed priority, the port number associated with the maximum PDP is
                 * obtained from the DFP power sharing configuration. */
                portNum = context->ptrDfpPowerSharingCfg->fixed_priority_dfp_port;

                /* Update port number for which the maximum PDP needs to be advertised. */
                context->portNum = portNum;

                /* Check whether power sharing is enabled or not for the given port. */
                if (0u == context->ptrDfpPowerSharingCfg->dfp_port_config[portNum].power_sharing_enable)
                {
                    /* As the algorithm is Fixed Priority and the required port is disabled to participate in power sharing,
                     * there is no need to advertise the maximum PDP source capabilities.  */
                    context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_MAX_PDP_ADVERTISED;
                }
            }
            /* Execute source PDO update state machine after identifying power sharing port. */
            if ((DPS_INVALID_PORT_NUM != context->portNum) &&
                (0u != context->portInfo[context->portNum].isPortReady) &&
                ((uint8_t)DPS_PORT_STATE_MAX_PDP_ADVERTISED != context->portInfo[context->portNum].portState))
            {
                /* Handle source PDO update if the device active. */
                Cy_App_DfpPowerSharing_HandleSrcPdoUpdateFsm(context, context->portNum);
            }
            break;
        }
        case CY_APP_DPS_ALG_FCFS:
        {
            if ((DPS_INVALID_PORT_NUM == context->portNum) &&
                (!Cy_PdUtils_SwTimer_IsRunning(context->ptrTimerContext, context->idleTimerId)))
            {
                /* DFP power sharing port is not defined, hence check for capability mismatch field for all the DFP ports. */
                for (portNum = 0u; portNum < context->ptrDfpPowerSharingCfg->num_dfp_ports; portNum++)
                {
                    /* Check whether power sharing is enabled or not for the given port. */
                    if (context->ptrDfpPowerSharingCfg->dfp_port_config[portNum].power_sharing_enable != 0u)
                    {
                        /* Check for capability mismatch bit. */
                        if (0u != context->portInfo[portNum].isCapMismatchSet)
                        {
                            /* Update port number for which the maximum PDP needs to be advertised. */
                            context->portNum = portNum;
                            /* Set source PDO update state to update the source PDOs. */
                            context->portInfo[portNum].portState = (uint8_t)DPS_PORT_STATE_SRC_PDO_UPDATE;
                        }
                    }
                }

                /* Start timer to check for the capability mismatch again after 1 second
                 * if the capability mismatch bit is not set for any port. */
                if(DPS_INVALID_PORT_NUM == context->portNum)
                {
                    (void)Cy_PdUtils_SwTimer_StartWocb(context->ptrTimerContext,
                                                 context->idleTimerId,
                                                 DPS_IDLE_TIMER_PERIOD);
                }
            }
            /* Execute source PDO update state machine after identifying power sharing port. */
            portNum = context->portNum;
            if ((DPS_INVALID_PORT_NUM != portNum) &&
                ((uint8_t)DPS_PORT_STATE_MAX_PDP_ADVERTISED != context->portInfo[portNum].portState))
            {
                /* Handle source PDO update. */
                Cy_App_DfpPowerSharing_HandleSrcPdoUpdateFsm(context, portNum);
            }

            /* Handle restore source PDO if restore port number is not invalid. */
            if (DPS_INVALID_PORT_NUM != context->restorePortNum)
            {
                portNum = context->restorePortNum;
                Cy_App_DfpPowerSharing_HandleSrcPdoUpdateFsm(context, portNum);
            }
            break;
        }
        case CY_APP_DPS_ALG_EQUAL_PRIORITY:
        {
            cy_stc_app_dps_port_info_t *ptrPortInfo = &context->portInfo[gl_dfpPortNum];

            /* Update the maximum PDP for the current DFP port. */
            if ((context->ptrDfpPowerSharingCfg->dfp_port_config[gl_dfpPortNum].power_sharing_enable != 0u) &&
                (0u != ptrPortInfo->isPortReady) &&
                ((uint8_t)DPS_PORT_STATE_MAX_PDP_ADVERTISED != ptrPortInfo->portState))
            {
                /* Handle source PDO update FSM. */
                Cy_App_DfpPowerSharing_HandleSrcPdoUpdateFsm(context, gl_dfpPortNum);
            }
            break;

        }
        default:
            /* Nothing to do */
            break;
    }

    /* Increment the global port number. */
    gl_dfpPortNum++;
}
#endif /* CY_APP_DFP_POWER_SHARING_ENABLE */

/* [] End of file */
