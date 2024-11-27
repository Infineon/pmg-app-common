/***************************************************************************//**
* \file cy_app_moisture_detect.c
* \version 2.0
*
* \brief
* Implements functions associated with corrosion mitigation.
* This only interacts with PDStack on detection of any kind of moisture.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include "cy_pdstack_dpm.h"
#include "cy_app_moisture_detect.h"
#include "cy_usbpd_vbus_ctrl.h"
#include "cy_app.h"
#include "cy_app_timer_id.h"

#if CY_CORROSION_MITIGATION_ENABLE

static uint8_t gl_moisture_present_filter_cnt[NO_OF_TYPEC_PORTS];
static uint8_t gl_moisture_absent_filter_cnt[NO_OF_TYPEC_PORTS];

void Cy_App_MoistureDetect_Init(cy_stc_pdstack_context_t * context)
{
    gl_moisture_present_filter_cnt[context->port] = 0u;
    gl_moisture_absent_filter_cnt[context->port] = 0u;
}

bool Cy_App_MoistureDetect_IsMoisturePresent(cy_stc_usbpd_context_t * context)
{
    bool ret = false;
    uint32_t intr_state = Cy_SysLib_EnterCriticalSection();

#if CY_APP_MOISTURE_DETECT_USING_DP_DM
#if (defined(CY_DEVICE_PMG1S3) && PMG1_PD_DUALPORT_ENABLE)
    if(context->port == TYPEC_PORT_0_IDX)
#endif /* (defined(CY_DEVICE_PMG1S3) && PMG1_PD_DUALPORT_ENABLE) */
    {
        /* Enables charger detect block. */
        Cy_USBPD_Bch_Phy_En(context);

        /* Enables pull-up on R_DP_UP. */
        Cy_USBPD_Bch_ApplyDpPu(context);

        /* Applies R_DM_DAT_LKG (311.54 - 454.86K). */
        Cy_USBPD_Bch_ApplyRdatLkgDm(context);

        /* Provides delay for voltage to settle. */
        Cy_SysLib_DelayUs(100);

        /* If there is any liquid then the voltage is observed on the DM line. */
        if(Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DM,
                CHGB_COMP_N_VREF, CHGB_VREF_0_325V, CHGB_COMP_NO_INTR) == true)
        {
            /* If it is true, then liquid is detected. */
            ret = true;
        }
        else
        {
            /* If there is any liquid then voltage is reduced on DP line */
            if(Cy_USBPD_Bch_Phy_Config_Comp(context, BC_CMP_0_IDX, CHGB_COMP_P_DP,
                    CHGB_COMP_N_VREF, CHGB_VREF_2_9V, CHGB_COMP_NO_INTR) == false)
            {
                ret = true;
            }
        }

        /* Restores everything back. */
        Cy_USBPD_Bch_Phy_RemoveTerm(context);
        Cy_USBPD_Bch_Phy_Dis(context);

        Cy_USBPD_Bch_RemoveDpPu(context);
    }
#else
    /* Moisture detection on CCG3PA is supported using DP/DM only. */
#if (!defined(CY_DEVICE_CCG3PA))
    /* Moisture detection using SBU. */
    uint8_t level;
    uint16_t voltage;
    bool adcRefVddd = context->adcRefVddd[CY_USBPD_ADC_ID_0];

    /* Connects SBU1 to AUXN and SBU2 to AUXP. */
    Cy_USBPD_Mux_SbuSwitchConfigure(context, CY_USBPD_SBU_CONNECT_AUX2, CY_USBPD_SBU_CONNECT_AUX1);
    Cy_USBPD_Mux_AuxTermConfigure(context, CY_USBPD_AUX_1_470K_PD_RESISTOR, CY_USBPD_AUX_2_100K_PU_RESISTOR);

    /* Connects SBU1 line to the AMUX-A. */
#if (defined(CY_DEVICE_PMG1S3))
    Cy_USBPD_Mux_SbuAdftEnable(context, CY_USBPD_SBU_ADFT_AUX1_SBU1);
#elif (defined(CY_DEVICE_CCG6))
    Cy_USBPD_Mux_SbuAdftEnable(context, CY_USBPD_SBU_ADFT_GND_SBU1_INT);
