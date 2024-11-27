/***************************************************************************//**
* \file cy_app_hpi.c
* \version 2.0
*
* \brief
* Implements functions for handling the HPI PD response register read/write operation.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#if (CY_HPI_ENABLED && CY_HPI_RW_PD_RESP_MSG_DATA)

#include "cy_hpi.h"
#include "cy_app_hpi.h"
#include "cy_pdstack_common.h"


/** Data buffer to store the PD responses.
 * This buffer has to be aligned to 4-byte boundary. Otherwise, it results in hardfault.
 * Increase the buffer size in case more number responses need to be stored.
 */
static uint8_t glPdRespMsgData [CY_APP_PD_RESP_DATA_BUFF_LEN];


/**< Constant to hold the command specific minimum length. */
static const uint8_t gl_pdRespMinLen[CY_APP_PD_RESP_ID_MAX_NUM] =
{
    CY_APP_PD_RESP_MIN_DATA_LEN_WO_HDR,        /* Reserved PD Response ID min length is 1. Not used. */
    CY_APP_PD_RESP_MIN_DATA_LEN_W_HDR,         /* Battery Status PD Response ID min length is 5 with header. */
    CY_APP_PD_RESP_MIN_DATA_LEN_W_HDR,         /* Battery Capabilities Response ID min length is 5 with header. */
    CY_APP_PD_RESP_MIN_DATA_LEN_WO_HDR,        /* Source Info Response ID min length is 1 without header. */
    CY_APP_PD_RESP_MIN_DATA_LEN_WO_HDR         /* PD Revision (message) Response ID min length is 1 without header. */
};

/**< Constant to hold the response ID-specific match length for data. */
static const uint8_t gl_pdRespMatchLen[CY_APP_PD_RESP_ID_MAX_NUM] =
{
    CY_APP_PD_RESP_MATCH_LENGTH_WO_HDR,         /* Reserved PD Response ID match length is 0. Not used. */
    CY_APP_PD_RESP_MATCH_LENGTH_W_HDR,          /* Battery Status PD Response ID match length is 2 bytes. */
    CY_APP_PD_RESP_MATCH_LENGTH_W_HDR,          /* Battery Capabilities PD Response ID match length is 2 bytes. */
    CY_APP_PD_RESP_MATCH_LENGTH_WO_HDR,         /* Source Info Response ID match length is 0 bytes. */
    CY_APP_PD_RESP_MATCH_LENGTH_WO_HDR          /* PD Revision (Message) Response ID match length is 0 bytes. */
};

/**
 * @brief Gets the match index for the response ID.
 *
 * This function is used to get the match index if the response is already present in the buffer.
 * If the response is not found, it returns the index where the response can be stored.
 *
 * @param ptrPdRespData Pointer to the PD response command data.
 * @param matchLen Length of the data to be matched.
 *
 * @return
 * MatchIndex if the match is found.
 * StoreIndex if the match is not found but the response can be stored.
 * InvalidIndex if the response cannot be stored.
 *
 */
