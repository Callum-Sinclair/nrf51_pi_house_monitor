# nrf51_pi_house_monitor
Designing a home temperature monitoring system running on a Raspberry Pi using nrf51 chips for 2.4 GHz wireless data transfer

The system is in three basic parts:
- Code for the Pi
- Code for the temparature sensing modules (running on nRF51 chips)
- Code running on an nRF51 chip to communicate with the Pi (via SPI) and each temparature sensing module

This project was undertaken in connection with a final year module in my
Electronics and Electrical Engineering MEng degree at the University of Glasgow.
This project was to be completed by 18/04/16 and is joint work with Tamas Lukacs and myself.

## Overview

The project involves wirelessly monitoring the temperature of up to 10 separate locations, and displaying it on a Raspberry Pi.  These locations can be within or outside of the walls of a typical home in a radius of around 25 meters. It is realised by using Bluetooth Low Energy (BLE) (also known as Bluetooth Smart) devices with external analogue temperature sensing circuits. Up to 10 custom BLE thermometers can be used.  Each of these measures the temperature using thermistors, and sends this data via a 2.4 GHz BLE connection to a BLE central device.  This central device is connected to the Raspberry Pi via a UART connection. The Raspberry Pi is connected to a display and peripherals and is running a python application which visually shows the temperature values received in real time both graphically an in a table.  To be able to identify which sensor module is which, there is a button in the python application to illuminate the LED of a specific sensor for 2 seconds.  This is done by sending a signal from the BLE central device, connected to the Pi, to the specified BLE thermometer.

The project work was split such that Tamas wrote the Raspberry Pi software and created the serial communication's protocol and Callum designed the hardware and wrote the software for the two Bluetooth devices.

## Equipment

### Raspberry Pi

* Raspberry Pi 1, or Pi 2 (rev 2011.12, Raspbian GNU/Linux 8 (jessie), python 2.7.9)
* Python-serial package and its dependencies
* Python-Qt4 package and its dependencies
* PyQtGraph package and its dependencies (numpy, scipy..)

### BLE Central Device
* RedBearLab BLE Nano module, featuring Nordic Semiconductor nRF51822 Bluetooth SoC with ARM Cortex-M0 core
* UART connection to Raspberry Pi (GPIO 14 & 15)
* 1 GPIO connection to Raspberry Pi (GPIO 18)
* Red status LED
* Powered by 5 V output from Rasperry Pi
* Header connects to pins 2, 4, 6, 8, 10 & 12 of Rasperry Pi GPIO header
* SWD programming pins available for firmware update

### BLE Thermemeter
* RedBearLab BLE Nano module, featuring Nordic Semiconductor nRF51822 Bluetooth SoC with ARM Cortex-M0 core
* Thermistor temperature sensing circuit connected to nRF51822's ADC
* Red status LED
* Powered by 2 x AAA batteris
* SWD programming header for easy firmware update

## Github folder structre

The code is contained in 3 directories:
* tempmonitor - rapberry pi code
* nrf_temp_sensor
* nrf_pi_extention
