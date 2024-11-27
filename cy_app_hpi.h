/***************************************************************************//**
* \file cy_app_hpi.h
* \version 2.0
*
* \brief
* Defines data structures and function prototypes associated with
* HPI PD response read/write operation.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_HPI_H_
#define _CY_APP_HPI_H_

/*******************************************************************************
 * Header files including
 ******************************************************************************/
#include <stdint.h>
#include "cy_app_debug.h"
#include "cy_app_config.h"

#if ((CY_HPI_ENABLED && CY_HPI_RW_PD_RESP_MSG_DATA) || (DOXYGEN))
#include "cy_hpi.h"


/*****************************************************************************
 * Macros
 *****************************************************************************/
/**
* \addtogroup group_pmg_app_common_hpi
* \{
* This module provides APIs and data structures for read/write the PD response register.
* These APIs are registered as application callbacks and used by the HPI middleware library.
*
* <b>Features:</b>
* * Read/write battery status
* * Read/write battery capabilities
* * Read/write source Info and PD revision message
*
********************************************************************************
* \section section_pmg_app_common_hpi Configuration considerations
********************************************************************************
* The following steps describe the usage of these APIs.
*
* 1. Include cy_app_hpi.h to get access to all functions and other declarations.
*    \snippet hpi_sut.c snippet_hpi_include
* 
* 2. Make sure the Host Processor Interface (HPI) middleware library is configured
*    and initialized before using these APIs.<br>
*    See the [HPI middleware API reference manual](https://infineon.github.io/hpi/html/index.html)
*
* 3. Initializes the HPI application callback struct member hpi_rw_pd_resp_data.
*    \snippet hpi_sut.c snippet_hpi_pdrespdata_callback
*
* 4. The following example demonstrates the steps required to store data into the response buffer.
*    \snippet hpi_sut.c snippet_hpi_response_write
*
* 5. The following example demonstrates the steps required to retrieve data from the
*    response buffer.
*    \snippet hpi_sut.c snippet_hpi_response_retrieve
*
*
* \defgroup group_pmg_app_common_hpi_macros Macros
* \defgroup group_pmg_app_common_hpi_enums Enumerated types
* \defgroup group_pmg_app_common_hpi_data_structures Data structures
* \defgroup group_pmg_app_common_hpi_functions Functions
*/

/**
* \addtogroup group_pmg_app_common_hpi_macros
* \{
*/

#define CY_APP_PD_RESP_DATA_BUFF_LEN                           (128u)
/**< Size allocated for storing generic PD response data. */

#define CY_APP_PD_RESP_MIN_DATA_LEN                            (5u)
/**< Minimum PD response data length that is allowed to be stored. */

#define CY_APP_PD_RESP_DATA_HDR_SIZE                           (4u)
/**< PD response header size stored along with data. */

#define CY_APP_PD_RESP_DATA_PORT_FLAG_MSK                      (0x11u)
/**< Mask for selecting port-specific response data. */

#define CY_APP_PD_RESP_DATA_PORT_FLAG_POS                      (4u)
/**< Position for selecting port-specific response data. */

#define CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE                     (0x03u)
/**< Value to be added to make the length to align to 4 bytes. */

#define CY_APP_PD_RESP_MATCH_LENGTH                            (2u)
/**< Length of PD response data to be matched for record to be same. */

#define CY_APP_PD_RESP_MATCH_LENGTH_W_HDR                      (2u)
/**< Length of PD response data to be matched for record to be same with 4-byte header. */

#define CY_APP_PD_RESP_MATCH_LENGTH_WO_HDR                     (0u)
/**< Length of PD response data to be matched for record to be same without header. */

#define CY_APP_PD_RESP_MIN_DATA_LEN_W_HDR                      (5u)
/**< Minimum PD response data length that is allowed to be stored with 4-byte header. */

#define CY_APP_PD_RESP_MIN_DATA_LEN_WO_HDR                     (1u)
/**< Minimum PD response data length that is allowed to be stored without header. */

#define CY_APP_PD_RESP_INVLD_BAT_SLOT_ID                       (0x00u)
/**< Battery slot ID reserved for invalid battery reference so that EC can populate the response. */

#define CY_APP_PD_RESP_INVLD_BATT_REF_FLAG                     (0x01u)
/**< Flag to indicate the invalid battery reference. */

#define CY_APP_PD_RESP_CMD_SRC_INFO_LEN                        (4u)
/**< Minimum length of source information. */

#define CY_APP_PD_RESP_CMD_REV_MSG_LEN                         (4u)
/**< Minimum length of PD Revision message. */

/** \} group_pmg_app_common_hpi_macros */

/*****************************************************************************
 * Data struct definition
 ****************************************************************************/
/**
* \addtogroup group_pmg_app_common_hpi_enums
* \{
*/

/**
 * @typedef cy_en_app_pd_resp_id_t
 * @brief List of PD response IDs that are supported.
 */
typedef enum
{
    CY_APP_PD_RESP_ID_RESERVED = 0x00u,     /**< Reserved PD Response ID. */
    CY_APP_PD_RESP_ID_BAT_STAT,             /**< Battery Status PD Response ID. */
    CY_APP_PD_RESP_ID_BAT_CAP,              /**< Battery Capabilities Response ID. */
    CY_APP_PD_RESP_ID_SRC_INFO,             /**< Source Info Response ID. */
    CY_APP_PD_RESP_ID_PD_REV_MSG,           /**< PD Revision message Response ID. */
    CY_APP_PD_RESP_ID_MAX_NUM               /**< Maximum number of Allowed IDs for PD response data. */
}cy_en_app_pd_resp_id_t;

