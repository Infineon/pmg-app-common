/***************************************************************************//**
* \file cy_app_rt6190.h
* \version 1.0
*
* \brief
* Defines the data structures and function prototypes associated with 
* RT6190 buck-boost controller
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_RT6190_H_
#define _CY_APP_RT6190_H_

#include <stdbool.h>
#include <stdint.h>
#include "cy_scb_i2c.h"

/* RT6190 controller I2C address */
#define RT6190_CTRL_ADDR                      (0x2Cu)

#define RT6190_MODE_INVALID                   (0xFFu)

#define RT6190_VOLTAGE_INVALID                (0xFFFFu)

#define RT6190_CURRENT_INVALID                (0xFFFFu)

#define RT6190_ERR_FLAG_INVALID               (0xFFFFu)

/* Register space of RT6190 buck-boost controller */

/* Manufacturer ID */
#define RT6190_MANUFACTURER_ID                (0x00u)

/* Lower 8-bits of 11-bit output constant voltage setting */
#define RT6190_OUTPUT_CV_LSB_REG              (0x01u)
/* Upper 3-bits of 11-bit output constant voltage setting */
#define RT6190_OUTPUT_CV_MSB_REG              (0x02u)

/* Lower 8-bits of 9-bit output constant current setting */
#define RT6190_OUTPUT_CC_LSB_REG              (0x03u)
/* Upper 1-bit of 9-bit output constant current setting */
#define RT6190_OUTPUT_CC_MSB_REG              (0x04u)

/* Minimum input constant voltage (CV) setting. */
#define RT6190_INPUT_CV_REG                   (0x05u)

/* Lower 8 bits of 9-bit input constant current (CC) setting */
#define RT6190_INPUT_CC_LSB_REG               (0x06u)
/* Upper 1 bit of 9-bit input constant current (CC) setting */
#define RT6190_INPUT_CC_MSB_REG               (0x07u)

/* Slope compensation ramp setting for internal use */
#define RT6190_VREF_SC_REG                    (0x08u)

/* Vcomp gain and minimum peak current setting for TON in PSM */
#define RT6190_VREF_PSM_REG                   (0x09u)
#define RT6190_VREF_PSM_POS                   (0u)
#define RT6190_VREF_PSM_MSK                   (0x1Fu)
#define RT6190_VREF_GAIN_VCOMP_POS            (6u)
#define RT6190_VREF_GAIN_VCOMP_MSK            (0x30u)

/* Input peak current limit setting */
#define RT6190_VREF_POCP_REG                  (0x0Au)

/* OVP delay time and threshold setting */
#define RT6190_OVP_REG                        (0x0Bu)
#define RT6190_OVP_LEVEL_POS                  (0u)
#define RT6190_OVP_LEVEL_MSK                  (0x03u)
#define RT6190_OVP_LEVEL_115_PERCENT          (1u << RT6190_OVP_LEVEL_POS)
#define RT6190_OVP_LEVEL_120_PERCENT          (2u << RT6190_OVP_LEVEL_POS)
#define RT6190_OVP_LEVEL_125_PERCENT          (3u << RT6190_OVP_LEVEL_POS)
#define RT6190_OVP_DELAY_EXT_SET_POS          (2u)
#define RT6190_OVP_DELAY_EXT_SET_MSK          (0xC0u)
#define RT6190_OVP_DELAY_EXT_SET_8US          (0u << RT6190_OVP_DELAY_EXT_SET_POS)
#define RT6190_OVP_DELAY_EXT_SET_16US         (1u << RT6190_OVP_DELAY_EXT_SET_POS)
#define RT6190_OVP_DELAY_EXT_SET_32US         (2u << RT6190_OVP_DELAY_EXT_SET_POS)
#define RT6190_OVP_DELAY_EXT_SET_64US         (3u << RT6190_OVP_DELAY_EXT_SET_POS)
#define RT6190_OVP_DELAY_INT_SET_POS          (4u)
#define RT6190_OVP_DELAY_INT_SET_MSK          (0x30u)
#define RT6190_OVP_DELAY_INT_SET_96US         (0u << RT6190_OVP_DELAY_INT_SET_POS)
#define RT6190_OVP_DELAY_INT_SET_192US        (1u << RT6190_OVP_DELAY_INT_SET_POS)
#define RT6190_OVP_DELAY_INT_SET_288US        (2u << RT6190_OVP_DELAY_INT_SET_POS)
#define RT6190_OVP_DELAY_INT_SET_386US        (3u << RT6190_OVP_DELAY_INT_SET_POS)

