/***************************************************************************//**
* \file cy_app_instrumentation.c
* \version 3.0
*
* \brief
* Implements functions to monitor CPU resource (execution time
* and stack usage) usage
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

#include <stddef.h>
#include "cy_app_config.h"
#include "cy_app_instrumentation.h"
#include "cy_pdl.h"


/* Run-time stack lower limit defined in linker script */
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 8.6', 1, 'Stack limit symbol is defined in linker script.');
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 8.4', 1, 'Stack limit symbol is defined in linker script.');
#if defined(__ARMCC_VERSION)
    extern unsigned long Image$$ARM_LIB_STACK$$ZI$$Base;
#elif defined (__ICCARM__)
    #pragma language=extended
    #pragma segment="CSTACK"
#else
    extern uint32_t __StackLimit;
#endif /* defined(__ARMCC_VERSION) */

static cy_app_instrumentation_cb_t gl_instrumentation_cb = NULL;

#if CY_APP_RESET_ON_ERROR_ENABLE

/* RAM based signature and offset used to check whether reset data is valid */
#define RESET_DATA_VALID_OFFSET         (0u)
#define RESET_DATA_VALID_SIG            (0xDEADBEEFu)

/* RAM based signature and offset used to check whether watchdog reset has happened */
#define WATCHDOG_RESET_OFFSET           (1u)
#define WATCHDOG_RESET_SIG              (0xC003300Cu)

/* RAM offset where the watchdog reset count is maintained */
#define RESET_COUNT_OFFSET              (2u)

/* RAM based signature and offset used to check whether hard fault has occurred */
#define HARDFAULT_RESET_OFFSET           (1u)
#define HARDFAULT_RESET_SIG              (0xD003300Cu)

/* Size of the reset tracking data structure in DWORDs */
#define RESET_DATA_STRUCT_SIZE          (3u)

/* Address of the run-time instrumentation data structure */
#if defined(__ARMCC_VERSION)
    volatile uint32_t *gl_runtime_data_addr = (uint32_t *)&Image$$ARM_LIB_STACK$$ZI$$Base;
#elif defined (__ICCARM__)
    #pragma language=extended
    #pragma segment="CSTACK"
    volatile uint32_t *gl_runtime_data_addr = (uint32_t volatile *)__sfb( "CSTACK" );
#else
    volatile uint32_t *gl_runtime_data_addr = &__StackLimit;
#endif /* defined(__ARMCC_VERSION) */

/* Variable used to identify whether main loop has been run */
static volatile uint32_t gl_main_loop_delay = 0u;

/* Pointer to the Timer context */
static cy_stc_pdutils_sw_timer_t *glPtrTimerCtx;

/* Timer callback to reset device if main loop has not been run as expected */
static void watchdog_timer_cb (
    cy_timer_id_t id,           /**< Timer ID for which callback is being generated */
    void *callbackContext)       /**< Timer module context */
{
    CY_UNUSED_PARAMETER(callbackContext);
    CY_UNUSED_PARAMETER(id);

    /*
     * It is possible that this timer is the only reason for the device to wake from sleep.
     * Hence allow three consecutive timer expires before resetting the device.
     */
    gl_main_loop_delay++;
    if (gl_main_loop_delay >= 3u)
    {
        /* Store the reset signature into RAM */
        gl_runtime_data_addr[WATCHDOG_RESET_OFFSET] = WATCHDOG_RESET_SIG;
        NVIC_SystemReset ();
    }

    /* Start the timer again */
    (void)Cy_PdUtils_SwTimer_Start (glPtrTimerCtx, NULL, (uint16_t)CY_PDUTILS_WATCHDOG_TIMER,
            CY_APP_WATCHDOG_RESET_PERIOD_MS, &watchdog_timer_cb);
}

#endif /* CY_APP_RESET_ON_ERROR_ENABLE */

#if CY_APP_STACK_USAGE_CHECK_ENABLE

