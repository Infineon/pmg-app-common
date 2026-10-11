/***************************************************************************//**
* \file cy_app_pdo.h
* \version 3.0
*
* \brief
* Define functions associated source capability (PDO) evaluation
* functions
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

#ifndef CY_APP_PDO_H
#define CY_APP_PDO_H

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

#define CY_APP_RDO_EPR_SUPPORT_MASK             (0x80000000u)
/**< Mask for EPR support bit in RDO. */

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
 */
void Cy_App_Pdo_EvalRdo(cy_stc_pdstack_context_t* context, cy_pd_pd_do_t rdo, cy_pdstack_app_resp_cbk_t app_resp_handler) ;

/** \} group_pmg_app_common_pdo_functions */

#endif /* CY_APP_PDO_H */

/* [] END OF FILE */