/* Enable or disable OVP operation and UVP delay time setting */
#define RT6190_UVP_REG                        (0x0Cu)
#define RT6190_UVP_LEVEL_POS                  (0u)
#define RT6190_UVP_LEVEL_MSK                  (0x03u)
#define RT6190_UVP_LEVEL_50_PERCENT           (0u << RT6190_UVP_LEVEL_POS)
#define RT6190_UVP_LEVEL_60_PERCENT           (1u << RT6190_UVP_LEVEL_POS)
#define RT6190_UVP_LEVEL_70_PERCENT           (2u << RT6190_UVP_LEVEL_POS)
#define RT6190_UVP_LEVEL_80_PERCENT           (3u << RT6190_UVP_LEVEL_POS)
#define RT6190_UVP_DELAY_EXT_SET_POS          (2u)
#define RT6190_UVP_DELAY_EXT_SET_MSK          (0x0Cu)
#define RT6190_UVP_DELAY_EXT_SET_32US         (0u << RT6190_UVP_DELAY_EXT_SET_POS)
#define RT6190_UVP_DELAY_EXT_SET_64US         (1u << RT6190_UVP_DELAY_EXT_SET_POS)
#define RT6190_UVP_DELAY_EXT_SET_128US        (2u << RT6190_UVP_DELAY_EXT_SET_POS)
#define RT6190_UVP_DELAY_EXT_SET_256US        (3u << RT6190_UVP_DELAY_EXT_SET_POS)
#define RT6190_UVP_DELAY_INT_SET_POS          (4u)
#define RT6190_UVP_DELAY_INT_SET_MSK          (0x30u)
#define RT6190_UVP_DELAY_INT_SET_256US        (0u << RT6190_UVP_DELAY_INT_SET_POS)
#define RT6190_UVP_DELAY_INT_SET_512US        (1u << RT6190_UVP_DELAY_INT_SET_POS)
#define RT6190_UVP_DELAY_INT_SET_768US        (2u << RT6190_UVP_DELAY_INT_SET_POS)
#define RT6190_UVP_DELAY_INT_SET_1024US       (3u << RT6190_UVP_DELAY_INT_SET_POS)
#define RT6190_POWER_ROLE_POS                 (6u)
#define RT6190_POWER_ROLE_MSK                 (0x40u)
#define RT6190_POWER_ROLE_FORWARD_OP          (0u << RT6190_POWER_ROLE_POS)
#define RT6190_POWER_ROLE_REVERSE_OP          (1u << RT6190_POWER_ROLE_POS)
#define RT6190_IN_OVP_POS                     (7u)
#define RT6190_IN_OVP_MSK                     (0x80u)
#define RT6190_IN_OVP_DIS                     (0u << RT6190_IN_OVP_POS)
#define RT6190_IN_OVP_EN                      (1u << RT6190_IN_OVP_POS)

/* Operation mode, slew rate, and switching frequency setting. */
#define RT6190_SET1_REG                       (0x0D)
#define RT6190_SET1_FSM_POS                   (0u)
#define RT6190_SET1_FSM_MSK                   (0x07u)
#define RT6190_SET1_FSM_250KHZ                (0u << RT6190_SET1_FSM_POS)
#define RT6190_SET1_FSM_325KHZ                (1u << RT6190_SET1_FSM_POS)
#define RT6190_SET1_FSM_400KHZ                (2u << RT6190_SET1_FSM_POS)
#define RT6190_SET1_FSM_500KHZ                (3u << RT6190_SET1_FSM_POS)
#define RT6190_SET1_FSM_615KHZ                (4u << RT6190_SET1_FSM_POS)
#define RT6190_SET1_FSM_730KHZ                (5u << RT6190_SET1_FSM_POS)
#define RT6190_SET1_FSM_845KHZ                (6u << RT6190_SET1_FSM_POS)
#define RT6190_SET1_FSM_960KHZ                (7u << RT6190_SET1_FSM_POS)
/* Slew rate = Delta VOUT/SLEWRATE */
#define RT6190_SET1_SLEWRATE_F_POS            (3u)
#define RT6190_SET1_SLEWRATE_F_MSK            (0x18u)
#define RT6190_SET1_SLEWRATE_F_VOUT_4US       (0u << RT6190_SET1_SLEWRATE_F_POS)
#define RT6190_SET1_SLEWRATE_F_VOUT_8US       (1u << RT6190_SET1_SLEWRATE_F_POS)
#define RT6190_SET1_SLEWRATE_F_VOUT_16US      (2u << RT6190_SET1_SLEWRATE_F_POS)
#define RT6190_SET1_SLEWRATE_F_VOUT_32US      (3u << RT6190_SET1_SLEWRATE_F_POS)
#define RT6190_SET1_SLEWRATE_R_POS            (5u)
#define RT6190_SET1_SLEWRATE_R_MSK            (0x60u)
#define RT6190_SET1_SLEWRATE_R_VOUT_4US       (0u << RT6190_SET1_SLEWRATE_R_POS)
#define RT6190_SET1_SLEWRATE_R_VOUT_8US       (1u << RT6190_SET1_SLEWRATE_R_POS)
#define RT6190_SET1_SLEWRATE_R_VOUT_16US      (2u << RT6190_SET1_SLEWRATE_R_POS)
#define RT6190_SET1_SLEWRATE_R_VOUT_32US      (3u << RT6190_SET1_SLEWRATE_R_POS)
#define RT6190_SET1_F_CCM_POS                 (7u)
#define RT6190_SET1_F_CCM_MSK                 (0x80u)
#define RT6190_SET1_F_CCM_LIGHT_LOAD_PSM      (0u << RT6190_SET1_F_CCM_POS)
#define RT6190_SET1_F_CCM_FORCE_CCM           (1u << RT6190_SET1_F_CCM_POS)

