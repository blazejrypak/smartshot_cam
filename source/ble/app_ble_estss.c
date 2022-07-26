//-----------------------------------------------------------------------------
// Copyright (c) 2019 Semiconductor Components Industries LLC
// (d/b/a "ON Semiconductor").  All rights reserved.
// This software and/or documentation is licensed by ON Semiconductor under
// limited terms and conditions.  The terms and conditions pertaining to the
// software and/or documentation are available at
// http://www.onsemi.com/site/pdf/ONSEMI_T&C.pdf ("ON Semiconductor Standard
// Terms and Conditions of Sale, Section 8 Software") and if applicable the
// software license agreement.  Do not use this software and/or documentation
// unless you have carefully read and you agree to the limited terms and
// conditions.  By using this software and/or documentation, you agree to the
// limited terms and conditions.
//-----------------------------------------------------------------------------

#include <app_ble_estss_int.h>
#include <app_ble_peripheral_server.h>
#include <msg_handler.h>

#include <smartshot_assert.h>
#include <smartshot_printf.h>

/* Stores one copy of filename for ASSERT calls. */
DEFINE_THIS_FILE_FOR_ASSERT;

static uint8_t ESTSS_TriggerValueReadHandler(uint8_t conidx, uint16_t attidx,
        uint16_t handle, uint8_t *to, const uint8_t *from, uint16_t length,
        uint16_t operation);

static uint8_t ESTSS_TriggerCCCUpdateHandler(uint8_t conidx, uint16_t attidx,
        uint16_t handle, uint8_t *to, const uint8_t *from, uint16_t length,
        uint16_t operation);

static uint8_t ESTSS_ValueTriggerSettingUpdateHandler(uint8_t conidx,
        uint16_t attidx, uint16_t handle, uint8_t *to, const uint8_t *from,
        uint16_t length, uint16_t operation);

static uint8_t ESTSS_TimeTriggerSettingUpdateHandler(uint8_t conidx,
        uint16_t attidx, uint16_t handle, uint8_t *to, const uint8_t *from,
        uint16_t length, uint16_t operation);

static ESTSS_Environment_t estss_env;

