#!/usr/bin/python
# -*- coding: utf-8 -*-
'''
Created on 12 Mar 2016

@author: Tamas Lukacs

@version: 0.2
'''
########################################################################################################################
#This script contains the core of project                                                                              #
########################################################################################################################

#Import dependenices
import PyQt4
from PyQt4.QtCore import SIGNAL, QTimer, QRect
from PyQt4.QtGui import QAbstractItemView, QDialog
from PyQt4.uic.uiparser import QtGui

import resources
import bg_thread
import config
import table_model

########################################################################################################################

#define global constants

#Inditacors used to enable scrolling of the pyqtgraph
current_axis_x = 0
prev_axis_x = 0

#containers for plotting the sensor values
data1_y = []
data2_y = []
data3_y = []
data4_y = []
data5_y = []
data6_y = []
data7_y = []
data8_y = []
data9_y = []
data10_y = []

#concatanating the sensor containers
datas = [data1_y,data2_y,data3_y,data4_y,data5_y,data6_y,data7_y,data8_y,data9_y,data10_y]

#variables used to achive discontinous curve plotting for pyqtgraph
begin = [0,0,0,0,0,0,0,0,0,0]
flag = [False,False,False,False,False,False,False,False,False,False]

#array used to store the initial handles the pyqtgraph's plot fuction returns
curves = [[None],[None],[None],[None],[None],[None],[None],[None],[None],[None]]

########################################################################################################################

#get handles for the PYQT GUI file

widgetForm, baseClass= PyQt4.uic.loadUiType("activity_main.ui")

# DEFINE Ui_MainWindow class
class Ui_MainWindow(baseClass, widgetForm):
    """
    __init__ -> ManualConfig -> centerWindow -> bgThread.start -> ConnectSignals -> setupTable -> setupGraph
    +
    updateDisplay method @ 24 Hz
    """



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

        #Setup 40 msec timer to update graph - 24Hz
        timer = QTimer(self)
        timer.timeout.connect(self.updateDisplay)
        timer.start(41.7)

        #start the background thread
        self.bgThread.start()

        #Connect Signals with Slots (PYQT mechanism)
        self.ConnectSignals()

        #Load configuration from config.ini or create it with default values
        config.LoadConfig()

        #Initialise Table
        self.setupTable()

        #Initialise Graph
        self.setupGraph()

