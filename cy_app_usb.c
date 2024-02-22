/***************************************************************************//**
* \file cy_app_usb.c
* \version 1.0
*
* \brief
* Implements the function for USB operation
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#if CY_APP_USB_ENABLE
#include "cy_pdl.h"
#include "cybsp.h"

#include "cy_app_status.h"
#include "cy_app_usb.h"
#include "cy_app_dmc_vendor.h"
#include "cy_app_flash_log.h"
#include "cy_pdutils.h"
#include "cy_pdstack_dpm.h"

/*******************************************************************************
* Macro definitions
********************************************************************************/
#define USB_EP0_SIZE  (8u)

/*******************************************************************************
* Global variables
********************************************************************************/
cy_stc_app_usb_handle_t gl_usb;
uint8_t gl_ep0_buffer[516];
cy_stc_usb_init_ctxt_t *glUsbInitCtx;

/*******************************************************************************
* Function prototypes
********************************************************************************/
static void usb_high_isr(void);
static void usb_medium_isr(void);
static void usb_low_isr(void);

/* USB interrupt configuration */
const cy_stc_sysint_t usb_high_interrupt_cfg =
{
    .intrSrc = (IRQn_Type) usb_interrupt_hi_IRQn,
    .intrPriority = 0U,
};
const cy_stc_sysint_t usb_medium_interrupt_cfg =
{
    .intrSrc = (IRQn_Type) usb_interrupt_med_IRQn,
    .intrPriority = 1U,
};
const cy_stc_sysint_t usb_low_interrupt_cfg =
{
    .intrSrc = (IRQn_Type) usb_interrupt_lo_IRQn,
    .intrPriority = 2U,
};

/***************************************************************************
* Function name: usb_high_isr
********************************************************************************
* Summary:
*  Processed the high-priority USB interrupts.
*
***************************************************************************/
static void usb_high_isr(void)
{
    /* Call interrupt processing */
    Cy_USBFS_Dev_Drv_Interrupt(glUsbInitCtx->base,
                               Cy_USBFS_Dev_Drv_GetInterruptCauseHi(glUsbInitCtx->base),
                               glUsbInitCtx->usb_drvContext);
}

/***************************************************************************
* Function name: usb_medium_isr
********************************************************************************
* Summary:
* Process the medium-priority USB interrupts.
*
***************************************************************************/
static void usb_medium_isr(void)
{
    /* Call interrupt processing */
    Cy_USBFS_Dev_Drv_Interrupt(glUsbInitCtx->base,
                               Cy_USBFS_Dev_Drv_GetInterruptCauseMed(glUsbInitCtx->base),
                               glUsbInitCtx->usb_drvContext);
}

/***************************************************************************
* Function name: usb_low_isr
********************************************************************************
* Summary:
*  Processes the low-priority USB interrupts.
*
**************************************************************************/
static void usb_low_isr(void)
{
    /* Call interrupt processing */
    Cy_USBFS_Dev_Drv_Interrupt(glUsbInitCtx->base,
                               Cy_USBFS_Dev_Drv_GetInterruptCauseLo(glUsbInitCtx->base),
                               glUsbInitCtx->usb_drvContext);
}

void Cy_App_Usb_Init (cy_stc_usb_init_ctxt_t *usbInitCtx)
{
    glUsbInitCtx = usbInitCtx;

    if((Cy_PdStack_Dpm_GetContext(TYPEC_PORT_0_IDX))->ptrUsbPdContext->adcVdddMv[CY_USBPD_ADC_ID_0] > 3700)
    {
        Cy_USBFS_Dev_Drv_RegEnable(glUsbInitCtx->base, glUsbInitCtx->usb_drvContext);
    }
    else
    {
        Cy_USBFS_Dev_Drv_RegDisable(glUsbInitCtx->base, glUsbInitCtx->usb_drvContext);
    }

    Cy_USB_Dev_Init(glUsbInitCtx->base, glUsbInitCtx->drvConfig, glUsbInitCtx->usb_drvContext,
                    glUsbInitCtx->usbDevice, glUsbInitCtx->usbDevConfig, glUsbInitCtx->usb_devContext);

#if CY_APP_USB_HID_INTF_ENABLE
    /* Initialize the HID class */
    Cy_USB_Dev_HID_Init(glUsbInitCtx->hidConfig, glUsbInitCtx->usb_hidContext, glUsbInitCtx->usb_devContext);

    Cy_USB_Dev_HID_RegisterGetReportCallback(glUsbInitCtx->getReportCallback, glUsbInitCtx->usb_hidContext);
    Cy_USB_Dev_HID_RegisterSetReportCallback(glUsbInitCtx->setReportCallback, glUsbInitCtx->usb_hidContext);
#endif /* CY_APP_USB_HID_INTF_ENABLE */

    /* Initialize the USB interrupts */
    Cy_SysInt_Init(&usb_high_interrupt_cfg,   &usb_high_isr);
    Cy_SysInt_Init(&usb_medium_interrupt_cfg, &usb_medium_isr);
    Cy_SysInt_Init(&usb_low_interrupt_cfg,    &usb_low_isr);

    /* Enable the USB interrupts */
    NVIC_EnableIRQ(usb_high_interrupt_cfg.intrSrc);
    NVIC_EnableIRQ(usb_medium_interrupt_cfg.intrSrc);
    NVIC_EnableIRQ(usb_low_interrupt_cfg.intrSrc);
}

