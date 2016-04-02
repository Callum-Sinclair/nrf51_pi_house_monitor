#!/usr/bin/python
# -*- coding: utf-8 -*-
'''
Created on 12 Mar 2016

@author: Tamas Lukacs

@version: 0.2
'''
########################################################################################################################
#This script contains the global resources                                                                             #
########################################################################################################################

#Import dependenices

########################################################################################################################
#Gui Constants

windowSize = "640x480"

tableHeadings = ["ID","Name","Value","Col"]

########################################################################################################################
#Operation Constants

#Define ring buffer's size
buffer_size = 200
#Initialise empty ring buffer
buffer = [[None,None,None,None,None,None,None,None,None,None]] * buffer_size
#Ring buffer pointers
end_pointer = 0
plot_pointer = 0
#Flag to mark ring buffer's overflow
overflow = False

#Defines the maximum number of values stored and plotted for each sensor data
#The plot resets after this value
mainBuffer = 50

#default colour idetifiers for thre sensors
plotColors = [(255,0,0),#RED
              (255,128,0),#ORANGE
              (255,255,0),#YELLOW
              (128,255,0),#LGREEN
              (0,255,0),#GREEN
              (0,255,128),#TEAL
              (0,255,255),#LBLUE
              (0,128,255),#BLUE
              (0,0,255),#DBLUE
              (127,0,255),]#PURPLE

#default device names
deviceNames = ["default0","default1","default2","default3","default4",
               "default5","default6","default7","default8","default9"]

########################################################################################################################

#switches
feedback = False
onPi = False
busy = False

########################################################################################################################
#Define COMM constants

packet_start = chr(2) #ASCII 'STX'

packet_end = chr(3) #ASCII 'ETX'

request_message = chr(82) #ASCII 'R'

firstchar = chr(48) #ASCII '0'
device0 = chr(48) #ASCII '0'
device1 = chr(49) #ASCII '1'
device2 = chr(50) #ASCII '2'
device3 = chr(51) #ASCII '3'
device4 = chr(52) #ASCII '4'
device5 = chr(53) #ASCII '5'
device6 = chr(54) #ASCII '6'
device7 = chr(55) #ASCII '7'
device8 = chr(56) #ASCII '8'
device9 = chr(57) #ASCII '9'

#define invlaued sensor value (255) but subtract 70 because of the negative celcius degrees mapping (0 == -70 C)
invalid = 185

#flag to show request to identify device == send message
wantToSend = False
#container for the device ID to be sent (99 to make it invalid)
sendId = 99
########################################################################################################################