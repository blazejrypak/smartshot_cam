/* ----------------------------------------------------------------------------
 * Copyright (c) 2018 Semiconductor Components Industries, LLC (d/b/a
 * ON Semiconductor), All Rights Reserved
 *
 * This code is the property of ON Semiconductor and may not be redistributed
 * in any form without prior written permission from ON Semiconductor.
 * The terms of use and warranty for this code are covered by contractual
 * agreements between ON Semiconductor and the licensee.
 *
 * This is Reusable Code.
 *
 * ----------------------------------------------------------------------------
 * app_config.c
 * - Application configuration source file
 * ------------------------------------------------------------------------- */

#include "app.h"
#include <RTE_Components.h>

extern ARM_DRIVER_I2C Driver_I2C0;
extern ARM_DRIVER_SPI Driver_SPI0;

DEFINE_THIS_FILE_FOR_ASSERT;

SYS_FOTA_VERSION(CFG_FOTA_VER_ID, CFG_FOTA_VER_MAJOR, CFG_FOTA_VER_MINOR,
    CFG_FOTA_VER_REVISION);

static void Device_I2C0EventHandler(uint32_t event)
{
    SMARTSHOT_PMIC_I2CEventHandler(event);
    SMARTSHOT_ENV_I2CEventHandler(event);
    SMARTSHOT_ACCEL_I2CEventHandler(event);
}

static void Device_SPI0EventHandler(uint32_t event)
{
    SMARTSHOT_ISP_SPIEventHandler(event);
}

static void Device_SetPowerSupplies(void)
{
    uint8_t trim_status;

    trim_status = Load_Trim_Values_And_Calibrate_MANU_CALIB();
    if (trim_status != VOLTAGES_CALIB_NO_ERROR)
    {
        while (true)
        {
            Sys_Watchdog_Refresh();
        }
    }

    /* Configure the current trim settings for VCC, VDDA */
    ACS_VCC_CTRL->ICH_TRIM_BYTE  = VCC_ICHTRIM_16MA_BYTE;
    ACS_VDDA_CP_CTRL->PTRIM_BYTE = VDDA_PTRIM_16MA_BYTE;

    /* Enable buck converter for VBAT > 1.4V */
    ACS_VCC_CTRL->BUCK_ENABLE_ALIAS = VCC_BUCK_BITBAND;

    /* Enable CDDRF to start 48 MHz XTAL oscillator */
    ACS_VDDRF_CTRL->ENABLE_ALIAS = VDDRF_ENABLE_BITBAND;
    ACS_VDDRF_CTRL->CLAMP_ALIAS  = VDDRF_DISABLE_HIZ_BITBAND;

    /* Wait until VDDRF supply has powered up */
    while (ACS_VDDRF_CTRL->READY_ALIAS != VDDRF_READY_BITBAND);

    /* Disable RF TX power amplifier supply voltage and
     * connect the switched output to VDDRF regulator */
    ACS_VDDPA_CTRL->ENABLE_ALIAS = VDDPA_DISABLE_BITBAND;
    ACS_VDDPA_CTRL->VDDPA_SW_CTRL_ALIAS    = VDDPA_SW_VDDRF_BITBAND;

    /* Enable RF power switches */
    SYSCTRL_RF_POWER_CFG->RF_POWER_ALIAS   = RF_POWER_ENABLE_BITBAND;
}

/**
 * Configures frequencies of internal clocks required for operation.
 *
 * Clocks must be configured after POR and after wake-up from deep sleep.
 */
