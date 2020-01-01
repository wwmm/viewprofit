# -*- coding: utf-8 -*-


import os

from PySide2.QtCharts import QtCharts
from PySide2.QtCore import QDateTime, QLocale, QObject, Qt, Signal
from PySide2.QtGui import QColor
from PySide2.QtSql import QSqlQuery, QSqlTableModel
from PySide2.QtUiTools import QUiLoader
from PySide2.QtWidgets import (QFrame, QGraphicsDropShadowEffect, QPushButton,
                               QTableView)

from ViewProfit.model_portfolio import ModelPortfolio


class TablePortfolio(QObject):
    new_mouse_coords = Signal(object,)

    def __init__(self, app):
        QObject.__init__(self)

        self.app = app
        self.db = app.db
        self.chart1 = app.chart1
        self.chart2 = app.chart2
        self.name = "portfolio"

        self.module_path = os.path.dirname(__file__)

        self.model = ModelPortfolio(self, app.db)

        self.model.setTable(self.name)
        self.model.setEditStrategy(QSqlTableModel.OnManualSubmit)
        self.model.setSort(1, Qt.SortOrder.DescendingOrder)

        currency = QLocale().currencySymbol()

        self.model.setHeaderData(1, Qt.Horizontal, "Date")
        self.model.setHeaderData(2, Qt.Horizontal, "Total Contribution " + currency)
        self.model.setHeaderData(3, Qt.Horizontal, "Real Bank Balance " + currency)
        self.model.setHeaderData(4, Qt.Horizontal, "Real Return " + currency)
        self.model.setHeaderData(5, Qt.Horizontal, "Real Return %")

        self.model.select()

        loader = QUiLoader()

        self.main_widget = loader.load(self.module_path + "/ui/table_portfolio.ui")

        table_cfg_frame = self.main_widget.findChild(QFrame, "table_cfg_frame")
        self.table_view = self.main_widget.findChild(QTableView, "table_view")
        button_calculate = self.main_widget.findChild(QPushButton, "button_calculate")
        button_remove_row = self.main_widget.findChild(QPushButton, "button_remove_row")
        button_remove_table = self.main_widget.findChild(QPushButton, "button_remove_table")

        self.table_view.setModel(self.model)

        self.table_view.setColumnHidden(0, True)  # do no show the id column

        # effects

        table_cfg_frame.setGraphicsEffect(self.card_shadow())
        button_calculate.setGraphicsEffect(self.button_shadow())
        button_remove_row.setGraphicsEffect(self.button_shadow())
        button_remove_table.setGraphicsEffect(self.button_shadow())

        # signals

        button_calculate.clicked.connect(self.calculate)
        button_remove_table.clicked.connect(self.remove_table)

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

    def remove_table(self):
        query = QSqlQuery(self.db)

        query.prepare("delete from " + self.name)

        if query.exec_():
            self.model.submitAll()
            self.model.select()
        else:
            print(self.model.lastError().text())

    def calculate(self):
        list_dates = set()

        for table_dict in self.app.tables:
            t = table_dict['object']
            table_type = table_dict['type']

            # making sure all the latest data was saved to the database

            t.model.submitAll()

            if table_type == "investment":
                query = QSqlQuery(self.db)

                query.prepare("select distinct date from " + t.name + " order by date")

                if query.exec_():
                    while query.next():
                        list_dates.add(query.value(0))
                else:
                    print(t.model.lastError().text())

        list_dates = list(list_dates)

        list_dates.sort()

        for date in list_dates:
            total_contribution = 0.0
            real_bank_balance = 0.0

            for table_dict in self.app.tables:
                t = table_dict['object']
                table_type = table_dict['type']

                if table_type == "investment":
                    query = QSqlQuery(self.db)

                    """
                        864000 seconds = 10 days. If the time difference is smaller than that we consider that the dates
                        refer to the same month
                    """

                    query.prepare("select distinct date,total_contribution,real_bank_balance from " + t.name +
                                  " where abs(date - ?) < 864000")

                    query.addBindValue(date)

                    if query.exec_():
                        while query.next():
                            total_contribution += query.value(1)
                            real_bank_balance += query.value(2)
                    else:
                        print(t.model.lastError().text())

            real_return = real_bank_balance - total_contribution
            real_return_perc = 100 * real_return / total_contribution

            query = QSqlQuery(self.db)

            query.prepare("insert or replace into " + self.name + " values ((select id from " + self.name +
                          " where date == ?),?,?,?,?,?)")

            query.addBindValue(date)
            query.addBindValue(date)
            query.addBindValue(total_contribution)
            query.addBindValue(real_bank_balance)
            query.addBindValue(real_return)
            query.addBindValue(real_return_perc)

            if query.exec_():
                self.model.submitAll()
                self.model.select()
            else:
                print(t.model.lastError().text())

        self.show_chart()

    def make_chart1(self):
        self.chart1.setTitle(self.name)

        series0 = QtCharts.QLineSeries()
        series1 = QtCharts.QLineSeries()

        series0.setName("Total Contribution")
        series1.setName("Real Bank Balance")

        series0.hovered.connect(self.on_hover)
        series1.hovered.connect(self.on_hover)

        vmin, vmax = None, None

        for n in range(self.model.rowCount()):
            qdt = QDateTime.fromString(self.model.record(n).value("date"), "dd/MM/yyyy")

            epoch_in_ms = qdt.toMSecsSinceEpoch()

            total_contribution = self.model.record(n).value("total_contribution")
            real_bank_balance = self.model.record(n).value("real_bank_balance")

            if vmin:
                vmin = min(vmin, total_contribution)
                vmin = min(vmin, real_bank_balance)
            else:
                vmin = total_contribution

            if vmax:
                vmax = max(vmax, total_contribution)
                vmax = max(vmax, real_bank_balance)
            else:
                vmax = total_contribution

            series0.append(epoch_in_ms, total_contribution)
            series1.append(epoch_in_ms, real_bank_balance)

        self.chart1.addSeries(series0)
        self.chart1.addSeries(series1)

        axis_x = QtCharts.QDateTimeAxis()
        axis_x.setTitleText("Date")
        axis_x.setFormat("dd/MM/yyyy")
        axis_x.setLabelsAngle(-10)

        axis_y = QtCharts.QValueAxis()
        axis_y.setTitleText(QLocale().currencySymbol())
        axis_y.setLabelFormat("%.2f")
        axis_y.setRange(0.9 * vmin, 1.2 * vmax)

        self.chart1.addAxis(axis_x, Qt.AlignBottom)
        self.chart1.addAxis(axis_y, Qt.AlignLeft)

        series0.attachAxis(axis_x)
        series0.attachAxis(axis_y)

        series1.attachAxis(axis_x)
        series1.attachAxis(axis_y)

    def make_chart2(self):
        self.chart2.setTitle(self.name)

        series0 = QtCharts.QLineSeries()

        series0.setName("Real Return")

        series0.hovered.connect(self.on_hover)

        vmin, vmax = None, None

        for n in range(self.model.rowCount()):
            qdt = QDateTime.fromString(self.model.record(n).value("date"), "dd/MM/yyyy")

            epoch_in_ms = qdt.toMSecsSinceEpoch()

            real_return_perc = self.model.record(n).value("real_return_perc")

            if vmin:
                vmin = min(vmin, real_return_perc)
            else:
                vmin = real_return_perc

            if vmax:
                vmax = max(vmax, real_return_perc)
            else:
                vmax = real_return_perc

            series0.append(epoch_in_ms, real_return_perc)

        self.chart2.addSeries(series0)

        axis_x = QtCharts.QDateTimeAxis()
        axis_x.setTitleText("Date")
        axis_x.setFormat("dd/MM/yyyy")
        axis_x.setLabelsAngle(-10)

        axis_y = QtCharts.QValueAxis()
        axis_y.setTitleText("%")
        axis_y.setLabelFormat("%.2f")

        self.chart2.addAxis(axis_x, Qt.AlignBottom)
        self.chart2.addAxis(axis_y, Qt.AlignLeft)

        series0.attachAxis(axis_x)
        series0.attachAxis(axis_y)

        # add benchmarks

        for table_dict in self.app.tables:
            if table_dict['type'] == "benchmark":
                t = table_dict['object']

                series = QtCharts.QLineSeries()

                series.setName(table_dict['name'])

                series.hovered.connect(self.on_hover)

                for n in range(t.model.rowCount()):
                    qdt = QDateTime.fromString(t.model.record(n).value("date"), "dd/MM/yyyy")

                    epoch_in_ms = qdt.toMSecsSinceEpoch()

                    accu = t.model.record(n).value("accumulated")

                    vmin = min(vmin, accu)
                    vmax = max(vmax, accu)

                    series.append(epoch_in_ms, accu)

                self.chart2.addSeries(series)

                series.attachAxis(axis_x)
                series.attachAxis(axis_y)

        axis_y.setRange(0.9 * vmin, 1.2 * vmax)

    def show_chart(self):
        self.clear_charts()

        if self.model.rowCount() == 0:
            return

        self.make_chart1()
        self.make_chart2()

    def on_hover(self, point, state):
        if state:
            self.new_mouse_coords.emit(point)

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
