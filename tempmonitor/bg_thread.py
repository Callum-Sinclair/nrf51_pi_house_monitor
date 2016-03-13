#!/usr/bin/python
# -*- coding: utf-8 -*-
'''
Created on 13 Mar 2016

@author: Tamas Lukacs

@version: 0.1
'''
########################################################################################################################
#This script contains the background thread                                                                            #
########################################################################################################################

#Import dependenices
import time
from PyQt4 import QtCore
from PyQt4.QtCore import SIGNAL

import resources

########################################################################################################################

#Class definintion
class bgThread(QtCore.QThread):
    '''
    Main's subthread which is used to deal with data collection from I2C connection
    '''

    #Class variable for Matlab handle from matlabcom (indirect handle)
    mlab = None
    #Class variable to store logs about what happens in this thread across methods
    log = ''

    def __init__(self,parent = None):
        '''
        Init
        '''
        super(bgThread,self).__init__(parent)


    def run(self):
        '''
        Method runs when the thread is started with the .start() commnad in the main
        '''
        temp = -25
        repeat = 1
        counter = 1
        countDirection = 1
        while (1):
            #get data
            rawdata = self.getTempData()

            #simulate change#####################################################
            data = self.modifyData(rawdata,repeat, countDirection)


            repeat = repeat + 15

            if (data["bedroom"] > 100 and countDirection == 1):
                countDirection = 0
                repeat = 1
                print "Count Down"

            if (data["bedroom"] < -20 and countDirection == 0):
                countDirection = 1
                repeat = 1
                print "Count up"
            #####################################################################

            #simulate interrupt
            time.sleep(2)

            #report data in terminal
            for key, value in data.items():
                print ("Device: " + str(key) + ", Value: " + str(value))

            print ("Repeat: " + str(repeat) + "\n")


            #COmmunicate to main's thread
            text = "LOL"
            self.emit(SIGNAL("measurementDone(int,int)"),counter, data["bedroom"])
            counter = counter +1

    def getTempData(self):
        """
        Method used to get incoming data
        DUMMY
        """

        data = {'bedroom': 20, 'kitchen': -20}
        return data


    def modifyData(self, rawdata,repeat, countDirection):
        """
        Method used to emulate chaging data
        """
        for key, value in rawdata.items():
            if countDirection == 1:
                rawdata[key] = value+repeat
            if countDirection == 0:
                rawdata[key] = value-repeat
        return rawdata

########################################################################################################################