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
 * @file app_ble_peripheral_server.h
 *
 */

#ifndef APP_BLE_PERIPHERAL_SERVER_H
#define APP_BLE_PERIPHERAL_SERVER_H

#include <stdbool.h>
#include <stdint.h>

#include <rsl10.h>
#include <rsl10_protocol.h>

#include <ble_gap.h>
#include <ble_gatt.h>
#include <msg_handler.h>

#include <onsemi_smartshot_config.h>
#include "calibration.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

/* Timer setting in units of 10ms (kernel timer resolution) */
#define TIMER_SETTING_MS(MS)            (MS / 10)
#define TIMER_SETTING_S(S)              (S * 100)

/* Number of APP Task Instances */
#define APP_IDX_MAX                    BLE_CONNECTION_MAX

#define APP_BLE_DEV_PARAM_SOURCE        FLASH_PROVIDED_or_DFLT /* or APP_PROVIDED  */

/*
 * If APP_BD_ADDRESS_TYPE == GAPM_CFG_ADDR_PUBLIC and
 * APP_DEVICE_PARAM_SRC == FLASH_PROVIDED_or_DFLT the bluetooth address is
 * loaded from FLASH NVR3. Otherwise, this address is used.
 */
#define APP_BD_ADDRESS_TYPE             GAPM_CFG_ADDR_PUBLIC /* or GAPM_CFG_ADDR_PUBLIC*/
#ifndef APP_BD_ADDRESS
#define APP_BD_ADDRESS                  { 0x94, 0x11, 0x22, 0xff, 0xbb, 0xD5 }
#endif

#define APP_NB_PEERS                    1

/* The number of standard profiles and custom services added in this application */
#define APP_NUM_STD_PRF                 1
#define APP_NUM_CUSTOM_SVC              1

/* RF output power in dBm */
#define OUTPUT_POWER_DBM                0

#define RADIO_CLOCK_ACCURACY            20 /* RF Oscillator accuracy in ppm */

#define APP_DEVICE_APPEARANCE           0
#define APP_PREF_SLV_MIN_CON_INTERVAL   8
#define APP_PREF_SLV_MAX_CON_INTERVAL   10
#define APP_PREF_SLV_LATENCY            0
#define APP_PREF_SLV_SUP_TIMEOUT        200

/* Advertising data is composed by device name and company id */
#define APP_DEVICE_NAME                 CFG_APP_NAME
#define APP_DEVICE_NAME_LEN             (sizeof(APP_DEVICE_NAME) - 1)

/* Advertising interval high duty.
 *
 * In units of 0.625 ms.
 * 338 * 0.625 ms = 211.25 ms
 */
#define APP_ADV_INT_HIGH_DUTY                (338)

/* Advertising interval low duty.
 *
 * In units of 0.625 ms.
 * 1364 * 0.625 ms = 852.5 ms
 */
#define APP_ADV_INT_LOW_DUTY               (1364)

/* Timeout of 1 minute to change to low duty cycle advertising */
#define APP_ADV_DUTY_CYCLE_TIMEOUT_MS      (TIMER_SETTING_MS(1000 * 60))

/* Timeout of 30 seconds to update connection parameters after connection establishment */
#define APP_UPD_CONN_PARAMS_TIMEOUT_MS      (TIMER_SETTING_MS(1000 * 30))

/* Timeout of 1 second to update connection parameters to Low Power */
#define APP_UPD_CONN_PARAMS_TIMEOUT_LP_MS   (TIMER_SETTING_MS(1000 * 1))

/* Connection Parameters Common */
#define APP_UPD_CONN_TIMEOUT (6000 / 10) /* Timeout setting in units of 10ms */
#define APP_UPD_CONN_CE_LEN_MIN (0xFFFF)
#define APP_UPD_CONN_CE_LEN_MAX (0xFFFF)

/* Connection Parameters for Low Latency mode */
#define APP_UPD_CONN_INTV_LL_MIN (9) /* 11.25 ms (n * 1.25 ms) */
#define APP_UPD_CONN_INTV_LL_MAX (21) /* 26.25 ms (n * 1.25 ms) */
#define APP_UPD_CONN_LATENCY_LL (0)

/* Connection Parameters for Low Power mode */
#define APP_UPD_CONN_INTV_LP_MIN (48) /* 60 ms (n * 1.25 ms) */
#define APP_UPD_CONN_INTV_LP_MAX (120) /* 150 ms (n * 1.25 ms) */
#define APP_UPD_CONN_LATENCY_LP (3)

typedef enum APP_Connection_Parameters_t
{
    APP_UPD_CONN_LOW_POWER = 0,
    APP_UPD_CONN_LOW_LATENCY = 1,
} APP_Connection_Parameters_t;

/* Application-provided IRK */
#define APP_IRK                         { 0x01, 0x23, 0x45, 0x68, 0x78, 0x9a, \
                                          0xbc, 0xde, 0x01, 0x23, 0x45, 0x68, \
                                          0x78, 0x9a, 0xbc, 0xde }

/* Application-provided CSRK */
#define APP_CSRK                        { 0x01, 0x23, 0x45, 0x68, 0x78, 0x9a, \
                                          0xbc, 0xde, 0x01, 0x23, 0x45, 0x68, \
                                          0x78, 0x9a, 0xbc, 0xde }

