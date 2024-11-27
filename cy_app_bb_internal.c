/***************************************************************************//**
* \file cy_app_bb_internal.c
* \version 2.0
*
* \brief
* Implements functions for the billboard control interface
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#if (CCG_BB_ENABLE != 0)
#include "cy_pdstack_common.h"
#include "cy_pdstack_dpm.h"
#include "cy_app_config.h"
#include "cy_app_vdm.h"
#include "cy_app.h"
#include "cy_app_usb.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_timer_id.h"
#include "cy_gpio.h"
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
#include "cy_pdaltmode_defines.h"
#include "cy_pdaltmode_billboard.h"
#include "cy_pdaltmode_hw.h"
#include "cy_pdaltmode_vdm_task.h"
#include "cy_pdaltmode_mngr.h"
#include "cy_pdaltmode_dp_sid.h"
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
#include "cy_pdutils.h"
#include "cy_app_billboard.h"
#include "cy_usbpd_config_table.h"
#include "cy_usb_dev.h"
#include "cy_app_timer_id.h"

extern cy_stc_usb_init_ctxt_t *glUsbInitCtx;

/* Update the BOS descriptor with the alternate mode status */
void bb_update_bos_descriptor (void * context)
{
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)context;
    cy_stc_usbpd_context_t *ptrUsbPdContext = ptrAltModeContext->pdStackContext->ptrUsbPdContext;
    uint8_t *ptr = NULL;
    uint8_t *tmp_ptr = NULL;
    uint16_t length = 0u;
    const cy_stc_bb_settings_t * bb_cfg;
#if BILLBOARD_1_2_2_SUPPORT
    uint8_t num_alt_modes;
#endif /* BILLBOARD_1_2_2_SUPPORT */

    bb_cfg = pd_get_ptr_bb_tbl(ptrUsbPdContext);

#if BILLBOARD_1_2_2_SUPPORT
    num_alt_modes = *((uint8_t *)ptrUsbPdContext->cfg_table + bb_cfg->bb_bos_dscr_offset + CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_NUM_ALT_MODE_OFFSET);
#endif /* BILLBOARD_1_2_2_SUPPORT */
    
    ptr = ((uint8_t *)(&glUsbInitCtx->usbDevice->bosDescriptor[0]));

    if ((bb_cfg->bb_bos_dscr_offset != 0xFFFFu) && (ptrUsbPdContext->cfg_table != NULL))
    {

        /* Calculate BOS descriptor address (config. table start address + BOS descriptor offset) */
        tmp_ptr = (uint8_t *)ptrUsbPdContext->cfg_table + bb_cfg->bb_bos_dscr_offset;

        length = CY_PDUTILS_MAKE_WORD(tmp_ptr[3], tmp_ptr[2]);
        memcpy(ptr, tmp_ptr, length);

    }

    /* Update the status of the alternate mode */
    memcpy((uint8_t *)(&ptr[CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_ALT_STATUS_OFFSET]),
            (uint8_t *)(&ptrAltModeContext->billboard.altStatus),
            sizeof(ptrAltModeContext->billboard.altStatus));

    ptr[CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_ADD_INFO_OFFSET] = ptrAltModeContext->billboard.bbAddInfo;

#if BILLBOARD_1_2_2_SUPPORT
    if(ptrAltModeContext->altModeAppStatus->usb4Supp)
    {
        /* Store the EUDO information in the BOS descriptor */
        memcpy((uint8_t *)&ptr[CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_MODE0_INFO_OFFSET + (num_alt_modes * CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_MODE_INFO_SIZE) + CY_PDALTMODE_BILLBOARD_USB_AUM_CAP_DSCR_ALT_MODE_VDO_OFFSET], (uint8_t *)&ptrAltModeContext->altModeAppStatus->eudo.val, 4);

    }
#endif /* BILLBOARD_1_2_2_SUPPORT */

}