static void Device_SetClockDividers(void)
{
    /* Enable the 48 MHz oscillator divider using the desired prescale value */
    RF_REG2F->CK_DIV_1_6_CK_DIV_1_6_BYTE = CK_DIV_1_6_PRESCALE_1_BYTE;

    /* Wait until 48 MHz oscillator is started */
    while (RF_REG39->ANALOG_INFO_CLK_DIG_READY_ALIAS !=
           ANALOG_INFO_CLK_DIG_READY_BITBAND);

    /* Switch to (divided 48 MHz) oscillator clock */
    Sys_Clocks_SystemClkConfig(JTCK_PRESCALE_1   |
                               EXTCLK_PRESCALE_1 |
                               SYSCLK_CLKSRC_RFCLK);

    /* Configure clock dividers
     * SLOWCLK = 1 MHz
     * BBCLK = 16 MHz
     * BBCLK_DIV = 1 MHz
     * USRCLK = 48 MHz
     * CPCLK = 250 kHz
     * DCCLK = 12 MHz - Recommended DCCLK frequency for 3.3V
     */
    CLK->DIV_CFG0 = (SLOWCLK_PRESCALE_48 | BBCLK_PRESCALE_6 |
                     USRCLK_PRESCALE_1);
    CLK->DIV_CFG2 = (CPCLK_PRESCALE_4 | DCCLK_PRESCALE_4);

    /* Configure peripheral clock dividers:
     * PWM0CLK = 100 kHz
     * PWM1CLK = 100 kHz
     * UARTCLK - Configured later by peripheral driver.
     * AUDIOCLK - Unused
     */
    CLK_DIV_CFG1->PWM0CLK_PRESCALE_BYTE = 9;
    CLK_DIV_CFG1->PWM1CLK_PRESCALE_BYTE = 9;

    BBIF->CTRL    = (BB_CLK_ENABLE | BBCLK_DIVIDER_8 | BB_DEEP_SLEEP);
}

static void Device_ResetDioPads(void)
{
    /* Set initial DIO configuration.
     * Overrides the default value after POR (DISABLED with weak pull-up) to
     * all pads disabled with no pull resistors.
     */
    for (int i = 0; i < 16; ++i)
    {
        Sys_DIO_Config(i, DIO_MODE_DISABLE);
    }

    /* Lower drive strength (required when VDDO > 2.7)*/
    DIO->PAD_CFG = PAD_LOW_DRIVE;

    /* Disable JTAG debug port to gain application control over DIO 13,14,15 */
    DIO_JTAG_SW_PAD_CFG->CM3_JTAG_DATA_EN_ALIAS = 0;
    DIO_JTAG_SW_PAD_CFG->CM3_JTAG_TRST_EN_ALIAS = 0;
}

/**
 * Configure hardware and initialize BLE stack.
 */
