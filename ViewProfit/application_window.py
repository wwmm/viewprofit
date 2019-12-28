# -*- coding: utf-8 -*-

import os

from PySide2.QtCharts import QtCharts
from PySide2.QtCore import QFile, QObject, Qt
from PySide2.QtGui import QColor, QPainter
from PySide2.QtSql import QSqlDatabase, QSqlQuery
from PySide2.QtUiTools import QUiLoader
from PySide2.QtWidgets import (QFileDialog, QFrame, QGraphicsDropShadowEffect,
                               QLabel, QPushButton, QTabWidget)

from ViewProfit.table_benchmarks import TableBenchmarks


class ApplicationWindow(QObject):
    def __init__(self):
        QObject.__init__(self)

        self.module_path = os.path.dirname(__file__)

        self.tables = []
        self.db = None

        # loading widgets from designer file

        loader = QUiLoader()

        loader.registerCustomWidget(QtCharts.QChartView)

        self.window = loader.load(self.module_path + "/ui/application_window.ui")

        chart_frame = self.window.findChild(QFrame, "chart_frame")
        self.chart_view = self.window.findChild(QtCharts.QChartView, "chart_view")
        self.tab_widget = self.window.findChild(QTabWidget, "tab_widget")
        button_add_fund = self.window.findChild(QPushButton, "button_add_fund")
        button_add_benchmark = self.window.findChild(QPushButton, "button_add_benchmark")
        button_reset_zoom = self.window.findChild(QPushButton, "button_reset_zoom")
        button_save_image = self.window.findChild(QPushButton, "button_save_image")
        self.label_mouse_coords = self.window.findChild(QLabel, "label_mouse_coords")

        # Creating QChart
        self.chart = QtCharts.QChart()
        # self.chart.setAnimationOptions(QtCharts.QChart.AllAnimations)
        self.chart.setTheme(QtCharts.QChart.ChartThemeLight)

        # self.chart.addAxis(self.axis_x, Qt.AlignBottom)
        # self.chart.addAxis(self.axis_y, Qt.AlignLeft)

        self.chart_view.setChart(self.chart)
        self.chart_view.setRenderHint(QPainter.Antialiasing)
        self.chart_view.setRubberBand(QtCharts.QChartView.RectangleRubberBand)

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
        button_add_fund.setGraphicsEffect(self.button_shadow())
        button_add_benchmark.setGraphicsEffect(self.button_shadow())

        # signal connection

        self.tab_widget.currentChanged.connect(self.on_tab_changed)
        button_add_benchmark.clicked.connect(self.add_benchmark_table)
        button_reset_zoom.clicked.connect(self.reset_zoom)
        button_save_image.clicked.connect(self.save_image)

        # init sqlite

        if not QSqlDatabase.isDriverAvailable("QSQLITE"):
            print("sqlite driver is not available!")
        else:
            self.db = QSqlDatabase.addDatabase("QSQLITE")

            self.db.setDatabaseName("viewprofit.sqlite")

            if self.db.open():
                print("the database was opened!")

        self.load_saved_tables()

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
                names.append(query.value(0))
        else:
            print("failed to get table names")

        for name in names:
            query.prepare("select * from " + name)

            if query.exec_():
                n_cols = query.record().count()

                if n_cols == 4:
                    self.add_table('benchmark', name)

    def on_tab_changed(self, index):
        self.chart.removeAllSeries()

        if self.chart.axisX():
            self.chart.removeAxis(self.chart.axisX())

        if self.chart.axisY():
            self.chart.removeAxis(self.chart.axisY())

        if index > 0:
            index = index - 1  # do not count the Total tab

            table_dict = self.tables[index]
            table = table_dict['object']
            table_type = table_dict['type']

            if table_type == "benchmark":
                pass
            else:
                pass

            table.create_series()
        else:
            self.chart.setTitle("Total")

    def remove_table(self, name):
        for index in range(len(self.tables)):
            table_dict = self.tables[index]

            if name == table_dict['name']:
                self.tab_widget.removeTab(index + 1)  # Total tab is not in self.tables

                t = table_dict['object']

                self.chart.removeSeries(t.series)

                self.tables.remove(table_dict)

                query = QSqlQuery(self.db)

                query.prepare("drop table if exists " + name)

                if not query.exec_():
                    print("failed remove table " + name + ". Maybe has already been removed.")

                break

    def add_benchmark_table(self):
        name = "Benchmark" + str(len(self.tables))

        query = QSqlQuery(self.db)

        query.prepare("create table " + name + " (id integer primary key, date int, value real, accumulated real)")

        if not query.exec_():
            print("failed to create table " + name + ". Maybe it already exists.")

        self.add_table('benchmark', name)

    def add_table(self, table_type, name):
        table = TableBenchmarks(self.chart, self.db)

        # data series

        table.create_series()

        # signals

        table.new_name.connect(self.update_table_name)
        table.remove_from_db.connect(self.remove_table)

        table.name = name

        table.lineedit_name.setText(name)

        table.model.setTable(name)
        table.model.setHeaderData(1, Qt.Horizontal, "Date")
        table.model.setHeaderData(2, Qt.Horizontal, "Value %")
        table.model.setHeaderData(3, Qt.Horizontal, "Accumulated %")
        table.model.select()

        table.table_view.setColumnHidden(0, True)

        self.tab_widget.addTab(table.main_widget, name)

        self.tables.append(dict({'object': table, 'type': table_type, 'name': name}))

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

                if not query.exec_():
                    print("failed to rename table " + old_name)

        self.chart.setTitle(new_name)

    def on_new_mouse_coords(self, point):
        self.label_mouse_coords.setText("x = {0:.6f}, y = {1:.6f}".format(point.x(), point.y()))

    def save_image(self):
        home = os.path.expanduser("~")

        path = QFileDialog.getSaveFileName(self.window, "Save Image",  home, "PNG (*.png)")[0]

        if path != "":
            if not path.endswith(".png"):
                path += ".png"

            pixmap = self.chart_view.grab()

            pixmap.save(path)

    def reset_zoom(self):
        self.chart.zoomReset()
