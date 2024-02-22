/***************************************************************************//**
* \file cy_app_ncp81239.c
* \version 1.0
*
* \brief
* Implements functions associated with NCP81239 buck-boost controller
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cy_pdl.h"
#include "cy_app_ncp81239.h"
#include "cy_app_i2c_master.h"

#if CY_APP_BUCKBOOST_NCP81239_ENABLE
/* Initialize NCP81239 buck-boost controller */
bool Cy_App_NCP81239_Init (cy_stc_app_ncp81239_context_t *context)
{
    bool status = false;
    uint8_t reg_addr[2] = {PD_CTRL_SKEW_RATE_REG_ADDR, PD_CTRL_VPS_REG_ADDR};
    uint8_t reg_value[2] = {PD_CTRL_SKEW_RATE_4_9_MV_US, (PD_CTRL_VPS_5V + NCP_REG_EXCESS_VOLTAGE)};

    if(context == NULL)
    {
        return false;
    }

    if((NULL == context->i2cContext) || (NULL == context->scbBase))
    {
        return false;
    }

    for (uint8_t itr = 0; itr < 2; itr++)
    {
        status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                           &reg_addr[itr], 1u, &reg_value[itr], 1u, context->i2cContext);
    }

    return status;
}

/* Set the output voltage */
bool Cy_App_NCP81239_SetVolt (cy_stc_app_ncp81239_context_t *context, uint16_t volt_in_mv)
{
    bool status = false;
    uint8_t reg_addr;
    uint8_t reg_value;

    if(NULL == context)
    {
        return false;
    }

    reg_addr = PD_CTRL_VPS_REG_ADDR;

    reg_value = (volt_in_mv / NCP_REG_VOLT_RESOLUTION) + NCP_REG_EXCESS_VOLTAGE;

    status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                &reg_addr, 1u, &reg_value, 1u, context->i2cContext);

    return status;

}

/* Enable NCP81239 */
bool Cy_App_NCP81239_Enable(cy_stc_app_ncp81239_context_t *context)
{
    bool status = false;
    uint8_t reg_addr;
    uint8_t reg_value;

    if(NULL == context)
    {
        return false;
    }

    reg_addr = PD_CTRL_EN_ADDR;
    reg_value = (1u << PD_CTRL_EN_MASK_POS) | (1u << PD_CTRL_EN_INT_POS);

    status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);
    return(status);
}

/* Disable NCP81239 */
bool Cy_App_NCP81239_Disable(cy_stc_app_ncp81239_context_t *context)
{
    bool status = false;
    uint8_t reg_addr;
    uint8_t reg_value;

    if(NULL == context)
    {
        return false;
    }

    reg_addr = PD_CTRL_EN_ADDR;

    status = Cy_App_I2CMaster_RegRead(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);

    if(status == true)
    {
        /* Clearing the mask and the interrupt bit */
        reg_value = reg_value & (~((1u << PD_CTRL_EN_MASK_POS) | (1u << PD_CTRL_EN_INT_POS)));

        status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                        &reg_addr, 1u, &reg_value, 1u, context->i2cContext);
    }

    return(status);
}

/* Sets the slew rate */
bool Cy_App_NCP81239_SetSlewRate(cy_stc_app_ncp81239_context_t *context, cy_en_ncp81239_slew_rates_t slew_rate)
{
    bool status = false;
    uint8_t reg_addr;
    uint8_t reg_value;

    if(NULL == context)
    {
        return false;
    }

    reg_addr = PD_CTRL_SKEW_RATE_REG_ADDR;
    reg_value = slew_rate;

    status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);

    return(status);
}

/* Set the PWM frequency */
bool Cy_App_NCP81239_SetPwmFrequency (cy_stc_app_ncp81239_context_t *context, cy_en_ncp81239_freq_t freq_khz)
{
    bool status = false;
    uint8_t reg_addr;
    uint8_t reg_value;

    if(NULL == context)
    {
        return false;
    }

    reg_addr = PD_CTRL_PWM_FREQ_ADDR;

    status = Cy_App_I2CMaster_RegRead(context->scbBase, context->i2cAddr, \
                                    &reg_addr, 1u, &reg_value, 1u, context->i2cContext);

    if(status == true)
    {
        reg_value = (reg_value & (~PD_CRTL_PWM_FREQ_MASK)) | freq_khz;

        status = Cy_App_I2CMaster_RegWrite(context->scbBase, context->i2cAddr, \
                                            &reg_addr, 1u, &reg_value, 1u, context->i2cContext);
    }

    return(status);
}
#endif /* CY_APP_BUCKBOOST_NCP81239_ENABLE */

/* [] END OF FILE */