/* Enabling RT6190, output discharge resistor and cable voltage drop setting. */
#define RT6190_SET2_REG                       (0x0Eu)
#define RT6190_SET2_IR_COMPR_POS              (0u)
#define RT6190_SET2_IR_COMPR_MSK              (0x07u)
#define RT6190_SET2_IR_COMPR_DISABLE          (0u << RT6190_SET2_IR_COMPR_POS)
#define RT6190_SET2_IR_COMPR_10MOHM           (1u << RT6190_SET2_IR_COMPR_POS)
#define RT6190_SET2_IR_COMPR_20MOHM           (2u << RT6190_SET2_IR_COMPR_POS)
#define RT6190_SET2_IR_COMPR_40MOHM           (3u << RT6190_SET2_IR_COMPR_POS)
#define RT6190_SET2_IR_COMPR_80MOHM           (4u << RT6190_SET2_IR_COMPR_POS)
#define RT6190_SET2_IR_COMPR_120MOHM          (5u << RT6190_SET2_IR_COMPR_POS)
#define RT6190_SET2_IR_COMPR_160MOHM          (6u << RT6190_SET2_IR_COMPR_POS)
#define RT6190_SET2_IR_COMPR_200MOHM          (7u << RT6190_SET2_IR_COMPR_POS)
#define RT6190_SET2_DISCHARGE_POS             (4u)
#define RT6190_SET2_DISCHARGE_MSK             (0x10u)
#define RT6190_SET2_DISCHARGE_DIS             (0u << RT6190_SET2_DISCHARGE_POS)
#define RT6190_SET2_DISCHARGE_EN              (1u << RT6190_SET2_DISCHARGE_POS)
#define RT6190_SET2_INCC_POS                  (5u)
#define RT6190_SET2_INCC_MSK                  (0x20u)
#define RT6190_SET2_INCC_EN                   (0u << RT6190_SET2_INCC_POS)
#define RT6190_SET2_INCC_DIS                  (1u << RT6190_SET2_INCC_POS)
#define RT6190_SET2_INCV_POS                  (6u)
#define RT6190_SET2_INCV_MSK                  (0x40u)
#define RT6190_SET2_INCV_EN                   (0u << RT6190_SET2_INCV_POS)
#define RT6190_SET2_INCV_DIS                  (1u << RT6190_SET2_INCV_POS)
#define RT6190_SET2_PWM_POS                   (7u)
#define RT6190_SET2_PWM_MSK                   (0x80u)
#define RT6190_SET2_PWM_DIS                   (0u << RT6190_SET2_PWM_POS)
#define RT6190_SET2_PWM_EN                    (1u << RT6190_SET2_PWM_POS)

/* Dead time, Error amplifier gain, and current sense gain setting. */
#define RT6190_SET3_REG                       (0x0Fu)
#define RT6190_SET3_GAIN_OCS_POS              (0u)
#define RT6190_SET3_GAIN_OCS_MSK              (0x03u)
#define RT6190_SET3_GAIN_OCS_20X              (1u << RT6190_SET3_GAIN_OCS_POS)
#define RT6190_SET3_GAIN_OCS_10X              (0u << RT6190_SET3_GAIN_OCS_POS)
#define RT6190_SET3_GAIN_OCS_30X              (2u << RT6190_SET3_GAIN_OCS_POS)
#define RT6190_SET3_GAIN_OCS_40X              (3u << RT6190_SET3_GAIN_OCS_POS)
#define RT6190_SET3_GAIN_ICS_POS              (2u)
#define RT6190_SET3_GAIN_ICS_MSK              (0xC0u)
#define RT6190_SET3_GAIN_ICS_10X              (0u << RT6190_SET3_GAIN_ICS_POS)
#define RT6190_SET3_GAIN_ICS_20X              (1u << RT6190_SET3_GAIN_ICS_POS)
#define RT6190_SET3_GAIN_ICS_30X              (2u << RT6190_SET3_GAIN_ICS_POS)
#define RT6190_SET3_GAIN_ICS_40X              (3u << RT6190_SET3_GAIN_ICS_POS)
#define RT6190_SET3_GM_EA_POS                 (4u)
#define RT6190_SET3_GM_EA_MSK                 (0x30u)
#define RT6190_SET3_GM_EA_275UA_V             (0u << RT6190_SET3_GM_EA_POS)
#define RT6190_SET3_GM_EA_550UA_V             (1u << RT6190_SET3_GM_EA_POS)
#define RT6190_SET3_GM_EA_825UA_V             (2u << RT6190_SET3_GM_EA_POS)
#define RT6190_SET3_GM_EA_1100UA_V            (3u << RT6190_SET3_GM_EA_POS)
#define RT6190_SET3_DT_SEL_POS                (6u)
#define RT6190_SET3_DT_SEL_MSK                (0xC0u)
#define RT6190_SET3_DT_SEL_30NS               (0u << RT6190_SET3_DT_SEL_POS)
#define RT6190_SET3_DT_SEL_50NS               (1u << RT6190_SET3_DT_SEL_POS)
#define RT6190_SET3_DT_SEL_70NS               (2u << RT6190_SET3_DT_SEL_POS)
#define RT6190_SET3_DT_SEL_90NS               (3u << RT6190_SET3_DT_SEL_POS)

