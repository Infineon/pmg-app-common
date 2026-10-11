/***************************************************************************//**
* \file cy_app_bb_internal.c
* \version 3.0
*
* \brief
* Implements functions for the billboard control interface
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

#if (CCG_BB_ENABLE != 0)
#include "cy_pdstack_common.h"
#include "cy_pdstack_dpm.h"
#include "cy_app_config.h"
#include "cy_app_vdm.h"
#include "cy_app.h"
#include "cy_app_config.h"
#include "cy_app_internal.h"
#include "cy_app_usb.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_gpio.h"
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
#include "cy_pdaltmode_defines.h"
#include "cy_pdaltmode_billboard.h"
#include "cy_pdaltmode_mngr.h"
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
#include "cy_pdutils.h"
#include "cy_app_billboard.h"
#include "cy_usbpd_config_table.h"
#include "cy_usb_dev.h"
#include "cy_app_timer_id.h"

/* Update the BOS descriptor with the alternate mode status */
static void bb_update_bos_descriptor (void * context)
{
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)context;
    cy_stc_usbpd_context_t *ptrUsbPdContext = ptrAltModeContext->pdStackContext->ptrUsbPdContext;
    uint8_t *ptr = NULL;
    uint8_t *tmp_ptr = NULL;
    uint16_t length = 0u;
    const cy_stc_bb_settings_t * bb_cfg;
#if BILLBOARD_1_2_2_SUPPORT
    uint8_t num_alt_modes = 0u;
#endif /* BILLBOARD_1_2_2_SUPPORT */

    bb_cfg = pd_get_ptr_bb_tbl(ptrUsbPdContext);

#if BILLBOARD_1_2_2_SUPPORT
    if (ptrUsbPdContext->cfg_table != NULL)
    {
        num_alt_modes = *((uint8_t *)ptrUsbPdContext->cfg_table + bb_cfg->bb_bos_dscr_offset + CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_NUM_ALT_MODE_OFFSET);
    }   
#endif /* BILLBOARD_1_2_2_SUPPORT */
    
    ptr = ((uint8_t *)(&glUsbInitCtx->usbDevice->bosDescriptor[0]));

    if ((bb_cfg->bb_bos_dscr_offset != 0xFFFFu) && (ptrUsbPdContext->cfg_table != NULL))
    {

        /* Calculate BOS descriptor address (config. table start address + BOS descriptor offset) */
        tmp_ptr = (uint8_t *)ptrUsbPdContext->cfg_table + bb_cfg->bb_bos_dscr_offset;

        length = CY_PDUTILS_MAKE_WORD(tmp_ptr[3], tmp_ptr[2]);
        (void)memcpy(ptr, tmp_ptr, length);

    }

    /* Update the status of the alternate mode */
    (void)memcpy((uint8_t *)(&ptr[CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_ALT_STATUS_OFFSET]),
            (uint8_t *)(&ptrAltModeContext->billboard.altStatus),
            sizeof(ptrAltModeContext->billboard.altStatus));

    ptr[CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_ADD_INFO_OFFSET] = ptrAltModeContext->billboard.bbAddInfo;

#if BILLBOARD_1_2_2_SUPPORT
    if(ptrAltModeContext->altModeAppStatus->usb4Supp)
    {
        /* Store the EUDO information in the BOS descriptor */
        (void)memcpy((uint8_t *)&ptr[(uint32_t)CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_MODE0_INFO_OFFSET + (num_alt_modes * (uint32_t)CY_PDALTMODE_BILLBOARD_USB_BOS_DSCR_MODE_INFO_SIZE) + (uint32_t)CY_PDALTMODE_BILLBOARD_USB_AUM_CAP_DSCR_ALT_MODE_VDO_OFFSET], (uint8_t *)&ptrAltModeContext->altModeAppStatus->eudo.val, 4u);

    }
#endif /* BILLBOARD_1_2_2_SUPPORT */

}

/**
 * @brief Initialize the Billboard module
 *
 * The API initializes the Billboard module. This is mainly a software state
 * machine initialization. The USB device is not enabled at this point. The API
 * helps to cleanup previous state information.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 *
 * @return Status of the call
 */
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
    (void)Cy_PdAltMode_Billboard_UpdateAllStatus(ptrAltModeContext, CY_PDALTMODE_BILLBOARD_ALT_MODE_STATUS_INIT_VAL);

    ptrAltModeContext->billboard.updateBosDescrCbk = &bb_update_bos_descriptor;

    return status;
}

