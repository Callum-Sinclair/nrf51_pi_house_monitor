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
//#include "app_uart.h"
#include "ble_advdata.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
#include "ble_db_discovery.h"
#include "ble_rscs.h"
#include "ble_rscs_c.h"
#include "ble_conn_state.h"
#include "nrf_log.h"
#include "fstorage.h"

#define RB_LED (1<<19)

#define CENTRAL_LINK_COUNT          2                                  /**<number of central links used by the application. When changing this number remember to adjust the RAM settings*/
#define PERIPHERAL_LINK_COUNT       1                                  /**<number of peripheral links used by the application. When changing this number remember to adjust the RAM settings*/

//#define APPL_LOG                    app_trace_log                      /**< Macro used to log debug information over UART. */
//#define UART_TX_BUF_SIZE            256                                /**< Size of the UART TX buffer, in bytes. Must be a power of two. */
//#define UART_RX_BUF_SIZE            1                                  /**< Size of the UART RX buffer, in bytes. Must be a power of two. */

/* Central related. */

#define CENTRAL_SCANNING_LED        BSP_LED_0_MASK
#define CENTRAL_CONNECTED_LED       BSP_LED_1_MASK

#define APP_TIMER_PRESCALER         0                                  /**< Value of the RTC1 PRESCALER register. */
#define APP_TIMER_MAX_TIMERS        (2+BSP_APP_TIMERS_NUMBER)          /**< Maximum number of timers used by the application. */
#define APP_TIMER_OP_QUEUE_SIZE     2                                  /**< Size of timer operation queues. */

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
static uint16_t                  m_conn_handle_rscs_c = BLE_CONN_HANDLE_INVALID;    /**< Connection handle for the RSC central application */
static ble_db_discovery_t        m_ble_db_discovery_rsc;                            /**< RSC service DB structure used by the database discovery module. */

/* Peripheral related. */

#define PERIPHERAL_ADVERTISING_LED       BSP_LED_2_MASK
#define PERIPHERAL_CONNECTED_LED         BSP_LED_3_MASK

#define DEVICE_NAME                      "MultiTemp"                                /**< Name of device used for advertising. */
#define MANUFACTURER_NAME                "CallumSinclair"                           /**< Manufacturer. Will be passed to Device Information Service. */
#define APP_ADV_INTERVAL                 300                                        /**< The advertising interval (in units of 0.625 ms). This value corresponds to 25 ms. */
#define APP_ADV_TIMEOUT_IN_SECONDS       180                                        /**< The advertising timeout in units of seconds. */

#define FIRST_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(5000, APP_TIMER_PRESCALER) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (5 seconds). */
#define NEXT_CONN_PARAMS_UPDATE_DELAY    APP_TIMER_TICKS(30000, APP_TIMER_PRESCALER)/**< Time between each call to sd_ble_gap_conn_param_update after the first call (30 seconds). */
#define MAX_CONN_PARAMS_UPDATE_COUNT     3                                          /**< Number of attempts before giving up the connection parameter negotiation. */

static ble_rscs_t   m_rscs;                                                         /**< Main structure for the Running speed and cadence server module. */

/**@brief UUIDs which the central applications will scan for, and which will be advertised by the peripherals. */
static ble_uuid_t m_adv_uuids[] = {{BLE_UUID_RUNNING_SPEED_AND_CADENCE,  BLE_UUID_TYPE_BLE}};

/* Tempartature logging, readying for UART */
/**@brief Structure containing the Temparature measurement received from the peer
 *        This originates from a recieved RSC service packet. 
 */
typedef struct
{
    //bool     is_inst_stride_len_present;             /**< True if Instantaneous Stride Length is present in the measurement. */
    //bool     is_total_distance_present;              /**< True if Total Distance is present in the measurement. */
    bool     posative;  //is_running;                             /**< True if running, False if walking. */
    uint16_t temp;      //inst_speed;                             /**< Instantaneous Speed. */
    uint8_t  id;        //inst_cadence;                           /**< Instantaneous Cadence. */
    //uint16_t inst_stride_length;                     /**< Instantaneous Stride Length. */
    //uint32_t total_distance;                         /**< Total Distance. */
} temp_t;
#define TEMP_ID     inst_cadence
#define TEMP_TEMP   inst_speed
#define TEMP_POS    is_running

temp_t latest_temp[10];