/* Enabling driver charge function and ADC function, selecting I2C speed, and ADC average times. */
#define RT6190_SET4_REG                       (0x10u)
#define RT6190_SET4_DRIVER_CHARGE_POS         (0u)
#define RT6190_SET4_DRIVER_CHARGE_MSK         (0x01u)
#define RT6190_SET4_DRIVER_CHARGE_DIS         (0u << RT6190_SET4_DRIVER_CHARGE_POS)
#define RT6190_SET4_DRIVER_CHARGE_EN          (1u << RT6190_SET4_DRIVER_CHARGE_POS)
#define RT6190_SET4_ADC_POS                   (1u)
#define RT6190_SET4_ADC_MSK                   (0x02u)
#define RT6190_SET4_ADC_DIS                   (0u << RT6190_SET4_ADC_POS)
#define RT6190_SET4_ADC_EN                    (1u << RT6190_SET4_ADC_POS)
#define RT6190_SET4_OCP4_TIME_POS             (4u)
#define RT6190_SET4_OCP4_TIME_MSK             (0x10u)
#define RT6190_SET4_OCP4_TIME_X1              (0u << RT6190_SET4_OCP4_TIME_POS)
#define RT6190_SET4_OCP4_TIME_X10             (1u << RT6190_SET4_OCP4_TIME_POS)
#define RT6190_SET4_I2C_SPEED_POS             (5u)
#define RT6190_SET4_I2C_SPEED_MSK             (0x20u)
#define RT6190_SET4_I2C_SPEED_300KHZ          (0u << RT6190_SET4_I2C_SPEED_POS)
#define RT6190_SET4_I2C_SPEED_1MHZ            (1u << RT6190_SET4_I2C_SPEED_POS)
#define RT6190_SET4_ADC_AVG_SEL_POS           (6u)
#define RT6190_SET4_ADC_AVG_SEL_MSK           (0xC0u)
#define RT6190_SET4_ADC_AVG_SEL_2X            (0u << RT6190_SET4_ADC_AVG_SEL_POS)
#define RT6190_SET4_ADC_AVG_SEL_4X            (1u << RT6190_SET4_ADC_AVG_SEL_POS)
#define RT6190_SET4_ADC_AVG_SEL_8X            (2u << RT6190_SET4_ADC_AVG_SEL_POS)
#define RT6190_SET4_ADC_AVG_SEL_16X           (3u << RT6190_SET4_ADC_AVG_SEL_POS)

/* VIN and VOUT ratio selection and enabling spread spectrum function */
#define RT6190_RATIO_REG                      (0x11u)
#define RT6190_RATIO_CHIP_VERSION_POS         (0u)
#define RT6190_RATIO_CHIP_VERSION_MSK         (0x0Fu)
#define RT6190_RATIO_VOUT_POS                 (5u)
#define RT6190_RATIO_VOUT_MSK                 (0x20u)
#define RT6190_RATIO_VOUT_0_08V               (0u << RT6190_RATIO_VOUT_POS)
#define RT6190_RATIO_VOUT_0_05V               (1u << RT6190_RATIO_VOUT_POS)
#define RT6190_RATIO_VIN_POS                  (6u)
#define RT6190_RATIO_VIN_MSK                  (0x40u)
#define RT6190_RATIO_VIN_0_08V                (0u << RT6190_RATIO_VIN_POS)
#define RT6190_RATIO_VIN_0_05V                (1u << RT6190_RATIO_VIN_POS)
#define RT6190_RATIO_SSP_POS                  (7u)
#define RT6190_RATIO_SSP_MSK                  (0x80u)
#define RT6190_RATIO_SSP_DIS                  (0u << RT6190_RATIO_SSP_POS)
#define RT6190_RATIO_SSP_EN                   (1u << RT6190_RATIO_SSP_POS)

/* Lower 8 bits of 11-bit output voltage */
#define RT6190_OUTPUT_VOLTAGE_LSB_REG         (0x12u)
/* Upper 3 bits of 11-bit output voltage */
#define RT6190_OUTPUT_VOLTAGE_MSB_REG         (0x13u)

/* Lower 8 bits of 11-bit output average current */
#define RT6190_OUTPUT_CURRENT_LSB_REG         (0x14u)
/* Lower 3 bits of 11-bit output average current */
#define RT6190_OUTPUT_CURRENT_MSB_REG         (0x15u)

/* Upper 8-bits of 11-bit input voltage */
#define RT6190_INPUT_VOLTAGE_LSB_REG          (0x16u)
/* Upper 3-bits of 11-bit input voltage */
#define RT6190_INPUT_VOLTAGE_MSB_REG          (0x17u)

/* Lower 8 bits of 11-bit input average current */
#define RT6190_INPUT_CURRENT_LSB_REG          (0x18u)
/* Lower 3 bits of 11-bit input average current */
#define RT6190_INPUT_CURRENT_MSB_REG          (0x19u)

/* Lower 8 bits of 11-bit temperature */
#define RT6190_TEMPERATURE_LSB_REG            (0x1Au)
/* Lower 3 bits of 11-bit temperature */
#define RT6190_TEMPERATURE_MSB_REG            (0x1Bu)

/* UVP, OVP, and temperature fault status */
#define RT6190_STATUS1_REG                    (0x1Cu)
#define RT6190_STATUS1_EXT_OVP_A              (0b00000001u)
#define RT6190_STATUS1_EXT_UVP_A              (0b00000010u)
#define RT6190_STATUS1_EXT_OVP_C              (0b00000100u)
#define RT6190_STATUS1_EXT_UVP_C              (0b00001000u)
#define RT6190_STATUS1_INT_OVP                (0b00010000u)
#define RT6190_STATUS1_INT_UVP                (0b00100000u)
#define RT6190_STATUS1_OTP                    (0b01000000u)
#define RT6190_STATUS1_IN_OVP                 (0b10000000u)

/* OCP and Power good indicator */
#define RT6190_STATUS2_REG                    (0x1Du)
#define RT6190_STATUS2_OCP1                   (0b00000001u)
#define RT6190_STATUS2_OCP2                   (0b00000010u)
#define RT6190_STATUS2_OCP3                   (0b00000100u)
#define RT6190_STATUS2_OCP4                   (0b00001000u)
#define RT6190_STATUS2_CV_CC                  (0b00010000u)
#define RT6190_STATUS2_PG                     (0b01000000u)

