/***************************************************************************//**
* \file cy_app_config.h
* \version 2.0
*
* \brief
* Defines the pmg-app-common configurations
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_CONFIG_H_
#define _CY_APP_CONFIG_H_

#include "cy_scb_i2c.h"

/**
* \addtogroup group_pmg_app_common_app
* \{
*/

/**
* \addtogroup group_pmg_app_common_app_features
* \{
*/

#ifndef CY_APP_ROLE_PREFERENCE_ENABLE
/** Power and Data Role preference control enable. If this option is disabled, 
 * then CY_APP_POWER_ROLE_PREFERENCE_ENABLE should also be disabled.
 */
#define CY_APP_ROLE_PREFERENCE_ENABLE                           (1u)
#endif /* CY_APP_ROLE_PREFERENCE_ENABLE */

#ifndef CY_APP_POWER_ROLE_PREFERENCE_ENABLE
/** Power role preference enable. */
#define CY_APP_POWER_ROLE_PREFERENCE_ENABLE                     (1u)
#endif /* CY_APP_POWER_ROLE_PREFERENCE_ENABLE */

/** @cond DOXYGEN_HIDE */
/*******************************************************************************
 * Enable PD spec rev 3 support
 ******************************************************************************/
#if CY_PD_REV3_ENABLE
#ifndef CY_PD_FRS_RX_ENABLE
#define CY_PD_FRS_RX_ENABLE                                     (0u)
#endif /* CY_PD_FRS_RX_ENABLE */

#ifndef CY_PD_FRS_TX_ENABLE
#define CY_PD_FRS_TX_ENABLE                                     (0u)
#endif /* CY_PD_FRS_TX_ENABLE */
#endif /* CY_PD_REV3_ENABLE */
/** @endcond */

#ifndef CY_APP_PD_PDO_SEL_ALGO
/** PDO selection algorithm:
 * 0: Pick the source PDO which delivers maximum amount of power
 * 1: Pick the fixed source PDO which delivers maximum amount of power
 * 2: Pick the fixed source PDO which delivers the maximum current
 * 3: Pick the fixed source PDO which delivers power at maximum voltage */
#define CY_APP_PD_PDO_SEL_ALGO                                  (0u)
#endif /* CY_APP_PD_PDO_SEL_ALGO */

/** @cond DOXYGEN_HIDE */
#define CY_APP_REGULATOR_REQUIRE_STABLE_ON_TIME                 (0)
#define REGULATOR_ENABLE(port)                                  false
#define REGULATOR_DISABLE(port)                                 false
#define REGULATOR_STATUS(port)                                  false
#define REGULATOR_TURN_ON_DELAY                                 (50)    

#ifndef CCG_SRC_FET
#define CCG_SRC_FET                                             (0u)
#endif /* CCG_SRC_FET */

/** VBUS PGDO FET control selection based on the FET control pin used in the
 * system hardware */
#if defined(CY_DEVICE_CCG3PA)
#define CY_APP_VBUS_FET_CTRL_0                                  (1u)
#define CY_APP_VBUS_FET_CTRL_1                                  (0u)

#ifndef CY_APP_VBUS_P_FET_CTRL
#define CY_APP_VBUS_P_FET_CTRL                                  (CY_APP_VBUS_FET_CTRL_0)
#endif /* CY_APP_VBUS_P_FET_CTRL */

#ifndef CY_APP_VBUS_C_FET_CTRL
#define CY_APP_VBUS_C_FET_CTRL                                  (CY_APP_VBUS_FET_CTRL_1)
#endif /* CY_APP_VBUS_C_FET_CTRL */
#endif /* defined(CY_DEVICE_CCG3PA) */
/** @endcond */

#ifndef CY_APP_USB_ENABLE
/** Enable/disable the USB feature. This macro should be set to '1' for solutions that need
 * the USB (Vendor, Billboard and HID) feature support. If this option is disabled, then
 * CCG_BB_ENABLE, CY_APP_USB_HID_INTF_ENABLE and any other macro for enabling USB interfaces should be disabled. 
 * CCG_BB_ENABLE should not be disabled if the alternate mode support is needed in the Dock solution. */
