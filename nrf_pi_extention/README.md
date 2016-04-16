# nRF51 Central Communication Node

This section of the project contains the code for the nRF51 wireless communication module
that interfaces with the Pi.

The module consists of an nRF51 chip connected via UART to the Pi. The module communicates
with each temparature sensor, providing this data to the Pi.

This folder contains two projects, a UART demo, wich does not implement any BLE communication,
and the full project, found in the BLE_central folder

These projects were built using Kiel Microvision, and it is reccomended to open them in this,
though the main.c files can be opened sucessfully in any text editor.

To open the Kiel Microvison project, open
```
.../BLE_central/ble_central_and_peripheral/experimental/ble_app_hrs_rscs_relay/pac10028/s130/arm5/ble_app_hrs_rscs_relay_pca10028.uvprojx
```

To open the main file (where all my work is), open
```
.../BLE_central/ble_central_and_peripheral/experimental/ble_app_hrs_rscs_relay/main.c
```

## Code Description

The code for the BLE Central Device is written in C and uses Nordic Semiconductor's Soft Device (Bluetooth Stack) and libraries.  No operating system is used but the code is highly event driven, with almost everything other than the device initialisation happening in interrupts and calls from the Soft Device.  The code for the central device is significantly more complex than the thermometer.

The central device implements two way UART communication with the Pi and two way BLE communication and data logging with up to 7 peripheral devices.  This is due to a limitation in the BLE stack that was used in this project. It may be possible to use 10 thermometers with the same BLE stack, by disconnecting from one and reconnecting to another frequently, however this was not investigated, and using an alternative stack would be a better solution.

In addition to receiving BLE data, and sending it to the Pi via UART, the device transmits some of the received data via BLE, which can be read by a phone.  This was used primarily before the Pi software, and PCB were available, but was also useful for debugging once the Pi software and PCB were available.

The primary tasks fulfilled are:

* All requirements to act as a BLE central device for up to 7 peripherals, and as a peripheral device for one external central device
* UART communication to and from the Raspberry Pi
These tasks can be broken down to the following functions, which have been customised or created for this application, along with existing code to full the remaining BLE requirements.
* The device constantly scans for peripheral devices which it can connect to, and upon discovering one pairs to it, storing the details of the device.  If the device disconnects, it is removed from the list of devices.
* When a paired device sends data, the program reads this data, and stores it in array featuring the latest measurements of all thermometers, along with the device ID and device address.
* Every second a timer interrupt triggers a new UART transmission of all the latest temperature measurements to the Pi.  The initial interrupt transmits a single character, and subsequent interrupts are triggered for each subsequent charter once the previous one has transmitted.
* When incoming UART data is detected it is decoded, and if it matches the ID of a device, the ID is saved to a variable, ready to transmit once all other tasks are complete.  Once the device has completed its other tasks, it requests the device with a corresponding ID to turn on its LED.
* Each time new data is received from any peripheral, the device transmits some of this data via BLE to be read by a phone or similar.  In a final product this should be modified to either be removed, or contain all the data, however the current implementation provides very useful debug information.

## Hardware Description

The main function of the BLE Central Device circuit is to interface the BLE device to the Raspberry Pi, and to communicate via BLE to all the wireless thermometers.  As the RedBearLab module contains all that is required for BLE communication, other than a power supply, the only requirement of the PCB was to connect the relevant signals to the Raspberry Pi.  In addition a red LED controlled by the chip's output was added to the board to be used to indicate the boards status, and assist debugging the device's code.  This board was powered from the Raspberry Pi.  As the Raspberry Pi header places the UART pins adjacent to the 5V supply, this was convenient to be used.  To program the device, the two SWD pin need to be connected to a debugger, no header for this was implemented to minimise the size, as it was realised that the mounting pins for the board are long enough to connect the debugger to.

The device connects to 6 pins on the Pi's header:

* Pin 2   - 5V
* Pin 4   - 5V
* Pin 6   - GND
* Pin 8   - GPIO14/UART_TXD
* Pin 10 - GPIO15/UART_RXD
* Pin 12 - GPIO18
In the current implementation Pin 12 is not used, but was included for future use, eg as a Ready indicator.  By connecting to only 6 pins of the Pi's header the device allows other modules to be added to other pins.
