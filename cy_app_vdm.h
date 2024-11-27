/***************************************************************************//**
* \file cy_app_vdm.h
* \version 2.0
*
* \brief
* Defines the data structures and function prototypes
* for the vendor defined message (VDM) handler.
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#ifndef _CY_APP_VDM_H_
#define _CY_APP_VDM_H_

/*******************************************************************************
 * Includes header files
 ******************************************************************************/

#include "cy_pdstack_common.h"
#if (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP)
#include "cy_pdaltmode_defines.h"
#endif /* (DFP_ALT_MODE_SUPP || UFP_ALT_MODE_SUPP) */
#include "cy_app.h"

/**
* \addtogroup group_pmg_app_common_vdm
* \{
*
* The vendor defined message (VDM) handler provides APIs and data structures to evaluate
* the VDM command and send response.
* These APIs are registered as application callbacks and used by the PDStack middleware library.
* 
* <b>Features:</b>
* * Evaluate VDM command
* * Initialize and send VDM response
*
*********************************************************************************
* \section section_pmg_app_common_vdm_config Configuration Considerations
********************************************************************************
*
* The following steps describe the usage of VDM handler APIs.
*
* 1. Include cy_app_vdm.h to get access to all functions and other declarations.
*    \snippet vdm_sut.c snippet_vdm_include
*    \n
* 2. Initialize the functions to the application callback structure.
*    \snippet vdm_sut.c snippet_vdm_cbk_structure
*    \n
* 3. Register the application callback to the PDStack middleware library.
*    See \ref section_pmg_app_common_quick_start section.
*    \n
* 4. Invoke API to initialize VDM response message.
*    \snippet vdm_sut.c snippet_vdm_data_init
*    where port0_app_params and port1_app_params are application-specific 
*    parameters for Port 0 and Port 1 respectively. See \ref cy_stc_app_params_t
*    for more information. 
*
* \defgroup group_pmg_app_common_vdm_macros Macros
* \defgroup group_pmg_app_common_vdm_functions Functions
*/
/** \} group_pmg_app_common_vdm */

/**
* \addtogroup group_pmg_app_common_vdm_macros
* \{
*/

/** Vendor defined object start index */ 
#define CY_APP_VDM_VDO_START_IDX                   (1u)
/** \} group_pmg_app_common_vdm_macros */

/*****************************************************************************
 * Global function declaration
 *****************************************************************************/
/**
* \addtogroup group_pmg_app_common_vdm_functions
* \{
*/

/**
 * @brief Store the VDM data from the configuration table
 *
 * Retrieves the VDM data (for CCG as UFP) in 
 * the configuration table and stores it in the runtime data structures.
 *
 * @param context Pointer to the PDStack context
 *
 * @param appParams Pointer to the application parameters
 *
 * @return None
 */
void Cy_App_Vdm_Init(cy_stc_pdstack_context_t * context, const cy_stc_app_params_t *appParams);

/**
 * @brief Analyses and processes the received VDM.
 * This function also makes a decision about necessity of response to the received
 * VDM.
 *
 * @param context Pointer to the PDStack context
 * @param vdm Pointer to PD packet which contains received VDM
 * @param vdm_resp_handler VDM handler callback function
 *
 * @return None
 */
void Cy_App_Vdm_EvalVdmMsg(cy_stc_pdstack_context_t * context, const cy_stc_pdstack_pd_packet_t *vdm,
        cy_pdstack_vdm_resp_cbk_t vdm_resp_handler);

/**
 * @brief Evaluates an Enter_USB request and reports whether it should be
 * accepted or rejected.
 *
 * @param context Pointer to the PDStack context
 * @param eudo Pointer to the Enter_USB packet
 * @param app_resp_handler Response callback through which the response is passed to the PDStack
 * @return None
 */
void Cy_App_Vdm_EvalEnterUsb(cy_stc_pdstack_context_t * context, const cy_stc_pdstack_pd_packet_t *eudo, cy_pdstack_app_resp_cbk_t app_resp_handler);

/** \} group_pmg_app_common_vdm_functions */

#endif /* _CY_APP_VDM_H_ */

/* [] END OF FILE */