#define CY_APP_USB_ENABLE                                       (0u)
#endif /* CY_APP_USB_ENABLE */

#ifndef CY_APP_PD_USB4_SUPPORT_ENABLE
/** Enable/disable the USB4 support feature at the application level */
#define CY_APP_PD_USB4_SUPPORT_ENABLE                           (0u)
#endif /* CY_APP_PD_USB4_SUPPORT_ENABLE */

#ifndef CY_APP_SINK_FET_CTRL_GPIO_EN
/** This macro should be set to '1' if consumer path is controlled by GPIO */
#define CY_APP_SINK_FET_CTRL_GPIO_EN                            (0u)
#endif /* CY_APP_SINK_FET_CTRL_GPIO_EN */

/*******************************************************************************
 * VBUS monitor configuration
 ******************************************************************************/
#ifndef CY_APP_VBUS_TURN_ON_MARGIN
/** Allowed VBUS valid margin as percentage of expected voltage */
#define CY_APP_VBUS_TURN_ON_MARGIN                              (-20)
#endif /* CY_APP_VBUS_TURN_ON_MARGIN */

#ifndef CY_APP_VBUS_DISCHARGE_MARGIN
/** Allowed margin over expected voltage (as percentage) for negative VBUS
 * voltage transitions */
#define CY_APP_VBUS_DISCHARGE_MARGIN                            (10)
#endif /* CY_APP_VBUS_DISCHARGE_MARGIN */

#ifndef CY_APP_VBUS_DISCHARGE_TO_5V_MARGIN
/** Allowed margin over 5 V before the provider FET is turned OFF when
 * discharging to VSAFE0 */
#define CY_APP_VBUS_DISCHARGE_TO_5V_MARGIN                      (10)
#endif /* CY_APP_VBUS_DISCHARGE_TO_5V_MARGIN */

#ifndef CY_APP_VBUS_NEW_VALID_MARGIN
/** Allowed margin over expected voltage (as percentage) */
#define CY_APP_VBUS_NEW_VALID_MARGIN                            (5)
#endif /* CY_APP_VBUS_NEW_VALID_MARGIN */

#ifndef VBUS_SOFT_START_ENABLE
/** Set to '1' to enable VBUS soft start feature */
#define VBUS_SOFT_START_ENABLE                                  (0u)
#endif /* VBUS_SOFT_START_ENABLE */

/** Enable to set VBUS_MAX_VOLTAGE to 50V for EPR voltages. */
#ifndef CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE
#define CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE                      (0u)
#endif /* CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE */

/*******************************************************************************
 * Firmware feature configuration
 ******************************************************************************/

#ifndef CY_APP_WATCHDOG_HARDWARE_RESET_ENABLE
/** Enable watchdog hardware reset for CPU lock-up recovery */
#define CY_APP_WATCHDOG_HARDWARE_RESET_ENABLE                   (1u)
#endif /* CY_APP_WATCHDOG_HARDWARE_RESET_ENABLE */

#ifndef CY_APP_RESET_ON_ERROR_ENABLE
/** Enable PMG device reset on error (watchdog expiry or hard fault) */
#define CY_APP_RESET_ON_ERROR_ENABLE                            (1u)
#endif /* CY_APP_RESET_ON_ERROR_ENABLE */

#ifndef CY_APP_WATCHDOG_RESET_PERIOD_MS
/** Watchdog reset period in milliseconds.
 * A periodic timer (CY_PDUTILS_WATCHDOG_TIMER) is run using this as a period.
 * This timer resets the device if the main loop has not been run as expected
 * for three consecutive times.
 */
#define CY_APP_WATCHDOG_RESET_PERIOD_MS                         (750u)
#endif /* CY_APP_WATCHDOG_RESET_PERIOD_MS */

#ifndef CY_APP_STACK_USAGE_CHECK_ENABLE
/** Enable tracking of maximum stack usage */
#define CY_APP_STACK_USAGE_CHECK_ENABLE                         (0u)
#endif /* CY_APP_STACK_USAGE_CHECK_ENABLE */