/** Complete attribute database of the ESTSS. */
const static struct att_db_desc estss_att_db[ESTSS_ATT_COUNT] =
{
    /* External Sensor Trigger Service 0 */
    CS_SERVICE_UUID_128(
            ESTSS_ATT_SERVICE_0, /* attidx */
            ESTS_SVC_UUID),      /* uuid */

    /* Motion Trigger Characteristic */
    CS_CHAR_UUID_128(
            ESTSS_ATT_MOTION_CHAR_0,                           /* attidx_char */
            ESTSS_ATT_MOTION_VAL_0,                            /* attidx_val */
            ESTS_CHAR_MOTION_TRIGGER_UUID,                      /* uuid */
            PERM(RD, ENABLE) | PERM(NTF, ENABLE),              /* perm */
            ESTSS_CHAR_VALUE_SIZE,                             /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_MOTION].value, /* data */
            ESTSS_TriggerValueReadHandler),                    /* callback */

    CS_CHAR_CCC(
            ESTSS_ATT_MOTION_CCC_0,                            /* attidx */
            estss_env.att.trigger[ESTSS_TRIGGER_MOTION].ccc,   /* data */
            ESTSS_TriggerCCCUpdateHandler),                    /* callback */

    CS_CHAR_VALUE_TRIGGER(
            ESTSS_ATT_MOTION_TRIGGER_VALUE_0,                       /* attidx */
            ESTSS_CHAR_TRIGGER_VALUE_SIZE,                          /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_MOTION].trig.value, /* data */
            ESTSS_ValueTriggerSettingUpdateHandler),                /* callback */

    CS_CHAR_TIME_TRIGGER(
            ESTSS_ATT_MOTION_TRIGGER_TIME_0,                        /* attidx */
            ESTSS_CHAR_TRIGGER_TIME_SIZE,                           /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_MOTION].trig.time,  /* data */
            ESTSS_TimeTriggerSettingUpdateHandler),                 /* callback */

    CS_CHAR_PRESENTATION_FORMAT(
            ESTSS_ATT_MOTION_FMT_0,                                 /* attidx */
            ESTSS_CHAR_FORMAT_SIZE,                                 /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_MOTION].fmt,        /* data */
            NULL),                                                  /* callback */

    CS_CHAR_USER_DESC(
            ESTSS_ATT_MOTION_DESC_0,              /* attidx */
            (sizeof(ESTSS_CHAR_MOTION_DESC) - 1), /* length */
            ESTSS_CHAR_MOTION_DESC,               /* data */
            NULL),                                /* callback */

    /* Acceleration Trigger Characteristic */
    CS_CHAR_UUID_128(
            ESTSS_ATT_ACCELERATION_CHAR_0,                           /* attidx_char */
            ESTSS_ATT_ACCELERATION_VAL_0,                            /* attidx_val */
            ESTS_CHAR_ACCELERATION_TRIGGER_UUID,                     /* uuid */
            PERM(RD, ENABLE) | PERM(NTF, ENABLE),                    /* perm */
            ESTSS_CHAR_VALUE_SIZE,                                   /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_ACCELERATION].value, /* data */
            ESTSS_TriggerValueReadHandler),                          /* callback */

    CS_CHAR_CCC(
            ESTSS_ATT_ACCELERATION_CCC_0,                          /* attidx */
            estss_env.att.trigger[ESTSS_TRIGGER_ACCELERATION].ccc, /* data */
            ESTSS_TriggerCCCUpdateHandler),                        /* callback */

    CS_CHAR_VALUE_TRIGGER(
            ESTSS_ATT_ACCELERATION_TRIGGER_VALUE_0,                       /* attidx */
            ESTSS_CHAR_TRIGGER_VALUE_SIZE,                                /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_ACCELERATION].trig.value, /* data */
            ESTSS_ValueTriggerSettingUpdateHandler),                      /* callback */

    CS_CHAR_TIME_TRIGGER(
            ESTSS_ATT_ACCELERATION_TRIGGER_TIME_0,                       /* attidx */
            ESTSS_CHAR_TRIGGER_TIME_SIZE,                                /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_ACCELERATION].trig.time, /* data */
            ESTSS_TimeTriggerSettingUpdateHandler),                      /* callback */

    CS_CHAR_PRESENTATION_FORMAT(
            ESTSS_ATT_ACCELERATION_FMT_0,                          /* attidx */
            ESTSS_CHAR_FORMAT_SIZE,                                /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_ACCELERATION].fmt, /* data */
            NULL),                                                 /* callback */

    CS_CHAR_USER_DESC(
            ESTSS_ATT_ACCELERATION_DESC_0,              /* attidx */
            (sizeof(ESTSS_CHAR_ACCELERATION_DESC) - 1), /* length */
            ESTSS_CHAR_ACCELERATION_DESC,               /* data */
            NULL),                                      /* callback */

    /* Temperature Trigger Characteristic */
    CS_CHAR_UUID_128(
            ESTSS_ATT_TEMPERATURE_CHAR_0,                           /* attidx_char */
            ESTSS_ATT_TEMPERATURE_VAL_0,                            /* attidx_val */
            ESTS_CHAR_TEMPERATURE_TRIGGER_UUID,                     /* uuid */
            PERM(RD, ENABLE) | PERM(NTF, ENABLE),                   /* perm */
            ESTSS_CHAR_VALUE_SIZE,                                  /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_TEMPERATURE].value, /* data */
            ESTSS_TriggerValueReadHandler),                         /* callback */

    CS_CHAR_CCC(
            ESTSS_ATT_TEMPERATURE_CCC_0,                          /* attidx */
            estss_env.att.trigger[ESTSS_TRIGGER_TEMPERATURE].ccc, /* data */
            ESTSS_TriggerCCCUpdateHandler),                       /* callback */

    CS_CHAR_VALUE_TRIGGER(
            ESTSS_ATT_TEMPERATURE_TRIGGER_VALUE_0,                       /* attidx */
            ESTSS_CHAR_TRIGGER_VALUE_SIZE,                               /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_TEMPERATURE].trig.value, /* data */
            ESTSS_ValueTriggerSettingUpdateHandler),                     /* callback */

    CS_CHAR_TIME_TRIGGER(
            ESTSS_ATT_TEMPERATURE_TRIGGER_TIME_0,                       /* attidx */
            ESTSS_CHAR_TRIGGER_TIME_SIZE,                               /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_TEMPERATURE].trig.time, /* data */
            ESTSS_TimeTriggerSettingUpdateHandler),

    CS_CHAR_PRESENTATION_FORMAT(
            ESTSS_ATT_TEMPERATURE_FMT_0,                          /* attidx */
            ESTSS_CHAR_FORMAT_SIZE,                               /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_TEMPERATURE].fmt, /* data */
            NULL),                                                /* callback */

    CS_CHAR_USER_DESC(
            ESTSS_ATT_TEMPERATURE_DESC_0,              /* attidx */
            (sizeof(ESTSS_CHAR_TEMPERATURE_DESC) - 1), /* length */
            ESTSS_CHAR_TEMPERATURE_DESC,               /* data */
            NULL),                                     /* callback */

    /* Humidity Trigger Characteristic */
    CS_CHAR_UUID_128(
            ESTSS_ATT_HUMIDITY_CHAR_0,                           /* attidx_char */
            ESTSS_ATT_HUMIDITY_VAL_0,                            /* attidx_val */
            ESTS_CHAR_HUMIDITY_TRIGGER_UUID,                     /* uuid */
            PERM(RD, ENABLE) | PERM(NTF, ENABLE),                /* perm */
            ESTSS_CHAR_VALUE_SIZE,                               /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_HUMIDITY].value, /* data */
            ESTSS_TriggerValueReadHandler),                      /* callback */

    CS_CHAR_CCC(
            ESTSS_ATT_HUMIDITY_CCC_0,                          /* attidx */
            estss_env.att.trigger[ESTSS_TRIGGER_HUMIDITY].ccc, /* data */
            ESTSS_TriggerCCCUpdateHandler),                    /* callback */

    CS_CHAR_VALUE_TRIGGER(
            ESTSS_ATT_HUMIDITY_TRIGGER_VALUE_0,                       /* attidx */
            ESTSS_CHAR_TRIGGER_VALUE_SIZE,                            /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_HUMIDITY].trig.value, /* data */
            ESTSS_ValueTriggerSettingUpdateHandler),                  /* callback */

    CS_CHAR_TIME_TRIGGER(
            ESTSS_ATT_HUMIDITY_TRIGGER_TIME_0,                       /* attidx */
            ESTSS_CHAR_TRIGGER_TIME_SIZE,                            /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_HUMIDITY].trig.time, /* data */
            ESTSS_TimeTriggerSettingUpdateHandler),                  /* callback */

    CS_CHAR_PRESENTATION_FORMAT(
            ESTSS_ATT_HUMIDITY_FMT_0,                          /* attidx */
            ESTSS_CHAR_FORMAT_SIZE,                            /* length */
            estss_env.att.trigger[ESTSS_TRIGGER_HUMIDITY].fmt, /* data */
            NULL),                                             /* callback */

    CS_CHAR_USER_DESC(
            ESTSS_ATT_HUMIDITY_DESC_0,              /* attidx */
            (sizeof(ESTSS_CHAR_HUMIDITY_DESC) - 1), /* length */
            ESTSS_CHAR_HUMIDITY_DESC,               /* data */
            NULL),                                  /* callback */

};

