#!/usr/bin/python
# -*- coding: utf-8 -*-
'''
Created on 13 Mar 2016

@author: Tamas Lukacs

@version: 0.2
'''
########################################################################################################################
#This script contains the background thread which establishes uart connection or feeding main with fake data           #
########################################################################################################################

#Import dependenices
import random
import time
from PyQt4 import QtCore

import resources

#import RPI exclusive package if run on RPI (set in resources.py)
if (resources.onPi):
    import serial
########################################################################################################################

#Class definintion of background thread
class bgThread(QtCore.QThread):
    '''
    Main's subthread which is used to deal with data collection from UART connection or providing fake data for testing
    '''

    #Class variables

    #for serial connection's handle
    ser_handle = None

    def __init__(self,parent = None):
        '''
        Init
        '''
        super(bgThread,self).__init__(parent)

    def run(self):
        '''
        Method run when the thread is started with the .start() command in the main's thread
        '''

        #init uart connection if run on RPI
        if (resources.onPi):
            ser_handle = initSerial(9600)


        while (1):

            #get UART data if on RPI, otherwise get fake random data
            if (resources.onPi):
                data = self.getRealTempData(ser_handle)
            else:
                data = self.getFakeTempData()

            #report data in terminal if feedback is desired
            if (resources.feedback):
                for value in range(0,len(data)):
                    print ("Sensor: " + str(value) + ", Value: " + str(data[value]))

            #save to ring buffer
            resources.buffer[resources.end_pointer]=data
            #increment end pointer
            resources.end_pointer = resources.end_pointer + 1

            #if end pointer reach the end of the ring buffer then point to the beginning
            if resources.end_pointer == resources.buffer_size:
                resources.end_pointer = 0
                #signal overflow
                resources.overflow = True

            #if there is message to send to identify sensor (both request and data is set)
            if (resources.wantToSend == True and resources.sendId != 99):
                #save id locally
                id = resources.sendId
                #reset flag and id
                resources.wantToSend = False
                resources.sendId = 99

                #send the id on UART
                self.sendMessage(id)

    def getFakeTempData(self):
        """
        Method used to emulate incoming data in the form of a list @ every 200 msec
        [SENS0,SENS1, ... SENS9]
        (255-70) identifies as "no value" == N/A == Disconnect
        """
        #emulate passage of time
        time.sleep(0.2)
        #define seed values
        data = [185,50,40,30,25,20,15,0,-30,-40]
        #modify seed values
        modifier = random.randint(-5, 5)
        for i in range(0,len(data)):
            data[i] = data[i] + modifier
        #return list
        return data


    def getRealTempData(self,ser):
        """
        Method used to receive a complete message from UART connection, decode it and return the list of values

        UART MESSAGE STRUCTURE (42 bytes total):
        [STX,value0_upper_char,value0_lower_char,value1_upper_char,value1_lower_char,...,
        value9_upper_char,value9_lower_char,ETX]
        """

        #setup state machine variables
        #flag for finding the beginning of the message marked by chararter 'STX'
        start_found = False
        #flag for finding the end of the message marked by chararter 'ETX'
        end_found = False
        #container for the actual message
        message = []
        #flag to indicate if both the beginning and the end of the packet is received
        incomplete = True

        #stay in state machine until complete message is retrived
        while(incomplete):

            #if the beginning of the message is not yet found
            if (start_found == False):
                #Read 1 character from serial port to data
                data = ser.read(1)

            #if the data is the start of message change message start flag
            if data == resources.packet_start:

                start_found = True

            #if beginning of packet found but the end is not yet found
            if (end_found == False and start_found == True):
                #Read 1 character from serial port to data
                data = ser.read(1)
                #if the data read is the end character
                if data == resources.packet_end:
                    #change flag
                    end_found = True
                #otherwise append to message container to save message
                else:
                    message.append(data)

            #if both the beginning and the end of the message is found
            if (end_found == True and start_found == True):
                #reset flags
                start_found = False
                end_found = False
                #exit from state machine by indicating full message
                incomplete = False

        #decode message
        converted = self.decodeMessage(message)
        #return decoded message
        return converted


    def decodeMessage(self,message):
        """
        Convert characters back into integers
        ENCODING MECHANISM:
        -temperature value represented as 8 bit integer from 0-255 (e.g 00001111)
        -value converted into HEX (leading zeros are kept) (0x0F)
        -hex value converted into character representations and the 0x is stripped ('0','F')
        -the ASCII character codes are sent via UART
        """
        converted = []
        for j in range(0, len(message),2):
            #format(ord(message[j]), "x")
            hex = ( message[j] + message[j+1])
            num = int(hex,16)-70
            converted.append(num)
        print converted
        return converted


    def sendMessage(self,id):
        """
        Method used to send id of device to flash LED for 5 seconds in order to identify device
        Packet description:
        [STX,'0','0-9',ETX]
        """
        #if on RPI
        if (resources.onPi):
            #assemble message
            tx= [resources.packet_start,resources.firstchar,str(id),resources.packet_end]
            #send message 1 byte at a time
            for x in range(0,len(tx)):
                self.ser_handle.write(tx[x])
        else:
            #emulate message sent
            print "Message sent: " + str(id)

########################################################################################################################

def initSerial(baud):
    """
    Method used to initialise UART connection on RPI environment
    :param baud: baud rate for the UART serial connection
    :return: the handle for serial comms
    """
    #setup uart on Pi
    ser = serial.Serial ("/dev/ttyAMA0")    #Open named port
    ser.baudrate = baud                     #Set baud rate (to 9600)
    return ser

########################################################################################################################