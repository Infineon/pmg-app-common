/***************************************************************************//**
* \file cy_app_swap.c
* \version 3.0
*
* \brief
* Implements the functions for handling of USB Power Delivery 
* role swap requests.
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

#include "cy_app_config.h"
#include "cy_app_swap.h"
#include "cy_app.h"
#include "cy_app_internal.h"

#define APP_PD_SWAP_RESP_ACCEPT         (0u)
#define APP_PD_SWAP_RESP_REJECT         (1u)
#define APP_PD_SWAP_RESP_WAIT           (2u)
#define APP_PD_SWAP_RESP_NOT_SUPP       (3u)

#define GET_DR_SWAP_RESP(resp)              ((resp) & 0x3u)
/**< Macro to extract the default DR_SWAP command response from the swap_response field
     in the configuration table. */

#define GET_PR_SWAP_RESP(resp)              (((resp) & 0xCu) >> 2u)
/**< Macro to extract the default PR_SWAP command response from the swap_response field in the configuration table. */

#define GET_VCONN_SWAP_RESP(resp)           (((resp) & 0x30u) >> 4u)
/**< Macro to extract the default VCONN_SWAP command response from the swap_response field
     in the configuration table. */

static cy_en_pdstack_app_req_status_t get_response(cy_stc_pdstack_context_t * context, uint8_t raw_resp)
{
#if CY_PD_REV3_ENABLE
    cy_stc_pd_dpm_config_t* dpm = &(context->dpmConfig);
#endif /* CCG_PD_REV3_ENABLE */

    cy_en_pdstack_app_req_status_t retVal;
    switch((app_swap_resp_t)raw_resp)
    {
        case APP_RESP_ACCEPT:
            retVal = CY_PDSTACK_REQ_ACCEPT;
            break;
        case APP_RESP_WAIT:
            retVal =  CY_PDSTACK_REQ_WAIT;
            break;
#if CY_PD_REV3_ENABLE
        case APP_RESP_NOT_SUPPORTED:
            if(dpm->specRevSopLive <= CY_PD_REV2)
            {
                retVal = CY_PDSTACK_REQ_REJECT;
            }
            else
            {
                retVal = CY_PDSTACK_REQ_NOT_SUPPORTED;
            }
            break;
#endif /* CCG_PD_REV3_ENABLE */
        default:
            retVal = CY_PDSTACK_REQ_REJECT;
            break;
    }
    return retVal;
}

void Cy_App_Swap_EvalDrSwap (cy_stc_pdstack_context_t * context, cy_pdstack_app_resp_cbk_t app_resp_handler)
{
    cy_en_pdstack_app_req_status_t result = CY_PDSTACK_REQ_REJECT;
    
    /*
     * Default response is NOT_SUPPORTED instead of REJECT if the current mode
     * is PD REV3 and port role is source/sink only.
     */
#if CY_PD_REV3_ENABLE
    if (
            (context->dpmConfig.specRevSopLive >= CY_PD_REV3) &&
            (context->dpmStat.portRole != CY_PD_PRT_DUAL)
       )
    {
        result = CY_PDSTACK_REQ_NOT_SUPPORTED;
    }
#endif /* CY_PD_REV3_ENABLE */

#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
    cy_stc_pdaltmode_context_t * pdAltModeContext = context->ptrAltModeContext;

    if (pdAltModeContext->altModeAppStatus->altModeEntered == true)
    {
        result = CY_PDSTACK_REQ_SEND_HARD_RESET;
    }
    else
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
    {
#if (CY_APP_ROLE_PREFERENCE_ENABLE)
        /* DR_SWAP from preferred role is not accepted */
        if ((uint8_t)(context->dpmConfig.curPortType) != glAppPrefDataRole[context->port])
#endif /* (CY_APP_ROLE_PREFERENCE_ENABLE) */
        {
            result = get_response(context, GET_DR_SWAP_RESP(context->dpmStat.swapResponse));
        }
    }

    Cy_App_GetRespBuffer(context->port)->reqStatus = result;
    app_resp_handler(context, Cy_App_GetRespBuffer(context->port));
}