/**
 * This atlas defines list of supported Value Trigger setting conditions for
 * each Value Trigger characteristic.
 */
const static uint16_t estss_value_trigger_setting_atlas[ESTSS_TRIGGER_COUNT] =
{
    [ESTSS_TRIGGER_MOTION] = (1 << ESTS_TRIG_VAL_CHANGED)
                             | (1 << ESTS_TRIG_VAL_ON_BOUNDARY)
                             | (1 << ESTS_TRIG_VAL_NO_TRIGGER),
    [ESTSS_TRIGGER_ACCELERATION] = (1 << ESTS_TRIG_VAL_CHANGED)
                                   | (1 << ESTS_TRIG_VAL_ON_BOUNDARY)
                                   | (1 << ESTS_TRIG_VAL_NO_TRIGGER),
    [ESTSS_TRIGGER_TEMPERATURE] = (1 << ESTS_TRIG_VAL_CHANGED)
                                  | (1 << ESTS_TRIG_VAL_CROSSED_BOUNDARY)
                                  | (1 << ESTS_TRIG_VAL_CRROSSED_INTERVAL)
                                  | (1 << ESTS_TRIG_VAL_NO_TRIGGER),
    [ESTSS_TRIGGER_HUMIDITY] = (1 << ESTS_TRIG_VAL_CHANGED)
                               | (1 << ESTS_TRIG_VAL_CROSSED_BOUNDARY)
                               | (1 << ESTS_TRIG_VAL_CRROSSED_INTERVAL)
                               | (1 << ESTS_TRIG_VAL_NO_TRIGGER),
};

/**
 * Converts attribute id given by BLE stack to trigger id that is associated
 * with that attribute.
 *
 * @param attidx
 * Attribute ID of one of the ESTSS attributes.
 *
 * @return
 * Trigger ID that given attribute belongs to.
 */
static ESTSS_TriggerId_t ESTSS_GetTriggerIdFromAttidx(uint16_t attidx)
{
    REQUIRE(attidx > estss_env.att.attidx_offset);
    REQUIRE(attidx < (estss_env.att.attidx_offset + ESTSS_ATT_COUNT));

    uint16_t estss_attidx = attidx - estss_env.att.attidx_offset;
    ESTSS_TriggerId_t tidx;

    if (estss_attidx < ESTSS_ATT_ACCELERATION_CHAR_0)
    {
        tidx = ESTSS_TRIGGER_MOTION;
    }
    else if (estss_attidx < ESTSS_ATT_TEMPERATURE_CHAR_0)
    {
        tidx = ESTSS_TRIGGER_ACCELERATION;
    }
    else if (estss_attidx < ESTSS_ATT_HUMIDITY_CHAR_0)
    {
        tidx = ESTSS_TRIGGER_TEMPERATURE;
    }
    else
    {
        tidx = ESTSS_TRIGGER_HUMIDITY;
    }

    ENSURE(tidx < ESTSS_TRIGGER_COUNT);
    return tidx;
}

/**
 * Used to notify application when trigger mode changes state from/to
 * enabled/disabled states.
 *
 * Application can enable/disable underlying sensor hardware.
 *
 * @param tidx
 * Trigger ID of trigger that was enabled or disabled.
 *
 * @param enabled
 * New state of the trigger.
 */
static void ESTSS_ConfigureTriggerSource(ESTSS_TriggerId_t tidx, bool enabled)
{
    ESTSS_Characteristic_t *p_char = estss_env.att.trigger + tidx;

    /* Trigger configuration was changed while trigger is enabled. */
    if ((p_char->trig.enabled) && (enabled))
    {
        PRINTF("ESTSS: Updating sensor configuration. tidx=%d\r\n", tidx);

        estss_env.p_update_cb(tidx);
    }
    else
    {
        /* Trigger state was changed. */
        if (p_char->trig.enabled != enabled)
        {
            PRINTF("ESTSS: Changing sensor state. tidx=%d enabled=%d\r\n", tidx,
                    enabled);

            p_char->trig.enabled = enabled;
            p_char->trig.ntf_pending = false;

            estss_env.p_update_cb(tidx);
        }
    }
}

/**
 * Checks if value has crossed given boundary threshold during an Value Push
 * operation.
 *
 * @param old_value
 * Trigger value from before value push was initiated.
 *
 * @param new_value
 * New trigger value that is being stored in this push operation.
 *
 * @param boundary
 * Trigger value threshold.
 *
 * @return
 * true - If the value has crossed the boundary in either low to high or high
 *        to low direction.
 * false - If value has not crossed the boundary.
 */
static bool ESTSS_BoundaryCheck(int32_t old_value, int32_t new_value,
        int32_t boundary)
{
    /* Check low to high boundary transition. */
    if ((old_value <= boundary) && (new_value > boundary))
    {
        return true;
    }

    /* Check high to low boundary transition. */
    if ((old_value >= boundary) && (new_value < boundary))
    {
        return true;
    }

    return false;
}

/**
 * Evaluates the configured Value Trigger setting condition to determine if
 * Notification should be generated or not.
 *
 * @param p_char
 * Pointer to the Value Trigger characteristic structure.
 *
 * @param value
 * New trigger value.
 *
 * @param old_value
 * Previous trigger value.
 *
 * @return
 * true - The change of characteristic value satisfied the set trigger
 *        condition. <br>
 * false - New value did not trigger the configured value condition.
 */