/**
 * @brief Function returns whether the billboard module is idle or not
 *
 * This function indicates whether there are any pending tasks or not. This
 * function can be invoked before device enters Sleep mode to check if it is
 * allowed. But idle condition does not allow Deep Sleep entry. For this,
 * the application is expected to use the Cy_App_Usb_Sleep() function.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 *
 * @return true = billboard module is idle; false = billboard module is busy.
 */
bool Cy_App_Usb_BbIsIdle(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    bool localQueueEnable = ptrAltModeContext->billboard.queueEnable;
    bool localQueueDisable = ptrAltModeContext->billboard.queueDisable;
    return !(localQueueEnable || localQueueDisable);
}

#if CY_APP_BB_DISABLE_SUPPORT
/**
 * @brief Timer callback to turn-off billboard interface in case of Billboard flashing
 * support is enabled.
 *
 * @param id Timer ID associated with Timer callback.
 * @param context Context associated with Timer callback.
 *
 * @return None
 */
static void bb_off_timer_cb(cy_timer_id_t id,  void *context)
{
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;
    CY_UNUSED_PARAMETER(id);

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

        (void)Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_BB_OFF_TIMER),
                timeout, bb_off_timer_cb);
    }
}

/**
 * @brief Function to start turn-off billboard timer in case of Billboard flashing
 * support is enabled.
 *
 * @param context Context to be used for Timer callback.
 *
 * @return None
 */
static void bb_off_timer_start(void *context)
{
    uint32_t timeout = 0u;
    cy_stc_pdstack_context_t *ptrPdStackContext = (cy_stc_pdstack_context_t *)context;
    cy_stc_pdaltmode_context_t *ptrAltModeContext = (cy_stc_pdaltmode_context_t *)ptrPdStackContext->ptrAltModeContext;

    /* Disable the billboard interface after the specified timeout if the alt. mode status is success. */
    if (ptrAltModeContext->billboard.bbAddInfo == 0u)
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

        (void)Cy_PdUtils_SwTimer_Start(ptrPdStackContext->ptrTimerContext, ptrPdStackContext,
                CY_APP_GET_TIMER_ID(ptrPdStackContext, CY_APP_BB_OFF_TIMER),
                timeout, bb_off_timer_cb);
    }
    else
    {
        ptrAltModeContext->billboard.timeout = 0u;
    }
}
#endif /* CY_APP_BB_DISABLE_SUPPORT */

/**
 * @brief Function requests for flashing mode operation
 *
 * The function enables/disables the billboard interface in USB flashing mode.
 * This should be invoked only if the solution requires to start
 * the flashing mode. This request shall prevent the billboard expiry on enable
 * and restart the billboard expiry on disable as required.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 * @param enable Enable/disable control. true = Enable, false = Disable.
 *
 * @return Status of the call
 */
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
        /* Now start the billboard expire timer if required */
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
            ptrAltModeContext->billboard.timeout = 0u;
        }
    }
    ptrAltModeContext->billboard.flashing_mode = enable;
    Cy_SysLib_ExitCriticalSection(state);
#endif /* (CY_APP_FLASHING_MODE_USB_ENABLE != 0) */

    CY_UNUSED_PARAMETER(ptrAltModeContext);
    CY_UNUSED_PARAMETER(enable);

    return CY_PDALTMODE_BILLBOARD_STAT_SUCCESS;
}

#if (CY_APP_I2CM_BRIDGE_ENABLE)
/**
 * @brief Returns pointer to EP buffer associated with USB interface.
 *
 * @param ptrAltModeContext Pointer to Altmode context.
 *
 * @return pointer to EP buffer.
 */
uint8_t *usb_i2cm_get_ep0_buffer(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    return ptrAltModeContext->billboard.ep0_buffer;
}
#endif /* (CY_APP_I2CM_BRIDGE_ENABLE) */

/**
 * @brief This function enables Billboard interface in the USB. This function is
 * expected to be called only from the Cy_App_Usb_BbTask() function.
 *
 * When billboard is enabled or disabled, USB interface requires re-enumeration.
 * USB re-enumeration requires minimum time duration between USB interface disable and enable.
 *
 * @param ptrAltModeContext Pointer to Altmode context.
 *
 * Note: Internal functions do not validate the parameters. Caller is expected
 * to ensure that the parameters are valid.
 */