static uint8_t Cy_App_Hpi_GetPdRespMatchIndex(cy_stc_app_pd_resp_data_t *ptrPdRespData, uint8_t matchLen)
{
    uint8_t matchIdx = CY_APP_PD_RESP_DATA_BUFF_LEN - CY_APP_PD_RESP_DATA_HDR_SIZE;
    uint8_t index = 0u;
    /* Gets the index where the new field can be stored. */
    while (index < CY_APP_PD_RESP_DATA_BUFF_LEN - CY_APP_PD_RESP_DATA_HDR_SIZE)
    {
        cy_stc_app_pd_resp_stored_t *ptrPdRespTmp = (cy_stc_app_pd_resp_stored_t *)&glPdRespMsgData[index];
        /* Gets the local response ID and length. */
        uint8_t localRespId = ptrPdRespTmp->respId;
        uint8_t localRespLen = ptrPdRespTmp->respLen;

        /* Check if the length at this index is non-zero or not. */
        if (localRespLen == 0u)
        {
            /* No more records found. Return this as match index as there will be additional check for ID. */
            if (matchIdx == (CY_APP_PD_RESP_DATA_BUFF_LEN - CY_APP_PD_RESP_DATA_HDR_SIZE))
            {
                matchIdx = index;
            }
            break;
        }
        else
        {
            /* Round up the response length. */
            localRespLen += CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE;
            localRespLen &= (uint8_t)(~CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE);

            /* Checks if the respId is reserved, then it indicates as an available slot. */
            if (localRespId == CY_APP_PD_RESP_ID_RESERVED)
            {
                /* Checks if this record can be fit in here. */
                uint8_t matchLen = ptrPdRespData->respLen;
                matchLen += CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE;
                matchLen &= (uint8_t)(~CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE);
                if (0x00u == (localRespLen - matchLen))
                {
                    /* Responses can be fit in. However, search for other records as well.*/
                    matchIdx = index;
                }
                else
                {
                    if (localRespLen > (matchLen + CY_APP_PD_RESP_DATA_HDR_SIZE))
                    {
                        /* Responses can be fit in. However, search for other records as well.*/
                        /* Checks if an exact match is not found. */
                        if (matchIdx == (CY_APP_PD_RESP_DATA_BUFF_LEN - CY_APP_PD_RESP_DATA_HDR_SIZE))
                        {
                            matchIdx = index;
                        }
                    }
                }
            }
            else
            {
                /* Checks if the respId is matching. */
                if (localRespId == ptrPdRespData->respId)
                {
                    uint8_t i;
                    /* Checks if the data is matching. */
                    for (i = 0u; i < matchLen; i ++)
                    {
                        uint8_t *ptrData = (uint8_t*)&ptrPdRespTmp->respData;
                        uint8_t *respData = (uint8_t*)&ptrPdRespData->respData;
                        if (ptrData[i] != respData[i])
                        {
                            break;
                        }
                    }
                    if (i == matchLen)
                    {
                        uint8_t portCheck = ptrPdRespData->cmdVal ^ ptrPdRespTmp->portFlag;
                        portCheck &= (uint8_t)(portCheck >> CY_APP_PD_RESP_DATA_PORT_FLAG_POS);
                        if (false == portCheck)
                        {
                            /* Matches found, then replace the existing response. */
                            matchIdx = index;
                            break;
                        }
                    }
                    else
                    {
                        /* Not matched with the resp data. Therefore, continue the search. */
                    }
                }
            }
        }
        /* Increments the index to next aligned length + header size. */
        index += localRespLen + CY_APP_PD_RESP_DATA_HDR_SIZE;
    }
    return matchIdx;
}

/**
 * @brief Resets the indices in the PD response data memory after delete or write.
 * This is to avoid memory fragmentation.
 *
 * This function is used to reset the indices in the PD response memory buffer.
 * It merges the multiple free records into a single record so that it can be used next time with high size.
 * If the record is at the last then it clears that record to make it available.
 *
 * @return
 * None.
 *
 */
static void Cy_App_Hpi_ResetPdRespIdx (void)
{
    uint8_t index = 0u;
    uint8_t prev_free_idx = CY_APP_PD_RESP_DATA_BUFF_LEN;
    /* Gets the index where the new field can be stored. */
    while (index < CY_APP_PD_RESP_DATA_BUFF_LEN - CY_APP_PD_RESP_DATA_HDR_SIZE)
    {
        cy_stc_app_pd_resp_stored_t *ptrPdRespTmp = (cy_stc_app_pd_resp_stored_t *)&glPdRespMsgData[index];
        /* Gets the local response ID and length. */
        uint8_t localRespId = ptrPdRespTmp->respId;
        uint8_t localRespLen = ptrPdRespTmp->respLen;

        /* Checks if the length at this index is non-zero or not. */
        if (localRespLen == 0u)
        {
            if(CY_APP_PD_RESP_DATA_BUFF_LEN != prev_free_idx)
            {
                /* No more records found. Clear the length to make it available. */
                cy_stc_app_pd_resp_stored_t *ptrPdRespPrev = (cy_stc_app_pd_resp_stored_t *)&glPdRespMsgData[prev_free_idx];
                ptrPdRespPrev->respLen = 0x00u;
            }
            else
            {
                /* No previous free records found. */
            }
            break;
        }
        else
        {
            /* Checks if the respId is reserved, then it indicates as an available slot. */
            if (CY_APP_PD_RESP_ID_RESERVED == localRespId)
            {
                if(CY_APP_PD_RESP_DATA_BUFF_LEN == prev_free_idx)
                {
                    prev_free_idx = index;
                }
                else
                {
                    /* This is not the first free record. */
                }
            }
            else
            {
                /* Response is occupied. Therefore, merge the previous free records. */
                if(CY_APP_PD_RESP_DATA_BUFF_LEN != prev_free_idx)
                {
                    uint8_t newRespLen = index - prev_free_idx - CY_APP_PD_RESP_DATA_HDR_SIZE;
                    cy_stc_app_pd_resp_stored_t *ptrPdRespPrev = (cy_stc_app_pd_resp_stored_t *)&glPdRespMsgData[prev_free_idx];
                    ptrPdRespPrev->respLen = newRespLen;
                    /* Invalidate the previous free index.*/
                    prev_free_idx = CY_APP_PD_RESP_DATA_BUFF_LEN;
                }
                else
                {
                    /* No previous free records. */
                }
            }
        }
        /* Increments the index to next aligned length + header size. */
        {
            uint8_t lenAlign = localRespLen + CY_APP_PD_RESP_DATA_HDR_SIZE + CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE;
            lenAlign &= (uint8_t)(~CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE);
            index += lenAlign;
        }
    }
}