#ifndef CY_CORROSION_MITIGATION_ENABLE
/** Set this to 1 to enable corrosion mitigation feature
 *  By default, SBU lines are used for moisture detection.
 *  Set CY_APP_MOISTURE_DETECT_USING_DP_DM to 1 use DP/DM instead of
 *  SBU lines. */
#define CY_CORROSION_MITIGATION_ENABLE                          (0u)
#endif /* CY_CORROSION_MITIGATION_ENABLE */

#ifndef CY_APP_MOISTURE_DETECT_IN_ATTACH_ENABLE
/** Set this to 1 to enable moisture detection in attach wait state. */
#define CY_APP_MOISTURE_DETECT_IN_ATTACH_ENABLE                 (0u)
#endif /* CY_APP_MOISTURE_DETECT_IN_ATTACH_ENABLE */

#ifndef CY_APP_MOISTURE_DETECT_USING_DP_DM
/** Set this to 1 to use DP/DM lines for moisture detection. */
#define CY_APP_MOISTURE_DETECT_USING_DP_DM                      (0u)
#endif /* CY_APP_MOISTURE_DETECT_USING_DP_DM */

/*******************************************************************************
 * Battery Charging configuration
 ******************************************************************************/
#if CCG_TYPE_A_PORT_ENABLE
/** TYPE-A port ID. */
#define TYPE_A_PORT_ID                                          (1u)
#endif /* CCG_TYPE_A_PORT_ENABLE */
#ifndef APPLE_SOURCE_DISABLE
/** Disable Apple charging as source */
#define APPLE_SOURCE_DISABLE                                    (1u)
#endif /* APPLE_SOURCE_DISABLE */

#ifndef APPLE_SINK_DISABLE
/** Disable Apple charging as sink */
#define APPLE_SINK_DISABLE                                      (1u)
#endif /* APPLE_SINK_DISABLE */

#ifndef LEGACY_APPLE_SRC_SLN_TERM_ENABLE
/** Enable solution terminations for Apple charging */
#define LEGACY_APPLE_SRC_SLN_TERM_ENABLE                        (0u)
#endif /* LEGACY_APPLE_SRC_SLN_TERM_ENABLE */

#ifndef APPLE_SRC_EXT_TERM_ENABLE
/** Enable external terminations through GPIO for Apple charging.
 *  LEGACY_APPLE_SRC_SLN_TERM_ENABLE should be enabled along with this.
 */
#define APPLE_SRC_EXT_TERM_ENABLE                               (0u)
#endif /* APPLE_SRC_EXT_TERM_ENABLE */

/******************************** other *********************************/
/** @cond DOXYGEN_HIDE */
#define CySoftwareReset()                                       NVIC_SystemReset()

#ifndef CY_APP_HPI_I2C_HW
#define CY_APP_HPI_I2C_HW                                       SCB5
#endif /* CY_APP_HPI_I2C_HW */
/** @endcond */

#ifndef CY_APP_FLASH_LOG_ROW_NUM
/** Flash address row number where the logs are stored */
#define CY_APP_FLASH_LOG_ROW_NUM                                (0x3F5)
#endif /* CY_APP_FLASH_LOG_ROW_NUM */

#ifndef CY_APP_FLASH_LOG_BACKUP_ROW_NUM
/** Flash address row number where a redundant copy of the logs are stored */
#define CY_APP_FLASH_LOG_BACKUP_ROW_NUM                         (0x3F7)
#endif /* CY_APP_FLASH_LOG_BACKUP_ROW_NUM */

#ifndef CY_APP_FW1_CONFTABLE_MAX_ADDR 
/** Flash address within which the FW1's configuration table address is 
 * located */
#define CY_APP_FW1_CONFTABLE_MAX_ADDR                           (0x8000)
#endif /* CY_APP_FW1_CONFTABLE_MAX_ADDR  */

#ifndef CY_APP_BOOT_LOADER_LAST_ROW
/** Last flash row number of the bootloader image space */
#define CY_APP_BOOT_LOADER_LAST_ROW                             (0x0F)
#endif /* CY_APP_BOOT_LOADER_LAST_ROW */