static bool ESTSS_EvaluateValueTrigger(ESTSS_Characteristic_t *p_char,
        int32_t value, int32_t old_value)
{
    ENSURE(p_char != NULL);

    bool is_triggered = false;

    switch (p_char->trig.value[0])
    {
        case ESTS_TRIG_VAL_CHANGED:
        {
            if (value != old_value)
            {
                is_triggered = true;
            }
            break;
        }

        case ESTS_TRIG_VAL_ON_BOUNDARY:
        {
            int32_t boundary;
            memcpy(&boundary, p_char->trig.value + 1, ESTSS_CHAR_VALUE_SIZE);

            if (value == boundary)
            {
                is_triggered = true;
            }
            break;
        }

        case ESTS_TRIG_VAL_CROSSED_BOUNDARY:
        {
            int32_t boundary;
            memcpy(&boundary, p_char->trig.value + 1, ESTSS_CHAR_VALUE_SIZE);

            if (ESTSS_BoundaryCheck(old_value, value, boundary) == true)
            {
                is_triggered = true;
            }
            break;
        }

        case ESTS_TRIG_VAL_CRROSSED_INTERVAL:
        {
            int32_t limit_low;
            int32_t limit_high;

            memcpy(&limit_low, p_char->trig.value + 1, ESTSS_CHAR_VALUE_SIZE);
            memcpy(&limit_high,
                    p_char->trig.value + (1 + ESTSS_CHAR_VALUE_SIZE),
                    ESTSS_CHAR_VALUE_SIZE);

            if ((ESTSS_BoundaryCheck(old_value, value, limit_low) == true)
                || (ESTSS_BoundaryCheck(old_value, value, limit_high) == true))
            {
                is_triggered = true;
            }

            break;
        }

        case ESTS_TRIG_VAL_ON_INTERVAL:
        {
            int32_t limit_low;
            int32_t limit_high;

            memcpy(&limit_low, p_char->trig.value + 1, ESTSS_CHAR_VALUE_SIZE);
            memcpy(&limit_high,
                    p_char->trig.value + (1 + ESTSS_CHAR_VALUE_SIZE),
                    ESTSS_CHAR_VALUE_SIZE);

            if ((value == limit_low) || (value == limit_high))
            {
                is_triggered = true;
            }

            break;
        }

        case ESTS_TRIG_VAL_NO_TRIGGER:
            /* Trigger disabled. */
            break;

        default:
            ASSERT(false);
            break;
    }

    return is_triggered;
}

/**
 * Evaluates the configured Time Trigger setting condition to determine if
 * Notification should be generated or not.
 *
 * @param tidx
 * Trigger ID of the trigger to evaluate.
 *
 * @return
 * true - Configured Time Trigger condition is met and the device is allowed to
 *        transmit notification. <br>
 * false - Time Trigger condition is not satisfied and device should not
 *        transmit notification at this moment.
 *        A delayed notification may be scheduled if number of notifications
 *        is limited.
 */
static bool ESTSS_EvaluateTimeTrigger(ESTSS_TriggerId_t tidx)
{
    ENSURE(tidx < ESTSS_TRIGGER_COUNT);

    bool is_triggered = false;

    ESTSS_Characteristic_t *p_char = estss_env.att.trigger + tidx;

    /* Time elapsed since last notification was sent. */
    uint32_t since_last_ntf = estss_env.p_time_cb()
                              - p_char->trig.ntf_last_timestamp;

    switch (p_char->trig.time[0])
    {
        case ESTS_TRIG_TIME_NO_TRIGGER:
            is_triggered = true;
            break;

        case ESTS_TRIG_TIME_MIN_INTERVAL:
            if (since_last_ntf >= p_char->trig.ntf_min_interval)
            {
                is_triggered = true;
            }
            break;

        default:
            break;
    }

    return is_triggered;
}

/**
 * Transmits Value Trigger characteristic notification with current trigger
 * value.
 *
 * @param tidx
 * Trigger ID of the trigger.
 */
static void ESTSS_NotifyValue(ESTSS_TriggerId_t tidx)
{
    REQUIRE(estss_env.state == ESTSS_STATE_CONNECTED);

    REQUIRE(tidx < ESTSS_TRIGGER_COUNT);

    ESTSS_Characteristic_t *p_char = estss_env.att.trigger + tidx;

    /* Clear notify pending flag and update last notify timestamp. */
    p_char->trig.ntf_pending = false;
    p_char->trig.ntf_last_timestamp = estss_env.p_time_cb();

    /* Calculate attidx of the characteristic's value attribute. */
    uint16_t attidx = estss_env.att.attidx_offset
                      + ESTSS_ATT_MOTION_VAL_0
                      + (tidx * ESTSS_CHAR_ATT_COUNT);
    uint16_t handle = GATTM_GetHandle(attidx);

    /* Schedule notification. */
    GATTC_SendEvtCmd(0, GATTC_NOTIFY, attidx, handle, ESTSS_CHAR_VALUE_SIZE,
            p_char->value);

    PRINTF("ESTSS: Notify tidx=%d\r\n", tidx);
}

/**
 * Sets new sensor value to selected Value Trigger and re-evaluates trigger
 * settings to see if all notification conditions were met.
 *
 * @param tidx
 * Trigger ID of the trigger to update.
 *
 * @param value
 * New trigger value.
 */
