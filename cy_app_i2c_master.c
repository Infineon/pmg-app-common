/***************************************************************************//**
* \file cy_app_i2c_master.c
* \version 3.0
*
* \brief
* Implements the functions associated with i2c master
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

#include "cy_pdl.h"
#include "cy_app_i2c_master.h"

/* Waiting time in milliseconds for the I2C operation to complete */
#define I2CM_TIMER_PERIOD           (10U)

/* Transfer data to slave */
bool Cy_App_I2CMaster_Write(CySCB_Type *base, uint8_t addr, uint8_t *buffer, uint32_t count,
                     cy_stc_scb_i2c_context_t *context)
{
    cy_en_scb_i2c_status_t status;
    uint32_t timeout = I2CM_TIMER_PERIOD;

    /* Send start condition, address, and receive ACK/NACK response from slave */
    status = Cy_SCB_I2C_MasterSendStart(base, addr, CY_SCB_I2C_WRITE_XFER, timeout, context);
    if (status == CY_SCB_I2C_SUCCESS)
    {
        uint32_t cnt = 0UL;
        /* Write data into the slave from the buffer */
        do
        {
            /* Write byte and receive ACK/NACK response */
            status = Cy_SCB_I2C_MasterWriteByte(base, buffer[cnt], timeout, context);
            ++cnt;
        }
        while((status == CY_SCB_I2C_SUCCESS) && (cnt < count));
    }
    /* Check status of transaction */
    if ((status == CY_SCB_I2C_SUCCESS)           ||
        (status == CY_SCB_I2C_MASTER_MANUAL_NAK) ||
        (status == CY_SCB_I2C_MASTER_MANUAL_ADDR_NAK))
    {
        /* Send stop condition on the bus */
        (void)Cy_SCB_I2C_MasterSendStop(base, timeout, context);
    }

    return (status == CY_SCB_I2C_SUCCESS) ? true : false;
}

/* Receive data from slave */
bool Cy_App_I2CMaster_Read(CySCB_Type *base, uint8_t addr, uint8_t *buffer, uint32_t count,
                    cy_stc_scb_i2c_context_t *context)
{
    cy_en_scb_i2c_status_t status;
    uint32_t timeout = I2CM_TIMER_PERIOD;

    /* Send start condition, address, and receive ACK/NACK response from slave */
    status = Cy_SCB_I2C_MasterSendStart(base, addr, CY_SCB_I2C_READ_XFER, timeout, context);
    if (status == CY_SCB_I2C_SUCCESS)
    {
        uint32_t cnt = 0UL;
        /* Write data into the slave from the buffer */
        do
        {
            /* Read byte and send ACK/NACK response */
            status = Cy_SCB_I2C_MasterReadByte(base, ((cnt + 1u == count) ? 
                CY_SCB_I2C_NAK : CY_SCB_I2C_ACK), &buffer[cnt], timeout, context);
            ++cnt;
        }
        while((status == CY_SCB_I2C_SUCCESS) && (cnt < count));
    }
    /* Check status of transaction */
    if ((status == CY_SCB_I2C_SUCCESS)           ||
        (status == CY_SCB_I2C_MASTER_MANUAL_NAK) ||
        (status == CY_SCB_I2C_MASTER_MANUAL_ADDR_NAK))
    {
        /* Send stop condition on the bus */
        (void)Cy_SCB_I2C_MasterSendStop(base, timeout, context);
    }

    return (status == CY_SCB_I2C_SUCCESS) ? true : false;
}

bool Cy_App_I2CMaster_RegRead(CySCB_Type *base, uint8_t addr, uint8_t *reg_addr, uint8_t reg_size,
                              uint8_t *buffer, uint32_t count, cy_stc_scb_i2c_context_t *context)
{
    bool status = false;

    if ((base == NULL) || (context == NULL) || (reg_addr == NULL) ||
         (reg_size == 0u) || (buffer == NULL) || (count == 0u))
    {
        return false;
    }

    status = Cy_App_I2CMaster_Write(base, addr, reg_addr, reg_size, context);
    if(status == true)
    {
        status = Cy_App_I2CMaster_Read(base, addr, buffer, count, context);
    }

    return status;
}

