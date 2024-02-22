/***************************************************************************//**
* \file cy_app_dmc_vendor.h
* \version 1.0
*
* \brief
* Implements the DMC USB vendor interfaces
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_DMC_VENDOR_H_
#define _CY_APP_DMC_VENDOR_H_

#if (CY_APP_DMC_ENABLE || DOXYGEN)
#include "cy_usb_dev.h"
#include "cy_app_dmc_common.h"
#include "cy_app_config.h"

/**
* \addtogroup group_pmg_app_common_dmc
* \{
*/

/**
* \addtogroup group_pmg_app_common_dmc_enums
* \{
*/

/**
 * @typedef cy_en_usb_dmc_vdr_rqt_t
 * @brief USB vendor commands used in the firmware update process
 */
typedef enum
{
    CY_APP_DMC_VDR_RQT_UPGRADE_START = 0xD0,   /**< Firmware upgrade start. Also send ECDSA signature
                                              in case of signed FW update>
                                              CMD_CODE: 0xD0
                                              VALUE:
                                                    0x00: Non-signed update
                                                    0x01: Signed update
                                              INDEX: 0.
                                              LENGTH: 0 in case of non-signed update.
                                                    : 64 in case of signed update
                                              EP0 DATA: None. In case of non-signed update.
                                                      : ECDSA Signature; in case of signed update.
                                        */
    CY_APP_DMC_VDR_RQT_FWCT_WRITE = 0xD2,       /**< FWCT Write >
                                              CMD_CODE: 0xD2
                                              VALUE: 0
                                              INDEX: 0.
                                              LENGTH: FWCT table length
                                              EP0 DATA: FWCT table
                                        */
    CY_APP_DMC_VDR_RQT_IMG_WRITE,              /**< Image write >
                                              CMD_CODE: 0xD3
                                              VALUE: Segment row start address.
                                              INDEX: Image's segment length (in units of num of rows).
                                              LENGTH: 0
                                              EP0 DATA: None
                                        */
    CY_APP_DMC_VDR_RQT_GET_IMG_WRITE_STATUS,   /**< Image write status. Returns the status of last
                                                image write. >
                                              CMD_CODE: 0xD4
                                              VALUE: 0
                                              INDEX: 0
                                              LENGTH: 1
                                              EP0 DATA: Status
                                                      0x00: SUCCESS
                                                      0x01: IN_PROGRESS
                                                      0x02 to 0xFF: FAIL
                                        */
    CY_APP_DMC_VDR_RQT_GET_FW_UPGRADE_STATUS,  /**< Overall firmware upgrade status >
                                              CMD_CODE: 0xD5
                                              VALUE: 0
                                              INDEX: 0
                                              LENGTH: 1
                                              EP0 DATA:
                                              DATA0:  Status
                                                      0x00: SUCCESS
                                                      0x01: IN_PROGRESS
                                                      0x02: IDLE (no FW update in progress)
                                                      0x03 to 0xFF: FAIL
                                        */
    CY_APP_DMC_VDR_RQT_GET_DOCK_STATUS,        /**< Overall dock status >
                                              CMD_CODE: 0xD6
                                              VALUE: 0
                                              INDEX: 0
                                              LENGTH: Length of EP0 data in bytes
                                              EP0 DATA:
                                              DATA0:  Dock status
                                              DATA1:  Device count
                                              DATA2: Length of status (including DATA0, DATA1)
                                              DATA3: Composite version
                                              DATA(4+x) - DATA(36+x) - DEVx STATUS
                                                       BYTE0 - Device type
                                                       BYTE1 - Component ID
                                                       BYTE2 - Image mode (single image/ dual symmetric/ dual asymmetric image)
                                                       BYTE3 - Current running image
                                                       BYTE4 - Image status
                                                                 * B7:B4 => Image 2 status
                                                                 * B3:B0 => Image 1 status
                                                                 *  0 = Unknown
                                                                 *  1 = Valid
                                                                 *  2 = Invalid
                                                                 *  3-0xF = Reserved
                                                       BYTE5-7   - Reserved.
                                                       BYTE8-15  - Devx bootloader version
                                                       BYTE16-23 - Devx img1 version
                                                       BYTE24-31 - Devx img2 version
                                        */
    CY_APP_DMC_VDR_RQT_GET_DOCK_IDENTITY,      /**< Get dock identity >
                                              CMD_CODE: 0xD7
                                              VALUE: 0
                                              INDEX: 0
                                              LENGTH: Length of EP0 data in bytes
                                              EP0 DATA:
                                              DATA0:     cdtt valid/ not
                                              DATA1:     CDTT_Version
                                              DATA2-3:   Vendor ID
                                              DATA4-5:   Product ID
                                              DATA6-7:   Device ID
                                              DATA8-39:  Vendor string
                                              DATA40-71: Product string
                                              DATA72:    signed_only update/not
                                        */
    CY_APP_DMC_VDR_RQT_STATE_RESET,            /**< Resets DMC to a known state before starting FW update >
                                              CMD_CODE: 0xD8
                                              Reserved - Not implemented
                                        */
#if (CY_APP_USR_DEFINED_SN_SUPPORT)
    CY_APP_DMC_VDR_RQT_SN_UPDATE,              /**< Update the user provided serial number in HX3 >
                                              CMD_CODE: 0xD9
                                              VALUE: 0
                                              INDEX: 0
                                              LENGTH: Length of the serial number(SN) in bytes
                                        */
#endif /* CY_APP_USR_DEFINED_SN_SUPPORT */
    CY_APP_DMC_VDR_RQT_TRIGGER_PHASE2_UPDATE = 0xDA,  /**< Update the user provided serial number in HX3 >
                                              CMD_CODE: 0xDA
                                              VALUE: 0
                                              INDEX: 0
                                              LENGTH: Length of the serial number(SN) in bytes
                                        */
    
    CY_APP_DMC_VDR_RQT_RETRIEVE_FLASH_LOG    = 0xDE, /**< Read the logs stored in the internal flash >
                                               CMD_CODE: 0xDE
                                              VALUE: 0
                                              INDEX: 0
                                              LENGTH: Length of Flash logs in bytes
                                        */    
                                        
    /* Debug vendor commands */
#if (CY_APP_DEBUG_VENDOR_CMD_ENABLE == 1)
    CY_APP_DMC_DBG_FLASH_WRITE = 0xE0,

    CY_APP_DMC_DBG_GET_IMG_VERSION = 0xE1,

    CY_APP_DMC_DBG_OVERRIDE_STATE = 0xE2,       /* Pass state number in wValue */

    CY_APP_DMC_DBG_JUMP_TO_ALT = 0xE3,

    CY_APP_DMC_DBG_ADC_CTRL = 0xE4,

    CY_APP_DMC_DBG_SHA2_CALC = 0xE5,

    CY_APP_DMC_DBG_SPI_COMMANDS = 0xE7,

    CY_APP_DMC_DBG_TRIGGER_PHASE = 0xE8,

    CY_APP_DMC_DBG_SOFT_RESET = 0xE9,

    CY_APP_DMC_DBG_CUSTOM_CMD = 0xEA,

#endif /* (CY_APP_DEBUG_VENDOR_CMD_ENABLE == 1) */

} cy_en_usb_dmc_vdr_rqt_t;