cy_en_pdaltmode_billboard_status_t Cy_App_Usb_BbInit(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_en_pdaltmode_billboard_status_t status = CY_PDALTMODE_BILLBOARD_STAT_SUCCESS;
    cy_stc_usbpd_context_t *ptrUsbPdContext = ptrAltModeContext->pdStackContext->ptrUsbPdContext;
    const cy_stc_bb_settings_t * bb_cfg = pd_get_ptr_bb_tbl(ptrAltModeContext->pdStackContext->ptrUsbPdContext);

    if (ptrAltModeContext->billboard.state != CY_PDALTMODE_BB_STATE_DEINITED)
    {
        return CY_PDALTMODE_BILLBOARD_STAT_BUSY;
    }

    if ((bb_cfg->bb_bos_dscr_offset != 0xFFFFu) && (ptrUsbPdContext->cfg_table != NULL))
    {
        ptrAltModeContext->billboard.numAltModes = *((uint8_t *)ptrUsbPdContext->cfg_table + \
                                         bb_cfg->bb_bos_dscr_offset + CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_NUM_ALT_MODE_OFFSET);
    }
    else
    {
        ptrAltModeContext->billboard.numAltModes = glUsbInitCtx->usbDevice->bosDescriptor[CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_NUM_ALT_MODE_OFFSET];
    }

    ptrAltModeContext->billboard.type = CY_PDALTMODE_BB_TYPE_INTERNAL;
    ptrAltModeContext->billboard.state = CY_PDALTMODE_BB_STATE_DISABLED;
    ptrAltModeContext->billboard.usbPort = ptrAltModeContext->pdStackContext->port;
    Cy_PdAltMode_Billboard_UpdateAllStatus(ptrAltModeContext, CY_PDALTMODE_BILLBOARD_ALT_MODE_STATUS_INIT_VAL);

    ptrAltModeContext->billboard.updateBosDescrCbk = bb_update_bos_descriptor;

    return status;
}

bool Cy_App_Usb_BbIsIdle(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    if ((ptrAltModeContext->billboard.queueEnable != false) ||
            (ptrAltModeContext->billboard.queueDisable != false))
    {
        return false;
    }

    return true;
}

#if CY_APP_BB_DISABLE_SUPPORT
void bb_off_timer_cb(cy_timer_id_t id,  void *context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;
    (void)id;

    if(ptrAltModeContext->billboard.timeout == 0u)
    {
        /* Indicates the port is disabled */
        bb_disable(context, false);
    }
    else
    {
        /* Continue the timer until the required delay is achieved */
        uint32_t timeout = ptrAltModeContext->billboard.timeout;

        if(timeout > BB_OFF_TIMER_MAX_INTERVAL)
        {
            timeout = BB_OFF_TIMER_MAX_INTERVAL;
        }
        ptrAltModeContext->billboard.timeout -= timeout;

        Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_BB_OFF_TIMER),
                timeout, bb_off_timer_cb);
    }
}

static void bb_off_timer_start(void *context)
{
    uint32_t timeout = 0u;
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    /* Disable the billboard interface after the specified timeout if the alt. mode status is success. */
    if (ptrAltModeContext->billboard.bbAddInfo == 0)
    {
        if (timeout < BB_OFF_TIMER_MIN_VALUE)
        {
            timeout = BB_OFF_TIMER_MIN_VALUE;
        }

        /* Convert time to milliseconds */
        timeout *= 1000u;
        ptrAltModeContext->billboard.timeout = timeout;

        /* Ensure that the timeout does not exceed parameter boundary */
        if(timeout > BB_OFF_TIMER_MAX_INTERVAL)
        {
            timeout = BB_OFF_TIMER_MAX_INTERVAL;
        }
        ptrAltModeContext->billboard.timeout -= timeout;

        Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_BB_OFF_TIMER),
                timeout, bb_off_timer_cb);
    }
    else
    {
        ptrAltModeContext->billboard.timeout = 0u;
    }
}
#endif /* CY_APP_BB_DISABLE_SUPPORT */

cy_en_pdaltmode_billboard_status_t Cy_App_Usb_BbFlashingCtrl(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool enable)
{
#if (CY_APP_FLASHING_MODE_USB_ENABLE != 0)
    uint8_t state;

    /* Change only if the block is initialized and not in flashing mode */
    if ((ptrAltModeContext->billboard.state <= CY_PDALTMODE_BB_STATE_DISABLED) ||
            (ptrAltModeContext->billboard.flashing_mode == enable))
    {
        return BB_STAT_NOT_READY;
    }

    /* Load the information and clear any pending enable/disable */
    state = Cy_SysLib_EnterCriticalSection();
    ptrAltModeContext->billboard.queueEnable = false;
    ptrAltModeContext->billboard.queueDisable = false;
    if (enable == false)
    {
#if CY_APP_BB_DISABLE_SUPPORT
        /* Now start the billboard expiry timer if required */
        bb_off_timer_start(port);
#endif /* CY_APP_BB_DISABLE_SUPPORT */
    }
    else
    {
#if (CY_APP_I2CM_BRIDGE_ENABLE)
        /* Do this only if not in bridge mode */
        if (ptrAltModeContext->billboard.usb_i2cm_mode == false)
#endif /* (CY_APP_I2CM_BRIDGE_ENABLE) */
        {
            ptrAltModeContext->billboard.state = BB_STATE_LOCKED;
            /* Now stop the billboard expiry timer */
            Cy_PdUtils_SwTimer_Stop(ptrAltModeContext->pdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrAltModeContext->pdStackContext,CY_APP_BB_OFF_TIMER));
            ptrAltModeContext->billboard.timeout = 0;
        }
    }
    ptrAltModeContext->billboard.flashing_mode = enable;
    Cy_SysLib_ExitCriticalSection(state);
