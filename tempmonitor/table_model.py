#!/usr/bin/python
# -*- coding: utf-8 -*-
'''
Created on 13 Mar 2016

@author: Tamas Lukacs

@version: 0.1
'''
########################################################################################################################
#This script contains model for TableView                                                                              #
########################################################################################################################

#Import dependenices

########################################################################################################################
from PyQt4 import QtCore, QtGui
from PyQt4.QtCore import Qt
from PyQt4.QtGui import QSortFilterProxyModel, QColor


class sensorModel(QtCore.QAbstractTableModel):
    """
    Class used to instantiate QAbstractTableModel for the Active Sensor talbe
    """
    def __init__(self,entries = [[]],rowtype=[], headers = [], parent = None):
        '''
        Init with entry, rowtype and header
        '''
        QtCore.QAbstractTableModel.__init__(self,parent)
        #Take input and assign it to local private array
        self.__entries = entries
        self.__headers = headers
        self.__rowtype = rowtype


    def rowCount(self, parent= QtCore.QAbstractTableModel):
        '''
        Allows views to know how many data to display
        '''
        #Should return # of rows wanted to be displayed
        return len(self.__entries)

    def columnCount(self, parent):
        '''
        Allows views to know how many column data to display
        '''
        #Should return #of rows wanted to be displayed
        #If all rows are removed return 1 to preserve headers
        if self.__entries == []:
            return 1
        else:
            return len(self.__entries[0])

    def flags(self, index):
        '''
        Called by data() asking whether cell is editable
        '''
        #Prevent table from being edited directly

        #return QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable

        #Enable to edit everything in table
        return QtCore.Qt.ItemIsEditable | QtCore.Qt.ItemIsEnabled | QtCore.Qt.ItemIsSelectable

    def data(self, index, role):
        '''
        Actually hadnles data
        '''
        #Get data from the index object passed
        data = self.__entries[index.row()][index.column()]

        #If at indicator column, prevent the actual color code text to be displayed
        if role == QtCore.Qt.DisplayRole and (index.column()==3):
            #get type of data
            datatype = type(data)
            #If QColor type
            if datatype == type(QColor):
                #Return QColor
                return data
            else:
                #Do not return the color code text
                return QtCore.QVariant()

        #If for dispaying data, return it
        elif role == QtCore.Qt.DisplayRole:
            return data

        #If for editing data, return it
        elif role == QtCore.Qt.EditRole:
            return data

        #If for decorating data(Creating indicator)
        elif role == QtCore.Qt.DecorationRole and (index.column()==3):

            #Create the colored pixmap object based on the data that is there
            pixmap = QtGui.QPixmap(26,26)
            pixmap.fill(QColor(data))
            #Return it
            return pixmap
            #Can convert it to icon but it will be smaller fixed square
            #icon = QtGui.QIcon(pixmap)
            #return icon

        #If for asking row type
        elif role == QtCore.Qt.UserRole:
            #Get position
            row = index.row()
            column = index.column()
            #Get related rowtype and check it
            if self.__rowtype[row] == 'special':
                #Return it if it is special
                return "special"

        elif role ==  QtCore.Qt.TextAlignmentRole:
            return QtCore.Qt.AlignCenter

        #Else do not return data
        else:
            return QtCore.QVariant()


    def setData(self, index, value, role = QtCore.Qt.EditRole):
        '''
        Method which submits data into the model
        '''
        #If data is wanted to be edited
        if role == QtCore.Qt.EditRole:
            #Get position
            row = index.row()
            column = index.column()
            #Stare it to the model's variable
            self.__entries[row][column] = value.toString()
            #Store related rowtype "special" if it was special beforehand
            if self.__rowtype[row] == 'special':
                self.__rowtype[row] = 'special'

            #Store related rowtype "normal" otherwise
            else:
                self.__rowtype[row] = 'normal'

            #Send refreshing signal
            self.dataChanged.emit(index, index)
            #Return success
            return True

        #If "special" data is submitted to the model
        elif role == QtCore.Qt.UserRole:
            #Get position
            row = index.row()
            column = index.column()

            #Store data
            self.__entries[row][column] = value.toString()
            #Store rowtype as "special"
            self.__rowtype[row] = 'special'
            #Send refreshing signal
            self.dataChanged.emit(index, index)
            #Return success
            return True



        #If other Role used (by default QT continuously invokes these methods so it is needed)
        else:
            #Return fail
            return False

    def headerData(self,section, orientation, role):
        '''
        Method enables display information in the header, set headers
        '''
        #If want to get header
        if role == QtCore.Qt.DisplayRole:

            #for horizontal headers
            if orientation == QtCore.Qt.Horizontal:

                #If position is within the number headers specified for model
                if section < len(self.__headers):
                    #Return horizontal headers
                    return self.__headers[section]

                #If position is outside the number headers specified for model
                else:
                    print "Column name not specified"
                    return "NOT SPECIFIED"

            #Return row names
            else:
                return QtCore.QString("SENS %1").arg(section)


    def insertRows(self, position, rows,rowdata=None,rowtype=None, parent = QtCore.QModelIndex()):
        '''
        Method used to insert an entire row into the model
        '''
        #Start indicator for inserting process to start
        self.beginInsertRows(parent, position, position + rows - 1)

        #For the number of rows to be inserted - usually done 1 by 1
        for i in range(rows):
            #Insert row info into model
            self.__entries.insert(position, rowdata)
            #Insert hidden row info about row type
            self.__rowtype.insert(position, rowtype)

        #End indicator for inserting process that it had finished
        self.endInsertRows()
        #Return success
        return True


    def insertColumns(self, position, columns, parent = QtCore.QModelIndex()):
        '''
        Method used to insert an entire column into the model - not used in ACES
        This is jsut to override default method and showcase how to do it
        '''
        #Start indicator for inserting process to start
        self.beginInsertColumns(parent, position, position + columns - 1)

        #Get the currently exsiting rows
        rowCount = len(self.__entries)

        #For the number of columns to be inserted
        for i in range(columns):
            #For each row
            for j in range(rowCount):
                #Create a cell with TEMPCOL data
                self.__entries[j].insert(position, QtCore.QString("TEMPCOL"))

        #End indicator for inserting process that it had finished
        self.endInsertColumns()
        #Return success
        return True

    def removeRows(self, position, rows, columns, parent = QtCore.QModelIndex()):
        '''
        Method used to remove row(s) from model
        '''
        #Start indicator for removing process to start
        self.beginRemoveRows(parent, position, position + columns - 1)

        #For the nubmer of rows to be removed
        for i in range(rows):
            #Get data
            value = self.__entries[position]
            #Get rowtype data
            rowtypeval = self.__rowtype[position]
            #Remove data
            self.__entries.remove(value)
            #Remove rowtype data
            self.__rowtype.remove(rowtypeval)

        #End indicator for removing process that it had finished
        self.endRemoveRows()
        return True


class mySortFilterProxy(QSortFilterProxyModel):
    '''
    Class used to enable overriding QSortFilterProxyModel's default lessThan() method
    in order to prevent thespecial row to be ordered
    Overcome problem by implementing rowtype where rowtype with the value "special" is always
    put at the bottom
    '''
    #Overwrite sorting/ordering method
    def lessThan(self, left, right):
        #If upper row has a rowtype "SPECIAL"
        if (left.data(Qt.UserRole) == "special"):
            #Put it down
            return (self.sortOrder() == Qt.DescendingOrder)
        #If lower row has a rowtype "SPECIAL"
        elif (right.data(Qt.UserRole) == "special"):
            #Keep it down
            return (self.sortOrder() == Qt.AscendingOrder)

        #else not special rowtype
        else:
            #Use the normal sorting/ordering method
            return QSortFilterProxyModel.lessThan(self, left, right)