/* \} group_pmg_app_common_dmc_enums */

/**
* \addtogroup group_pmg_app_common_dmc_data_structures
* \{
*/
/**
 * @brief Internal structure to hold the data to be sent over USB INT IN EP.
 */
typedef struct
{
    uint8_t   opcode;     /**< Opcode for the request/event to be sent over INT IN endpoint */
    uint8_t   length;     /**< Length of the data including OPCODE and LENGTH fields */
    uint8_t   data;       /**< data */

}cy_stc_app_dmc_int_ep_status_t;

/* \} group_pmg_app_common_dmc_data_structures */

/**
* \addtogroup group_pmg_app_common_dmc_functions
* \{
*/

/**
 * @brief DMC vendor request handler
 * @param params Pointer to the DMC parameters
 * @param transfer Pointer to the control endpoint transfer request
 * @return Returns CY_USB_DEV_SUCCESS if successful otherwise
 * CY_USB_DEV_REQUEST_NOT_HANDLED
 * */
cy_en_usb_dev_status_t Cy_App_Dmc_VendorRequestHandler (cy_stc_dmc_params_t *params, cy_stc_usb_dev_control_transfer_t *transfer);
/* \} group_pmg_app_common_dmc_functions */
/* \} group_pmg_app_common_dmc */
#endif /* (CY_APP_DMC_ENABLE || DOXYGEN) */

#endif /* _CY_APP_DMC_VENDOR_H_ */

/* [] END OF FILE */
