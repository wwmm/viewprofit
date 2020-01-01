# -*- coding: utf-8 -*-

from PySide2.QtCore import QDateTime, Signal
from PySide2.QtWidgets import (QFrame, QHeaderView, QLineEdit, QMessageBox,
                               QPushButton, QTableView)

from ViewProfit.table_base import TableBase


class TableBaseIB(TableBase):
    new_name = Signal(str, str)
    remove_from_db = Signal(str,)

    def __init__(self, app, name):
        TableBase.__init__(self, app, name)

        self.main_widget = self.loader.load(self.module_path + "/ui/table.ui")

        self.table_view = self.main_widget.findChild(QTableView, "table_view")
        table_cfg_frame = self.main_widget.findChild(QFrame, "table_cfg_frame")
        self.lineedit_name = self.main_widget.findChild(QLineEdit, "table_name")
        button_update_name = self.main_widget.findChild(QPushButton, "button_update_name")
        button_add_row = self.main_widget.findChild(QPushButton, "button_add_row")
        button_calculate = self.main_widget.findChild(QPushButton, "button_calculate")
        button_save_table = self.main_widget.findChild(QPushButton, "button_save_table")
        button_remove_table = self.main_widget.findChild(QPushButton, "button_remove_table")
        button_remove_row = self.main_widget.findChild(QPushButton, "button_remove_row")

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

    def add_row(self):
        rec = self.model.record()

        rec.setGenerated("id", False)
        rec.setValue("date", int(QDateTime().currentSecsSinceEpoch()))

        if not self.model.insertRecord(0, rec):
            print("failed to add row to table " + self.name)

            print(self.model.lastError().text())

    def calculate(self):
        self.recalculate_columns()

        self.show_chart()

    def on_remove_table(self):
        box = QMessageBox(self.main_widget)

        box.setText("Remove this table from the database?")
        box.setInformativeText("This action cannot be undone!")
        box.setStandardButtons(QMessageBox.Yes | QMessageBox.No)
        box.setDefaultButton(QMessageBox.Yes)

        r = box.exec_()

        if r == QMessageBox.Yes:
            self.remove_from_db.emit(self.name)

    def save_table_to_db(self):
        if not self.model.submitAll():
            print("failed to save table " + self.name + " to the database")

            print(self.model.lastError().text())
