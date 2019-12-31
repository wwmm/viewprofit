# -*- coding: utf-8 -*-


import numpy as np
from PySide2.QtCharts import QtCharts
from PySide2.QtCore import QDateTime, Qt, Signal
from PySide2.QtWidgets import QCheckBox, QFrame

from ViewProfit.model_investment import ModelInvestment
from ViewProfit.table_base import TableBase


class TableInvestment(TableBase):
    new_mouse_coords = Signal(object,)

    def __init__(self, db, chart):
        TableBase.__init__(self, db, chart)

        self.model = ModelInvestment(self, db)

        self.table_view.setModel(self.model)

        cfg_widget = self.loader.load(self.module_path + "/ui/investment_chart_cfg.ui")

        chart_cfg_frame = cfg_widget.findChild(QFrame, "chart_cfg_frame")
        self.checbox_total_contribution = cfg_widget.findChild(QFrame, "checbox_total_contribution")
        self.checkbox_real_bank_balance = cfg_widget.findChild(QFrame, "checkbox_real_bank_balance")
        self.checkbox_real_return = cfg_widget.findChild(QFrame, "checkbox_real_return")
        self.checkbox_real_return_perc = cfg_widget.findChild(QFrame, "checkbox_real_return_perc")

        self.main_widget.layout().addWidget(chart_cfg_frame)

        # effects

        chart_cfg_frame.setGraphicsEffect(self.card_shadow())

    def recalculate_columns(self):
        self.calculate_total_contribution()

        for n in range(self.model.rowCount()):
            v = self.model.record(n).value("total_contribution")

            dv = self.model.record(n).value("bank_balance") - v

            rec = self.model.record(n)

            rec.setGenerated("gross_return", True)
            rec.setGenerated("gross_return_perc", True)

            rec.setValue("gross_return", float(dv))

            if v > 0:
                rec.setValue("gross_return_perc", float(100 * dv / v))

            self.model.setRecord(n, rec)

    def calculate_total_contribution(self):
        list_v = []

        for n in range(self.model.rowCount()):
            list_v.append(self.model.record(n).value("contribution"))

        if len(list_v) > 0:
            list_v.reverse()

            cum_v = np.cumsum(np.array([list_v]))

            cum_v = cum_v[::-1]

            for n in range(self.model.rowCount()):
                rec = self.model.record(n)

                rec.setGenerated("total_contribution", True)

                rec.setValue("total_contribution", float(cum_v[n]))

                self.model.setRecord(n, rec)

    def show_chart(self):
        self.clear_chart()

        self.chart.setTitle(self.name)

        # series0 = QtCharts.QLineSeries()
        # series1 = QtCharts.QLineSeries()

        # series0.setName("Monthly Value")
        # series1.setName("Accumulated")

        # series0.hovered.connect(self.on_hover)
        # series1.hovered.connect(self.on_hover)

        # vmin, vmax = 0, 0

        # for n in range(self.model.rowCount()):
        #     qdt = QDateTime.fromString(self.model.record(n).value("date"), "dd/MM/yyyy")

        #     epoch_in_ms = qdt.toMSecsSinceEpoch()

        #     v = self.model.record(n).value("value")
        #     accu = self.model.record(n).value("accumulated")

        #     vmin = min(vmin, v)
        #     vmin = min(vmin, accu)

        #     vmax = max(vmax, v)
        #     vmax = max(vmax, accu)

        #     series0.append(epoch_in_ms, v)
        #     series1.append(epoch_in_ms, accu)

        # self.chart.addSeries(series0)
        # self.chart.addSeries(series1)

        # axis_x = QtCharts.QDateTimeAxis()
        # axis_x.setTitleText("Date")
        # axis_x.setFormat("dd/MM/yyyy")
        # axis_x.setLabelsAngle(-10)

        # axis_y = QtCharts.QValueAxis()
        # axis_y.setTitleText("%")
        # # axis_y.setLabelFormat("%.1f")
        # axis_y.setRange(1.01 * vmin, 1.01 * vmax)

        # self.chart.addAxis(axis_x, Qt.AlignBottom)
        # self.chart.addAxis(axis_y, Qt.AlignLeft)

        # series0.attachAxis(axis_x)
        # series0.attachAxis(axis_y)

        # series1.attachAxis(axis_x)
        # series1.attachAxis(axis_y)
