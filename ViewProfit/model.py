# -*- coding: utf-8 -*-

import numpy as np
from PySide2.QtCore import QAbstractTableModel, QModelIndex, Qt
from PySide2.QtGui import QColor


class Model(QAbstractTableModel):
    def __init__(self):
        QAbstractTableModel.__init__(self)

        self.ncols = 3
        nrows = 1  # initial number of rows

        self.data_name = np.empty(nrows, dtype="object")
        self.data_name[0] = "sample name"

        self.data_pc1 = np.zeros(nrows)
        self.data_pc2 = np.zeros(nrows)

    def rowCount(self, parent=QModelIndex()):
        return self.data_name.size

    def columnCount(self, parent=QModelIndex()):
        return self.ncols

    def flags(self, index):
        return Qt.ItemIsEnabled | Qt.ItemIsSelectable

    def headerData(self, section, orientation, role):
        if role != Qt.DisplayRole:
            return None

        if orientation == Qt.Horizontal:
            return ("Name", "PC1", "PC2")[section]

        return "{}".format(section)

    def data(self, index, role):
        if role == Qt.BackgroundRole:
            return QColor(Qt.white)

        elif role == Qt.TextAlignmentRole:
            return Qt.AlignRight

        elif role == Qt.DisplayRole:
            column = index.column()
            row = index.row()

            if column == 0:
                return self.data_name[row]

            if column == 1:
                return "{0:.6e}".format(self.data_pc1[row])

            if column == 2:
                return "{0:.6e}".format(self.data_pc2[row])

    def remove_rows(self, index_list):
        index_list.sort(reverse=True)

        for index in index_list:
            self.beginRemoveRows(QModelIndex(), index, index)
            self.endRemoveRows()

        self.data_name = np.delete(self.data_name, index_list)
        self.data_pc1 = np.delete(self.data_pc1, index_list)
        self.data_pc2 = np.delete(self.data_pc2, index_list)

    def get_min_max_xy(self):
        return np.amin(self.data_pc1), np.amax(self.data_pc1), np.amin(self.data_pc2), np.amax(self.data_pc2)
