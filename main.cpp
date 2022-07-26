////============================================================================
//// Name        : main.cpp
//// Author      : Blazej Rypak
//// Version     :
//// Copyright   : Your copyright notice
//// Description : Hello World in C++
////============================================================================
//
//#include <iostream>
//using namespace std;
//
////
//// Print a greeting message on standard output and exit.
////
//// On embedded platforms this might require semi-hosting or similar.
////
//// For example, for toolchains derived from GNU Tools for Embedded,
//// to enable semi-hosting, the following was added to the linker:
////
//// `--specs=rdimon.specs -Wl,--start-group -lgcc -lc -lc -lm -lrdimon -Wl,--end-group`
////
//// Adjust it for other toolchains.
////
//// If functionality is not required, to only pass the build, use
//// `--specs=nosys.specs`.
////
//
//int
//main()
//{
//  cout << "Hello Arm World!" << endl;
//  return 0;
//}

/* ----------------------------------------------------------------------------
 * Copyright (c) 2018 Semiconductor Components Industries, LLC (d/b/a
 * ON Semiconductor), All Rights Reserved
 *
 * Copyright (C) RivieraWaves 2009-2018
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
 * app.c
 * - Main application file
 * ------------------------------------------------------------------------- */

#include <app.h>
#include <Driver_DMA.h>
#include <Driver_SPI.h>
#include <Driver_I2C.h>

#include <RTE_Device.h>
#include <co_error.h>

#include "main_functions.h"

DEFINE_THIS_FILE_FOR_ASSERT;

static APP_Environemnt_t app_env = { 0 };

/** Cache for storing of image data before it gets transmitted over BLE. */
static uint8_t app_img_cache_storage[APP_IMG_CACHE_SIZE];

/**
 * Poll current state of on-board push button.
 *
 * @return
 * true - If push button is pressed.
 * false - If push button is released.
 */
static bool APP_BTN_IsPressed(void)
{
    return (DIO_DATA->ALIAS[SMARTSHOT_PIN_PUSH_BUTTON] == 0);
}

/**
 * Called when starting image transfer from ISP to RSL10 and then after every
 * transaction.
 *
 * Ensures SPI transfer is always active as long as there is enough space in
 * image cache.
 */
static void APP_ISP_ReadNextDataChunk(void)
{
    if ((!app_env.isp_read_in_progress)
        && (CIRCBUF_GetFree(&app_env.img_cache)
            >= SMARTSHOT_ISP_DATA_CHUNK_SIZE))
    {
        /* Start to read image data from ISP over SPI. */
        SMARTSHOT_ISP_ReadImageDataCommand(1);

        app_env.isp_read_in_progress = true;
    }
}

/**
 * Attempts to push image data from buffer to BLE service every time new chunk
 * of data is received or BLE indicates it transmitted a packet.
 */
static void APP_PTSS_PushImageData(void)
{
    do
    {
        uint32_t max_data_to_push = PTSS_GetMaxImageDataPushSize();
        uint32_t data_available = CIRCBUF_GetUsed(&app_env.img_cache);

        if ((max_data_to_push > 0) && (data_available > 0))
        {
            uint8_t buf[SMARTSHOT_ISP_DATA_CHUNK_SIZE];
            uint32_t status;
            uint32_t buf_to_write =
                    (max_data_to_push > data_available) ? data_available :
                                                          max_data_to_push;

            if (buf_to_write > SMARTSHOT_ISP_DATA_CHUNK_SIZE)
            {
                buf_to_write = SMARTSHOT_ISP_DATA_CHUNK_SIZE;
            }

            status = CIRCBUF_PopFront(buf, buf_to_write, &app_env.img_cache);
            ENSURE(status == 0);

            status = PTSS_ImageDataPush(buf, buf_to_write);
            ENSURE(status == PTSS_OK);

            /* for unused variable warnings. */
            (void)status;
        }
        else
        {
            break;
        }
    } while (1);
}

