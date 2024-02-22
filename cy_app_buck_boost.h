/***************************************************************************//**
* \file cy_app_buck_boost.h
* \version 1.0
*
* \brief
* Defines function prototypes for buck-boost regulator
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/
#ifndef _CY_APP_BUCK_BOOST_H_
#define _CY_APP_BUCK_BOOST_H_

#include <stdbool.h>
#include <stdint.h>

#if CY_APP_BUCKBOOST_MP4247_ENABLE
#include "cy_app_mp4247.h"
#endif /* CY_APP_BUCKBOOST_MP4247_ENABLE */

#if CY_APP_BUCKBOOST_RT6190_ENABLE
#include "cy_app_rt6190.h"
#endif /* CY_APP_BUCKBOOST_RT6190_ENABLE */

#if CY_APP_BUCKBOOST_NCP81239_ENABLE
#include "cy_app_ncp81239.h"
#endif /* CY_APP_BUCKBOOST_NCP81239_ENABLE */

/**
* \addtogroup group_pmg_app_common_buck_boost
* \{
* The buck-boost driver provides APIs and data structures to control the external
* buck-boost controller. This library supports two types of buck-boost controllers,
* namely MP4247 from MPS<sup>&reg;</sup> and RT6190 from Richtek<sup>&reg;</sup>. 
* Only one controller driver can be enabled at a time. To enable the desired 
* controller driver, its respective macro, either <code>CY_APP_BUCKBOOST_MP4247_ENABLE</code> 
* or <code>CY_APP_BUCKBOOST_RT6190_ENABLE</code> should be enabled.
*
* <b>Features:</b>
* * Supports controller initialization
* * Enable/disable output voltage
* * Set output current limit
*
********************************************************************************
* \section section_pmg_app_common_buck_boost_config configuration considerations
********************************************************************************
* Buck-boost driver is built over the I2C master driver. I2C SCB block should be
* initialized as a master mode before using this driver. Refer to the
* <a href="https://infineon.github.io/mtb-pdl-cat2/pdl_api_reference_manual/html/index.html"><b>CAT2 Peripheral Driver Library</b></a>
* API reference manual for configuring the SCB block.
*
* The following steps describe the method for enabling buck-boost driver in an application.
*
* 1. Include buck_boost.h to get access to all functions and other declarations.
*    \snippet buck_boost_sut.c snippet_buck_boost_include
*
* 2. Define the I2C SCB context data structure.
*    \snippet buck_boost_sut.c snippet_buck_boost_i2c_context
*
* 3. Define the following data structures required by the buck-boost driver.
*    \snippet buck_boost_sut.c snippet_buck_boost_data_struct
*
* 4. Initialize the buck-boost driver.
*    \snippet buck_boost_sut.c snippet_buck_boost_init
*
* 5. Invoke the set voltage API to set the required voltage.
*    \snippet buck_boost_sut.c snippet_buck_boost_set_volt
*
*
* \defgroup group_pmg_app_common_buck_boost_data_structures data structures
* \defgroup group_pmg_app_common_buck_boost_functions functions
*/
/** \} group_pmg_app_common_buck_boost*/

/**
* \addtogroup group_pmg_app_common_buck_boost_functions
* \{
*/
/**
 * @brief Power Delivery buck-boost initialization for port 1.
 * I2C SCB block should be initialized before calling this function.
 * @param contextPtr - void pointer to controller context structure
 * @return Return true if the operation is success, otherwise false.
 */
bool Cy_App_BuckBoost_InitPort1(void *contextPtr);

/**
 * @brief Power Delivery buck-boost initialization for Port 2 .
 * I2C SCB block should be initialized before calling this function.
 * @param contextPtr - void pointer to controller context structure
 * @return Return true if the operation is success; otherwise false.
 */
bool Cy_App_BuckBoost_InitPort2(void *contextPtr);

/**
 * @brief Selects Power Delivery voltage by buck-boost regulator for port 1
 * @param vol_in_mv Voltage unit in PDO (mV)
 * @return Return true if the operation is success; otherwise false.
 */
bool Cy_App_BuckBoost_SetVoltPort1(uint16_t vol_in_mv);

/**
 * @brief Selects Power Delivery voltage by buck-boost regulator for port 2
 * @param vol_in_mv Voltage unit in PDO (mV)
 * @return Return true if the operation is success; otherwise false.
 */
bool Cy_App_BuckBoost_SetVoltPort2(uint16_t vol_in_mv);

/**
 * @brief Function enable/disable the buck-boost controller for port 1
 * @param output - true:enable false:disable
 * @return Return true if operation is success; otherwise false.
 */
bool Cy_App_BuckBoost_SetOutputPort1(bool output);

/**
 * @brief Function enable/disable the buck-boost controller for port 2
 * @param output - true:enable false:disable
 * @return Return true if the operation is success; otherwise false.
 */
bool Cy_App_BuckBoost_SetOutputPort2(bool output);

/** \} group_pmg_app_common_buck_boost_functions */
#endif /* _CY_APP_BUCK_BOOST_H_ */

/* [] END OF FILE */