#endif /* (CY_APP_FLASHING_MODE_USB_ENABLE != 0) */

    (void)ptrAltModeContext;
    (void)enable;

    return CY_PDALTMODE_BILLBOARD_STAT_SUCCESS;
}

#if (CY_APP_I2CM_BRIDGE_ENABLE)
cy_en_pdaltmode_billboard_status_t bb_bridge_ctrl(cy_stc_pdaltmode_context_t *ptrAltModeContext, bool enable)
{
    uint8_t state;
    cy_en_pdaltmode_billboard_status_t status = CY_PDALTMODE_BILLBOARD_STAT_SUCCESS;

    /* Queue an enable only if the block is initialized, not in flashing mode. */
    if ((ptrAltModeContext->billboard.state == CY_PDALTMODE_BB_STATE_DEINITED) ||
            (ptrAltModeContext->billboard.flashing_mode != false) ||
            (ptrAltModeContext->billboard.usb_i2cm_mode == enable))
    {
        return BB_STAT_NOT_READY;
    }

    /* Load the information and clear any pending enable/disable */
    state = Cy_SysLib_EnterCriticalSection();
    if (enable == false)
    {
        /* This is no different from the forced disable */
        bb_disable(port, true);
    }
    else
    {
        ptrAltModeContext->billboard.state = BB_STATE_LOCKED;
        /* Now stop the billboard expiry timer */
        Cy_PdUtils_SwTimer_Stop(ptrAltModeContext->pdStackContext->ptrTimerContext, CY_APP_GET_TIMER_ID(ptrAltModeContext->pdStackContext,CY_APP_BB_OFF_TIMER));
        ptrAltModeContext->billboard.timeout = 0;
        ptrAltModeContext->billboard.queue_i2cm_enable = true;
        ptrAltModeContext->billboard.queueDisable = true;
    }
    Cy_SysLib_ExitCriticalSection(state);

    return status;
}

uint8_t *usb_i2cm_get_ep0_buffer(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    return ptrAltModeContext->billboard.ep0_buffer;
}
#endif /* (CY_APP_I2CM_BRIDGE_ENABLE) */

/*
 * Internal functions do not validate the parameters. Caller is expected
 * to ensure that the parameters are valid. The function does the actual
 * start of the USB module based on the request queued. This function is
 * expected to be called only from the Cy_App_Usb_BbTask() function.
 */
static cy_en_pdaltmode_billboard_status_t bb_start(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_en_pdaltmode_billboard_status_t status = CY_PDALTMODE_BILLBOARD_STAT_SUCCESS;
    Cy_App_Usb_Enable ();
    ptrAltModeContext->billboard.state = CY_PDALTMODE_BB_STATE_BILLBOARD;
    ptrAltModeContext->billboard.usbConfigured = false;

#if CY_APP_BB_DISABLE_SUPPORT
    /* Start the billboard interface disable timer */
    bb_off_timer_start(context);
#endif /* CY_APP_BB_DISABLE_SUPPORT */

    /* Clear the queue flag */
    ptrAltModeContext->billboard.queueEnable = false;

    return status;
}

/*
 * Internal functions do not validate the parameters. Caller is expected
 * to ensure that the parameters are valid. The function does the actual
 * disable of the USB module based on the request queued. This function is
 * expected to be called only from the Cy_App_Usb_BbTask() function.
 */
