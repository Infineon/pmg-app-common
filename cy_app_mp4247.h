/***************************************************************************//**
* \file cy_app_mp4247.h
* \version 1.0
*
* \brief
* Defines data structures and function prototypes associated with 
* MP4247 buck-boost controller.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_MP4247_H_
#define _CY_APP_MP4247_H_

#include <stdbool.h>
#include <stdint.h>
#include "cy_scb_i2c.h"

/* For MP4247 buck-boost converter */

/* PD controller I2C address */
#define MP4247_DEFAULT_ADDR                       (0x67u)
#define MP4247_MAX_CURRENT_LIMIT                  (108u)

#define MP4247_OPER                               (0x01u)
#define MP4247_CLR_STS                            (0x03u)
#define MP4247_VOUT                               (0x21u)
#define MP4247_STATUS                             (0x79u)
#define MP4247_STS_TEMPERATURE                    (0x7Du)

#define MP4247_MFR_CTRL1_REG                      (0xD0)
#define MP4247_MFR_CTRL1_MODE_POS                 (0u)
#define MP4247_MFR_CTRL1_MODE_MSK                 (0x01u)
#define MP4247_MFR_CTRL1_MODE_AUTO_PWM            (0u << MP4247_MFR_CTRL1_MODE_POS)
#define MP4247_MFR_CTRL1_MODE_FORCE_PWM           (1u << MP4247_MFR_CTRL1_MODE_POS)
#define MP4247_MFR_CTRL1_OUT_DISCH_POS            (1u)
#define MP4247_MFR_CTRL1_OUT_DISCH_MSK            (0x02u)
#define MP4247_MFR_CTRL1_OUT_DISCH_DIS            (0u << MP4247_MFR_CTRL1_OUT_DISCH_POS)
#define MP4247_MFR_CTRL1_OUT_DISCH_EN             (1u << MP4247_MFR_CTRL1_OUT_DISCH_POS)
#define MP4247_MFR_CTRL1_OUT_OVP_POS              (2u)
#define MP4247_MFR_CTRL1_OUT_OVP_MSK              (0x04u)
#define MP4247_MFR_CTRL1_OUT_OVP_DIS              (0u << MP4247_MFR_CTRL1_OUT_OVP_POS)
#define MP4247_MFR_CTRL1_OUT_OVP_EN               (1u << MP4247_MFR_CTRL1_OUT_OVP_POS)
#define MP4247_MFR_CTRL1_SWA_FET_RON_POS          (3u)
#define MP4247_MFR_CTRL1_SWA_FET_RON_MSK          (0x18u)
#define MP4247_MFR_CTRL1_SWA_FET_RON_5M           (0u << MP4247_MFR_CTRL1_SWA_FET_RON_POS)
#define MP4247_MFR_CTRL1_SWA_FET_RON_10M          (1u << MP4247_MFR_CTRL1_SWA_FET_RON_POS)
#define MP4247_MFR_CTRL1_SWA_FET_RON_15M          (2u << MP4247_MFR_CTRL1_SWA_FET_RON_POS)
#define MP4247_MFR_CTRL1_SWA_FET_RON_20M          (3u << MP4247_MFR_CTRL1_SWA_FET_RON_POS)
#define MP4247_MFR_CTRL1_FREQ_POS                 (5u)
#define MP4247_MFR_CTRL1_FREQ_MSK                 (0x60u)
#define MP4247_MFR_CTRL1_280KHZ                   (0u << MP4247_MFR_CTRL1_FREQ_POS)
#define MP4247_MFR_CTRL1_420KHZ                   (1u << MP4247_MFR_CTRL1_FREQ_POS)
#define MP4247_MFR_CTRL1_600KHZ                   (2u << MP4247_MFR_CTRL1_FREQ_POS)
#define MP4247_MFR_CTRL1_DITHER_EN_POS            (7u)
#define MP4247_MFR_CTRL1_DITHER_EN_MSK            (0x80u)
#define MP4247_MFR_CTRL1_DITHER_DIS               (0u << MP4247_MFR_CTRL1_DITHER_EN_POS)
#define MP4247_MFR_CTRL1_DITHER_EN                (1u << MP4247_MFR_CTRL1_DITHER_EN_POS)

#define MP4247_CUR_LIMIT_REG                      (0xD1)
#define MP4247_CUR_LIMTT_POS                      (0u)
#define MP4247_CUR_LIMIT_MSK                      (0x7Fu)
#define MP4247_CUR_LIMIT_LDC_POS                  (7u)
#define MP4247_CUR_LIMIT_LDC_MSK                  (0x80u)
#define MP4247_CUR_LIMIT_LDC_EN                   (0u << MP4247_CUR_LIMIT_LDC_POS)
#define MP4247_CUR_LIMIT_LDC_DIS                  (1u << MP4247_CUR_LIMIT_LDC_POS)

