# -*- coding: utf-8 -*-

import os

import numpy as np
from PySide2.QtCore import QEvent, QObject, Qt, Signal
from PySide2.QtGui import QColor, QGuiApplication, QKeySequence
from PySide2.QtWidgets import QGraphicsDropShadowEffect


class TableBase(QObject):
    new_name = Signal(str, str)
    remove_from_db = Signal(str,)

    def __init__(self, plot):
        QObject.__init__(self)

        self.module_path = os.path.dirname(__file__)

        self.plot = plot
        self.model = None
        self.table_view = None
        self.series = None
        self.name = "table"

    def button_shadow(self):
        effect = QGraphicsDropShadowEffect(self.main_widget)

        effect.setColor(QColor(0, 0, 0, 100))
        effect.setXOffset(1)
        effect.setYOffset(1)
        effect.setBlurRadius(5)

        return effect

    def card_shadow(self):
        effect = QGraphicsDropShadowEffect(self.main_widget)

        effect.setColor(QColor(0, 0, 0, 100))
        effect.setXOffset(2)
        effect.setYOffset(2)
        effect.setBlurRadius(5)

        return effect

    def eventFilter(self, obj, event):
        if event.type() == QEvent.KeyPress:
            if event.key() == Qt.Key_Delete:
                self.remove_selected_rows()

                return True
            elif event.matches(QKeySequence.Copy):
                s_model = self.table_view.selectionModel()

                if s_model.hasSelection():
                    selection_range = s_model.selection().constFirst()

                    table_str = ""
                    clipboard = QGuiApplication.clipboard()

                    for i in range(selection_range.top(), selection_range.bottom() + 1):
                        row_value = []

                        for j in range(selection_range.left(), selection_range.right() + 1):
                            row_value.append(s_model.model().index(i, j).data())

                        table_str += "\t".join(row_value) + "\n"

                    clipboard.setText(table_str)

                return True
            elif event.matches(QKeySequence.Paste):
                s_model = self.table_view.selectionModel()

                if s_model.hasSelection():
                    clipboard = QGuiApplication.clipboard()

                    table_str = clipboard.text()
                    table_rows = table_str.splitlines()  # splitlines avoids an empty line at the end

                    selection_range = s_model.selection().constFirst()

                    first_row = selection_range.top()
                    first_col = selection_range.left()
                    last_col_idx = 0
                    last_row_idx = 0

                    for i in range(len(table_rows)):
                        model_i = first_row + i

                        if model_i < self.model.rowCount():
                            row_cols = table_rows[i].split("\t")

                            for j in range(len(row_cols)):
                                model_j = first_col + j

                                if model_j < self.model.columnCount():
                                    self.model.setData(self.model.index(model_i, model_j), row_cols[j], Qt.EditRole)

                                    if model_j > last_col_idx:
                                        last_col_idx = model_j

                            if model_i > last_row_idx:
                                last_row_idx = model_i

                    first_index = self.model.index(first_row, first_col)
                    last_index = self.model.index(last_row_idx, last_col_idx)

                    self.model.dataChanged.emit(first_index, last_index)

                return True
            else:
                return QObject.eventFilter(self, obj, event)
        else:
            return QObject.eventFilter(self, obj, event)

        return QObject.eventFilter(self, obj, event)

    def add_row(self):
        self.model.append_row()

    def remove_selected_rows(self):
        s_model = self.table_view.selectionModel()

        if s_model.hasSelection():
            index_list = s_model.selectedRows()
            int_index_list = []

            for index in index_list:
                int_index_list.append(index.row())

            self.model.remove_rows(int_index_list)

    def selection_changed(self, selected, deselected):
        s_model = self.table_view.selectionModel()

        if s_model.hasSelection():
            selection_labels = []
            selection_pc1 = []
            selection_pc2 = []
            indexes = s_model.selectedRows()

            for index in indexes:
                row_idx = index.row()

                selection_labels.append(self.model.data_name[row_idx])
                selection_pc1.append(self.model.data_pc1[row_idx])
                selection_pc2.append(self.model.data_pc2[row_idx])

            # update the model used to show the selected rows

            self.model_selection.beginResetModel()

            self.model_selection.data_name = np.asarray(selection_labels)
            self.model_selection.data_pc1 = np.asarray(selection_pc1)
            self.model_selection.data_pc2 = np.asarray(selection_pc2)

            self.model_selection.endResetModel()

            first_index = self.model_selection.index(0, 0)
            last_index = self.model_selection.index(self.model_selection.rowCount() - 1,
                                                    self.model_selection.columnCount() - 1)

            self.model_selection.dataChanged.emit(first_index, last_index)
        else:
            print("no selection")