void rb_led_on(void)
{
    NRF_GPIO->OUTCLR = RB_LED;
}
void rb_led_off(void)
{
    NRF_GPIO->OUTSET = RB_LED;
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
            //logging
            
            latest_temp[p_rscs_c_evt->params.rsc.TEMP_ID].id        = p_rscs_c_evt->params.rsc.TEMP_ID;
            latest_temp[p_rscs_c_evt->params.rsc.TEMP_ID].posative  = p_rscs_c_evt->params.rsc.TEMP_POS;
            latest_temp[p_rscs_c_evt->params.rsc.TEMP_ID].temp      = p_rscs_c_evt->params.rsc.TEMP_TEMP;
            
            ble_rscs_meas_t rscs_measurment;

            rscs_measurment.is_running                  = true;
            rscs_measurment.is_inst_stride_len_present  = false;
            rscs_measurment.is_total_distance_present   = true;

            rscs_measurment.inst_stride_length = 0;
            rscs_measurment.inst_cadence       = latest_temp[1].temp;
            rscs_measurment.inst_speed         = latest_temp[0].temp;
            rscs_measurment.total_distance     = latest_temp[2].temp;

            uint32_t err_code = ble_rscs_measurement_send(&m_rscs, &rscs_measurment);
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
    static ble_gap_addr_t periph_addr_rsc;
    // For readability.
    const ble_gap_evt_t   * const p_gap_evt = &p_ble_evt->evt.gap_evt;

    switch (p_ble_evt->header.evt_id)
    {
        /** Upon connection, check which peripheral has connected (HR or RSC), initiate DB
         *  discovery, update LEDs status and resume scanning if necessary. */
        case BLE_GAP_EVT_CONNECTED:
        {
            //rb_led_on();
            uint32_t err_code;


                err_code = ble_db_discovery_start(&m_ble_db_discovery_rsc, p_gap_evt->conn_handle);
                APP_ERROR_CHECK(err_code);

        } break; // BLE_GAP_EVT_CONNECTED

        /** Upon disconnection, reset the connection handle of the peer which disconnected, update
         * the LEDs status and start scanning again. */
        case BLE_GAP_EVT_DISCONNECTED:
        {
            //uint8_t n_centrals;

            if(p_gap_evt->conn_handle == m_conn_handle_rscs_c)
            {
                m_conn_handle_rscs_c = BLE_CONN_HANDLE_INVALID;
            }

            // Start scanning
            // scan_start();

            // Update LEDs status.
            //LEDS_ON(CENTRAL_SCANNING_LED);
            //n_centrals = ble_conn_state_n_centrals();
            /*if (n_centrals == 0)
            {
                LEDS_OFF(CENTRAL_CONNECTED_LED);
            }*/
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
                if ((extracted_uuid       == BLE_UUID_RUNNING_SPEED_AND_CADENCE) &&
                         (m_conn_handle_rscs_c == BLE_CONN_HANDLE_INVALID))
                {
                    do_connect = true;
                    memcpy(&periph_addr_rsc, peer_addr, sizeof(ble_gap_addr_t));
                }
                if (do_connect)
                {
                    
                    // Initiate connection.
                    err_code = sd_ble_gap_connect(peer_addr, &m_scan_param, &m_connection_param);
                    
                }
            }
        } break; // BLE_GAP_ADV_REPORT

        case BLE_GAP_EVT_TIMEOUT:
        {
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
            //LEDS_OFF(PERIPHERAL_ADVERTISING_LED);
            //LEDS_ON(PERIPHERAL_CONNECTED_LED);
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            //(PERIPHERAL_CONNECTED_LED);
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
            //LEDS_ON(PERIPHERAL_ADVERTISING_LED);
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

        else if (conn_handle == m_conn_handle_rscs_c)
        {
            ble_rscs_c_on_ble_evt(&m_ble_rsc_c, p_ble_evt);
            ble_db_discovery_on_ble_evt(&m_ble_db_discovery_rsc, p_ble_evt);
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


/**@brief rscs collector initialization.
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
}


/**@brief Function for initializing buttons and leds.
 *
 * @param[out] p_erase_bonds  Will be true if the clear bonding button was pressed to
 *                            wake the application up.
 */
static void buttons_leds_init(bool * p_erase_bonds)
{
    bsp_event_t startup_event;

    ret_code_t err_code = bsp_init(BSP_INIT_LED | BSP_INIT_BUTTONS,
                                 APP_TIMER_TICKS(100, APP_TIMER_PRESCALER),
                                 NULL);
    APP_ERROR_CHECK(err_code);

    err_code = bsp_btn_ble_init(NULL, &startup_event);
    APP_ERROR_CHECK(err_code);

    *p_erase_bonds = (startup_event == BSP_EVENT_CLEAR_BONDING_DATA);
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


/** @brief Function to sleep until a BLE event is received by the application.
 */
static void power_manage(void)
{
    ret_code_t err_code = sd_app_evt_wait();
    APP_ERROR_CHECK(err_code);
}

void temp_array_init(void)
{
    // initialise tempartature array, id = 0xff represents no connection
    // id = i represents correct connection
    for (uint8_t i = 0; i < 10; i++)
    {
        latest_temp[i].id       = 0xff;
        latest_temp[i].posative = false;
        latest_temp[i].temp     = 0;
    }
}

int main(void)
{
    ret_code_t err_code;
    bool       erase_bonds;

    err_code = NRF_LOG_INIT();
    APP_ERROR_CHECK(err_code);

    APP_TIMER_INIT(APP_TIMER_PRESCALER, APP_TIMER_OP_QUEUE_SIZE, NULL);
    buttons_leds_init(&erase_bonds);
    LEDS_CONFIGURE(RB_LED);
    rb_led_off();
    
    temp_array_init();

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

    // Turn on the LED to signal scanning.
    //LEDS_ON(CENTRAL_SCANNING_LED);
    // Start advertising.
    err_code = ble_advertising_start(BLE_ADV_MODE_FAST);
    APP_ERROR_CHECK(err_code);

    for (;;)
    {
        // Wait for BLE events.
        power_manage();
    }
}