/***************************************************************************//**
* \file cy_app_pdo.h
* \version 2.0
*
* \brief
* Define functions associated source capability (PDO) evaluation
* functions
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_PDO_H_
#define _CY_APP_PDO_H_

/**
* \addtogroup group_pmg_app_common_pdo
* \{
* The PDO handler provides APIs to evaluate the source capabilities advertised by
* port partner and identify the optimal power contract to be entered into and
* evaluate a PD request data object and determine whether to accept or reject
* the request.
* These APIs are registered as application callbacks and used by the PDStack middleware library.
*
*********************************************************************************
* \section section_pmg_app_common_pdo_config Configuration considerations
********************************************************************************
*
* The following steps describe the usage of these APIs:
*
* 1. Include cy_app_pdo.h to get access to all functions and other declarations.
*    \snippet pdo_snippet.c snippet_pdo_include
*    \n
* 2. Initialize the functions to the application callback structure.
*    \snippet pdo_snippet.c snippet_pdo_cbk_structure
*    \n
* 3. Register the application callback to the PDStack middleware library.
*    Refer to the \ref section_pmg_app_common_quick_start section.
*
* \defgroup group_pmg_app_common_pdo_functions Functions
*/

/** \} group_pmg_app_common_pdo */

/*******************************************************************************
 * Header files including
 ******************************************************************************/

#include "cy_pdstack_common.h"

/*****************************************************************************
 * Macro definitions
 *****************************************************************************/

#define APP_PPS_SNK_CONTRACT_PERIOD             (9000u)
/**< Period after which a PPS sink repeats PD contract attempts. This should be faster than once in 10 s. */

#define APP_PPS_SNK_CONTRACT_RETRY_PERIOD       (5u)
/**< Period after which a failed PPS sink re-contract attempt will be retried */

/**
* \addtogroup group_pmg_app_common_pdo_functions
* \{
*/

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/

/**
 * @brief This function is called by the PD stack to allow the application
 * logic to evaluate the source capabilities received from the port partner
 * and generate the desired request. The request object is expected to be
 * passed back to the stack through the app_resp_handler() callback.
 *
 * The default implementation of this function matches each of the received
 * source PDOs against the active sink capabilities; and then selects the
 * source PDO that can deliver the maximum power to the system as a sink.
 *
 * @param context Pointer to the PDStack context
 * @param srcCap Pointer to PD packet which contains source capabilities
 * @param app_resp_handler Application handler callback function
 *
 * @return None
 */
void Cy_App_Pdo_EvalSrcCap(cy_stc_pdstack_context_t* context, const cy_stc_pdstack_pd_packet_t* srcCap, cy_pdstack_app_resp_cbk_t app_resp_handler) ;

/**
 * @brief This function is called by the PD stack to allow the application
 * to evaluate a power request data object received from the port partner and
 * decide whether it should be satisfied. The response to the request should
 * be passed back to the stack through the app_resp_handler() callback.
 *
 * @param context Pointer to the PDStack context
 * @param rdo The request data object received
 * @param app_resp_handler Application handler callback function
 *
 * @return None
 */
void Cy_App_Pdo_EvalRdo(cy_stc_pdstack_context_t* context, cy_pd_pd_do_t rdo, cy_pdstack_app_resp_cbk_t app_resp_handler) ;

/** \} group_pmg_app_common_pdo_functions */

#endif /* _CY_APP_PDO_H_ */

/* [] END OF FILE */

