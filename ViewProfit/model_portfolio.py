# -*- coding: utf-8 -*-

from PySide2.QtCore import QDateTime, Qt
from PySide2.QtGui import QColor
from PySide2.QtSql import QSqlTableModel


class ModelPortfolio(QSqlTableModel):
    def __init__(self, parent, db):
        QSqlTableModel.__init__(self, parent, db)

    def flags(self, index):
        return Qt.ItemIsEnabled | Qt.ItemIsSelectable

    def data(self, index, role):
        if role == Qt.BackgroundRole:
            return QColor(Qt.white)

        elif role == Qt.TextAlignmentRole:
            return Qt.AlignRight

        elif role == Qt.DisplayRole or role == Qt.EditRole:
            column = index.column()

            if column == 1:
                v = QSqlTableModel.data(self, index, role)

                if isinstance(v, int):
                    qdt = QDateTime()

                    qdt.setSecsSinceEpoch(v)

                    return qdt.toString(Qt.SystemLocaleDate).split(" ")[0]
                else:
                    return v

            return QSqlTableModel.data(self, index, role)

        else:
            return QSqlTableModel.data(self, index, role)