#if (CFG_SMARTSHOT_PRINTF_INTERFACE != SMARTSHOT_PRINTF_INTERFACE_DISABLED)

/**
 * Print verbose information about change of sensor trigger settings to debug
 * terminal.
 *
 * @param tidx
 * Index of trigger to print information about.
 */
static void APP_ESTSS_DebugPrintConfiguration(ESTSS_TriggerId_t tidx)
{
    static const char* app_trig_name[ESTSS_TRIGGER_COUNT] =
    {
        [ESTSS_TRIGGER_MOTION]       = "MOTION",
        [ESTSS_TRIGGER_ACCELERATION] = "ACCELERATION",
        [ESTSS_TRIGGER_TEMPERATURE]  = "TEMPERATURE",
        [ESTSS_TRIGGER_HUMIDITY]     = "HUMIDITY"
    };

    static const char* val_cond_name[8] =
    {
        [ESTS_TRIG_VAL_CHANGED]           = "VALUE_CHANGED",
        [ESTS_TRIG_VAL_CROSSED_BOUNDARY]  = "CROSSED_BOUNDARY",
        [ESTS_TRIG_VAL_ON_BOUNDARY]       = "ON_BOUNDARY",
        [ESTS_TRIG_VAL_CHANGED_MORE_THAN] = "CHANGED_MORE_THAN",
        [ESTS_TRIG_VAL_MASK_COMPARE]      = "MASK_COMPARE",
        [ESTS_TRIG_VAL_CRROSSED_INTERVAL] = "CROSSED_INTERVAL",
        [ESTS_TRIG_VAL_ON_INTERVAL]       = "ON_INTERVAL",
        [ESTS_TRIG_VAL_NO_TRIGGER]        = "NO_TRIGGER",
    };

    static const char* time_cond_name[4] =
    {
        [ESTS_TRIG_TIME_NO_TRIGGER]      = "NO_TRIGGER",
        [ESTS_TRIG_TIME_PERIODIC]        = "PERIODIC",
        [ESTS_TRIG_TIME_MIN_INTERVAL]    = "MIN_INTERVAL",
        [ESTS_TRIG_TIME_ON_CHANGE_COUNT] = "ON_CHANGE_COUNT",
    };

    if (ESTSS_TriggerIsEnabled(tidx) == true)
    {
        ESTSS_TriggerSettings_t conf;

        ESTSS_GetTriggerSettings(tidx, &conf);

        PRINTF("APP: Enabled %s trigger.\r\n  val_cond=%s", app_trig_name[tidx],
            val_cond_name[conf.cond_value]);

        /* Print value trigger setting specific arguments. */
        switch (conf.cond_value)
        {
        case ESTS_TRIG_VAL_CROSSED_BOUNDARY:
        case ESTS_TRIG_VAL_ON_BOUNDARY:
        case ESTS_TRIG_VAL_CHANGED_MORE_THAN:
            PRINTF(", boundary=%d", conf.boundary[0]);
            break;

        case ESTS_TRIG_VAL_CRROSSED_INTERVAL:
        case ESTS_TRIG_VAL_ON_INTERVAL:
            PRINTF(", int_low=%d, int_high=%d", conf.boundary[0],
                conf.boundary[1]);
            break;

        default:
            break;
        }

        /* Print time trigger setting specific arguments. */
        PRINTF("\r\n  time_cond=%s", time_cond_name[conf.cond_time]);
        switch (conf.cond_time)
        {
        case ESTS_TRIG_TIME_PERIODIC:
        case ESTS_TRIG_TIME_MIN_INTERVAL:
            PRINTF(", interval=%d ms", conf.time_interval);
            break;

        default:
            break;
        }

        PRINTF("\r\n");
    }
    else
    {
        PRINTF("APP: Disabled %s trigger.\r\n", app_trig_name[tidx]);
    }
}

#endif /* if (CFG_SMARTSHOT_PRINTF_INTERFACE != SMARTSHOT_PRINTF_INTERFACE_DISABLED) */

