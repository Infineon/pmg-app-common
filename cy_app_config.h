/***************************************************************************//**
* \file cy_app_config.h
* \version 3.0
*
* \brief
* Defines the pmg-app-common configurations
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

#ifndef CY_APP_CONFIG_H
#define CY_APP_CONFIG_H

#include "cy_scb_i2c.h"

/**
* \addtogroup group_pmg_app_common_app
* \{
*/

/**
* \addtogroup group_pmg_app_common_app_features
* \{
*/

#ifndef DOXYGEN
/** Expose conditionally compiled API to be discoverable by Doxygen.
 *  Doxygen defines this macro when analyses sources.
 */
#define DOXYGEN                                                 (0u)
#endif /* DOXYGEN */

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

#ifndef CY_APP_EXTD_MSG_HANDLER_ENABLE
#define CY_APP_EXTD_MSG_HANDLER_ENABLE                         (0u)
#endif /* CY_APP_EXTD_MSG_HANDLER_ENABLE */

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

#ifndef CCG_BB_ENABLE
/** Enable/disable the Billboard USB feature at the application level. 
 * If the USB feature is disabled CCG_BB_ENABLE shall be also disabled. */
#define CCG_BB_ENABLE                                           (0u)
#endif /* CCG_BB_ENABLE */

#ifndef CY_APP_USB_HID_INTF_ENABLE
/** Enable/disable the HID USB feature at the application level.
 * If the USB feature is disabled CY_APP_USB_HID_INTF_ENABLE shall be also disabled. */
#define CY_APP_USB_HID_INTF_ENABLE                              (0u)
#endif /* CY_APP_USB_HID_INTF_ENABLE */

#ifndef CY_APP_PD_USB4_SUPPORT_ENABLE
/** Enable/disable the USB4 support feature at the application level */
#define CY_APP_PD_USB4_SUPPORT_ENABLE                           (0u)
#endif /* CY_APP_PD_USB4_SUPPORT_ENABLE */

#ifndef CY_APP_SINK_FET_CTRL_GPIO_EN
/** This macro should be set to '1' if consumer path is controlled by GPIO */
#define CY_APP_SINK_FET_CTRL_GPIO_EN                            (0u)
#endif /* CY_APP_SINK_FET_CTRL_GPIO_EN */

/** Number of swap attempts to be made when port partner is sending a WAIT response. */
#ifndef CY_APP_MAX_SWAP_ATTEMPT_COUNT
#define CY_APP_MAX_SWAP_ATTEMPT_COUNT                           (10u)
#endif /* CY_APP_MAX_SWAP_ATTEMPT_COUNT */

/** Override value for the maximum current in RDO. */
#ifndef RDO_MAXCUR_OVERRIDE
#define RDO_MAXCUR_OVERRIDE                                     (0u)
#endif /* RDO_MAXCUR_OVERRIDE */

/** Enables or disables bi-directional buck-boost opertaions with external switches in PMG1B1. */
#ifndef PMG1B1_BI_DIRECTION
#define PMG1B1_BI_DIRECTION                                     (0u)
#endif /* PMG1B1_BI_DIRECTION */

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

#ifndef VBUS_OPP_ENABLE
/** Set to '1' to enable over power protection */
#define VBUS_OPP_ENABLE                                         (0u)
#endif /* VBUS_OPP_ENABLE */

#ifndef CY_APP_USE_EXTERNAL_CURRENT_MON
/** Set to '1' to use external current monitor, to detect OCP on Type-C VBUS */
#define CY_APP_USE_EXTERNAL_CURRENT_MON                         (0u)
#endif /* CY_APP_USE_EXTERNAL_CURRENT_MON */

#ifndef CY_APP_POWER_ON_UPDATE_SUPPORT
/** Set to '1' to keep track of power on firmware update status */
#define CY_APP_POWER_ON_UPDATE_SUPPORT                          (0u)
#endif /* CY_APP_POWER_ON_UPDATE_SUPPORT */

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

