/******************************************************************************
* \file cy_app_buck_boost.c
* \version 1.0
*
* \brief
* Source code for the buck-boost regulator
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "cy_app_buck_boost.h"
#include "cy_pdl.h"

#if CY_APP_BUCKBOOST_MP4247_ENABLE
static cy_stc_app_mp4247_context_t *mp4247Port1CtxPtr = NULL;
static cy_stc_app_mp4247_context_t *mp4247Port2CtxPtr = NULL;
#elif CY_APP_BUCKBOOST_RT6190_ENABLE
static cy_stc_app_rt6190_context_t *rt6190Port1CtxPtr = NULL;
static cy_stc_app_rt6190_context_t *rt6190Port2CtxPtr = NULL;
#elif CY_APP_BUCKBOOST_NCP81239_ENABLE
static cy_stc_app_ncp81239_context_t *ncp81239Port1CtxPtr = NULL;
static cy_stc_app_ncp81239_context_t *ncp81239Port2CtxPtr = NULL;
#endif /* CY_APP_BUCKBOOST_MP4247_ENABLE */

/* Initialize buck-boost regulator for Port 1 */
bool Cy_App_BuckBoost_InitPort1(void *contextPtr)
{
    if(NULL == contextPtr) return false;

#if CY_APP_BUCKBOOST_MP4247_ENABLE
    mp4247Port1CtxPtr = (cy_stc_app_mp4247_context_t *)contextPtr;

    if (mp4247Port1CtxPtr->enableGpioPort != NULL)
    {
        Cy_GPIO_Write(mp4247Port1CtxPtr->enableGpioPort, mp4247Port1CtxPtr->enableGpioPin, 1U);
        Cy_SysLib_Delay(1u);
    }

    return Cy_App_MP4247_Init(mp4247Port1CtxPtr);
#elif CY_APP_BUCKBOOST_RT6190_ENABLE
    rt6190Port1CtxPtr = (cy_stc_app_rt6190_context_t *)contextPtr;

    if (rt6190Port1CtxPtr->enableGpioPort != NULL)
    {
        Cy_GPIO_Write(rt6190Port1CtxPtr->enableGpioPort, rt6190Port1CtxPtr->enableGpioPin, 1U);
        Cy_SysLib_Delay(1u);
    }

    return Cy_App_RT6190_Init(rt6190Port1CtxPtr);
#elif CY_APP_BUCKBOOST_NCP81239_ENABLE
    ncp81239Port1CtxPtr = (cy_stc_app_ncp81239_context_t *)contextPtr;

    if (ncp81239Port1CtxPtr->enableGpioPort != NULL)
    {
        Cy_GPIO_Write(ncp81239Port1CtxPtr->enableGpioPort, ncp81239Port1CtxPtr->enableGpioPin, 1U);
        Cy_SysLib_Delay(1u);
    }

    return Cy_App_NCP81239_Init(ncp81239Port1CtxPtr);
#else
    return false;
#endif /* CY_APP_BUCKBOOST_MP4247_ENABLE */
}

/* Initialize buck-boost regulator for Port 2 */
bool Cy_App_BuckBoost_InitPort2(void *contextPtr)
{
    if(NULL == contextPtr) return false;

#if CY_APP_BUCKBOOST_MP4247_ENABLE
    mp4247Port2CtxPtr = (cy_stc_app_mp4247_context_t *)contextPtr;

    if (mp4247Port2CtxPtr->enableGpioPort != NULL)
    {
        Cy_GPIO_Write(mp4247Port2CtxPtr->enableGpioPort, mp4247Port2CtxPtr->enableGpioPin, 1U);
        Cy_SysLib_Delay(1u);
    }

    return Cy_App_MP4247_Init(mp4247Port2CtxPtr);
#elif CY_APP_BUCKBOOST_RT6190_ENABLE
    rt6190Port2CtxPtr = (cy_stc_app_rt6190_context_t *)contextPtr;

    if (rt6190Port2CtxPtr->enableGpioPort != NULL)
    {
        Cy_GPIO_Write(rt6190Port2CtxPtr->enableGpioPort, rt6190Port2CtxPtr->enableGpioPin, 1U);
        Cy_SysLib_Delay(1u);
    }

    return Cy_App_RT6190_Init(rt6190Port2CtxPtr);
#elif CY_APP_BUCKBOOST_NCP81239_ENABLE
    ncp81239Port2CtxPtr = (cy_stc_app_ncp81239_context_t *)contextPtr;

    if (ncp81239Port2CtxPtr->enableGpioPort != NULL)
    {
        Cy_GPIO_Write(ncp81239Port2CtxPtr->enableGpioPort, ncp81239Port2CtxPtr->enableGpioPin, 1U);
        Cy_SysLib_Delay(1u);
    }

    return Cy_App_NCP81239_Init(ncp81239Port2CtxPtr);
#else
    return false;
#endif /* CY_APP_BUCKBOOST_MP4247_ENABLE */
}

