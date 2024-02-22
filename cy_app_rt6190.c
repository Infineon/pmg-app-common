/***************************************************************************//**
* \file cy_app_rt6190.c
* \version 1.0
*
* \brief
* Implements functions associated with RT6190 buck-boost controller
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
#include "cy_app_rt6190.h"
#include "cy_pdutils.h"
#include "cy_app_i2c_master.h"

/* Delta VOUT is 12.5 mV/step for VOUT ratio of 0.08V/V */
#define RT6190_DELTA_VOUT_RATIO_0_08V    (12.5)
/* Delta VOUT is 20 mV/step for VOUT ratio of 0.05V/V */
#define RT6190_DELTA_VOUT_RATIO_0_05V    (20u)

#define RT6190_DELTA_VOUT_MV             (RT6190_DELTA_VOUT_RATIO_0_05V)

#define RT6190_OUT_MIN_MA                (306u)
#define RT6190_OUT_MAX_MA                (12114u)
#define RT6190_OUT_STEP_MA               (24u)
#define RT6190_OUTC_MINSEL               (19u)

const uint8_t rt6190_config[][2] =
{
    /* Select VOUT ratio 0.05V/V */
    {RT6190_RATIO_REG, RT6190_RATIO_VOUT_0_05V},
    /* Mask unused alert */
    {RT6190_MASK2_REG, 0u},
    /* Disable OCP */
    {RT6190_OCP_ENABLE_REG, 0u},
    /* Enable VBUSC ADC */
    {RT6190_VBUSC_ADC_REG, RT6190_VBUSC_ADC_EN}
};

bool Cy_App_RT6190_Init(cy_stc_app_rt6190_context_t *context)
{
    bool status = false;
    uint8_t idx, count;
    
    if(context == NULL)
    {
        return false;
    }

    if((NULL == context->i2cContext) || (NULL == context->scbBase))
    {
        return false;
    }

    count = sizeof (rt6190_config) / (2 * sizeof(uint8_t));

    for(idx = 0u; idx < count; idx++)
    {
        status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                        (uint8_t *)&rt6190_config[idx][0], 1u, \
                                        (uint8_t *)&rt6190_config[idx][1], 1u, context->i2cContext);
        if(status != true)
        {
            return status;
        }
    }

    status = Cy_App_RT6190_Enable(context);

    return status;
}

bool Cy_App_RT6190_SetVolt(cy_stc_app_rt6190_context_t *context, uint16_t vol_in_mv)
{
    bool status;
    uint16_t vsel;
    uint8_t reg_addr;
    uint8_t reg_value[2];

    if(context == NULL)
    {
        return false;
    }

    vsel = vol_in_mv / RT6190_DELTA_VOUT_MV;

    /* The data bytes are written to OUTPUT_CV LSB and MSB registers */
    reg_addr = RT6190_OUTPUT_CV_LSB_REG;
    reg_value[0] = vsel;
    reg_value[1] = vsel >> 8u;

    status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                     &reg_addr, 1u, reg_value, 2u, context->i2cContext);
    return status;
}

uint16_t Cy_App_RT6190_GetVolt(cy_stc_app_rt6190_context_t *context)
{
    bool status = false;
    uint8_t reg_addr = RT6190_OUTPUT_CV_LSB_REG;
    uint8_t rd_buffer[2];

    if(context == NULL)
    {
        return RT6190_VOLTAGE_INVALID;
    }

    status = Cy_App_I2CMaster_RegRead(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, rd_buffer, 2u, context->i2cContext);

    return status ? ((rd_buffer[1] << 8u | rd_buffer[0]) * RT6190_DELTA_VOUT_MV) : RT6190_VOLTAGE_INVALID;
}

bool Cy_App_RT6190_Enable(cy_stc_app_rt6190_context_t *context)
{
    bool status = false;
    uint8_t reg_addr;
    uint8_t reg_value;

    if(context == NULL)
    {
        return false;
    }

    reg_addr = RT6190_SET2_REG;

    status = Cy_App_I2CMaster_RegRead(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);
    if(status == true)
    {
        reg_value = reg_value | RT6190_SET2_PWM_EN;

        status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);
    }

    return status;
}

