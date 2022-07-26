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
 * @file app_ble_estss.h
 *
 * External Sensor Trigger Service Server - header file
 *
 * Defines custom BLE service for reporting of various trigger events to any
 * connected client device.
 *
 * This implementation supports the following sensor triggers:
 *
 * - Motion Trigger <br>
 *   Reports events from a motion sensor (PIR) that occur within the field of
 *   view of the device.
 *
 *   Supported Value trigger conditions:
 *
 *   - No Value trigger
 *   - Value Changed trigger
 *   - On Boundary trigger
 *
 *   Supported Time trigger conditions:
 *
 *   - No Time trigger
 *   - Minimal Time Interval condition
 *
 * - Acceleration Trigger <br>
 *   Reports motion events of the board itself based on an acceleration
 *   threshold.
 *
 *   Supported Value trigger conditions:
 *
 *   - No Value trigger
 *   - Value Changed trigger
 *
 *   Supported Time trigger conditions:
 *
 *   - No Time trigger
 *   - Minimal Time Interval condition
 *
 * - Temperature Trigger <br>
 *   Reports temperature change events based on configured temperature
 *   threshold.
 *
 *   Supported Value trigger conditions:
 *
 *   - No Value trigger
 *   - Value Changed trigger
 *   - Value Crossed Boundary trigger
 *   - Value Crossed Interval trigger
 *
 *   Supported Time trigger conditions:
 *
 *   - No Time trigger
 *   - Minimal Time Interval condition
 *
 * - Humidity Trigger <br>
 *   Reports relative humidity change events based on configured humidity
 *   threshold.
 *
 * @see External Sensor Trigger Service specification included with
 *      the CMSIS-Pack.
 *
 * @see https://www.bluetooth.com/specifications/gatt/
 *      Automation IO Service (AIOS) specification provided by Bluetooth SIG
 *      that describes the operation of Value Trigger setting descriptor
 *      and Time Trigger setting descriptor.
 */

#ifndef APP_BLE_ESTSS_H
#define APP_BLE_ESTSS_H