/**
 * @brief Gets the PD response minimum length.
 *
 * This function returns the minimum length held by the PD response command.
 *
 * @param cmd PD response command ID.
 * @return PD response command minimum length.
 */
const uint8_t Cy_App_Hpi_GetResponseMinLen(uint8_t cmd)
{
    return cmd < CY_APP_PD_RESP_ID_MAX_NUM ? gl_pdRespMinLen[cmd] : 0u ;
}

/**
 * @brief Gets the PD response match length.
 *
 * This function returns the match length of the specific PD response command.
 *
 * @param cmd PD response command ID.
 * @return PD response command match length.
 */
const uint8_t Cy_App_Hpi_GetResponseMatchLen(uint8_t cmd)
{
    return cmd < CY_APP_PD_RESP_ID_MAX_NUM ? gl_pdRespMatchLen[cmd] : 0u ;
}

/**
 * @brief Stores the response into the buffer.
 *
 * This function is used to store the response in the common buffer so that it can be retrieved later.
 *
 * @param ptrPdRespData Pointer to the PD response command data.
 * @param matchLen Length of the data to be matched.
 *
 * @return
 * Pointer to the stored data if the response is stored successfully.
 * NULL if the response cannot be stored due to insufficient memory in the common buffer.
 *
 */
cy_stc_app_pd_resp_stored_t * Cy_App_Hpi_StorePdRespData(cy_stc_app_pd_resp_data_t *ptrPdRespData, uint8_t matchLen)
{
    cy_stc_app_pd_resp_stored_t *ptrPdResp = NULL;
    /* Gets the index where the new field can be stored. */
    uint8_t storeIdx = Cy_App_Hpi_GetPdRespMatchIndex(ptrPdRespData, matchLen);
    uint8_t newLen = (ptrPdRespData->respLen + CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE) & (uint8_t)(~CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE);

    if (storeIdx < CY_APP_PD_RESP_DATA_BUFF_LEN - CY_APP_PD_RESP_DATA_HDR_SIZE)
    {
        ptrPdResp = (cy_stc_app_pd_resp_stored_t *)&glPdRespMsgData[storeIdx];
        /* Checks if the record already exists. */
        if (ptrPdResp->respId == ptrPdRespData->respId)
        {
            /* Checks if the new length is matching with the old one. */
            uint8_t oldLen = (ptrPdResp->respLen + CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE) & (uint8_t)(~CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE);
            if (oldLen == newLen)
            {
                ;/* Replaces the old one.*/
            }
            else
            {
                /* Deletes the old one and find a new place. */
                ptrPdResp->respId = CY_APP_PD_RESP_ID_RESERVED;
                Cy_App_Hpi_ResetPdRespIdx();
                storeIdx = Cy_App_Hpi_GetPdRespMatchIndex(ptrPdRespData, matchLen);
                if ((CY_APP_PD_RESP_DATA_BUFF_LEN - CY_APP_PD_RESP_DATA_HDR_SIZE - storeIdx) >= ptrPdRespData->respLen)
                {
                    /* Place available. Therefore, store it. */
                    ptrPdResp = (cy_stc_app_pd_resp_stored_t *)&glPdRespMsgData[storeIdx];
                }
                else
                {
                    /* No space available for storing this response. */
                    ptrPdResp = NULL;
                }
            }
        }
        else
        {
            if ((CY_APP_PD_RESP_DATA_BUFF_LEN - CY_APP_PD_RESP_DATA_HDR_SIZE - storeIdx) < ptrPdRespData->respLen)
            {
                ptrPdResp = NULL;
            }
        }
    }
    /* Checks if memory is available for storing. */
    if (NULL != ptrPdResp)
    {
        /* Stores the response. */
        uint8_t *ptrData = (uint8_t *)ptrPdResp;
        uint8_t *respData = (uint8_t *)ptrPdRespData;
        uint8_t oldLen = (ptrPdResp->respLen + CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE) & (uint8_t)(~CY_APP_PD_RESP_ALIGN_LEN_ADD_VALUE);
        /* If the record is stored at the end then clear the next record len and ID. */
        if (0x00u == ptrPdResp->respLen)
        {
            /* The next item shall be set to zero. */
            uint8_t nextIdx = storeIdx + newLen + CY_APP_PD_RESP_DATA_HDR_SIZE;
            if (nextIdx < CY_APP_PD_RESP_DATA_BUFF_LEN - CY_APP_PD_RESP_DATA_HDR_SIZE)
            {
                cy_stc_app_pd_resp_stored_t * ptrNxtPdResp = (cy_stc_app_pd_resp_stored_t *)&glPdRespMsgData[nextIdx];
                ptrNxtPdResp->respLen = 0x00u;
                ptrNxtPdResp->respId = CY_APP_PD_RESP_ID_RESERVED;
            }
        }
        /* If the allocated size is higher then split into two. */
        if (oldLen > newLen)
        {
            cy_stc_app_pd_resp_stored_t *splitPdresp = (cy_stc_app_pd_resp_stored_t *)(ptrData + newLen);
            splitPdresp->respId = CY_APP_PD_RESP_ID_RESERVED;
            splitPdresp->respLen = oldLen - newLen;
        }
        for (uint8_t i = 0u; i < ptrPdRespData->respLen + CY_APP_PD_RESP_DATA_HDR_SIZE; i ++)
        {
            ptrData[i] = respData[i];
        }
    }
    return ptrPdResp;
}

