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
 * ------------------------------------------------------------------------- */

/**
 * @file app_ble_dfus_int.h
 *
 * Device Firmware Update Service Server - internal definitions header file
 *
 * This file contains definitions required for DFUS operation that are only
 * for internal use within the module and not to be exposed to the application.
 */

#ifndef APP_BLE_DFUS_INT_H
#define APP_BLE_DFUS_INT_H

#ifdef __cplusplus
extern "C"
{
#endif    /* ifdef __cplusplus */

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/
#include <app_ble_dfus.h>

#include <ble_gap.h>
#include <ble_gatt.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

/**
 * Collects all attribute database related variables of Picture Transfer
 * Service.
 */
typedef struct DFUS_AttDb_t
{
    /**
     * Offset of the attidx of ESTSS service attributes.
     *
     * This can change depending on number and order of registered custom
     * services in the shared attribute database.
     */
    uint16_t attidx_offset;
} DFUS_AttDb_t;

/**
 * Main storage structure that holds all variables of the DFUS module.
 */
typedef struct DFUS_Environment_t
{
    /** Stores all attribute and attribute database related variables. */
    DFUS_AttDb_t att;

    /**
     * Application callback function called when write to DFU Enter is written.
     */
    DFUS_DfuEnterCallback_t p_dfu_enter_cb;
} DFUS_Environment_t;

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* APP_BLE_DFUS_INT_H */
