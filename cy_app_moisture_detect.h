/***************************************************************************//**
* \file cy_app_moisture_detect.h
* \version 3.0
*
* \brief
* Defines data structures and function prototypes associated with
* moisture detection.
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

#ifndef CY_APP_MOISTURE_DETECT_H
#define CY_APP_MOISTURE_DETECT_H

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
 */
void Cy_App_MoistureDetect_Run(cy_stc_pdstack_context_t *context);

#endif /* CY_CORROSION_MITIGATION_ENABLE */
#endif /* CY_APP_MOISTURE_DETECT_H */