/**
 * @brief Retrieves the response from the buffer.
 *
 * This function is used to get the response from the common buffer.
 *
 * @param ptrPdRespData Pointer to the PD response command data.
 * @param matchLen Length of the data to be matched.
 *
 * @return
 * Pointer to the retrieved data if a response is found.
 * NULL if the response cannot be found.
 *
 */
cy_stc_app_pd_resp_stored_t * Cy_App_Hpi_RetrievePdRespData(cy_stc_app_pd_resp_data_t *ptrPdRespData, uint8_t matchLen)
{
    cy_stc_app_pd_resp_stored_t *ptrPdResp = NULL;
    /* Gets the index where the response is stored. */
    uint8_t retrieveIdx = Cy_App_Hpi_GetPdRespMatchIndex(ptrPdRespData, matchLen);
    if (retrieveIdx < CY_APP_PD_RESP_DATA_BUFF_LEN - CY_APP_PD_RESP_DATA_HDR_SIZE)
    {
        cy_stc_app_pd_resp_stored_t *ptrPdRespTmp = (cy_stc_app_pd_resp_stored_t *)&glPdRespMsgData[retrieveIdx];
        /* Checks if the response ID is matching to check if the returned index is free one or actually present. */
        if (ptrPdRespTmp->respId == ptrPdRespData->respId)
        {
            ptrPdResp = ptrPdRespTmp;
        }
    }
    return ptrPdResp;
}

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
cy_stc_app_pd_resp_stored_t * Cy_App_Hpi_DeletePdRespData(cy_stc_app_pd_resp_data_t *ptrPdRespData, uint8_t matchLen)
{
    cy_stc_app_pd_resp_stored_t *ptrPdResp = NULL;
    /* Gets the index where the response is stored. */
    uint8_t deleteIdx = Cy_App_Hpi_GetPdRespMatchIndex(ptrPdRespData, matchLen);;
    if (deleteIdx < CY_APP_PD_RESP_DATA_BUFF_LEN - CY_APP_PD_RESP_DATA_HDR_SIZE)
    {
        cy_stc_app_pd_resp_stored_t *ptrPdRespTmp = (cy_stc_app_pd_resp_stored_t *)&glPdRespMsgData[deleteIdx];
        /* Checks if the response ID is matching to check if the returned index is free one or actually present. */
        if (ptrPdRespTmp->respId == ptrPdRespData->respId)
        {
            ptrPdResp = ptrPdRespTmp;
            ptrPdResp->respId = CY_APP_PD_RESP_ID_RESERVED;
            /* Re-calculate the free space available. */
            Cy_App_Hpi_ResetPdRespIdx();
        }
    }
    return ptrPdResp;
}