bool Cy_App_Usb_IsIdle (void)
{
    static uint8_t debounce = 0;
    
    if (glUsbInitCtx->usb_devContext->state == CY_USB_DEV_DISABLED)
    {
        return true;
    }

    if (0U == Cy_USBFS_Dev_Drv_CheckActivity(glUsbInitCtx->base))
    {
        debounce ++;

        if (debounce >= 6)
        {
            debounce = 0;
            return true;
        }
    }
    else
    {
        debounce = 0;
    }

    return false;
}

void Cy_App_Usb_Sleep (void)
{
    /* Prepare the device to move to Deep Sleep */
    Cy_USBFS_Dev_Drv_Suspend(glUsbInitCtx->base, glUsbInitCtx->usb_drvContext);

#if defined(CY_DEVICE_PMG1S3)
    Cy_GPIO_SetInterruptEdge(GPIO_PRT8, 0u, CY_GPIO_INTR_FALLING);
#endif /* defined(CY_DEVICE_PMG1S3) */

}

void Cy_App_Usb_Resume (void)
{
#if defined(CY_DEVICE_PMG1S3)
    /* Disable the wake-up interrupt pin */
    Cy_GPIO_SetInterruptEdge(GPIO_PRT8, 0u, CY_GPIO_INTR_DISABLE);
#endif /* defined(CY_DEVICE_PMG1S3) */

    /* Prepares the USBFS component for operation after exiting Deep Sleep mode */
    Cy_USBFS_Dev_Drv_Resume(glUsbInitCtx->base, glUsbInitCtx->usb_drvContext);
}

void Cy_App_Usb_Enable (void)
{
    /* Makes device appear on the bus. This function call is blocked,
     * till the device enumerates.
     */
    Cy_USB_Dev_Connect(false, 1, glUsbInitCtx->usb_devContext);

    Cy_USB_Dev_RegisterVendorCallbacks(&Cy_App_Usb_VendorReqReceived, &Cy_App_Usb_VendorReqCompleted, glUsbInitCtx->usb_devContext);
}

void Cy_App_Usb_Disable (void)
{
    Cy_USB_Dev_Disconnect(glUsbInitCtx->usb_devContext);
}

void Cy_App_Usb_GetEp0State(void)
{
    cy_stc_usb_dev_control_transfer_t *transfer = &glUsbInitCtx->usb_devContext->ControlTransfer;
    cy_stc_usbfs_dev_drv_context_t      *USB_drv_context = glUsbInitCtx->usb_drvContext;

    switch (USB_drv_context->ep0CtrlState)
    {
        case CY_USBFS_DEV_DRV_EP0_CTRL_STATE_IDLE:
                gl_usb.ep0_state = CY_APP_USB_EP0_STATE_DISABLED;
            break;
        case CY_USBFS_DEV_DRV_EP0_CTRL_STATE_SETUP:
                gl_usb.ep0_state = CY_APP_USB_EP0_STATE_SETUP;
            break;
        case CY_USBFS_DEV_DRV_EP0_CTRL_STATE_DATA:
            if(transfer->direction == CY_USB_DEV_DIR_DEVICE_TO_HOST)
            {
                gl_usb.ep0_state = CY_APP_USB_EP0_STATE_DATA_IN;
            }
            else{
                gl_usb.ep0_state = CY_APP_USB_EP0_STATE_DATA_OUT;
            }
            break;
        case CY_USBFS_DEV_DRV_EP0_CTRL_STATE_STATUS_IN:
                gl_usb.ep0_state = CY_APP_USB_EP0_STATE_STATUS_IN;
            break;
        case CY_USBFS_DEV_DRV_EP0_CTRL_STATE_STATUS_OUT:
                gl_usb.ep0_state = CY_APP_USB_EP0_STATE_STATUS_OUT;
            break;
        default:
                gl_usb.ep0_state = CY_APP_USB_EP0_STATE_STALL;
    }
}