static void ESTSS_PushValue(ESTSS_TriggerId_t tidx, int32_t value)
{
    REQUIRE(tidx < ESTSS_TRIGGER_COUNT);
    REQUIRE(estss_env.state >= ESTSS_STATE_IDLE);

    PRINTF("ESTSS: Pushed trigger value. tidx=%d value=%d\r\n", tidx, value);

    ESTSS_Characteristic_t *p_char = estss_env.att.trigger + tidx;

    /* Store old value for notification logic. */
    int32_t old_value;
    memcpy(&old_value, p_char->value, ESTSS_CHAR_VALUE_SIZE);

    /* Update characteristic value. */
    memcpy(p_char->value, &value, ESTSS_CHAR_VALUE_SIZE);

    /* Schedule characteristic notification if value changed, notifications
     * are enabled and a value trigger is set.
     */
    if ((p_char->ccc[0] == ATT_CCC_START_NTF)
        && (!p_char->trig.ntf_pending))
    {
        if (ESTSS_EvaluateValueTrigger(p_char, value, old_value) == true)
        {
            if (ESTSS_EvaluateTimeTrigger(tidx) == true)
            {
                p_char->trig.ntf_pending = true;

                ESTSS_NotifyValue(tidx);
            }
        }
    }

    ENSURE(
        ((uint32_t )value)
            == (((uint32_t )p_char->value[0])
                | ((uint32_t )p_char->value[1] << 8)
                | ((uint32_t )p_char->value[2] << 16)
                | ((uint32_t )p_char->value[3] << 24)));
}

/**
 * Callback function called when client directly reads the value of a trigger.
 *
 * Manual read of the value causes the service to enable the trigger source
 * to be able to fulfill clients request and copies cached value for the client.
 *
 * @note
 * Since the current BLE Abstraction layer does not allow to postpone the
 * transmission of Read Request Confirmation the first value of the trigger
 * is invalid as the sensor is not enabled yet (unless notifications were
 * already enabled by CCC beforehand).
 *
 * @param conidx
 * @param attidx
 * @param handle
 * @param to
 * @param from
 * @param length
 * @param operation
 * @return
 */
static uint8_t ESTSS_TriggerValueReadHandler(uint8_t conidx, uint16_t attidx,
        uint16_t handle, uint8_t *to, const uint8_t *from, uint16_t length,
        uint16_t operation)
{
    REQUIRE(conidx == 0);
    REQUIRE(operation == GATTC_READ_REQ_IND);
    REQUIRE(attidx > estss_env.att.attidx_offset);
    REQUIRE(length == ESTSS_CHAR_VALUE_SIZE);

    ESTSS_TriggerId_t tidx = ESTSS_GetTriggerIdFromAttidx(attidx);

    /* Copy characteristic value from attribute db to tx buffer. */
    memcpy(to, from, length);

    /* Enable hardware sensor associated with this trigger if this is first
     * read.
     */
    if (!estss_env.att.trigger[tidx].trig.enabled)
    {
        ESTSS_ConfigureTriggerSource(tidx, true);
    }

    ENSURE(estss_env.att.trigger[tidx].trig.enabled == true);
    return ATT_ERR_NO_ERROR;
}

/**
 * Callback called when client reads or writes one of the Value Trigger
 * characteristic CCC descriptors.
 *
 * Enabling of notifications automatically enables the sensor and activates the
 * trigger.
 * Disabling of the notifications disables the sensor and trigger.
 *
 * @note
 * It is advised to configure Value Trigger setting and Time Trigger setting
 * before enabling notifications to prevent invalid events being sent to client.
 *
 * @param conidx
 * @param attidx
 * @param handle
 * @param to
 * @param from
 * @param length
 * @param operation
 * @return
 */
static uint8_t ESTSS_TriggerCCCUpdateHandler(uint8_t conidx, uint16_t attidx,
        uint16_t handle, uint8_t *to, const uint8_t *from, uint16_t length,
        uint16_t operation)
{
    REQUIRE(conidx == 0);
    REQUIRE(operation == GATTC_READ_REQ_IND || operation == GATTC_WRITE_REQ_IND);
    REQUIRE(attidx > estss_env.att.attidx_offset);

    const ESTSS_TriggerId_t tidx = ESTSS_GetTriggerIdFromAttidx(attidx);
    uint8_t status = ATT_ERR_NO_ERROR;

    if (length == ESTSS_CHAR_CCC_SIZE)
    {
        if (operation == GATTC_WRITE_REQ_IND)
        {
            const uint16_t ccc_value = from[0] | ((uint16_t)from[1] << 8);

            switch (ccc_value)
            {
                case ATT_CCC_STOP_NTFIND:
                    memcpy(to, from, length);
                    ESTSS_ConfigureTriggerSource(tidx, false);
                    break;

                case ATT_CCC_START_NTF:
                    memcpy(to, from, length);
                    ESTSS_ConfigureTriggerSource(tidx, true);
                    break;

                default:
                    status = ATT_ERR_REQUEST_NOT_SUPPORTED;
                    break;
            }

            PRINTF("ESTSS: CCC value changed: tidx=%d ccc=%d\r\n", tidx,
                    ccc_value);
        }
        else
        {
            /* READ */
            memcpy(to, from, length);
        }
    }
    else
    {
        status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
    }

    return status;
}

/**
 * Callback function called when one of the supported Value Trigger setting
 * descriptors is read/written by the client.
 *
 * Written configuration is checked and if valid written into the attribute
 * database as the new configuration.
 * If an invalid or unsupported value is written then
 * ATT_ERR_ESTS_TRIGGER_NOT_SUPPORTED attribute protocol error is transmitted.
 *
 * @param conidx
 * @param attidx
 * @param handle
 * @param to
 * @param from
 * @param length
 * @param operation
 * @return
 */
