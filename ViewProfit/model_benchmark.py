# -*- coding: utf-8 -*-

from PySide2.QtCore import QDate, QDateTime, Qt
from PySide2.QtGui import QColor
from PySide2.QtSql import QSqlTableModel


class ModelBenchmark(QSqlTableModel):
    def __init__(self, parent, db):
        QSqlTableModel.__init__(self, parent, db)

    def flags(self, index):
        column = index.column()

        if column < 3:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable | Qt.ItemIsEditable
        else:
            return Qt.ItemIsEnabled | Qt.ItemIsSelectable

    def data(self, index, role):
        if role == Qt.BackgroundRole:
            return QColor(Qt.white)

        elif role == Qt.TextAlignmentRole:
            return Qt.AlignRight

        elif role == Qt.DisplayRole or role == Qt.EditRole:
            column = index.column()

            if column == 1:
                variant_data = QSqlTableModel.data(self, index, role)

                if isinstance(variant_data, int):
                    qdt = QDateTime()

                    qdt.setSecsSinceEpoch(variant_data)

                    return qdt.toString(Qt.SystemLocaleDate).split(" ")[0]
                else:
                    return variant_data

            elif column == 2 or column == 3:
                variant_data = QSqlTableModel.data(self, index, role)

                return variant_data * 100

            return QSqlTableModel.data(self, index, role)
        else:
            return QSqlTableModel.data(self, index, role)

    def setData(self, index, value, role):
        if role != Qt.EditRole:
            return False

        column = index.column()

        if column == 1:
            if isinstance(value, str):
                qdt = QDateTime()

                qdt.setDate(QDate.fromString(value, "dd/MM/yyyy"))

                return QSqlTableModel.setData(self, index, qdt.toSecsSinceEpoch(), role)
            else:
                return False
        elif column == 2:
            return QSqlTableModel.setData(self, index, value * 0.01, role)
        else:
            return QSqlTableModel.setData(self, index, value, role)