/**
 * Processes events generated by ISP library.
 *
 * @param msgid
 * @param p_param
 */
void APP_ISP_EventHandler(SMARTSHOT_ISP_MessageId_t msgid, const void *p_param)
{
    switch (msgid)
    {
        case SMARTSHOT_ISP_ERROR_IND:
        {
//            const SMARTSHOT_ISP_ErrorInd_t *p_err_ind = p_param;
            const SMARTSHOT_ISP_ErrorInd_t *p_err_ind = const_cast<SMARTSHOT_ISP_ErrorInd_t*>(reinterpret_cast<const SMARTSHOT_ISP_ErrorInd_t*>(p_param));
            PRINTF("\r\nISP: ERROR_IND state=%d error=%d\r\n", p_err_ind->state,
                    p_err_ind->error);

            PTSS_AbortImageTransfer(PTSS_INFO_ERR_ABORTED_BY_SERVER);
            SMARTSHOT_ISP_PowerDownCommand();
            break;
        }

        /* ISP indicates that it is ready to capture new image. */
        case SMARTSHOT_ISP_READY_IND:
        {
//            const SMARTSHOT_ISP_ReadyInd_t *p_ready_ind = p_param;
            const SMARTSHOT_ISP_ReadyInd_t *p_ready_ind = const_cast<SMARTSHOT_ISP_ReadyInd_t*>(reinterpret_cast<const SMARTSHOT_ISP_ReadyInd_t*>(p_param));
            PRINTF("ISP: READY_IND reason=%d\r\n", p_ready_ind->reason);

            if (p_ready_ind->reason == SMARTSHOT_ISP_READY_IMAGE_TRANSFER_COMPLETE)
            {
                if (PTSS_IsContinuousCapture() == true)
                {
                    SMARTSHOT_ISP_CaptureCommand();

                    app_env.time_capture_start = APP_RTC_GetTimeMs();
                }
                else
                {
                    SMARTSHOT_ISP_PowerDownCommand();
                }
            }
            else
            {
                /* Power up is always caused by the need to take picture so just
                 * store the time when capture started for debugging.
                 */
                app_env.time_capture_start = APP_RTC_GetTimeMs();
                PRINTF("STAT: time_power_up = %d ms\r\n",
                    (app_env.time_capture_start - app_env.time_capture_req));
            }

            break;
        }

        /* ISP informs application that it has finished processing of image and
         * data are ready to be transferred over SPI.
         */
        case SMARTSHOT_ISP_IMAGE_INFO_IND:
        {
//            const SMARTSHOT_ISP_ImageInfo_t *p_img_info = p_param;
            const SMARTSHOT_ISP_ImageInfo_t *p_img_info = const_cast<SMARTSHOT_ISP_ImageInfo_t*>(reinterpret_cast<const SMARTSHOT_ISP_ImageInfo_t*>(p_param));
            int32_t status;

            PRINTF("ISP: IMAGE_INFO_IND size=%d width=%d height=%d\r\n",
                    p_img_info->size, p_img_info->width, p_img_info->height);

            app_env.img_size = p_img_info->size;

            /* Print time it took to take picture. */
            PRINTF("STAT: time_capture = %d ms\r\n",
                (APP_RTC_GetTimeMs() - app_env.time_capture_start));

            /* Notify connected peer device that image data are ready. */
            status = PTSS_StartImageTransfer(p_img_info->size);
            if (status != PTSS_OK)
            {
                /* Abort image capture. */
                PRINTF("PTSS: Rejected image info (err=%d)\r\n", status);

                SMARTSHOT_ISP_PowerDownCommand();
            }

            break;
        }

        /* A chunk of image data was read from ISP and is ready to be processed
         * by application.
         */
        case SMARTSHOT_ISP_IMAGE_DATA_IND:
        {
            int32_t status;
//            const SMARTSHOT_ISP_ImageData_t *p_img_data = p_param;
            const SMARTSHOT_ISP_ImageData_t *p_img_data = const_cast<SMARTSHOT_ISP_ImageData_t*>(reinterpret_cast<const SMARTSHOT_ISP_ImageData_t*>(p_param));


#if (CFG_SMARTSHOT_PRINTF_INTERFACE != SMARTSHOT_PRINTF_INTERFACE_DISABLED)
            /* Print status message after every 4KB of image data is read from
             * ISP to not flood the debug output.
             */
            if ((p_img_data->offset & 0xFFF) == 0)
            {
                PRINTF("ISP: IMAGE_DATA_IND offset=%d\r\n", p_img_data->offset);
            }
#endif /* (CFG_SMARTSHOT_PRINTF_INTERFACE != SMARTSHOT_PRINTF_INTERFACE_DISABLED) */

            REQUIRE(app_env.isp_read_in_progress == true);
            app_env.isp_read_in_progress = false;

            /* Store received data in cache. */
            status = CIRCBUF_PushBack((uint8_t*) p_img_data->data,
                    p_img_data->size, &app_env.img_cache);
            /* Enough free space is guaranteed when data retrieval is started. */
            ENSURE(status == 0);

            /* Try to pass cached data to PTSS. */
            APP_PTSS_PushImageData();

            /* Start next image data transfer if enough cache space is
             * available.
             */
            APP_ISP_ReadNextDataChunk();

            break;
        }

        default:
            ASSERT(false);
            break;
    }
}

