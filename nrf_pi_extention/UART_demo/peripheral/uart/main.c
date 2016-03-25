/* Copyright (c) 2014 Nordic Semiconductor. All Rights Reserved.
 *
 * The information contained herein is property of Nordic Semiconductor ASA.
 * Terms and conditions of usage are described in detail in NORDIC
 * SEMICONDUCTOR STANDARD SOFTWARE LICENSE AGREEMENT.
 *
 * Licensees are granted free, non-transferable use of the information. NO
 * WARRANTY of ANY KIND is provided. This heading must NOT be removed from
 * the file.
 *
 */

/** @file
 * @defgroup uart_example_main main.c
 * @{
 * @ingroup uart_example
 * @brief UART Example Application main file.
 *
 * This file contains the source code for a sample application using UART.
 * 
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "app_uart.h"
#include "app_error.h"
#include "nrf_delay.h"
#include "nrf.h"
#include "bsp.h"

//#define ENABLE_LOOPBACK_TEST  /**< if defined, then this example will be a loopback test, which means that TX should be connected to RX to get data loopback. */

#define MAX_TEST_DATA_BYTES     (15U)                /**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 10                          /**< UART RX buffer size. */
#define TS_NOT_USED 0xFF

#define CON_ID 0
#define CON_DATA 1

uint8_t con_data[10][2] = {{'A', 70}, {'b', 50}, {'C', 27}, {'d', 71}, {'E', 54}, {'f', 82}, {'G', 54}, {'h', 55}, {'I', 49}, {'j', 62}};

void uart_error_handle(app_uart_evt_t * p_event)
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}



#ifdef ENABLE_LOOPBACK_TEST
/** @brief Function for setting the @ref ERROR_PIN high, and then enter an infinite loop.
 */
static void show_error(void)
{
    
    LEDS_ON(LEDS_MASK);
    while(true)
    {
        // Do nothing.
    }
}


/** @brief Function for testing UART loop back. 
 *  @details Transmitts one character at a time to check if the data received from the loopback is same as the transmitted data.
 *  @note  @ref TX_PIN_NUMBER must be connected to @ref RX_PIN_NUMBER)
 */
static void uart_loopback_test()
{
    uint8_t * tx_data = (uint8_t *)("\n\rLOOPBACK_TEST\n\r");
    uint8_t   rx_data;

    // Start sending one byte and see if you get the same
    for (uint32_t i = 0; i < MAX_TEST_DATA_BYTES; i++)
    {
        uint32_t err_code;
        while(app_uart_put(tx_data[i]) != NRF_SUCCESS);

        nrf_delay_ms(10);
        err_code = app_uart_get(&rx_data);

        if ((rx_data != tx_data[i]) || (err_code != NRF_SUCCESS))
        {
            show_error();
        }
    }
    return;
}


#endif


/**
 * @brief Function for main application entry.
 */
int main(void)
{
    LEDS_CONFIGURE(LEDS_MASK);
    LEDS_OFF(LEDS_MASK);
    uint32_t err_code;
    const app_uart_comm_params_t comm_params =
      {
          RX_PIN_NUMBER,
          TX_PIN_NUMBER,
          TS_NOT_USED,
          TS_NOT_USED,
          APP_UART_FLOW_CONTROL_DISABLED,
          false,
          UART_BAUDRATE_BAUDRATE_Baud38400
      };

    APP_UART_FIFO_INIT(&comm_params,
                         UART_RX_BUF_SIZE,
                         UART_TX_BUF_SIZE,
                         uart_error_handle,
                         APP_IRQ_PRIORITY_LOW,
                         err_code);

    APP_ERROR_CHECK(err_code);

#ifndef ENABLE_LOOPBACK_TEST
    uint8_t tx_ch = 0;
    uint8_t rx_ch = 0;
    uint8_t tx_data_num = 0;
    bool id_next = true;

    while (true)
    {
        // wait for indicate (i) or request data (r) command
        while(app_uart_get(&rx_ch) != NRF_SUCCESS);
        while(app_uart_put(tx_ch) != NRF_SUCCESS);

        if (rx_ch == 'i' || rx_ch == 'I')
        {
            // indicate
            tx_ch = 'i';
            tx_data_num = 0;
        }
        else if (rx_ch == 'r' || rx_ch == 'R')
        {
            // send sensor data
            if (id_next)
            {
                tx_ch = con_data[tx_data_num][CON_ID];
                id_next = false;
            }
            else
            {
                tx_ch = con_data[tx_data_num][CON_DATA];
                tx_data_num ++;
                id_next = true;
            }
            if (tx_data_num > 9)
            {
                tx_data_num = 0;
            }
        }
        else
        {
            tx_data_num = 0;
            tx_ch = 0;
        }
    }
#else

    // This part of the example is just for testing the loopback .
    while (true)
    {
        uart_loopback_test();
    }
#endif
}


/** @} */