#elif (defined(CY_DEVICE_CCG3))
    uint32_t regVal = context->base->uvov_ctrl;
    /* Disconnects the UV/OV resistor divider from ADFT so that SBU voltage can be measured. */
    context->base->uvov_ctrl &= ~(PDSS_UVOV_CTRL_UVOV_ADFT_EN | PDSS_UVOV_CTRL_UVOV_ADFT_CTRL_MASK);
    Cy_SysLib_DelayUs (10);
    Cy_USBPD_Mux_SbuAdftEnable(context, CY_USBPD_SBU_ADFT_SBU1);
#endif /* (defined(CY_DEVICE_PMG1S3)) */

    if(adcRefVddd != true)
    {
        /* Sets the ADC reference voltage to VDDD. */
        Cy_USBPD_Adc_SelectVref(context, CY_USBPD_ADC_ID_0, CY_USBPD_ADC_VREF_VDDD);
    }

    Cy_SysLib_DelayUs(100);

    level = Cy_USBPD_Adc_Sample(context, CY_USBPD_ADC_ID_0, CY_USBPD_ADC_INPUT_AMUX_A);
    voltage = Cy_USBPD_Adc_LevelToVolt(context, CY_USBPD_ADC_ID_0, level);

    /* If the voltage is less than 2.7 V, then moisture is detected. */
    if(voltage < SBU1_MOISTURE_DETECT_THRESHOLD)
    {
        ret = true;
    }
    else
    {
        /* Connects SBU2 line to the AMUX-A. */
#if (defined(CY_DEVICE_PMG1S3))
        Cy_USBPD_Mux_SbuAdftEnable(context, CY_USBPD_SBU_ADFT_AUX2_SBU2);
#elif (defined(CY_DEVICE_CCG6))
        Cy_USBPD_Mux_SbuAdftEnable(context, CY_USBPD_SBU_ADFT_ISNK_OVP_SBU2_INT);
#elif (defined(CY_DEVICE_CCG3))
        Cy_USBPD_Mux_SbuAdftEnable(context, CY_USBPD_SBU_ADFT_SBU2);
#endif /* (defined(CY_DEVICE_PMG1S3)) */
        /** Allows for voltages to settle. */
        Cy_SysLib_DelayUs(20);
        level = Cy_USBPD_Adc_Sample(context, CY_USBPD_ADC_ID_0, CY_USBPD_ADC_INPUT_AMUX_A);
        voltage = Cy_USBPD_Adc_LevelToVolt(context, CY_USBPD_ADC_ID_0, level);
        /* If the voltage is greater than 0.3 V, then moisture is detected. */
        if(voltage > SBU2_MOISTURE_DETECT_THRESHOLD)
        {
            ret = true;
        }
    }

    if(adcRefVddd != true)
    {
        /* Sets the ADC reference to voltage from the RefGen block. */
        Cy_USBPD_Adc_SelectVref(context, CY_USBPD_ADC_ID_0, CY_USBPD_ADC_VREF_PROG);
    }
    /* Removes SBU to AUX connection and terminations on AUX lines. */
    Cy_USBPD_Mux_SbuSwitchConfigure(context, CY_USBPD_SBU_NOT_CONNECTED, CY_USBPD_SBU_NOT_CONNECTED);
    Cy_USBPD_Mux_AuxTermConfigure(context, CY_USBPD_AUX_NO_RESISTOR, CY_USBPD_AUX_NO_RESISTOR);
    /* Removes AMUX-A connection. */
    Cy_USBPD_Mux_SbuAdftDisable(context);
#if (defined(CY_DEVICE_CCG3))
    Cy_SysLib_DelayUs(10);
    /* Restores the original ADFT settings. */
    context->base->uvov_ctrl = regVal;
#endif /* (defined(CY_DEVICE_CCG3)) */
#endif /* (!defined(CY_DEVICE_CCG3PA)) */
#endif /* MOISTURE_DETECT_USING_DP_DM */
    Cy_SysLib_ExitCriticalSection(intr_state);
    return ret;
}

/**
 * @brief This function monitors DP and DM or SBU lines for moisture detection.
 *
 * @param ptrPdStackContext PDStack context.
 * @return None.
 */
