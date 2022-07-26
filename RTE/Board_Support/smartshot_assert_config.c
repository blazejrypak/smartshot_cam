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
 * @file smartshot_assert_config.c
 *
 * Template file for implementation of AssertFailed function to handle failed
 * assertions.
 *
 * @addtogroup SMARTSHOT_BSP_GRP
 * @{
 *
 * @addtogroup SMARTSHOT_TEMPLATES_GRP
 * @{
 *
 * Template AssertFailed implementation file: <br>
 * Filename: smartshot_assert_config.c <br>
 * Project location: <b> RTE/Board_Support/smartshot_assert_confg.c </b>
 */

/* ----------------------------------------------------------------------------
 * Includes
 * ------------------------------------------------------------------------- */

#include <rsl10.h>

#include <smartshot_assert.h>
#include <smartshot_pinmap.h>
#include <smartshot_printf.h>

/* ----------------------------------------------------------------------------
 * Public Function Definition
 * ------------------------------------------------------------------------- */

/**
 * Handler called when an assertion fails.
 *
 * @param p_file
 * String with path to file with the failed assertion.
 *
 * @param line
 * Number of line with the failed assertion.
 */
void AssertFailed(const char *p_file, int line)
{
    PRINTF("\r\n%s:%d Assertion Failed!\r\n", p_file, line);

    /* Toggle LED every second at full power to indicate failure.
     * Total blinking duration is 10 seconds, after that reset the device if
     * debugger is not connected.
     */
    Sys_DIO_Config(SMARTSHOT_PIN_LED_GREEN, DIO_MODE_GPIO_OUT_1 | DIO_6X_DRIVE);
    for (int i = 0; i < 10; ++i)
    {
        for (int j = 0; j < 8; ++j)
        {
            Sys_Delay_ProgramROM(SystemCoreClock / 8);
            Sys_Watchdog_Refresh();
        }
        Sys_GPIO_Toggle(SMARTSHOT_PIN_LED_GREEN);
    }
    Sys_DIO_Config(SMARTSHOT_PIN_LED_GREEN, DIO_MODE_GPIO_OUT_0 | DIO_6X_DRIVE);

    /* Wait for Watchdog to reset the device. */
    while (1);
}

/** @} */
/** @} */

/* ----------------------------------------------------------------------------
 * End of File
 * ------------------------------------------------------------------------- */
