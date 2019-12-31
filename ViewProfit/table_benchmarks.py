# -*- coding: utf-8 -*-


import numpy as np
from PySide2.QtCharts import QtCharts
from PySide2.QtCore import QDateTime, Qt, Signal

from ViewProfit.model_benchmark import ModelBenchmark
from ViewProfit.table_base import TableBase


class TableBenchmarks(TableBase):
    new_mouse_coords = Signal(object,)

    def __init__(self, db, chart):
        TableBase.__init__(self, db, chart)

        self.model = ModelBenchmark(self, db)

        self.table_view.setModel(self.model)

    def recalculate_columns(self):
        list_value = []

        for n in range(self.model.rowCount()):
            list_value.append(self.model.record(n).value("value"))

        if len(list_value) > 0:
            list_value.reverse()

            list_value = np.array(list_value) * 0.01

            accumulated = (np.cumprod(list_value + 1.0) - 1.0) * 100.0

            accumulated = accumulated[::-1]  # reversed array

            for n in range(self.model.rowCount()):
                rec = self.model.record(n)

                rec.setValue("accumulated", float(accumulated[n]))

                self.model.setRecord(n, rec)

    def show_chart(self):
        self.clear_chart()

        self.chart.setTitle(self.name)

        series0 = QtCharts.QLineSeries()
        series1 = QtCharts.QLineSeries()

        series0.setName("Monthly Value")
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
        axis_y.setTitleText("%")
        # axis_y.setLabelFormat("%.1f")
        axis_y.setRange(1.01 * vmin, 1.01 * vmax)

        self.chart.addAxis(axis_x, Qt.AlignBottom)
        self.chart.addAxis(axis_y, Qt.AlignLeft)

        series0.attachAxis(axis_x)
        series0.attachAxis(axis_y)

        series1.attachAxis(axis_x)
        series1.attachAxis(axis_y)
