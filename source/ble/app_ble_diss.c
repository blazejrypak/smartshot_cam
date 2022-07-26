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
 * app_diss.c
 * - DISS Application-specific source file
 * ------------------------------------------------------------------------- */
#include <app_ble_diss.h>

#include <string.h>

static struct DISS_DeviceInfo_t APP_BLE_DISS_deviceInfo;

int32_t APP_BLE_DISS_Initialize(void)
{
    memset(&APP_BLE_DISS_deviceInfo, 0, sizeof(struct DISS_DeviceInfo_t));

    APP_BLE_DISS_deviceInfo.MANUFACTURER_NAME.data = (uint8_t*)APP_BLE_DISS_MANUFACTURER_NAME;
    APP_BLE_DISS_deviceInfo.MANUFACTURER_NAME.len = strlen(APP_BLE_DISS_MANUFACTURER_NAME);

    APP_BLE_DISS_deviceInfo.MODEL_NB_STR.data = (uint8_t*)APP_BLE_DISS_MODEL;
    APP_BLE_DISS_deviceInfo.MODEL_NB_STR.len = strlen(APP_BLE_DISS_MODEL);

    APP_BLE_DISS_deviceInfo.SW_REV_STR.data = (uint8_t*)APP_BLE_DISS_SW_VERSION;
    APP_BLE_DISS_deviceInfo.SW_REV_STR.len = strlen(APP_BLE_DISS_SW_VERSION);

    APP_BLE_DISS_deviceInfo.HARD_REV_STR.data = (uint8_t*)APP_BLE_DISS_HW_REV;
    APP_BLE_DISS_deviceInfo.HARD_REV_STR.len = strlen(APP_BLE_DISS_HW_REV);

    /* Initialize DISS Service */
    return DISS_Initialize(APP_BLE_DISS_SUPPORTED_FEATURES, &APP_BLE_DISS_deviceInfo);
}