/**
 * Event handler for the SMARTSHOT_ENV library.
 */
void APP_ENV_DataReadyHandler(SMARTSHOT_ENV_SensorData_t *p_data)
{
    REQUIRE(p_data != NULL);

    PRINTF("ENV: temp=%d hum=%d\r\n", p_data->temperature,
            p_data->humidity);

    /* Push new data to BLE service so it generate notifications based on set
     * up trigger conditions.
     */
    ESTSS_PushTemperatureValue(p_data->temperature);
    ESTSS_PushHumidityValue(p_data->humidity);
}

/**
 * Event handler for the Device Firmware Update Server BLE service.
 */
void APP_DFU_EventHandler(void)
{
    app_env.enter_fota_mode = true;

    /* Going to FOTA Mode
     * 0x13 - CONNECTION TERMINATED BY REMOTE USER
     */
    APP_BLE_PeripheralServerDisconnect(CO_ERROR_REMOTE_USER_TERM_CON);
}

/**
 * Event handler for the Picture Transfer Service Server BLE service.
 */
void APP_PTSS_EventHandler(PTSS_ControlPointOpCode_t opcode,
        const void *p_param)
{
    switch (opcode)
    {
        /* Connected peer device requested capture of single image. */
        case PTSS_OP_CAPTURE_ONE_SHOT_REQ:
        {
            PRINTF("PTSS: CAPTURE_ONE_SHOT_REQ\r\n");

            APP_BLE_UpdateConnectionParameters(APP_UPD_CONN_LOW_LATENCY);

            SMARTSHOT_ISP_CaptureCommand();

            app_env.time_capture_req = APP_RTC_GetTimeMs();

            /* Increase LED brightness during capture. */
            Sys_PWM_Config(0, APP_LED_DUTY_CYCLE, APP_LED_CAPTURE_PWM_DUTY);
            break;
        }

        /* Connected peer device requested continuous image capture. */
        case PTSS_OP_CAPTURE_CONTINUOUS_REQ:
        {
            PRINTF("PTSS: CAPTURE_CONTINUOUS_REQ\r\n");

            APP_BLE_UpdateConnectionParameters(APP_UPD_CONN_LOW_LATENCY);

            SMARTSHOT_ISP_CaptureCommand();

            app_env.time_capture_req = APP_RTC_GetTimeMs();

            /* Increase LED brightness during capture. */
            Sys_PWM_Config(0, APP_LED_DUTY_CYCLE, APP_LED_CAPTURE_PWM_DUTY);
            break;
        }

        /* Connected peer device requested to abort any ongoing capture
         * operation.
         */
        case PTSS_OP_CAPTURE_CANCEL_REQ:
        {
            PRINTF("PTSS: CAPTURE_CANCEL_REQ\r\n");

            APP_BLE_UpdateConnectionParameters(APP_UPD_CONN_LOW_POWER);

            SMARTSHOT_ISP_PowerDownCommand();

            Sys_PWM_Config(0, APP_LED_DUTY_CYCLE, APP_LED_IDLE_PWM_DUTY);
            break;
        }

        /* Connected peer device indicates that it received image info and is
         * ready to accept image data.
         */
        case PTSS_OP_IMAGE_DATA_TRANSFER_REQ:
        {
            PRINTF("PTSS: IMAGE_DATA_TRANSFER_REQ\r\n");
            app_env.time_transfer_start = APP_RTC_GetTimeMs();

            /* Reset circular buffer to receive new image. */
            CIRCBUF_Initialize(app_img_cache_storage, APP_IMG_CACHE_SIZE,
                    &app_env.img_cache);
            app_env.isp_read_in_progress = 0;

            APP_ISP_ReadNextDataChunk();

            Sys_PWM_Config(0, APP_LED_DUTY_CYCLE, APP_LED_TRANSFER_PWM_DUTY);
            break;
        }

        case PTSS_OP_IMAGE_DATA_TRANSFER_DONE_IND:
        {
            PRINTF("PTSS: PTSS_OP_IMAGE_DATA_TRANSFER_DONE_IND\r\n");

            if(!PTSS_IsContinuousCapture())
            {
                APP_BLE_UpdateConnectionParameters(APP_UPD_CONN_LOW_POWER);
            }

            /* Print image transfer statistics */
            uint32_t time_transfer_done = APP_RTC_GetTimeMs();
            PRINTF("STAT: time_transfer = %d ms\r\n",
                (time_transfer_done - app_env.time_transfer_start));
            PRINTF("STAT: transfer_rate = %d Bps\r\n",
                app_env.img_size * 1000
                    / (time_transfer_done - app_env.time_transfer_start));

            /* Return LED brightness into idle level.  */
            Sys_PWM_Config(0, APP_LED_DUTY_CYCLE, APP_LED_IDLE_PWM_DUTY);
            break;
        }

        /* PTSS is able to accept more image data. */
        case PTSS_OP_IMAGE_DATA_SPACE_AVAIL_IND:
        {
            /* Push any cached data. */
            APP_PTSS_PushImageData();

            /* Start receiving more image data if cache has enough free space.
             */
            APP_ISP_ReadNextDataChunk();

            break;
        }

        default:
        {
            ASSERT(false);
            break;
        }
    }
}