#define MP4247_MFR_CTRL2_REG                      (0xD2)
#define MP4247_MFR_CTRL2_LDC_POS                  (0u)
#define MP4247_MFR_CTRL2_lDC_MSK                  (0x03u)
#define MP4247_MFR_CTRL2_LDC_NO                   (0x00u)
#define MP4247_MFR_CTRL2_LDC_100MV                (0x01u)
#define MP4247_MFR_CTRL2_LDC_300MV                (0x02u)
#define MP4247_MFR_CTRL2_LDC_600MV                (0x03u)

#define MP4247_MFR_CTRL3_REG                      (0xD3)
#define MP4247_MFR_CTRL3_FREQ_MODE_POS            (0u)
#define MP4247_MFR_CTRL3_FREQ_MODE_MSK            (0x01u)
#define MP4247_MFR_CTRL3_FREQ_MODE_HALF           (0u << MP4247_MFR_CTRL3_FREQ_MODE_POS)
#define MP4247_MFR_CTRL3_FREQ_MODE_SAME           (1u << MP4247_MFR_CTRL3_FREQ_MODE_POS)
#define MP4247_MFR_CTRL3_SLEW_RATE_FALL_POS       (1u)
#define MP4247_MFR_CTRL3_SLEW_RATE_FALL_MSK       (0x06u)
#define MP4247_MFR_CTRL3_SLEW_RATE_FALL_0_02MV    (0u << MP4247_MFR_CTRL3_SLEW_RATE_FALL_POS)
#define MP4247_MFR_CTRL3_SLEW_RATE_FALL_0_04MV    (1u << MP4247_MFR_CTRL3_SLEW_RATE_FALL_POS)
#define MP4247_MFR_CTRL3_SLEW_RATE_FALL_0_1MV     (2u << MP4247_MFR_CTRL3_SLEW_RATE_FALL_POS)
#define MP4247_MFR_CTRL3_SLEW_RATE_FALL_0_2MV     (3u << MP4247_MFR_CTRL3_SLEW_RATE_FALL_POS)
#define MP4247_MFR_CTRL3_SLEW_RATE_RISE_POS       (3u)
#define MP4247_MFR_CTRL3_SLEW_RATE_RISE_MSK       (0x18u)
#define MP4247_MFR_CTRL3_SLEW_RATE_RISE_0_08MV    (0u << MP4247_MFR_CTRL3_SLEW_RATE_RISE_POS)
#define MP4247_MFR_CTRL3_SLEW_RATE_RISE_0_16MV    (1u << MP4247_MFR_CTRL3_SLEW_RATE_RISE_POS)
#define MP4247_MFR_CTRL3_SLEW_RATE_RISE_0_4MV     (2u << MP4247_MFR_CTRL3_SLEW_RATE_RISE_POS)
#define MP4247_MFR_CTRL3_SLEW_RATE_RISE_0_8MV     (3u << MP4247_MFR_CTRL3_SLEW_RATE_RISE_POS)
#define MP4247_MFR_CTRL3_RSENS_POS                (5u)
#define MP4247_MFR_CTRL3_RSENS_MSK                (0x20u)
#define MP4247_MFR_CTRL3_RSENS_5M                 (0u << MP4247_MFR_CTRL3_RSENS_POS)
#define MP4247_MFR_CTRL3_RSENS_10M                (1u << MP4247_MFR_CTRL3_RSENS_POS)
#define MP4247_MFR_CTRL3_SW_CUR_LMT_POS           (6u)
#define MP4247_MFR_CTRL3_SW_CUR_LMT_MSK           (0xC0u)
#define MP4247_MFR_CTRL3_SW_CUR_LMT_8A            (0u << MP4247_MFR_CTRL3_SW_CUR_LMT_POS)
#define MP4247_MFR_CTRL3_SW_CUR_LMT_12A           (1u << MP4247_MFR_CTRL3_SW_CUR_LMT_POS)
#define MP4247_MFR_CTRL3_SW_CUR_LMT_15A           (2u << MP4247_MFR_CTRL3_SW_CUR_LMT_POS)
#define MP4247_MFR_CTRL3_SW_CUR_LMT_20A           (3u << MP4247_MFR_CTRL3_SW_CUR_LMT_POS)

