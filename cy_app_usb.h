/***************************************************************************//**
* \file cy_app_usb.h
* \version 1.0
*
* \brief
* Defines the function prototypes for USB
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_USB_H_
#define _CY_APP_USB_H_

#if (CY_APP_USB_ENABLE || DOXYGEN)
#include "cy_usb_dev.h"
#include "cy_usb_dev_hid.h"

/**
* \addtogroup group_pmg_app_common_usb
* \{
********************************************************************************
* \section section_pmg_app_common_usb_config Configuration considerations
********************************************************************************
*
* To enable USB module for the application, the steps are:
* 
* Step 1: Enable the USB device middleware. \n 
* Launch ModusToolbox(TM) library manager and enable the USB device middleware. This step is required if the ModusToolbox IDE is used. Otherwise, ensure the USB Device Middleware is included in your project.
*
* Step 2: Write the USB descriptors.
* 1. Generate the USB descriptors and initialization arrays using the USB configurator. 
* 2. If the descriptors are to be modified, then appropriate code should be added as per the application requirements.
*
* Step 3: Update main.c as follows:
* 1. Include headers to get access to the USB module.
* \snippet snippet/usb_snippet.c snippet_configuration_include
* 2. Declare the USB context global variables. 
* \snippet snippet/usb_snippet.c snippet_global_context
* 3. Declare and initialize the USB driver initialization context. Initialize the USB interface using the USB driver initialization context. 
* \snippet snippet/usb_snippet.c snippet_usb_initialization
* 4. If the application makes use of alternate modes, then Billboard support must be enabled. In this case, the USB module is automatically enabled when an alternate mode is entered or when tAMETimeout is expired. If alternate modes are not used, then enable the USB interface by using the API Cy_App_Usb_Enable(). 
* \snippet snippet/usb_snippet.c snippet_usb_enable 
* 5. Call tasks related to the USB module in the main loop. The commonly used tasks are: \n 
* a. Cy_App_Task() which internally calls Cy_App_Usb_BbTask() to handle billboard events. This task should be called if alternate modes are supported by the application. \n 
* b. Any custom task to handle events related to a USB interface. For example, HID task to handle HID events. 
* \snippet snippet/usb_snippet.c snippet_usb_task 
*
* */

/**
* \addtogroup group_pmg_app_common_usb_enums
* \{
*/
/**
 * @brief USB EP0 states
 */
typedef enum
{
    CY_APP_USB_EP0_STATE_DISABLED = 0, /**< EP0 is disabled. */
    CY_APP_USB_EP0_STATE_SETUP,        /**< EP0 in setup phase. */
    CY_APP_USB_EP0_STATE_DATA_IN,      /**< EP0 in data IN phase. */
    CY_APP_USB_EP0_STATE_DATA_OUT,     /**< EP0 in data OUT phase. */
    CY_APP_USB_EP0_STATE_STATUS_IN,    /**< EP0 in status IN ZLP phase. */
    CY_APP_USB_EP0_STATE_STATUS_OUT,   /**< EP0 in status OUT ZLP phase. */
    CY_APP_USB_EP0_STATE_STALL         /**< EP0 in stall phase. */
} cy_en_usb_ep0_state_t;
/** \} group_pmg_app_common_usb_enums */

/**
* \addtogroup group_pmg_app_common_usb_data_structures
* \{
*/

/** USB EP index type */
typedef uint8_t cy_app_usb_ep_index_t;

/** USB setup callback */
typedef cy_en_usb_dev_status_t (*cy_app_usb_setup_cbk_t) (cy_stc_usb_dev_control_transfer_t *transfer);

/** USB vendor request handler Callback */
typedef cy_en_usb_dev_status_t (*cy_app_vendor_req_hdlr_cbk_t) (cy_stc_usb_dev_control_transfer_t *transfer);


/** USB Handle. */
typedef struct
{
    cy_en_usb_ep0_state_t ep0_state;  /**< EP0 state. */

    uint8_t *ep0_buffer;        /**< Current EP0 data transfer buffer */
    uint16_t ep0_length;        /**< Pending transfer size for EP0 */
    cy_app_usb_setup_cbk_t ep0_xfer_cb; /**< Transfer completion callback */
    
} cy_stc_app_usb_handle_t;

