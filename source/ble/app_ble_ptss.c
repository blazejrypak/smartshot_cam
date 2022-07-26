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

#include <app_ble_ptss_int.h>
#include <app_ble_peripheral_server.h>
#include <msg_handler.h>

#include <smartshot_assert.h>
#include <smartshot_printf.h>


DEFINE_THIS_FILE_FOR_ASSERT;


static uint8_t PTSS_ControlPointWriteHandler(uint8_t conidx, uint16_t attidx,
        uint16_t handle, uint8_t *to, const uint8_t *from, uint16_t length,
        uint16_t operation);


static PTSS_Environment_t ptss_env = { 0 };

const struct att_db_desc ptss_att_db[ATT_PTSS_COUNT] =
{
    /* Picture Transfer Service 0 */

    CS_SERVICE_UUID_128(
            ATT_PTSS_SERVICE_0, /* attidx */
            PTSS_SVC_UUID),     /* uuid */

    /* Picture Transfer Control Point Characteristic */

    CS_CHAR_UUID_128(
            ATT_PTSS_CONTROL_POINT_CHAR_0,                         /* attidx_char */
            ATT_PTSS_CONTROL_POINT_VAL_0,                          /* attidx_val */
            PTSS_CHAR_CONTROL_POINT_UUID,                          /* uuid */
            PERM(WRITE_REQ, ENABLE) | PERM(WRITE_COMMAND, ENABLE), /* perm */
            sizeof(ptss_env.att.cp.value),                         /* length */
            ptss_env.att.cp.value,                                 /* data */
            PTSS_ControlPointWriteHandler),                        /* callback */

    CS_CHAR_USER_DESC(
            ATT_PTSS_CONTROL_POINT_DESC_0,              /* attidx */
            (sizeof(PTSS_CONTROL_POINT_USER_DESC) - 1), /* length */
            PTSS_CONTROL_POINT_USER_DESC,               /* data */
            NULL),                                      /* callback */

    /* Picture Transfer Info Characteristic */

    CS_CHAR_UUID_128(
            ATT_PTSS_INFO_CHAR_0, /* attidx_char */
            ATT_PTSS_INFO_VAL_0,  /* attidx_val */
            PTSS_CHAR_INFO_UUID,  /* uuid */
            PERM(NTF, ENABLE),    /* perm */
            0,                    /* length */
            NULL,                 /* data */
            NULL),                /* callback */

    CS_CHAR_CCC(
            ATT_PTSS_INFO_CCC_0,             /* attidx */
            ptss_env.att.info.ccc,           /* data */
            NULL),                           /* callback */

    CS_CHAR_USER_DESC(
            ATT_PTSS_INFO_DESC_0,                  /* attidx */
            (sizeof(PTSS_IMG_INFO_USER_DESC) - 1), /* length*/
            PTSS_IMG_INFO_USER_DESC,               /* data */
            NULL),                                 /* callback */

    /* Picture Transfer Image Data Characteristic */

    CS_CHAR_UUID_128(ATT_PTSS_IMAGE_DATA_CHAR_0, /* attidx_char */
            ATT_PTSS_IMAGE_DATA_VAL_0,           /* attidx_val */
            PTS_CHAR_IMAGE_DATA_UUID,            /* uuid */
            PERM(NTF, ENABLE),                   /* perm */
            0,                                   /* length */
            NULL,                                /* data */
            NULL),                               /* callback */

    CS_CHAR_CCC(
            ATT_PTSS_IMAGE_DATA_CCC_0,       /* attidx */
            ptss_env.att.img_data.ccc,       /* data */
            NULL),                           /* callback */

    CS_CHAR_USER_DESC(
            ATT_PTSS_IMAGE_DATA_DESC_0,            /* attidx */
            (sizeof(PTSS_IMG_DATA_USER_DESC) - 1), /* length*/
            PTSS_IMG_DATA_USER_DESC,               /* data */
            NULL),                                 /* callback */
};

static uint32_t PTSS_GetMaxDataOctets(void)
{
    uint32_t max_data_octets = ptss_env.max_tx_octets
                               - PTSS_NTF_PDU_HEADER_LENGTH
                               - PTSS_NTF_L2CAP_HEADER_LENGTH
                               - PTSS_NTF_ATT_HEADER_LENGTH;

    return max_data_octets;
}