uint8_t *Cy_App_Usb_GetEp0Buffer(void)
{
    return (uint8_t *)gl_ep0_buffer;
}

cy_en_usb_dev_status_t Cy_App_Usb_Ep0SetupWrite(uint8_t *data, uint16_t length, bool last, cy_app_usb_setup_cbk_t cb)
{
    cy_stc_usb_dev_control_transfer_t *transfer = &glUsbInitCtx->usb_devContext->ControlTransfer;

    if (data == NULL)
    {
        return CY_USB_DEV_BAD_PARAM;
    }

    Cy_App_Usb_GetEp0State();

    if ((gl_usb.ep0_state != CY_APP_USB_EP0_STATE_SETUP) &&
            (gl_usb.ep0_state != CY_APP_USB_EP0_STATE_DATA_IN))
    {
        return CY_USB_DEV_DRV_HW_DISABLED;
    }

    transfer->ptr = (uint8_t *)data;
    transfer->remaining = length;

    if(length == 0)
    {
        transfer->zlp = true;
    }

    gl_usb.ep0_xfer_cb = cb;

    (void) last;
    return CY_USB_DEV_SUCCESS;
}

cy_en_usb_dev_status_t Cy_App_Usb_Ep0SetupRead(uint8_t *data, uint16_t length, cy_app_usb_setup_cbk_t cb)
{
    cy_stc_usb_dev_control_transfer_t *transfer = &glUsbInitCtx->usb_devContext->ControlTransfer;

    /* All read should be multiple of EP0 size. */
    if ((data == NULL) || ((length & (USB_EP0_SIZE - 1)) != 0))
    {
        return CY_USB_DEV_BAD_PARAM;
    }

    Cy_App_Usb_GetEp0State();

    if ((gl_usb.ep0_state != CY_APP_USB_EP0_STATE_SETUP) &&
            (gl_usb.ep0_state != CY_APP_USB_EP0_STATE_DATA_OUT))
    {
        return CY_USB_DEV_DRV_HW_DISABLED;
    }

    transfer->buffer = gl_ep0_buffer;
    transfer->remaining = length;
    transfer->bufferSize = transfer->setup.wLength;
    transfer->notify = true;

    gl_usb.ep0_state = CY_APP_USB_EP0_STATE_DATA_OUT;
    gl_usb.ep0_buffer = data;
    gl_usb.ep0_length = length;
    gl_usb.ep0_xfer_cb = cb;

    return CY_USB_DEV_SUCCESS;
}

bool Cy_App_Usb_IsEpReady(cy_app_usb_ep_index_t ep_index)
{
    cy_en_usb_dev_ep_state_t epState;

    epState = Cy_USBFS_Dev_Drv_GetEndpointState(glUsbInitCtx->usb_devContext->drvBase, (ep_index), glUsbInitCtx->usb_drvContext);
    if ((CY_USB_DEV_EP_COMPLETED == epState) || (CY_USB_DEV_EP_IDLE == epState)) 
    {
        return true;
    }

    return false;
}

cy_en_usb_dev_status_t Cy_App_Usb_WriteSingle(cy_app_usb_ep_index_t ep_index, uint8_t *data, uint8_t count)
{
    cy_en_usb_dev_status_t status = CY_USB_DEV_SUCCESS;
    uint8_t len;

    /* Send status functions which are always <64B. If it is changed, multiple writes occurs.*/
    len = CY_PDUTILS_GET_MIN(64, count);

    status = Cy_USB_Dev_WriteEpNonBlocking(ep_index, data, len, glUsbInitCtx->usb_devContext);
    if (status != CY_USB_DEV_SUCCESS)
    {
        status = Cy_USB_Dev_AbortEpTransfer(ep_index, glUsbInitCtx->usb_devContext);
    }

    return status;
}

cy_en_usb_dev_status_t Cy_App_Usb_ReadSingle(cy_app_usb_ep_index_t ep_index, uint8_t *data, uint8_t *count)
{
    cy_en_usb_dev_status_t status = CY_USB_DEV_REQUEST_NOT_HANDLED;
    uint32_t len, readLen;
    cy_en_usb_dev_ep_state_t ep_state;

    /* Read the OUT endpoint state */
    ep_state = Cy_USBFS_Dev_Drv_GetEndpointState(glUsbInitCtx->usb_devContext->drvBase,
                                ep_index, glUsbInitCtx->usb_drvContext);
    /* Check if any data from USB host is ready to be read */
    if (ep_state == CY_USB_DEV_EP_COMPLETED)
    {
        len = Cy_USB_Dev_GetEpNumToRead(ep_index, glUsbInitCtx->usb_devContext);
        status = Cy_USB_Dev_ReadEpNonBlocking(ep_index, data, len, &readLen,glUsbInitCtx->usb_devContext);

        if (status != CY_USB_DEV_SUCCESS)
        {
            status = Cy_USB_Dev_AbortEpTransfer(ep_index,glUsbInitCtx->usb_devContext);
        }
        else
        {
            *count = readLen;
        }
    }
    return status;
}

