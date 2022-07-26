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
 * @file app_rtc.c
 *
 * Routines to keep track of time using the RTC module of RSL10 as a free
 * running counter.
 */

#include <rsl10.h>

#include "app_rtc.h"

void APP_RTC_Start(void)
{
    /* Disable all RTC interrupts. */
    NVIC_DisableIRQ(RTC_ALARM_IRQn);
    NVIC_DisableIRQ(RTC_CLOCK_IRQn);
    NVIC_ClearPendingIRQ(RTC_ALARM_IRQn);
    NVIC_ClearPendingIRQ(RTC_CLOCK_IRQn);

    /* Set maximum reload value for RTC counter.
     * Reset RTC counter.
     * Disable RTC alarm -> RTC is not able to wake-up RSL10 from deep sleep.
     * Set XTAL32K oscillator as clock source (32.768 kHz).
     */
    Sys_RTC_Config(RTC_CNT_START_4294967295,
            RTC_RESET | RTC_ALARM_DISABLE | RTC_CLK_SRC_XTAL32K);

    /* Start counting. */
    Sys_RTC_Start(RTC_ENABLE);
}

uint32_t APP_RTC_GetTicks(void)
{
    uint32_t rtc_time = 0;

    /* Calculate time elapsed since last counter under-flow event. */
    rtc_time = RTC_CNT_START_4294967295 - Sys_RTC_Value();

    return rtc_time;
}

uint32_t APP_RTC_GetTimeMs(void)
{
    static uint32_t ms_time = 0;
    static uint32_t tick_checkpoint = 0;

    uint32_t now = APP_RTC_GetTicks();
    uint32_t diff = (now - tick_checkpoint) / 32;

    ms_time += diff;
    tick_checkpoint += (diff * 32);

    return ms_time;
}

uint64_t APP_RTC_GetTimeUs(void)
{
    static uint64_t us_time = 0;
    static uint32_t tick_checkpoint = 0;

    uint32_t now = APP_RTC_GetTicks();
    uint64_t diff = (uint64_t)(now - tick_checkpoint) * 30;

    us_time += diff;
    tick_checkpoint += (diff / 30);

    return us_time;
}

/* ----------------------------------------------------------------------------
 * End of File
 * ------------------------------------------------------------------------- */