/**
 * Event handler for the External Sensor Trigger Service Server BLE service.
 *
 * Enables corresponding sensors based on configuration set by peer device.
 */
void APP_ESTSS_EventHandler(ESTSS_TriggerId_t tidx)
{
#if (CFG_SMARTSHOT_PRINTF_INTERFACE != SMARTSHOT_PRINTF_INTERFACE_DISABLED)
    /* Print verbose information about updated trigger configuration. */
    APP_ESTSS_DebugPrintConfiguration(tidx);
#endif /* if (CFG_SMARTSHOT_PRINTF_INTERFACE != SMARTSHOT_PRINTF_INTERFACE_DISABLED) */

    switch (tidx)
    {
        case ESTSS_TRIGGER_MOTION:
        {
            if (ESTSS_TriggerIsEnabled(tidx))
            {
                SMARTSHOT_PIR_Enable();
            }
            else
            {
                SMARTSHOT_PIR_Disable();
            }

            break;
        }

        case ESTSS_TRIGGER_ACCELERATION:
        {
            if (ESTSS_TriggerIsEnabled(tidx))
            {
                if (SMARTSHOT_ACCEL_GetState() == SMARTSHOT_ACCEL_STATE_READY)
                {
                    SMARTSHOT_ACCEL_EnableMotionDetection();
                }
            }
            else
            {
                if (SMARTSHOT_ACCEL_GetState() == SMARTSHOT_ACCEL_STATE_ACTIVE)
                {
                    SMARTSHOT_ACCEL_DisableMotionDetection();
                }
            }

            break;
        }

        case ESTSS_TRIGGER_TEMPERATURE:
        case ESTSS_TRIGGER_HUMIDITY:
        {
            /* Enable environmental sensor if either temperature or humidity
             * trigger is enabled.
             */
            if (ESTSS_TriggerIsEnabled(ESTSS_TRIGGER_TEMPERATURE)
                || ESTSS_TriggerIsEnabled(ESTSS_TRIGGER_HUMIDITY))
            {
                int32_t status;
                ESTSS_TriggerSettings_t settings;

                /* Default sample period to be used if no time condition is
                 * set.
                 */
                uint32_t sample_period_ms = APP_ENV_SAMPLE_PERIOD_DEFAULT_MS;

                /* Reduce sample period if time setting is applied for
                 * temperature.
                 */
                ESTSS_GetTriggerSettings(ESTSS_TRIGGER_TEMPERATURE, &settings);
                if ((ESTSS_TriggerIsEnabled(ESTSS_TRIGGER_TEMPERATURE) == true)
                    && (settings.cond_value != ESTS_TRIG_VAL_NO_TRIGGER)
                    && (settings.cond_time == ESTS_TRIG_TIME_MIN_INTERVAL))
                {
                    if (sample_period_ms > settings.time_interval)
                    {
                        sample_period_ms = settings.time_interval;
                    }
                }

                /* Reduce sample period if time setting is applied for
                 * humidity.
                 */
                ESTSS_GetTriggerSettings(ESTSS_TRIGGER_HUMIDITY, &settings);
                if ((ESTSS_TriggerIsEnabled(ESTSS_TRIGGER_HUMIDITY) == true)
                    && (settings.cond_value != ESTS_TRIG_VAL_NO_TRIGGER)
                    && (settings.cond_time == ESTS_TRIG_TIME_MIN_INTERVAL))
                {
                    if (sample_period_ms > settings.time_interval)
                    {
                        sample_period_ms = settings.time_interval;
                    }
                }

                /* Restart measurement with updated configuration. */
                SMARTSHOT_ENV_StopMeasurement();
                status = SMARTSHOT_ENV_StartMeasurement(sample_period_ms);
                if (status != 0)
                {
                    PRINTF("APP: Failed to start environmental sensor (err=%d).\r\n",
                            status);
                }
            }
            else
            {
                SMARTSHOT_ENV_StopMeasurement();
            }
            break;
        }

        default:
            ASSERT(false);
            break;
    }
}

