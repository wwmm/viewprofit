# -*- coding: utf-8 -*-

import os

from PySide2.QtCharts import QtCharts
from PySide2.QtSql import QSqlTableModel
from PySide2.QtUiTools import QUiLoader
from PySide2.QtWidgets import (QFrame, QGroupBox, QHeaderView, QLineEdit, QMessageBox,
                               QPushButton, QTableView)

from ViewProfit.table_base import TableBase


class TableBenchmarks(TableBase):

    def __init__(self, chart, db):
        TableBase.__init__(self, chart)

        self.module_path = os.path.dirname(__file__)

        self.chart = chart

        self.model = QSqlTableModel(self, db)

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

        self.table_view.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.table_view.verticalHeader().setSectionResizeMode(QHeaderView.ResizeToContents)
        self.table_view.setModel(self.model)

        # chart series

        self.series = QtCharts.QScatterSeries(self.table_view)
        self.series.setName("table")
        self.series.setMarkerSize(15)

        self.chart.addSeries(self.series)

        self.mapper = QtCharts.QVXYModelMapper()
        self.mapper.setXColumn(1)
        self.mapper.setYColumn(2)
        self.mapper.setSeries(self.series)
        self.mapper.setModel(self.model)

        # effects

        table_cfg_frame.setGraphicsEffect(self.card_shadow())
        button_update_name.setGraphicsEffect(self.button_shadow())
        button_add_row.setGraphicsEffect(self.button_shadow())
        button_remove_table.setGraphicsEffect(self.button_shadow())

        # signals

        button_update_name.clicked.connect(lambda: self.new_name.emit(self.name, self.lineedit_name.displayText()))
        button_remove_table.clicked.connect(self.on_remove_table)

        # event filter

        self.table_view.installEventFilter(self)

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

    def on_remove_table(self):
        box = QMessageBox(self.main_widget)

        box.setText("Remove this benchmark permanentely from the database?")
        box.setInformativeText("This action cannot be undone!")
        box.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        box.setDefaultButton(QMessageBox.Yes)

        r = box.exec_()

        if r == QMessageBox.Yes:
            self.remove_from_db.emit(self.name)