########################################################################################################################
    #CLASS'S METHOD DEFINITIONS

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
        self.tableView_sensors.sortByColumn(1, 0)

        #Resize column to fit data - probably no need at the moment
        self.tableView_sensors.resizeColumnsToContents()

        #Can set fixed size for columns if needed
        #self.tableView_sensors.setColumnWidth(7, 250)
        self.tableView_sensors.setColumnWidth(3, 34)

        #Additional table behaviour setups
        self.tableView_sensors.setDragEnabled(False)
        self.tableView_sensors.setAcceptDrops(False)
        self.tableView_sensors.setDragDropOverwriteMode(False)
        self.tableView_sensors.setDropIndicatorShown(True)
        self.tableView_sensors.setSelectionMode(QAbstractItemView.SingleSelection)
        self.tableView_sensors.setSelectionBehavior(QAbstractItemView.SelectRows)
        self.tableView_sensors.setDragDropMode(QAbstractItemView.InternalMove)

        #adding all rows to the table
        rowtypes = ['normal','normal','normal','normal']
        for n in range(9,-1,-1):
            rowdata = [str(n),resources.deviceNames[n], "N/A", self.rgb_to_hex(resources.plotColors[n])]
            self._model.insertRows(0, 1, rowdata, rowtypes)

        #remove dummy row (used to keep all headers,otherwise they would disappear)
        self._model.removeRows(0,0,1)


    def setupGraph(self):
        """
        SETTING UP PYQTGRAPH by creating initial handles, called at app's start (initialisation) as well as
        when pyqtgrah is resetted
        """
        #connecting global variables
        global curves
        global datas

        #redefining the handles array because method is called after erease as well
        curves = [[None],[None],[None],[None],[None],[None],[None],[None],[None],[None]]

        #create plot handles with right pen coloring
        for k in range(0,len(curves)):
                curves[k][0] = self.graphicsView_graph.plot(datas[k],
                                                            pen=(resources.plotColors[k]), name="Sensor"+str(k))

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
        #Connecting the clickong on the Detect Sensor button to the sendIdentifySignal() method
        self.pushButton_main_detectsensor.clicked.connect(self.sendIdentifySignal)

        #Connecting the signal "identify" with the sendMessage() method in the background thread
        self.connect(self,SIGNAL("identify(int)"), self.bgThread.sendMessage,PyQt4.QtCore.Qt.QueuedConnection)

        #Enables the use of SIGNALS AND SLOTS for main thread
        PyQt4.QtCore.QMetaObject.connectSlotsByName(MainWindow)


    def updateDisplay(self):
        """
        Method used to update pyqtgraph at 24 Hz
        """

        #Connecting global variables
        global datas, current_axis_x, prev_axis_x, curves, begin, flag

        #save begining of x axis
        prev_axis_x = current_axis_x

        #Mark the application busy so app doesnt close and create errors while pyqtgrah is drawing
        resources.busy = True


        #if ring buffer overflown
        if (resources.plot_pointer >= resources.end_pointer and resources.overflow== True):

                #for the second part of the buffer
                for i in range(resources.plot_pointer,resources.buffer_size):
                    #save values from ring buffer locally
                    values = resources.buffer[i]
                    #inrement x axis
                    current_axis_x = current_axis_x +1

                    #save values to separate streams to be plotted
                    for j in range(0,10):
                        datas[j].append(values[j])

                #for the first part of the buffer
                for i in range(0,resources.end_pointer):
                    #save values from ring buffer locally
                    values = resources.buffer[i]
                    #inrement x axis
                    current_axis_x = current_axis_x +1

                    #save values to separate streams to be plotted
                    for j in range(0,10):
                        datas[j].append(values[j])

                #catch up plot pointer with end pointer for ring buffer
                resources.plot_pointer = resources.end_pointer
                #lower overflow flag
                resources.overflow = False


        #if there are values in the ring buffer
        if (resources.plot_pointer < resources.end_pointer and resources.overflow==False):
            for i in range(resources.plot_pointer,resources.end_pointer):
                #save values from ring buffer locally
                values = resources.buffer[i]
                #inrement x axis
                current_axis_x = current_axis_x +1

                #save values to separate streams to be plotted
                for j in range(0,10):
                    datas[j].append(values[j])

            #catch up plot pointer with end pointer for ring buffer
            resources.plot_pointer = resources.end_pointer

        #if there are NO values in the ring buffer == Nothing to do
        if (resources.plot_pointer == resources.end_pointer and resources.overflow== False):
            pass

        #Unhandled situation, should never occour if buffer sizes are ideal
        else:
            print "Unhandled plotting situation!"
            print "res end pointer "+ str(resources.end_pointer)
            print "res plot pointer "+ str(resources.plot_pointer)
            print "overflow "+ str(resources.overflow)


        #Plot
        self.plottingFunct(curves,datas)

        #Moving graph x axis
        if (prev_axis_x <= 20):
            self.graphicsView_graph.setXRange(0, 20)
        else:
            self.graphicsView_graph.setXRange(prev_axis_x-19, current_axis_x)


        #Updating table values

        #set up role
        role = PyQt4.QtCore.Qt.EditRole

        #get the number of rows present
        rowCount = self.tableView_sensors.model().rowCount()

        #for each row
        for h in range(0,rowCount):
            #prepare index
            idindex = self.tableView_sensors.model().index(h, 0)
            dataindex = self.tableView_sensors.model().index(h, 2)
            #get id
            id = int(((self.tableView_sensors.model().data(idindex,role)).toString()))

            #if there is data
            if ((len(datas[id])) != 0):
                #if the last data is invalid value, write "N/A"
                if datas[id][-1] == resources.invalid:
                    self.tableView_sensors.model().setData(dataindex,"N/A",role)
                #Otherwise write the last incoming value to the table
                else:
                    self.tableView_sensors.model().setData(dataindex,str(datas[id][-1]),role)


        #Clearing main's buffer if bigger then the allowed size
        if (len(datas[0])>resources.mainBuffer):
            #clearing data streams
            for k in range(0,10):
                 datas[k] = []
            #resetting axis
            current_axis_x=0

            #removing the handles from the graph == wipeing graph
            for b in range(0,len(curves)):
                for a in range(0,len(curves[b])):
                    self.graphicsView_graph.removeItem(curves[b][a])
            #resetting the rest of the globals
            begin = [0,0,0,0,0,0,0,0,0,0]
            flag = [False,False,False,False,False,False,False,False,False,False]
            #recreating the initial handles
            self.setupGraph()

        #Mark the application NOT busy so app can close without creating errors
        resources.busy = False


    def plottingFunct(self,curves,datas):
        """
        Plotting function which realises discontinous plotting if sensor data is deeped invlaid
        """
        #link global variables
        global begin
        global flag

        #create x axis variables
        xaxis = range(0,len(datas[0]))

        #print out values if user wants feedback
        if resources.feedback:
            print "DATA: " + str(datas[0])
            print "XAXIS: " + str(xaxis)

        #for each data stream
        for c in range(0, len(datas)):
            #for each value not yet plotted in the data stream
            for a in range(begin[c], len(datas[0])):
                #if the value in the stream is VALID
                if (datas[c][a] != resources.invalid):
                    #Lower the flag for the data stream
                    flag[c] = False
                    #Updata data for the last curve for the data stream
                    curves[c][-1].setData(xaxis[begin[c]:a], datas[c][begin[c]:a])
                #if the value in the stream is INVALID
                else:
                    #mark the start of the invalid value in the stream
                    begin[c] = a
                    #id the flag is not set(first time the invalid value occoured)
                    if flag[c]==False:
                        #set the flag
                        flag[c] = True
                        #create a new plot handle == new curve on the plot for the data stream
                        curves[c].append(self.graphicsView_graph.plot
                                         (pen=(resources.plotColors[c]), name="Sensor"+str(c)))

    def rgb_to_hex(self,rgb):
        """
        Method used to convert the string RGB values to HEX values to set up the table pixmaps
        """
        return '#%02x%02x%02x' % rgb

    def getCellData(self,row,column):
        """
        Method used to retrive string value from table to get colours
        """
        #set up index and role for table manipulation
        index = self.tableView_sensors.model().index(row, column)
        role = PyQt4.QtCore.Qt.EditRole
        #return string from the specific cell
        return str(((self.tableView_sensors.model().data(index,role)).toString()).toLower())

    def sendIdentifySignal(self):
        """
        Method called when thew user clicks on the "Detect Sensor" button
        """
        #get index of selected row
        indexes = self.tableView_sensors.selectedIndexes()
        #if no rows were selected previously
        if (len(indexes) == 0):
            #Notify user
            print "No sensor is selected!"
        #if there was a row selected
        else:
            #retrive the ID from the ID column
            id = self.getCellData(indexes[0].row(),indexes[0].column())
            #Emit custom identify signal which invokes the background thread's sendMessage method with the id value
            self.emit(SIGNAL("identify(int)"),int(id))


    def closeEvent(self, event):
        """
        Override the closeEvent method of QWidget in main window
        In order to delete window safely when pyqtgraph is not updating, which would cause error
        as well as to save configuration in the config.ini
        """
        #save config
        config.SaveProfile()
        #do not quit until budy flag is raised
        while(resources.busy):
            pass
        #let the window close
        event.accept()

########################################################################################################################

#Main - THIS IS WHERE THE APP STARTS

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
    #Show the Main Window
    ui.show()
    #Hadle exit
    sys.exit(app.exec_())

########################################################################################################################