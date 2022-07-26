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
 * @file app_sleep.c
 *
 * This file contains routines related to entry and wake-up from deep sleep
 * power mode.
 */

#include "app_sleep.h"
#include "app.h"

DEFINE_THIS_FILE_FOR_ASSERT;

/** Sleep mode environment structure used when entering sleep mode. */
static struct sleep_mode_env_tag app_sleep_mode_env;

/**
 * Start up XTAL32K oscillator to be used as STANDBYCLK clock source.
 *
 * @pre
 * Systick interrupt is disabled in NVIC.
 */
void APP_XTAL32K_Start(void)
{
    /* Enable XTAL32K oscillator amplitude control
     * Set XTAL32K load capacitance to 0x38: 22.4 pF
     * Enable XTAL32K oscillator
     * Set startup current to approx. 4 x 320nA for fastest startup
     */
    ACS->XTAL32K_CTRL = (XTAL32K_XIN_CAP_BYPASS_DISABLE            |
                         XTAL32K_AMPL_CTRL_ENABLE                  |
                         XTAL32K_NOT_FORCE_READY                   |
                         (0x38 << ACS_XTAL32K_CTRL_CLOAD_TRIM_Pos) |
                         XTAL32K_ITRIM_320NA                       |
                         XTAL32K_IBOOST_ENABLE                     |
                         XTAL32K_ENABLE);

    /*
     * Use SysTick to avoid continuously checking for READY flag and save some
     * power.
     */
    do
    {
        /* Initialize SYSTICK
         * Used to put ARM core to sleep while waiting for crystal to start up.
         * EXTREF cock source -> SLOWCLK32 (1MHz / 32)
         * LOAD values:
         *     3125 -> 100 ms +/- 16 us
         *     312  -> 10 ms
         */
        SysTick->LOAD = 312;
        SysTick->CTRL = SYSTICK_ENABLE | SYSTICK_TICKINT_ENABLE
                | SYSTICK_CLKSOURCE_EXTREF_CLK;

        SYS_WAIT_FOR_INTERRUPT;

        /* Clear SysTick interrupt pending flag.
         * The ISR will not be called to not interfere with custom application
         * SysTick IRQ handler or the default one which is just infinite loop.
         */
        SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk;

        /* Disable SysTick. */
        SysTick->CTRL = SYSTICK_DISABLE;

        /* Wait for XTAL32K oscillator to be ready */
    } while (ACS_XTAL32K_CTRL->READY_ALIAS != XTAL32K_OK_BITBAND);
}

void APP_EnterFlashSleep(void)
{
    struct sleep_mode_flash_env_tag power_down_sleep_env;

    /* Clear all reset flags. */
    ACS->RESET_STATUS = 0x7F;
    DIG->RESET_STATUS = 0xF0;

    /* Set wake-up configuration.
     *
     * Button DIO is the only active wake-up source.
     * Wake-up on falling edge -> button press.
     */
    power_down_sleep_env.wakeup_cfg = WAKEUP_DELAY_32              |
                                      WAKEUP_WAKEUP_PAD_RISING     |
                                      WAKEUP_DCDC_OVERLOAD_DISABLE |
                                      WAKEUP_DIO0_FALLING          |
                                      WAKEUP_DIO1_FALLING          |
                                      WAKEUP_DIO2_FALLING          |
                                      WAKEUP_DIO3_FALLING;

    /* Set wake-up mode for boot from flash.
     *
     * Pad retention must be enabled to provide internal pull-up to the button
     * DIO.
     */
    power_down_sleep_env.wakeup_ctrl = PADS_RETENTION_ENABLE         |
                                       BOOT_FLASH_APP_REBOOT_DISABLE |
                                       BOOT_FLASH_XTAL_DISABLE       |
                                       WAKEUP_DCDC_OVERLOAD_CLEAR    |
                                       WAKEUP_PAD_EVENT_CLEAR        |
                                       WAKEUP_RTC_ALARM_CLEAR        |
                                       WAKEUP_BB_TIMER_CLEAR         |
                                       WAKEUP_DIO3_EVENT_CLEAR       |
                                       WAKEUP_DIO2_EVENT_CLEAR       |
                                       WAKEUP_DIO1_EVENT_CLEAR       |
                                       WAKEUP_DIO0_EVENT_CLEAR;

    /* Disable memory retention. */
    power_down_sleep_env.mem_power_cfg = 0;
    power_down_sleep_env.VDDMRET_enable = VDDMRET_DISABLE_BITBAND;

    /* Disconnect logic from power supply. */
    power_down_sleep_env.VDDCRET_enable = VDDCRET_DISABLE_BITBAND;

    /* Disconnect baseband timer from power supply. */
    power_down_sleep_env.VDDTRET_enable = VDDTRET_DISABLE_BITBAND;

    /* Disable Base Band timer. */
    ACS_BB_TIMER_CTRL->BB_TIMER_NRESET_ALIAS = BB_TIMER_RESET_BITBAND;

    /* Disable RTC. */
    NVIC_DisableIRQ(RTC_ALARM_IRQn);
    NVIC_DisableIRQ(RTC_CLOCK_IRQn);
    ACS_RTC_CTRL->ENABLE_ALIAS = RTC_DISABLE_BITBAND;

    /* Disable low power oscillators. */
    ACS_XTAL32K_CTRL->ENABLE_ALIAS = XTAL32K_DISABLE_BITBAND;
    ACS_RCOSC_CTRL->RC_OSC_EN_ALIAS = RC_OSC_DISABLE_BITBAND;

    /* Disable Clock Detection. */
    ACS_CLK_DET_CTRL->RESET_IGNORE_ALIAS = ACS_CLK_DET_RESET_DISABLE_BITBAND;
    ACS_CLK_DET_CTRL->ENABLE_ALIAS = ACS_CLK_DET_DISABLE_BITBAND;

    /* Load sleep configuration onto ACS registers and enter sleep. */
    Sys_PowerModes_Sleep_WakeupFromFlash(&power_down_sleep_env);
}

