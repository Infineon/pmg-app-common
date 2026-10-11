/***************************************************************************//**
* \file cy_app_led_ctrl.c
* \version 3.0
*
* \brief
* Implements the functions associated with application level
* management of the led control module.
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

#if CY_APP_LED_CONTROL_ENABLE

#include "cy_app_led_ctrl.h"

cy_en_app_status_t Cy_App_LedCtrl_Init(cy_stc_led_ctrl_context_t *ledCtrlCtx)
{
    /* Check if the context is NULL. If NULL, then return with error. */
    if(ledCtrlCtx == NULL)
    {
        return CY_APP_STAT_BAD_PARAM;
    }

    /* Drive the pin LOW and set the state to OFF */
    Cy_GPIO_Write(ledCtrlCtx->port, ledCtrlCtx->pinNum, ledCtrlCtx->ledOrientation ? 0u : 1u);
    ledCtrlCtx->prev_state = CY_APP_LED_CTRL_LED_OFF;

    /* Initialize blink and breath states */
    ledCtrlCtx->blink_state = ledCtrlCtx->ledOrientation;
    ledCtrlCtx->led_breathing_state = CY_APP_LED_CTRL_BREATHING_STATE_INIT;

    return CY_APP_STAT_SUCCESS;
}

/* Timer callback function used for implementing blinking mode of operation */
static void Cy_App_LedCtrl_BlinkingCbk ( cy_timer_id_t id, void *callbackContext)
{
    cy_stc_led_ctrl_context_t *ledCtrlCtx = (cy_stc_led_ctrl_context_t *)callbackContext;

    /* Invert the blink state */
    ledCtrlCtx->blink_state = !(ledCtrlCtx->blink_state);

    Cy_GPIO_Write(ledCtrlCtx->port, ledCtrlCtx->pinNum, ledCtrlCtx->blink_state ? 1u : 0u);
    (void)Cy_PdUtils_SwTimer_Start(ledCtrlCtx->timerContext, ledCtrlCtx, ledCtrlCtx->timerId,
                             ledCtrlCtx->blink_rate, &Cy_App_LedCtrl_BlinkingCbk);

    CY_UNUSED_PARAMETER(id);
}

