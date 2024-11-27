/***************************************************************************//**
* \file cy_app_moisture_detect.h
* \version 2.0
*
* \brief
* Defines data structures and function prototypes associated with
* moisture detection.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_MOISTURE_DETECT_H_
#define _CY_APP_MOISTURE_DETECT_H_

#include "cy_pdstack_common.h"
#include "cy_usbpd_common.h"
#include "cy_usbpd_bch.h"
#include "cy_app_config.h"

#if CY_CORROSION_MITIGATION_ENABLE

/** Moisture present debounce count */
#define APP_MOISTURE_DET_FILTER_CNT                     (2u)

/** Moisture absence debounce count */
#define APP_MOISTURE_ABS_FILTER_CNT                     (4u)

/** Threshold voltage (in mV) for moisture detection on SBU1 line. */
#define SBU1_MOISTURE_DETECT_THRESHOLD                  (2700u)
/** Threshold voltage (in mV) for moisture detection on SBU2 line. */
#define SBU2_MOISTURE_DETECT_THRESHOLD                  (300u)

#if (defined(CY_DEVICE_PMG1S3))
#if PMG1_PD_DUALPORT_ENABLE
#define SBU1_PORT_FOR_DETECTION    GPIO_PRT6
#define SBU1_NUM_FOR_DETECTION     0U
#define SBU2_PORT_FOR_DETECTION    GPIO_PRT6
#define SBU2_NUM_FOR_DETECTION     1U
#endif /* PMG1_PD_DUALPORT_ENABLE */
#endif  /* (defined(CY_DEVICE_PMG1S3)) */
    
/**
 * @brief This function initializes variables for moisture detection.
 *
 * @param context PDStack context.
 * @return None.
 */
void Cy_App_MoistureDetect_Init(cy_stc_pdstack_context_t *context);

/**
 * @brief This function checks the presence of moisture.
 *
 * @param context USBPD context.
 * @return True if moisture is present. 
 */
bool Cy_App_MoistureDetect_IsMoisturePresent(cy_stc_usbpd_context_t *context);

/**
 * @brief This function calls functions to start moisture detection.
 *
 * @param context PDStack context.
 * @return None.
 */
void Cy_App_MoistureDetect_Run(cy_stc_pdstack_context_t *context);

#endif /* CY_CORROSION_MITIGATION_ENABLE */
#endif /* _CY_APP_MOISTURE_DETECT_H_ */
