#!/usr/bin/python
# -*- coding: utf-8 -*-
'''
Created on 29 Mar 2016

@author: Tamas Lukacs

@version: 0.2
'''
########################################################################################################################
#This script is responsible for loading saving configurations from and to the config.ini file as well as creating it   #
########################################################################################################################

#Import dependenices
from ConfigParser import SafeConfigParser

import resources
########################################################################################################################


def LoadConfig(existingProfile = None):
    '''
    Method responsible for Loading default settings from the config.ini into the
    software`s resources' variables. It also creates the config.ini file if it does not exists or
    it does not have the RuntimeOptions section in it.
    '''

    #Set up flag
    SuccessfulWrite = False
    #Initiate SafeConfigParser
    config = SafeConfigParser()
    #Try opening or creating config.ini
    try:
        config.read('config.ini')
    except Exception as e:
        print "Could not initiate config.ini!"
        return -1
    #If config.ini exists with valid Default Profile section
    if config.has_section("RuntimeOptions"):
        #GEt general Runtime settings from config.ini
        resources.windowSize=config.get('RuntimeOptions','WindowSize')
        devices=(config.get('RuntimeOptions','deviceNames'))
        resources.deviceNames = devices.split(',')
        return 1

        #If config.ini does not exists then create one with the DefaultProfile and default settings
    else:
        #Initiate Config File with Runtime Options
        config.add_section('RuntimeOptions')
        config.set('RuntimeOptions', 'WindowSize', "640x480")
        devices = ",".join(resources.deviceNames)
        config.set('RuntimeOptions', 'deviceNames', devices)

        #Write default Profile settings to config.ini
        try:
            with open('config.ini', 'w') as f:
                config.write(f)
                #Set falg
                SuccessfulWrite = True
        except Exception as e:
            print e
        #If config.ini created successfully
        if SuccessfulWrite == True:
            #Rerun the method to load it in the GUI
            LoadConfig()
        else:
            print "Could not create config.ini, exiting"
            return -1

########################################################################################################################
def SaveProfile(existingProfile= None):
    '''
    Method responsible for saving current settings into the config.ini
    '''
    #Set up flag
    SuccessfulWrite = False
    #Initiate SafeConfigParser
    config = SafeConfigParser()
    #read config.ini
    try:
        config.read('config.ini')
    except Exception as e:
        print "Could not initiate config.ini!"
    #If profile exsists then update its settings in config.ini from up to data RGENConstants
    if config.has_section("RuntimeOptions"):
        config.set("RuntimeOptions", 'WindowSize', \
                    str(resources.windowSize))

        devices = ",".join(resources.deviceNames)
        config.set('RuntimeOptions', 'deviceNames', devices)

        #write to config.ini
        try:
            with open('config.ini', 'w') as f:
                config.write(f)
                SuccessfulWrite = True
                return 1
        except Exception as e:
            print e
            return -1
    #If Profile was not found in cofig.ini
    else:
        print "Problem with existing Profile, no ini information found!"
        return -2
########################################################################################################################