bool Cy_App_I2CMaster_RegWrite(CySCB_Type *base, uint8_t addr, uint8_t *reg_addr, uint8_t reg_size,
                               uint8_t *buffer, uint32_t count, cy_stc_scb_i2c_context_t *context)
{
    cy_en_scb_i2c_status_t status;
    uint32_t timeout = I2CM_TIMER_PERIOD;

    if ((base == NULL) || (context == NULL) || (reg_addr == NULL) || (reg_size == 0u))
    {
        return false;
    }

    status = Cy_SCB_I2C_MasterSendStart(base, addr, CY_SCB_I2C_WRITE_XFER, timeout, context);
    if (status == CY_SCB_I2C_SUCCESS)
    {
        uint32_t cnt = 0UL;
        /* Write data into the slave from the buffer */
        do
        {
            /* Write byte and receive ACK/NACK response */
            status = Cy_SCB_I2C_MasterWriteByte(base, reg_addr[cnt], timeout, context);
            ++cnt;
        }
        while((status == CY_SCB_I2C_SUCCESS) && (cnt < reg_size));

        /* Write the register data if the register address write is success and there are
         * register data to write; else send the stop bit.
         */
        if((status == CY_SCB_I2C_SUCCESS) && (buffer != NULL) && (count != 0u))
        {
            cnt = 0UL;
            do
            {
                /* Write byte and receive ACK/NACK response */
                status = Cy_SCB_I2C_MasterWriteByte(base, buffer[cnt], timeout, context);
                ++cnt;
            }
            while((status == CY_SCB_I2C_SUCCESS) && (cnt < count));
        }
    }

    if ((status == CY_SCB_I2C_SUCCESS) ||
        (status == CY_SCB_I2C_MASTER_MANUAL_NAK) ||
        (status == CY_SCB_I2C_MASTER_MANUAL_ADDR_NAK))
    {
        /* Send stop condition on the bus */
        (void)Cy_SCB_I2C_MasterSendStop(base, timeout, context);
    }

    return (status == CY_SCB_I2C_SUCCESS) ? true : false;
}

/* Read data from slave with Repeated start. */
bool Cy_App_I2CMaster_RegReStartRead(CySCB_Type *base, uint8_t addr, uint8_t *reg_addr, uint8_t reg_size,
                              uint8_t *buffer, uint32_t count, cy_stc_scb_i2c_context_t *context)
{
    cy_en_scb_i2c_status_t status;
    uint32_t timeout = 10u;

    /* Send start condition, address, and receive ACK/NACK response from slave */
    status = Cy_SCB_I2C_MasterSendStart(base, addr, CY_SCB_I2C_WRITE_XFER, timeout, context);
    if (status == CY_SCB_I2C_SUCCESS)
    {
        uint32_t cnt = 0UL;
        /* Write data into the slave from the buffer */
        do
        {
            /* Write byte and receive ACK/NACK response */
            status = Cy_SCB_I2C_MasterWriteByte(base, reg_addr[cnt], timeout, context);
            ++cnt;
        }
        while((status == CY_SCB_I2C_SUCCESS) && (cnt < reg_size));
    }

    /* Send start condition, address, and receive ACK/NACK response from slave */
    status = Cy_SCB_I2C_MasterSendReStart(base, addr, CY_SCB_I2C_READ_XFER, timeout, context);
    if (status == CY_SCB_I2C_SUCCESS)
    {
        uint32_t cnt = 0UL;
        /* Write data into the slave from the buffer */
        do
        {
            /* Read byte and send ACK/NACK response */
            status = Cy_SCB_I2C_MasterReadByte(base, ((cnt + 1u == count) ?
                CY_SCB_I2C_NAK : CY_SCB_I2C_ACK), &buffer[cnt], timeout, context);
            ++cnt;
        }
        while((status == CY_SCB_I2C_SUCCESS) && (cnt < count));
    }
    /* Check status of transaction */
    if ((status == CY_SCB_I2C_SUCCESS)           ||
        (status == CY_SCB_I2C_MASTER_MANUAL_NAK) ||
        (status == CY_SCB_I2C_MASTER_MANUAL_ADDR_NAK))
    {
        /* Send stop condition on the bus */
        (void)Cy_SCB_I2C_MasterSendStop(base, timeout, context);
    }

    return (status == CY_SCB_I2C_SUCCESS) ? true : false;
}


/* [] END OF FILE */