static uint8_t ESTSS_ValueTriggerSettingUpdateHandler(uint8_t conidx,
        uint16_t attidx, uint16_t handle, uint8_t *to, const uint8_t *from,
        uint16_t length, uint16_t operation)
{
    REQUIRE(conidx == 0);
    REQUIRE(operation == GATTC_READ_REQ_IND || operation == GATTC_WRITE_REQ_IND);
    REQUIRE(attidx > estss_env.att.attidx_offset);
    REQUIRE(length >= 1);

    const ESTSS_TriggerId_t tidx = ESTSS_GetTriggerIdFromAttidx(attidx);
    ESTSS_Characteristic_t *p_char = estss_env.att.trigger + tidx;
    uint8_t status = ATT_ERR_NO_ERROR;

    if (operation == GATTC_WRITE_REQ_IND)
    {
        /* Test if this trigger supports requested value trigger setting
         * condition.
         */
        if (((estss_value_trigger_setting_atlas[tidx] >> from[0]) & 1) == 1)
        {
            PRINTF("ESTSS: Value trigger setting updated. tidx=%d, condition=%d\r\n",
                    tidx, from[0]);

            /* Check for trigger specific write errors. */
            switch (from[0])
            {
                case ESTS_TRIG_VAL_CHANGED:
                    if (length != 1)
                    {
                        status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
                    }
                    break;

                case ESTS_TRIG_VAL_CHANGED_MORE_THAN:
                case ESTS_TRIG_VAL_ON_BOUNDARY:
                case ESTS_TRIG_VAL_CROSSED_BOUNDARY:
                    if (length != (1 + ESTSS_CHAR_VALUE_SIZE))
                    {
                        status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
                    }
                    break;

                case ESTS_TRIG_VAL_ON_INTERVAL:
                case ESTS_TRIG_VAL_CRROSSED_INTERVAL:
                    if (length != ESTSS_CHAR_TRIGGER_VALUE_SIZE)
                    {
                        status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
                    }
                    break;

                case ESTS_TRIG_VAL_NO_TRIGGER:
                    if (length != 1)
                    {
                        status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
                    }
                    break;

                default:
                    ASSERT(false);
                    break;
            }

            if (status == ATT_ERR_NO_ERROR)
            {
                /* Copy descriptor data to / from attribute database if no
                 * errors were detected.
                 */
                memcpy(to, from, length);

                /* Notify application of updated trigger settings if trigger is
                 * already enabled.
                 */
                if (p_char->trig.enabled)
                {
                    ESTSS_ConfigureTriggerSource(tidx, true);
                }
            }
        }
        else
        {
            status = ATT_ERR_ESTS_TRIGGER_NOT_SUPPORTED;
        }
    }
    else
    {
        /* Copy data for read requests. */
        memcpy(to, from, length);
    }

    return status;
}

/**
 * Callback function called when one of the supported Time Trigger setting
 * descriptors is read/written by the client.
 *
 * Written configuration is checked and if valid written into the attribute
 * database as the new configuration.
 * If an invalid or unsupported value is written then
 * ATT_ERR_ESTS_TRIGGER_NOT_SUPPORTED attribute protocol error is transmitted.
 *
 * @param conidx
 * @param attidx
 * @param handle
 * @param to
 * @param from
 * @param length
 * @param operation
 * @return
 */
static uint8_t ESTSS_TimeTriggerSettingUpdateHandler(uint8_t conidx,
        uint16_t attidx, uint16_t handle, uint8_t *to, const uint8_t *from,
        uint16_t length, uint16_t operation)
{
    REQUIRE(conidx == 0);
    REQUIRE(operation == GATTC_READ_REQ_IND || operation == GATTC_WRITE_REQ_IND);
    REQUIRE(attidx > estss_env.att.attidx_offset);
    REQUIRE(length >= 1);

    const ESTSS_TriggerId_t tidx = ESTSS_GetTriggerIdFromAttidx(attidx);
    ESTSS_Characteristic_t *p_char = estss_env.att.trigger + tidx;
    uint8_t status = ATT_ERR_NO_ERROR;

    if (operation == GATTC_WRITE_REQ_IND)
    {
        switch (from[0])
        {
            case ESTS_TRIG_TIME_NO_TRIGGER:
            {
                if (length == 1)
                {
                    p_char->trig.ntf_min_interval = 0;

                    PRINTF("ESTSS: Time trigger disabled. tidx=%d\r\n", tidx);
                }
                else
                {
                    status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
                }
                break;
            }

            case ESTS_TRIG_TIME_MIN_INTERVAL:
            {
                if (length == ESTSS_CHAR_TRIGGER_TIME_SIZE)
                {
                    uint32_t interval = 0;

                    interval |= (uint32_t) from[1];
                    interval |= ((uint32_t) from[2] << 8);
                    interval |= ((uint32_t) from[3] << 16);

                    /* Convert from seconds to milliseconds. */
                    p_char->trig.ntf_min_interval = interval * 1000;

                    PRINTF("ESTSS: Time trigger updated to MIN_INTERVAL tidx=%d interval=%ds\r\n",
                            tidx, interval);
                }
                else
                {
                    status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
                }
                break;
            }

            default:
                status = ATT_ERR_ESTS_TRIGGER_NOT_SUPPORTED;
                break;
        }

        if (status == ATT_ERR_NO_ERROR)
        {
            /* Copy descriptor data to / from attribute database if no
             * errors were detected.
             */
            memcpy(to, from, length);

            /* Notify application of updated trigger settings if trigger is
             * already enabled.
             */
            if (p_char->trig.enabled)
            {
                ESTSS_ConfigureTriggerSource(tidx, true);
            }
        }
    }
    else
    {
        /* Copy data for read requests. */
        memcpy(to, from, length);
    }

    return status;
}

/**
 * Message handler for kernel messages generated by the GAPC task of BLE stack.
 *
 * The service listens for Connection Requests to reset its state for new
 * connection.
 *
 * Disconnection events are used to disable any enabled trigger sources to save
 * power when there is no client to receive events.
 *
 * @param msg_id
 * @param param
 * @param dest_id
 * @param src_id
 */
