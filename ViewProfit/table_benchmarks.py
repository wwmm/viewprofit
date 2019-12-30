# -*- coding: utf-8 -*-

import os

import numpy as np
from PySide2.QtCharts import QtCharts
from PySide2.QtCore import QDateTime, Qt, Signal
from PySide2.QtUiTools import QUiLoader
from PySide2.QtWidgets import (QFrame, QGroupBox, QHeaderView, QLineEdit,
                               QMessageBox, QPushButton, QTableView)

from ViewProfit.model_benchmark import ModelBenchmark
from ViewProfit.table_base import TableBase


class TableBenchmarks(TableBase):
    new_mouse_coords = Signal(object,)

    def __init__(self, db, chart):
        TableBase.__init__(self, db, chart)

        self.module_path = os.path.dirname(__file__)

        self.model = ModelBenchmark(self, db)

        loader = QUiLoader()

        self.main_widget = loader.load(self.module_path + "/ui/table_benchmarks.ui")

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
        self.table_view.setModel(self.model)

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

    def add_row(self):
        rec = self.model.record()

        rec.setGenerated("id", False)
        rec.setValue("date", int(QDateTime().currentSecsSinceEpoch()))
        rec.setValue("value", float(0.0))
        rec.setValue("accumulated", float(0.0))

        if self.model.insertRecord(0, rec):
            # self.load_data()
            pass
        else:
            print("failed to add row to table " + self.name)

    def on_remove_table(self):
        box = QMessageBox(self.main_widget)

        box.setText("Remove this benchmark permanentely from the database?")
        box.setInformativeText("This action cannot be undone!")
        box.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        box.setDefaultButton(QMessageBox.Yes)

        r = box.exec_()

        if r == QMessageBox.Yes:
            self.remove_from_db.emit(self.name)

    def recalculate_columns(self):
        list_value = []

        for n in range(self.model.rowCount()):
            list_value.append(self.model.record(n).value("value"))

        if len(list_value) > 0:
            list_value.sort(reverse=True)

            list_value = np.array(list_value) * 0.01

            accumulated = (np.cumprod(list_value + 1.0) - 1.0) * 100.0

            accumulated = accumulated[::-1]  # reversed array

            for n in range(self.model.rowCount()):
                rec = self.model.record(n)

                rec.setGenerated("accumulated", True)
                rec.setValue("accumulated", float(accumulated[n]))

                self.model.setRecord(n, rec)

    def show_chart(self):
        self.chart.removeAllSeries()

        if self.chart.axisX():
            self.chart.removeAxis(self.chart.axisX())

        if self.chart.axisY():
            self.chart.removeAxis(self.chart.axisY())

        self.chart.setTitle(self.name)

        series0 = QtCharts.QLineSeries()
        series1 = QtCharts.QLineSeries()

        series0.setName("Value")
        series1.setName("Accumulated")

        series0.hovered.connect(self.on_hover)
        series1.hovered.connect(self.on_hover)

        vmin, vmax = 0, 0

        for n in range(self.model.rowCount()):
            qdt = QDateTime.fromString(self.model.record(n).value("date"), "dd/MM/yyyy")

            epoch_in_ms = qdt.toMSecsSinceEpoch()

            v = self.model.record(n).value("value")
            accu = self.model.record(n).value("accumulated")

            vmin = min(vmin, v)
            vmin = min(vmin, accu)

            vmax = max(vmax, v)
            vmax = max(vmax, accu)

            series0.append(epoch_in_ms, v)
            series1.append(epoch_in_ms, accu)

        self.chart.addSeries(series0)
        self.chart.addSeries(series1)

        axis_x = QtCharts.QDateTimeAxis()
        axis_x.setTitleText("Date")
        axis_x.setFormat("dd/MM/yyyy")
        axis_x.setLabelsAngle(-10)

        axis_y = QtCharts.QValueAxis()
        # axis_y.setLabelFormat("%.1f")
        axis_y.setRange(1.01 * vmin, 1.01 * vmax)

        self.chart.addAxis(axis_x, Qt.AlignBottom)
        self.chart.addAxis(axis_y, Qt.AlignLeft)

        series0.attachAxis(axis_x)
        series0.attachAxis(axis_y)

        series1.attachAxis(axis_x)
        series1.attachAxis(axis_y)

    def save_table_to_db(self):
        if not self.model.submitAll():
            print("failed to save table " + self.name + " to the database")

            print(self.model.lastError().text())

    def on_hover(self, point, state):
        if state:
            self.new_mouse_coords.emit(point)