/* Timer callback function used for implementing breathing mode of operation */
static void Cy_App_LedCtrl_BreathingCbk ( cy_timer_id_t id, void *callbackContext)
{
    cy_stc_led_ctrl_context_t *ledCtrlCtx = (cy_stc_led_ctrl_context_t *)callbackContext;
    switch(ledCtrlCtx->led_breathing_state)
    {
        case CY_APP_LED_CTRL_BREATHING_STATE_INIT:
            ledCtrlCtx->breath_count = 0u;
            ledCtrlCtx->breath_value = 1u;
            ledCtrlCtx->breath_state = true;
            ledCtrlCtx->led_period = (CY_APP_LED_CTRL_BREATHING_PERIOD * CY_APP_LED_CTRL_BREATHING_COUNT) >> 1u;
            ledCtrlCtx->led_breathing_state = CY_APP_LED_CTRL_BREATHING_STATE_RISE;
            break;
        case CY_APP_LED_CTRL_BREATHING_STATE_RISE:
            if ((ledCtrlCtx->breath_count & 1u) != 0u)
            {
                ledCtrlCtx->breath_state = true;
                ledCtrlCtx->led_period = CY_APP_LED_CTRL_BREATHING_PERIOD - ledCtrlCtx->breath_value;
            }
            else
            {
                ledCtrlCtx->breath_state = false;
                ledCtrlCtx->led_period = ledCtrlCtx->breath_value;
            }
            if (++ledCtrlCtx->breath_count >= CY_APP_LED_CTRL_BREATHING_COUNT)
            {
                ledCtrlCtx->breath_count = 0u;
                if (++ledCtrlCtx->breath_value >= CY_APP_LED_CTRL_BREATHING_PERIOD)
                {
                    ledCtrlCtx->led_breathing_state = CY_APP_LED_CTRL_BREATHING_STATE_PEAK;
                }
            }
            break;
        case CY_APP_LED_CTRL_BREATHING_STATE_PEAK:
            ledCtrlCtx->breath_count = 0u;
            ledCtrlCtx->breath_value = 1u;
            ledCtrlCtx->breath_state = false;
            ledCtrlCtx->led_period = CY_APP_LED_CTRL_PEAK_PERIOD;
            ledCtrlCtx->led_breathing_state = CY_APP_LED_CTRL_BREATHING_STATE_FALL;
            break;
        case CY_APP_LED_CTRL_BREATHING_STATE_FALL:
            if ((ledCtrlCtx->breath_count & 1u) == 0u)
            {
                ledCtrlCtx->breath_state = false;
                ledCtrlCtx->led_period = CY_APP_LED_CTRL_BREATHING_PERIOD - ledCtrlCtx->breath_value;
            }
            else
            {
                ledCtrlCtx->breath_state = true;
                ledCtrlCtx->led_period = ledCtrlCtx->breath_value;
            }
            if (++ledCtrlCtx->breath_count >= CY_APP_LED_CTRL_BREATHING_COUNT)
            {
                ledCtrlCtx->breath_count = 0u;
                if (++ledCtrlCtx->breath_value >= CY_APP_LED_CTRL_BREATHING_PERIOD)
                {
                    ledCtrlCtx->led_breathing_state = CY_APP_LED_CTRL_BREATHING_STATE_INIT;
                }
            }
            break;
        default:
            /* No handling needed in other cases */
            break;
    }
    Cy_GPIO_Write(ledCtrlCtx->port, ledCtrlCtx->pinNum, ledCtrlCtx->breath_state ? 1u : 0u);
    (void)Cy_PdUtils_SwTimer_Start (ledCtrlCtx->timerContext, ledCtrlCtx, ledCtrlCtx->timerId,
                              ledCtrlCtx->led_period, &Cy_App_LedCtrl_BreathingCbk);

    CY_UNUSED_PARAMETER(id);
}

cy_en_app_status_t Cy_App_LedCtrl_SwitchMode( cy_en_led_ctrl_mode_t ledMode,
                                           cy_stc_led_ctrl_context_t *ledCtrlCtx)
{
    cy_en_app_status_t status = CY_APP_STAT_SUCCESS;

    if(ledCtrlCtx == NULL)
    {
        return CY_APP_STAT_BAD_PARAM;
    }

    if (ledCtrlCtx->prev_state == ledMode)
    {
        return CY_APP_STAT_NO_STATE_CHANGE;
    }

    Cy_PdUtils_SwTimer_Stop(ledCtrlCtx->timerContext, ledCtrlCtx->timerId);
    ledCtrlCtx->prev_state = ledMode;

    switch(ledMode)
    {
        case CY_APP_LED_CTRL_LED_OFF:
            Cy_GPIO_Write(ledCtrlCtx->port, ledCtrlCtx->pinNum, ledCtrlCtx->ledOrientation ? 0u : 1u);
            break;
        case CY_APP_LED_CTRL_LED_ON:
            Cy_GPIO_Write(ledCtrlCtx->port, ledCtrlCtx->pinNum, ledCtrlCtx->ledOrientation ? 1u : 0u);
            break;
        case CY_APP_LED_CTRL_LED_BLINKING:
            (void)Cy_PdUtils_SwTimer_Start(ledCtrlCtx->timerContext, ledCtrlCtx, ledCtrlCtx->timerId,
                                     CY_APP_LED_CTRL_BLINKING_PERIOD, &Cy_App_LedCtrl_BlinkingCbk);
            break;
        case CY_APP_LED_CTRL_LED_BREATHING:
            (void)Cy_PdUtils_SwTimer_Start (ledCtrlCtx->timerContext, ledCtrlCtx, ledCtrlCtx->timerId,
                                      CY_APP_LED_CTRL_BREATHING_PERIOD, &Cy_App_LedCtrl_BreathingCbk);
            break;
        default:
            status = CY_APP_STAT_BAD_PARAM;
            break;
    }

    return status;
}

#endif /* CY_APP_LED_CONTROL_ENABLE */
/* [] END OF FILE */