/**
 * @brief Load status data into the interrupt endpoint.
 */
cy_en_usb_dev_status_t Cy_App_Usb_SendStatus(cy_app_usb_ep_index_t ep_index, uint8_t *data, uint8_t size)
{
    bool ep_state;
    cy_en_usb_dev_status_t status = CY_USB_DEV_SUCCESS;

    ep_state = Cy_App_Usb_IsEpReady(ep_index);
    /* This is the first state. Wait for the IN EP buffer to be free. */
    if (ep_state == false)
    {
        return CY_USB_DEV_DRV_HW_ERROR;
    }

    /* Copy INT IN data to the EP buffer and send. */
    status = Cy_App_Usb_WriteSingle(ep_index, (uint8_t *)data, size);
    if (status != CY_USB_DEV_SUCCESS)
    {
        /* Flush the status endpoint to allow for retry */
        status = Cy_USB_Dev_AbortEpTransfer(ep_index, glUsbInitCtx->usb_devContext);
    }

    return status;
}

/**
 * @brief Read the USB data received into a buffer.
 */
cy_en_usb_dev_status_t Cy_App_Usb_ReceiveData(cy_app_usb_ep_index_t ep_index, uint8_t *buffer_p, uint8_t *recd_bytes)
{
    cy_en_usb_dev_status_t status = CY_USB_DEV_REQUEST_NOT_HANDLED;

    *recd_bytes = 0;

    /* This is the first state. Wait for the USB data. */
    if (Cy_App_Usb_IsEpReady(ep_index) == true)
    {
        /* Copies the data if it is available. */
        status = Cy_App_Usb_ReadSingle(ep_index, buffer_p, recd_bytes);
    }

    return status;
}

/**************************************************************************
 * Vendor request handling function
 * ***********************************************************************/
cy_en_usb_dev_status_t Cy_App_Usb_VendorReqReceived (cy_stc_usb_dev_control_transfer_t *transfer, void *classContext, struct cy_stc_usb_dev_context *devContext)
{
    cy_en_usb_dev_status_t status = CY_USB_DEV_REQUEST_NOT_HANDLED;

    status = glUsbInitCtx->vendor_req_handler(transfer);

    if(status == CY_USB_DEV_REQUEST_NOT_HANDLED)
    {  
#if CY_APP_FLASH_LOG_ENABLE
        /** Application handling */
        if(transfer->setup.bRequest == CY_APP_DMC_VDR_RQT_RETRIEVE_FLASH_LOG)
        {
            uint8_t *ptr = Cy_App_Usb_GetEp0Buffer();
            cy_stc_app_flash_log_t *flash_log_ptr;
            flash_log_ptr = Cy_App_Debug_FlashReadLogs();
            if(flash_log_ptr != NULL)
            {
                memcpy(ptr, (uint8_t *)flash_log_ptr, CY_APP_DEBUG_FLASH_LOG_SIZE);
                status = Cy_App_Usb_Ep0SetupWrite(ptr, CY_APP_DEBUG_FLASH_LOG_SIZE, true, NULL);
            }

        }
        else
#endif /* CY_APP_FLASH_LOG_ENABLE */
        {
            /* Add handling for other vendor commands. */
            status = CY_USB_DEV_SUCCESS;
        }
    }

    (void) classContext;
    (void) devContext;
    return status;
}

/************************************************************************************
 * Vendor request data stage handling
 * This API does post processing of the control OUT data stage.
 * The actual response to the data stage is taken care by the Middleware itself.
 ***********************************************************************************/

cy_en_usb_dev_status_t Cy_App_Usb_VendorReqCompleted (cy_stc_usb_dev_control_transfer_t *transfer, void *classContext, struct cy_stc_usb_dev_context *devContext)
{
    cy_en_usb_dev_status_t status = CY_USB_DEV_REQUEST_NOT_HANDLED;

    memcpy(gl_usb.ep0_buffer, transfer->ptr, gl_usb.ep0_length);

    if (gl_usb.ep0_xfer_cb != NULL)
    {
        status = gl_usb.ep0_xfer_cb(transfer);
    }

    (void) classContext;
    (void) devContext;
    return status;
}

#endif /* CY_APP_USB_ENABLE */

/* [] End of file */