void APP_EnterFotaMode(void)
{
    /* Indicate switch to power down mode by turning on LED for 1 second. */
    Sys_PWM_Config(0, APP_LED_DUTY_CYCLE, APP_LED_FOTA_ENTER_PWM_DUTY);

    uint32_t timestamp = APP_RTC_GetTimeMs();
    while ((APP_RTC_GetTimeMs() - timestamp) <= 1000)
    {
        Sys_Watchdog_Refresh();
    }

    /* Disable all peripherals. */
    Device_PrepareForSleep();

    /* Reset BLE stack and disable Baseband. */
    BLE_Reset();
    BBIF->CTRL = BB_CLK_DISABLE | BBCLK_DIVIDER_8 | BB_DEEP_SLEEP;

    /* Switch SYSCLK to frequency suitable for executing of initialization
     * code from ROM.
     *
     * SYSCLK = 8 MHz sourced from RFCLK
     */
    RF_REG2F->CK_DIV_1_6_CK_DIV_1_6_BYTE = CK_DIV_1_6_PRESCALE_6_BYTE;
    Sys_Clocks_SystemClkConfig(SYSCLK_CLKSRC_RFCLK);
    ASSERT(SystemCoreClock == 8000000);

    /* Pass control to FOTA application.
     *
     * Enter as stackless application since BLE stack got reset above.
     */
    Sys_Fota_StartDfu(0);

    /* Unreachable */
    while (true);
}

