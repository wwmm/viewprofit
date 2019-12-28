# -*- coding: utf-8 -*-

import os

from PySide2.QtCore import QDateTime
from PySide2.QtSql import QSqlQuery
from PySide2.QtUiTools import QUiLoader
from PySide2.QtWidgets import (QFrame, QGroupBox, QHeaderView, QLineEdit,
                               QMessageBox, QPushButton, QTableView)

from ViewProfit.model_benchmark import ModelBenchmark
from ViewProfit.table_base import TableBase


class TableBenchmarks(TableBase):

    def __init__(self, plot, db):
        TableBase.__init__(self, plot)

        self.module_path = os.path.dirname(__file__)

        self.plot = plot
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
        self.model.dataChanged.connect(self.data_changed)

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

    def load_data(self):
        list_date = []
        list_value = []
        list_accumulated = []

        query = QSqlQuery(self.db)

        query.prepare("select date,value,accumulated from " + self.name + " order by date")

        if query.exec_():
            while query.next():
                date, value, accumulated = query.value(0), query.value(1), query.value(2)

                list_date.append(date)
                list_value.append(value)
                list_accumulated.append(accumulated)

        self.plot.set_title(self.name)
        self.plot.plot_date(list_date, list_value, 0, "Monthly Value")
        self.plot.plot_date(list_date, list_accumulated, 1, "Accumulated")

        self.plot.redraw_canvas()

    def data_changed(self, top_left_index, bottom_right_index, roles):
        self.load_data()