int main(void)
{
    /* Configure hardware and initialize BLE stack */
    Device_Initialize();

    /* Debug/trace initialization. In order to enable UART or RTT trace,
     * configure the 'OUTPUT_INTERFACE' macro in printf.h */
    PRINTF("\n\r" CFG_APP_NAME " has started!\r\n");

    setup();

    /* Enter into flash deep sleep if button is pressed after reset. */
    if (APP_BTN_IsPressed() == true)
    {
        PRINTF("APP: Entering into storage mode.\r\n");

        Sys_PWM_Config(0, APP_LED_DUTY_CYCLE, APP_LED_PWR_DOWN_ENTER_PWM_DUTY);
        for (int i = 0; i < 16*3; ++i)
        {
            Sys_Watchdog_Refresh();
            Sys_Delay_ProgramROM(SystemCoreClock/16);
        }

        Device_PrepareForSleep();

        APP_EnterFlashSleep();
    }

    CIRCBUF_Initialize(app_img_cache_storage, APP_IMG_CACHE_SIZE,
            &app_env.img_cache);

#if (CFG_SMARTSHOT_APP_POWER_ISP_ON_BOOT == 1)
    /* Power-up ISP to allow to update ISP firmware over USB.
     * RSL10 will not enter into sleep mode if this option is enabled!
     */
    SMARTSHOT_ISP_PowerUpCommand(SMARTSHOT_ISP_FW_UPDATE_ENABLE);
#endif /* if (CFG_SMARTSHOT_APP_POWER_ISP_ON_BOOT == 1) */

//    Main_Loop();

    while (1) {
    	loop();
    }
}

