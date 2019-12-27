# -*- coding: utf-8 -*-

import os

import numpy as np
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
        self.chart.setAnimationOptions(QtCharts.QChart.AllAnimations)
        self.chart.setTheme(QtCharts.QChart.ChartThemeLight)
        self.chart.setAcceptHoverEvents(True)

        self.axis_x = QtCharts.QValueAxis()
        self.axis_x.setTitleText("x")
        self.axis_x.setRange(-10, 10)
        self.axis_x.setLabelFormat("%.1f")

        self.axis_y = QtCharts.QValueAxis()
        self.axis_y.setTitleText("y")
        self.axis_y.setRange(-10, 10)
        self.axis_y.setLabelFormat("%.1f")

        self.chart.addAxis(self.axis_x, Qt.AlignBottom)
        self.chart.addAxis(self.axis_y, Qt.AlignLeft)

        self.chart_view.setChart(self.chart)
        self.chart_view.setRenderHint(QPainter.Antialiasing)
        self.chart_view.setRubberBand(QtCharts.QChartView.RectangleRubberBand)

        # 1 tab by default

        # self.graph = Graph()

        # self.tab_widget.addTab(self.graph.main_widget, "Chart")

        # self.add_tab()

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
        if index > 0:
            index = index - 1  # do not count the Total tab

            table_dict = self.tables[index]
            # table = table_dict['object']
            table_type = table_dict['type']
            table_name = table_dict['name']

            if table_type == "benchmark":
                pass
            else:
                pass

            self.chart.setTitle(table_name)
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

                break

    def add_benchmark_table(self):
        name = "Benchmark" + str(len(self.tables))

        query = QSqlQuery(self.db)

        query.prepare("create table " + name +
                      " (id integer primary key, data int, mensal int, acumulado int)")

        if not query.exec_():
            print("failed to create table " + name + ". Maybe it already exists.")

        self.add_table('benchmark', name)

    def add_table(self, table_type, name):
        table = TableBenchmarks(self.chart, self.db)

        # data series

        table.series.attachAxis(self.axis_x)
        table.series.attachAxis(self.axis_y)

        # signals

        # table.model.dataChanged.connect(self.update_scale)
        table.new_name.connect(self.update_table_name)
        table.remove_from_db.connect(self.remove_table)

        table.name = name
        table.lineedit_name.setText(name)

        table.model.setTable(name)
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

    def update_scale(self):
        n_tables = len(self.tables)

        if n_tables > 0:
            Xmin, Xmax, Ymin, Ymax = self.tables[0].model.get_min_max_xy()

            if n_tables > 1:
                for n in range(n_tables):
                    xmin, xmax, ymin, ymax = self.tables[n].model.get_min_max_xy()

                    if xmin < Xmin:
                        Xmin = xmin

                    if xmax > Xmax:
                        Xmax = xmax

                    if ymin < Ymin:
                        Ymin = ymin

                    if ymax > Ymax:
                        Ymax = ymax

            fraction = 0.15
            self.axis_x.setRange(Xmin - fraction * np.fabs(Xmin), Xmax + fraction * np.fabs(Xmax))
            self.axis_y.setRange(Ymin - fraction * np.fabs(Ymin), Ymax + fraction * np.fabs(Ymax))

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