static void PTSS_MsgHandler(ke_msg_id_t const msg_id, void const *param,
        ke_task_id_t const dest_id, ke_task_id_t const src_id)
{
    switch (msg_id)
    {
        case GATTC_CMP_EVT:
        {
            const struct gattc_cmp_evt *p = param;

            if (p->operation == GATTC_NOTIFY)
            {
                uint16_t attidx = ptss_env.att.attidx_offset
                                  + ATT_PTSS_IMAGE_DATA_VAL_0;

                /* If the sequence number is the number of Image Data Value
                 * attribute reduce number of pending packets. */
                if (p->seq_num == attidx)
                {
                    ENSURE(ptss_env.transfer.packets_pending > 0);

                    ptss_env.transfer.packets_pending -= 1;

                    if (ptss_env.transfer.bytes_queued
                        < ptss_env.transfer.bytes_total)
                    {
                        /* Not all data is queued yet.
                         * Notify application that PTSS is ready to accept image
                         * data for next packet. */
                        ptss_env.att.cp.callback(
                                PTSS_OP_IMAGE_DATA_SPACE_AVAIL_IND,
                                NULL);
                    }
                    else
                    {
                        /* Last data notification was transferred. */
                        if (ptss_env.transfer.packets_pending == 0)
                        {
                            /* Determine next state of PTSS. */
                            switch (ptss_env.att.cp.capture_mode)
                            {
                                case PTSS_CONTROL_POINT_OPCODE_CAPTURE_ONE_SHOT_REQ:
                                    ptss_env.transfer.state = PTSS_STATE_CONNECTED;
                                    ptss_env.att.cp.capture_mode = 0;
                                    break;

                                case PTSS_CONTROL_POINT_OPCODE_CAPTURE_CONTINUOUS_REQ:
                                    ptss_env.transfer.state = PTSS_STATE_CAPTURE_REQUEST;
                                    break;

                                default:
                                    INVARIANT(false);
                                    break;
                            }

                            /* Notify application that Image data transfer finished. */
                            ptss_env.att.cp.callback(
                                    PTSS_OP_IMAGE_DATA_TRANSFER_DONE_IND,
                                    NULL);
                        }
                    }
                }
            }

            break;
        }

        case GATTC_MTU_CHANGED_IND:
        {
            if (ptss_env.transfer.state >= PTSS_STATE_IMG_INFO_PROVIDED)
            {
                /* Update of MTU not allowed while image transfer is in
                 * progress.
                 *
                 * 0x16 - CONNECTION TERMINATED BY LOCAL HOST
                 */
                GAPC_DisconnectAll(0x16);
            }

            break;
        }

        case GAPC_CONNECTION_REQ_IND:
        {
            REQUIRE(ptss_env.transfer.state == PTSS_STATE_IDLE);

            ptss_env.transfer.state = PTSS_STATE_CONNECTED;

            /* Reset connection related variables. */
            ptss_env.max_tx_octets = PTSS_MIN_TX_OCTETS;

            break;
        }

        case GAPC_DISCONNECT_IND:
        {
            REQUIRE(ptss_env.transfer.state >= PTSS_STATE_CONNECTED);

            /* Send abort indication if disconnected during image transfer. */
            if (ptss_env.transfer.state >= PTSS_STATE_IMG_INFO_PROVIDED)
            {
                ptss_env.att.cp.callback(PTSS_OP_CAPTURE_CANCEL_REQ, NULL);
            }

            ptss_env.transfer.state = PTSS_STATE_IDLE;
            break;
        }

        case GAPC_LE_PKT_SIZE_IND:
        {

            if (ptss_env.transfer.state < PTSS_STATE_IMG_INFO_PROVIDED)
            {
                /* Allow to update DLE parameters when there is no image
                 * transfer.
                 */

                const struct gapc_le_pkt_size_ind *p = param;

                ptss_env.max_tx_octets = p->max_tx_octets;
                PRINTF("PTSS : Set max_tx_octets=%d\r\n", ptss_env.max_tx_octets);
            }
            else
            {
                /* Update of DLE parameters not allowed while image transfer is
                 * in progress.
                 *
                 * 0x16 - CONNECTION TERMINATED BY LOCAL HOST
                 */
                GAPC_DisconnectAll(0x16);
            }

            ENSURE(ptss_env.max_tx_octets <= 251);
            break;
        }

        default:
            INVARIANT(false);
            break;
    }
}

