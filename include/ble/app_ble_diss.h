/* ----------------------------------------------------------------------------
 * Copyright (c) 2021 Semiconductor Components Industries, LLC (d/b/a
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
 * app_ble_diss.h
 * - DISS Application-specific header file
 * ------------------------------------------------------------------------- */

#ifndef APP_BLE_DISS_H
#define APP_BLE_DISS_H

/* ----------------------------------------------------------------------------
 * If building with a C++ compiler, make all of the definitions in this header
 * have a C binding.
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C"
{
#endif    /* ifdef __cplusplus */

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/
#include <ble_diss.h>
#include <onsemi_smartshot_config.h>
#include <RTE_Components.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/
#define STR_IMPL_(x) #x      // stringify argument
#define STR(x) STR_IMPL_(x)  // indirection to expand argument macros

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/
#define APP_BLE_DISS_SUPPORTED_FEATURES (DIS_MANUFACTURER_NAME_CHAR_SUP | \
                                         DIS_MODEL_NB_STR_CHAR_SUP | \
                                         DIS_HARD_REV_STR_CHAR_SUP | \
                                         DIS_SW_REV_STR_CHAR_SUP)

#define APP_BLE_DISS_MANUFACTURER_NAME "ON Semiconductor"

#if defined(RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_GEVB)
#define APP_BLE_DISS_MODEL "SECO-RSL10-CAM-GEVB"
#elif defined(RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_COLOR_GEVK)
#define APP_BLE_DISS_MODEL "SECO-RSL10-CAM-COLOR-GEVK"
#else
#warning "APP_BLE_DISS_MODEL was not set! Select correct board: SECO-RSL10-CAM-GEVB or SECO-RSL10-CAM-COLOR-GEVK."
#endif

#if defined(RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_GEVB)
#define APP_BLE_DISS_HW_REV "rev0.1"
#elif defined(RTECFG_SMARTSHOT_BSP_COMMON_SECO_RSL10_CAM_COLOR_GEVK)
#define APP_BLE_DISS_HW_REV "rev0.3"
#else
#warning "APP_BLE_DISS_MODEL was not set! Select correct board: SECO-RSL10-CAM-GEVB or SECO-RSL10-CAM-COLOR-GEVK."
#endif

#define APP_BLE_DISS_SW_VERSION (STR(CFG_FOTA_VER_MAJOR) "." STR(CFG_FOTA_VER_MINOR) "." STR(CFG_FOTA_VER_REVISION))

/* ----------------------------------------------------------------------------
 * Function prototype definitions
 * --------------------------------------------------------------------------*/

int32_t APP_BLE_DISS_Initialize(void);

/* ----------------------------------------------------------------------------
 * Close the 'extern "C"' block
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* APP_H */
