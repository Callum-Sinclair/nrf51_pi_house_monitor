# Raspberry Pi code

This section of the project contains all custom code that will run on the Pi. This section of the project is primarily created and maintained by Tamas.

## Setup

The Rasperry Pi requires settup before the UART can be used. In order to establish a serial connection using python, between the Raspberry Pi and the BLE central device, there are certain steps which need to be followed, to configure the Pi to be able to receive and send information. These are the follows:

* Enable “Serial” at Interfaces in Raspberry Pi Configuration which is found at Menu > Preferences
* Reboot
* By default Raspberry Pi’s UART pins (GPIO 14 and 15) are configured as a serial console. Free up pins by modifying cmdline.txt by the command “sudo nano /boot/cmdline.txt” and removing parameters containing ‘ttyAMA0‘. ie. ‘console=ttyAMA0,115200’ and ‘kgdboc=ttyAMA0,115200’ and saving the file.
* Additionally the file inittab containing serial console data is needed to be modified by “sudo nano /etc/inittab” and commenting out the line “2:23:respawn:/sbin/getty -L ttyAMA0 115200 vt100’” by inserting a “#” symbol to the beginning of the line to complete freeing up the serial pins.
* Install the python packages
* Reboot
Now the Raspberry Pi is prepared for the serial connection.

## Software description

The application run on the Raspberry Pi is coded in python and makes use of the Qt 4 graphical framework using the PyQt4 package. It incorporates the PyQtGraph package to graph the temperature values from each beacon in a "live scrolling" fashion and the table in which the sensors and related information appear makes use of the Model View architectural pattern. The serial connection is managed by the PySerial package which is capable of sending and receiving data in characters.

The GUI has the following features:

1. Title
2. Detect Sensor Button
3. List of Sensors
4. Graph plotting the temperature values
 The application is made up of several modules as follows:

* activity_main.ui - contains PyQt GUI interface
* bg_thread.py - contains the background thread which establishes and deals with serial connection
* config.py - responsible for loading saving configurations of beacon names from and to the config.ini file as well as creating the file
* main.py - mainly deals with handling and updating the GUI
* resources.py - contains global resources for the project like the ring buffer, string variables for the GUI and communication constants
* table_model.py - contains the realisation of Model for the TableView widget
As the serial communication in itself does not provide a mechanism to handle bundled information together, interpreting a message which is made out of more than 1 character requires packetisation. For this purpose the following structure for packet was implemented:

```
[STX,value0_upper_char,value0_lower_char,value1_upper_char,value1_lower_char,...,value9_upper_char,value9_lower_char,ETX]
```
where the ACII character 'STX' marks the beginning of the packet and the ACII character 'ETX' signals the end of the packet. The 1 byte temperature value (0-255) is encoded such that the binary value is turned into a hex one (omitting the 0x part) and the ASCII characters which make up the hex value are sent through the serial line, most significant char first. The negative temperatures are represented by mapping the temperature 0 degrees to the integer value 70 and if the sensor is not connected to the hub or the connection between the hub and the beacon is lost then the value 255 is sent. This way a constant size (22 bytes) packet is sent and received between the hub and the Raspberry Pi. The decoding on the Pi side is realised via a state machine which searches for the beginning of the packet and saves the message part until the end of the packet is found. Then the message is decoded from the string hex value into an integer one and processed. For example, if the sensor reads a temperature of -10 degree celsius, then the value 60 (70 -10) decimal is needed to be transmitted such that:

```
60(decimal) = 0011 1100(binary) = 3C(hex) --> ACII characters '3' and 'C' will be transmitted to the Pi inside the packet
```
Furthermore, due to the asynchronous nature of the serial connection, a ring buffer is implemented into which the background thread saves incoming packets of data and the main thread loads and separates the data into separate data streams for each sensor in order to plot them. Its size is initialized in the resources.py file (buffer_size) to store 200 packets before looping back to its its beginning.

There is also communication from the Pi towards the hub and that is in order to make the beacons identifiable. By selecting a sensor row in the table and clicking on the "Detect Sensor" button, the python app sends a request packet to the hub with the id number of the sensor selected to blink its LED. The packet for this is made up as follows:

```
[STX,'VALUE',ETX]
```
where 'VALUE' is the ACII character from 0-9 depending on which sensor in the table was selected.

The table in which the sensor details are presented is using the Model - View pattern where the data is only modified by the Model (table_model.py) through the requests of the view widget. The model overrides the necessary methods for implementation (flags, data, setData ...) which takes care of the correct functioning of the table. This way the table is capable of showing the following details for 10 separate sensors:

* ID value from 0 till 9
* User defined name which is saved between sessions of the app to an external .ini file
* decimal value of temperature in Celsius degrees which was last received from an on-line sensor ("N/A" if sensor is disconnected or off-line)
* The colour of the sensor's trace on the graph
The graph makes use of the decoded data separated out into individual streams of temperature values and is updated at around 24 Hz in the main.py thanks to a 40 milisecond Qtimer. By default, the graph appends and updates separate curves for each sensor module, but if there is a disconnect value received, then a new curve will be created when the next valid value is received.

The y axis shows integer values of degree Celsius and the x axis relates the value to the data point. As temperature values are received at every second the data points relate to seconds.

As more and more data is represented on the graph, the x axis - showing the data points - is shifted continuously shifted along to show only the last ~25 temperature measurements received, giving a scrolling "live" representation of the temperatures. The maximum length of the data stream represented by the graph is defined to be 50 data points after which the graph clears and restarts at data point 1.

Additional Quirks of the python application:

* manages discontinuity of data (incoming value 255) in plotting by creating new curves for pyqtgraph
* showing latest value received from sensor in the table and also shows past values on the plot
* saves the names the user chooses for the sensors in the name column in the table to an external config.ini file
* has a built in emulation mode of communication for testing purposes ("onPi = False" in resources.py)
