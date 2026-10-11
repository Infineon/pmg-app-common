/***************************************************************************//**
* \file cy_app_i2c_master.h
* \version 3.0
*
* \brief
* Defines data structures and function prototypes
* associated with i2c master.
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

#ifndef CY_APP_I2C_MASTER_H
#define CY_APP_I2C_MASTER_H

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

/**
 * @brief reads data from the given register of the slave using repeated start
 */
/*******************************************************************************
* Function name: Cy_App_I2CMaster_RegReStartRead
****************************************************************************//**
*
* This function reads data from the given register of the slave using repeated
* start, after sending register address
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
bool Cy_App_I2CMaster_RegReStartRead(CySCB_Type *base, uint8_t addr, uint8_t *reg_addr, uint8_t reg_size,
                              uint8_t *buffer, uint32_t count, cy_stc_scb_i2c_context_t *context);

/** \} group_pmg_app_common_i2cm_functions */

#endif /* CY_APP_I2C_MASTER_H */

/* [] END OF FILE */
