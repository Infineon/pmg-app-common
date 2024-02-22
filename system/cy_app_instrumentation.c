/***************************************************************************//**
* \file cy_app_instrumentation.c
* \version 1.0
*
* \brief
* Implements functions to monitor CPU resource (execution time
* and stack usage) usage
*
********************************************************************************
* \copyright
* Copyright 2024, Cypress Semiconductor Corporation (an Infineon company)
* or an affiliate of Cypress Semiconductor Corporation. All rights reserved.
* You may use this file only in accordance with the license, terms, conditions,
* disclaimers, and limitations in the end user license agreement accompanying
* the software package with which this file was provided.
*******************************************************************************/

#include <stddef.h>
#include "cy_app_config.h"
#include "cy_app_instrumentation.h"
#include "cy_pdl.h"
#include "cy_pdutils_sw_timer.h"
#include "cy_pdstack_common.h"

/* Run-time stack lower limit defined in linker script */
#if defined(__ARMCC_VERSION)
    extern unsigned long Image$$ARM_LIB_STACK$$ZI$$Base;
#elif defined (__ICCARM__)
    #pragma language=extended
    #pragma segment="CSTACK"
#else
    extern int __StackLimit;
#endif /* defined(__ARMCC_VERSION) */

cy_app_instrumentation_cb_t gl_instrumentation_cb = NULL;

/* Pointer to the Timer context */
static cy_stc_pdutils_sw_timer_t *glPtrTimerCtx;

#if CY_APP_RESET_ON_ERROR_ENABLE

/* RAM based signature and offset used to check whether reset data is valid */
#define RESET_DATA_VALID_OFFSET         (0)
#define RESET_DATA_VALID_SIG            (0xDEADBEEF)

/* RAM based signature and offset used to check whether watchdog reset has happened */
#define WATCHDOG_RESET_OFFSET           (1)
#define WATCHDOG_RESET_SIG              (0xC003300C)

/* RAM offset where the watchdog reset count is maintained */
#define RESET_COUNT_OFFSET              (2)

/* RAM based signature and offset used to check whether hard fault has occurred */
#define HARDFAULT_RESET_OFFSET           (1)
#define HARDFAULT_RESET_SIG              (0xD003300C)

/* Size of the reset tracking data structure in DWORDs */
#define RESET_DATA_STRUCT_SIZE          (3)

/* Address of the run-time instrumentation data structure */
#if defined(__ARMCC_VERSION)
    volatile uint32_t *gl_runtime_data_addr = (uint32_t *)&Image$$ARM_LIB_STACK$$ZI$$Base;
#elif defined (__ICCARM__)
    #pragma language=extended
    #pragma segment="CSTACK"
    volatile uint32_t *gl_runtime_data_addr = (uint32_t volatile *)__sfb( "CSTACK" );
#else
    volatile uint32_t *gl_runtime_data_addr = (uint32_t volatile *)&__StackLimit;
#endif /* defined(__ARMCC_VERSION) */

/* Variable used to identify whether main loop has been run */
volatile uint32_t gl_main_loop_delay = 0;

/* Margin (in ms) available until watchdog reset */
volatile uint16_t gl_min_reset_margin = CY_APP_WATCHDOG_RESET_PERIOD_MS;

/* Timer callback to reset device if main loop has not been run as expected */
void watchdog_timer_cb (
    cy_timer_id_t id,           /**< Timer ID for which callback is being generated */
    void *callbackContext)       /**< Timer module context */
{
    (void)callbackContext;
    (void)id;

    /*
     * It is possible that this timer is the only reason for the device to wake from sleep.
     * Hence allow three consecutive timer expires before resetting the device.
     */
    gl_main_loop_delay++;
    if (gl_main_loop_delay >= 3)
    {
        /* Store the reset signature into RAM */
        gl_runtime_data_addr[WATCHDOG_RESET_OFFSET] = WATCHDOG_RESET_SIG;
        NVIC_SystemReset ();
    }

    /* Start the timer again */
    Cy_PdUtils_SwTimer_Start (glPtrTimerCtx, NULL, CY_PDUTILS_WATCHDOG_TIMER,
            CY_APP_WATCHDOG_RESET_PERIOD_MS, watchdog_timer_cb);
}

#endif /* CY_APP_RESET_ON_ERROR_ENABLE */

#if CY_APP_STACK_USAGE_CHECK_ENABLE

/*
 * Minimum run-time stack usage value. This many bytes at the top of the stack
 * will not be tracked for usage.
 */
#define MIN_STACK_USAGE     (256u)

/* Signature used to track stack usage */
#define STACK_UNUSED_SIG    (0x00555500)

/* Address of the bottom location of the run-time stack */
uint32_t *gStackBottom  = (uint32_t *)CYDEV_SRAM_BASE;
volatile uint16_t gMinStackMargin   = 0;

#endif /* CY_APP_STACK_USAGE_CHECK_ENABLE */

void Cy_App_Instrumentation_Init(cy_stc_pdutils_sw_timer_t *ptrTimerContext)
{
    uint32_t wdr_cnt = 0;

    /* Added to avoid compiler warning if all features are disabled */
    (void)wdr_cnt;

    /* Store the timer context. */
    glPtrTimerCtx = ptrTimerContext;

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
                gl_instrumentation_cb(0, CY_APP_INST_EVT_WDT_RESET);
            }
            wdr_cnt++;
        }
        else if(gl_runtime_data_addr[HARDFAULT_RESET_OFFSET] == HARDFAULT_RESET_SIG)
        {
            if (gl_instrumentation_cb != NULL)
            {
                gl_instrumentation_cb(0, CY_APP_INST_EVT_HARD_FAULT);
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
            gl_instrumentation_cb(0, CY_APP_INST_EVT_POWER_CYCLE);
        }
    }

    /*
     * Store the reset data valid signature and current reset count.
     * Also clear the reset detected signature.
     */
    gl_runtime_data_addr[RESET_DATA_VALID_OFFSET] = RESET_DATA_VALID_SIG;
    gl_runtime_data_addr[WATCHDOG_RESET_OFFSET]   = 0;
    gl_runtime_data_addr[HARDFAULT_RESET_OFFSET] = 0;
    gl_runtime_data_addr[RESET_COUNT_OFFSET]      = wdr_cnt;
#endif /* CY_APP_RESET_ON_ERROR_ENABLE */
}

void Cy_App_Instrumentation_Start(void)
{
#if CY_APP_RESET_ON_ERROR_ENABLE
    /* Start the timer used for watchdog reset */
    Cy_PdUtils_SwTimer_Start (glPtrTimerCtx, NULL,  CY_PDUTILS_WATCHDOG_TIMER,
            CY_APP_WATCHDOG_RESET_PERIOD_MS, watchdog_timer_cb);
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
    gl_main_loop_delay = 0;
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

void HardFault_Handler(void)
{
#if CY_APP_RESET_ON_ERROR_ENABLE
    /* Store the reset signature into RAM */
    gl_runtime_data_addr[HARDFAULT_RESET_OFFSET] = HARDFAULT_RESET_SIG;
    NVIC_SystemReset ();
#endif /* CY_APP_RESET_ON_ERROR_ENABLE */
}

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
    return 0;
#endif /* CY_APP_RESET_ON_ERROR_ENABLE */

}

/* [] END OF FILE */
