/*
 * Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is confidential property of Nordic Semiconductor. The use,
 * copying, transfer or disclosure of such information is prohibited except by express written
 * agreement with Nordic Semiconductor.
 *
 */

/**
 * @brief BLE Heart Rate and Running speed Relay application main file.
 *
 * @detail This application demonstrates a simple "Relay".
 * Meaning we pass on the values that we receive. By combining a collector part on
 * one end and a sensor part on the other, we show that the s130 can function
 * simultaneously as a central and a peripheral device.
 *
 * In the figure below, the sensor ble_app_hrs connects and interacts with the relay
 * in the same manner it would connect to a heart rate collector. In this case, the Relay
 * application acts as a central.
 *
 * On the other side, a collector (such as Master Control panel or ble_app_hrs_c) connects
 * and interacts with the relay the same manner it would connect to a heart rate sensor peripheral.
 *
 * Led layout:
 * LED 1: Central side is scanning       LED 2: Central side is connected to a peripheral
 * LED 3: Peripheral side is advertising LED 4: Peripheral side is connected to a central
 *
 * @note While testing, be careful that the Sensor and Collector are actually connecting to the Relay,
 *       and not directly to each other!
 *
 *    Peripheral                  Relay                    Central
 *    +--------+        +-----------|----------+        +-----------+
 *    | Heart  |        | Heart     |   Heart  |        |           |
 *    | Rate   | -----> | Rate     -|-> Rate   | -----> | Collector |
 *    | Sensor |        | Collector |   Sensor |        |           |
 *    +--------+        +-----------|   and    |        +-----------+
 *                      | Running   |   Running|
 *    +--------+        | Speed    -|-> Speed  |
 *    | Running|------> | Collector |   Sensor |
 *    | Speed  |        +-----------|----------+
 *    | Sensor |
 *    +--------+
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "nordic_common.h"
#include "softdevice_handler.h"
#include "peer_manager.h"
#include "app_timer.h"
#include "app_trace.h"
#include "boards.h"
#include "bsp.h"
#include "bsp_btn_ble.h"
#include "ble.h"
#include "app_uart.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_db_discovery.h"
#include "ble_rscs.h"
#include "ble_rscs_c.h"
#include "ble_conn_state.h"
#include "nrf_log.h"
#include "fstorage.h"

#include "fds.h"

#define NUM_TEMP_DEVS               3                                  /**<max number of wireless thermometers to be connected to the the application. When changing this number remember to adjust the RAM settings*/

#define CENTRAL_LINK_COUNT          NUM_TEMP_DEVS                      /**<number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT       1                                  /**<number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

#define APPL_LOG                    app_trace_log                      /**< Macro used to log debug information over UART. */
#define UART_TX_BUF_SIZE            256                                /**< Size of the UART TX buffer, in bytes. Must be a power of two. */
#define UART_RX_BUF_SIZE            1                                  /**< Size of the UART RX buffer, in bytes. Must be a power of two. */

/* Central related. */

#define CENTRAL_SCANNING_LED        BSP_LED_0_MASK
#define CENTRAL_CONNECTED_LED       BSP_LED_1_MASK

#define APP_TIMER_PRESCALER         0                                  /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS        (2+BSP_APP_TIMERS_NUMBER)          /**< Maximum number of timers used by the application. */
#define APP_TIMER_OP_QUEUE_SIZE     2                                  /**< Size of timer operation queues. */

#define UART_TX_INTERVAL            APP_TIMER_TICKS(2000, APP_TIMER_PRESCALER)  /**< UART tx interval (ticks). */

APP_TIMER_DEF(m_uart_tx_timer_id);

#define SEC_PARAM_BOND              1                                  /**< Perform bonding. */
#define SEC_PARAM_MITM              0                                  /**< Man In The Middle protection not required. */
#define SEC_PARAM_IO_CAPABILITIES   BLE_GAP_IO_CAPS_NONE               /**< No I/O capabilities. */
#define SEC_PARAM_OOB               0                                  /**< Out Of Band data not available. */
#define SEC_PARAM_MIN_KEY_SIZE      7                                  /**< Minimum encryption key size in octets. */
#define SEC_PARAM_MAX_KEY_SIZE      16                                 /**< Maximum encryption key size in octets. */

#define SCAN_INTERVAL               0x00A0                             /**< Determines scan interval in units of 0.625 millisecond. */
#define SCAN_WINDOW                 0x0050                             /**< Determines scan window in units of 0.625 millisecond. */

#define MIN_CONNECTION_INTERVAL     MSEC_TO_UNITS(7.5, UNIT_1_25_MS)   /**< Determines minimum connection interval in milliseconds. */
#define MAX_CONNECTION_INTERVAL     MSEC_TO_UNITS(30, UNIT_1_25_MS)    /**< Determines maximum connection interval in milliseconds. */
#define SLAVE_LATENCY               0                                  /**< Determines slave latency in terms of connection events. */
#define SUPERVISION_TIMEOUT         MSEC_TO_UNITS(4000, UNIT_10_MS)    /**< Determines supervision time-out in units of 10 milliseconds. */

#define UUID16_SIZE                 2                                  /**< Size of a UUID, in bytes. */

