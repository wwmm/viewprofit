# -*- coding: utf-8 -*-

import os

from PySide2.QtCore import QEvent, QObject, Qt, Signal
from PySide2.QtGui import QColor, QGuiApplication, QKeySequence
from PySide2.QtUiTools import QUiLoader
from PySide2.QtWidgets import QGraphicsDropShadowEffect


class TableBase(QObject):
    new_name = Signal(str, str)
    hide_progress_bar = Signal()

    def __init__(self, app, name):
        QObject.__init__(self)

        self.app = app
        self.db = app.db
        self.chart1 = app.chart1
        self.chart2 = app.chart2
        self.model = None
        self.table_view = None
        self.name = name

        self.module_path = os.path.dirname(__file__)

        self.loader = QUiLoader()

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

                        table_str += "\t".join(map(str, row_value)) + "\n"

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

                return True
            else:
                return QObject.eventFilter(self, obj, event)
        else:
            return QObject.eventFilter(self, obj, event)

        return QObject.eventFilter(self, obj, event)

    def remove_selected_rows(self):
        s_model = self.table_view.selectionModel()

        if s_model.hasSelection():
            index_list = s_model.selectedIndexes()

            row_list, column_list = set(), set()

            for index in index_list:
                row_list.add(index.row())
                column_list.add(index.column())

            # go ahead only if all columns above the id were selected
            for idx in range(1, self.model.columnCount()):
                if idx not in column_list:
                    return

            row_list = list(row_list)

            row_list.sort(reverse=True)

            if len(row_list) > 0:
                for index in row_list:
                    self.model.removeRow(index)

    def clear_charts(self):
        self.chart1.removeAllSeries()
        self.chart2.removeAllSeries()

        if self.chart1.axisX():
            self.chart1.removeAxis(self.chart1.axisX())

        if self.chart1.axisY():
            self.chart1.removeAxis(self.chart1.axisY())

        if self.chart2.axisX():
            self.chart2.removeAxis(self.chart2.axisX())

        if self.chart2.axisY():
            self.chart2.removeAxis(self.chart2.axisY())

    def on_hover(self, point, state):
        if state:
            self.new_mouse_coords.emit(point)
