# -*- coding: utf-8 -*-

import os

from PySide2.QtCharts import QtCharts
from PySide2.QtCore import (QCoreApplication, QDateTime, QFile, QObject,
                            QSettings)
from PySide2.QtGui import QColor, QPainter
from PySide2.QtSql import QSqlDatabase, QSqlQuery
from PySide2.QtUiTools import QUiLoader
from PySide2.QtWidgets import (QFileDialog, QFrame, QGraphicsDropShadowEffect,
                               QLabel, QPushButton, QRadioButton,
                               QStackedWidget, QTabWidget)

from ViewProfit.table_benchmarks import TableBenchmarks
from ViewProfit.table_investment import TableInvestment
from ViewProfit.table_portfolio import TablePortfolio


class ApplicationWindow(QObject):
    def __init__(self):
        QObject.__init__(self)

        self.module_path = os.path.dirname(__file__)

        self.tables = []
        self.db = None

        # configuring QSettings

        QCoreApplication.setOrganizationName("wwmm")
        QCoreApplication.setApplicationName("ViewProfit")

        self.qsettings = QSettings()

        # loading widgets from designer file

        loader = QUiLoader()

        loader.registerCustomWidget(QtCharts.QChartView)

        self.window = loader.load(self.module_path + "/ui/application_window.ui")

        chart_frame = self.window.findChild(QFrame, "chart_frame")
        self.chart_view1 = self.window.findChild(QtCharts.QChartView, "chart_view1")
        self.chart_view2 = self.window.findChild(QtCharts.QChartView, "chart_view2")
        self.radio_chart1 = self.window.findChild(QRadioButton, "radio_chart1")
        self.radio_chart2 = self.window.findChild(QRadioButton, "radio_chart2")
        self.stackedwidget = self.window.findChild(QStackedWidget, "stackedwidget")
        self.tab_widget = self.window.findChild(QTabWidget, "tab_widget")
        button_add_investment = self.window.findChild(QPushButton, "button_add_investment")
        button_add_benchmark = self.window.findChild(QPushButton, "button_add_benchmark")
        button_reset_zoom = self.window.findChild(QPushButton, "button_reset_zoom")
        button_save_image = self.window.findChild(QPushButton, "button_save_image")
        self.label_mouse_xy = self.window.findChild(QLabel, "label_mouse_xy")

        # custom stylesheet

        style_file = QFile(self.module_path + "/ui/custom.css")
        style_file.open(QFile.ReadOnly)

        self.window.setStyleSheet(style_file.readAll().data().decode("utf-8"))

        style_file.close()

        # effects

        self.tab_widget.setGraphicsEffect(self.card_shadow())
        chart_frame.setGraphicsEffect(self.card_shadow())
        button_reset_zoom.setGraphicsEffect(self.button_shadow())
        button_save_image.setGraphicsEffect(self.button_shadow())
        button_add_investment.setGraphicsEffect(self.button_shadow())
        button_add_benchmark.setGraphicsEffect(self.button_shadow())

        # signal connection

        self.tab_widget.currentChanged.connect(self.on_tab_changed)
        button_add_benchmark.clicked.connect(self.add_benchmark_table)
        button_add_investment.clicked.connect(self.add_investment_table)
        button_reset_zoom.clicked.connect(self.reset_zoom)
        button_save_image.clicked.connect(self.save_image)
        self.radio_chart1.toggled.connect(self.on_chart_selection)
        self.radio_chart2.toggled.connect(self.on_chart_selection)

        # init sqlite

        if not QSqlDatabase.isDriverAvailable("QSQLITE"):
            print("sqlite driver is not available!")
        else:
            self.db = QSqlDatabase.addDatabase("QSQLITE")

            self.db.setDatabaseName("viewprofit.sqlite")

            if self.db.open():
                print("the database was opened!")

                query = QSqlQuery(self.db)

                query.prepare("create table if not exists portfolio" " (id integer primary key, date int," +
                              " total_contribution real, real_bank_balance real, real_return real," +
                              " real_return_perc real)")

                if not query.exec_():
                    print("failed to create table portfolio. Maybe it already exists.")

        # Creating QChart
        self.chart1 = QtCharts.QChart()
        self.chart1.setAnimationOptions(QtCharts.QChart.GridAxisAnimations)
        self.chart1.setTheme(QtCharts.QChart.ChartThemeLight)
        self.chart1.setAcceptHoverEvents(True)

        self.chart2 = QtCharts.QChart()
        self.chart2.setAnimationOptions(QtCharts.QChart.GridAxisAnimations)
        self.chart2.setTheme(QtCharts.QChart.ChartThemeLight)
        self.chart2.setAcceptHoverEvents(True)

        self.chart_view1.setChart(self.chart1)
        self.chart_view1.setRenderHint(QPainter.Antialiasing)
        self.chart_view1.setRubberBand(QtCharts.QChartView.RectangleRubberBand)

        self.chart_view2.setChart(self.chart2)
        self.chart_view2.setRenderHint(QPainter.Antialiasing)
        self.chart_view2.setRubberBand(QtCharts.QChartView.RectangleRubberBand)

        # select the default chart

        if self.radio_chart1.isChecked():
            self.stackedwidget.setCurrentIndex(0)
        elif self.radio_chart2.isChecked():
            self.stackedwidget.setCurrentIndex(1)

        # load the portfolio table

        self.table_portfolio = TablePortfolio(self)

        self.tab_widget.addTab(self.table_portfolio.main_widget, self.table_portfolio.name)

        self.table_portfolio.new_mouse_coords.connect(self.on_new_mouse_coords)

        # load the other tables

        self.load_saved_tables()

        # now that we have the benchmarks tables we can call this function

        self.table_portfolio.show_chart()

        # show window

        self.window.show()

    def button_shadow(self):
        effect = QGraphicsDropShadowEffect(self.window)

        effect.setColor(QColor(0, 0, 0, 100))
        effect.setXOffset(1)
        effect.setYOffset(1)
        effect.setBlurRadius(5)

        return effect

    def card_shadow(self):
        effect = QGraphicsDropShadowEffect(self.window)

        effect.setColor(QColor(0, 0, 0, 100))
        effect.setXOffset(2)
        effect.setYOffset(2)
        effect.setBlurRadius(5)

        return effect

    def load_saved_tables(self):
        query = QSqlQuery(self.db)

        query.prepare("select name from sqlite_master where type='table'")

        names = []

        if query.exec_():
            while query.next():
                name = query.value(0)

                if name != "portfolio":
                    names.append(name)

            names.sort()
        else:
            print("failed to get table names")

        for name in names:
            query.prepare("select * from " + name)

            if query.exec_():
                n_cols = query.record().count()

                if n_cols == 4:
                    self.add_table('benchmark', name)
                else:
                    self.add_table('investment', name)

    def on_tab_changed(self, index):
        if index > 0:
            index = index - 1  # do not count the Portfolio tab

            table_dict = self.tables[index]
            table = table_dict['object']
            table_type = table_dict['type']

            if table_type == "benchmark":
                self.radio_chart1.setText("Monthly Values")
                self.radio_chart2.setText("Accumulated Values")
            elif table_type == "investment":
                self.radio_chart1.setText("Absolute Values")
                self.radio_chart2.setText("Percentage Values")

            table.calculate()
        else:
            self.radio_chart1.setText("Absolute Values")
            self.radio_chart2.setText("Percentage Values")

            self.table_portfolio.show_chart()

    def remove_table(self, name):
        for index in range(len(self.tables)):
            table_dict = self.tables[index]

            if name == table_dict['name']:
                self.tab_widget.removeTab(index + 1)  # The Portfolio tab is not in self.tables

                self.tables.remove(table_dict)

                self.qsettings.beginGroup(name)

                self.qsettings.remove("")

                self.qsettings.endGroup()

                query = QSqlQuery(self.db)

                query.prepare("drop table if exists " + name)

                if not query.exec_():
                    print("failed remove table " + name + ". Maybe has already been removed.")

                break

    def add_benchmark_table(self):
        name = "Benchmark" + str(len(self.tables))

        query = QSqlQuery(self.db)

        query.prepare("create table " + name + " (id integer primary key," +
                      " date int default (cast(strftime('%s','now') as int))," + " value real default 0.0," +
                      " accumulated real default 0.0)")

        if not query.exec_():
            print("failed to create table " + name + ". Maybe it already exists.")

        self.add_table('benchmark', name)

        self.tab_widget.setCurrentIndex(self.tab_widget.count() - 1)

    def add_investment_table(self):
        name = "Investment" + str(len(self.tables))

        query = QSqlQuery(self.db)

        query.prepare("create table " + name + " (id integer primary key," +
                      " date int default (cast(strftime('%s','now') as int))," + " contribution real default 0.0," +
                      " bank_balance real default 0.0," + " total_contribution real default 0.0," +
                      " gross_return real default 0.0," + " gross_return_perc real default 0.0," +
                      " real_return real default 0.0," + " real_return_perc real default 0.0," +
                      " real_bank_balance real default 0.0)")

        if not query.exec_():
            print("failed to create table " + name + ". Maybe it already exists.")

        self.add_table('investment', name)

        self.tab_widget.setCurrentIndex(self.tab_widget.count() - 1)

    def add_table(self, table_type, name):
        table = None

        if table_type == "benchmark":
            table = TableBenchmarks(self, name)
        elif table_type == "investment":
            table = TableInvestment(self, name)

        self.tab_widget.addTab(table.main_widget, name)

        self.tables.append(dict({'object': table, 'type': table_type, 'name': name}))

        # signals

        table.new_name.connect(self.update_table_name)
        table.remove_from_db.connect(self.remove_table)
        table.new_mouse_coords.connect(self.on_new_mouse_coords)

    def update_table_name(self, old_name, new_name):
        for n in range(len(self.tables)):
            table_dict = self.tables[n]
            table_name = table_dict['name']

            if table_name == old_name:
                table = table_dict['object']

                table.name = new_name
                table_dict['name'] = new_name

                self.tab_widget.setTabText(n + 1, new_name)

                query = QSqlQuery(self.db)

                query.prepare("alter table " + old_name + " rename to " + new_name)

                if query.exec_():
                    # table.model.submitAll()
                    # table.model.setTable(new_name)
                    # table.model.select()
                    pass
                else:
                    print("failed to rename table " + old_name)

                break

        self.chart1.setTitle(new_name)
        self.chart2.setTitle(new_name)

    def on_new_mouse_coords(self, point):
        qdt = QDateTime()

        qdt.setMSecsSinceEpoch(point.x())

        self.label_mouse_xy.setText("x = {0}, y = {1:.2f}".format(qdt.toString("dd/MM/yyyy"), point.y()))

    def save_image(self):
        home = os.path.expanduser("~")

        path = QFileDialog.getSaveFileName(self.window, "Save Image",  home, "PNG (*.png)")[0]

        if path != "":
            if not path.endswith(".png"):
                path += ".png"

            if self.radio_chart1.isChecked():
                pixmap = self.chart_view1.grab()

                pixmap.save(path)

            if self.radio_chart2.isChecked():
                pixmap = self.chart_view2.grab()

                pixmap.save(path)

    def reset_zoom(self):
        if self.radio_chart1.isChecked():
            self.chart1.zoomReset()

        if self.radio_chart2.isChecked():
            self.chart2.zoomReset()

    def on_chart_selection(self, state):
        if state:
            if self.radio_chart1.isChecked():
                self.stackedwidget.setCurrentIndex(0)
            elif self.radio_chart2.isChecked():
                self.stackedwidget.setCurrentIndex(1)
