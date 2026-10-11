/***************************************************************************//**
* \file cy_app_swap.h
* \version 3.0
*
* \brief
* Defines the function prototypes for handling of
* USB Power Delivery role Swap requests.
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

#ifndef CY_APP_SWAP_H
#define CY_APP_SWAP_H

/*******************************************************************************
 * Header files including
 ******************************************************************************/

#include "cy_pdstack_common.h"

/**
* \addtogroup group_pmg_app_common_swap
* \{
* The swap handler provides APIs and data structures for evaluating the swap requests.
* These APIs are registered as application callbacks and used by the PDStack middleware library.
* 
* <b>Features:</b>
* * Data role swap
* * Power role swap
* * Fast role swap
* * VConn swap
*
*********************************************************************************
* \section section_pmg_app_common_swap_config Configuration considerations
********************************************************************************
*
* The following steps describe the usage of the swap handler APIs.
*
* 1. Include swap.h to get access to all functions and other declarations.
*    \snippet swap_sut.c snippet_swap_include
*    \n
* 2. Initialize the swap functions to the application callback structure.
*    \snippet swap_sut.c snippet_swap_cbk_structure
*    \n
* 3. Register the application callback to the PdStack middleware library.
*    Refer to the \ref section_pmg_app_common_quick_start section.
*
*
* \defgroup group_pmg_app_common_swap_functions Functions
*/
/** \} group_pmg_app_common_swap */


/**
* \addtogroup group_pmg_app_common_swap_functions
* \{
*/

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/
/**
 * @brief Evaluates dta role swap request
 *
 * @param context Pointer to the PDStack context
 * @param app_resp_handler Application handler callback function
 *
 */
void Cy_App_Swap_EvalDrSwap(cy_stc_pdstack_context_t * context, cy_pdstack_app_resp_cbk_t app_resp_handler);

/**
 * @brief Evaluates the power role swap request
 *
 * @param context Pointer to the PDStack context
 * @param app_resp_handler Application handler callback function
 *
 */
void Cy_App_Swap_EvalPrSwap(cy_stc_pdstack_context_t * context, cy_pdstack_app_resp_cbk_t app_resp_handler);

/**
 * @brief Evaluates VConn swap request
 *
 * @param context Pointer to the PDStack Context
 * @param app_resp_handler Application handler callback function
 *
 */
void Cy_App_Swap_EvalVconnSwap(cy_stc_pdstack_context_t * context, cy_pdstack_app_resp_cbk_t app_resp_handler);

#if (((!CY_PD_SOURCE_ONLY) && (!CY_PD_SINK_ONLY) && CY_PD_REV3_ENABLE && CY_PD_FRS_TX_ENABLE) || DOXYGEN)
/**
 * @brief Evaluates fast role swap request
 *
 * @param context Pointer to the PDStack context
 * @param app_resp_handler Application handler callback function
 *
 */
void Cy_App_Swap_EvalFrSwap(cy_stc_pdstack_context_t * context, cy_pdstack_app_resp_cbk_t app_resp_handler);

#endif /* (((!CY_PD_SOURCE_ONLY) && (!CY_PD_SINK_ONLY) && CY_PD_REV3_ENABLE && CY_PD_FRS_TX_ENABLE) || DOXYGEN) */ 

/** \} group_pmg_app_common_swap_functions */

#endif /* CY_APP_SWAP_H */
/* [] END OF FILE */

