# nRF51 Temparature Sensing Module

This section of the project contains the code for the wirless tempartaure sensing modules.

The module's hardware was be designed as part of the project, and consists of an nRF51
chip connected to a temparature sensor. The tempartature sensor will be connected to the
ADC on the nRF51. The module will be powered by two AAA batteries. Several of these
modules will interface with a nRF51 chip connected to the Pi.

This folder contains two projects, an ADC demo, which transmits the ADC value as a battery
measurement type, in a cycle speed and cadence packet,
and the full project, found in the BLE_cperiph_using_rcs folder

These projects were built using Kiel Microvision, and it is reccomended to open them in this,
though the main.c files can be opened sucessfully in any text editor.

To open the Kiel Microvison project, open
```
.../temp_periph_using_rsc/ble_peripheral/ble_app_rscs/pac10028/s130/arm5/ble_app_rscs_s130_pca10028.uvprojx
```

To open the main file (where all my work is), open
```
.../temp_periph_using_rsc/ble_peripheral/main.c
```

## Code Description

The code for the BLE Thermometer is written in C and uses Nordic Semiconductor's Soft Device (Bluetooth Stack) and libraries.  No operating system is used but the code is highly event driven, with everything other than the device initialisation happening in interrupts and calls from the Soft Device.

BLE devices transfer data in set data structures, which form services.  Each use case should have it's own service, such as heart rate monitoring, cycle speed and cadence, or here temperature measurement.  The closest standard service to this projects use case is a health thermometer.  This was initially investigated, however it was not a perfect fit, and the Nordic libraries provided limited support for this packet type.  For a more professional project this service could have been used, or a custom service defined, however it was decided to "hack" another service to ease development and debug time.  The service that was chosen was the running speed and cadence service.  This was chosen as it had good support for both peripheral and central devices in the Nordic libraries, and had suitably formatted data fields.  The transmitted values could also be read by an app provided by Nordic Semiconductor, which eased debugging as the transmission could be checked before trying to implement reception.

The key aspects of the code operate as follows:

* When the device initially turns on it initialises then begins to advertise.
* When a the central device connects it ceases to advertise and pairs with the central device.
* If the central device disconnects from the thermometer the termometer begins advertising again.
* When the device pairs with another, its indicator LED illuminates for two seconds.  Hardware timers programmed to connect to the LED's GPIO pin cause the LED to turn off without any core involvement.
* Whilst the devices are paired a timer interrupt causes the device to read the ADC voltage, convert this to a temperature, and transmit the temperature once every second. A table of data values is used to convert the ADC voltage to a temperature.
* When the central device sends a request, the indicator LED illuminates for 2 seconds
* Each thermometer has a unique ID code between 0 & 9, which is set at the time of programming.  This is used to identify it, and ensures that the same device has the same code even if it is disconnected or turned off

## Hardware Description

The circuitry for the Thermometer needed to supply the RedBearLab board with power, provide a voltage input that varied with temperature, and have a LED controlled by the chip.

The ADC on the chip is a 10-bit Incremental Analog to Digital Converter.  The input resistance of the ADC varies, depending on various settings, from around 130 kΩ upwards.  As stated in the chip's reference manual, this means that there should be negligible error due to the input resistance, if the sensor circuit has output resistance of < 1 kΩ.  The input stage of the thermometer was designed accordingly, with input resistance of < 330 Ω.  The analogue reference voltage used can be configured to a variety of sources: internal 1.2 V band gap voltage, analogue reference provided to an input pin, half the supply voltage, or one third of the supply voltage.  The input circuit was designed to be referenced to the supply voltage, and operate in the temperature range of -50 to 100 °C.  It was designed to have a range of 0.005*Vsupply to 0.327*Vsupply.  Therefore the ADC is configured to use one third of the supply voltage as the voltage reference, with a range of outputs from 0x001 at -50 °C, to 0x066 at 25 °C, to 0x3F2 at 100 °C.

In order to measure the temperature a thermistor was used. The NTCLE100E3103JB0 was used as it has a high resistance in the temperature range of interest, thereby reducing current, and increasing battery life, but able to be used in a low output resistance sensing circuit.  It was also needed to have a measurable change in resistance per degree in the range of interest.  As used in this system it can detect with a resolution of 1 °C or better from -17 to 100 °C, and operates at a slightly poorer resolution down to -50 °C.  To measure the change of resistance, the thermistor is placed in a potential divider with a 330 Ω resistor, and connected to the battery voltage.  The output from this voltage divider ranges from 0.5-32.7% of the supply voltage, and is fed into the nRF51822's ADC.  The input resistance of the ADC is much greater than the output resistance of the voltage divider, so does not bias the reading.

A header for SWD connection was included.  This allows a debugger to be easily and securely attached to the board.  The debugger is needed to flash the code to the device, and was used to debug the code.  The header allows for easy firmware updates, but future designs could be made without it, reducing the size of the PCB.

The sensor modules require batteries which supply at least 1.8 V throughout the lifespan of the batteries where low current drawn was expected.  The most logical choices were therefore coin cells, or 2 x AAA.  A coin cell initially was thought to be the best choice, due to its small size.  Ideally this would be mounted on the opposite side of the PCB to the BLE chip, therefore minimizing the size of the PCB. However, as the BLE chip had through hole mounting, the battery could not be directly opposite the chip, and would double the size of the PCB.  Other disadvantages of coin cells is the fact that they are less readily available than AAA batteries, battery mounts would have to be ordered specifically for the project as they are not kept in the university store, and footprints would need to be designed for them.  It was therefore decided that using 2 x AAA batteries are a better solution. To ensure that the design was kept as small as possible, the PCB was designed to be smaller than the AAA battery holder.  Using AAA batteries should give the circuit a battery life of at least several months.