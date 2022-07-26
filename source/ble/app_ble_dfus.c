//-----------------------------------------------------------------------------
// Copyright (c) 2021 Semiconductor Components Industries LLC
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

#include <app_ble_dfus.h>
#include <app_ble_dfus_int.h>
#include <app_ble_peripheral_server.h>
#include <msg_handler.h>

#include <smartshot_assert.h>
#include <smartshot_printf.h>

/* Stores one copy of filename for ASSERT calls. */
DEFINE_THIS_FILE_FOR_ASSERT;

/* Extern variables */
extern const Sys_Fota_version_t Sys_Boot_app_version;

#define  CS_CHAR_TEXT_DESC(idx, text)   \
    CS_CHAR_USER_DESC(idx, sizeof(text) - 1, text, NULL)

static uint8_t DFUS_Handler(uint8_t conidx, uint16_t attidx,
        uint16_t handle, uint8_t *to, const uint8_t *from, uint16_t length,
        uint16_t operation);

static DFUS_Environment_t dfus_env;

/** Complete attribute database of the DFUS. */
const static struct att_db_desc dfus_att_db[DFUS_ATT_COUNT] =
{
    /* Device Firmware Update Service 0 */
    CS_SERVICE_UUID_128(
            DFUS_ATT_SERVICE_0, /* attidx */
            SYS_FOTA_DFU_SVC_UUID), /* uuid */

    /* Device ID Characteristic in Service DFU */
    CS_CHAR_UUID_128(
            DFUS_ATT_DEVID_CHAR_0, /* attidx_char */
            DFUS_ATT_DEVID_VAL_0, /* attidx_val */
            SYS_FOTA_DFU_DEVID_UUID, /* uuid */
            PERM(RD, ENABLE), /* perm */
            DFUS_DEVID_CHAR_VALUE_SIZE, /* length */
            NULL, /* data */
            DFUS_Handler), /* callback */
    CS_CHAR_TEXT_DESC(DFUS_ATT_DEVID_NAME_0, DFUS_CHAR_DEVID_DESC),

    /* BootLoader Version Characteristic in Service DFU */
    CS_CHAR_UUID_128(
            DFUS_ATT_BOOTVER_CHAR_0, /* attidx_char */
            DFUS_ATT_BOOTVER_VAL_0, /* attidx_val */
            SYS_FOTA_DFU_BOOTVER_UUID, /* uuid */
            PERM(RD, ENABLE), /* perm */
            DFUS_BOOTVER_CHAR_VALUE_SIZE, /* length */
            NULL, /* data */
            DFUS_Handler), /* callback */
    CS_CHAR_TEXT_DESC(DFUS_ATT_BOOTVER_NAME_0, DFUS_CHAR_BOOTVER_DESC),

    /* BLE Stack Version Characteristic in Service DFU */
    CS_CHAR_UUID_128(
            DFUS_ATT_STACKVER_CHAR_0, /* attidx_char */
            DFUS_ATT_STACKVER_VAL_0, /* attidx_val */
            SYS_FOTA_DFU_STACKVER_UUID, /* uuid */
            PERM(RD, ENABLE), /* perm */
            DFUS_STACKVER_CHAR_VALUE_SIZE, /* length */
            NULL, /* data */
            DFUS_Handler), /* callback */
    CS_CHAR_TEXT_DESC(DFUS_ATT_STACKVER_NAME_0, DFUS_CHAR_STACKVER_DESC),

    /* Application Version Characteristic in Service DFU */
    CS_CHAR_UUID_128(
            DFUS_ATT_APPVER_CHAR_0, /* attidx_char */
            DFUS_ATT_APPVER_VAL_0, /* attidx_val */
            SYS_FOTA_DFU_APPVER_UUID, /* uuid */
            PERM(RD, ENABLE), /* perm */
            DFUS_APPVER_CHAR_VALUE_SIZE, /* length */
            NULL, /* data */
            DFUS_Handler), /* callback */
    CS_CHAR_TEXT_DESC(DFUS_ATT_APPVER_NAME_0, DFUS_CHAR_APPVER_DESC),

    /* Build ID Characteristic in Service DFU */
    CS_CHAR_UUID_128(
            DFUS_ATT_BUILDID_CHAR_0, /* attidx_char */
            DFUS_ATT_BUILDID_VAL_0, /* attidx_val */
            SYS_FOTA_DFU_BUILDID_UUID, /* uuid */
            PERM(RD, ENABLE), /* perm */
            DFUS_BUILDID_CHAR_VALUE_SIZE, /* length */
            NULL, /* data */
            DFUS_Handler), /* callback */
    CS_CHAR_TEXT_DESC(DFUS_ATT_BUILDID_NAME_0, DFUS_CHAR_BUILDID_DESC),

    /* Enter mode Characteristic in Service DFU */
    CS_CHAR_UUID_128(
            DFUS_ATT_ENTER_CHAR_0, /* attidx_char */
            DFUS_ATT_ENTER_VAL_0, /* attidx_val */
            SYS_FOTA_DFU_ENTER_UUID, /* uuid */
            PERM(WRITE_REQ, ENABLE), /* perm */
            DFUS_ENTER_CHAR_VALUE_SIZE, /* length */
            NULL, /* data */
            DFUS_Handler), /* callback */
    CS_CHAR_TEXT_DESC(DFUS_ATT_ENTER_NAME_0, DFUS_CHAR_ENTER_DESC)
};