/* OVP, UVP, and OTP alert flags */
#define RT6190_ALERT1_REG                     (0x1Eu)
#define RT6190_ALERT1_EXT_OVP_A               (0b00000001u)
#define RT6190_ALERT1_EXT_UVP_A               (0b00000010u)
#define RT6190_ALERT1_EXT_OVP_C               (0b00000100u)
#define RT6190_ALERT1_EXT_UVP_C               (0b00001000u)
#define RT6190_ALERT1_INT_OVP                 (0b00010000u)
#define RT6190_ALERT1_INT_UVP                 (0b00100000u)
#define RT6190_ALERT1_OTP                     (0b01000000u)
#define RT6190_ALERT1_IN_OVP                  (0b10000000u)

/* OCP, Watchdog timer, and timer1 alert flags */
#define RT6190_ALERT2_REG                     (0x1Fu)
#define RT6190_ALERT2_OCP1                    (0b00000001u)
#define RT6190_ALERT2_OCP2                    (0b00000010u)
#define RT6190_ALERT2_OCP3                    (0b00000100u)
#define RT6190_ALERT2_OCP4                    (0b00001000u)
#define RT6190_ALERT2_WDT                     (0b00010000u)
#define RT6190_ALERT2_TM1                     (0b00100000u)
#define RT6190_ALERT2_RAMP_PG                 (0b01000000u)
#define RT6190_ALERT2_OTP_R                   (0b10000000u)

/* Mask internal flag output of UVP, OVP, and OTP to ALERT pin */
#define RT6190_MASK1_REG                      (0x20u)
#define RT6190_MASK1_ALERT_EXT_OVP_A          (0b00000001u)
#define RT6190_MASK1_ALERT_EXT_UVP_A          (0b00000010u)
#define RT6190_MASK1_ALERT_EXT_OVP_C          (0b00000100u)
#define RT6190_MASK1_ALERT_EXT_UVP_C          (0b00001000u)
#define RT6190_MASK1_ALERT_INT_OVP            (0b00010000u)
#define RT6190_MASK1_ALERT_INT_UVP            (0b00100000u)
#define RT6190_MASK1_ALERT_OTP                (0b01000000u)
#define RT6190_MASK1_ALERT_IN_OVP             (0b10000000u)

/* Mask internal flag output of OCP, Watchdog timer, and timer1 to ALERT pin */
#define RT6190_MASK2_REG                      (0x21u)
#define RT6190_MASK2_ALERT_OCP1               (0b00000001u)
#define RT6190_MASK2_ALERT_OCP2               (0b00000010u)
#define RT6190_MASK2_ALERT_OCP3               (0b00000100u)
#define RT6190_MASK2_ALERT_OCP4               (0b00001000u)
#define RT6190_MASK2_ALERT_WDT                (0b00010000u)
#define RT6190_MASK2_ALERT_TM1                (0b00100000u)
#define RT6190_MASK2_ALERT_RAMP_PG            (0b01000000u)
#define RT6190_MASK2_ALERT_OTP_R              (0b10000000u)

/* OCP1 setting */
#define RT6190_OCP1_SETTING_REG               (0x22u)

/* OCP2 setting */
#define RT6190_OCP2_SETTING_REG               (0x23u)

/* OCP3 setting */
#define RT6190_OCP3_SETTING_REG               (0x24u)

/* OCP4 setting */
#define RT6190_OCP4_SETTING_REG               (0x25u)

/* OCP1 delay time */
#define RT6190_OCP1_DELAY_TIME_REG            (0x26u)
#define RT6190_OCP1_DELAY_TIME_TIMMING_POS    (0u)
#define RT6190_OCP1_DELAY_TIME_TIMMING_MSK    (0x3Fu)
#define RT6190_OCP1_DELAY_TIME_LSB_POS        (7u)
#define RT6190_OCP1_DELAY_TIME_LSB_MSK        (0x80u)
#define RT6190_OCP1_DELAY_TIME_LSB_8MS        (0u << RT6190_OCP1_DELAY_TIME_LSB_POS)
#define RT6190_OCP1_DELAY_TIME_LSB_32MS       (1u << RT6190_OCP1_DELAY_TIME_LSB_POS)

/* OCP2 delay time */
#define RT6190_OCP2_DELAY_TIME_REG            (0x27u)
#define RT6190_OCP2_DELAY_TIME_TIMMING_POS    (0u)
#define RT6190_OCP2_DELAY_TIME_TIMMING_MSK    (0x3Fu)
#define RT6190_OCP2_DELAY_TIME_LSB_POS        (7u)
#define RT6190_OCP2_DELAY_TIME_LSB_MSK        (0x80u)
#define RT6190_OCP2_DELAY_TIME_LSB_8MS        (0u << RT6190_OCP2_DELAY_TIME_LSB_POS)
#define RT6190_OCP2_DELAY_TIME_LSB_32MS       (1u << RT6190_OCP2_DELAY_TIME_LSB_POS)