/* Set the output voltage for Port 1 */
bool Cy_App_BuckBoost_SetVoltPort1(uint16_t vol_in_mv)
{
#if CY_APP_BUCKBOOST_MP4247_ENABLE
    return (NULL == mp4247Port1CtxPtr)? false : Cy_App_MP4247_SetVolt(mp4247Port1CtxPtr, vol_in_mv);
#elif CY_APP_BUCKBOOST_RT6190_ENABLE
    return (NULL == rt6190Port1CtxPtr)? false : Cy_App_RT6190_SetVolt(rt6190Port1CtxPtr, vol_in_mv);
#elif CY_APP_BUCKBOOST_NCP81239_ENABLE 
    return (NULL == ncp81239Port1CtxPtr)? false : Cy_App_NCP81239_SetVolt(ncp81239Port1CtxPtr, vol_in_mv);
#else
    return false;
#endif /* CY_APP_BUCKBOOST_MP4247_ENABLE */
}

/* Set the output voltage for Port 2 */
bool Cy_App_BuckBoost_SetVoltPort2(uint16_t vol_in_mv)
{
#if CY_APP_BUCKBOOST_MP4247_ENABLE
    return (NULL == mp4247Port2CtxPtr)? false : Cy_App_MP4247_SetVolt(mp4247Port2CtxPtr, vol_in_mv);
#elif CY_APP_BUCKBOOST_RT6190_ENABLE
    return (NULL == rt6190Port2CtxPtr)? false : Cy_App_RT6190_SetVolt(rt6190Port2CtxPtr, vol_in_mv);
#elif CY_APP_BUCKBOOST_NCP81239_ENABLE
    return (NULL == ncp81239Port2CtxPtr)? false : Cy_App_NCP81239_SetVolt(ncp81239Port2CtxPtr, vol_in_mv);
#else
    return false;
#endif /* CY_APP_BUCKBOOST_MP4247_ENABLE */
}

/* Turn off the output */
bool Cy_App_BuckBoost_SetOutputPort1(bool output)
{
    bool status = false;
#if CY_APP_BUCKBOOST_MP4247_ENABLE
    if(NULL != mp4247Port1CtxPtr)
    {
        if(output)
        {
            status = Cy_App_MP4247_Enable(mp4247Port1CtxPtr);
        }
        else
        {
            status = Cy_App_MP4247_Disable(mp4247Port1CtxPtr);
        }
    }
#elif CY_APP_BUCKBOOST_RT6190_ENABLE
    if(NULL != rt6190Port1CtxPtr)
    {
        if(output)
        {
            status = Cy_App_RT6190_Enable(rt6190Port1CtxPtr);
        }
        else
        {
            status = Cy_App_RT6190_Disable(rt6190Port1CtxPtr);
        }
    }
#elif CY_APP_BUCKBOOST_NCP81239_ENABLE 
    if(NULL != ncp81239Port1CtxPtr)
    {
        if(output)
        {
            status = Cy_App_NCP81239_Enable(ncp81239Port1CtxPtr);
        }
        else
        {
            status = Cy_App_NCP81239_Disable(ncp81239Port1CtxPtr);
        }
    }
#endif /* CY_APP_BUCKBOOST_MP4247_ENABLE */
    return status;
}

/* Turn on the output */
bool Cy_App_BuckBoost_SetOutputPort2(bool output)
{
    bool status = false;
#if CY_APP_BUCKBOOST_MP4247_ENABLE
    if(NULL != mp4247Port2CtxPtr)
    {
        if(output)
        {
            status = Cy_App_MP4247_Enable(mp4247Port2CtxPtr);
        }
        else
        {
            status = Cy_App_MP4247_Disable(mp4247Port2CtxPtr);
        }
    }
#elif CY_APP_BUCKBOOST_RT6190_ENABLE
    if(NULL != rt6190Port2CtxPtr)
    {
        if(output)
        {
            status = Cy_App_RT6190_Enable(rt6190Port2CtxPtr);
        }
        else
        {
            status = Cy_App_RT6190_Disable(rt6190Port2CtxPtr);
        }
    }
#elif CY_APP_BUCKBOOST_NCP81239_ENABLE 
    if(NULL != ncp81239Port2CtxPtr)
    {
        if(output)
        {
            status = Cy_App_NCP81239_Enable(ncp81239Port2CtxPtr);
        }
        else
        {
            status = Cy_App_NCP81239_Disable(ncp81239Port2CtxPtr);
        }
    }
#endif /* CY_APP_BUCKBOOST_MP4247_ENABLE */
    return status;
}

/* [] END OF FILE */