/*
 * Minimum run-time stack usage value. This many bytes at the top of the stack
 * will not be tracked for usage.
 */
#define MIN_STACK_USAGE     (256u)

/* Signature used to track stack usage */
#define STACK_UNUSED_SIG    (0x00555500u)

/* Address of the bottom location of the run-time stack */
uint32_t *gStackBottom  = (uint32_t *)CYDEV_SRAM_BASE;
volatile uint16_t gMinStackMargin   = 0u;

#endif /* CY_APP_STACK_USAGE_CHECK_ENABLE */

void Cy_App_Instrumentation_Init(cy_stc_pdutils_sw_timer_t *ptrTimerContext)
{
    uint32_t wdr_cnt = 0u;

    /* Added to avoid compiler warning if all features are disabled */
    CY_UNUSED_PARAMETER(wdr_cnt);

#if CY_APP_RESET_ON_ERROR_ENABLE
    /* Store the timer context. */
    glPtrTimerCtx = ptrTimerContext;
#else
    CY_UNUSED_PARAMETER(ptrTimerContext);
#endif /* CY_APP_RESET_ON_ERROR_ENABLE */

#if CY_APP_STACK_USAGE_CHECK_ENABLE
    uint32_t *addr_p;

    /* Store the stack bottom location */
#if defined(__ARMCC_VERSION)
    gStackBottom = (uint32_t *)&Image$$ARM_LIB_STACK$$ZI$$Base;
#elif defined (__ICCARM__)
    gStackBottom = (uint32_t *)__sfb( "CSTACK" );
#else
    gStackBottom = (uint32_t *)&__cy_stack_limit;
#endif /* defined(__ARMCC_VERSION) */

#if CY_APP_RESET_ON_ERROR_ENABLE
    /* If we have watchdog reset tracking enabled, the lowest twelve bytes of stack cannot be used. */
    gStackBottom += RESET_DATA_STRUCT_SIZE;
#endif /* CY_APP_RESET_ON_ERROR_ENABLE */

    /* Fill the stack memory with unused signature */
    for (addr_p = gStackBottom; addr_p < (uint32_t *)((CYDEV_SRAM_BASE + CYDEV_SRAM_SIZE) - MIN_STACK_USAGE); addr_p++)
    {
        *addr_p = STACK_UNUSED_SIG;
    }

    /* Initialize the stack margin value */
    gMinStackMargin = (uint16_t)((uint32_t)addr_p - (uint32_t)gStackBottom);

#endif /* CY_APP_STACK_USAGE_CHECK_ENABLE */

#if CY_APP_RESET_ON_ERROR_ENABLE
    if (gl_runtime_data_addr[RESET_DATA_VALID_OFFSET] == RESET_DATA_VALID_SIG)
    {
        wdr_cnt = gl_runtime_data_addr[RESET_COUNT_OFFSET];
        if (gl_runtime_data_addr[WATCHDOG_RESET_OFFSET] == WATCHDOG_RESET_SIG)
        {
            if(gl_instrumentation_cb != NULL)
            {
                gl_instrumentation_cb(0u, CY_APP_INST_EVT_WDT_RESET);
            }
            wdr_cnt++;
        }
        else if(gl_runtime_data_addr[HARDFAULT_RESET_OFFSET] == HARDFAULT_RESET_SIG)
        {
            if (gl_instrumentation_cb != NULL)
            {
                gl_instrumentation_cb(0u, CY_APP_INST_EVT_HARD_FAULT);
            }
        }
        else
        {
            /* Do nothing */
        }
    }
    else
    {
        if (gl_instrumentation_cb != NULL)
        {
            gl_instrumentation_cb(0u, CY_APP_INST_EVT_POWER_CYCLE);
        }
    }

    /*
     * Store the reset data valid signature and current reset count.
     * Also clear the reset detected signature.
     */
    gl_runtime_data_addr[RESET_DATA_VALID_OFFSET] = RESET_DATA_VALID_SIG;
    gl_runtime_data_addr[WATCHDOG_RESET_OFFSET]   = 0u;
    gl_runtime_data_addr[HARDFAULT_RESET_OFFSET] = 0u;
    gl_runtime_data_addr[RESET_COUNT_OFFSET]      = wdr_cnt;
#endif /* CY_APP_RESET_ON_ERROR_ENABLE */
}

