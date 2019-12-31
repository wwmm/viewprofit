# -*- coding: utf-8 -*-


import numpy as np
from PySide2.QtCharts import QtCharts
from PySide2.QtCore import QDateTime, QLocale, Qt, Signal
from PySide2.QtWidgets import QCheckBox, QFrame

from ViewProfit.model_investment import ModelInvestment
from ViewProfit.table_base import TableBase


class TableInvestment(TableBase):
    new_mouse_coords = Signal(object,)

    def __init__(self, db, chart1, chart2):
        TableBase.__init__(self, db, chart1, chart2)

        self.model = ModelInvestment(self, db)

        self.table_view.setModel(self.model)

        cfg_widget = self.loader.load(self.module_path + "/ui/investment_chart_cfg.ui")

        chart_cfg_frame = cfg_widget.findChild(QFrame, "chart_cfg_frame")
        self.checbox_total_contribution = cfg_widget.findChild(QCheckBox, "checbox_total_contribution")
        self.checkbox_real_bank_balance = cfg_widget.findChild(QCheckBox, "checkbox_real_bank_balance")
        self.checkbox_real_return = cfg_widget.findChild(QCheckBox, "checkbox_real_return")
        self.checkbox_real_return_perc = cfg_widget.findChild(QCheckBox, "checkbox_real_return_perc")

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

    def make_chart1(self):
        self.chart1.setTitle(self.name)

        series0 = QtCharts.QLineSeries()

        series0.setName("Total Contribution")

        series0.hovered.connect(self.on_hover)

        for n in range(self.model.rowCount()):
            qdt = QDateTime.fromString(self.model.record(n).value("date"), "dd/MM/yyyy")

            epoch_in_ms = qdt.toMSecsSinceEpoch()

            total_contribution = self.model.record(n).value("total_contribution")
            # real_bank_balance = self.model.record(n).value("real_bank_balance")

            series0.append(epoch_in_ms, total_contribution)
            # series1.append(epoch_in_ms, real_bank_balance)

        self.chart1.addSeries(series0)

        axis_x = QtCharts.QDateTimeAxis()
        axis_x.setTitleText("Date")
        axis_x.setFormat("dd/MM/yyyy")
        axis_x.setLabelsAngle(-10)

        axis_y = QtCharts.QValueAxis()
        axis_y.setTitleText(QLocale().currencySymbol())
        axis_y.setLabelFormat("%.2f")
        # axis_y.setRange(1.01 * vmin, 1.01 * vmax)

        self.chart1.addAxis(axis_x, Qt.AlignBottom)
        self.chart1.addAxis(axis_y, Qt.AlignLeft)

        series0.attachAxis(axis_x)
        series0.attachAxis(axis_y)

    def show_chart(self):
        self.clear_charts()

        self.make_chart1()