/**
 * @typedef cy_en_app_pd_resp_data_cmd_t
 * @brief List of PD response data commands.
 */
typedef enum
{
    CY_APP_PD_RESP_DATA_CMD_WRITE = 0x00u,  /**< Write PD response data command. */
    CY_APP_PD_RESP_DATA_CMD_READ,           /**< Read PD response data command. */
    CY_APP_PD_RESP_DATA_CMD_DELETE,         /**< Delete PD response data command. */
}cy_en_app_pd_resp_data_cmd_t;


/** \} group_pmg_app_common_hpi_enums */

/**
* \addtogroup group_pmg_app_common_hpi_data_structures
* \{
*/

/**
 * @brief This structure to store the PD response data command.
 */
typedef struct
{
    uint8_t respId;                         /**< Command response ID. */
    union
    {
        uint8_t cmdVal;
        struct
        {
            uint8_t dataCmd:2;              /**< Data command code. */
            uint8_t portFlag:1;             /**< Port-specific flag. */
            uint8_t :0;
        };
    };
    uint8_t respLen;                            /**< PD response data length. */
    uint8_t reserved;                           /**< Reserved. */
    uint8_t respData[CY_APP_PD_RESP_DATA_HDR_SIZE];    /**< Start of data. Length of data is dependent on respLen. */
}cy_stc_app_pd_resp_data_t;

/**
 * @brief Structure to store the response in the internal buffer.
 */
typedef struct
{
    uint8_t respId;                     /**< PD response ID. */
    uint8_t portFlag;                   /**< PD port flag. */
    uint8_t respLen;                    /**< PD response length. */
    uint8_t reserved;                   /**< Reserved for alignment. */
    uint8_t respData;                   /**< Start of data. Length of data is dependent on respLen. */
}cy_stc_app_pd_resp_stored_t;

/** \} group_pmg_app_common_hpi_data_structures */

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/
/**
* \addtogroup group_pmg_app_common_hpi_functions
* \{
*/

/**
 * @brief Gets the PD response minimum length.
 *
 * This function returns the minimum length hold by PD response command.
 *
 * @param cmd PD response command ID.
 * @return PD response command minimum length.
 */
const uint8_t Cy_App_Hpi_GetResponseMinLen(uint8_t cmd);

/**
 * @brief Gets the PD response match length.
 *
 * This function returns the match length of the specific PD response command.
 *
 * @param cmd PD response command ID.
 * @return PD response command match length.
 */
const uint8_t Cy_App_Hpi_GetResponseMatchLen(uint8_t cmd);

/**
 * @brief Stores the response into the buffer.
 *
 * This function is used to store the response in the common buffer so that it can be retrieved later.
 *
 * @param ptrPdRespData Pointer to the PD response command data.
 * @param matchLen Length of the data to be matched.
 *
 * @return
 * Pointer to the stored data if response is stored successfully.
 * NULL if the response cannot be stored due to insufficient memory in the common buffer.
 *
 */
cy_stc_app_pd_resp_stored_t * Cy_App_Hpi_StorePdRespData(cy_stc_app_pd_resp_data_t *ptrPdRespData,
                                                         uint8_t matchLen);

/**
 * @brief Retrieves the response from the buffer.
 *
 * This function is used to get the response from the common buffer.
 *
 * @param ptrPdRespData Pointer to the PD response command data.
 * @param matchLen Length of the data to be matched.
 *
 * @return
 * Pointer to the retrieved data if response is found.
 * NULL if the response cannot be found.
 *
 */
cy_stc_app_pd_resp_stored_t * Cy_App_Hpi_RetrievePdRespData(cy_stc_app_pd_resp_data_t *ptrPdRespData,
                                                            uint8_t matchLen);

/**
 * @brief Deletes the response from the buffer.
 *
 * This function is used to delete the response from the common buffer.
 *
 * @param ptrPdRespData Pointer to the PD response command data.
 * @param matchLen Length of the data to be matched.
 *
 * @return
 * Pointer to the deleted data if response is found.
 * NULL if the response cannot be found.
 *
 */
cy_stc_app_pd_resp_stored_t * Cy_App_Hpi_DeletePdRespData(cy_stc_app_pd_resp_data_t *ptrPdRespData,
                                                          uint8_t matchLen);

/**
 * @brief Handles the HPI PD control command for R/W PD response data.
 *
 * This function is used to handle the PD control command form the EC via HPI.
 *
 * @param ptrHpiContext Pointer to the HPI context.
 * @param ptrPdStackContext Pointer to the PD stack context.
 * @param data Pointer to the R/W PD response data command.
 *
 * @return
 * CY_PDSTACK_STAT_SUCCESS If the command handling is success.
 * CY_PDSTACK_STAT_INVALID_ARGUMENT If the arguments are invalid or the response cannot
 * be stored or the response is not found.
 *
 */
cy_en_pdstack_status_t Cy_App_Hpi_HandlePdRespDataRw(cy_stc_hpi_context_t *ptrHpiContext,
                                                     cy_stc_pdstack_context_t *ptrPdStackContext,
                                                     uint8_t *data);
/** \} group_pmg_app_common_hpi_functions */
/** \} group_pmg_app_common_hpi */

#endif /* ((CY_HPI_ENABLED && CY_HPI_RW_PD_RESP_MSG_DATA) || (DOXYGEN)) */
#endif /* _CY_APP_HPI_H_ */

/* [] END OF FILE */