void Device_Initialize(void)
{
    /* Mask all interrupts */
    __set_PRIMASK(PRIMASK_DISABLE_INTERRUPTS);

    /* Disable all interrupts and clear any pending interrupts */
    Sys_NVIC_DisableAllInt();
    Sys_NVIC_ClearAllPendingInt();

    Device_SetPowerSupplies();

    /* Remove RF isolation to start 48 MHz oscillator. */
    SYSCTRL_RF_ACCESS_CFG->RF_ACCESS_ALIAS = RF_ACCESS_ENABLE_BITBAND;

    /* Start the 48 MHz oscillator without changing the other register bits */
    RF->XTAL_CTRL = ((RF->XTAL_CTRL & ~XTAL_CTRL_DISABLE_OSCILLATOR) |
                     XTAL_CTRL_REG_VALUE_SEL_INTERNAL);

    /* Configure internal clock frequencies. */
    Device_SetClockDividers();

    Device_ResetDioPads();

    /* Disable pad retention in case of deep sleep wake-up from flash.
     *
     * Peripherals (i.e. DIO, I2C) are not guaranteed to report correct DIO pad
     *  status before this point in certain deep sleep use cases.
     */
    ACS_WAKEUP_CTRL->PADS_RETENTION_EN_BYTE = PADS_RETENTION_DISABLE_BYTE;

    /* Start the XTAL32K crystal oscillator.
     * Required for RTC and deep sleep operation.
     */
    APP_XTAL32K_Start();

    /* Start RTC counter. */
    APP_RTC_Start();

    /* Seed the random number generator.
     * Required dependency for BLE stack.
     */
    srand(1);

    /* Set default interrupt priority for all interrupts.
     * This allows to use BASEPRI based critical sections since BASEPRI is
     * unable to mask interrupts with highest priority (0).
     *
     * Modules that use BASEPRI critical sections by default on CM3:
     * SEGGER RTT, SEGGER SYSVIEW, CMSIS-FreeRTOS
     */
    for (int i = WAKEUP_IRQn; i < BLE_AUDIO2_IRQn; ++i)
    {
        NVIC_SetPriority(i, 1);
    }

    /* Initialize printf IO functionality. */
    SMARTSHOT_PRINTF_INIT();

    /* Initialize trace functionality. */
    APP_TRACE_INIT();

    /* APP INITIALIZE  */

    /* Initialize the kernel and Bluetooth stack */
    {
        int32_t status;

        APP_BLE_PeripheralServerInitialize(ATT_PTSS_COUNT + ESTSS_ATT_COUNT + DFUS_ATT_COUNT);

        status = PTSS_Initialize(APP_PTSS_EventHandler);
        ENSURE(status == PTSS_OK);

        status = ESTSS_Initialize(APP_ESTSS_EventHandler, APP_RTC_GetTimeMs);
        ENSURE(status == 0);

        status = APP_DFUS_Initialize(APP_DFU_EventHandler);
        ENSURE(status == 0);
    }

#if (CFG_SMARTSHOT_APP_SLEEP_ENABLED == 1)
    /* Trim startup RC oscillator to 3 MHz (required by Sys_PowerModes_Wakeup)
     */
    Sys_Clocks_OscRCCalibratedConfig(3000);

    /* Configure deep sleep feature. */
    APP_SleepModeConfigure();
#endif /* if (CFG_SMARTSHOT_APP_SLEEP_ENABLED == 1) */

    /* Initialize peripheral drivers.
     * Automatic peripheral configuration must be enabled in RTE_Device.h !
     */
    Driver_I2C0.Initialize(&Device_I2C0EventHandler);
    Driver_SPI0.Initialize(&Device_SPI0EventHandler);

    /* Stop masking interrupts */
    __set_PRIMASK(PRIMASK_ENABLE_INTERRUPTS);
    __set_FAULTMASK(FAULTMASK_ENABLE_INTERRUPTS);

    /* Configure DIOs and initialize external peripherals. */
    {
        int32_t status;

        /* Configure push button DIO */
        Sys_DIO_Config(SMARTSHOT_PIN_PUSH_BUTTON,
            DIO_MODE_INPUT | DIO_WEAK_PULL_UP | DIO_LPF_DISABLE | DIO_6X_DRIVE);

#ifdef RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_COLOR_GEVK
        /* Configure Battery measurement pins */
        Sys_DIO_Config(SMARTSHOT_PIN_VCC_BAT_EN, DIO_MODE_GPIO_OUT_0);
        Sys_DIO_Config(SMARTSHOT_PIN_VCC_BAT_ADC, DIO_MODE_DISABLE | DIO_NO_PULL);
#endif /* ifdef RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_COLOR_GEVK */

        /* Turn on PWM0 to control LED brightness. */
        Sys_PWM_Config(0, APP_LED_DUTY_CYCLE, APP_LED_IDLE_PWM_DUTY);
        Sys_PWM_Enable(0, PWM0_ENABLE_BITBAND);
        Sys_DIO_Config(SMARTSHOT_PIN_LED_GREEN, DIO_MODE_PWM0 | DIO_6X_DRIVE);

        status = SMARTSHOT_ISP_Initialize(&Driver_I2C0, &Driver_SPI0,
            &APP_ISP_EventHandler);
        ASSERT(status == 0);

        SMARTSHOT_PIR_Initialize();

        status = SMARTSHOT_ENV_Initialize(&Driver_I2C0, APP_RTC_GetTimeMs,
            APP_ENV_DataReadyHandler);
        ASSERT(status == 0);

        status = SMARTSHOT_ACCEL_Intitialize(&Driver_I2C0, APP_RTC_GetTimeMs);
        ASSERT(status == 0);
    }
}

/**
 * Restore hardware state after wake-up from deep sleep mode with RAM retention.
 *
 * - Restores system clock dividers
 * - Restores DIO pad configuration before disabling PAD retention feature.
 * - Wait for BLE stack to fully wake up.
 */