static cy_en_pdaltmode_billboard_status_t bb_stop(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    cy_en_pdaltmode_billboard_status_t status = CY_PDALTMODE_BILLBOARD_STAT_SUCCESS;

    Cy_App_Usb_Disable();
    ptrAltModeContext->billboard.state = CY_PDALTMODE_BB_STATE_DISABLED;
    ptrAltModeContext->billboard.usbConfigured = false;
    ptrAltModeContext->billboard.queueDisable = false;

    /* Start the timer for delaying the next start */
    Cy_PdUtils_SwTimer_Start (ptrAltModeContext->pdStackContext->ptrTimerContext, ptrAltModeContext->pdStackContext,
            CY_APP_GET_TIMER_ID(ptrAltModeContext->pdStackContext, CY_APP_BB_ON_TIMER), CY_APP_BB_ON_TIMER_PERIOD, NULL);
    return status;
}

void Cy_App_Usb_BbTask(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    uint32_t state;

    if (ptrAltModeContext->billboard.state == CY_PDALTMODE_BB_STATE_DEINITED)
    {
        return;
    }

    state = Cy_SysLib_EnterCriticalSection();
    if (ptrAltModeContext->billboard.queueDisable != false)
    {
        if (ptrAltModeContext->billboard.state > CY_PDALTMODE_BB_STATE_DISABLED)
        {
            bb_stop(ptrAltModeContext);
        }
#if (CY_APP_I2CM_BRIDGE_ENABLE)
        /* Update state to locked state to prevent any race condition */
        if (ptrAltModeContext->billboard.queue_i2cm_enable != false)
        {
            ptrAltModeContext->billboard.state = BB_STATE_LOCKED;
        }
#endif /* (CY_APP_I2CM_BRIDGE_ENABLE) */
    }
#if (CY_APP_I2CM_BRIDGE_ENABLE)
    /*
     * If this is a I2C bridge mode request, then start the bridge mode
     * after the required restart delay is imposed.
     */
    if ((ptrAltModeContext->billboard.queue_i2cm_enable != false) &&
            (cy_sw_timer_is_running(ptrPdStackContext->ptrTimerContext, CY_APP_BB_ON_TIMER) == false))
    {
        /* Start the enumeration */
        if (bb_start(context) == CY_PDALTMODE_BILLBOARD_STAT_SUCCESS)
        {
            ptrAltModeContext->billboard.state = BB_STATE_LOCKED;
            ptrAltModeContext->billboard.usb_i2cm_mode = true;
            ptrAltModeContext->billboard.queue_i2cm_enable = false;
        }
    }
#endif /* (CY_APP_I2CM_BRIDGE_ENABLE) */
    if (ptrAltModeContext->billboard.queueEnable != false)
    {
        /* Disable the billboard interface to present the update */
        if (ptrAltModeContext->billboard.state > CY_PDALTMODE_BB_STATE_DISABLED)
        {
            bb_stop(ptrAltModeContext);
        }
        /* Wait for the ON delay timer to expire */
        if (Cy_PdUtils_SwTimer_IsRunning(ptrAltModeContext->pdStackContext->ptrTimerContext, CY_APP_BB_ON_TIMER) == false)
        {
            if (ptrAltModeContext->billboard.state == CY_PDALTMODE_BB_STATE_DISABLED)
            {
                bb_start(ptrAltModeContext);
            }
            else
            {
                /* Interface is in another mode and cannot be started */
                ptrAltModeContext->billboard.queueEnable = false;
            }
        }
    }

    Cy_SysLib_ExitCriticalSection(state);

#if (CY_APP_I2CM_BRIDGE_ENABLE)
    /* Invoke the bridge tasks */
    if (ptrAltModeContext->billboard.usb_i2cm_mode != false)
    {
        usb_i2cm_task();
    }
#endif /* (CY_APP_I2CM_BRIDGE_ENABLE) */
}

uint8_t *Cy_App_Usb_BbGetVersion(void)
{
    /* No version associated with internal Billboard */
    return NULL;
}

cy_en_pdaltmode_billboard_status_t Cy_App_Usb_BbBindToPort (cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    (void) ptrAltModeContext;

    /* Nothing to do for internal Billboard cases */
    return CY_PDALTMODE_BILLBOARD_STAT_SUCCESS;
}

void Cy_App_Usb_BbUpdateSelfPwrStatus (cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t self_pwrd)
{
    (void) ptrAltModeContext;
    (void) self_pwrd;

    /* Nothing to do for internal Billboard cases */
}

#endif /* (CCG_BB_ENABLE != 0) */

/* [] END OF FILE */