static uint8_t PTSS_ProcessImageCaptureRequest(uint8_t capture_type)
{
    REQUIRE((capture_type == PTSS_CONTROL_POINT_OPCODE_CAPTURE_ONE_SHOT_REQ)
            || (capture_type == PTSS_CONTROL_POINT_OPCODE_CAPTURE_CONTINUOUS_REQ));

    uint8_t err = ATT_ERR_NO_ERROR;

    if (ptss_env.transfer.state == PTSS_STATE_CONNECTED)
    {
        if (ptss_env.att.info.ccc[0] == ATT_CCC_START_NTF)
        {
            ptss_env.transfer.state = PTSS_STATE_CAPTURE_REQUEST;
            ptss_env.att.cp.capture_mode = capture_type;

            if (capture_type == PTSS_CONTROL_POINT_OPCODE_CAPTURE_ONE_SHOT_REQ)
            {
                ptss_env.att.cp.callback(PTSS_OP_CAPTURE_ONE_SHOT_REQ, NULL);
            }
            else
            {
                ptss_env.att.cp.callback(PTSS_OP_CAPTURE_CONTINUOUS_REQ, NULL);
            }
        }
        else
        {
            err = PTSS_ATT_ERR_NTF_DISABLED;
        }
    }
    else
    {
        err = PTSS_ATT_ERR_PROC_IN_PROGRESS;
    }

    return err;
}

static uint8_t PTSS_ProcessImageDataTransferRequest(uint8_t packet_count)
{
    uint8_t err = ATT_ERR_NO_ERROR;

    if (ptss_env.transfer.state >= PTSS_STATE_IMG_INFO_PROVIDED)
    {
        ptss_env.transfer.state = PTSS_STATE_IMG_DATA_TRANSMISSION;

        ptss_env.att.cp.callback(PTSS_OP_IMAGE_DATA_TRANSFER_REQ, NULL);
    }
    else
    {
        err = PTSS_ATT_ERR_IMG_TRANSFER_DISALLOWED;
    }

    return err;
}

static uint8_t PTSS_ProcessAbortCaptureRequest(void)
{
    uint8_t status = ATT_ERR_NO_ERROR;

    /* Cancel if operation is really ongoing.
     *
     * Silently ignore if there is nothing to cancel.
     */
    if (ptss_env.transfer.state >= PTSS_STATE_CAPTURE_REQUEST)
    {
        PTSS_AbortImageTransfer(PTSS_INFO_ERR_ABORTED_BY_CLIENT);

        /* Notify application that capture is aborted. */
        ptss_env.att.cp.callback(PTSS_OP_CAPTURE_CANCEL_REQ, NULL);
    }

    return status;
}

static uint8_t PTSS_ControlPointWriteHandler(uint8_t conidx, uint16_t attidx,
        uint16_t handle, uint8_t *to, const uint8_t *from, uint16_t length,
        uint16_t operation)
{
    if (length > PTSS_CONTROL_POINT_VALUE_LENGTH || length == 0)
    {
        return ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
    }

    uint8_t status = ATT_ERR_NO_ERROR;

    switch (from[0])
    {
        case PTSS_CONTROL_POINT_OPCODE_CAPTURE_ONE_SHOT_REQ:
        case PTSS_CONTROL_POINT_OPCODE_CAPTURE_CONTINUOUS_REQ:
        {
            if (length == 1)
            {
                status = PTSS_ProcessImageCaptureRequest(from[0]);
            }
            else
            {
                status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            }
            break;
        }

        case PTSS_CONTROL_POINT_OPCODE_CAPTURE_CANCEL_REQ:
        {
            if (length == 1)
            {
                status = PTSS_ProcessAbortCaptureRequest();
            }
            else
            {
                status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            }
            break;
        }

        case PTSS_CONTROL_POINT_OPCODE_CAPTURE_IMG_DATA_TRANSFER_REQ:
        {
            if (length == 1)
            {
                status = PTSS_ProcessImageDataTransferRequest(from[1]);
            }
            else
            {
                status = ATT_ERR_INVALID_ATTRIBUTE_VAL_LEN;
            }
            break;
        }

        default:
        {
            status = ATT_ERR_REQUEST_NOT_SUPPORTED;
        }
    }

    return status;
}


static void PTSS_TransmitImgDataNotification(void)
{
    /* There should not be any attempt to transmit notification while no data
     * are queued.
     */
    REQUIRE(ptss_env.att.img_data.value_length > PTSS_INFO_OFFSET_LENGTH);

    uint16_t attidx = ptss_env.att.attidx_offset + ATT_PTSS_IMAGE_DATA_VAL_0;
    uint16_t att_handle = GATTM_GetHandle(attidx);

    GATTC_SendEvtCmd(0, GATTC_NOTIFY, attidx, att_handle,
            ptss_env.att.img_data.value_length,
            ptss_env.att.img_data.value);

    ptss_env.att.img_data.value_length = 0;

    ptss_env.transfer.packets_pending += 1;
}