#ifndef CY_APP_IMG1_LAST_FLASH_ROW_NUM
/** Last flash row number of FW1 image  */
#define CY_APP_IMG1_LAST_FLASH_ROW_NUM                          (0x010F)
#endif /* CY_APP_IMG1_LAST_FLASH_ROW_NUM */

#ifndef CY_APP_IMG2_LAST_FLASH_ROW_NUM
/** Last flash row number of FW2 image  */
#define CY_APP_IMG2_LAST_FLASH_ROW_NUM                          (0x3EF)
#endif /* CY_APP_IMG2_LAST_FLASH_ROW_NUM */

#ifndef CY_APP_UART_DEBUG_ENABLE
/** Enable/disable UART-based debug logging */
#define CY_APP_UART_DEBUG_ENABLE                                (0u)
#endif /* CY_APP_UART_DEBUG_ENABLE */

#ifndef CY_APP_FLASH_LOG_ENABLE
/** Enable/disable flash logging */
#define CY_APP_FLASH_LOG_ENABLE                                 (0u)
#endif /* CY_APP_FLASH_LOG_ENABLE */

/** @cond DOXYGEN_HIDE */
#ifndef CY_APP_DEBUG_VENDOR_CMD_ENABLE
#define CY_APP_DEBUG_VENDOR_CMD_ENABLE                          (0u)
#endif /* CY_APP_DEBUG_VENDOR_CMD_ENABLE */
/** @endcond */

#ifndef CCG_CBL_DISC_DISABLE
/** Macro to enable/disable the cable discovery from the application */
#define CCG_CBL_DISC_DISABLE                                    (0u)
#endif /* CCG_CBL_DISC_DISABLE */

#ifndef CY_APP_SNK_STANDBY_FET_SHUTDOWN_ENABLE
/** Set to '1' to shutdown the SNK FET in the application layer */
#define CY_APP_SNK_STANDBY_FET_SHUTDOWN_ENABLE                  (0u)
#endif /* CY_APP_SNK_STANDBY_FET_SHUTDOWN_ENABLE */

#ifndef CY_APP_HOST_ALERT_MSG_DISABLE
/** Set to '0' to handle the alert message */
#define CY_APP_HOST_ALERT_MSG_DISABLE                           (0u)
#endif /* CY_APP_HOST_ALERT_MSG_DISABLE */

#ifndef CY_APP_GET_REVISION_ENABLE
/** Set to '1' to initiate the get revision PD message by the device to determine
 * the PD revision used by the port partner. */
#define CY_APP_GET_REVISION_ENABLE                              (0u)
#endif /* CY_APP_GET_REVISION_ENABLE */

/** @cond DOXYGEN_HIDE */
#if CY_USE_CONFIG_TABLE
#ifndef CY_CONFIG_TABLE_TYPE
#define CY_CONFIG_TABLE_TYPE                                    CY_CONFIG_TABLE_DOCK
#endif /* CY_CONFIG_TABLE_TYPE */
#endif /* CY_USE_CONFIG_TABLE */
/** @endcond */

#ifndef VPRO_WITH_USB4_MODE
/** Set to '1' to enable VPRO support with USB4 mode */
#define VPRO_WITH_USB4_MODE                                     (0u)
#endif /* VPRO_WITH_USB4_MODE */

/** @cond DOXYGEN_HIDE */
#ifndef MUX_DELAY_EN
#define MUX_DELAY_EN                                            (0u)
#endif /* MUX_DELAY_EN */
/** @endcond */

#ifndef CY_APP_SMART_POWER_ENABLE
/** Enable/disable Smart Power algorithm */
#define CY_APP_SMART_POWER_ENABLE                               (0u)
#endif /* CY_APP_SMART_POWER_ENABLE */

#if CY_APP_DMC_ENABLE

#ifndef CY_APP_DEBUG_PULLUP_ON_UART
/** Set to '1' if there is PULLUP present on UART TX line */
#define CY_APP_DEBUG_PULLUP_ON_UART                             (0u)
#endif /* CY_APP_DEBUG_PULLUP_ON_UART */

