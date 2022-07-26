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
 * @file app_ble_estss_int.h
 *
 * External Sensor Trigger Service Server - internal definitions header file
 *
 * This file contains definitions required for ESTSS operation that are only
 * for internal use within the module and not to be exposed to the application.
 */

#ifndef APP_BLE_ESTSS_INT_H
#define APP_BLE_ESTSS_INT_H

#ifdef __cplusplus
extern "C"
{
#endif    /* ifdef __cplusplus */

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/
#include <app_ble_estss.h>

#include <ble_gap.h>
#include <ble_gatt.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

/**
 * Defines internal state of the module.
 */
typedef enum ESTSS_ServiceState_t
{
    /** Module was not initialized yet. */
    ESTSS_STATE_INIT,

    /** Module is initialized and ready to accept trigger values. */
    ESTSS_STATE_IDLE,

    /** Device is connected to a client and able to transmit notifications. */
    ESTSS_STATE_CONNECTED,
} ESTSS_ServiceState_t;

/**
 * List of kernel mesg_id used to schedule kernel timers when a notification
 * for a trigger is postponed due to active Time Trigger setting condition.
 */
typedef enum ESTSS_KernelMsgId_t
{
    ESTSS_MSG_ID_NTF_MOTION,
    ESTSS_MSG_ID_NTF_ACCELERATION,
    ESTSS_MSG_ID_NTF_TEMPERATURE,
    ESTSS_MSG_ID_NTF_HUMIDITY,

    ESTSS_MSG_ID_COUNT,
} ESTSS_KernelMsgId_t;

/**
 * Stores all variables related to to trigger operation and Value Trigger
 * setting and Time Trigger Setting descriptor.
 */
typedef struct ESTS_TriggerSetting_t
{
    /** Attribute value of the Value Trigger setting descriptor. */
    uint8_t value[ESTSS_CHAR_TRIGGER_VALUE_SIZE];

    /** Attribute value of the Time Trigger setting descriptor. */
    uint8_t time[ESTSS_CHAR_TRIGGER_TIME_SIZE];

    /**
     * Set to true if reporting of trigger events is enabled for this
     * trigger.
     *
     * Trigger can be enabled either by reading the Value Trigger characteristic
     * value or by enabling notifications on the characteristic.
     *
     * Trigger is automatically disabled when connection disconnects or when
     * characteristic notifications are disabled.
     */
    bool enabled;

    /**
     * Prevents re-evaluation of trigger conditions if they were already met
     * previously but notification cannot be sent immediately.
     */
    bool ntf_pending;

    /**
     * System time of last notification transmission in milliseconds.
     */
    uint32_t ntf_last_timestamp;

    /**
     * Minimal time between two notifications in milliseconds when specific
     * Time Trigger settings are active.
     *
     * Set to 0 to disable notification limiting.
     */
    uint32_t ntf_min_interval;
} ESTS_TriggerSetting_t;

/**
 * Stores all variables required for Value Trigger characteristic.
 */
typedef struct ESTSS_Characteristic_t
{
    /** Data storage for Value Trigger characteristic value. */
    uint8_t value[ESTSS_CHAR_VALUE_SIZE];

    /** Data storage for Client Characteristic Configuration descriptor. */
    uint8_t ccc[ESTSS_CHAR_CCC_SIZE];

    /** Data storage for Characteristic Presentation Format descriptor. */
    uint8_t fmt[ESTSS_CHAR_FORMAT_SIZE];

    /** Data storage for Value Trigger setting descriptor, Time Trigger setting
     * descriptor and trigger control variables.
     */
    ESTS_TriggerSetting_t trig;
} ESTSS_Characteristic_t;

/**
 * Collects all attribute database related variables of Picture Transfer
 * Service.
 */
typedef struct ESTSS_AttDb_t
{
    /**
     * Offset of the attidx of ESTSS service attributes.
     *
     * This can change depending on number and order of registered custom
     * services in the shared attribute database.
     */
    uint16_t attidx_offset;

    /**
     * Storage for the attributes of all supported Value Trigger
     * characteristics.
     */
    ESTSS_Characteristic_t trigger[ESTSS_TRIGGER_COUNT];
} ESTSS_AttDb_t;

/**
 * Main storage structure that holds all variables of the ESTSS module.
 */
typedef struct ESTSS_Environment_t
{
    /** Stores all attribute and attribute database related variables. */
    ESTSS_AttDb_t att;

    /**
     * Application callback function called when trigger configuration changes.
     */
    ESTSS_TriggerUpdatedCallback_t p_update_cb;

    /**
     * Application callback for retrieving of system time timestamps.
     */
    ESTSS_TimeCallbackMs_t p_time_cb;

    /**
     * Initialization state of the module.
     */
    ESTSS_ServiceState_t state;
} ESTSS_Environment_t;

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* APP_BLE_ESTSS_INT_H */
