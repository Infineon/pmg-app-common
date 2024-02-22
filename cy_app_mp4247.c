/***************************************************************************//**
* \file cy_app_mp4247.c
* \version 1.0
*
* \brief
* Implements functions associated with MP4247 buck-boost controller
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "cy_pdl.h"
#include "cy_app_mp4247.h"
#include "cy_pdutils.h"
#include "cy_app_i2c_master.h"

#define MP4247_OPER_ENABLE          (1U)
#define MP4247_OPER_DISABLE         (0U)


/* Enable or disable output OVP */
static bool mp4247_set_ovp(cy_stc_app_mp4247_context_t *context, bool on_off)
{
    bool status = false;
    uint8_t reg_value;
    uint8_t reg_addr;

    reg_addr = MP4247_MFR_CTRL1_REG;

    status = Cy_App_I2CMaster_RegRead(context->scbBase, context->i2cAddr, \
                                   &reg_addr, 1u, &reg_value, 1u, context->i2cContext);

    if(status == true)
    {
        reg_value = ((reg_value & (~(MP4247_MFR_CTRL1_OUT_OVP_MSK))) | (on_off));

        status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                   &reg_addr, 1u, &reg_value, 1u, context->i2cContext);
    }

    return status;
}

/* Initialize MP4247*/
bool Cy_App_MP4247_Init(cy_stc_app_mp4247_context_t *context)
{
    bool status = false;
    
    if(context == NULL)
    {
        return false;
    }

    if((NULL == context->i2cContext) || (NULL == context->scbBase))
    {
        return false;
    }

    /* Disable OVP */
    status = mp4247_set_ovp(context, false);

    return status;
}

/* Enable VOut */
bool Cy_App_MP4247_Enable(cy_stc_app_mp4247_context_t *context)
{
    bool status = false;
    uint8_t reg_addr;
    uint8_t reg_value;

    if(NULL == context)
    {
        return false;
    }

    reg_addr = MP4247_OPER;
    reg_value = MP4247_OPER_ENABLE;

    status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);
    return(status);
}

/* Disable VOut */
bool Cy_App_MP4247_Disable(cy_stc_app_mp4247_context_t *context)
{
    bool status = false;
    uint8_t reg_addr;
    uint8_t reg_value;

    if(NULL == context)
    {
        return false;
    }

    reg_addr = MP4247_OPER;
    reg_value = MP4247_OPER_DISABLE;

    status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);

    return(status);
}

/* Set the output voltage */
bool Cy_App_MP4247_SetVolt(cy_stc_app_mp4247_context_t *context, uint16_t vol_in_mv)
{
    bool status = false;
    uint16_t vsel;
    uint8_t reg_addr;
    uint8_t reg_value[2];

    if(NULL == context)
    {
        return false;
    }

    /* V = ((Vout * 1024 * 10) / Fb_ratio). In this formula Vout is in
     * volts so, dividing the voltage value with 1000 */
    vsel = ((vol_in_mv * 1024u * 10u) / (context->fbRatio * 1000u));

    reg_addr = MP4247_VOUT;
    reg_value[0] = vsel;
    reg_value[1] = vsel >> 8u;

    status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                &reg_addr, 1u, reg_value, 2u, context->i2cContext);

    return(status);
}

/* Sets the current limit */
bool Cy_App_MP4247_SetCurrentLimit(cy_stc_app_mp4247_context_t *context, uint16_t cur_in_10ma)
{
    bool status = false;
    uint8_t reg_addr;
    uint8_t reg_value;

    if(NULL == context)
    {
        return false;
    }

    reg_addr = MP4247_CUR_LIMIT_REG;
    reg_value = cur_in_10ma / 5u;
    /*max is 5.4 A*/
    if(reg_value > MP4247_MAX_CURRENT_LIMIT)
    {
        reg_value = MP4247_MAX_CURRENT_LIMIT;
    }

    status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);

    return(status);
}


/* Sets the VOut rising slew rate */
bool Cy_App_MP4247_SetSlewRateRise(cy_stc_app_mp4247_context_t *context, uint8_t slew_rate)
{
    bool status = false;
    uint8_t reg_addr;
    uint8_t reg_value;

    if(NULL == context)
    {
        return false;
    }

    reg_addr = MP4247_MFR_CTRL3_REG;

    status = Cy_App_I2CMaster_RegRead(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);

    if(status == true)
    {
        reg_value = ((reg_value & (~(MP4247_MFR_CTRL3_SLEW_RATE_RISE_MSK))) | (slew_rate));

        status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                      &reg_addr, 1u, &reg_value, 1u, context->i2cContext);
    }

    return(status);
}

/* Sets the VOut falling slew rate */
bool Cy_App_MP4247_SetSlewRateFall(cy_stc_app_mp4247_context_t *context, uint8_t slew_rate)
{
    bool status = false;
    uint8_t reg_addr;
    uint8_t reg_value;

    if(NULL == context)
    {
        return false;
    }

    reg_addr = MP4247_MFR_CTRL3_REG;

    status = Cy_App_I2CMaster_RegRead(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);

    if(status == true)
    {
        reg_value = ((reg_value & (~(MP4247_MFR_CTRL3_SLEW_RATE_FALL_MSK))) | (slew_rate));

        status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                      &reg_addr, 1u, &reg_value, 1u, context->i2cContext);
    }

    return(status);
}

/* [] END OF FILE */