#define MP4247_MFR_CTRL4_REG                      (0xD4)
#define MP4247_MFR_CTRL4_I2C_ADDR_POS             (0u)
#define MP4247_MFR_CTRL4_I2C_ADDR_MSK             (0x1Fu)
#define MP4247_MFR_CTRL4_SW2_EDGE_POS             (5u)
#define MP4247_MFR_CTRL4_SW2_EDGE_MSK             (0x20u)
#define MP4247_MFR_CTRL4_SW2_EDGE_NORMAL          (0u << MP4247_MFR_CTRL4_SW2_EDGE_POS)
#define MP4247_MFR_CTRL4_SW2_EDGE_FAST            (1u << MP4247_MFR_CTRL4_SW2_EDGE_POS)
#define MP4247_MFR_CTRL4_CC_BLANK_TIME_POS        (6u)
#define MP4247_MFR_CTRL4_CC_BLANK_TIME_Mask       (0xC0u)
#define MP4247_MFR_CTRL4_CC_BLANK_TIME_320US      (0u << MP4247_MFR_CTRL4_CC_BLANK_TIME_POS)
#define MP4247_MFR_CTRL4_CC_BLANK_TIME_4MS        (1u << MP4247_MFR_CTRL4_CC_BLANK_TIME_POS)
#define MP4247_MFR_CTRL4_CC_BLANK_TIME_8MS        (2u << MP4247_MFR_CTRL4_CC_BLANK_TIME_POS)
#define MP4247_MFR_CTRL4_CC_BLANK_TIME_12MS       (3u << MP4247_MFR_CTRL4_CC_BLANK_TIME_POS)

#define MP4247_MFR_STS_MSK_REG                    (0xD8)
#define MP4247_MFR_OTP_CONFIG_CODE_REG            (0xD9)
#define MP4247_MFR_OTP_REV_NUM_REG                (0xDA)


/**
* \addtogroup group_pmg_app_common_buck_boost_data_structures
* \{
*/
/**
* MP4247 buck-boost regulator data structure
*/
typedef struct mp4247_context
{
    /** I2C 7-bit address */
    uint8_t i2cAddr;

    /** I2C SCB base address */
    CySCB_Type *scbBase;

    /** I2C SCB context pointer */
    cy_stc_scb_i2c_context_t *i2cContext;

    /** Controller enable pin GPIO address*/
    GPIO_PRT_Type *enableGpioPort;

    /** Controller enable pin GPIO number */
    uint8_t enableGpioPin;

    /** Ratio between output voltage and feedback voltage */
    uint8_t fbRatio;

}cy_stc_app_mp4247_context_t;

/** \} group_pmg_app_common_buck_boost_data_structures */

/**
* \addtogroup group_pmg_app_common_buck_boost_functions
* \{
*/
/**
 * @brief MP4247 power delivery controller initialize.
 * I2C SCB block should be initialized and all the structure member of
 * cy_stc_app_mp4247_context_t should be initialized before calling this function.
 * @param context - Pointer to the MP4247 data structure context.
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_MP4247_Init(cy_stc_app_mp4247_context_t *context);

/**
 * @brief Enable output voltage from MP4247
 * @param context - Pointer to the MP4247 data structure context
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_MP4247_Enable(cy_stc_app_mp4247_context_t *context);

/**
 * @brief Disable output voltage from MP4247
 * @param context - Pointer to the MP4247 data structure context
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_MP4247_Disable(cy_stc_app_mp4247_context_t *context);

/**
 * @brief Selects Power Delivery voltage by MP4247
 * @param context - Pointer to the MP4247 data structure context
 * @param vol_in_mv - Voltage unit in PDO (mV)
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_MP4247_SetVolt(cy_stc_app_mp4247_context_t *context, uint16_t vol_in_mv);

/**
 * @brief Sets the current limit
 * @param context - Pointer to the MP4247 data structure context
 * @param cur_in_10ma - current limit value (10 mA)
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_MP4247_SetCurrentLimit(cy_stc_app_mp4247_context_t *context, uint16_t cur_in_10ma);

/**
 * @brief Sets the VOut rising slew rate. As a slew_rate parameter, use the slew rate
 * value provided by the macros beginning with MP4247_MFR_CTRL3_SLEW_RATE_RISE_0_XXXX.
 *
 * @param context - Pointer to the MP4247 data structure context
 * @param slew_rate - VOut rising slew rate
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_MP4247_SetSlewRateRise(cy_stc_app_mp4247_context_t *context, uint8_t slew_rate);

/**
 * @brief Sets the VOut falling slew rate. As a slew_rate parameter, use the slew rate
 * value provided by the macros beginning with MP4247_MFR_CTRL3_SLEW_RATE_FALL_0_XXXX.
 *
 * @param context - Pointer to the MP4247 data structure context
 * @param slew_rate - VOut falling slew rate
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_MP4247_SetSlewRateFall(cy_stc_app_mp4247_context_t *context, uint8_t slew_rate);

/** \} group_pmg_app_common_buck_boost_functions */
#endif /* _CY_APP_MP4247_H_ */

/* [] END OF FILE */
