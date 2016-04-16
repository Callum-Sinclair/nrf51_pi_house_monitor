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
#define UART_RX_BUF_SIZE 1                           /**< UART RX buffer size. */
#define TS_NOT_USED 0xFF

#define CON_ID 0
#define CON_DATA 1

#define STX 2 //packet_start
#define ETX 3 //packet_end

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

#define RECIEVE_IDLE  0
#define RECIEVED_STX  1
#define RECIEVED_0    2
#define RECIEVED_ID   3

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
                         uart_error_handle,
                         APP_IRQ_PRIORITY_LOW,
                         err_code);

    APP_ERROR_CHECK(err_code);

    indicate_led_init();
    indicate_led_on();
    uint8_t rx_ch       = 0;
    uint8_t status      = RECIEVE_IDLE;
    char    id_ch       = 0;
    uint8_t id_num      = 99;
    
    //uint8_t data_send[] = {STX, 0x37, 0x30, 0x37, 0x35, 0x37, 0x38, 0x38, 0x30, 0x39, 0x31, 0x37, 0x30, 0x37, 0x35, 0x37, 0x38, 0x38, 0x30, 0x39, 0x31, ETX};
    uint8_t data_send[] = {STX, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, ETX};
    //uint8_t data_send[] = {STX, 7, 0, 7, 5, 7, 7, 7, 5, 8, 0, 8, 8, 9, 2, 7, 9, 9, 5, 7, 1, ETX};
        

    while (true)
    {
        // wait for indicate (i) or request data (r) command
        while((app_uart_get(&rx_ch) != NRF_SUCCESS));
                indicate_led_off();
/*        if ((rx_ch == STX) && (status == RECIEVE_IDLE))
        {
            status = RECIEVED_STX;
        }
        else if ((rx_ch == '0') && (status == RECIEVED_STX))
        {
            status = RECIEVED_0;
        }
        else if (status == RECIEVED_0)
        {
            //indicate thermometer stated
            id_ch = rx_ch;
            id_num = id_ch - 48;
            //indicate_dev(id_num);
            if (id_num % 2)
            {
                indicate_led_on();
            }
            else
            {
                indicate_led_off();
            }
            status = RECIEVED_ID;
        }
        else if ((rx_ch == ETX) && (status == RECIEVED_ID))
        {
            status = RECIEVE_IDLE;
        }
        else
        {
            // there has been an issue in  UART, reset to idle
            status = RECIEVE_IDLE;
        }
        /*for (uint8_t i = 0; i < 22; i++)
        {
            while(app_uart_put(data_send[i]) != NRF_SUCCESS);
        }
        
        nrf_delay_ms(1000);*/
    }
}


/** @} */
