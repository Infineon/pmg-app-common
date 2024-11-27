/***************************************************************************//**
* \file cy_app_swap.h
* \version 2.0
*
* \brief
* Defines the function prototypes for handling of
* USB Power Delivery role Swap requests.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_SWAP_H_
#define _CY_APP_SWAP_H_

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
 * @return None
 */
void Cy_App_Swap_EvalDrSwap(cy_stc_pdstack_context_t * context, cy_pdstack_app_resp_cbk_t app_resp_handler);

/**
 * @brief Evaluates the power role swap request
 *
 * @param context Pointer to the PDStack context
 * @param app_resp_handler Application handler callback function
 *
 * @return None
 */
void Cy_App_Swap_EvalPrSwap(cy_stc_pdstack_context_t * context, cy_pdstack_app_resp_cbk_t app_resp_handler);

/**
 * @brief Evaluates VConn swap request
 *
 * @param context Pointer to the PDStack Context
 * @param app_resp_handler Application handler callback function
 *
 * @return None
 */
void Cy_App_Swap_EvalVconnSwap(cy_stc_pdstack_context_t * context, cy_pdstack_app_resp_cbk_t app_resp_handler);

#if (CY_PD_REV3_ENABLE || DOXYGEN)
/**
 * @brief Evaluates fast role swap request
 *
 * @param context Pointer to the PDStack context
 * @param app_resp_handler Application handler callback function
 *
 * @return None
 */
void Cy_App_Swap_EvalFrSwap(cy_stc_pdstack_context_t * context, cy_pdstack_app_resp_cbk_t app_resp_handler);

#endif /* (CY_PD_REV3_ENABLE || DOXYGEN) */ 

/** \} group_pmg_app_common_swap_functions */

#endif /* _CY_APP_SWAP_H_ */
/* [] END OF FILE */