/** USB driver initialization context structure. */
typedef struct
{
    USBFS_Type *base;   /**< Pointer to USBFS base */
    struct cy_stc_usbfs_dev_drv_config const *drvConfig; /**< Pointer to USBFS device driver configuration structure */
    cy_stc_usb_dev_config_t  const *usbDevConfig; /**< Pointer to USB device configuration structure */
    cy_stc_usb_dev_device_t  const *usbDevice; /**< Pointer to USB device structure */
    cy_stc_usb_dev_hid_config_t  const *hidConfig; /**< Pointer to HID configuration structure */
    cy_cb_usbfs_dev_hid_get_report_t getReportCallback; /**< HID report callback */
    cy_cb_usbfs_dev_hid_set_report_t setReportCallback; /**< HID set report callback */
    cy_app_vendor_req_hdlr_cbk_t vendor_req_handler; /**< Vendor request handler */
    cy_stc_usbfs_dev_drv_context_t  *usb_drvContext; /**< USBFS device context structure */
    cy_stc_usb_dev_context_t        *usb_devContext; /**< USB device context structure */
#if (CY_APP_USB_HID_INTF_ENABLE || DOXYGEN)
    cy_stc_usb_dev_hid_context_t    *usb_hidContext; /**< HID class context structure */
#endif /* (CY_APP_USB_HID_INTF_ENABLE || DOXYGEN) */
} cy_stc_usb_init_ctxt_t;

/** \} group_pmg_app_common_usb_data_structures */

/**
* \addtogroup group_pmg_app_common_usb_functions
* \{
*/

/**
 * @brief Initializes the USB application layer
 * @param usbInitCtx Pointer to the USB initialization context
 * @return None
 * */
void Cy_App_Usb_Init (cy_stc_usb_init_ctxt_t *usbInitCtx);


/**
 * @brief Enables the USB device
 * @return None
 * */
void Cy_App_Usb_Enable (void);

/**
 * @brief Disables the USB device
 * @return None
 * */
void Cy_App_Usb_Disable (void);

/**
 * @brief Checks if the USB module is idle
 * @return True if idle; false otherwise.
 * */
bool Cy_App_Usb_IsIdle (void);

/**
 * @brief Prepares the USB module for Deep Sleep
 * @return None
 * */
void Cy_App_Usb_Sleep (void);

/**
 * @brief Resumes the USB module from Deep Sleep
 * @return None
 * */
void Cy_App_Usb_Resume (void);

/**
 * @brief Reads the control endpoint status from the driver and stores the state in the application context.
 * @return None
 * */
void Cy_App_Usb_GetEp0State(void);

/**
 * @brief Retrieves the pointer to the control endpoint buffer
 * @return None
 * */
uint8_t *Cy_App_Usb_GetEp0Buffer(void);

/**
 * @brief Checks if the given endpoint is ready
 * @param ep_index Pointer to the endpoint number
 * @return True if the endpoint is ready; false otherwise.
 * */
bool Cy_App_Usb_IsEpReady(cy_app_usb_ep_index_t ep_index);

/**
 * @brief Checks the OUT endpoint status and reads data into given
 * buffer if data is available.
 * @param ep_index Endpoint number to read data.
 * @param data Pointer to the buffer into which data is read.
 * @param count Number of data bytes read from the endpoint.
 * @return Returns CY_USB_DEV_SUCCESS if data is read otherwise, returns
 * CY_USB_DEV_REQUEST_NOT_HANDLED or CY_USB_DEV_TIMEOUT.
 * */
cy_en_usb_dev_status_t Cy_App_Usb_ReadSingle(cy_app_usb_ep_index_t ep_index, uint8_t *data, uint8_t *count);

/**
 * @brief Queues a non-blocking write of data on the given endpoint
 * @param ep_index Endpoint number to write data
 * @param data Pointer to the buffer containing the data
 * @param count Number of data bytes to be written
 * @return Returns CY_USB_DEV_SUCCESS if data write queue is successful
 * otherwise, returns CY_USB_DEV_REQUEST_NOT_HANDLED or CY_USB_DEV_TIMEOUT.
 * */
