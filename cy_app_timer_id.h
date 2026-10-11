/***************************************************************************//**
* \file cy_app_timer_id.h
* \version 3.0
*
* \brief
* Provides application software timer identifier definitions
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

#ifndef CY_APP_TIMER_ID_H
#define CY_APP_TIMER_ID_H

#include <stdint.h>

#define CY_APP_GET_TIMER_ID(context, id)                                 \
    (uint16_t)(((context)->port != 0U) ? ((((uint16_t)id) & 0x00FFU) + (uint16_t)CY_PDUTILS_TIMER_APP_PORT1_START_ID) : (uint16_t)(id))

/**
* \addtogroup group_pmg_app_common_app_enums
* \{
*/
/**
 * @typedef cy_en_timer_id_t
 * @brief Lists the software timer ID used by the application layer
 */
typedef enum {
    CY_APP_TIMERS_START_ID = CY_PDUTILS_TIMER_APP_PORT0_START_ID,
    /**< Start index for application level timers */

    CY_APP_PSOURCE_EN_TIMER = CY_APP_TIMERS_START_ID,
    /**< Timer is used for timely completion of power source enable operation. */

    CY_APP_PSOURCE_EN_MONITOR_TIMER,
    /**< Timer is used to monitor voltage during power source enable operation. */

    CY_APP_PSOURCE_EN_HYS_TIMER,
    /**< Timer is used to add hysteresis at the end of a power source enable operation. */

    CY_APP_PSOURCE_DIS_TIMER,
    /**< Timer is used for timely completion of power source disable operation. */

    CY_APP_PSOURCE_DIS_MONITOR_TIMER,
    /**< Timer is used to monitor voltage during power source disable operation. */

    CY_APP_PSOURCE_CF_TIMER,
    /**< Power source current foldback restart timer ID. */

    CY_APP_PSOURCE_DIS_EXT_DIS_TIMER,
    /**< Timer is used to discharge VBUS for some extra time at the end of a power source disable operation. */

    CY_APP_DB_SNK_FET_DIS_DELAY_TIMER,
    /**< Dead battery sink FET disable delay timer. */

    CY_APP_PSINK_DIS_TIMER,
    /**< Timer is used for timely completion of power sink disable operation. */

    CY_APP_PSINK_DIS_MONITOR_TIMER,
    /**< Timer is used to monitor voltage during power sink disable operation. */

    CY_APP_PSINK_VBUS_UVP_DEFER_TIMER,
    /**< Timer is used to defer the handling of UV fault. */

    CY_APP_VBUS_OCP_OFF_TIMER,
    /**< Timer is used to disable VBUS supply after OC fault. */

    CY_APP_VBUS_OVP_OFF_TIMER,
    /**< Timer is used to disable VBUS supply after OV fault. */

    CY_APP_VBUS_UVP_OFF_TIMER,
    /**< Timer is used to disable VBUS supply after UV fault. */

    CY_APP_VBUS_SCP_OFF_TIMER,
    /**< Timer is used to disable VBUS supply after SC fault. */

    CY_APP_FAULT_RECOVERY_TIMER,
    /**< App timer is used to delay port enable after detecting a fault. */

    CY_APP_SBU_DELAYED_CONNECT_TIMER,
    /**< Timer is used for delayed SBU connection in Thunderbolt mode. */

    CY_APP_CBL_DISC_TRIGGER_TIMER,
    /**< Timer is used to trigger cable discovery after a V5V supply change. */

    CY_APP_V5V_CHANGE_DEBOUNCE_TIMER,
    /**< Timer is used to debounce V5V voltage changes. */
    
    CY_APP_OT_DETECTION_TIMER,
    /**< Timer is used to call OT measurement handler. */

    CY_APP_CHUNKED_MSG_RESP_TIMER,
    /**< Timer ID is used to respond to chunked messages with NOT_SUPPORTED. */

    CY_APP_RESET_VDM_LAYER_TIMER,
    /**< Timer is used to run reset of VDM layer. */

    CY_APP_BB_ON_TIMER,
    /**< Timer is used to provide delay between disabling the Billboard device and re-enabling it. */

    CY_APP_BB_OFF_TIMER,
    /**< Timer is used to display USB billboard interface to save power. */

    CY_APP_INITIATE_SWAP_TIMER,
    /**< Timer is used to initiate SWAP operations in DRP applications with a power/data role preference. */

    CY_APP_VDM_NOT_SUPPORT_RESP_TIMER_ID,
    /**< VDM not supported response timer. */

    CY_APP_BC_TIMERS_START_ID,
    /**< Start of battery charging state machine timers. */

    CY_APP_BC_GENERIC_TIMER1,
    /**< Generic timer #1 is used by the BC state machine. */

    CY_APP_BC_GENERIC_TIMER2,
    /**< Generic timer #2 is used by the BC state machine. */

    CY_APP_BC_DP_DM_DEBOUNCE_TIMER,
    /**< Timer is used to debounce voltage changes on DP and DM pins. */

    CY_APP_BC_DETACH_DETECT_TIMER,
    /**< Timer is used to detect detach of a BC 1.2 sink while functioning as a CDP. */

    CY_APP_CDP_DP_DM_POLL_TIMER,
    /**< Timer is used to initiate DP/DM voltage polling while connected as a CDP. */

    CY_APP_EPR_MODE_TIMER,
    /**< Timer is used by EPR state machine. */

    CY_APP_EPR_EXT_CMD_TIMER,
    /**< Timer is used to send enter/exit EPR mode events to EPR state machine. */


    CY_APP_PSOURCE_VBUS_SET_TIMER_ID,
    /**< Power source VBUS set timer ID */

    CY_APP_PSOURCE_SAFE_FET_ON_MONITOR_TIMER_ID,
    /**< Timer to monitor voltage during FET On operation */

    CY_APP_PSOURCE_SAFE_FET_ON_TIMER_ID,
    /**< Timeout timer to set safe voltage during FET On operation */

    CY_APP_VBUS_DISCHARGE_SCHEDULE_TIMER,
    /**< Timer for VBUS slow discharge */

    CY_APP_CCG_LS_MASTER_PORT_DEBOUNCE_TIMER_ID,
    /**< Macro defines master debounce timer ID */

    CY_APP_CCG_LS_SLAVE_PORT_DEBOUNCE_TIMER_ID,
    /**< Macro defines slave debounce timer ID */

    CY_APP_CCG_LS_MASTER_WRITE_TIMER_ID,
    /**< Macro defines master write timer ID */

    CY_APP_CCG_LS_HEART_BEAT_TIMER_ID,
    /**< Macro defines heart beat timer ID */

    CY_APP_THROTTLE_TIMER_ID,
    /**< Power throttling timer ID */

    CY_APP_THROTTLE_WAIT_FOR_PD_TIMER_ID,
    /**< Power throttling timer ID */

    CY_APP_SINK_REQUEST_TIMER_ID,
    /**< Timer to repeat sending Request after Wait */

    CY_APP_LINS_BUS_INACTIVE_TIMER,
    /**< Bus inactivity timeout for LIN */
    
    CY_APP_LINS_BUS_LISTEN_TIMER,
    /**< Nominal time for reception of a single frame from BREAK */
    
    CY_APP_LINS_MULTIFRAME_DROP_TIMER,
    /**< Multiframe timer to drop the frame upon late reception */

    CY_APP_FET_SOFT_START_TIMER_ID,
    /**< Timer is used to control soft turn-on of power FET gate drivers */

    CY_APP_HAL_VREG_TIMER,
    /**< Timer is used for Vreg fault handling */

    CY_APP_HAL_GENERIC_TIMER,
    /**< Timer is used for generic HAL functions */

    CY_APP_REGULATOR_STARTUP_MONITOR_TIMER,
    /**< Timer ID reserved for regulator startup monitoring */

    CY_APP_DATA_RESET_TIMER,
    /**< Timer ID for DATA reset handling */

    CY_APP_SYS_BLACK_BOX_TIMER_ID,
    /**< Timer ID reserved for blackbox */

    CY_APP_PSOURCE_REGULATOR_MON_TIMER,
    /**< Timer ID used to monitor regulator enable status periodically */

    CY_APP_BAD_SINK_TIMEOUT_TIMER,
    /**< PD bad sink timeout timer ID */

    CY_APP_VBAT_GND_SCP_TIMER_ID,
    /**< VBAT-GND SCP recovery timer */

    CY_APP_VCONN_OCP_TIMER,
    /**< Timer performs delayed start for VCONN OCP */
    
    CY_APP_CCG_LS_SNK_CAP_TIMEOUT_TIMER_ID,
    /**< PD timeout timer for LS slave */

    CY_APP_GPIO_HPD_TIMER_ID,
    /**< GPIO based HPD timer */
    
    CY_APP_VBUS_FET_ON_TIMER,
    /**< Timer is used for providing delay for VBUS FET on */

    CY_APP_VBUS_FET_OFF_TIMER,
    /**< Timer is used for providing delay for VBUS FET off */

    CY_APP_VCONN_TURN_ON_DELAY_TIMER,
    /**< Timer is used for providing delay for VConn gate pull up enable */

    CY_APP_MOISTURE_DETECT_TIMER_ID,
    /**< Timer used to start moisture detection after typec attach wait */

    CY_APP_PD_GET_REVISION_COMMAND_RETRY_TIMER,
    /**< Timer is used to retry get revision command */

    CY_APP_GOSHEN_AUTH_RESP_WAIT_TIMER,
    /**< Goshen Ridge authentication response wait timer ID */

    CY_APP_FXVL_UPD_TIMER,
    /**< Foxville update timer ID */

    CY_APP_FXVL_SMBUS_TIMER,
    /**< Foxville SM BUS timer ID */

    CY_APP_BR_RIDGE_UPDATE_AFTER_ATTACH_WAIT_TIMER,
    /**< 50mS DFP Debounce timer ID for Barlow Ridge based docks. */

    CY_APP_VBUS_DIV_TURN_OFF_TIMER,
    /**< For EPR Max Voltages divider will be enabled, for SPR this timer is used until VBUS enter SPR range. */

    CY_APP_HX2G3_APPL_TIMER,
    /**< Wait duration for hx2g3 update activities */

    CY_APP_PSRC_VOUTBB_RCP_DELAY_TIMER_ID
    /**< Timer used for providing delay for VoutBB RCP enable. */
} cy_en_timer_id_t;

/** \} group_pmg_app_common_app_enums */
#endif /* CY_APP_TIMER_ID_H */

/* [] End of file */
