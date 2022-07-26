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
 * @file app_rtc.h
 *
 * Routines to keep track of time using the RTC module of RSL10 as a free
 * running counter.
 */

#ifndef APP_RTC_H
#define APP_RTC_H

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/

#include <stdint.h>

/* ----------------------------------------------------------------------------
 * Define declaration
 * ------------------------------------------------------------------------- */


/* ----------------------------------------------------------------------------
 * Global variables declaration
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Forward declaration
 * ------------------------------------------------------------------------- */

/**
 * Initialize RTC counter using XTAL32K as clock source.
 *
 * THe RTC is configured as free-running count-down timer that works
 * both in active and deep sleep modes of RSL10.
 *
 * RTC Alarm functionality is disabled and RTC will not be allowed to wake-up
 * RSL10 from deep sleep modes.
 *
 * @pre
 * XTAL32K oscillator is ready.
 */
void APP_RTC_Start(void);

/** Returns number of RTC ticks since last counter reload.
 *
 * RTC counter is set to reload every RTC_CNT_START_4294967295 SLOWCLK ticks.
 * This corresponds to ~36.4 hours with 32.768 kHz SLOWCLK.
 *
 * @return
 * Number of RTC ticks since last counter reload.
 */
uint32_t APP_RTC_GetTicks(void);

/**
 * Get current system time in milliseconds.
 *
 * This function converts the overflowing 32-bit RTC counter at 32768 ticks per
 * second into overflowing 32-bit counter at ~1000 ticks per second.
 *
 * RTC counter overflows approx. every 36 hours and the converted counter
 * overflows every approx. 49 days.
 *
 * \pre
 * This function is called at least every 18 hours to update internal time
 * keeping variables.
 *
 * \returns
 * System time in milliseconds.
 */
uint32_t APP_RTC_GetTimeMs(void);

uint64_t APP_RTC_GetTimeUs(void);

#ifdef __cplusplus
}
#endif

#endif    /* APP_RTC_H_ */

/* ----------------------------------------------------------------------------
 * End of File
 * ------------------------------------------------------------------------- */
