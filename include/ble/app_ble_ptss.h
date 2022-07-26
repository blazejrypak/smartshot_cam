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
 * @file APP_BLE_PTSS_H.h
 *
 * Defines list of custom application specific attributes that are used when
 * creating BLE attribute database.
 */

#ifndef APP_BLE_PTSS_H
#define APP_BLE_PTSS_H

#ifdef __cplusplus
extern "C"
{
#endif    /* ifdef __cplusplus */

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/
#include <rsl10_ke.h>
#include <gattc_task.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define PTSS_SVC_UUID \
    { 0xF8, 0x85, 0x74, 0xD2, 0x2D, 0x01, \
      0xDA, 0xB5, \
      0x62, 0x03, \
      0x01, 0x00, \
      0x04, 0x00, 0x00, 0x00 }

#define PTSS_CHAR_CONTROL_POINT_UUID \
    { 0xF8, 0x85, 0x74, 0xD2, 0x2D, 0x01, \
      0xDA, 0xB5, \
      0x62, 0x03, \
      0x02, 0x00, \
      0x04, 0x00, 0x00, 0x00 }

#define PTSS_CHAR_INFO_UUID \
    { 0xF8, 0x85, 0x74, 0xD2, 0x2D, 0x01, \
      0xDA, 0xB5, \
      0x62, 0x03, \
      0x03, 0x00, \
      0x04, 0x00, 0x00, 0x00 }

#define PTS_CHAR_IMAGE_DATA_UUID \
    { 0xF8, 0x85, 0x74, 0xD2, 0x2D, 0x01, \
      0xDA, 0xB5, \
      0x62, 0x03, \
      0x04, 0x00, \
      0x04, 0x00, 0x00, 0x00 }

#define PTSS_IMG_DATA_MAX_SIZE         (GAPM_DEFAULT_MTU_MAX - 7)

typedef enum PTSS_ApiError_t
{
    PTSS_OK = 0,
    PTSS_ERR = -1,
    PTSS_ERR_NOT_PERMITTED = -2,
    PTSS_ERR_INSUFFICIENT_ATT_DB_SIZE = -3,
} PTSS_ApiError_t;

typedef enum PTSS_AttIdx_t
{
    /* Picture Transfer Service 0 */
    ATT_PTSS_SERVICE_0,

    /* Picture Transfer Control Point Characteristic */
    ATT_PTSS_CONTROL_POINT_CHAR_0,
    ATT_PTSS_CONTROL_POINT_VAL_0,
    ATT_PTSS_CONTROL_POINT_DESC_0,

    /* Picture Transfer Info Characteristic */
    ATT_PTSS_INFO_CHAR_0,
    ATT_PTSS_INFO_VAL_0,
    ATT_PTSS_INFO_CCC_0,
    ATT_PTSS_INFO_DESC_0,

    /* Picture Transfer Image Data Characteristic */
    ATT_PTSS_IMAGE_DATA_CHAR_0,
    ATT_PTSS_IMAGE_DATA_VAL_0,
    ATT_PTSS_IMAGE_DATA_CCC_0,
    ATT_PTSS_IMAGE_DATA_DESC_0,

    /* Total number of all custom attributes of PTSS. */
    ATT_PTSS_COUNT,
} PTSS_AttIdx_t;

typedef enum PTSS_ControlPointOpCode_t
{
    /**
     * Generated when an one-shot image capture command is received over BLE
     * from peer device.
     */
    PTSS_OP_CAPTURE_ONE_SHOT_REQ,

    /**
     * Generated when an continuous image capture command is received over BLE.
     */
    PTSS_OP_CAPTURE_CONTINUOUS_REQ,

    /**
     * Generated when capture cancel command is received over BLE from peer
     * device.
     */
    PTSS_OP_CAPTURE_CANCEL_REQ,

    /**
     * Generated after peer device received image info and is ready to receive
     * image data.
     */
    PTSS_OP_IMAGE_DATA_TRANSFER_REQ,

    /**
     * Generated when all image data were successfully transferred.
     */
    PTSS_OP_IMAGE_DATA_TRANSFER_DONE_IND,

    /**
     * Generated during image data transfer to inform application that PTSS is
     * ready to accept more image data.
     */
    PTSS_OP_IMAGE_DATA_SPACE_AVAIL_IND,
} PTSS_ControlPointOpCode_t;

typedef enum PTSS_InfoErrorCode_t
{
    PTSS_INFO_ERR_ABORTED_BY_SERVER = 0x00,
    PTSS_INFO_ERR_ABORTED_BY_CLIENT = 0x01,
} PTSS_InfoErrorCode_t;

typedef void (*PTSS_ControlHandler)(PTSS_ControlPointOpCode_t opcode,
        const void *p_param);

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------
 * Function prototype definitions
 * --------------------------------------------------------------------------*/

/**
 *
 * @pre
 * BLE stack was initialized using @ref APP_BLE_PeripheralServerInitialize
 * with att_db size of at least ATT_PTSS_COUNT
 *
 * @param control_event_handler
 * Application callback function that informs application of any PTSS related
 * events.
 *
 * @return
 */
int32_t PTSS_Initialize(PTSS_ControlHandler control_event_handler);

/**
 *
 * @pre
 * PTSS was initialized and there is active connection.
 *
 * @param img_size
 * Total size of image in bytes.
 */
int32_t PTSS_StartImageTransfer(uint32_t img_size);

/**
 * Inform client that image capture or transfer operation was aborted with given
 * error code.
 *
 * @param errcode
 * Reason for aborting of the operation.
 *
 * @return
 * PTSS_OK - Abort notification was transmitted. <br>
 * PTSS_ERR_NOT_PERMITTED - There is either no connection established or there
 *     is no ongoing image capture or transfer operation.
 */
int32_t PTSS_AbortImageTransfer(PTSS_InfoErrorCode_t errcode);

int32_t PTSS_GetMaxImageDataPushSize(void);

/**
 * Queue image data for transmission over BLE.
 *
 * The service automatically merges data to as little packets as possible
 * depending on the currently negotiated MTU for the active connection.
 *
 * @pre
 * Image info was provided using @ref PTSS_SendImageInfo before any image data
 * are provided.
 * This is necessary in order to determine whether all image data were
 * transmitted or not to send last shorter data packet.
 *
 * @param p_img_data
 *
 * @param data_len
 * Number of bytes to queue for transmission.
 *
 * @return
 * Actual number of bytes queued for transmission or 0 if unable to queue any
 * additional bytes.
 * Negative error code on error.
 */
int32_t PTSS_ImageDataPush(const uint8_t* p_img_data, const int32_t data_len);

bool PTSS_IsContinuousCapture(void);

#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* APP_BLE_PTSS_H */