#ifdef __cplusplus
extern "C"
{
#endif /* ifdef __cplusplus */

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/

#include <rsl10_ke.h>
#include <gattc_task.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

/** 128-bit UUID for the External Sensor Trigger Service */
#define ESTS_SVC_UUID \
    { 0xF8, 0x85, 0x74, 0xD2, 0x2D, 0x01, \
      0xDA, 0xB5, \
      0x62, 0x03, \
      0x01, 0x00, \
      0x05, 0x00, 0x00, 0x00 }

/** 128-bit UUID for the Motion Trigger Characteristic */
#define ESTS_CHAR_MOTION_TRIGGER_UUID \
    { 0xF8, 0x85, 0x74, 0xD2, 0x2D, 0x01, \
      0xDA, 0xB5, \
      0x62, 0x03, \
      0x03, 0x00, \
      0x05, 0x00, 0x00, 0x00 }

/** 128-bit UUID for the Acceleration Trigger Characteristic */
#define ESTS_CHAR_ACCELERATION_TRIGGER_UUID \
    { 0xF8, 0x85, 0x74, 0xD2, 0x2D, 0x01, \
      0xDA, 0xB5, \
      0x62, 0x03, \
      0x04, 0x00, \
      0x05, 0x00, 0x00, 0x00 }

/** 128-bit UUID for the Temperature Trigger Characteristic */
#define ESTS_CHAR_TEMPERATURE_TRIGGER_UUID \
    { 0xF8, 0x85, 0x74, 0xD2, 0x2D, 0x01, \
      0xDA, 0xB5, \
      0x62, 0x03, \
      0x05, 0x00, \
      0x05, 0x00, 0x00, 0x00 }

/** 128-bit UUID for the Mumidity Trigger Characteristic */
#define ESTS_CHAR_HUMIDITY_TRIGGER_UUID \
    { 0xF8, 0x85, 0x74, 0xD2, 0x2D, 0x01, \
      0xDA, 0xB5, \
      0x62, 0x03, \
      0x06, 0x00, \
      0x05, 0x00, 0x00, 0x00 }

/** Size of all ESTSS characteristic values. */
#define ESTSS_CHAR_VALUE_SIZE          (4)

/** Size of the Client Characteristic Configuration descriptor. */
#define ESTSS_CHAR_CCC_SIZE            (2)

/** Size of the Characteristic Presentation Format descriptor. */
#define ESTSS_CHAR_FORMAT_SIZE         (7)

/** Maximum size of Value Trigger Setting descriptors used by ESTS.
 *
 * 1-octet for opcode
 * 4-octets for threshold / low interval border
 * 4-octets for high interval border
 */
#define ESTSS_CHAR_TRIGGER_VALUE_SIZE  (1 + (2 * ESTSS_CHAR_VALUE_SIZE))

/** Maximum size of Time Trigger Setting descriptors used by ESTS.
 *
 * 1-octet for opcode
 * 3-octets to fit the Time Interval value format (uint24) or counter (uint16)
 */
#define ESTSS_CHAR_TRIGGER_TIME_SIZE   (4)

#define ESTSS_CHAR_MOTION_DESC       "Motion Trigger"
#define ESTSS_CHAR_ACCELERATION_DESC "Acceleration Trigger"
#define ESTSS_CHAR_TEMPERATURE_DESC  "Temperature Trigger"
#define ESTSS_CHAR_HUMIDITY_DESC     "Humidity Trigger"

/**
 * List of Value Trigger Setting conditions.
 *
 * @see Automation IO Service specification for trigger condition details.
 */
typedef enum ESTSS_ValueTriggerSetting_t
{
    ESTS_TRIG_VAL_CHANGED           = 0x00,
    ESTS_TRIG_VAL_CROSSED_BOUNDARY  = 0x01,
    ESTS_TRIG_VAL_ON_BOUNDARY       = 0x02,
    ESTS_TRIG_VAL_CHANGED_MORE_THAN = 0x03,
    ESTS_TRIG_VAL_MASK_COMPARE      = 0x04,
    ESTS_TRIG_VAL_CRROSSED_INTERVAL = 0x05,
    ESTS_TRIG_VAL_ON_INTERVAL       = 0x06,
    ESTS_TRIG_VAL_NO_TRIGGER        = 0x07,
} ESTSS_ValueTriggerSetting_t;

/**
 * List of Time Trigger Setting conditions.
 *
 * @see Automation IO Service specification for trigger condition details.
 */
typedef enum ESTSS_TimeTriggerSetting_t
{
    ESTS_TRIG_TIME_NO_TRIGGER      = 0x00,
    ESTS_TRIG_TIME_PERIODIC        = 0x01,
    ESTS_TRIG_TIME_MIN_INTERVAL    = 0x02,
    ESTS_TRIG_TIME_ON_CHANGE_COUNT = 0x03,
} ESTSS_TimeTriggerSetting_t;

/**
 * List of Attribute Protocol Application Error codes defined by ESTS.
 */
typedef enum ESTSS_AttErr_t
{
    /**
     * An attempt was made to configure a trigger condition value not supported
     * by this ESTS server.
     */
    ATT_ERR_ESTS_TRIGGER_NOT_SUPPORTED = 0x80,
} ESTSS_AttErr_t;

/**
 * List of all attributes supported by this by ESTS server.
 */
typedef enum ESTSS_AttIdx_t
{
    /* External Sensor Trigger Service 0 */
    ESTSS_ATT_SERVICE_0,

    /* Motion Trigger Characteristic */
    ESTSS_ATT_MOTION_CHAR_0,
    ESTSS_ATT_MOTION_VAL_0,
    ESTSS_ATT_MOTION_CCC_0,
    ESTSS_ATT_MOTION_TRIGGER_VALUE_0,
    ESTSS_ATT_MOTION_TRIGGER_TIME_0,
    ESTSS_ATT_MOTION_FMT_0,
    ESTSS_ATT_MOTION_DESC_0,

    /* Acceleration Trigger Characteristic */
    ESTSS_ATT_ACCELERATION_CHAR_0,
    ESTSS_ATT_ACCELERATION_VAL_0,
    ESTSS_ATT_ACCELERATION_CCC_0,
    ESTSS_ATT_ACCELERATION_TRIGGER_VALUE_0,
    ESTSS_ATT_ACCELERATION_TRIGGER_TIME_0,
    ESTSS_ATT_ACCELERATION_FMT_0,
    ESTSS_ATT_ACCELERATION_DESC_0,

    /* Temperature Trigger Characteristic */
    ESTSS_ATT_TEMPERATURE_CHAR_0,
    ESTSS_ATT_TEMPERATURE_VAL_0,
    ESTSS_ATT_TEMPERATURE_CCC_0,
    ESTSS_ATT_TEMPERATURE_TRIGGER_VALUE_0,
    ESTSS_ATT_TEMPERATURE_TRIGGER_TIME_0,
    ESTSS_ATT_TEMPERATURE_FMT_0,
    ESTSS_ATT_TEMPERATURE_DESC_0,

    /* Humidity Trigger Characteristic */
    ESTSS_ATT_HUMIDITY_CHAR_0,
    ESTSS_ATT_HUMIDITY_VAL_0,
    ESTSS_ATT_HUMIDITY_CCC_0,
    ESTSS_ATT_HUMIDITY_TRIGGER_VALUE_0,
    ESTSS_ATT_HUMIDITY_TRIGGER_TIME_0,
    ESTSS_ATT_HUMIDITY_FMT_0,
    ESTSS_ATT_HUMIDITY_DESC_0,

    /* Total number of all custom attributes of ESTSS. */
    ESTSS_ATT_COUNT,
} ESTSS_AttIdx_t;

/** Number of attributes associated with each Value Trigger characteristic. */
#define ESTSS_CHAR_ATT_COUNT (7)

/** List of all sensor triggers supported by this implementation of ESTS. */
typedef enum ESTSS_TriggerId_t
{
    ESTSS_TRIGGER_MOTION,
    ESTSS_TRIGGER_ACCELERATION,
    ESTSS_TRIGGER_TEMPERATURE,
    ESTSS_TRIGGER_HUMIDITY,

    /* Total number of available triggers. */
    ESTSS_TRIGGER_COUNT,
} ESTSS_TriggerId_t;

/**
 * Exposes trigger configuration to the application to allow for customization
 * of sensor parameters.
 */
typedef struct ESTSS_TriggerSettings_t
{
    /** Value Trigger setting condition applied to the trigger. */
    ESTSS_ValueTriggerSetting_t cond_value;

    /** Time Trigger setting condition applied to the trigger. */
    ESTSS_TimeTriggerSetting_t cond_time;

    /** Configured value interval for Value Trigger setting.
     *
     * First boundary is valid for these #cond_value values:
     * - ESTS_TRIG_VAL_CROSSED_BOUNDARY
     * - ESTS_TRIG_VAL_ON_BOUNDARY
     * - ESTS_TRIG_VAL_CHANGED_MORE_THAN
     * - ESTS_TRIG_VAL_CRROSSED_INTERVAL
     * - ESTS_TRIG_VAL_ON_INTERVAL
     *
     * Second boundary is valid for these #cond_value values:
     * - ESTS_TRIG_VAL_CRROSSED_INTERVAL
     * - ESTS_TRIG_VAL_ON_INTERVAL
     */
    int32_t boundary[2];

    /** Configured time interval of Time Trigger Setting in milliseconds.
     *
     * Time interval value is valid for these #cond_time values:
     * - ESTS_TRIG_TIME_PERIODIC
     * - ESTS_TRIG_TIME_MIN_INTERVAL
     */
    uint32_t time_interval;
} ESTSS_TriggerSettings_t;

/**
 * Event callback used to notify application that trigger configuration was
 * changed.
 *
 * @param tidx
 * ID of the trigger with configuration change.
 *
 * @see ESTSS_TriggerIsEnabled to check is trigger is enabled or disabled.
 * @see ESTSS_GetTriggerSettings to retreive trigger configuration if trigger
 *      is enabled.
 */
typedef void (*ESTSS_TriggerUpdatedCallback_t)(ESTSS_TriggerId_t tidx);

/**
 * Callback used to retrieve current system time to allow for Time Trigger
 * setting operation.
 *
 * @return
 * Current system time in milliseconds.
 */
typedef uint32_t (*ESTSS_TimeCallbackMs_t)(void);

/* ----------------------------------------------------------------------------
 * Function definitions
 * --------------------------------------------------------------------------*/

/**
 * Initialize the External Sensor Trigger Service Server.
 *
 * Sets initial values of all service related characteristics and descriptors.
 * All service related attributes are then added to attribute database.
 *
 * @pre
 * The Peripheral Server library was initialized with sufficient attribute
 * database size using APP_BLE_PeripheralServerInitialize .
 *
 * @param p_update_cb
 * Application provided callback that will be called when trigger configuration
 * has changed.
 *
 * @param p_time_cb
 * Application provided callback function to provide system time.
 *
 * @return
 * 0  - On success. <br>
 * -1 - Attribute database does not have enough free space to fit all ESTSS
 *      attributes.
 */
int32_t ESTSS_Initialize(ESTSS_TriggerUpdatedCallback_t p_update_cb,
        ESTSS_TimeCallbackMs_t p_time_cb);

/**
 * Checks if given sensor trigger is enabled or disabled.
 *
 * Should be called from Trigger Updated callback to update sensor configuration.
 *
 * @param tidx
 * ID of the trigger to check.
 *
 * @return
 * true - Given trigger is enabled. <br>
 * false - Given trigger is disabled.
 */
bool ESTSS_TriggerIsEnabled(ESTSS_TriggerId_t tidx);

/**
 * Gets current configuration of sensor trigger based on Value Trigger setting
 * and Time Trigger setting descriptor values.
 *
 * @param tidx
 * ID of the trigger to check.
 *
 * @param p_settings
 * Pointer to settings structure to fill with trigger configuration.
 */
void ESTSS_GetTriggerSettings(ESTSS_TriggerId_t tidx,
        ESTSS_TriggerSettings_t *p_settings);

/**
 * Set new sensor value for the Motion Trigger.
 *
 * @param motion_state
 * Set to `true` to indicate start of motion detection event.
 * Set to `false` to indicate end of motion detection event.
 */
void ESTSS_PushMotionValue(bool motion_state);

/**
 * Set new sensor value for the Acceleration Trigger.
 *
 * @param acceleration_state
 * Set to `true` to indicate start of acceleration motion event.
 * Set to `false` to indicate end of acceleration motion event.
 */
void ESTSS_PushAccelerationValue(bool acceleration_state);

/**
 * Set new sensor value for the Temperature Trigger.
 *
 * @param temperature
 * New measured temperature value in degrees Celsius x100.
 *
 * Example:
 * To report temperature of 24,50 degC set @p temperature to 2450.
 * To report temperature of -5,76 degC set @p temperature to -576.
 */
void ESTSS_PushTemperatureValue(int32_t temperature);

/**
 * Set new sensor value for the Humidity Trigger.
 *
 * @param humidity
 * New measured relative humidity value in Percent x100.
 *
 * Example:
 * To report humidity of 46,85% set the @p humidity to 4685.
 */
void ESTSS_PushHumidityValue(int32_t humidity);

#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* APP_BLE_ESTSS_H */