static void ESTSS_BleMsgHandler(ke_msg_id_t const msg_id, void const *param,
        ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    switch (msg_id)
    {
        case GAPC_CONNECTION_REQ_IND:
        {
            REQUIRE(estss_env.state == ESTSS_STATE_IDLE);

            estss_env.state = ESTSS_STATE_CONNECTED;

            /* Make sure all connection related variables are reset.*/
            for (ESTSS_TriggerId_t tidx = 0; tidx < ESTSS_TRIGGER_COUNT; ++tidx)
            {
                estss_env.att.trigger[tidx].ccc[0] = 0;
                estss_env.att.trigger[tidx].ccc[1] = 0;
            }

            ENSURE(estss_env.state == ESTSS_STATE_CONNECTED);
            break;
        }

        case GAPC_DISCONNECT_IND:
        {
            REQUIRE(estss_env.state == ESTSS_STATE_CONNECTED);

            estss_env.state = ESTSS_STATE_IDLE;

            /* Disable any active triggers upon disconnection. */
            for (ESTSS_TriggerId_t tidx = 0; tidx < ESTSS_TRIGGER_COUNT; ++tidx)
            {
                ESTSS_ConfigureTriggerSource(tidx, false);
            }

            ENSURE(estss_env.state == ESTSS_STATE_IDLE);
            break;
        }

        default:
            break;
    }
}

int32_t ESTSS_Initialize(ESTSS_TriggerUpdatedCallback_t p_update_cb,
        ESTSS_TimeCallbackMs_t p_time_cb)
{
    REQUIRE(estss_env.state == ESTSS_STATE_INIT);
    INVARIANT(ESTSS_CHAR_VALUE_SIZE == sizeof(uint32_t));
    INVARIANT(ESTSS_ATT_MOTION_CHAR_0 == (ESTSS_ATT_SERVICE_0 + 1));
    /* Make sure that all characteristics have identical number of attributes for offset calculations. */
    INVARIANT(
            (ESTSS_ATT_ACCELERATION_CHAR_0 - ESTSS_ATT_MOTION_CHAR_0) == ESTSS_CHAR_ATT_COUNT);
    INVARIANT(
            (ESTSS_ATT_TEMPERATURE_CHAR_0 - ESTSS_ATT_ACCELERATION_CHAR_0) == ESTSS_CHAR_ATT_COUNT);
    INVARIANT(
            (ESTSS_ATT_HUMIDITY_CHAR_0 - ESTSS_ATT_TEMPERATURE_CHAR_0) == ESTSS_CHAR_ATT_COUNT);
    INVARIANT(
            (ESTSS_ATT_COUNT - ESTSS_ATT_HUMIDITY_CHAR_0) == ESTSS_CHAR_ATT_COUNT);

    REQUIRE(p_update_cb != NULL);
    REQUIRE(p_time_cb != NULL);

    int32_t status;
    ESTSS_Characteristic_t *p_char;

    /* Reset environment structure. */
    memset(&estss_env, 0, sizeof(estss_env));

    /* Register update callback. */
    estss_env.p_update_cb = p_update_cb;

    /* Register time callback */
    estss_env.p_time_cb = p_time_cb;

    /* Initialize Motion Trigger Characteristic and Descriptors. */
    p_char = estss_env.att.trigger + ESTSS_TRIGGER_MOTION;
    p_char->fmt[0] = ATT_FORMAT_BOOL; /* Format[0] - boolean */
    p_char->fmt[1] = 0; /* Exponent[0] - 1e0 */
    p_char->fmt[2] = (uint8_t) ATT_UNIT_UNITLESS; /* Unit[0] - ATT_UNIT_UNITLESS */
    p_char->fmt[3] = (ATT_UNIT_UNITLESS >> 8); /* Unit[1] */
    p_char->fmt[4] = 0x01; /* Namespace[0] - Bluetooth SIG */
    p_char->fmt[5] = 0x01; /* Description[0] - First */
    p_char->fmt[6] = 0x00; /* Description[1] */
    p_char->trig.value[0] = ESTS_TRIG_VAL_NO_TRIGGER; /* Value Trigger Setting */
    p_char->trig.time[0] = ESTS_TRIG_TIME_NO_TRIGGER; /* Time Trigger Setting */

    /* Initialize Acceleration Trigger Characteristic and Descriptors. */
    p_char = estss_env.att.trigger + ESTSS_TRIGGER_ACCELERATION;
    p_char->fmt[0] = ATT_FORMAT_BOOL; /* Format[0] - boolean */
    p_char->fmt[1] = 0; /* Exponent[0] - 1e0 */
    p_char->fmt[2] = (uint8_t) ATT_UNIT_UNITLESS; /* Unit[0] - ATT_UNIT_UNITLESS */
    p_char->fmt[3] = (ATT_UNIT_UNITLESS >> 8); /* Unit[1] */
    p_char->fmt[4] = 0x01; /* Namespace[0] - Bluetooth SIG */
    p_char->fmt[5] = 0x02; /* Description[0] - Second */
    p_char->fmt[6] = 0x00; /* Description[1] */
    p_char->trig.value[0] = ESTS_TRIG_VAL_NO_TRIGGER; /* Value Trigger Setting */
    p_char->trig.time[0] = ESTS_TRIG_TIME_NO_TRIGGER; /* Time Trigger Setting */

    /* Initialize Temperature Trigger Characteristic and Descriptors. */
    p_char = estss_env.att.trigger + ESTSS_TRIGGER_TEMPERATURE;
    p_char->fmt[0] = ATT_FORMAT_SINT32; /* Format[0] - 32 bit signed integer */
    p_char->fmt[1] = -2; /* Exponent[0] - 10e-2 */
    p_char->fmt[2] = (uint8_t) ATT_UNIT_CELSIUS & 0xFF; /* Unit[0] - Celsius*/
    p_char->fmt[3] = (ATT_UNIT_CELSIUS >> 8); /* Unit[1] */
    p_char->fmt[4] = 0x01; /* Namespace[0] - Bluetooth SIG */
    p_char->fmt[5] = 0x03; /* Description[0] - Third */
    p_char->fmt[6] = 0x00; /* Description[1] */
    p_char->trig.value[0] = ESTS_TRIG_VAL_NO_TRIGGER; /* Value Trigger Setting */
    p_char->trig.time[0] = ESTS_TRIG_TIME_NO_TRIGGER; /* Time Trigger Setting */

    /* Initialize Humidity Trigger Characteristic and Descriptors. */
    p_char = estss_env.att.trigger + ESTSS_TRIGGER_HUMIDITY;
    p_char->fmt[0] = ATT_FORMAT_SINT32; /* Format[0] - 32 bit signed integer */
    p_char->fmt[1] = -3; /* Exponent[0] - 10e-3 */
    p_char->fmt[2] = (uint8_t) ATT_UNIT_PERCENTAGE; /* Unit[0] - Percentage */
    p_char->fmt[3] = (ATT_UNIT_PERCENTAGE >> 8); /* Unit[1] */
    p_char->fmt[4] = 0x01; /* Namespace[0] - Bluetooth SIG */
    p_char->fmt[5] = 0x04; /* Description[0] - Fourth */
    p_char->fmt[6] = 0x00; /* Description[1] */
    p_char->trig.value[0] = ESTS_TRIG_VAL_NO_TRIGGER; /* Value Trigger Setting */
    p_char->trig.time[0] = ESTS_TRIG_TIME_NO_TRIGGER; /* Time Trigger Setting */

    /* Add custom attributes into the attribute database. */
    status = APP_BLE_PeripheralServerAddCustomService(estss_att_db,
            ESTSS_ATT_COUNT, &estss_env.att.attidx_offset);
    if (status != 0)
    {
        return -1;
    }

    estss_env.state = ESTSS_STATE_IDLE;

    /* Listen for specific BLE kernel messages. */
    MsgHandler_Add(GAPC_DISCONNECT_IND, ESTSS_BleMsgHandler);
    MsgHandler_Add(GAPC_CONNECTION_REQ_IND, ESTSS_BleMsgHandler);

    ENSURE(estss_env.p_update_cb != NULL);
    ENSURE(estss_env.p_time_cb != NULL);
    ENSURE(estss_env.state == ESTSS_STATE_IDLE);
    return 0;
}