cy_en_usb_dev_status_t Cy_App_Usb_WriteSingle(cy_app_usb_ep_index_t ep_index, uint8_t *data, uint8_t count);

/**
 * @brief Sets up a read on the control endpoint.
 * @param data Pointer to tbe buffer from which the data is read.
 * @param length Number of bytes to read.
 * @param cb Invokes callback when the data is available.
 * @return Returns CY_USB_DEV_SUCCESS if set up is successful otherwise, returns
 * CY_USB_DEV_DRV_HW_DISABLED.
 * */
cy_en_usb_dev_status_t Cy_App_Usb_Ep0SetupRead(uint8_t *data, uint16_t length, cy_app_usb_setup_cbk_t cb);

/**
 * @brief Sets up a write on the control endpoint
 * @param data Pointer to tbe buffer containing the data to be written
 * @param length Number of bytes to write
 * @param cb Invokes callback when the data write is complete
 * @param last True if this is the last chunk in the transfer
 * @return Returns CY_USB_DEV_SUCCESS if set up is successful otherwise, returns
 * CY_USB_DEV_DRV_HW_DISABLED.
 * */
cy_en_usb_dev_status_t Cy_App_Usb_Ep0SetupWrite(uint8_t *data, uint16_t length, bool last, cy_app_usb_setup_cbk_t cb);

/**
 * @brief Receives a vendor request from the host
 * @param transfer Pointer to the USB transfer structure
 * @param classContext Callback context
 * @param devContext Pointer to the USB device context
 * @return Returns CY_USB_DEV_SUCCESS if successful otherwise,
 * CY_USB_DEV_REQUEST_NOT_HANDLED.
 * */
cy_en_usb_dev_status_t Cy_App_Usb_VendorReqReceived (cy_stc_usb_dev_control_transfer_t *transfer, void *classContext, struct cy_stc_usb_dev_context *devContext);

/**
 * @brief Completion of vendor request handling
 * @param transfer Pointer to the USB transfer structure
 * @param classContext Callback context
 * @param devContext Pointer to the USB device context
 * @return Returns CY_USB_DEV_SUCCESS if successful otherwise,
 * CY_USB_DEV_REQUEST_NOT_HANDLED.
 * */
cy_en_usb_dev_status_t Cy_App_Usb_VendorReqCompleted (cy_stc_usb_dev_control_transfer_t *transfer, void *classContext, struct cy_stc_usb_dev_context *devContext);

/**
 * @brief Processing the vendor request
 * @param transfer Pointer to the USB transfer structure
 * @return Returns CY_USB_DEV_SUCCESS if successful otherwise,
 * CY_USB_DEV_REQUEST_NOT_HANDLED.
 * */
cy_en_usb_dev_status_t Cy_App_Usb_VendorReqHandler (cy_stc_usb_dev_control_transfer_t *transfer);

/**
 * @brief Read the received USB data into a buffer
 * @param ep_index Endpoint number
 * @param buffer_p Pointer to the buffer into which received data is read
 * @param recd_bytes Output parameter containing the number of bytes received
 * @return Returns CY_USB_DEV_SUCCESS if data is read otherwise, returns
 * CY_USB_DEV_REQUEST_NOT_HANDLED or CY_USB_DEV_TIMEOUT.
 */
cy_en_usb_dev_status_t Cy_App_Usb_ReceiveData(cy_app_usb_ep_index_t ep_index, uint8_t *buffer_p, uint8_t *recd_bytes);
/**
 * @brief Loads status data into the interrupt endpoint
 * @param ep_index Endpoint number
 * @param data Pointer to the buffer containing data to be sent
 * @param size Number of bytes to be sent
 * @return Returns CY_USB_DEV_SUCCESS if data is read otherwise, returns
 * CY_USB_DEV_DRV_HW_ERROR or CY_USB_DEV_TIMEOUT.
 */
cy_en_usb_dev_status_t Cy_App_Usb_SendStatus(cy_app_usb_ep_index_t ep_index, uint8_t *data, uint8_t size);

#endif /* (CY_APP_USB_ENABLE || DOXYGEN) */

/** \} group_pmg_app_common_usb_functions */
/** \} group_pmg_app_common_usb */

#endif /* _CY_APP_USB_H_ */

/* [] End of file */
