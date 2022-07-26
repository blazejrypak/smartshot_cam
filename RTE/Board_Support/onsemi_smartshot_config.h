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
 * @file onsemi_smartshot_config.h
 *
 * Configuration file template for Smart Shot BSP libraries and applications.
 *
 * This file can be opened using the CMSIS Configuration Wizard editor in
 * Eclipse.
 *
 * @addtogroup SMARTSHOT_BSP_GRP
 * @{
 *
 * @addtogroup SMARTSHOT_TEMPLATES_GRP BSP configuration files and templates
 * @{
 *
 * @brief
 * List of configuration options and template files.
 *
 * Shared configuration file for all Smart Shot BSP libraries: <br>
 * Filename: onsemi_smartshot_config.h <br>
 * Project location: <b> RTE/Board_Support/onsemi_smartshot_config.h </b>
 *
 */

#ifndef ONSEMI_SMARTSHOT_CONFIG_H
#define ONSEMI_SMARTSHOT_CONFIG_H

#include <RTE_Components.h>

/* ----------------------------------------------------------------------------
 * Configuration Defines
 * --------------------------------------------------------------------------*/

// <<< Use Configuration Wizard in Context Menu >>>

// <n> SmartShot Software Component Settings

// <h> SmartShot::Board Part.Common
// <i> Settings for the Board Part.Common software component.

// <e> Override default Application Name
// <i> Choose custom Application Name
// <i> If this option is not selected default Application Name will be used.
// <i> Default Application Name for SECO-RSL10-CAM-GEVB board is 'smartshot_demo_cam'
// <i> Default Application Name for SECO-RSL10-CAM-COLOR-GEVK board is 'smartshot_demo_color_cam'
#ifndef CFG_APP_NAME_USER_DEFINED
#define CFG_APP_NAME_USER_DEFINED 0
#endif

// <s> Application Name
#define CFG_APP_NAME_USER  "custom_app_name"
// </e>

// </h>

// <h> SmartShot::Board Part.Library.PIR
// <i> Settings for the Board Part.Library.PIR software component.

// <o> DIO Interrupt source <0-3>
// <i> Defines which of the 4 available DIO Interrupts to assign for PIR functionality.
#define CFG_SMARTSHOT_PIR_INT_CFG_NUM  0

// </h>

// <h> SmartShot::Board Part.Library.ENV
// <i> Settings for the Board Part.Library.ENV software component.

// <n> Measurement parameters for BME280 sensor

// <e> Temperature measurement
// <i> Enables temperature measurement.
// <i> Disabling is discouraged as other measurements depend on temperature measurement for calibration.
#ifndef CFG_SMARTSHOT_ENV_TEMP_ENABLE
#define CFG_SMARTSHOT_ENV_TEMP_ENABLE 1
#endif

#if defined CFG_SMARTSHOT_ENV_TEMP_ENABLE == 1

// <o> Oversampling
// <i> Oversampling improves temperature measurement resolution by reducing noise.
// <i> Higher oversampling values will result in longer measurement duration.
//    <1=> 1x
//    <2=> 2x
//    <3=> 4x
//    <4=> 8x
//    <5=> 16x
#define CFG_SMARTSHOT_ENV_OS_TEMP  2

#else
#define CFG_SMARTSHOT_ENV_OS_TEMP         0
#endif /* defined CFG_SMARTSHOT_ENV_TEMP_ENABLE == 1 */

// </e>

// <e> Pressure measurement
// <i> Enables pressure measurement.
#ifndef CFG_SMARTSHOT_ENV_PRES_ENABLE
#define CFG_SMARTSHOT_ENV_PRES_ENABLE  0
#endif

#if defined CFG_SMARTSHOT_ENV_PRES_ENABLE == 1

// <o> Oversampling
// <i> Oversampling improves pressure measurement resolution by reducing noise.
// <i> Higher oversampling values will result in longer measurement duration.
//    <1=> 1x
//    <2=> 2x
//    <3=> 4x
//    <4=> 8x
//    <5=> 16x
#define CFG_SMARTSHOT_ENV_OS_PRES  2

#else
#define CFG_SMARTSHOT_ENV_OS_PRES  0
#endif /* defined CFG_SMARTSHOT_ENV_PRES_ENABLE == 1 */

// </e>

// <e> Humidity measurement
// <i> Enables humidity measurement
#ifndef CFG_SMARTSHOT_ENV_HUM_ENABLE
#define CFG_SMARTSHOT_ENV_HUM_ENABLE  1
#endif

#if defined CFG_SMARTSHOT_ENV_HUM_ENABLE == 1

// <o> Oversampling
// <i> Higher oversampling setting reduces noise.
// <i> Higher oversampling values will result in longer measurement duration.
//    <1=> 1x
//    <2=> 2x
//    <3=> 4x
//    <4=> 8x
//    <5=> 16x
#ifndef CFG_SMARTSHOT_ENV_OS_HUM
#define CFG_SMARTSHOT_ENV_OS_HUM  2
#endif

#else
#define CFG_SMARTSHOT_ENV_OS_HUM  0
#endif /* defined CFG_SMARTSHOT_ENV_HUM_ENABLE == 1 */

// </e>

// <o> IIR Filter Coefficient
// <i> Low pass filter for pressure and temperature measurements.
//    <0=> Off
//    <1=> 2
//    <2=> 4
//    <3=> 8
//    <4=> 16
#ifndef CFG_SMARTSHOT_ENV_FILTER_COEFF
#define CFG_SMARTSHOT_ENV_FILTER_COEFF  1
#endif

// </h>

// <h> SmartShot::Board Part.Library.ACCEL
// <i> Settings for the Board Part.Library.ACCEL software component.

// <o> Default Acceleration Event Threshold [mg] <0-2040:8> <#/8>
// <i> Acceleration delta threshold that need to be reached on any accelerometer axis in order to trigger interrupt.
// <i> Default: 96 mg
#define CFG_SMARTSHOT_ACCEL_DFLT_THRESHOLD  7

// <o> Default Acceleration Event Duration [samples] <0-65535>
// <i> Number of samples for which the Event Threshold must be met in order to generate interrupt.
// <i> Default: 5
#define CFG_SMARTSHOT_ACCEL_DFLT_DURATION  4

// <o> DIO Interrupt source <0-3>
// <i> Defines which of the 4 available DIO Interrupts to assign for Accelerometer interrupt functionality.
#define CFG_SMARTSHOT_ACCEL_INT_CFG_NUM  2

// </h>

// <h> SmartShot::Board Part.Library.ISP

// <o> DIO Interrupt source for ISP_READY signal <0-3>
// <i> Defines which of the 4 available DIO Interrupts to assign for monitoring of ISP_READY signal.
#define CFG_SMARTSHOT_ISP_INT_GFC_NUM  1

// </h>

// <n>
// <n> Common Application Settings
// <i> Configuration macros used for application configuration that are expected to be used across all examples for SmartShot platform.

// <h> Debugging Options

// <q> Enable ASSERT
#define CFG_SMARTSHOT_ASSERT_ENABLED  (1)

// <o> PRINTF Output Interface
//    <0=> Disabled
//    <2=> SEGGER RTT
//    <3=> SEGGER System View (Host)
//    <4=> SEGGER System View (Target)
#define CFG_SMARTSHOT_PRINTF_INTERFACE  (2)

// <e> Enable SystemView Real-Time Trace
#define CFG_SMARTSHOT_TRACE_ENABLED  (1)

// <o> Recording buffer size [bytes]
#define CFG_SMARTSHOT_TRACE_SYSVIEW_BUFFER_SIZE 3000

// <q> Auto start recording
#define CFG_SMARTSHOT_TRACE_AUTO_START  (0)

// </e>

// <q> Enable Deep Sleep
// <i> Allows to disable the deep sleep feature of RSL10 for debugging purposes.
// <i> Default: enabled
#define CFG_SMARTSHOT_APP_SLEEP_ENABLED  (0)

// <q> Power up ISP on boot
// <i> Option to start ISP on application start to allow firmware updates for ISP over USB.
// <i> Default: disabled
#define CFG_SMARTSHOT_APP_POWER_ISP_ON_BOOT  (0)

// </h>

// <h> FOTA Application Information

// <e> Override default FOTA Application Identifier
// <i> Choose custom FOTA Application Identifier
// <i> If this option is not selected default FOTA Application Identifier will be used.
// <i> Default FOTA Identifier for SECO-RSL10-CAM-GEVB board is 'SS_CAM'
// <i> Default FOTA Identifier for SECO-RSL10-CAM-COLOR-GEVK board is 'SS_COL'
#ifndef CFG_FOTA_USER_DEFINED
#define CFG_FOTA_USER_DEFINED 0
#endif
// <s.6> Custom Application Identifier
// <i> Short identification string for this application.
// <i> Must be 6 characters long.
#define CFG_FOTA_USER_VER_ID  "USR_ID"
// </e>

// <h> Version

// <o> Major <0-15>
// <i> Range: 0 - 15
#define CFG_FOTA_VER_MAJOR  2

// <o> Minor <0-15>
// <i> Range: 0 - 15
#define CFG_FOTA_VER_MINOR  1

// <o> Revision <0-255>
// <i> Range: 0 - 255
#define CFG_FOTA_VER_REVISION  0

// </h>
// </h>

// <<< end of configuration section >>>

/* ----------------------------------------------------------------------------
 * Compile time preprocessor checks
 * --------------------------------------------------------------------------*/

#if (CFG_SMARTSHOT_PIR_INT_CFG_NUM == CFG_SMARTSHOT_ISP_INT_GFC_NUM)
#error PIR and ISP both assigned to the same DIO interupt source
#endif

#if (CFG_SMARTSHOT_PIR_INT_CFG_NUM == CFG_SMARTSHOT_ACCEL_INT_CFG_NUM)
#error PIR and ACCEL both assigned to the same DIO interupt source
#endif

#if (CFG_SMARTSHOT_ISP_INT_GFC_NUM == CFG_SMARTSHOT_ACCEL_INT_CFG_NUM)
#error ACCEL and ISP both assigned to the same DIO interupt source
#endif

#if CFG_FOTA_USER_DEFINED == 0
#if defined(RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_GEVB)
#define CFG_FOTA_VER_ID "SS_CAM"
#elif defined(RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_COLOR_GEVK)
#define CFG_FOTA_VER_ID "SS_COL"
#else
#warning "FOTA Application Identifier was not set! Either override default FOTA Identifier \
          or select correct board: SECO-RSL10-CAM-GEVB or SECO-RSL10-CAM-COLOR-GEVK."
#endif
#else
#define CFG_FOTA_VER_ID CFG_FOTA_USER_VER_ID
#endif

#if CFG_APP_NAME_USER_DEFINED == 0
#if defined(RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_GEVB)
#define CFG_APP_NAME "smartshot_demo_cam"
#elif defined(RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_COLOR_GEVK)
#define CFG_APP_NAME "smartshot_demo_color_cam"
#else
#warning "Application Name was not set! Either override default Application Name \
          or select correct board: SECO-RSL10-CAM-GEVB or SECO-RSL10-CAM-COLOR-GEVK."
#endif
#else
#define CFG_APP_NAME CFG_APP_NAME_USER
#endif

#endif /* ONSEMI_SMARTSHOT_CONFIG_H */

/** @} */
/** @} */

/* ----------------------------------------------------------------------------
 * End of File
 * --------------------------------------------------------------------------*/