bool ESTSS_TriggerIsEnabled(ESTSS_TriggerId_t tidx)
{
    REQUIRE(estss_env.state >= ESTSS_STATE_IDLE);

    REQUIRE(tidx < ESTSS_TRIGGER_COUNT);

    return estss_env.att.trigger[tidx].trig.enabled;
}

void ESTSS_GetTriggerSettings(ESTSS_TriggerId_t tidx,
        ESTSS_TriggerSettings_t *p_settings)
{
    REQUIRE(estss_env.state >= ESTSS_STATE_IDLE);

    REQUIRE(tidx < ESTSS_TRIGGER_COUNT);
    REQUIRE(p_settings != NULL);

    ESTSS_Characteristic_t *p_char = estss_env.att.trigger + tidx;

    p_settings->cond_value = p_char->trig.value[0];
    switch (p_char->trig.value[0])
    {
        case ESTS_TRIG_VAL_CROSSED_BOUNDARY:
        case ESTS_TRIG_VAL_ON_BOUNDARY:
        case ESTS_TRIG_VAL_CHANGED_MORE_THAN:
            memcpy(p_settings->boundary, p_char->trig.value + 1, ESTSS_CHAR_VALUE_SIZE);
            p_settings->boundary[1] = 0;
            break;

        case ESTS_TRIG_VAL_CRROSSED_INTERVAL:
        case ESTS_TRIG_VAL_ON_INTERVAL:
            memcpy(p_settings->boundary, p_char->trig.value + 1, ESTSS_CHAR_VALUE_SIZE);
            memcpy(p_settings->boundary + 1, p_char->trig.value + 5, ESTSS_CHAR_VALUE_SIZE);
            break;

        default:
            p_settings->boundary[0] = 0;
            p_settings->boundary[1] = 0;
    }

    p_settings->cond_time = p_char->trig.time[0];
    switch (p_char->trig.time[0])
    {
        case ESTS_TRIG_TIME_PERIODIC:
        case ESTS_TRIG_TIME_MIN_INTERVAL:
            p_settings->time_interval = p_char->trig.ntf_min_interval;
            break;

        default:
            p_settings->time_interval = 0;
            break;
    }
}

void ESTSS_PushMotionValue(bool motion_state)
{
    REQUIRE(estss_env.state >= ESTSS_STATE_IDLE);

    ESTSS_PushValue(ESTSS_TRIGGER_MOTION, motion_state);
}

void ESTSS_PushAccelerationValue(bool acceleration_state)
{
    REQUIRE(estss_env.state >= ESTSS_STATE_IDLE);

    ESTSS_PushValue(ESTSS_TRIGGER_ACCELERATION, acceleration_state);
}

void ESTSS_PushTemperatureValue(int32_t temperature)
{
    REQUIRE(estss_env.state >= ESTSS_STATE_IDLE);

    ESTSS_PushValue(ESTSS_TRIGGER_TEMPERATURE, temperature);
}

void ESTSS_PushHumidityValue(int32_t humidity)
{
    REQUIRE(estss_env.state >= ESTSS_STATE_IDLE);

    ESTSS_PushValue(ESTSS_TRIGGER_HUMIDITY, humidity);
}