void Main_Loop(void)
{
    bool isp_busy = false;

    while (1)
    {
        Sys_Watchdog_Refresh();

        Kernel_Schedule();

        loop();

        isp_busy = SMARTSHOT_ISP_MainLoop();


        if (SMARTSHOT_PIR_IsEventPending() == true)
        {
            bool detection_state = SMARTSHOT_PIR_DetectionState();
            PRINTF("PIR: Motion event %s.\r\n", detection_state ? "START" : "END");
            ESTSS_PushMotionValue(detection_state);
            ESTSS_PushMotionValue(0);
            SMARTSHOT_PIR_EventClear();
        }

        if (SMARTSHOT_ACCEL_IsEventPending() == true)
        {
            PRINTF("ACCEL: Acceleration event detected!\r\n");
            SMARTSHOT_ACCEL_ClearEvent();

            ESTSS_PushAccelerationValue(true);
            ESTSS_PushAccelerationValue(false);
        }

        SMARTSHOT_ENV_MainLoop();

        /* Operations to be executed only when device is advertising. */
        if (APP_BLE_PeripheralServerIsAdvertising())
        {
            /* Enter FOTA mode if button is pressed. */
            if(APP_BTN_IsPressed())
            {
                app_env.enter_fota_mode = true;
            }

            /* Enter FOTA mode if initiated either from button press of DFU Service. */
            if (app_env.enter_fota_mode == true)
            {
                /* Power down ISP before entering FOTA mode if ISP FW update
                 * debug feature is enabled.
                 */
                if (SMARTSHOT_ISP_IsPowered() == true)
                {
                    SMARTSHOT_ISP_PowerDownCommand();
                }

                /* Enter FOTA Mode only if all sensors are power down */
                if((SMARTSHOT_ISP_IsPowered() == false) &&
                   (SMARTSHOT_ACCEL_GetState() == SMARTSHOT_ACCEL_STATE_READY) &&
                   (SMARTSHOT_PIR_GetState() == false) &&
                   (SMARTSHOT_ENV_SENSOR_GetState() == SMARTSHOT_ENV_STATE_IDLE)
                )
                {
                    app_env.enter_fota_mode = false;
                    PRINTF("APP: Entering FOTA mode.\r\n");
                    APP_EnterFotaMode();
                }
            }
        }

#if (CFG_SMARTSHOT_APP_SLEEP_ENABLED == 1)
            /* Allow to enter into deep sleep mode if following conditions are
             * met:
             *
             * - ISP power down sequence completed.
             * - Device is in Advertising mode or Connected mode with Low Power Connection Parameters
             */
            if ((SMARTSHOT_ISP_IsPowered() == false) &&
                (APP_BLE_PeripheralServerIsAdvertising() ||
                 APP_BLE_PeripheralServerConnectedInLowPowerParams()))
            {
                APP_EnterSleep();
            }
#endif /* if (CFG_SMARTSHOT_APP_SLEEP_ENABLED == 1) */


        /* Wait for event. */
        if (!isp_busy)
        {
            SMARTSHOT_TRACE_ON_TASK_STOP_READY(app_main_task_id, 0);
            SMARTSHOT_TRACE_ON_IDLE();

            SYS_WAIT_FOR_INTERRUPT;

            SMARTSHOT_TRACE_ON_TASK_START_EXEC(app_main_task_id);
        }
    }
}