void APP_SleepModeConfigure(void)
{
    struct sleep_mode_init_env_tag sleep_mode_init_env;
    struct lld_sleep_params_t desired_lld_sleep_params;

    /* Set the clock source for RTC to XTAL 32.768 kHz source. */
    sleep_mode_init_env.rtc_ctrl = RTC_CLK_SRC_XTAL32K;

    /* Set wake-up stabilization delay when XTAL32K standby clock is used. */
    desired_lld_sleep_params.twosc = 1400;
    BLE_LLD_Sleep_Params_Set(desired_lld_sleep_params);

    /* Set delay and wake-up sources, use
     *    WAKEUP_DELAY_[ 1 | 2 | 4 | ... | 128],
     *    WAKEUP_DCDC_OVERLOAD_[ENABLE | DISABLE],
     *    WAKEUP_WAKEUP_PAD_[RISING | FALLING],
     *    WAKEUP_DIO*_[RISING | FALLING],
     *    WAKEUP_DIO*_[ENABLE | DISABLE]
     *
     *    RTC wake-up is enabled separately from RTC configuration register.
     */
    sleep_mode_init_env.wakeup_cfg = WAKEUP_DELAY_32          |
                                     WAKEUP_WAKEUP_PAD_RISING |
                                     WAKEUP_DIO0_DISABLE      |
                                     WAKEUP_DIO1_DISABLE      |
                                     WAKEUP_DIO2_DISABLE      |
                                     WAKEUP_DIO3_DISABLE      |
                                     WAKEUP_DIO0_FALLING      |
                                     WAKEUP_DIO1_FALLING      |
                                     WAKEUP_DIO2_FALLING      |
                                     WAKEUP_DIO3_FALLING;

    /* Enable External DIO IRQ wake up, based on Board Support
     *
     * Enable only if IRQ pin is wake-up capable (DIO[0-3]).
     */
#if ((SMARTSHOT_PIR_IRQ_DIO_NUM >= 0) && (SMARTSHOT_PIR_IRQ_DIO_NUM <= 3))
    sleep_mode_init_env.wakeup_cfg = sleep_mode_init_env.wakeup_cfg | (1 << SMARTSHOT_PIR_IRQ_DIO_NUM);
#endif /* if ((SMARTSHOT_PIR_IRQ_DIO_NUM > 0) && (SMARTSHOT_PIR_IRQ_DIO_NUM <= 3)) */

#if ((SMARTSHOT_ACCEL_INT1_DIO_NUM >= 0) && (SMARTSHOT_ACCEL_INT1_DIO_NUM <= 3))
    sleep_mode_init_env.wakeup_cfg = sleep_mode_init_env.wakeup_cfg | (1 << SMARTSHOT_ACCEL_INT1_DIO_NUM);
#endif /* if ((SMARTSHOT_ACCEL_INT1_DIO_NUM > 0) && (SMARTSHOT_ACCEL_INT1_DIO_NUM <= 3)) */

    /* Set wake-up control/status registers, use
     *    PADS_RETENTION_[ENABLE | DISABLE],
     *    BOOT_FLASH_APP_REBOOT_[ENABLE | DISABLE],
     *    BOOT_[CUSTOM | FLASH_XTAL_*],
     *    WAKEUP_DCDC_OVERLOAD_CLEAR,
     *    WAKEUP_PAD_EVENT_CLEAR,
     *    WAKEUP_RTC_ALARM_CLEAR,
     *    WAKEUP_BB_TIMER_CLEAR,
     *    WAKEUP_DIO3_EVENT_CLEAR,
     *    WAKEUP_DIO2_EVENT_CLEAR,
     *    WAKEUP_DIO1_EVENT_CLEAR,
     *    WAKEUP_DIO0_EVENT_CLEAR
     */
    app_sleep_mode_env.wakeup_ctrl = PADS_RETENTION_ENABLE         |
                                     BOOT_FLASH_APP_REBOOT_DISABLE |
                                     BOOT_CUSTOM                   |
                                     WAKEUP_DCDC_OVERLOAD_CLEAR    |
                                     WAKEUP_PAD_EVENT_CLEAR        |
                                     WAKEUP_RTC_ALARM_CLEAR        |
                                     WAKEUP_BB_TIMER_CLEAR         |
                                     WAKEUP_DIO3_EVENT_CLEAR       |
                                     WAKEUP_DIO2_EVENT_CLEAR       |
                                     WAKEUP_DIO1_EVENT_CLEAR       |
                                     WAKEUP_DIO0_EVENT_CLEAR;

    /* Set wake-up application start address (LSB must be set) */
    sleep_mode_init_env.app_addr = (uint32_t) (&Wakeup_From_Sleep_Application_asm)
                                   | 1;

    /* Set wake-up restore address
     *
     * This address must be adjusted according to the number of DRAM blocks
     * defined in ldscript.
     *
     * - DRAM0_TOP - If 8 KB of DRAM are used.
     * - DRAM1_TOP - If 16 KB of DRAM are used.
     * - DRAM2_TOP - If 24 KB of DRAM are used.
     */
    sleep_mode_init_env.wakeup_addr = (uint32_t) (DRAM2_TOP + 1
                                                  - POWER_MODE_WAKEUP_INFO_SIZE);

    /* Configure memory retention */
    app_sleep_mode_env.mem_power_cfg = FLASH_POWER_ENABLE
                                     | DRAM0_POWER_ENABLE
                                     | DRAM1_POWER_ENABLE
                                     | DRAM2_POWER_ENABLE
                                     | BB_DRAM0_POWER_ENABLE
                                     | BB_DRAM1_POWER_ENABLE;

    /* Configure memory at wake-up (PROM must be part of this) */
    sleep_mode_init_env.mem_power_cfg_wakeup = PROM_POWER_ENABLE
                                               | FLASH_POWER_ENABLE
                                               | DRAM0_POWER_ENABLE
                                               | DRAM1_POWER_ENABLE
                                               | DRAM2_POWER_ENABLE
                                               | BB_DRAM0_POWER_ENABLE
                                               | BB_DRAM1_POWER_ENABLE;

    /* Set DMA channel used to save/restore RF registers
     * in each sleep/wake-up cycle */
    sleep_mode_init_env.DMA_channel_RF = DMA_CHAN_SLP_WK_RF_REGS_COPY;

    /* Perform initializations required for sleep mode */
    Sys_PowerModes_Sleep_Init_2Mbps(&sleep_mode_init_env);
}

void APP_EnterSleep(void)
{
    /* Disable all peripherals and configure DIO pads for sleep. */
    Device_PrepareForSleep();

    /* Clear all reset flags. */
    ACS->RESET_STATUS = 0x7F;
    DIG->RESET_STATUS = 0xF0;

    /* Attempt to enter into deep sleep mode. */
    __disable_irq();
    BLE_Power_Mode_Enter(&app_sleep_mode_env, POWER_MODE_SLEEP);
    __enable_irq();

    /* Restore Device State if BLE stack refused to enter into
     * low power mode */
    Device_Restore();

    /* Execute any pending BLE events */
    Kernel_Schedule();
}

void Wakeup_From_Sleep_Application(void)
{
    /* Restore RF and BB registers. */
    Sys_PowerModes_Wakeup_2Mbps();

    /* The system is awake from this point, continue application from flash. */
    ContinueApplication();
}

void ContinueApplication(void)
{
    /* Restore device configuration. */
    Device_Wakeup();

    /* Continue application from main loop. */
    Main_Loop();
}

/* ----------------------------------------------------------------------------
 * End of File
 * ------------------------------------------------------------------------- */
