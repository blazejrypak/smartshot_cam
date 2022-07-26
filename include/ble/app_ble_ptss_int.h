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
 * @file APP_BLE_PTSS_INT_H_int.h
 *
 * Internal definitions and types for the PTSS module.
 */

#ifndef APP_BLE_PTSS_INT_H
#define APP_BLE_PTSS_INT_H

#ifdef __cplusplus
extern "C"
{
#endif    /* ifdef __cplusplus */

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/
#include <app_ble_ptss.h>

#include <ble_gap.h>
#include <ble_gatt.h>

/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

#define PTSS_CONTROL_POINT_VALUE_LENGTH (1)
#define PTSS_IMG_INFO_CHAR_VALUE_LENGTH GAPM_DEFAULT_TX_OCT_MAX

#define PTSS_MIN_TX_OCTETS              (27)
#define PTSS_NTF_PDU_HEADER_LENGTH      (2)
#define PTSS_NTF_L2CAP_HEADER_LENGTH    (4)
#define PTSS_NTF_ATT_HEADER_LENGTH      (3)
#define PTSS_INFO_OFFSET_LENGTH         (4)

#define PTSS_CONTROL_POINT_USER_DESC   "Control Point"
#define PTSS_IMG_INFO_USER_DESC        "Info"
#define PTSS_IMG_DATA_USER_DESC        "Image Data"

#define PTSS_CONTROL_POINT_OPCODE_CAPTURE_ONE_SHOT_REQ          (0x01)
#define PTSS_CONTROL_POINT_OPCODE_CAPTURE_CONTINUOUS_REQ        (0x02)
#define PTSS_CONTROL_POINT_OPCODE_CAPTURE_CANCEL_REQ            (0x03)
#define PTSS_CONTROL_POINT_OPCODE_CAPTURE_IMG_DATA_TRANSFER_REQ (0x04)

#define PTSS_INFO_OPCODE_ERROR_IND        (0x00)
#define PTSS_INFO_OPCODE_IMG_CAPTURED_IND (0x01)

#define PTSS_INFO_ERR                     (0x00)
#define PTSS_INFO_ERR_CANCELLED           (0x01)
#define PTSS_INFO_IMG_CAPTURED_LENGTH     (5)

#define PTSS_IMG_DATA_SIZE_OFFSET (0)
#define PTSS_IMG_DATA_SIZE_LENGTH (4)
#define PTSS_IMG_DATA_OFFSET      (4)

#define PTSS_MAX_PENDING_PACKET_COUNT  (5)

/** List of application specific BLE error codes that can be send in
 * ATT_ERROR_RSP PDUs.
 */
typedef enum PTSS_AttErr_t
{
    PTSS_ATT_ERR_NTF_DISABLED              = 0x80,
    PTSS_ATT_ERR_PROC_IN_PROGRESS          = 0x81,
    PTSS_ATT_ERR_IMG_TRANSFER_DISALLOWED   = 0x82,
} PTSS_AttErr_t;

typedef enum PTSS_TransferState_t
{
    PTSS_STATE_IDLE,
    PTSS_STATE_CONNECTED,
    PTSS_STATE_CAPTURE_REQUEST,
    PTSS_STATE_IMG_INFO_PROVIDED,
    PTSS_STATE_IMG_DATA_TRANSMISSION,
} PTSS_State_t;

/** Stores values required for Control Point Characteristic attributes. */
typedef struct PTSS_ControlPointCharacteristic_t
{
    /** Application Write callback. */
    PTSS_ControlHandler callback;

    uint8_t value[PTSS_CONTROL_POINT_VALUE_LENGTH];

    /** Type of capture operation. */
    uint8_t capture_mode;
} PTSS_ControlPointAttribute_t;

/** Stores values required for Info Characteristic attributes. */
typedef struct PTSS_InfoCharacteristic_t
{
    /** Client Characteristic Configuration Descriptor Value */
    uint8_t ccc[2];
} PTSS_InfoCharacteristic_t;

/** Stores values required for Image Info Characteristic attributes. */
typedef struct PTSS_ImageDataCharacteristic_t
{
    /**
     * Number of bytes currently stored in #value that are queued for
     * transmission.
     */
    uint32_t value_length;

    uint8_t value[PTSS_IMG_INFO_CHAR_VALUE_LENGTH];

    /** Client Characteristic Configuration Descriptor Value */
    uint8_t ccc[2];
} PTSS_ImageDataCharacteristic_t;

/**
 * Collects all attribute database related variables of Picture Transfer
 * Service.
 */
typedef struct PTSS_AttDb_t
{
    /**
     * Offset of the attidx of PTSS service attributes.
     *
     * This can change depending on number and order of registered custom
     * services to shared attribute database.
     */
    uint16_t attidx_offset;

    PTSS_ControlPointAttribute_t cp;
    PTSS_InfoCharacteristic_t info;
    PTSS_ImageDataCharacteristic_t img_data;
} PTSS_AttDb_t;

/**
 * Retains all information required to execute complete image data transfers to
 * connected peer device.
 */
typedef struct PTSS_ImageTransferControl_t
{
    /** Current status of image transfer procedure. */
    PTSS_State_t state;

    /** Total size of the image data that needs to be transfered. */
    uint32_t bytes_total;

    /** */
    uint32_t bytes_queued;

    /** */
    uint8_t packets_pending;
} PTSS_ImageTransferControl_t;

typedef struct PTSS_Environment_t
{
    /**
     * Stores all attribute database related variables.
     */
    PTSS_AttDb_t att;

    /**
     * Stores progress information of ongoing image data transfers.
     */
    PTSS_ImageTransferControl_t transfer;

    /**
     * Maximum PDU size negotiated using Data Length Extension (DLE) for
     * current connection.
     *
     * Speeds up data transfers.
     * Available on on Bluetooth 4.2 and newer devices.
     * On 4.0 devices the value always remains at PTSS_MIN_TX_OCTETS .
     */
    uint16_t max_tx_octets;
} PTSS_Environment_t;

/* ----------------------------------------------------------------------------
 * Global variables and types
 * --------------------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* APP_BLE_PTSS_INT_H */
