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
 *
 * ------------------------------------------------------------------------- */

/**
 * @file app_trace.c
 *
 */

#include <stdio.h>
#include <stdarg.h>
#include <rsl10.h>

#include "app_trace.h"
#include "app_rtc.h"

/* ----------------------------------------------------------------------------
 * Real-Time trace support
 * ------------------------------------------------------------------------- */

#if (CFG_SMARTSHOT_TRACE_ENABLED == 1)

/**
 * Sends complete system description when trace is started.
 */
static void APP_TRACE_SendSystemDesc(void)
{
    SEGGER_SYSVIEW_SendSysDesc("N="APP_SYSVIEW_APP_NAME",O=NoOS,D=RSL10");

    /* Send names of interrupt handlers to be identified by SystemView. */
#if (CFG_SMARTSHOT_PIR_INT_CFG_NUM == 0)
    SEGGER_SYSVIEW_SendSysDesc("I#32=DIO0 (PIR)");
#endif /* (CFG_SMARTSHOT_PIR_INT_CFG_NUM == 0) */
#if (CFG_SMARTSHOT_ISP_INT_GFC_NUM == 1)
    SEGGER_SYSVIEW_SendSysDesc("I#33=DIO1 (ISP)");
#endif /* (CFG_SMARTSHOT_ISP_INT_GFC_NUM == 1) */
    SEGGER_SYSVIEW_SendSysDesc("I#43=I2C");
}

static U64 APP_TRACE_GetTime(void)
{
    return APP_RTC_GetTimeUs();
}

/**
 * Dummy Task ID for Main application task.
 *
 * This task is activated while application is executing code in the main loop.
 * If main loop contains WFI or WFE instructions it should report IDLE task
 * before calling WFI or WFE.
 *
 * This task represents the amount of time the application spends executing code
 * compared to idle time when Cortex core is suspended waiting for interrupts.
 */
const uint32_t app_main_task_id = (uint32_t) &app_main_task_id;

/**
 * Dummy Task ID for Sleep duration.
 *
 * This Task is activated during the last pre-sleep application step.
 * Then during wake-up the application switches back to Main task as soon as
 * the clock dividers are set and trace timestamp counter is adjusted for deep
 * sleep duration.
 *
 * The time this task is active roughly represents the amount of time spent
 * in deep sleep.
 */
const uint32_t app_sleep_task_id = (uint32_t) &app_sleep_task_id;

/** Stores system time in us when switching to deep sleep mode. */
static uint64_t app_trace_sleep_enter_time;

/** Stores value of SYSTICK counter when switching to deep sleep mode. */
static uint32_t app_trace_sleep_enter_counter;

/** Statically allocated buffer used to store recorded trace messages. */
static uint8_t app_trace_up_buffer[CFG_SMARTSHOT_TRACE_SYSVIEW_BUFFER_SIZE];

/**
 * Sends information about all OS tasks running in the system.
 */
static void APP_TRACE_SendTaskList(void)
{
    SEGGER_SYSVIEW_TASKINFO info;
    info.Prio = 1; /* dummy */
    info.StackBase = DRAM0_TOP - 6*4; /* dummy */
    info.StackSize = 1024; /* dummy */

    /* Send main task info. */
    info.TaskID = app_main_task_id;
    info.sName = "Main";
    SEGGER_SYSVIEW_SendTaskInfo(&info);

    /* Sent sleep task info. */
    info.TaskID = app_sleep_task_id;
    info.sName = "Sleep";
    SEGGER_SYSVIEW_SendTaskInfo(&info);
}

static SEGGER_SYSVIEW_OS_API app_os_api =
{
 APP_TRACE_GetTime,
 APP_TRACE_SendTaskList
};

void APP_TRACE_INIT(void)
{
    /* Start SYSCLK cycle counter. */
    SYSCTRL_CNT_CTRL->CNT_CLEAR_ALIAS = 1;
    SYSCTRL_CNT_CTRL->CNT_START_ALIAS = 1;

    SEGGER_SYSVIEW_Init(APP_SYSVIEW_TIMESTAMP_FREQ, APP_SYSVIEW_CPU_FREQ,
            &app_os_api, APP_TRACE_SendSystemDesc);

    /* Override the default SysView buffer that is set to very low size in the
     * pre-compiled System View component provided by Board Support component.
     *
     * Used to benefit from optimized pre-compiled trace routines while still
     * having the flexibility to adjust trace buffer size depending on
     * application needs.
     */
    SEGGER_RTT_ConfigUpBuffer(SEGGER_SYSVIEW_GetChannelID(), "SysView",
            app_trace_up_buffer, CFG_SMARTSHOT_TRACE_SYSVIEW_BUFFER_SIZE,
            SEGGER_RTT_MODE_NO_BLOCK_SKIP);

    SEGGER_SYSVIEW_SetRAMBase(APP_SYSVIEW_RAM_BASE);

#if (CFG_SMARTSHOT_TRACE_AUTO_START == 1)
    /* Start recording events immediately. */
    SEGGER_SYSVIEW_Start();
    SEGGER_SYSVIEW_OnTaskStartExec(app_main_task_id);
#endif /* if (CFG_SMARTSHOT_TRACE_AUTO_START == 1) */
}

void APP_TRACE_SLEEP(void)
{
    SEGGER_SYSVIEW_OnTaskStartExec(app_sleep_task_id);
    SEGGER_SYSVIEW_RecordSystime();

    /* Save current RTC time and systick counter to restore timebase after
     * wake-up.
     */
    app_trace_sleep_enter_time = APP_RTC_GetTimeUs();
    app_trace_sleep_enter_counter = SYSCTRL->SYSCLK_CNT;
}

void APP_TRACE_RESUME(void)
{
    /* Restore SYSCLK counter and adjust for sleep duration. */
    uint64_t rtc_time = APP_RTC_GetTimeUs();
    SYSCTRL->SYSCLK_CNT = app_trace_sleep_enter_counter;
    SYSCTRL->SYSCLK_CNT += ((rtc_time - app_trace_sleep_enter_time) * (SystemCoreClock / 1E6));
    SYSCTRL_CNT_CTRL->CNT_START_ALIAS = 1;

    SEGGER_SYSVIEW_OnTaskStartExec(app_main_task_id);

    /* Record RTC time as system time to provide absolute time reference for
     * events recorded during this wake-up cycle.
     */
    SEGGER_SYSVIEW_RecordSystime();
}

uint32_t SEGGER_SYSVIEW_X_GetTimestamp(void)
{
    /* Workaround for activity counter being disabled when it overflows.
     * It takes the system 8 minutes and 55 seconds to reach overflow at
     * 8MHz SYSCLK.
     *
     * As a workaround define the timestamp only as 31 bits wide (instead of 32
     * bits) in SEGGER_SYSVIEW_Conf.h and manually clear the MSB when it is set
     * so that the counter never overflows.
     */

    /* Use bitband access to poll and clear the MSB of the SYSCTRL_SYSCLK_CNT
     * register.
     */
    if (*((uint32_t*) SYS_CALC_BITBAND(0x40000034, 31)))
    {
        *((uint32_t*) SYS_CALC_BITBAND(0x40000034, 31)) = 0;
    }

    return SYSCTRL->SYSCLK_CNT;
}

#endif /* QTE_TRACE_ENABLED */