int32_t PTSS_Initialize(PTSS_ControlHandler control_event_handler)
{
    int32_t status;

    if (control_event_handler == NULL)
    {
        return PTSS_ERR;
    }

    ptss_env.att.attidx_offset = 0;
    ptss_env.att.cp.callback = control_event_handler;
    ptss_env.att.cp.capture_mode = 0;
    ptss_env.att.info.ccc[0] = 0x00;
    ptss_env.att.info.ccc[1] = 0x00;
    ptss_env.att.img_data.ccc[0] = 0x00;
    ptss_env.att.img_data.ccc[1] = 0x00;
    ptss_env.att.img_data.value_length = 0;

    ptss_env.transfer.state = PTSS_STATE_IDLE;
    ptss_env.transfer.bytes_total = 0;

    ptss_env.max_tx_octets = PTSS_MIN_TX_OCTETS;

    /* Add custom attributes into the attribute database. */
    status = APP_BLE_PeripheralServerAddCustomService(ptss_att_db, ATT_PTSS_COUNT,
            &ptss_env.att.attidx_offset);
    if (status != 0)
    {
        return PTSS_ERR_INSUFFICIENT_ATT_DB_SIZE;
    }

    /* Listen for specific BLE kernel messages. */
    MsgHandler_Add(GATTC_CMP_EVT, PTSS_MsgHandler);
    MsgHandler_Add(GAPC_CONNECTION_REQ_IND, PTSS_MsgHandler);
    MsgHandler_Add(GAPC_DISCONNECT_IND, PTSS_MsgHandler);
    MsgHandler_Add(GATTC_MTU_CHANGED_IND, PTSS_MsgHandler);
    MsgHandler_Add(GAPC_LE_PKT_SIZE_IND, PTSS_MsgHandler);

    return PTSS_OK;
}

int32_t PTSS_StartImageTransfer(uint32_t img_size)
{
    int32_t status = PTSS_OK;

    if (img_size > 0)
    {
        if (ptss_env.transfer.state == PTSS_STATE_CAPTURE_REQUEST)
        {
            uint16_t attidx = ptss_env.att.attidx_offset + ATT_PTSS_INFO_VAL_0;
            uint16_t att_handle = GATTM_GetHandle(attidx);
            uint8_t data[PTSS_INFO_IMG_CAPTURED_LENGTH];

            data[0] = PTSS_INFO_OPCODE_IMG_CAPTURED_IND;
            memcpy(data + 1, &img_size, sizeof(img_size));

            GATTC_SendEvtCmd(0, GATTC_NOTIFY, attidx, att_handle,
                    PTSS_INFO_IMG_CAPTURED_LENGTH, data);

            /* Switch to next state to allow data transfers. */
            ptss_env.transfer.state = PTSS_STATE_IMG_INFO_PROVIDED;
            ptss_env.transfer.bytes_total = img_size;
            ptss_env.transfer.bytes_queued = 0;
            ptss_env.transfer.packets_pending = 0;
            ptss_env.att.img_data.value_length = 0;
        }
        else
        {
            status = PTSS_ERR_NOT_PERMITTED;
        }
    }
    else
    {
        status = PTSS_ERR;
    }

    return status;
}

int32_t PTSS_AbortImageTransfer(PTSS_InfoErrorCode_t errcode)
{
    int32_t status = PTSS_OK;

    if (ptss_env.transfer.state >= PTSS_STATE_CAPTURE_REQUEST)
    {
        uint16_t attidx = ptss_env.att.attidx_offset + ATT_PTSS_INFO_VAL_0;
        uint16_t att_handle = GATTM_GetHandle(attidx);
        uint8_t data[2];

        data[0] = PTSS_INFO_OPCODE_ERROR_IND;
        data[1] = errcode;

        GATTC_SendEvtCmd(0, GATTC_NOTIFY, attidx, att_handle, 2, data);

        ptss_env.transfer.state = PTSS_STATE_CONNECTED;
        ptss_env.att.cp.capture_mode = 0;
    }
    else
    {
        status = PTSS_ERR_NOT_PERMITTED;
    }

    return status;
}