/* OCP enable */
#define RT6190_OCP_ENABLE_REG                 (0x28u)
#define RT6190_OCP_ENABLE_OCP3_TIMING_POS     (0u)
#define RT6190_OCP_ENABLE_OCP3_TIMING_MSK     (0x03u)
#define RT6190_OCP_ENABLE_OCP3_TIMING_0MS     (0u << RT6190_OCP_ENABLE_OCP3_TIMING_POS)
#define RT6190_OCP_ENABLE_OCP3_TIMING_5MS     (1u << RT6190_OCP_ENABLE_OCP3_TIMING_POS)
#define RT6190_OCP_ENABLE_OCP3_TIMING_10MS    (2u << RT6190_OCP_ENABLE_OCP3_TIMING_POS)
#define RT6190_OCP_ENABLE_OCP3_TIMING_20MS    (3u << RT6190_OCP_ENABLE_OCP3_TIMING_POS)
#define RT6190_OCP_ENABLE_OCP4_TIMING_POS     (2u)
#define RT6190_OCP_ENABLE_OCP4_TIMING_MSK     (0xC0u)
#define RT6190_OCP_ENABLE_OCP4_TIMING_0MS     (0u << RT6190_OCP_ENABLE_OCP4_TIMING_POS)
#define RT6190_OCP_ENABLE_OCP4_TIMING_5MS     (1u << RT6190_OCP_ENABLE_OCP4_TIMING_POS)
#define RT6190_OCP_ENABLE_OCP4_TIMING_10MS    (2u << RT6190_OCP_ENABLE_OCP4_TIMING_POS)
#define RT6190_OCP_ENABLE_OCP4_TIMING_20MS    (3u << RT6190_OCP_ENABLE_OCP4_TIMING_POS)
#define RT6190_OCP_ENABLE_OCP1_POS            (4u)
#define RT6190_OCP_ENABLE_OCP1_MSK            (0x10u)
#define RT6190_OCP_ENABLE_OCP1_DIS            (0u << RT6190_OCP_ENABLE_OCP1_POS)
#define RT6190_OCP_ENABLE_OCP1_EN             (1u << RT6190_OCP_ENABLE_OCP1_POS)
#define RT6190_OCP_ENABLE_OCP2_POS            (5u)
#define RT6190_OCP_ENABLE_OCP2_MSK            (0x20u)
#define RT6190_OCP_ENABLE_OCP2_DIS            (0u << RT6190_OCP_ENABLE_OCP2_POS)
#define RT6190_OCP_ENABLE_OCP2_EN             (1u << RT6190_OCP_ENABLE_OCP2_POS)
#define RT6190_OCP_ENABLE_OCP3_POS            (6u)
#define RT6190_OCP_ENABLE_OCP3_MSK            (0x40u)
#define RT6190_OCP_ENABLE_OCP3_DIS            (0u << RT6190_OCP_ENABLE_OCP3_POS)
#define RT6190_OCP_ENABLE_OCP3_EN             (1u << RT6190_OCP_ENABLE_OCP3_POS)
#define RT6190_OCP_ENABLE_OCP4_POS            (7u)
#define RT6190_OCP_ENABLE_OCP4_MSK            (0x80u)
#define RT6190_OCP_ENABLE_OCP4_DIS            (0u << RT6190_OCP_ENABLE_OCP4_POS)
#define RT6190_OCP_ENABLE_OCP4_EN             (1u << RT6190_OCP_ENABLE_OCP4_POS)

/* Power path control pins and power path status setting */
#define RT6190_SET5_REG                       (0x29u)
#define RT6190_SET5_POWER_PATH_GA_POS         (0u)
#define RT6190_SET5_POWER_PATH_GA_MSK         (0x01u)
#define RT6190_SET5_POWER_PATH_GA_DIS         (0u << RT6190_SET5_POWER_PATH_GA_POS)
#define RT6190_SET5_POWER_PATH_GA_EN          (1u << RT6190_SET5_POWER_PATH_GA_POS)
#define RT6190_SET5_POWER_PATH_GC_POS         (1u)
#define RT6190_SET5_POWER_PATH_GC_MSK         (0x02u)
#define RT6190_SET5_POWER_PATH_GC_DIS         (0u << RT6190_SET5_POWER_PATH_GC_POS)
#define RT6190_SET5_POWER_PATH_GC_EN          (1u << RT6190_SET5_POWER_PATH_GC_POS)
#define RT6190_SET5_PATH_A_TYPE_POS           (2u)
#define RT6190_SET5_PATH_A_TYPE_MSK           (0x04u)
#define RT6190_SET5_PATH_A_TYPE_N_MOS         (0u << RT6190_SET5_PATH_A_TYPE_POS)
#define RT6190_SET5_PATH_A_TYPE_P_MOS         (1u << RT6190_SET5_PATH_A_TYPE_POS)
#define RT6190_SET5_PATH_C_TYPE_POS           (3u)
#define RT6190_SET5_PATH_C_TYPE_MSK           (0x80u)
#define RT6190_SET5_PATH_C_TYPE_N_MOS         (0u << RT6190_SET5_PATH_C_TYPE_POS)
#define RT6190_SET5_PATH_C_TYPE_P_MOS         (1u << RT6190_SET5_PATH_C_TYPE_POS)
#define RT6190_SET5_PATH_FLOATING_POS         (4u)
#define RT6190_SET5_PATH_FLOATING_MSK         (0x10u)
#define RT6190_SET5_PATH_KEEP_ORIGIANL        (0u << RT6190_SET5_PATH_FLOATING_POS)
#define RT6190_SET5_PATH_FLOATING_ALL         (1u << RT6190_SET5_PATH_FLOATING_POS)
#define RT6190_SET5_PROT_PATH_1_POS           (5u)
#define RT6190_SET5_PROT_PATH_1_MSK           (0x20u)
#define RT6190_SET5_PROT_PATH_1_TURN_OFF      (0u << RT6190_SET5_PROT_PATH_1_POS)
#define RT6190_SET5_PROT_PATH_1_KEEP_ORIGINAL (1u << RT6190_SET5_PROT_PATH_1_POS)
#define RT6190_SET5_PROT_PATH_A_POS           (6u)
#define RT6190_SET5_PROT_PATH_A_MSK           (0x40u)
#define RT6190_SET5_PROT_PATH_A_TURN_OFF      (0u << RT6190_SET5_PROT_PATH_A_POS)
#define RT6190_SET5_PROT_PATH_A_KEEP_ORIGINAL (1u << RT6190_SET5_PROT_PATH_A_POS)
#define RT6190_SET5_PROT_PATH_C_POS           (7u)
#define RT6190_SET5_PROT_PATH_C_MSK           (0x80u)
#define RT6190_SET5_PROT_PATH_C_TURN_OFF      (0u << RT6190_SET5_PROT_PATH_C_POS)
#define RT6190_SET5_PROT_PATH_C_KEEP_ORIGINAL (1u << RT6190_SET5_PROT_PATH_C_POS)

