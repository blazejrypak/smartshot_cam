/* ----------------------------------------------------------------------------
 * Copyright (c) 2020 Semiconductor Components Industries, LLC (d/b/a
 * ON Semiconductor), All Rights Reserved
 *
 * This code is the property of ON Semiconductor and may not be redistributed
 * in any form without prior written permission from ON Semiconductor.
 * The terms of use and warranty for this code are covered by contractual
 * agreements between ON Semiconductor and the licensee.
 *
 * This is Reusable Code.
 * ------------------------------------------------------------------------- */

/**
 * @file app_trace.h
 *
 * Contains all routines related to terminal output and real-time event
 * recording.
 */

#ifndef APP_TRACE_H
#define APP_TRACE_H

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>

#include <onsemi_smartshot_config.h>

#if (CFG_SMARTSHOT_TRACE_ENABLED == 1)
#   include <SEGGER_RTT.h>
#   include <SEGGER_SYSVIEW.h>
#   include <smartshot_trace.h>
#endif /* if (CFG_SMARTSHOT_TRACE_ENABLED == 1) */

/* ----------------------------------------------------------------------------
 * Defines
 * ------------------------------------------------------------------------- */

#if (CFG_SMARTSHOT_TRACE_ENABLED == 1)

/** The application name to be displayed in SystemView */
#define APP_SYSVIEW_APP_NAME        CFG_APP_NAME

/** Frequency of the timestamp. Must match SEGGER_SYSVIEW_Conf.h */
#define APP_SYSVIEW_TIMESTAMP_FREQ  (SystemCoreClock)

/** System Frequency. SystemcoreClock is used in most CMSIS compatible
 * projects.
 */
#define APP_SYSVIEW_CPU_FREQ        (SystemCoreClock)

/** The lowest RAM address used for IDs (pointers) */
#define APP_SYSVIEW_RAM_BASE        ((0x10000000))

/** Pseudo task id that is active when CPU is executing code. */
extern const uint32_t app_main_task_id;

/**
 * Pseudo task id that is used while the whole system is in deep sleep mode
 * or is preparing to enter into it.
 */
extern const uint32_t app_sleep_task_id;

/**
 * Used for first time initialization of trace module after restart.
 *
 * @pre
 * SystemView is configured to use the SYSCTRL->SYSCLK_CNT register value as
 * timestamp.
 */
void APP_TRACE_INIT(void);

/** Called before entering into deep sleep mode. */
void APP_TRACE_SLEEP(void);

/**
 * Resume trace after deep sleep wake up.
 * Adjust timebase counter for time spent in sleep.
 */
void APP_TRACE_RESUME(void);

/**
 * @returns
 * true when SystemView trace is active and the recording buffer is full. <br>
 * false otherwise.
 */
static inline bool APP_TRACE_OVERFLOW(void)
{
    /* 0: Disabled, 1: Enabled, (2: Dropping) */
    return (SEGGER_SYSVIEW_IsStarted() == 2);
}

/**
 * Custom application provided timestamp function for SystemView.
 */
uint32_t SEGGER_SYSVIEW_X_GetTimestamp(void);

#else /* #ifndef CFG_SMARTSHOT_TRACE_ENABLED*/

#define APP_TRACE_INIT()
#define APP_TRACE_SLEEP()
#define APP_TRACE_RESUME()
#define APP_TRACE_OVERFLOW() (false)

#endif /* ifdef CFG_SMARTSHOT_TRACE_ENABLED */


#ifdef __cplusplus
}
#endif

#endif    /* APP_TRACE_H_ */