void Cy_App_Instrumentation_Start(void)
{
#if CY_APP_RESET_ON_ERROR_ENABLE
    /* Start the timer used for watchdog reset */
    (void)Cy_PdUtils_SwTimer_Start (glPtrTimerCtx, NULL,  (uint16_t)CY_PDUTILS_WATCHDOG_TIMER,
            CY_APP_WATCHDOG_RESET_PERIOD_MS, &watchdog_timer_cb);
#endif /* CY_APP_RESET_ON_ERROR_ENABLE */

#if CY_APP_WATCHDOG_HARDWARE_RESET_ENABLE
    /*
     * Enable WDT hardware reset.
     * WDT interrupt flag is expected to be cleared by software timer module
     * (At the least CY_PDUTILS_WATCHDOG_TIMER is active always).
     * If WDT interrupt handler is not executed because of CPU lock up and
     * the WDT interrupt flag is not cleared for the three consecutive
     * interrupts, a hardware reset is triggered for the recovery.
     */
    Cy_WDT_Enable();
#endif /* CY_APP_WATCHDOG_HARDWARE_RESET_ENABLE */
}

void Cy_App_Instrumentation_Task(void)
{
#if CY_APP_STACK_USAGE_CHECK_ENABLE
    uint32_t *addr_p = gStackBottom;
#endif /* CY_APP_STACK_USAGE_CHECK_ENABLE */

#if CY_APP_RESET_ON_ERROR_ENABLE
    /* Clear the variable to indicate main loop has been run */
    gl_main_loop_delay = 0u;
#endif /* CY_APP_RESET_ON_ERROR_ENABLE */

#if CY_APP_STACK_USAGE_CHECK_ENABLE
    for (addr_p = gStackBottom; addr_p < (uint32_t *)((CYDEV_SRAM_BASE + CYDEV_SRAM_SIZE) - MIN_STACK_USAGE); addr_p++)
    {
        if (*addr_p != STACK_UNUSED_SIG)
        {
            break;
        }
    }

    /* Calculate the minimum stack availability margin and update debug register */
    gMinStackMargin = GET_MIN(gMinStackMargin, ((uint32_t)addr_p - (uint32_t)gStackBottom));
#endif /* CY_APP_STACK_USAGE_CHECK_ENABLE */
}

CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 17.11', 1, 'This function will not return');
CY_MISRA_DEVIATE_BLOCK_START('MISRA C-2012 Rule 8.4', 1, 'This is the handler that overrides original weak one');
void HardFault_Handler(void)
{
#if CY_APP_RESET_ON_ERROR_ENABLE
    /* Store the reset signature into RAM */
    gl_runtime_data_addr[HARDFAULT_RESET_OFFSET] = HARDFAULT_RESET_SIG;
    NVIC_SystemReset ();
#endif /* CY_APP_RESET_ON_ERROR_ENABLE */
}
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 8.4');
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 17.11');

void Cy_App_Instrumentation_RegisterCb(cy_app_instrumentation_cb_t cb)
{
    if(cb != NULL)
    {
        gl_instrumentation_cb = cb;
    }
}

uint32_t Cy_App_Instrumentation_GetWdtResetCount(void)
{
#if CY_APP_RESET_ON_ERROR_ENABLE
    return gl_runtime_data_addr[RESET_COUNT_OFFSET];
#else
    return 0u;
#endif /* CY_APP_RESET_ON_ERROR_ENABLE */

}

CY_MISRA_BLOCK_END('MISRA C-2012 Rule 8.4');
CY_MISRA_BLOCK_END('MISRA C-2012 Rule 8.6');

/* [] END OF FILE */