/**@brief Macro to unpack 16bit unsigned UUID from an octet stream. */
#define UUID16_EXTRACT(DST, SRC) \
    do                           \
    {                            \
        (*(DST))   = (SRC)[1];   \
        (*(DST)) <<= 8;          \
        (*(DST))  |= (SRC)[0];   \
    } while (0)

/**@brief Variable length data encapsulation in terms of length and pointer to data. */
typedef struct
{
    uint8_t     * p_data;    /**< Pointer to data. */
    uint16_t      data_len;  /**< Length of data. */
} data_t;

/** @brief Scan parameters requested for scanning and connection. */
static const ble_gap_scan_params_t m_scan_param =
{
    0,              // Active scanning not set.
    0,              // Selective scanning not set.
    NULL,           // No whitelist provided.
    SCAN_INTERVAL,
    SCAN_WINDOW,
    0x0000          // No timeout.
};

/**@brief Connection parameters requested for connection. */
static const ble_gap_conn_params_t m_connection_param =
{
    (uint16_t)MIN_CONNECTION_INTERVAL,
    (uint16_t)MAX_CONNECTION_INTERVAL,
    0,
    (uint16_t)SUPERVISION_TIMEOUT
};

static ble_rscs_c_t              m_ble_rsc_c;                                       /**< Main structure used by the Running speed and cadence client module. */

static uint16_t                  m_conn_handle_c[NUM_TEMP_DEVS];                    /**< Connection handle for the central application */
static bool                      dev_connected[NUM_TEMP_DEVS];
static ble_db_discovery_t        m_ble_db_discovery_rsc;                            /**< RSC service DB structure used by the database discovery module. */

/* Peripheral related. */

#define PERIPHERAL_ADVERTISING_LED       BSP_LED_2_MASK
#define PERIPHERAL_CONNECTED_LED         BSP_LED_3_MASK

#define DEVICE_NAME                      "Relayer"                                    /**< Name of device used for advertising. */
#define MANUFACTURER_NAME                "Callum"                      /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                 300                                        /**< The advertising interval (in units of 0.625 ms). This value corresponds to 25 ms. */
#define APP_ADV_TIMEOUT_IN_SECONDS       180                                        /**< The advertising timeout in units of seconds. */

#define FIRST_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)/**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT     3                                          /**< Number of attempts before giving up the connection parameter negotiation. */

static ble_rscs_t   m_rscs;                                                         /**< Main structure for the Running speed and cadence server module. */

/**@brief UUIDs which the central applications will scan for, and which will be advertised by the peripherals. */
static ble_uuid_t m_adv_uuids[] = {{BLE_UUID_HEART_RATE_SERVICE,         BLE_UUID_TYPE_BLE},
                                   {BLE_UUID_RUNNING_SPEED_AND_CADENCE,  BLE_UUID_TYPE_BLE}};

#define UART_RECIEVE_IDLE  0
#define UART_RECIEVED_STX  1
#define UART_RECIEVED_0    2
#define UART_RECIEVED_ID   3
#define UART_TX_IDLE       0
#define UART_TX_BUSY       1
#define UART_STX 2 //packet_start
#define UART_ETX 3 //packet_end
static uint8_t uart_rx_status   = UART_RECIEVE_IDLE;

/* Tempartature logging, readying for UART */
/**@brief Structure containing the Temparature measurement received from the peer
 *        This originates from a recieved RSC service packet. 
 */
typedef struct
{
    bool     posative;  //(no longer used) whether the temparatuer is positive or negative
    uint16_t temp;      // the temparature detected + 70'C (to avoid signed numbers)
    uint8_t  id;        // the device ID
    uint16_t conn_hand; // the device BLE connection handle
} temp_t;

#define TEMP_ID     inst_cadence
#define TEMP_TEMP   inst_speed
#define TEMP_POS    is_running

temp_t latest_temp[10];
static uint8_t indicate = 0xff;

// Pin used to trigger debug events
#define DEBUG_PIN         8
#define DEBUG_PIN_MASK    (1 << DEBUG_PIN)
void debug_pin_init(void)
{
    NRF_GPIO->PIN_CNF[DEBUG_PIN] = ((GPIO_PIN_CNF_DIR_Input      << GPIO_PIN_CNF_DIR_Pos) | \
                                    (GPIO_PIN_CNF_INPUT_Connect  << GPIO_PIN_CNF_INPUT_Pos) | \
                                    (GPIO_PIN_CNF_PULL_Pullup    << GPIO_PIN_CNF_PULL_Pos) | \
                                    (GPIO_PIN_CNF_SENSE_Disabled << GPIO_PIN_CNF_SENSE_Pos));
    NRF_GPIO->DIRCLR = DEBUG_PIN_MASK;
}

// Functions for controlling the LED on the PCB
#define INDICATE_LED_PIN  4
#define INDICATE_LED_MASK (1 << INDICATE_LED_PIN)
void indicate_led_init(void)
{
    NRF_GPIO->PIN_CNF[INDICATE_LED_PIN] = ((GPIO_PIN_CNF_DIR_Output << GPIO_PIN_CNF_DIR_Pos) | \
                                           (GPIO_PIN_CNF_DRIVE_H0H1 << GPIO_PIN_CNF_DRIVE_Pos));
    NRF_GPIO->DIRSET = INDICATE_LED_MASK;
    NRF_GPIO->OUTCLR = INDICATE_LED_MASK;
}
void indicate_led_on(void)
{
    NRF_GPIO->OUTSET = INDICATE_LED_MASK;
}
void indicate_led_off(void)
{
    NRF_GPIO->OUTCLR = INDICATE_LED_MASK;
}