bool Cy_App_RT6190_Disable(cy_stc_app_rt6190_context_t *context)
{
    bool status = false;
    uint8_t reg_addr;
    uint8_t reg_value;

    if(context == NULL)
    {
        return false;
    }

    reg_addr = RT6190_SET2_REG;

    status = Cy_App_I2CMaster_RegRead(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);

    if(status == true)
    {
        reg_value = (reg_value & (~(RT6190_SET2_PWM_MSK))) | RT6190_SET2_PWM_DIS;

        status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);
    }

    return status;
}

bool Cy_App_RT6190_SetCurrentLimit(cy_stc_app_rt6190_context_t *context, uint16_t cur_in_10ma)
{
    bool status = false;
    uint16_t csel;
    uint8_t reg_addr;
    uint8_t wr_buffer[2];

    if(context == NULL)
    {
        return false;
    }

    csel = cur_in_10ma * 10;

    /* According to the datasheet, the current limit for GAIN_OCS=10x should be
     * between 0.306A and 12.114A. */
    if((csel < RT6190_OUT_MIN_MA) ||  (csel > RT6190_OUT_MAX_MA))
    {
        return false;
    }
    csel = CY_PDUTILS_DIV_ROUND_UP(csel - RT6190_OUT_MIN_MA, RT6190_OUT_STEP_MA);
    csel += RT6190_OUTC_MINSEL;
    
    /* The data bytes will be written to OUTPUT_CC LSB and MSB registers */
    reg_addr = RT6190_OUTPUT_CC_LSB_REG;
    wr_buffer[0] = csel;
    wr_buffer[1] = csel >> 8u;

    status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                     &reg_addr, 1u, wr_buffer, 2u, context->i2cContext);
    return status;
}

uint16_t Cy_App_RT6190_GetCurrentLimit(cy_stc_app_rt6190_context_t *context)
{
    bool status = false;
    uint8_t reg_addr = RT6190_OUTPUT_CC_LSB_REG;
    uint16_t csel;
    uint8_t rd_buffer[2];
    
    if(context == NULL)
    {
        return RT6190_CURRENT_INVALID;
    }

    status = Cy_App_I2CMaster_RegRead(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, rd_buffer, 2u, context->i2cContext);

    if(status != true)
    {
        return RT6190_CURRENT_INVALID;
    }

    csel = ((rd_buffer[1] << 8u) | rd_buffer[0]);
    csel -= RT6190_OUTC_MINSEL;

    return (RT6190_OUT_MIN_MA + RT6190_OUT_STEP_MA * csel);
}

bool Cy_App_RT6190_SetMode(cy_stc_app_rt6190_context_t *context, uint8_t mode)
{
    bool status = false;
    uint8_t reg_value;
    uint8_t reg_addr;

    if(context == NULL)
    {
        return false;
    }

    reg_addr = RT6190_SET1_REG;
    status = Cy_App_I2CMaster_RegRead(context->scbBase, context->i2cAddr, \
                            &reg_addr, 1u, &reg_value, 1u, context->i2cContext);

    if(status == true)
    {
        reg_value = (reg_value & (~(RT6190_SET1_F_CCM_MSK))) | mode;
        status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);
    }

    return status;
}

uint8_t Cy_App_RT6190_GetMode(cy_stc_app_rt6190_context_t *context)
{
    bool status = false;
    uint8_t reg_addr = RT6190_SET1_REG;
    uint8_t mode;

    if(context == NULL)
    {
        return RT6190_MODE_INVALID;
    }

    status = Cy_App_I2CMaster_RegRead(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &mode, 1u, context->i2cContext);

    return status ? (mode >> RT6190_SET1_F_CCM_POS) : RT6190_MODE_INVALID;
}

uint16_t Cy_App_RT6190_GetErrorStatus(cy_stc_app_rt6190_context_t *context)
{
    bool status = false;
    uint8_t reg_addr = RT6190_STATUS1_REG;
    uint8_t rd_buffer[2];

    if(context == NULL)
    {
        return RT6190_ERR_FLAG_INVALID;
    }

    status = Cy_App_I2CMaster_RegRead(context->scbBase, context->i2cAddr, \
                            &reg_addr, 1u, rd_buffer, 2u, context->i2cContext);

    return status ? (uint16_t)((rd_buffer[1] << 8u) | rd_buffer[0]) : RT6190_ERR_FLAG_INVALID;
}

/* [] End of file */
