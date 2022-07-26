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
 * @file app_ble_dfus.h
 *
 * Device Firmware Update Service Server DFUS - header file
 *
 * Defines custom BLE service to switch device to FOTA mode.
 *
 */

#ifndef APP_BLE_DFUS_H
#define APP_BLE_DFUS_H

#ifdef __cplusplus
extern "C"
{
#endif /* ifdef __cplusplus */

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/

#include <rsl10_ke.h>
#include <gattc_task.h>
#include <sys_fota.h>
#include <sys_boot.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

/* Size of DFUS characteristics. */
#define DFUS_DEVID_CHAR_VALUE_SIZE          (16)
#define DFUS_BOOTVER_CHAR_VALUE_SIZE        (sizeof(Sys_Boot_app_version_t))
#define DFUS_STACKVER_CHAR_VALUE_SIZE       (sizeof(Sys_Boot_app_version_t))
#define DFUS_APPVER_CHAR_VALUE_SIZE         (sizeof(Sys_Boot_app_version_t))
#define DFUS_BUILDID_CHAR_VALUE_SIZE        (32)
#define DFUS_ENTER_CHAR_VALUE_SIZE          (1)

/* Descriptions of DFUS characteristics. */
#define DFUS_CHAR_DEVID_DESC "Device ID"
#define DFUS_CHAR_BOOTVER_DESC "Bootloader Version"
#define DFUS_CHAR_STACKVER_DESC "BLE Stack Version"
#define DFUS_CHAR_APPVER_DESC "Application Version"
#define DFUS_CHAR_BUILDID_DESC "BLE Stack Build ID"
#define DFUS_CHAR_ENTER_DESC "Enter DFU Mode"


/**
 * List of all attributes supported by this by DFUS server.
 */
typedef enum DFUS_AttIdx_t
{
    /* DFU Service */
    DFUS_ATT_SERVICE_0,

    /* Device ID Characteristic */
    DFUS_ATT_DEVID_CHAR_0,
    DFUS_ATT_DEVID_VAL_0,
    DFUS_ATT_DEVID_NAME_0,

    /* Bootloader Version Characteristic */
    DFUS_ATT_BOOTVER_CHAR_0,
    DFUS_ATT_BOOTVER_VAL_0,
    DFUS_ATT_BOOTVER_NAME_0,

    /* BLE Stack Version Characteristic */
    DFUS_ATT_STACKVER_CHAR_0,
    DFUS_ATT_STACKVER_VAL_0,
    DFUS_ATT_STACKVER_NAME_0,

    /* Application Version Characteristic */
    DFUS_ATT_APPVER_CHAR_0,
    DFUS_ATT_APPVER_VAL_0,
    DFUS_ATT_APPVER_NAME_0,

    /* Application Build ID Characteristic */
    DFUS_ATT_BUILDID_CHAR_0,
    DFUS_ATT_BUILDID_VAL_0,
    DFUS_ATT_BUILDID_NAME_0,

    /* Enter DFU Mode Characteristic */
    DFUS_ATT_ENTER_CHAR_0,
    DFUS_ATT_ENTER_VAL_0,
    DFUS_ATT_ENTER_NAME_0,

    /* Total number of all custom attributes of DFUS. */
    DFUS_ATT_COUNT,
} DFUS_AttIdx_t;

/* ----------------------------------------------------------------------------
 * Function definitions
 * --------------------------------------------------------------------------*/
/**
 * Event callback used to notify application that write to DFU Enter Attribute was initiated.
 */
typedef void (*DFUS_DfuEnterCallback_t)(void);

/**
 * Initialize the Device Firmware Update Service Server.
 *
 * Sets initial values of all service related characteristics and descriptors.
 * All service related attributes are then added to attribute database.
 *
 * @pre
 * The Peripheral Server library was initialized with sufficient attribute
 * database size using APP_BLE_PeripheralServerInitialize.
 *
 * @param p_dfu_enter_cb
 * Application provided callback that will be called write to DFU Enter attribute is written.
 *
 * @return
 * 0  - On success. <br>
 * -1 - Attribute database does not have enough free space to fit all DFUS
 *      attributes.
 */
int32_t APP_DFUS_Initialize(DFUS_DfuEnterCallback_t p_dfu_enter_cb);


#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* APP_BLE_DFUS_H */