// Function to request that thermometer "dev" turns on its led
void indicate_dev(uint8_t dev)
{
    m_ble_rsc_c.conn_handle = latest_temp[dev].conn_hand;
    ble_rscs_c_rsc_notif_enable(&m_ble_rsc_c);
}

/**@brief Function to handle asserts in the SoftDevice.
 *
 * @details This function will be called in case of an assert in the SoftDevice.
 *
 * @warning This handler is an example only and does not fit a final product. You need to analyze
 *          how your product is supposed to react in case of Assert.
 * @warning On assert from the SoftDevice, the system can only recover on reset.
 *
 * @param[in] line_num     Line number of the failing ASSERT call.
 * @param[in] p_file_name  File name of the failing ASSERT call.
 */
void assert_nrf_callback(uint16_t line_num, const uint8_t * p_file_name)
{
    app_error_handler(0xDEADBEEF, line_num, p_file_name);
}

/**@brief Function for handling errors from the Connection Parameters module.
 *
 * @param[in] nrf_error  Error code containing information about what went wrong.
 */
static void conn_params_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}


/**
 * @brief Parses advertisement data, providing length and location of the field in case
 *        matching data is found.
 *
 * @param[in]  Type of data to be looked for in advertisement data.
 * @param[in]  Advertisement report length and pointer to report.
 * @param[out] If data type requested is found in the data report, type data length and
 *             pointer to data will be populated here.
 *
 * @retval NRF_SUCCESS if the data type is found in the report.
 * @retval NRF_ERROR_NOT_FOUND if the data type could not be found.
 */
static uint32_t adv_report_parse(uint8_t type, data_t * p_advdata, data_t * p_typedata)
{
    uint32_t  index = 0;
    uint8_t * p_data;

    p_data = p_advdata->p_data;

    while (index < p_advdata->data_len)
    {
        uint8_t field_length = p_data[index];
        uint8_t field_type   = p_data[index+1];

        if (field_type == type)
        {
            p_typedata->p_data   = &p_data[index+2];
            p_typedata->data_len = field_length-1;
            return NRF_SUCCESS;
        }
        index += field_length + 1;
    }
    return NRF_ERROR_NOT_FOUND;
}


/**@brief Function to start scanning.
 */
static void scan_start(void)
{
    ret_code_t err_code;

    sd_ble_gap_scan_stop();

    err_code = sd_ble_gap_scan_start(&m_scan_param);
    // It is okay to ignore this error since we are stopping the scan anyway.
    if (err_code != NRF_ERROR_INVALID_STATE)
    {
        APP_ERROR_CHECK(err_code);
    }
}


/**@brief Function for handling File Data Storage events.
 *
 * @param[in] p_evt  Peer Manager event.
 * @param[in] cmd
 */
static void fds_evt_handler(fds_evt_t const * const p_evt)
{
    
}


/**@brief Function for handling Peer Manager events.
 *
 * @param[in] p_evt  Peer Manager event.
 */
static void pm_evt_handler(pm_evt_t const * p_evt)
{
    ret_code_t err_code;

    switch(p_evt->evt_id)
    {
        case PM_EVT_BONDED_PEER_CONNECTED:
            break;

        case PM_EVT_LINK_SECURED:

        break;

        case PM_EVT_LINK_SECURE_FAILED:
            /** In some cases, when securing fails, it can be restarted directly. Sometimes it can
             *  be restarted, but only after changing some Security Parameters. Sometimes, it cannot
             *  be restarted until the link is disconnected and reconnected. Sometimes it is
             *  impossible, to secure the link, or the peer device does not support it. How to
             *  handle this error is highly application dependent. */
            switch (p_evt->params.link_secure_failed_evt.error)
            {
                case PM_SEC_ERROR_PIN_OR_KEY_MISSING:
                    // Rebond if one party has lost its keys.
                    err_code = pm_link_secure(p_evt->conn_handle, true);
                    if (err_code != NRF_ERROR_INVALID_STATE)
                    {
                        APP_ERROR_CHECK(err_code);
                    }
                    break;

                default:
                    break;
            }
            break;

        case PM_EVT_STORAGE_FULL:
            // Run garbage collection on the flash.
            err_code = fds_gc();
            if (err_code != NRF_ERROR_BUSY)
            {
                APP_ERROR_CHECK(err_code);
            }
            break;

        case PM_EVT_ERROR_UNEXPECTED:
            // A likely fatal error occurred. Assert.
            APP_ERROR_CHECK(p_evt->params.error_unexpected_evt.error);
            break;

        case PM_EVT_PEER_DATA_UPDATED:
            break;

        case PM_EVT_PEER_DATA_UPDATE_FAILED:
            APP_ERROR_CHECK_BOOL(false);
            break;

        case PM_EVT_ERROR_LOCAL_DB_CACHE_APPLY:
            // The local database has likely changed, send service changed indications.
            pm_local_database_has_changed();
            break;

        case PM_EVT_LOCAL_DB_CACHE_APPLIED:
            break;

        case PM_EVT_SERVICE_CHANGED_INDICATION_SENT:
            break;
    }
}