/* Enabling power path OVP/UVP */
#define RT6190_POWER_PATH_REG                 (0x2A)
#define RT6190_POWER_PATH_OVP_A_POS           (0u)
#define RT6190_POWER_PATH_OVP_A_MSK           (0x01u)
#define RT6190_POWER_PATH_OVP_A_EN            (0u << RT6190_POWER_PATH_OVP_A_POS)
#define RT6190_POWER_PATH_OVP_A_DIS           (1u << RT6190_POWER_PATH_OVP_A_POS)
#define RT6190_POWER_PATH_UVP_A_POS           (1u)
#define RT6190_POWER_PATH_UVP_A_MSK           (0x02u)
#define RT6190_POWER_PATH_UVP_A_EN            (0u << RT6190_POWER_PATH_UVP_A_POS)
#define RT6190_POWER_PATH_UVP_A_DIS           (1u << RT6190_POWER_PATH_UVP_A_POS)
#define RT6190_POWER_PATH_OVP_C_POS           (2u)
#define RT6190_POWER_PATH_OVP_C_MSK           (0x40u)
#define RT6190_POWER_PATH_OVP_C_EN            (0u << RT6190_POWER_PATH_OVP_C_POS)
#define RT6190_POWER_PATH_OVP_C_DIS           (1u << RT6190_POWER_PATH_OVP_C_POS)
#define RT6190_POWER_PATH_UVP_C_POS           (3u)
#define RT6190_POWER_PATH_UVP_C_MSK           (0x80u)
#define RT6190_POWER_PATH_UVP_C_EN            (0u << RT6190_POWER_PATH_UVP_C_POS)
#define RT6190_POWER_PATH_UVP_C_DIS           (1u << RT6190_POWER_PATH_UVP_C_POS)

/* OVP/UVP threshold and VBUSC alarm detection control */
#define RT6190_PPS_REG                        (0x2B)
#define RT6190_PPS_OVP_POS                    (4u)
#define RT6190_PPS_OVP_MSK                    (0x10u)
#define RT6190_PPS_OVP_LEVEL                  (0u << RT6190_PPS_OVP_POS)
#define RT6190_PPS_OVP_REF                    (1u << RT6190_PPS_OVP_POS)
#define RT6190_PPS_UVP_POS                    (5u)
#define RT6190_PPS_UVP_MSK                    (0x20u)
#define RT6190_PPS_UVP_LEVEL                  (0u << RT6190_PPS_UVP_POS)
#define RT6190_PPS_UVP_REF                    (1u << RT6190_PPS_UVP_POS)
#define RT6190_PPS_ALARM_HI_POS               (6u)
#define RT6190_PPS_ALARM_HI_MSK               (0x40u)
#define RT6190_PPS_ALARM_HI_EN                (0u << RT6190_PPS_ALARM_HI_POS)
#define RT6190_PPS_ALARM_HI_DIS               (1u << RT6190_PPS_ALARM_HI_POS)
#define RT6190_PPS_ALARM_LO_POS               (7u)
#define RT6190_PPS_ALARM_LO_MSK               (0x80u)
#define RT6190_PPS_ALARM_LO_EN                (0u << RT6190_PPS_ALARM_LO_POS)
#define RT6190_PPS_ALARM_LO_DIS               (1u << RT6190_PPS_ALARM_LO_POS)

/* Lower 8-bits of 11-bit VBUSC alarm high threshold */
#define RT6190_VBUSC_ALARM_HI_THLD_LSB_REG    (0x2Cu)
/* Upper 3-bits of 11-bit VBUSC alarm high threshold */
#define RT6190_VBUSC_ALARM_HI_THLD_MSB_REG    (0x2Du)

/* Lower 8-bits of 11-bit VBUSC alarm low threshold */
#define RT6190_VBUSC_ALARM_LO_THLD_LSB_REG    (0x2Eu)
/* Upper 3-bits of 11-bit VBUSC alarm low threshold */
#define RT6190_VBUSC_ALARM_LO_THLD_MSB_REG    (0x2Fu)

/* Watchdog and timer1 setting */
#define RT6190_WATCHDOG_REG                   (0x30u)
#define RT6190_WATCHDOG_SEL_POS               (0u)
#define RT6190_WATCHDOG_SEL_MSK               (0x07u)
#define RT6190_WATCHDOG_TIMER1_SEL_POS        (4u)
#define RT6190_WATCHDOG_TIMER1_SEL_MASK       (0x70u)

/* Enable ADC function for VBUSC voltage */
#define RT6190_VBUSC_ADC_REG                  (0x32u)
#define RT6190_VBUSC_ADC_POS                  (1u)
#define RT6190_VBUSC_ADC_MSK                  (0x02u)
#define RT6190_VBUSC_ADC_DIS                  (0u << RT6190_VBUSC_ADC_POS)
#define RT6190_VBUSC_ADC_EN                   (1u << RT6190_VBUSC_ADC_POS)

/* Lower 8-bits of 11-bit VBUSC voltage */
#define RT6190_VBUSC_VOL_LSB_REG              (0x33u)
/* Upper 3-bits of 11-bit VBUSC voltage */
#define RT6190_VBUSC_VOL_MSB_REG              (0x34u)

