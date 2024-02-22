/***************************************************************************//**
* \file cy_app_led_ctrl.c
* \version 1.0
*
* \brief
* Implements the functions associated with application level
* management of the led control module.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#if CY_APP_LED_CONTROL_ENABLE

#include "cy_app_led_ctrl.h"
#include "cy_app_config.h"

/* Timer callback function used for implementing blinking mode of operation */
static void Cy_App_LedCtrl_BlinkingCbk (cy_timer_id_t id, void *callbackContext);

/* Timer callback function used for implementing breathing mode of operation */
static void Cy_App_LedCtrl_BreathingCbk (cy_timer_id_t id, void *callbackContext);

cy_en_app_status_t Cy_App_LedCtrl_Init(cy_stc_led_ctrl_context_t *ledCtrlCtx)
{
    /* Check if the context is NULL. If NULL, then return with error. */
    if(ledCtrlCtx == NULL)
    {
        return CY_APP_STAT_BAD_PARAM;
    }

    /* Drive the pin LOW and set the state to OFF */
    Cy_GPIO_Write(ledCtrlCtx->port, ledCtrlCtx->pinNum, !(ledCtrlCtx->ledOrientation));
    ledCtrlCtx->prev_state = CY_APP_LED_CTRL_LED_OFF;

    /* Initialize blink and breath states */
    ledCtrlCtx->blink_state = ledCtrlCtx->ledOrientation;
    ledCtrlCtx->led_breathing_state = CY_APP_LED_CTRL_BREATHING_STATE_INIT;

    return CY_APP_STAT_SUCCESS;
}

static void Cy_App_LedCtrl_BlinkingCbk ( cy_timer_id_t id, void *callbackContext)
{
    cy_stc_led_ctrl_context_t *ledCtrlCtx = (cy_stc_led_ctrl_context_t *)callbackContext;

    /* Invert the blink state */
    ledCtrlCtx->blink_state = !(ledCtrlCtx->blink_state);

    Cy_GPIO_Write(ledCtrlCtx->port, ledCtrlCtx->pinNum, ledCtrlCtx->blink_state);
    Cy_PdUtils_SwTimer_Start(ledCtrlCtx->timerContext, ledCtrlCtx, ledCtrlCtx->timerId,
                             ledCtrlCtx->blink_rate, Cy_App_LedCtrl_BlinkingCbk);

    (void)id;
}

static void Cy_App_LedCtrl_BreathingCbk ( cy_timer_id_t id, void *callbackContext)
{
    cy_stc_led_ctrl_context_t *ledCtrlCtx = (cy_stc_led_ctrl_context_t *)callbackContext;
    switch(ledCtrlCtx->led_breathing_state)
    {
        case CY_APP_LED_CTRL_BREATHING_STATE_INIT:
            ledCtrlCtx->breath_count = 0u;
            ledCtrlCtx->breath_value = 1u;
            ledCtrlCtx->breath_state = true;
            ledCtrlCtx->led_period = (CY_APP_LED_CTRL_BREATHING_PERIOD * CY_APP_LED_CTRL_BREATHING_COUNT) >> 1;
            ledCtrlCtx->led_breathing_state = CY_APP_LED_CTRL_BREATHING_STATE_RISE;
            break;
        case CY_APP_LED_CTRL_BREATHING_STATE_RISE:
            if ((ledCtrlCtx->breath_count & 1u) != 0)
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
            break;
    }
    Cy_GPIO_Write(ledCtrlCtx->port, ledCtrlCtx->pinNum, ledCtrlCtx->breath_state);
    Cy_PdUtils_SwTimer_Start (ledCtrlCtx->timerContext, ledCtrlCtx, ledCtrlCtx->timerId,
                              ledCtrlCtx->led_period, Cy_App_LedCtrl_BreathingCbk);

    (void)id;
}

cy_en_app_status_t Cy_App_LedCtrl_SwitchMode( cy_en_led_ctrl_mode_t ledMode,
                                           cy_stc_led_ctrl_context_t *ledCtrlCtx)
{
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
            Cy_GPIO_Write(ledCtrlCtx->port, ledCtrlCtx->pinNum, !(ledCtrlCtx->ledOrientation));
            break;
        case CY_APP_LED_CTRL_LED_ON:
            Cy_GPIO_Write(ledCtrlCtx->port, ledCtrlCtx->pinNum, ledCtrlCtx->ledOrientation);
            break;
        case CY_APP_LED_CTRL_LED_BLINKING:
            Cy_PdUtils_SwTimer_Start(ledCtrlCtx->timerContext, ledCtrlCtx, ledCtrlCtx->timerId,
                                     CY_APP_LED_CTRL_BLINKING_PERIOD, Cy_App_LedCtrl_BlinkingCbk);
            break;
        case CY_APP_LED_CTRL_LED_BREATHING:
            Cy_PdUtils_SwTimer_Start (ledCtrlCtx->timerContext, ledCtrlCtx, ledCtrlCtx->timerId,
                                      CY_APP_LED_CTRL_BREATHING_PERIOD, Cy_App_LedCtrl_BreathingCbk);
            break;
        default:
            return CY_APP_STAT_BAD_PARAM;
            break;
    }

    return CY_APP_STAT_SUCCESS;
}

#endif /* CY_APP_LED_CONTROL_ENABLE */
/* [] END OF FILE */