/**@brief Handles events coming from  Running Speed and Cadence central module.
 */
static void rscs_c_evt_handler(ble_rscs_c_t * p_rscs_c, ble_rscs_c_evt_t * p_rscs_c_evt)
{
    switch (p_rscs_c_evt->evt_type)
    {
        case BLE_RSCS_C_EVT_DISCOVERY_COMPLETE:
        {
            ret_code_t err_code;

            // Initiate bonding.
            err_code = pm_link_secure(p_rscs_c->conn_handle, false);
            if (err_code != NRF_ERROR_INVALID_STATE)
            {
                APP_ERROR_CHECK(err_code);
            }

            // Running speed cadence service discovered. Enable notifications.
            err_code = ble_rscs_c_rsc_notif_enable(p_rscs_c);
            APP_ERROR_CHECK(err_code);
        } break; // BLE_RSCS_C_EVT_DISCOVERY_COMPLETE:

        case BLE_RSCS_C_EVT_RSC_NOTIFICATION:
        {
            uint32_t        err_code;
            ble_rscs_meas_t rscs_measurment;
            
            // Store new data
            latest_temp[p_rscs_c_evt->params.rsc.TEMP_ID].id        = p_rscs_c_evt->params.rsc.TEMP_ID;
            latest_temp[p_rscs_c_evt->params.rsc.TEMP_ID].posative  = p_rscs_c_evt->params.rsc.TEMP_POS;
            latest_temp[p_rscs_c_evt->params.rsc.TEMP_ID].temp      = p_rscs_c_evt->params.rsc.TEMP_TEMP;
            latest_temp[p_rscs_c_evt->params.rsc.TEMP_ID].conn_hand = p_rscs_c->conn_handle;
            
            // forward latest data from device 0, 1 & 2
            rscs_measurment.is_running                  = true;
            rscs_measurment.is_inst_stride_len_present  = false;
            rscs_measurment.is_total_distance_present   = false;

            rscs_measurment.inst_stride_length = 0;
            rscs_measurment.inst_cadence       = latest_temp[1].temp;
            rscs_measurment.inst_speed         = latest_temp[0].temp;
            rscs_measurment.total_distance     = latest_temp[2].temp;

            err_code = ble_rscs_measurement_send(&m_rscs, &rscs_measurment);
            
            if ((err_code != NRF_SUCCESS) &&
                (err_code != NRF_ERROR_INVALID_STATE) &&
                (err_code != BLE_ERROR_NO_TX_PACKETS) &&
                (err_code != BLE_ERROR_GATTS_SYS_ATTR_MISSING)
                )
            {
                APP_ERROR_HANDLER(err_code);
            }
        } break; // BLE_RSCS_C_EVT_RSC_NOTIFICATION

        default:
            // No implementation needed.
            break;
    }
}


static void uart_data_rx_handler(uint8_t rx_ch)
{
    char    id_ch           = 0;
    uint8_t id_num          = 99;

    
    /*if ((rx_ch == UART_STX) && (uart_rx_status == UART_RECIEVE_IDLE))
    {
        uart_rx_status = UART_RECIEVED_STX;
    }*/
    if ((rx_ch == '0') && (uart_rx_status == UART_RECIEVE_IDLE))
    {
        uart_rx_status = UART_RECIEVED_0;
    }
    else if (uart_rx_status == UART_RECIEVED_0)
    {
        //indicate thermometer stated
        id_ch = rx_ch;
        id_num = id_ch - 0x30;
        indicate = id_num;
        uart_rx_status = UART_RECIEVE_IDLE;
    }
    /*else if ((rx_ch == UART_ETX) && (uart_rx_status == UART_RECIEVED_ID))
    {
        uart_rx_status = UART_RECIEVE_IDLE;
    }*/
    /*else
    {
        // there has been an issue in  UART, reset to idle
        uart_rx_status = UART_RECIEVE_IDLE;
    }*/
}

static void uart_put_coded_val(uint8_t val)
{
    if (val < 10)
    {
        app_uart_put(0x30 + val); //send the ascii value of the number (0-9)
    }
    else
    {
        app_uart_put(0x41 + val - 10); //send the ascii value of the number (A-F)
    }
}

static void tx_temp_byte(uint8_t byte)
{
    if (latest_temp[byte/2].id == (byte/2))
    {
        if (byte % 2)
        {
            uint8_t lower = latest_temp[byte/2].temp & 0x0F;
            uart_put_coded_val(lower);
        }
        else
        {
            uint8_t upper = (latest_temp[byte/2].temp & 0xF0) >> 4;
            uart_put_coded_val(upper);
        }
    }
    else
    {
        // sending FF indicates no connection
        app_uart_put(0x46);
    }
}

// handler for when the UART Tx Buffer is empty
// transmits the rest of the message, then stops
static void uart_tx_empty_handler(void)
{
    static uint8_t i = 0;
            
    if (i < 20)
    {
        tx_temp_byte(i);
        i++;
    }
    else if (i == 20)
    {
        app_uart_put(UART_ETX);
        i++;
    }
    else
    {
        i=0;
    }
}