#ifndef CY_APP_BB_DISABLE_SUPPORT
/** Set this to 1 to support turn-off billboard interface. */
#define CY_APP_BB_DISABLE_SUPPORT                               (0u)
#endif /* CY_APP_BB_DISABLE_SUPPORT */

#ifndef CCG_SROM_CODE_ENABLE
/** Set this to 1 to support CCG ROM code. */
#define CCG_SROM_CODE_ENABLE                                    (0u)
#endif /* CCG_SROM_CODE_ENABLE */

#ifndef CY_APP_SILICON_ID
/** Set this to 1 to support reading of Silicon ID and Silicon revision. */
#define CY_APP_SILICON_ID                                       (0u)
#endif /* CY_APP_SILICON_ID */

/*******************************************************************************
 * Battery Charging configuration
 ******************************************************************************/
#ifndef CCG_TYPE_A_PORT_ENABLE
/** Set this to 1 to enable legacy charging using type A port */
#define CCG_TYPE_A_PORT_ENABLE                                  (0u)
#endif /* CCG_TYPE_A_PORT_ENABLE */

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

#ifndef CY_APP_RIDGE_SLAVE_I2C_HW
#define CY_APP_RIDGE_SLAVE_I2C_HW                               SCB1
#endif /* CY_APP_RIDGE_SLAVE_I2C_HW */

#ifndef CY_APP_HPI_I2C_HW
#define CY_APP_HPI_I2C_HW                                       SCB5
#endif /* CY_APP_HPI_I2C_HW */

#ifndef CY_APP_DEBUG_UART_TX_PORT
#define CY_APP_DEBUG_UART_TX_PORT                               GPIO_PRT5
#endif /* CY_APP_DEBUG_UART_TX_PORT */

#ifndef CY_APP_DEBUG_UART_TX_PIN
#define CY_APP_DEBUG_UART_TX_PIN                                1U
#endif /* CY_APP_DEBUG_UART_TX_PIN */

#ifndef CY_APP_DEBUG_UART_TX_HSIOM
#define CY_APP_DEBUG_UART_TX_HSIOM                              P5_1_SCB2_UART_TX
#endif /* CY_APP_DEBUG_UART_TX_HSIOM */

/** @endcond */

#ifndef CY_APP_FLASH_LOG_ROW_NUM
/** Flash address row number where the logs are stored */
#define CY_APP_FLASH_LOG_ROW_NUM                                (0x3F5u)
#endif /* CY_APP_FLASH_LOG_ROW_NUM */

#ifndef CY_APP_FLASH_LOG_BACKUP_ROW_NUM
/** Flash address row number where a redundant copy of the logs are stored */
#define CY_APP_FLASH_LOG_BACKUP_ROW_NUM                         (0x3F7u)
#endif /* CY_APP_FLASH_LOG_BACKUP_ROW_NUM */

#ifndef CY_APP_FW1_CONFTABLE_MAX_ADDR 
/** Flash address within which the FW1's configuration table address is 
 * located */
#define CY_APP_FW1_CONFTABLE_MAX_ADDR                           (0x8000u)
#endif /* CY_APP_FW1_CONFTABLE_MAX_ADDR  */

#ifndef CY_APP_BOOT_LOADER_LAST_ROW
/** Last flash row number of the bootloader image space */
#define CY_APP_BOOT_LOADER_LAST_ROW                             (0x0Fu)
#endif /* CY_APP_BOOT_LOADER_LAST_ROW */

#ifndef CY_APP_IMG1_LAST_FLASH_ROW_NUM
/** Last flash row number of FW1 image  */
#define CY_APP_IMG1_LAST_FLASH_ROW_NUM                          (0x010Fu)
#endif /* CY_APP_IMG1_LAST_FLASH_ROW_NUM */

#ifndef CY_APP_IMG2_LAST_FLASH_ROW_NUM
/** Last flash row number of FW2 image  */
#define CY_APP_IMG2_LAST_FLASH_ROW_NUM                          (0x3EFu)
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

