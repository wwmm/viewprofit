# -*- coding: utf-8 -*-

import os

from PySide2.QtCore import QEvent, QObject, Qt, Signal
from PySide2.QtGui import QColor, QGuiApplication, QKeySequence
from PySide2.QtUiTools import QUiLoader
from PySide2.QtWidgets import (QFrame, QGraphicsDropShadowEffect, QGroupBox,
                               QHeaderView, QLineEdit, QMessageBox,
                               QPushButton, QTableView)


class TableBase(QObject):
    new_name = Signal(str, str)
    remove_from_db = Signal(str,)

    def __init__(self, db, chart):
        QObject.__init__(self)

        self.db = db
        self.chart = chart
        self.model = None
        self.table_view = None
        self.series = None
        self.name = "table"

        self.module_path = os.path.dirname(__file__)

        loader = QUiLoader()

        self.main_widget = loader.load(self.module_path + "/ui/table.ui")

        self.table_view = self.main_widget.findChild(QTableView, "table_view")
        table_cfg_frame = self.main_widget.findChild(QFrame, "table_cfg_frame")
        self.lineedit_name = self.main_widget.findChild(QLineEdit, "benchmark_name")
        button_update_name = self.main_widget.findChild(QPushButton, "button_update_name")
        button_add_row = self.main_widget.findChild(QPushButton, "button_add_row")
        button_calculate = self.main_widget.findChild(QPushButton, "button_calculate")
        button_save_table = self.main_widget.findChild(QPushButton, "button_save_table")
        button_remove_table = self.main_widget.findChild(QPushButton, "button_remove_table")
        button_remove_row = self.main_widget.findChild(QPushButton, "button_remove_row")
        self.groupbox_axis = self.main_widget.findChild(QGroupBox, "groupbox_axis")
        self.groupbox_norm = self.main_widget.findChild(QGroupBox, "groupbox_norm")

        self.table_view.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.table_view.verticalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)

        # effects

        table_cfg_frame.setGraphicsEffect(self.card_shadow())
        button_update_name.setGraphicsEffect(self.button_shadow())
        button_add_row.setGraphicsEffect(self.button_shadow())
        button_calculate.setGraphicsEffect(self.button_shadow())
        button_save_table.setGraphicsEffect(self.button_shadow())
        button_remove_table.setGraphicsEffect(self.button_shadow())
        button_remove_row.setGraphicsEffect(self.button_shadow())

        # signals

        button_update_name.clicked.connect(lambda: self.new_name.emit(self.name, self.lineedit_name.displayText()))
        button_remove_table.clicked.connect(self.on_remove_table)
        button_save_table.clicked.connect(self.save_table_to_db)
        button_remove_row.clicked.connect(self.remove_selected_rows)
        button_add_row.clicked.connect(self.add_row)
        button_calculate.clicked.connect(self.calculate)

        # event filter

        self.table_view.installEventFilter(self)

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

    def calculate(self):
        self.recalculate_columns()

        self.show_chart()

    def clear_chart(self):
        self.chart.removeAllSeries()

        if self.chart.axisX():
            self.chart.removeAxis(self.chart.axisX())

        if self.chart.axisY():
            self.chart.removeAxis(self.chart.axisY())

    def on_remove_table(self):
        box = QMessageBox(self.main_widget)

        box.setText("Remove this table from the database?")
        box.setInformativeText("This action cannot be undone!")
        box.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        box.setDefaultButton(QMessageBox.Yes)

        r = box.exec_()

        if r == QMessageBox.Yes:
            self.remove_from_db.emit(self.name)

    def on_hover(self, point, state):
        if state:
            self.new_mouse_coords.emit(point)

    def save_table_to_db(self):
        if not self.model.submitAll():
            print("failed to save table " + self.name + " to the database")

            print(self.model.lastError().text())