int32_t PTSS_GetMaxImageDataPushSize(void)
{
    int32_t avail_bytes;

    if (ptss_env.transfer.state != PTSS_STATE_IMG_DATA_TRANSMISSION)
    {
        avail_bytes = 0;
    }
    else if (ptss_env.transfer.packets_pending == PTSS_MAX_PENDING_PACKET_COUNT)
    {
        avail_bytes = 0;
    }
    else
    {
        /* Determine:
         * (A) Number of packets that can be queued.
         * (B) Number of data  that can fit into single packet.
         * (C) Amount of data that is already queued for transmission in next
         *     packet.
         *
         * avail = (A * B) - C
         */
        avail_bytes = ((PTSS_MAX_PENDING_PACKET_COUNT
                       - ptss_env.transfer.packets_pending)
                      * (PTSS_GetMaxDataOctets() - PTSS_INFO_OFFSET_LENGTH))
                      - (ptss_env.att.img_data.value_length
                         - PTSS_INFO_OFFSET_LENGTH);
    }

    return avail_bytes;
}

int32_t PTSS_ImageDataPush(const uint8_t* p_img_data, const int32_t data_len)
{
    if ((p_img_data == NULL)
        || (data_len <= 0)
        || (data_len > PTSS_IMG_DATA_MAX_SIZE))
    {
        return PTSS_ERR;
    }

    if (ptss_env.transfer.state != PTSS_STATE_IMG_DATA_TRANSMISSION)
    {
       return PTSS_ERR_NOT_PERMITTED;
    }

    /* Safe to retype.
     * Pointer is is used as read-only iterator over the data array.
     */
    uint8_t* p_data = (uint8_t*) p_img_data;
    int32_t bytes_left = data_len;

    while (bytes_left > 0)
    {
        /* Start to fill out new packet if value buffer is clear.
         *
         * Populate notification with first 4 bytes that contain data offset
         * from start of file.
         */
        if (ptss_env.att.img_data.value_length == 0)
        {
            memcpy(ptss_env.att.img_data.value, &ptss_env.transfer.bytes_queued,
                    PTSS_INFO_OFFSET_LENGTH);
            ptss_env.att.img_data.value_length += PTSS_INFO_OFFSET_LENGTH;
        }

        const uint32_t max_data_octets = PTSS_GetMaxDataOctets();
        const uint32_t avail_space = max_data_octets
                                     - ptss_env.att.img_data.value_length;

        ENSURE(avail_space > 0);
        ENSURE(avail_space < ptss_env.max_tx_octets);

        if (avail_space >= bytes_left)
        {
            /* All pending data can be fit into single packet. */
            memcpy(ptss_env.att.img_data.value + ptss_env.att.img_data.value_length,
                    p_data, bytes_left);

            ptss_env.att.img_data.value_length += bytes_left;
            ptss_env.transfer.bytes_queued += bytes_left;
            p_data += bytes_left;
            bytes_left = 0;

            /* Transmit packet if (avail_space == data_len) */
            if (ptss_env.att.img_data.value_length >= max_data_octets)
            {
                ENSURE(ptss_env.att.img_data.value_length == max_data_octets);

                PTSS_TransmitImgDataNotification();
            }
        }
        else
        {
            /* Pending data must be split into multiple packets.
             *
             * Fill as much data as possible into currently open packet.
             */
            memcpy(ptss_env.att.img_data.value + ptss_env.att.img_data.value_length,
                    p_data, avail_space);

            ptss_env.att.img_data.value_length += avail_space;
            ptss_env.transfer.bytes_queued += avail_space;
            p_data += avail_space;
            bytes_left -= avail_space;

            PTSS_TransmitImgDataNotification();
        }

        /* Check if EOF was reached. */
        if (ptss_env.transfer.bytes_queued >= ptss_env.transfer.bytes_total)
        {
            ENSURE(ptss_env.transfer.bytes_queued == ptss_env.transfer.bytes_total);

            /* Transmit any remaining image data. */
            if (ptss_env.att.img_data.value_length > PTSS_INFO_OFFSET_LENGTH)
            {
                PTSS_TransmitImgDataNotification();
            }
        }
    }

    return PTSS_OK;
}

bool PTSS_IsContinuousCapture(void)
{
    bool is_continuous = (ptss_env.transfer.state >= PTSS_STATE_CAPTURE_REQUEST)
                         && (ptss_env.att.cp.capture_mode
                             == PTSS_CONTROL_POINT_OPCODE_CAPTURE_CONTINUOUS_REQ);
    return is_continuous;
}