void Device_Wakeup(void)
{
    __disable_irq();

    /* Restore clock dividers.
     *
     * The RFCLK is active at this point, started by the wake-up code.
     */
    Device_SetClockDividers();

    /* Restore default interrupt priorities. */
    for (int irqn = WAKEUP_IRQn; irqn < BLE_AUDIO2_IRQn; ++irqn)
    {
        NVIC_SetPriority(irqn, 1);
    }

    /* Restore default DIO pad configuration. */
    Device_ResetDioPads();

    /* Initialize IO functionality. */
    SMARTSHOT_PRINTF_INIT();
    APP_TRACE_RESUME();

    /* Configure push button DIO */
    Sys_DIO_Config(SMARTSHOT_PIN_PUSH_BUTTON,
        DIO_MODE_INPUT | DIO_WEAK_PULL_UP | DIO_LPF_DISABLE | DIO_6X_DRIVE);

#ifdef RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_COLOR_GEVK
        /* Configure Battery measurement pins */
        Sys_DIO_Config(SMARTSHOT_PIN_VCC_BAT_EN, DIO_MODE_GPIO_OUT_0);
        Sys_DIO_Config(SMARTSHOT_PIN_VCC_BAT_ADC, DIO_MODE_DISABLE | DIO_NO_PULL);
#endif /* ifdef RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_COLOR_GEVK */

    /* Turn on PWM0 to control LED brightness. */
    Sys_PWM_Config(0, APP_LED_DUTY_CYCLE, APP_LED_IDLE_PWM_DUTY);
    Sys_PWM_Enable(0, PWM0_ENABLE_BITBAND);
    Sys_DIO_Config(SMARTSHOT_PIN_LED_GREEN, DIO_MODE_PWM0 | DIO_6X_DRIVE);

    SMARTSHOT_ISP_HostWakeup();

    SMARTSHOT_PIR_HostWakeup();

    /* ENV library -> No action, only I2C pads required. */

    SMARTSHOT_ACCEL_HostWakeup();

    /* Disable pad retention.
     *
     * All application managed DIO pads must be restored at this point!
     * Exception are I2C and SPI peripherals that need pad retention disabled
     * to initialize.
     */
    ACS_WAKEUP_CTRL->PADS_RETENTION_EN_BYTE = PADS_RETENTION_DISABLE_BYTE;

    /* Stop masking interrupts. */
    __set_FAULTMASK(FAULTMASK_ENABLE_INTERRUPTS);
    __set_PRIMASK(PRIMASK_ENABLE_INTERRUPTS);
    __enable_irq();

    /* Re-initialize communication peripherals.
     * Automatic peripheral configuration must be enabled in RTE_Device.h !
     */
    Driver_I2C0.Initialize(Device_I2C0EventHandler);

    /* Customized local variant of SPI CMSIS-Driver is used that does not
     * configure the SSEL pad during initialization.
     * This is required to prevent current spike caused by protection diodes of
     * disabled ISP from the SSEL DIO output which is configured to high level
     * by default in RSL10 CMSIS-Pack provided code.
     */
    Driver_SPI0.Initialize(Device_SPI0EventHandler);

    /* Force BaseBand wake-up in case of external interrupt. */
    BBIF->CTRL = BB_CLK_ENABLE | BBCLK_DIVIDER_8 | BB_WAKEUP;

    /* Disable interrupts */
   __disable_irq();
   while (!(BLE_Is_Awake()))
   {
       /* Enable interrupts */
       __enable_irq();

       /* Allow pended interrupts to be recognized */
       __ISB();

       /* Disable interrupts */
       __disable_irq();
   }

   /* Stop masking interrupts */
   __enable_irq();

   /* Stop forcing baseband to wake-up. */
   BBIF->CTRL = BB_CLK_ENABLE | BBCLK_DIVIDER_8 | BB_DEEP_SLEEP;
}

/**
 * Prepares device to enter deep sleep mode by setting all DIO pads into their
 * desired sleep configuration.
 */
void Device_PrepareForSleep(void)
{
    SMARTSHOT_ISP_HostSleep();

    /* PIR -> no action. Keep current DIO pad configuration. */
    /* ENV -> no action. Uses only I2C pins. */
    /* ACCEL-> no action. Keep current DIO pad configuration. */

    /* Uninitialize CMSIS-Drivers to ensure that software library and hardware
     * peripheral states match after wake-up.
     */
    Driver_I2C0.PowerControl(ARM_POWER_OFF);
    Driver_I2C0.Uninitialize();

    Driver_SPI0.PowerControl(ARM_POWER_OFF);
    Driver_SPI0.Uninitialize();

    for (int dio_pad = 0; dio_pad < 16; ++dio_pad)
    {
        switch (dio_pad)
        {
        case SMARTSHOT_PIN_PMIC_HWEN:
        case SMARTSHOT_PIN_IRQ_BMA400:
        case SMARTSHOT_PIN_IRQ_PIR:
        case SMARTSHOT_PIN_PIR_VCC:
#ifdef RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_GEVB
        case SMARTSHOT_PIN_PMIC_GPIO1:
#endif /* ifdef RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_GEVB */
#ifdef RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_COLOR_GEVK
        case SMARTSHOT_PIN_VCC_BAT_EN:
#endif /* ifdef RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_COLOR_GEVK */
            /* Nothing to do for these DIO pads.
             * Their configuration for deep sleep is maintained by their respective
             * library.
             */
            break;

        default:
            /* By default disconnect all other DIO pads that are not required
             * during deep sleep to prevent unwanted current leakage through
             * these pads.
             */
            Sys_DIO_Config(dio_pad, DIO_MODE_DISABLE | DIO_NO_PULL);
            break;
        }
    }

    APP_TRACE_SLEEP();
    SMARTSHOT_PRINTF_SLEEP();
}