int32_t APP_DFUS_Initialize(DFUS_DfuEnterCallback_t p_dfu_enter_cb)
{
    REQUIRE(p_dfu_enter_cb != NULL);

    int32_t status;

    /* Reset environment structure. */
    memset(&dfus_env, 0, sizeof(dfus_env));

    /* Register DFU Enter callback. */
    dfus_env.p_dfu_enter_cb = p_dfu_enter_cb;

    /* Add custom attributes into the attribute database. */
    status = APP_BLE_PeripheralServerAddCustomService(dfus_att_db,
            DFUS_ATT_COUNT, &dfus_env.att.attidx_offset);
    if (status != 0)
    {
        return -1;
    }

    ENSURE(dfus_env.p_dfu_enter_cb != NULL);
    return 0;
}

static uint8_t DFUS_Handler(uint8_t conidx, uint16_t attidx, uint16_t handle,
        uint8_t *to, const uint8_t *from, uint16_t length,
        uint16_t operation)
{
    REQUIRE(attidx > dfus_env.att.attidx_offset);
    REQUIRE(attidx < (dfus_env.att.attidx_offset + DFUS_ATT_COUNT));

    const uint16_t dfus_attidx = attidx - dfus_env.att.attidx_offset;

    if (operation == GATTC_WRITE_REQ_IND)
    {
        PRINTF("DFUS: GATTC_WRITE_REQ_IND ATT_DFU_ENTER_VAL_0 value=%u\n\r", *from);

        if (dfus_attidx == DFUS_ATT_ENTER_VAL_0)
        {
            if (length != DFUS_ENTER_CHAR_VALUE_SIZE)
            {
                return ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            }

            if (*from != 1)
            {
                return ATT_ERR_REQUEST_NOT_SUPPORTED;
            }

            if (dfus_env.p_dfu_enter_cb != NULL)
            {
                dfus_env.p_dfu_enter_cb();
            }
        }
    }

    if (operation == GATTC_READ_REQ_IND)
    {
        PRINTF("DFUS: GATTC_READ_REQ_IND att_idx=%u, dfus_attidx=%u, length=%u\n\r", attidx, dfus_attidx, length);

        switch (dfus_attidx)
        {
        case DFUS_ATT_DEVID_VAL_0:
        {
            if (length != DFUS_DEVID_CHAR_VALUE_SIZE)
            {
                return ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            }

            const Sys_Fota_version_t *version;
            version = (const Sys_Fota_version_t *) Sys_Boot_GetVersion(APP_BASE_ADR);
            memcpy(to, &version->dev_id, length);
        }
        break;

        case DFUS_ATT_BOOTVER_VAL_0:
        {
            if (length != DFUS_BOOTVER_CHAR_VALUE_SIZE)
            {
                return ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            }

            memcpy(to, Sys_Boot_GetVersion(BOOT_BASE_ADR), length);
        }
        break;

        case DFUS_ATT_STACKVER_VAL_0:
        {
            if (length != DFUS_STACKVER_CHAR_VALUE_SIZE)
            {
                return ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            }

            memcpy(to, Sys_Boot_GetVersion(APP_BASE_ADR), length);
        }
        break;

        case DFUS_ATT_APPVER_VAL_0:
        {
            if (length != DFUS_APPVER_CHAR_VALUE_SIZE)
            {
                return ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            }

            memcpy(to, &Sys_Boot_app_version.app_id, length);
        }
        break;

        case DFUS_ATT_BUILDID_VAL_0:
        {
            if (length != DFUS_BUILDID_CHAR_VALUE_SIZE)
            {
                return ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            }

            memcpy(to, Sys_Boot_GetDscr(APP_BASE_ADR)->build_id_a, length);
        }
        break;

        }
    }

    return ATT_ERR_NO_ERROR;
}