/* Application-provided private key */
#define APP_PRIVATE_KEY                 { 0xEC, 0x89, 0x3C, 0x11, 0xBB, 0x2E, \
                                          0xEB, 0x5C, 0x80, 0x88, 0x63, 0x57, \
                                          0xCC, 0xE2, 0x05, 0x17, 0x20, 0x75, \
                                          0x5A, 0x26, 0x3E, 0x8D, 0xCF, 0x26, \
                                          0x63, 0x1D, 0x26, 0x0B, 0xCE, 0x4D, \
                                          0x9E, 0x07 }

/* Application-provided public key X */
#define APP_PUBLIC_KEY_X                { 0x56, 0x09, 0x79, 0x1D, 0x5A, 0x5F, \
                                          0x4A, 0x5C, 0xFE, 0x89, 0x56, 0xEC, \
                                          0xE6, 0xF7, 0x92, 0x21, 0xAC, 0x93, \
                                          0x99, 0x10, 0x51, 0x82, 0xF4, 0xDD, \
                                          0x84, 0x07, 0x50, 0x99, 0xE7, 0xC2, \
                                          0xF1, 0xC8 }

/* Application-provided public key Y */
#define APP_PUBLIC_KEY_Y                { 0x40, 0x84, 0xB4, 0xA6, 0x08, 0x67, \
                                          0xFD, 0xAC, 0x81, 0x5D, 0xB0, 0x41, \
                                          0x27, 0x75, 0x9B, 0xA7, 0x92, 0x57, \
                                          0x0C, 0x44, 0xB1, 0x57, 0x7C, 0x76, \
                                          0x5B, 0x56, 0xF0, 0xBA, 0x03, 0xF4, \
                                          0xAA, 0x67 }

typedef struct APP_BLE_Environment_t
{
    uint16_t adv_timer_task_id;
    uint16_t param_upd_timer_task_id;
    uint16_t high_duty_adv_interval;
    uint16_t low_duty_adv_interval;
    bool timer_expired;
    bool disconnection_initiated;
} APP_BLE_Environment_t;

typedef struct APP_BLE_AttDb_t
{
    /**
     *
     * Dynamically allocated on initialization.
     *
     */
    struct att_db_desc *cs_att_db;
    uint16_t cs_att_db_size;
    uint16_t cs_att_count;
    uint16_t cs_service_count;
} APP_BLE_AttDb_t;

/* ---------------------------------------------------------------------------
* Additional Attribute database definitions
* --------------------------------------------------------------------------*/

/* Standard 16-bit UUID for Characteristic Presentation Format Descriptor. */
#define CS_ATT_CHAR_PRES_FMT_128      { 0x04, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                        0x00, 0x00, 0x00 }

#define CS_ATT_CHAR_VAL_TRIGGER_128   { 0x0A, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                        0x00, 0x00, 0x00 }

#define CS_ATT_CHAR_TIME_TRIGGER_128  { 0x0E, 0x29, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                        0x00, 0x00, 0x00 }

#define CS_CHAR_PRESENTATION_FORMAT(attidx, length, data, callback) \
    {attidx, {CS_ATT_CHAR_PRES_FMT_128, PERM(RD, ENABLE), length, PERM(RI, ENABLE) }, false, length, data, callback }

#define CS_CHAR_VALUE_TRIGGER(attidx, length, data, callback) \
    {attidx, {CS_ATT_CHAR_VAL_TRIGGER_128, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), length, PERM(RI, ENABLE) }, false, length, data, callback }

#define CS_CHAR_TIME_TRIGGER(attidx, length, data, callback) \
    {attidx, {CS_ATT_CHAR_TIME_TRIGGER_128, PERM(RD, ENABLE) | PERM(WRITE_REQ, ENABLE), length, PERM(RI, ENABLE) }, false, length, data, callback }


/* ---------------------------------------------------------------------------
* Function prototype definitions
* --------------------------------------------------------------------------*/

void APP_BLE_PeripheralServerInitialize(const uint16_t cs_att_db_size);

int32_t APP_BLE_PeripheralServerAddCustomService(const struct att_db_desc *p_atts,
        const uint16_t atts_count, uint16_t *p_attidx_offset);

/**
 * Reserve kernel message IDs from application task id pool that can be used to
 * pass kernel messages or to set kernel timers.
 *
 * @param msg_id_count
 * Number of IDs to reserve for caller.
 *
 * @return
 * ID of first allocated kernel message ID.
 */
uint16_t APP_BLE_PeripheralServerRegisterKernelMsgIds(uint8_t msg_id_count);

/**
 * Return if GAPM is Advertising.
 *
 * @return
 * true - Device is Advertising
 * false - Device is not Advertising or in process of canceling of advertisement
 */
bool APP_BLE_PeripheralServerIsAdvertising(void);

/**
 * @brief Initiate GPAC Disconnect all connected peer devices.
 *
 * @param[in] reason disconnect reason. See GAPC_DISCONNECT_CMD documentation
 */
void APP_BLE_PeripheralServerDisconnect(uint8_t reason);

/**
 * Update connection parameters.
 *
 * @param low_latency
 * true: update connection parameters for low latency
 * false: update connection parameters for low power
 *
 */
void APP_BLE_UpdateConnectionParameters(const APP_Connection_Parameters_t conn_params);

/**
 * Return if Device is connected with Low Latency Parameters
 *
 * @return
 * true - device is connected with Low Latency Parameters
 * false - device is not connected with Low Latency Parameters
 *
 */
bool APP_BLE_PeripheralServerConnectedInLowPowerParams(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* APP_BLE_PERIPHERAL_SERVER_H */

/* ----------------------------------------------------------------------------
 * End of File
 * ------------------------------------------------------------------------- */