#ifndef CY_APP_I2CM_BRIDGE_ENABLE
/** Enable/disable I2C-Master Bridge */
#define CY_APP_I2CM_BRIDGE_ENABLE                               (0u)
#endif /* CY_APP_I2CM_BRIDGE_ENABLE */

#ifndef CY_APP_FLASHING_MODE_USB_ENABLE
/** Enable/disable USB flashing mode */
#define CY_APP_FLASHING_MODE_USB_ENABLE                         (0u)
#endif /* CY_APP_FLASHING_MODE_USB_ENABLE */

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

#ifndef CY_PD_BAT_STATUS_HANDLER_ENABLE
/** Set to '1' to enable the battery status handler */
#define CY_PD_BAT_STATUS_HANDLER_ENABLE                         (0u)
#endif /* CY_PD_BAT_STATUS_HANDLER_ENABLE */

#ifndef CY_PD_BAT_CAPS_HANDLER_ENABLE
/** Set to '1' to enable the battery capabilities handler */
#define CY_PD_BAT_CAPS_HANDLER_ENABLE                           (0u)
#endif /* CY_PD_BAT_CAPS_HANDLER_ENABLE */

#ifndef CY_APP_RW_PD_RESP_MSG_DATA
/** Set to '1' to enable the read/write PD response message data */
#define CY_APP_RW_PD_RESP_MSG_DATA                              (0u)
#endif /* CY_APP_RW_PD_RESP_MSG_DATA */

#ifndef CY_APP_BOOT_ENABLE
/** Set to '1' to enable the boot features */
#define CY_APP_BOOT_ENABLE                                      (0u)
#endif /* CY_APP_BOOT_ENABLE */

#ifndef GATKEX_CREEK
/** Set '1' to enable Gatkex Creek related features */
#define GATKEX_CREEK                                            (0u)
#endif /* GATKEX_CREEK */

#endif /* CY_APP_DMC_ENABLE */

#ifndef CY_APP_DUALAPP_DISABLE
/** Macro enable/disable the dual application architecture */
#define CY_APP_DUALAPP_DISABLE                                  (0u)
#endif /* CY_APP_DUALAPP_DISABLE */

#ifndef CY_APP_RTOS_ENABLED
/** Set '1' to enable the RTOS support. */
#define CY_APP_RTOS_ENABLED                                     (0u)
#endif /* CY-APP_RTOS_ENABLED */

#if CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE

#ifndef EPR_PERCENTAGE
/** Set '100' to calculate percentage */
#define EPR_PERCENTAGE                                          (100u)
#endif /* EPR_PERCENTAGE */

#ifndef EPR_MAX_VOLTAGE_DIV_RATIO
/** Set '50' for VBUS divider ration for EPR high voltage */
#define EPR_MAX_VOLTAGE_DIV_RATIO                               (50u)
#endif /* EPR_PERCENTAGE */

#ifndef EPR_MAX_VOLTAGE_FET_VGS_MIN_THRESHOLD_IN_MILLI_VOLT
/** Set '1.1V' as the minimum VGS threshold for the FET */
#define EPR_MAX_VOLTAGE_FET_VGS_MIN_THRESHOLD_IN_MILLI_VOLT     (1100u)
#endif /* EPR_MAX_VOLTAGE_FET_VGS_MIN_THRESHOLD_IN_MILLI_VOLT */

#ifndef EPR_MAX_VBUS_DIV_OFF_MARGIN_IN_MILLI
/** Set '500' mV offset to check if VBUS is above requested voltage */
#define EPR_MAX_VBUS_DIV_OFF_MARGIN_IN_MILLI                    (500u)
#endif /* EPR_MAX_VBUS_DIV_OFF_MARGIN_IN_MILLI */
#endif /* CY_APP_VBUS_EPR_MAX_VOLTAGE_ENABLE */
/** \} group_pmg_app_common_app_features */
/** \} group_pmg_app_common_app */

#endif /* CY_APP_CONFIG_H */

/* [] End of file */