/* UVP reference setting */
#define RT6190_UVP_REF_REG                    (0x35u)

/* OVP reference setting */
#define RT6190_OVP_REF_REG                    (0x36u)

/* VBUSC alarm low, high, and input UVLO indicator */
#define RT6190_STATUS3_REG                    (0x37u)
#define RT6190_STATUS3_IN_UVLO_LO_2_7V        (0b00000001u)
#define RT6190_STATUS3_IN_UVLO_HI_3V          (0b00000011u)
#define RT6190_STATUS3_TO_275MS               (0b00000100u)
#define RT6190_STATUS3_ALARM_HI               (0b00001000u)
#define RT6190_STATUS3_ALARM_LO               (0b00010000u)

/* VBUSC alarm low, high, and input UVLO alert flags. */
#define RT6190_ALERT3_REG                     (0x38u)
#define RT6190_ALERT3_IN_UVLO_R               (0b00000001u)
#define RT6190_ALERT3_IN_UVLO_F               (0b00000010u)
#define RT6190_ALERT3_TO_275MS                (0b00000100u)
#define RT6190_ALERT3_ALARM_HI                (0b00001000u)
#define RT6190_ALERT3_ALARM_LO                (0b00010000u)

/* Mask internal flag output of VBUSC alarm low, high, and UVLO to alert pin. */
#define RT6190_MASK3_REG                      (0x39u)
#define RT6190_MASK3_ALERT_IN_UVLO_R          (0b00000001u)
#define RT6190_MASK3_ALERT_IN_UVLO_F          (0b00000010u)
#define RT6190_MASK3_ALERT_TO_275MS           (0b00000100u)
#define RT6190_MASK3_ALERT_ALARM_HI           (0b00001000u)
#define RT6190_MASK3_ALERT_ALARM_LO           (0b00010000u)

/**
* \addtogroup group_pmg_app_common_buck_boost
* \{
*/

/**
* \addtogroup group_pmg_app_common_buck_boost_data_structures
* \{
*/
/**
* RT6190 buck-boost regulator data structure
*/
typedef struct rt6190_context
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
} cy_stc_app_rt6190_context_t;

/** \} group_pmg_app_common_buck_boost_data_structures */

/**
* \addtogroup group_pmg_app_common_buck_boost_functions
* \{
*/
/**
 * @brief RT6190 power delivery controller initialize
 * I2C SCB block should be initialized and all the structure member of
 * cy_stc_app_rt6190_context_t should be initialized before calling this function.
 * @param context - Pointer to the RT6190 data structure context
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_RT6190_Init(cy_stc_app_rt6190_context_t *context);

/**
 * @brief Selects the output voltage
 * @param context - Pointer to the RT6190 data structure context.
 * @param vol_in_mv - Voltage unit in PDO (mV)
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_RT6190_SetVolt(cy_stc_app_rt6190_context_t *context, uint16_t vol_in_mv);

/**
 * @brief Returns the current output voltage
 * @param context - Pointer to the RT6190 data structure context.
 * @return Return voltage in mV on successful read else RT6190_VOLTAGE_INVALID.
 */
uint16_t Cy_App_RT6190_GetVolt(cy_stc_app_rt6190_context_t *context);

/**
 * @brief Enables RT6190
 * @param context - Pointer to the RT6190 data structure context
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_RT6190_Enable(cy_stc_app_rt6190_context_t *context);

/**
 * @brief Disables RT6190
 * @param context - Pointer to the RT6190 data structure context
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_RT6190_Disable(cy_stc_app_rt6190_context_t *context);

/**
 * @brief Sets the current limit value
 * @param context - Pointer to the RT6190 data structure context
 * @param cur_in_10ma - current limit value (10 mA)
 * @return Return true if operation is success; false otherwise.
 */
bool Cy_App_RT6190_SetCurrentLimit(cy_stc_app_rt6190_context_t *context, uint16_t cur_in_10ma);

/**
 * @brief Returns the current limit value
 * @param context - Pointer to the RT6190 data structure context
 * @return Return current limit value in mA on successful read else RT6190_CURRENT_INVALID.
 */
uint16_t Cy_App_RT6190_GetCurrentLimit(cy_stc_app_rt6190_context_t *context);

/**
 * @brief Sets the rt6190 operating mode. As a mode parameter use
 * RT6190_SET1_F_CCM_LIGHT_LOAD_PSM or RT6190_SET1_F_CCM_FORCE_CCM macros.
 *
 * @param context - Pointer to the RT6190 data structure context
 * @param mode - operating mode
 * @return Return true if operation is success; false otherwise
 */
bool Cy_App_RT6190_SetMode(cy_stc_app_rt6190_context_t *context, uint8_t mode);

/**
 * @brief Returns the current operating mode
 * @param context - Pointer to the RT6190 data structure context
 * @return Return current operating mode on successful read else RT6190_MODE_INVALID
 */
uint8_t Cy_App_RT6190_GetMode(cy_stc_app_rt6190_context_t *context);

/**
 * @brief Returns the status of different fault events
 * @param context - Pointer to the RT6190 data structure context
 * @return Return current fault status on successful read else RT6190_ERR_FLAG_INVALID
 */
uint16_t Cy_App_RT6190_GetErrorStatus(cy_stc_app_rt6190_context_t *context);

/** \} group_pmg_app_common_buck_boost_functions */
/** \} group_pmg_app_common_buck_boost*/

#endif /* _CY_APP_RT6190_H_ */

/* [] End of file */
