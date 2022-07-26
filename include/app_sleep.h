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
 * @file app_sleep.h
 *
 * This file contains routines related to entry and wake-up from deep sleep
 * power mode.
 */

#ifndef APP_SLEEP_H
#define APP_SLEEP_H

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/

#include <rsl10.h>

/* ----------------------------------------------------------------------------
 * Define declaration
 * ------------------------------------------------------------------------- */

/**
 * DMA channel number used to backup RF registers during sleep environment
 * initialization.
 */
#define DMA_CHAN_SLP_WK_RF_REGS_COPY   (0)

/* ----------------------------------------------------------------------------
 * Global variables declaration
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * Forward declaration
 * ------------------------------------------------------------------------- */

/**
 * Immediately enter into deep sleep mode with power down configuration.
 *
 * After calling this function the RLS10 is put into deep sleep mode in
 * wake-up from FLASH configuration:
 *
 * - No RAM memory retention.
 * - Pad Retention enabled. Pads must be configured before calling.
 * - BLE Base Band timer is disabled.
 * - RTC is disabled.
 * - Low power oscillators XTAL32K and RCOSC are both disabled.
 * - Clock detection is disabled.
 *
 * The only way to wake-up RSL10 from this sleep mode is by PAD_RESET which is
 * usually connected to reset button of the evaluation kit.
 *
 * Upon wake-up the device boots from FLASH and starts program execution.
 *
 * @note
 * Optionally the routine can be modified to enable wake-up on external trigger
 * caused by rising/falling edge of DIO][0-4] pads.
 * This option can be utilized on boards with wake-up capable signals (button,
 * sensor interrupt) connected to DIO[0-4] pads.
 *
 * @note
 * Additional power can be saved by disabling Pad Retention setting if hardware
 * design allows for DIO pads of RSL10 to be in undefined state.
 *
 * @return
 * This function does not return.
 */
void APP_EnterFlashSleep(void);

/**
 * Switch application control to FOTA application to perform firmware update.
 *
 * After FOTA mode exits the application is restarted, launching updated
 * firmware if available.
 */
void APP_EnterFotaMode(void);

/**
 * Configure sleep mode environment and start STANDBYCLK clock.
 *
 * @pre
 * All interrupts are disabled.
 *
 */
void APP_SleepModeConfigure(void);

/**
 * Puts RSL10 into deep sleep mode with wake-up from RAM based on configuration
 * provided by @ref APP_SleepModeConfigure
 *
 * @returns
 * This function does not return.
 */
void APP_EnterSleep(void);

/**
 * Start up XTAL32K oscillator to be used as STANDBYCLK clock source.
 *
 * @note
 * Oscillator startup typically takes around ~500 ms with the parameters used
 * in this function.
 *
 * @pre
 * Systick interrupt is disabled in NVIC.
 */
void APP_XTAL32K_Start(void);

/**
 * First code executed after wake-up.
 *
 * Restores stack pointer and then calls @ref Wakeup_From_Sleep_Application
 */
void Wakeup_From_Sleep_Application_asm(void);

/**
 * Restore system states from retention RAM and continue application from
 * flash.
 *
 * This function shall be placed into RAM as it is executed before flash access
 * is available after wake-up.
 */
void Wakeup_From_Sleep_Application(void) __attribute__ ((section(".app_wakeup")));

/**
 * Restore application state and continue execution.
 */
void ContinueApplication(void);

#ifdef __cplusplus
}
#endif

#endif    /* APP_SLEEP_H_ */

/* ----------------------------------------------------------------------------
 * End of File
 * ------------------------------------------------------------------------- */