/**
 * Restore Device configuration disabled by Device_PrepareForSleep() function.
 * Used in case BLE did not enter into deep sleep mode to continue correct operation.
 */
void Device_Restore(void)
{
    __disable_irq();

    /* Initialize IO functionality. */
    SMARTSHOT_PRINTF_INIT();
    APP_TRACE_RESUME();

    /* Configure push button DIO */
    Sys_DIO_Config(SMARTSHOT_PIN_PUSH_BUTTON,
        DIO_MODE_INPUT | DIO_WEAK_PULL_UP | DIO_LPF_DISABLE | DIO_6X_DRIVE);

#ifdef RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_COLOR_GEVK
        /* Configure Battery measurement pins */
        Sys_DIO_Config(SMARTSHOT_PIN_VCC_BAT_EN, DIO_MODE_GPIO_OUT_0);
        Sys_DIO_Config(SMARTSHOT_PIN_VCC_BAT_ADC, DIO_MODE_DISABLE | DIO_NO_PULL);
#endif /* ifdef RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_COLOR_GEVK */

    /* Turn on PWM0 to control LED brightness. */
    Sys_PWM_Config(0, APP_LED_DUTY_CYCLE, APP_LED_IDLE_PWM_DUTY);
    Sys_PWM_Enable(0, PWM0_ENABLE_BITBAND);
    Sys_DIO_Config(SMARTSHOT_PIN_LED_GREEN, DIO_MODE_PWM0 | DIO_6X_DRIVE);

    __enable_irq();

    /* Re-initialize communication peripherals.
     * Automatic peripheral configuration must be enabled in RTE_Device.h !
     */
    Driver_I2C0.Initialize(Device_I2C0EventHandler);
    /* Customized local variant of SPI CMSIS-Driver is used that does not
     * configure the SSEL pad during initialization.
     * This is required to prevent current spike caused by protection diodes of
     * disabled ISP from the SSEL DIO output which is configured to high level
     * by default in RSL10 CMSIS-Pack provided code.
     */
    Driver_SPI0.Initialize(Device_SPI0EventHandler);
}

/* ----------------------------------------------------------------------------
 * Function      : Device_Param_Prepare(struct app_device_param * param)
 * ----------------------------------------------------------------------------
 * Description   : This function allows the application to overwrite a few BLE
 *                 parameters (BD address and keys) without having to write
 *                 data into RSL10 flash (NVR3). This function is called by the
 *                 stack and it's useful for debugging and testing purposes.
 * Inputs        : - param    - pointer to the parameters to be configured
 * Outputs       : None
 * Assumptions   : None
 * ------------------------------------------------------------------------- */
void Device_Param_Prepare(app_device_param_t *param)
{
    param->device_param_src_type = APP_BLE_DEV_PARAM_SOURCE;

    if(param->device_param_src_type == APP_PROVIDED)
    {
        uint8_t temp_bleAddress[6] = APP_BD_ADDRESS;
        uint8_t temp_irk[16] = APP_IRK;
        uint8_t temp_csrk[16] = APP_CSRK;
        uint8_t temp_privateKey[32] = APP_PRIVATE_KEY;
        uint8_t temp_publicKey_x[32] = APP_PUBLIC_KEY_X;
        uint8_t temp_publicKey_y[32] = APP_PUBLIC_KEY_Y;

        memcpy(param->bleAddress, temp_bleAddress, 6);
        memcpy(param->irk, temp_irk, 16);
        memcpy(param->csrk, temp_csrk, 16);
        memcpy(param->privateKey, temp_privateKey, 32);
        memcpy(param->publicKey_x, temp_publicKey_x, 32);
        memcpy(param->publicKey_y, temp_publicKey_y, 32);
    }
}

