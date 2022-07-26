/* ----------------------------------------------------------------------------
 * Copyright (c) 2018 Semiconductor Components Industries, LLC (d/b/a
 * ON Semiconductor), All Rights Reserved
 *
 * Copyright (C) RivieraWaves 2009-2016
 *
 * This module is derived in part from example code provided by RivieraWaves
 * and as such the underlying code is the property of RivieraWaves [a member
 * of the CEVA, Inc. group of companies], together with additional code which
 * is the property of ON Semiconductor. The code (in whole or any part) may not
 * be redistributed in any form without prior written permission from
 * ON Semiconductor.
 *
 * The terms of use and warranty for this code are covered by contractual
 * agreements between ON Semiconductor and the licensee.
 *
 * This is Reusable Code.
 *
 * ----------------------------------------------------------------------------
 * app.h
 * - Main application header
 * ------------------------------------------------------------------------- */

#ifndef APP_H
#define APP_H

/* ----------------------------------------------------------------------------
 * If building with a C++ compiler, make all of the definitions in this header
 * have a C binding.
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
extern "C"
{
#endif    /* ifdef __cplusplus */

/* ----------------------------------------------------------------------------
 * Include files
 * --------------------------------------------------------------------------*/

#include <rsl10.h>
#include <rsl10_protocol.h>
#include <sys_fota.h>
#include <ble_gap.h>
#include <ble_gatt.h>
#include <msg_handler.h>

#include <onsemi_smartshot.h>

#include "calibration.h"
#include "app_sleep.h"
#include "app_trace.h"
#include "app_rtc.h"
#include "app_ble_peripheral_server.h"
#include "app_ble_ptss.h"
#include "app_ble_estss.h"
#include "app_ble_dfus.h"
#include "app_circbuf.h"


/* ----------------------------------------------------------------------------
 * Defines
 * --------------------------------------------------------------------------*/

/* Manufacturer info (ON SEMICONDUCTOR Company ID) */
#define APP_COMPANY_ID                    {0x62, 0x3}
#define APP_COMPANY_ID_LEN                2

/**
 * Size of buffer for storing image data before it gets transmitted over BLE.
 *
 * Used mainly to accumulate enough data to send biggest allowed packet size
 * to minimize number of transmitted packets.
 */
#define APP_IMG_CACHE_SIZE             (8 * SMARTSHOT_ISP_DATA_CHUNK_SIZE)

/**
 * Default sample period for environmental sensor.
 *
 * This can get replaced if a trigger time condition is set over BLE by the
 * client.
 */
#define APP_ENV_SAMPLE_PERIOD_DEFAULT_MS (10000)

/** Maximum duty cycle used by PWM to control LED brightness. */
#define APP_LED_DUTY_CYCLE             (255)

/** Default LED brightness used while RSL10 is awake. */
#define APP_LED_IDLE_PWM_DUTY          (3)

/** LED brightness level used while ISP is capturing image. */
#define APP_LED_CAPTURE_PWM_DUTY       (10)

/** LED brightness level used during image data transfer over BLE. */
#define APP_LED_TRANSFER_PWM_DUTY      (20)

/** LED brightness level used when indicating switch to FOTA mode. */
#define APP_LED_FOTA_ENTER_PWM_DUTY    (128)

/** LED brightness level used when indicating switch to Power Down mode. */
#define APP_LED_PWR_DOWN_ENTER_PWM_DUTY (250)

/** Structure holding all data managed on application level. */
typedef struct APP_Environemnt_t
{
    /**
     * Flag to indicate if SPI transfer for reading of image data is in
     * progress.
     *
     * Makes sure the data are read only in single chunks to not overwhelm
     * limited buffer storage.
     */
    bool isp_read_in_progress;

    /**
     * Circular buffer for temporary storage of image data before it is
     * transmitted over BLE.
     */
    CIRCBUF_t img_cache;

    /**
     * Timestamp of when application received image capture request from peer
     * device.
     *
     * Used to calculate ISP power up time.
     */
    uint32_t time_capture_req;

    /**
     * Used to calculate time it took ISP to capture image.
     *
     * The capture may be variable due to auto exposure settings of ISP.
     */
    uint32_t time_capture_start;

    /**
     * Used to calculate time of transmission of image data over BLE.
     *
     * Time from receiving of data transfer request to last data packet
     * transmission.
     */
    uint32_t time_transfer_start;

    /** Store captured image size on application level.
     *
     * Used to approximate image data transfer speed over BLE.
     */
    uint32_t img_size;

    /** Store flag if DFU Initiated switch to FOTA Update Mode.
     *
     * Set by DFUS callback and is used to enter Device Firmware Update mode.
     */
    bool enter_fota_mode;
} APP_Environemnt_t;

/* ---------------------------------------------------------------------------
* Function prototype definitions
* --------------------------------------------------------------------------*/

void Device_Initialize(void);

void Device_Wakeup(void);

void Device_PrepareForSleep(void);

void Device_Restore(void);

void Main_Loop(void);


void APP_ISP_EventHandler(SMARTSHOT_ISP_MessageId_t msgid, const void *p_param);

void APP_ENV_DataReadyHandler(SMARTSHOT_ENV_SensorData_t *p_data);

void APP_DFU_EventHandler(void);

void APP_PTSS_EventHandler(PTSS_ControlPointOpCode_t opcode,
        const void *p_param);

void APP_ESTSS_EventHandler(ESTSS_TriggerId_t trigger_id);

/* ----------------------------------------------------------------------------
 * Close the 'extern "C"' block
 * ------------------------------------------------------------------------- */
#ifdef __cplusplus
}
#endif    /* ifdef __cplusplus */

#endif    /* APP_H */