static void bb_start(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    /* Enable/Disable BB interface in USB descriptors based on bbAddInfo value. */
    glUsbInitCtx->update_usb_descriptors(0u != ptrAltModeContext->billboard.bbAddInfo);

    /* Enable USB interface. */
    Cy_App_Usb_Enable ();
    ptrAltModeContext->billboard.state = CY_PDALTMODE_BB_STATE_BILLBOARD;
    ptrAltModeContext->billboard.usbConfigured = false;

#if CY_APP_BB_DISABLE_SUPPORT
    /* Start the billboard interface disable timer */
    bb_off_timer_start(context);
#endif /* CY_APP_BB_DISABLE_SUPPORT */

    /* Clear the queue flag */
    ptrAltModeContext->billboard.queueEnable = false;
}

/**
 * @brief This function disables Billboard interface in the USB. This function is
 * expected to be called only from the Cy_App_Usb_BbTask() function.
 *
 * When billboard is enabled or disabled, USB interface requires re-enumeration.
 * USB re-enumeration requires minimum time duration between USB interface disable and enable.
 *
 * @param ptrAltModeContext Pointer to Altmode context.
 *
 * Note: Internal functions do not validate the parameters. Caller is expected
 * to ensure that the parameters are valid.
 */
static void bb_stop(cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    /* Disable USB interface. */
    Cy_App_Usb_Disable();

    ptrAltModeContext->billboard.state = CY_PDALTMODE_BB_STATE_DISABLED;
    ptrAltModeContext->billboard.usbConfigured = false;
    ptrAltModeContext->billboard.queueDisable = false;

    /* Start the timer for delaying the next start */
    (void)Cy_PdUtils_SwTimer_Start (ptrAltModeContext->pdStackContext->ptrTimerContext, ptrAltModeContext->pdStackContext,
            CY_APP_GET_TIMER_ID(ptrAltModeContext->pdStackContext, CY_APP_BB_ON_TIMER), CY_APP_BB_ON_TIMER_PERIOD, NULL);
}

/**
 * @brief Billboard module task function
 *
 * The function implements the billboard state machine and needs to be invoked
 * in the main task loop. The task handler allows deferring interrupts and
 * improves interrupt latency of the system.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 *
 * @return None
 */
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
            (Cy_PdUtils_SwTimer_IsRunning(ptrAltModeContext->pdStackContext->ptrTimerContext, CY_APP_BB_ON_TIMER) == false))
    {
        /* Start the enumeration */
        bb_start(context);
        ptrAltModeContext->billboard.state = BB_STATE_LOCKED;
        ptrAltModeContext->billboard.usb_i2cm_mode = true;
        ptrAltModeContext->billboard.queue_i2cm_enable = false;
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
        if (Cy_PdUtils_SwTimer_IsRunning(ptrAltModeContext->pdStackContext->ptrTimerContext, (uint16_t)CY_APP_BB_ON_TIMER) == false)
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

/**
 * @brief Function that returns the billboard firmware version information
 *
 * @return Pointer to buffer containing billboard firmware version
 */
uint8_t *Cy_App_Usb_BbGetVersion(void)
{
    /* No version associated with internal Billboard */
    return NULL;
}

/**
 * @brief Bind the billboard function to a specific port. This will be required in
 * cases where a single billboard device is used across multiple PD ports. The binding
 * will be vacated when bb_disable is called.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 *
 * @return CCG_STAT_SUCCESS if the binding is successful, error code otherwise.
 */
cy_en_pdaltmode_billboard_status_t Cy_App_Usb_BbBindToPort (cy_stc_pdaltmode_context_t *ptrAltModeContext)
{
    CY_UNUSED_PARAMETER(ptrAltModeContext);

    /* Nothing to do for internal Billboard cases */
    return CY_PDALTMODE_BILLBOARD_STAT_SUCCESS;
}

/**
 * @brief Update the self powered status bit reported in the billboard device's
 * configuration descriptor.
 *
 * @param ptrAltModeContext Pointer to the AltMode context
 * @param self_pwrd The power-up status (0 = not self powered, other values mean self powered).
 *
 * @return None
 */
void Cy_App_Usb_BbUpdateSelfPwrStatus (cy_stc_pdaltmode_context_t *ptrAltModeContext, uint8_t self_pwrd)
{
    CY_UNUSED_PARAMETER(ptrAltModeContext);
    CY_UNUSED_PARAMETER(self_pwrd);

    /* Nothing to do for internal Billboard cases */
}

#endif /* (CCG_BB_ENABLE != 0) */

/* [] END OF FILE */