/**
 * @brief Handles the HPI PD control command for R/W PD response data.
 *
 * This function is used to handle the PD control command form the EC via HPI.
 *
 * @param ptrHpiContext Pointer to the HPI context.
 * @param ptrPdStackContext Pointer to the PD stack context.
 * @param data pointer to the R/W PD response data command.
 *
 * @return
 * CY_PDSTACK_STAT_SUCCESS If the command handling is success.
 * CY_PDSTACK_STAT_INVALID_ARGUMENT If the arguments are invalid or the response cannot
 * be stored, or the response is not found.
 *
 */
cy_en_pdstack_status_t Cy_App_Hpi_HandlePdRespDataRw(cy_stc_hpi_context_t *ptrHpiContext, cy_stc_pdstack_context_t *ptrPdStackContext, uint8_t *data)
{
    cy_en_pdstack_status_t cmdStat = CY_PDSTACK_STAT_SUCCESS;
    cy_stc_app_pd_resp_data_t *ptrPdRespData = (cy_stc_app_pd_resp_data_t *)data;
    cy_stc_app_pd_resp_stored_t *ptrPdResp;

    do
    {
        /* Byte 0 - Response Data ID. */
        if ((CY_APP_PD_RESP_ID_RESERVED == ptrPdRespData->respId) || (CY_APP_PD_RESP_ID_MAX_NUM <= ptrPdRespData->respId))
        {
            /* Not supported Data ID. */
            cmdStat = CY_PDSTACK_STAT_INVALID_ARGUMENT;
            break;
        }
        /* Data command. */
        uint8_t portFlag = false;
        /* Checks if the command is specific to port or not. */
        if (false == ptrPdRespData->portFlag)
        {
            portFlag = (CY_APP_PD_RESP_DATA_PORT_FLAG_MSK << ptrPdStackContext->port);
        }
        uint8_t dataCmd = ptrPdRespData->dataCmd;
        /* Stores the port flag in the dataCmd so that it will be stored. */
        ptrPdRespData->cmdVal = portFlag;
        if (CY_APP_PD_RESP_DATA_CMD_WRITE == dataCmd)
        {/* Write command. */
            /* Checks for minimum response length before storing it. */
            if(ptrPdRespData->respLen >= CY_APP_PD_RESP_MIN_DATA_LEN)
            {
                /* Stores the data if the space is available. */
                ptrPdResp = Cy_App_Hpi_StorePdRespData(ptrPdRespData, CY_APP_PD_RESP_MATCH_LENGTH);
                if (NULL == ptrPdResp)
                {
                    cmdStat = CY_PDSTACK_STAT_INVALID_ARGUMENT;
                }
            }
            else
            {
                cmdStat = CY_PDSTACK_STAT_INVALID_ARGUMENT;
            }
        }
        else if (CY_APP_PD_RESP_DATA_CMD_READ == dataCmd)
        {/* Read command. */
            /* Gets the stored data from the common buffer. */
            ptrPdResp = Cy_App_Hpi_RetrievePdRespData(ptrPdRespData, CY_APP_PD_RESP_MATCH_LENGTH);
            if (NULL == ptrPdResp)
            {
                /* Response is not found hence send invalid argument response. */
                cmdStat = CY_PDSTACK_STAT_INVALID_ARGUMENT;
            }
            else
            {
                /* Response is found. Send the data that has been read back. */
                ptrPdRespData->cmdVal = false;
                memcpy(&ptrPdRespData->respLen, &ptrPdResp->respData, ptrPdResp->respLen);
                Cy_Hpi_RegEnqueueEvent(ptrHpiContext,
                        (cy_en_hpi_reg_section_t)(ptrPdStackContext->port + 1),
                        CY_HPI_RESPONSE_PD_RESP_DATA,
                        ptrPdResp->respLen + 2u, &ptrPdRespData->respId);
                cmdStat = CY_PDSTACK_STAT_NO_RESPONSE;
            }
        }
        else if (CY_APP_PD_RESP_DATA_CMD_DELETE == dataCmd)
        {/* Delete command. */
            /* Deletes the response if found in the common buffer. */
            ptrPdResp = Cy_App_Hpi_DeletePdRespData(ptrPdRespData, CY_APP_PD_RESP_MATCH_LENGTH);
            if (NULL == ptrPdResp)
            {
                /* Responses is not found. Send command failure response. */
                cmdStat = (cy_en_pdstack_status_t)0x04u;
            }
        }
        else
        {
            /* Invalid command sent. */
            cmdStat = CY_PDSTACK_STAT_INVALID_ARGUMENT;
        }
    }
    while (false);

    return cmdStat;
}

#endif /* (CY_HPI_ENABLED && CY_HPI_RW_PD_RESP_MSG_DATA) */
/* [] END OF FILE */