static void Cy_App_MoistureDetect_MonitorDpDmSbu(cy_stc_pdstack_context_t *ptrPdStackContext)
{
    bool moisture_present;
    cy_stc_usbpd_context_t *context = ptrPdStackContext->ptrUsbPdContext;
    uint8_t port = ptrPdStackContext->port;

    moisture_present = Cy_App_MoistureDetect_IsMoisturePresent(context);
#if (defined(CY_DEVICE_CCG6) && CY_APP_MOISTURE_DETECT_USING_DP_DM)
    if(moisture_present != true)
    {
        /* Changes the polarity so that the other pair of DP/DM lines are used for moisture detection. */
        ptrPdStackContext->dpmConfig.polarity = !(ptrPdStackContext->dpmConfig.polarity);
        moisture_present = Cy_App_MoistureDetect_IsMoisturePresent(context);
        ptrPdStackContext->dpmConfig.polarity = !(ptrPdStackContext->dpmConfig.polarity);
    }
#endif /* (defined (CY_DEVICE_CCG6) &&  CY_APP_MOISTURE_DETECT_USING_DP_DM) */

    if(moisture_present == true)
    {
        gl_moisture_absent_filter_cnt[port] = 0u;
        gl_moisture_present_filter_cnt[port]++;

         if(gl_moisture_present_filter_cnt[port] >= APP_MOISTURE_DET_FILTER_CNT)
         {
             ptrPdStackContext->typecStat.moistureDetected = true;
             gl_moisture_present_filter_cnt[port] = 0u;
         }
    }
    else
    {
        gl_moisture_present_filter_cnt[port] = 0u;
        gl_moisture_absent_filter_cnt[port]++;
        if(gl_moisture_absent_filter_cnt[port] >= APP_MOISTURE_ABS_FILTER_CNT)
        {
            ptrPdStackContext->typecStat.moistureDetected = false;
            gl_moisture_absent_filter_cnt[port] = 0u;
        }
    }
}

void Cy_App_MoistureDetect_Run(cy_stc_pdstack_context_t * context)
{
    cy_pd_cc_state_t newState;

    if(Cy_PdUtils_SwTimer_IsRunning(context->ptrTimerContext, CY_APP_GET_TIMER_ID(context, CY_APP_MOISTURE_DETECT_TIMER_ID)))
    {
        return;
    }
    Cy_App_MoistureDetect_MonitorDpDmSbu(context);
    if(context->typecStat.moistureDetected == true)
    {
        if(context->typecStat.moisturePresent == false)
        {
            /* Notifies application layer about moisture status. */
            Cy_App_EventHandler(context, APP_EVT_CORROSION_FAULT, (const void *)&context->typecStat.moistureDetected);
            Cy_PdStack_Dpm_GoToErrorRecovery(context);
        }
        else
        {
           newState.state = 0;
           /* After one iteration, scan for active CC line and pull that down. */
           /* Scan CC1 with threshold vRa and vRdUsb. */
           newState.cc[0] = Cy_USBPD_TypeC_GetRpRdStatus(context->ptrUsbPdContext, CY_PD_CC_CHANNEL_1, 0);

           /* If CC1 status is not equal to RP_RA, then there is some voltage on that line and that is the active CC. */
           if(newState.cc[0] != (uint8_t)(CY_PD_RP_RA))
           {
               Cy_USBPD_TypeC_SetPolarity(context->ptrUsbPdContext , CY_PD_CC_CHANNEL_1);
           }
           else
           {
               /* If CC2 status is not equal to RP_RA, then there is some voltage on that line and that is the active CC. */
               /* Scan CC2 again with vRdusb and vRd1. The 5 A to determine the correct Rp value. */
               newState.cc[1] = Cy_USBPD_TypeC_GetRpRdStatus(context->ptrUsbPdContext, CY_PD_CC_CHANNEL_2, 0);

               if(newState.cc[1] != (uint8_t)(CY_PD_RP_RA))
               {
                   Cy_USBPD_TypeC_SetPolarity(context->ptrUsbPdContext , CY_PD_CC_CHANNEL_2);
               }
           }
       }
    }
    else
    {
        if(context->typecStat.moisturePresent == true)
        {
            /* Notifies the application layer about moisture status. */
            Cy_App_EventHandler(context, APP_EVT_CORROSION_FAULT, (const void *)&context->typecStat.moistureDetected);
            Cy_PdStack_Dpm_GoToErrorRecovery(context);
        }
    }
}
#endif /* CY_CORROSION_MITIGATION_ENABLE */