// handler for UART events
static void uart_evt_handler(app_uart_evt_t * p_app_uart_event)
{
    switch (p_app_uart_event->evt_type)
    {
        case APP_UART_DATA_READY:
        {
            // in this setup data is avaialbe to be used when the case is APP_UART_DATA
        } break;
        case APP_UART_FIFO_ERROR:
        {
            
        } break;
        case APP_UART_COMMUNICATION_ERROR:
        {
            
        } break;
        case APP_UART_TX_EMPTY:
        {
            uart_tx_empty_handler();
        } break;
        case APP_UART_DATA:
        {
            uart_data_rx_handler(p_app_uart_event->data.value);
        } break;
    }
}

static void uart_tx_timeout_handler(void * p_context)
{
    static bool on = true;
    if (on)
    {
        indicate_led_off();
        on = false;
    }
    else
    {
        indicate_led_on();
        on = true;
    }
    app_uart_put(UART_STX);
}

/**@brief Function for handling BLE Stack events concerning central applications.
 *
 * @details This function keeps the connection handles of central applications up-to-date. It
 * parses scanning reports, initiating a connection attempt to peripherals when a target UUID
 * is found, and manages connection parameter update requests. Additionally, it updates the status
 * of LEDs used to report central applications activity.
 *
 * @note        Since this function updates connection handles, @ref BLE_GAP_EVT_DISCONNECTED events
 *              should be dispatched to the target application before invoking this function.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void on_ble_central_evt(const ble_evt_t * const p_ble_evt)
{
    // The addresses of peers we attempted to connect to.
    static ble_gap_addr_t periph_addr_rsc;

    // For readability.
    const ble_gap_evt_t   * const p_gap_evt = &p_ble_evt->evt.gap_evt;

    switch (p_ble_evt->header.evt_id)
    {
        /** Upon connection, check which peripheral has connected (HR or RSC), initiate DB
         *  discovery, update LEDs status and resume scanning if necessary. */
        case BLE_GAP_EVT_CONNECTED:
        {
            uint32_t err_code;

            // For readability.
            const ble_gap_addr_t * const peer_addr = &p_gap_evt->params.connected.peer_addr;

            /** Check which peer has connected, save the connection handle and initiate DB discovery.
             *  DB discovery will invoke a callback (hrs_c_evt_handler and rscs_c_evt_handler)
             *  upon completion, which is used to enable notifications from the peer. */
            if(memcmp(&periph_addr_rsc, peer_addr, sizeof(ble_gap_addr_t)) == 0)
            {
                // Reset the peer address we had saved.
                memset(&periph_addr_rsc, 0, sizeof(ble_gap_addr_t));
                
                //find first unused connection, log that connection
                for (uint8_t i = 0; i < NUM_TEMP_DEVS; i++)
                {
                    if (!dev_connected[i])
                    {
                        m_conn_handle_c[i] = p_gap_evt->conn_handle;
                        dev_connected[i] = true;
                        i = NUM_TEMP_DEVS + 1; //break out of for loop as have got correct value
                    }
                }
                err_code = ble_db_discovery_start(&m_ble_db_discovery_rsc, p_gap_evt->conn_handle);
                APP_ERROR_CHECK(err_code);
            }

            scan_start();
        } break; // BLE_GAP_EVT_CONNECTED

        /** Upon disconnection, reset the connection handle of the peer which disconnected, update
         * the LEDs status and start scanning again. */
        case BLE_GAP_EVT_DISCONNECTED:
        {            
            for (uint8_t i = 0; i < NUM_TEMP_DEVS; i++)
            {
                if(p_gap_evt->conn_handle == m_conn_handle_c[i])
                {
                    m_conn_handle_c[i] = BLE_CONN_HANDLE_INVALID;
                    dev_connected[i] = false;
                    i = NUM_TEMP_DEVS + 1; //break out of for loop
                }
            }
        } break; // BLE_GAP_EVT_DISCONNECTED

        case BLE_GAP_EVT_ADV_REPORT:
        {
            uint32_t err_code;
            data_t   adv_data;
            data_t   type_data;

            // For readibility.
            const ble_gap_addr_t  * const peer_addr = &p_gap_evt->params.adv_report.peer_addr;

            // Initialize advertisement report for parsing.
            adv_data.p_data     = (uint8_t *)p_gap_evt->params.adv_report.data;
            adv_data.data_len   = p_gap_evt->params.adv_report.dlen;

            err_code = adv_report_parse(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_MORE_AVAILABLE,
                                        &adv_data,
                                        &type_data);

            if (err_code != NRF_SUCCESS)
            {
                // Look for the services in 'complete' if it was not found in 'more available'.
                err_code = adv_report_parse(BLE_GAP_AD_TYPE_16BIT_SERVICE_UUID_COMPLETE,
                                            &adv_data,
                                            &type_data);

                if (err_code != NRF_SUCCESS)
                {
                    // If we can't parse the data, then exit.
                    break;
                }
            }

            // Verify if any UUID match the Heart rate or Running speed and cadence services.
            for (uint32_t u_index = 0; u_index < (type_data.data_len / UUID16_SIZE); u_index++)
            {
                bool        do_connect = false;
                uint16_t    extracted_uuid;

                UUID16_EXTRACT(&extracted_uuid, &type_data.p_data[u_index * UUID16_SIZE]);

                /** Limit the number of connections possible */
                for (uint8_t i = 0; i < NUM_TEMP_DEVS; i++)
                {
                    if ((extracted_uuid       == BLE_UUID_RUNNING_SPEED_AND_CADENCE) &&
                             (dev_connected[i] == false))
                    {
                        do_connect = true;
                        memcpy(&periph_addr_rsc, peer_addr, sizeof(ble_gap_addr_t));
                        i = NUM_TEMP_DEVS + 1; //break out of for loop
                    }
                }

                if (do_connect)
                {
                    // Initiate connection.
                    err_code = sd_ble_gap_connect(peer_addr, &m_scan_param, &m_connection_param);
                    if (err_code != NRF_SUCCESS)
                    {

                    }
                }
            }
        } break; // BLE_GAP_ADV_REPORT

        case BLE_GAP_EVT_TIMEOUT:
        {
            // We have not specified a timeout for scanning, so only connection attemps can timeout.
            if (p_gap_evt->params.timeout.src == BLE_GAP_TIMEOUT_SRC_CONN)
            {
                //APPL_LOG("[APPL]: Connection Request timed out.\r\n");
            }
        } break; // BLE_GAP_EVT_TIMEOUT

        case BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST:
        {
            // Accept parameters requested by peer.
            ret_code_t err_code;
            err_code = sd_ble_gap_conn_param_update(p_gap_evt->conn_handle,
                                        &p_gap_evt->params.conn_param_update_request.conn_params);
            APP_ERROR_CHECK(err_code);
        } break; // BLE_GAP_EVT_CONN_PARAM_UPDATE_REQUEST

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for handling BLE Stack events involving peripheral applications. Manages the
 * LEDs used to report the status of the peripheral applications.
 *
 * @param[in] p_ble_evt  Bluetooth stack event.
 */