void Cy_App_Swap_EvalPrSwap (cy_stc_pdstack_context_t * context, cy_pdstack_app_resp_cbk_t app_resp_handler)
{
    cy_en_pdstack_app_req_status_t result = CY_PDSTACK_REQ_REJECT;

#if ((!CY_PD_SINK_ONLY) && (!CY_PD_SOURCE_ONLY))
    const cy_stc_pdstack_dpm_status_t* dpm = &context->dpmStat;
    uint8_t pdo_mask;

    if(context->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SOURCE)
    {
#if CY_USE_CONFIG_TABLE
        pdo_mask = dpm->srcPdoMask;
#else
        pdo_mask = dpm->srcPdoFlags[0];
#endif /* CY_USE_CONFIG_TABLE */
    }
    else
    {
#if CY_USE_CONFIG_TABLE
        pdo_mask = dpm->snkPdoMask;
#else
        pdo_mask = dpm->snkPdoFlags[0];
#endif /* CY_USE_CONFIG_TABLE */
    }

    /*
     * Default response is NOT_SUPPORTED instead of REJECT if the current mode
     * is PD REV3 and port role is source/sink only.
     */
#if CY_PD_REV3_ENABLE
    if (
            (context->dpmConfig.specRevSopLive >= CY_PD_REV3) &&
            (dpm->portRole != CY_PD_PRT_DUAL)
       )
    {
        result = CY_PDSTACK_REQ_NOT_SUPPORTED;
    }
#endif /* CY_PD_REV3_ENABLE */

#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
    /* Do not allow PR_SWAP to a non-preferred role */
    if (glAppPrefPowerRole[context->port] != (uint8_t)(context->dpmConfig.curPortRole))
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */
    {
        if (
                (dpm->deadBat == false) && (dpm->portRole == CY_PD_PRT_DUAL) &&
                (
                 ((pdo_mask & (0x1u << CY_PD_EXTERNALLY_POWERED_BIT_POS)) == 0u) ||
                 (context->dpmConfig.curPortRole == CY_PD_PRT_ROLE_SINK)
                )
#if CY_PD_REV3_ENABLE
                && (context->dpmExtStat.eprActive == false)
#endif /* CY_PD_REV3_ENABLE */
           )
        {
            result = get_response(context, GET_PR_SWAP_RESP(context->dpmStat.swapResponse));
        }
    }
#if (CY_APP_POWER_ROLE_PREFERENCE_ENABLE)
    else
    {
        if(dpm->portRole == CY_PD_PRT_DUAL)
        {
            result = CY_PDSTACK_REQ_REJECT;
        }
        else
        {
            result = get_response(context, (uint8_t)APP_RESP_NOT_SUPPORTED);
        }
    }
#endif /* (CY_APP_POWER_ROLE_PREFERENCE_ENABLE) */

#else

    if (context->dpmConfig.specRevSopLive >= CY_PD_REV3)
    {
        result = CY_PDSTACK_REQ_NOT_SUPPORTED;
    }

#endif /* ((!CY_PD_SINK_ONLY) && (!CY_PD_SOURCE_ONLY)) */

    Cy_App_GetRespBuffer(context->port)->reqStatus = result;
    app_resp_handler(context, Cy_App_GetRespBuffer(context->port));
}

void Cy_App_Swap_EvalVconnSwap (cy_stc_pdstack_context_t * context, cy_pdstack_app_resp_cbk_t app_resp_handler)
{
    cy_en_pdstack_app_req_status_t result = CY_PDSTACK_REQ_REJECT;

#if (!CY_PD_VCONN_DISABLE)
    const cy_stc_pd_dpm_config_t* dpm = &(context->dpmConfig);

    if (dpm->vconnLogical == true)
    {
        result = CY_PDSTACK_REQ_ACCEPT;
    }
    else
    {
        result = get_response(context, GET_VCONN_SWAP_RESP(context->dpmStat.swapResponse));

        if (result == CY_PDSTACK_REQ_ACCEPT)
        {
#if CY_PD_DP_VCONN_SWAP_FEATURE
            if (!Cy_USBPD_V5V_IsSupplyOn (context->ptrUsbPdContext))
            {
                result = CY_PDSTACK_REQ_WAIT;
            }
#endif /* CY_PD_DP_VCONN_SWAP_FEATURE */

#if VCONN_OCP_ENABLE
            /* Do not allow VCONN_SWAP to become VConn source if fault is active. */
            if ((Cy_App_GetPdAppStatus(context->port)->faultStatus & (uint8_t)CY_APP_PORT_VCONN_FAULT_ACTIVE) != 0u)
            {
                result = CY_PDSTACK_REQ_REJECT;
            }
#endif /* VCONN_OCP_ENABLE */
        }
    }
#else
    if (context->dpmConfig.specRevSopLive >= CY_PD_REV3)
    {
        result = CY_PDSTACK_REQ_NOT_SUPPORTED;
    }
#endif /* !CY_PD_VCONN_DISABLE */

    Cy_App_GetRespBuffer(context->port)->reqStatus = result;
    app_resp_handler(context, Cy_App_GetRespBuffer(context->port));
}

#if ((!CY_PD_SOURCE_ONLY) && (!CY_PD_SINK_ONLY) && CY_PD_REV3_ENABLE && CY_PD_FRS_TX_ENABLE)
void Cy_App_Swap_EvalFrSwap (cy_stc_pdstack_context_t* context, cy_pdstack_app_resp_cbk_t app_resp_handler)
{
    /* Accept FRS support is enabled/disabled by separate bit in configuration table.  */
    cy_en_pdstack_app_req_status_t result = CY_PDSTACK_REQ_ACCEPT;

    Cy_App_GetRespBuffer(context->port)->reqStatus = result;
    app_resp_handler(context, Cy_App_GetRespBuffer(context->port));
}
#endif /* ((!CY_PD_SOURCE_ONLY) && (!CY_PD_SINK_ONLY) && CY_PD_REV3_ENABLE && CY_PD_FRS_TX_ENABLE) */

/* [] END OF FILE */

