# -*- coding: utf-8 -*-

import os

from PySide2.QtCharts import QtCharts
from PySide2.QtCore import QDateTime
from PySide2.QtSql import QSqlQuery
from PySide2.QtUiTools import QUiLoader
from PySide2.QtWidgets import (QFrame, QGroupBox, QHeaderView, QLineEdit,
                               QMessageBox, QPushButton, QTableView)

from ViewProfit.model_benchmark import ModelBenchmark
from ViewProfit.table_base import TableBase


class TableBenchmarks(TableBase):

    def __init__(self, chart, db):
        TableBase.__init__(self, chart)

        self.module_path = os.path.dirname(__file__)

        self.chart = chart
        self.db = db
        self.model = ModelBenchmark(self, db)

        loader = QUiLoader()

        self.main_widget = loader.load(self.module_path + "/ui/table_benchmarks.ui")

        self.table_view = self.main_widget.findChild(QTableView, "table_view")
        table_cfg_frame = self.main_widget.findChild(QFrame, "table_cfg_frame")
        self.lineedit_name = self.main_widget.findChild(QLineEdit, "benchmark_name")
        button_update_name = self.main_widget.findChild(QPushButton, "button_update_name")
        button_add_row = self.main_widget.findChild(QPushButton, "button_add_row")
        button_remove_table = self.main_widget.findChild(QPushButton, "button_remove_table")
        self.groupbox_axis = self.main_widget.findChild(QGroupBox, "groupbox_axis")
        self.groupbox_norm = self.main_widget.findChild(QGroupBox, "groupbox_norm")

        self.table_view.horizontalHeader().setSectionResizeMode(QHeaderView.Stretch)
        self.table_view.verticalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.table_view.setModel(self.model)

        # effects

        table_cfg_frame.setGraphicsEffect(self.card_shadow())
        button_update_name.setGraphicsEffect(self.button_shadow())
        button_add_row.setGraphicsEffect(self.button_shadow())
        button_remove_table.setGraphicsEffect(self.button_shadow())

        # signals

        button_update_name.clicked.connect(lambda: self.new_name.emit(self.name, self.lineedit_name.displayText()))
        button_remove_table.clicked.connect(self.on_remove_table)
        button_add_row.clicked.connect(self.add_row)

        # event filter

        self.table_view.installEventFilter(self)

    def add_row(self):
        query = QSqlQuery(self.db)

        query.prepare("insert or ignore into " + self.name + " values (null,?,?,?)")

        query.addBindValue(QDateTime().currentMSecsSinceEpoch())
        query.addBindValue(0.0)
        query.addBindValue(0.0)

        if query.exec_():
            self.model.select()
        else:
            print("failed to create table " + self.name + ". Maybe it already exists.")

    def remove_selected_rows(self):
        s_model = self.table_view.selectionModel()

        if s_model.hasSelection():
            index_list = s_model.selectedRows()
            int_index_list = []

            for index in index_list:
                int_index_list.append(index.row())

            self.model.remove_rows(int_index_list)

    def on_remove_table(self):
        box = QMessageBox(self.main_widget)

        box.setText("Remove this benchmark permanentely from the database?")
        box.setInformativeText("This action cannot be undone!")
        box.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        box.setDefaultButton(QMessageBox.Yes)

        r = box.exec_()

        if r == QMessageBox.Yes:
            self.remove_from_db.emit(self.name)

    def create_series(self):
        self.chart.setTitle(self.name)

        self.series = QtCharts.QLineSeries(self.table_view)

        self.chart.addSeries(self.series)

        self.mapper = QtCharts.QVXYModelMapper()
        self.mapper.setXColumn(1)
        self.mapper.setYColumn(2)
        self.mapper.setSeries(self.series)
        self.mapper.setModel(self.model)

    def update_series(self):
        query = QSqlQuery(self.db)

        query.prepare("select month,value,accumulated from " + self.name)

        # output = []

        # if query.exec_():
        #     while query.next():
        #         month, value, accumulated = query.value(0), query.value(1), query.value(2)