static void on_ble_peripheral_evt(ble_evt_t * p_ble_evt)
{
    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for handling advertising events.
 *
 * @param[in] ble_adv_evt  Advertising event.
 */
static void on_adv_evt(ble_adv_evt_t ble_adv_evt)
{
    switch (ble_adv_evt)
    {
        case BLE_ADV_EVT_FAST:
            break;

        case BLE_ADV_EVT_IDLE:
        {
            ret_code_t err_code;
            err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
            APP_ERROR_CHECK(err_code);
        } break;

        default:
            // No implementation needed.
            break;
    }
}


/**@brief Function for dispatching a BLE stack event to all modules with a BLE stack event handler.
 *
 * @details This function is called from the scheduler in the main loop after a BLE stack event has
 * been received.
 *
 * @param[in]   p_ble_evt   Bluetooth stack event.
 */
static void ble_evt_dispatch(ble_evt_t * p_ble_evt)
{
    uint16_t conn_handle;
    uint16_t role;

    /** The Connection state module has to be fed BLE events in order to function correctly
     * Remember to call ble_conn_state_on_ble_evt before calling any ble_conns_state_* functions. */
    ble_conn_state_on_ble_evt(p_ble_evt);

    pm_ble_evt_handler(p_ble_evt);

    // The connection handle should really be retrievable for any event type.
    conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
    role        = ble_conn_state_role(conn_handle);

    // Based on the role this device plays in the connection, dispatch to the right applications.
    if (role == BLE_GAP_ROLE_PERIPH)
    {
        // Manages peripheral LEDs.
        on_ble_peripheral_evt(p_ble_evt);

        ble_advertising_on_ble_evt(p_ble_evt);
        ble_conn_params_on_ble_evt(p_ble_evt);

        // Dispatch to peripheral applications.
        ble_rscs_on_ble_evt(&m_rscs, p_ble_evt);
    }
    else if ((role == BLE_GAP_ROLE_CENTRAL) || (p_ble_evt->header.evt_id == BLE_GAP_EVT_ADV_REPORT))
    {
        /** on_ble_central_evt will update the connection handles, so we want to execute it
         * after dispatching to the central applications upon disconnection. */
        if (p_ble_evt->header.evt_id != BLE_GAP_EVT_DISCONNECTED)
        {
            on_ble_central_evt(p_ble_evt);
        }
        
        for (uint8_t i = 0; i < NUM_TEMP_DEVS; i++)
        {
            if (conn_handle == m_conn_handle_c[i])
            {
                ble_rscs_c_on_ble_evt(&m_ble_rsc_c, p_ble_evt);
                ble_db_discovery_on_ble_evt(&m_ble_db_discovery_rsc, p_ble_evt);
                i = NUM_TEMP_DEVS + 1; //break out of for loop
            }
        }

        // If the peer disconnected, we update the connection handles last.
        if (p_ble_evt->header.evt_id == BLE_GAP_EVT_DISCONNECTED)
        {
            on_ble_central_evt(p_ble_evt);
        }
    }
}


/**@brief Function for dispatching a system event to interested modules.
 *
 * @details This function is called from the System event interrupt handler after a system
 *          event has been received.
 *
 * @param[in]   sys_evt   System stack event.
 */
static void sys_evt_dispatch(uint32_t sys_evt)
{
    ble_advertising_on_sys_evt(sys_evt);
    /** Dispatch the system event to the Flash Storage module, where it will be
     *  dispatched to the Flash Data Storage module and from there to the Peer Manager. */
    fs_sys_event_handler(sys_evt);
}


/**@brief RSC collector initialization.
 */
static void rscs_c_init(void)
{
    uint32_t            err_code;
    ble_rscs_c_init_t   rscs_c_init_obj;

    rscs_c_init_obj.evt_handler = rscs_c_evt_handler;

    err_code = ble_rscs_c_init(&m_ble_rsc_c, &rscs_c_init_obj);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the BLE stack.
 *
 * @details Initializes the SoftDevice and the BLE event interrupts.
 */
static void ble_stack_init(void)
{
    ret_code_t err_code;

    // Initialize the SoftDevice handler module.
    SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_XTAL_20_PPM, NULL);
    
    ble_enable_params_t ble_enable_params;
    err_code = softdevice_enable_get_default_config(CENTRAL_LINK_COUNT,
                                                    PERIPHERAL_LINK_COUNT,
                                                    &ble_enable_params);
    APP_ERROR_CHECK(err_code);
    
    //Check the ram settings against the used number of links
    CHECK_RAM_START_ADDR(CENTRAL_LINK_COUNT,PERIPHERAL_LINK_COUNT);
    
    // Enable BLE stack.
    err_code = softdevice_enable(&ble_enable_params);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for BLE events.
    err_code = softdevice_ble_evt_handler_set(ble_evt_dispatch);
    APP_ERROR_CHECK(err_code);

    // Register with the SoftDevice handler module for System events.
    err_code = softdevice_sys_evt_handler_set(sys_evt_dispatch);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the Peer Manager initialization.
 *
 * @param[in] erase_bonds  Indicates whether bonding information should be cleared from
 *                         persistent storage during initialization of the Peer Manager.
 */
static void peer_manager_init(bool erase_bonds)
{
    ble_gap_sec_params_t sec_param;
    ret_code_t err_code;

    err_code = pm_init();
    APP_ERROR_CHECK(err_code);

    if (erase_bonds)
    {
        pm_peer_delete_all();
    }

    memset(&sec_param, 0, sizeof(ble_gap_sec_params_t));

    // Security parameters to be used for all security procedures.
    sec_param.bond              = SEC_PARAM_BOND;
    sec_param.mitm              = SEC_PARAM_MITM;
    sec_param.io_caps           = SEC_PARAM_IO_CAPABILITIES;
    sec_param.oob               = SEC_PARAM_OOB;
    sec_param.min_key_size      = SEC_PARAM_MIN_KEY_SIZE;
    sec_param.max_key_size      = SEC_PARAM_MAX_KEY_SIZE;
    sec_param.kdist_periph.enc  = 1;
    sec_param.kdist_periph.id   = 1;
    sec_param.kdist_central.enc = 1;
    sec_param.kdist_central.id  = 1;

    err_code = pm_sec_params_set(&sec_param);
    APP_ERROR_CHECK(err_code);

    err_code = pm_register(pm_evt_handler);
    APP_ERROR_CHECK(err_code);

    err_code = fds_register(fds_evt_handler);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for the GAP initialization.
 *
 * @details This function sets up all the necessary GAP (Generic Access Profile) parameters of the
 *          device including the device name, appearance, and the preferred connection parameters.
 */
static void gap_params_init(void)
{
    uint32_t                err_code;
    ble_gap_conn_params_t   gap_conn_params;
    ble_gap_conn_sec_mode_t sec_mode;

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

    err_code = sd_ble_gap_device_name_set(&sec_mode,
                                          (const uint8_t *)DEVICE_NAME,
                                          strlen(DEVICE_NAME));
    APP_ERROR_CHECK(err_code);

    memset(&gap_conn_params, 0, sizeof(gap_conn_params));

    gap_conn_params.min_conn_interval = MIN_CONNECTION_INTERVAL;
    gap_conn_params.max_conn_interval = MAX_CONNECTION_INTERVAL;
    gap_conn_params.slave_latency     = SLAVE_LATENCY;
    gap_conn_params.conn_sup_timeout  = SUPERVISION_TIMEOUT;

    err_code = sd_ble_gap_ppcp_set(&gap_conn_params);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Connection Parameters module.
 */
static void conn_params_init(void)
{
    uint32_t               err_code;
    ble_conn_params_init_t cp_init;

    memset(&cp_init, 0, sizeof(cp_init));

    cp_init.p_conn_params                  = NULL;
    cp_init.first_conn_params_update_delay = FIRST_CONN_PARAMS_UPDATE_DELAY;
    cp_init.next_conn_params_update_delay  = NEXT_CONN_PARAMS_UPDATE_DELAY;
    cp_init.max_conn_params_update_count   = MAX_CONN_PARAMS_UPDATE_COUNT;
    cp_init.start_on_notify_cccd_handle    = BLE_CONN_HANDLE_INVALID; // Start upon connection.
    cp_init.disconnect_on_fail             = true;
    cp_init.evt_handler                    = NULL;  // Ignore events.
    cp_init.error_handler                  = conn_params_error_handler;

    err_code = ble_conn_params_init(&cp_init);
    APP_ERROR_CHECK(err_code);
}


/**
 * @brief Database discovery initialization.
 */
static void db_discovery_init(void)
{
    ret_code_t err_code = ble_db_discovery_init();
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing services that will be used by the application.
 *
 * @details Initialize the Heart Rate, Battery and Device Information services.
 */
static void services_init(void)
{
    uint32_t        err_code;
    ble_rscs_init_t rscs_init;


    // Initialize the Running Speed and Cadence Service.
    memset(&rscs_init, 0, sizeof(rscs_init));

    rscs_init.evt_handler = NULL;
    rscs_init.feature     = BLE_RSCS_FEATURE_INSTANT_STRIDE_LEN_BIT |
                            BLE_RSCS_FEATURE_WALKING_OR_RUNNING_STATUS_BIT;

    // Here the sec level for the Running Speed and Cadence Service can be changed/increased.
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&rscs_init.rsc_meas_attr_md.cccd_write_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&rscs_init.rsc_meas_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&rscs_init.rsc_meas_attr_md.write_perm);

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&rscs_init.rsc_feature_attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_NO_ACCESS(&rscs_init.rsc_feature_attr_md.write_perm);

    err_code = ble_rscs_init(&m_rscs, &rscs_init);
    APP_ERROR_CHECK(err_code);
}


/**@brief Function for initializing the Advertising functionality.
 */
static void advertising_init(void)
{
    uint32_t      err_code;
    ble_advdata_t advdata;

    // Build advertising data struct to pass into @ref ble_advertising_init.
    memset(&advdata, 0, sizeof(advdata));

    advdata.name_type               = BLE_ADVDATA_FULL_NAME;
    advdata.include_appearance      = true;
    advdata.flags                   = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;
    advdata.uuids_complete.uuid_cnt = sizeof(m_adv_uuids) / sizeof(m_adv_uuids[0]);
    advdata.uuids_complete.p_uuids  = m_adv_uuids;

    ble_adv_modes_config_t options = {0};
    options.ble_adv_fast_enabled  = BLE_ADV_FAST_ENABLED;
    options.ble_adv_fast_interval = APP_ADV_INTERVAL;
    options.ble_adv_fast_timeout  = APP_ADV_TIMEOUT_IN_SECONDS;

    err_code = ble_advertising_init(&advdata, NULL, &options, on_adv_evt, NULL);
    APP_ERROR_CHECK(err_code);
}

void connections_log_init(void)
{
    for (uint8_t i = 0; i < NUM_TEMP_DEVS; i++)
    {
        m_conn_handle_c[i] = BLE_CONN_HANDLE_INVALID;
        dev_connected[i]   = false;
    }    
}

// used to read an input pin, used in debuggin
bool debug_trigger(void)
{
    if (NRF_GPIO->IN & DEBUG_PIN_MASK)
    {
        return false;
    }
    else
    {
        return true;
    }
}

// initiallise the UART module
void uart_init(void)
{
    uint32_t err_code;
    const app_uart_comm_params_t comm_params =
      {
          10,
          9,
          RTS_PIN_NUMBER,
          RTS_PIN_NUMBER,
          APP_UART_FLOW_CONTROL_DISABLED,
          false,
          UART_BAUDRATE_BAUDRATE_Baud9600
      };

    APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_evt_handler,
                         APP_IRQ_PRIORITY_LOW,
                         err_code);
    APP_ERROR_CHECK(err_code);
}

// populate the temparature data array with nul values
void temp_array_init(void)
{
    for (uint8_t i = 0; i < NUM_TEMP_DEVS; i++)
    {
        latest_temp[i].id = 0xff;
        latest_temp[i].conn_hand = BLE_CONN_HANDLE_INVALID;
    }
}
/** @brief Function to sleep until a BLE event is received by the application.
 */
static void power_manage(void)
{
    ret_code_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
    if (indicate < 10)
    {
        indicate_dev(indicate);
        indicate = 0xff;
    }
}


int main(void)
{
    ret_code_t err_code;
    bool       erase_bonds;

    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, NULL);
    // Create uart timer
    err_code = app_timer_create(&m_uart_tx_timer_id,
                                APP_TIMER_MODE_REPEATED,
                                uart_tx_timeout_handler);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_timer_start(m_uart_tx_timer_id, UART_TX_INTERVAL, NULL);
    APP_ERROR_CHECK(err_code);
    
    connections_log_init();
    indicate_led_init();
    indicate_led_on();
    debug_pin_init();
    temp_array_init();
    uart_init();

    ble_stack_init();

    peer_manager_init(erase_bonds);

    db_discovery_init();
    rscs_c_init();

    gap_params_init();
    conn_params_init();
    services_init();
    advertising_init();

    /** Start scanning for peripherals and initiate connection to devices which
     *  advertise Heart Rate or Running speed and cadence UUIDs. */
    scan_start();


    // Start advertising.
    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);
    app_uart_put(0);
    for (;;)
    {
        // Wait for BLE events.
        power_manage();
    }
}
