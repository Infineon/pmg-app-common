/***************************************************************************//**
* \file cy_app_i2c_master.h
* \version 1.0
*
* \brief
* Defines data structures and function prototypes
* associated with i2c master.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_I2C_MASTER_H_
#define _CY_APP_I2C_MASTER_H_

#include <stdbool.h>
#include <stdint.h>
#include "cy_pdl.h"

/**
* \addtogroup group_pmg_app_common_i2cm
* \{
* The I2C master provides APIs for transferring data to/from I2C slave device 
* registers. This module is built on top of the I2C SCB driver that is 
* available as part of PDL CAT2.
*
* <b>Features:</b>
* * Read and Write Registers
*
********************************************************************************
* \section section_pmg_app_common_i2cm_config Configuration considerations
********************************************************************************
* The following steps describe the way to enable the I2C master.
*
* 1. Include cy_app_i2c_master.h to get access to all functions and other declarations.
*    \snippet i2c_master_sut.c snippet_i2cm_include
* 
* 2. Configure the SCB block as I2C master using Device Configurator link available 
*    in the ModusToolbox&tm; quick panel. Refer to the 
*    <a href="https://infineon.github.io/mtb-pdl-cat2/pdl_api_reference_manual/html/index.html">
*    <b>PDL API reference</b></a>.
*
* 3. Define the I2C SCB context data structure
*    \snippet i2c_master_sut.c snippet_i2cm_data_structure
*
* 4. Invoke the read API to read data from the slave device registers
*    \snippet i2c_master_sut.c snippet_i2cm_read
*
* 5. Invoke the write API to write data to the slave device registers
*    \snippet i2c_master_sut.c snippet_i2cm_write
*
* \defgroup group_pmg_app_common_i2cm_functions Functions
*/
/** \} group_pmg_app_common_i2cm */


/**
* \addtogroup group_pmg_app_common_i2cm_functions
* \{
*/
/*******************************************************************************
* Function name: Cy_App_I2CMaster_Read
****************************************************************************//**
*
* This function read data from the slave device
*
* \param base
* SCB block base address
*
* \param addr
* Slave device I2C address (7-bit)
*
* \param buffer
* Pointer to the read data buffer
*
* \param count
* Read data length in bytes
*
* \param context
* Pointer to the I2C SCB context structure
*
* \return
* True on success; false otherwise.
*
*******************************************************************************/
bool Cy_App_I2CMaster_Read(CySCB_Type *base, 
                           uint8_t addr, 
                           uint8_t *buffer, 
                           uint32_t count,
                           cy_stc_scb_i2c_context_t *context);
/*******************************************************************************
* Function name: Cy_App_I2CMaster_Write
****************************************************************************//**
*
* This function write data to the slave device
*
* \param base
* SCB block base address
*
* \param addr
* Slave device I2C address (7-bit)
*
* \param buffer
* Pointer to the write data buffer
*
* \param count
* Write data length in bytes
*
* \param context
* Pointer to the I2C SCB context structure
*
* \return
* True on success; false otherwise.
*
*******************************************************************************/
bool Cy_App_I2CMaster_Write(CySCB_Type *base, 
                            uint8_t addr, 
                            uint8_t *buffer, 
                            uint32_t count,
                            cy_stc_scb_i2c_context_t *context);

/*******************************************************************************
* Function name: Cy_App_I2CMaster_RegRead
****************************************************************************//**
*
* This function reads data from the given register of the slave
*
* \param base
* SCB block base address
*
* \param addr
* Slave device I2C address (7-bit)
*
* \param reg_addr
* Pointer to the register address buffer
*
* \param reg_size
* Length of the register in bytes
*
* \param buffer
* Pointer to the read data buffer
*
* \param count
* Read data length in bytes
*
* \param context
* Pointer to the I2C SCB context structure
*
* \return
* True on success; false otherwise.
*
*******************************************************************************/
bool Cy_App_I2CMaster_RegRead(CySCB_Type *base, 
                              uint8_t addr, 
                              uint8_t *reg_addr, 
                              uint8_t reg_size,
                              uint8_t *buffer, 
                              uint32_t count,
                              cy_stc_scb_i2c_context_t *context);

/**
 * @brief writes data to the given register of the slave
 */
/*******************************************************************************
* Function name: Cy_App_I2CMaster_RegWrite
****************************************************************************//**
*
* This function writes data to the given register of the slave
*
* \param base
* SCB block base address
*
* \param addr
* Slave device I2C address (7-bit)
*
* \param reg_addr
* Pointer to the register address buffer
*
* \param reg_size
* Length of the register in bytes
*
* \param buffer
* Pointer to the write data buffer
*
* \param count
* Write data length in bytes
*
* \param context
* Pointer to the I2C SCB context structure
*
* \return
* True on success; false otherwise.
*
*******************************************************************************/ 
bool Cy_App_I2CMaster_RegWrite(CySCB_Type *base, 
                               uint8_t addr, 
                               uint8_t *reg_addr, 
                               uint8_t reg_size,
                               uint8_t *buffer, 
                               uint32_t count, 
                               cy_stc_scb_i2c_context_t *context);

/** \} group_pmg_app_common_i2cm_functions */

#endif /* _CY_APP_I2C_MASTER_H_ */

/* [] END OF FILE */
