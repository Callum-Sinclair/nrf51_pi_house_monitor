#!/usr/bin/python
# -*- coding: utf-8 -*-
'''
Created on 12 Mar 2016

@author: Tamas Lukacs

@version: 0.1
'''
########################################################################################################################
#This script contains the core of project                                                                              #
########################################################################################################################

#Import dependenices
import PyQt4
from PyQt4 import uic
from PyQt4.QtCore import SIGNAL, Qt
from PyQt4.QtGui import QAbstractItemView
import pyqtgraph as pg
import numpy as np

import resources
import bg_thread
import table_model

data =[]

########################################################################################################################

#get handles for the PYQT file


widgetForm, baseClass= PyQt4.uic.loadUiType("activity_main.ui")

# DEFINE Ui_MainWindow class
class Ui_MainWindow(baseClass, widgetForm):

    def __init__(self, parent = None):
        '''
        Init
        '''
        super(Ui_MainWindow, self).__init__(parent)
        #Initiates setup methods auto defined in the .ui PYQT file
        self.setupUi(self)
        #Do the rest of the init in separate function
        self.ManualConfig()


    def ManualConfig(self):

        '''
        Method called when the main gui is initiated
        It sets up the GUI to default behaviour, connects user actions with methods etc.
        '''

        #Resize and move position
        dimension = resources.windowSize
        dimension = dimension.split("x")
        if len(dimension) == 2:
            self.resize(int(dimension[0]),int(dimension[1]))

        #Center the window
        self.centerWindow()


        #Define bg thread contained in bg_thread.py
        self.bgThread = bg_thread.bgThread()

        #start the thread
        self.bgThread.start()



        #Connect Signals with Slots
        self.ConnectSignals()

        #Initialise Table
        self.setupTable()

        #Initialise Graph
        self.setupGraph()


    def setupTable(self):
        """
        SETTING UP PYQT MODEL VIEW STRUCTURE
            MODEL -> PROXY -> VIEW
        MODEL: contains data and data manipulation methods
        PROXY: enables and implements sorting/ordering of rows
        """
        self._model = table_model.sensorModel([['','','','']],[], resources.tableHeadings)

        #Create Proxy model
        self._proxyModel = table_model.mySortFilterProxy()

        #Connect Proxy to Model
        self._proxyModel.setSourceModel((self._model))

        #Connect View to Proxy
        self.tableView_sensors.setModel(self._proxyModel)

        #FURTHER SETTINGS FOR 'Personalise Campaign' TABLE
        #Enable sorting
        self.tableView_sensors.setSortingEnabled(True)
        #Set default sorting based upon the Temperature column
        self.tableView_sensors.sortByColumn(1,0)
        #Resize column to fit data - probably no need at the moment
        self.tableView_sensors.resizeColumnsToContents()
        #Can set fixed size for columns if needed
        #self.tableView_sensors.setColumnWidth(7, 250)
        self.tableView_sensors.setColumnWidth(3, 34)


        self.tableView_sensors.setDragEnabled(True)
        self.tableView_sensors.setAcceptDrops(True)
        self.tableView_sensors.setDragDropOverwriteMode(False)
        self.tableView_sensors.setDropIndicatorShown(True)

        self.tableView_sensors.setSelectionMode(QAbstractItemView.SingleSelection)
        self.tableView_sensors.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.tableView_sensors.setDragDropMode(QAbstractItemView.InternalMove)

        rowdata = ["ID1","kitchen","20", "#00FFFF"]
        rowtypes = ['normal','normal','normal','normal']
        self._model.removeRows(0,0,1)
        self._model.insertRows(0, 1,rowdata,rowtypes)



    def setupGraph(self):
        """
        SETTING UP PYQTGRAPH
        """

        #self.graph = pg.PlotWidget(title ="LOLLER")
        #self.graphicsView_graph = pg.plot(title="Three plot curves")
        #self.graphicsView_graph.plot()
        #x = np.arange(1000)
        #y = np.random.normal(size=(3, 1000))
        #for i in range(3):
        #    self.graphicsView_graph.plot(x, y[i], pen=(i,3))

    def centerWindow(self):
        """
        Method used to center the window on screen
        """
        # Get the current screens' dimensions
        screen = PyQt4.QtGui.QDesktopWidget().screenGeometry()
        #get this windows' dimensions
        mysize = self.geometry()
        # The horizontal position is calulated as screenwidth - windowwidth /2
        hpos = ( screen.width() - mysize.width() ) / 2
        # And vertical position the same, but with the height dimensions
        vpos = ( screen.height() - mysize.height() ) / 2
        # And the move call repositions the window
        self.move(hpos, vpos)

    def ConnectSignals(self):
        """
        Method used to realise the SIGNAL-SLOT mechanism of Qt
        """
        self.connect(self.bgThread,SIGNAL("measurementDone(int,int)"), \
                     self.updateDisplay,Qt.QueuedConnection)


    def updateDisplay(self,x,y):
        print ("Message received:" + str(x) + ", "+str(y))
        global data
        data.append(y)

        hexColor = self.getCellData(0)
        rgbColor = self.hex_to_rgb(hexColor)

        self.graphicsView_graph.plot(data, pen=(rgbColor))
        self.graphicsView_graph.setXRange(x-5, x+5)


    def hex_to_rgb(self,value):
        value = value.lstrip('#')
        lv = len(value)
        return tuple(int(value[i:i + lv // 3], 16) for i in range(0, lv, lv // 3))

    def getCellData(self,row):
        index = self.tableView_sensors.model().index(row, 3)
        role = PyQt4.QtCore.Qt.EditRole
        return str(((self.tableView_sensors.model().data(index,role)).toString()).toLower())

########################################################################################################################

#Main - THIS IS WHERE THE .EXE starts

if __name__ == "__main__":
    #Import sys for execution
    import sys
    #Create QApplication with possible input args
    app = PyQt4.QtGui.QApplication(sys.argv)
    #Creat QMainWindow instance
    MainWindow = PyQt4.QtGui.QMainWindow()
    #Creat Ui_MainWindow instance
    ui = Ui_MainWindow()
    #Override Close action to behave normally
    ui.setAttribute(PyQt4.QtCore.Qt.WA_DeleteOnClose)
    #Create PreferenceWindow instance
    #prefui = PreferenceWindow()
    #Show the Main Window
    ui.show()
    #Hadle exit
    sys.exit(app.exec_())

########################################################################################################################