#ifndef CY_APP_LED_CONTROL_ENABLE
/** Set to '1' to enable the LED control feature */
#define CY_APP_LED_CONTROL_ENABLE                               (0u)
#endif /* CY_APP_LED_CONTROL_ENABLE */

#ifndef CY_APP_DMC_PHASE1_UPDATE_ENABLE
/** Set to '1' to enable the phase 1 firmware update */
#define CY_APP_DMC_PHASE1_UPDATE_ENABLE                         (0u)
#endif /* CY_APP_DMC_PHASE1_UPDATE_ENABLE */

#ifndef CY_APP_SIGNED_FW_UPDATE_ENABLE
/** Set to '1' to enable signed firmware update in DMC */
#define CY_APP_SIGNED_FW_UPDATE_ENABLE                          (0u)
#endif /* CY_APP_SIGNED_FW_UPDATE_ENABLE */

#ifndef CY_APP_PSEUDO_METADATA_DISABLE
/** Set to '1' to disable the firmware pseudo metadata table */
#define CY_APP_PSEUDO_METADATA_DISABLE                          (1u)
#endif /* CY_APP_PSEUDO_METADATA_DISABLE */

#ifndef CY_APP_PRIORITIZE_FW2
/** Prioritize the primary firmware whenever the firmware update is done */
#define CY_APP_PRIORITIZE_FW2                                   (1u)
#endif /* CY_APP_PRIORITIZE_FW2 */

#ifndef CY_APP_PRIORITIZE_FW1
/** Prioritize the backup firmware whenever the firmware update is done */
#define CY_APP_PRIORITIZE_FW1                                   (0u)
#endif /* CY_APP_PRIORITIZE_FW1 */

#ifndef CY_APP_FIRMWARE_APP_ONLY
/** Set to '1' if building a debug enabled binary with no bootloader dependency */
#define CY_APP_FIRMWARE_APP_ONLY                                (0u)
#endif /*CY_APP_FIRMWARE_APP_ONLY */

#ifndef CY_APP_PRIORITY_FEATURE_ENABLE
/** Set to '1' to enable the application priority selection between primary 
 *  and the backup firmware */
#define CY_APP_PRIORITY_FEATURE_ENABLE                          (0u)
#endif /* CY_APP_PRIORITY_FEATURE_ENABLE */

#ifndef CY_APP_BOOT_WAIT_WINDOW_DISABLE
/** Set to '1' to enable the boot-wait delay before application start */
#define CY_APP_BOOT_WAIT_WINDOW_DISABLE                         (0u)
#endif /* CY_APP_BOOT_WAIT_WINDOW_DISABLE */

#ifndef CY_APP_FLASH_ENABLE_NB_MODE
/** Set to '1' to enable the flash operation in non-blocking mode */
#define CY_APP_FLASH_ENABLE_NB_MODE                             (0u)
#endif /* CY_APP_FLASH_ENABLE_NB_MODE */

#ifndef CY_APP_DUALAPP_DISABLE
/** Macro enable/disable the dual application architecture */
#define CY_APP_DUALAPP_DISABLE                                  (0u)
#endif /* CY_APP_DUALAPP_DISABLE */

#ifndef CY_APP_BOOT_ENABLE
/** Set to '1' to enable the boot features */
#define CY_APP_BOOT_ENABLE                                      (0u)
#endif /* CY_APP_BOOT_ENABLE */

#ifndef GATKEX_CREEK
/** Set '1' to enable Gatkex Creek related features */
#define GATKEX_CREEK                                            (0u)
#endif /* GATKEX_CREEK */

#endif /* CY_APP_DMC_ENABLE */

#ifndef CY_APP_RTOS_ENABLED
/** Set '1' to enable the RTOS support. */
#define CY_APP_RTOS_ENABLED                                        (0u)
#endif /* CY-APP_RTOS_ENABLED */

/** \} group_pmg_app_common_app_features */
/** \} group_pmg_app_common_app */

#endif /* _CY_APP_CONFIG_H_ */

/* [] End of file */
