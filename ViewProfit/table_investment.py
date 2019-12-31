# -*- coding: utf-8 -*-


import numpy as np
from PySide2.QtCharts import QtCharts
from PySide2.QtCore import QDateTime, QLocale, QSettings, Qt, Signal
from PySide2.QtWidgets import QCheckBox, QDoubleSpinBox, QFrame

from ViewProfit.model_investment import ModelInvestment
from ViewProfit.table_base import TableBase


class TableInvestment(TableBase):
    new_mouse_coords = Signal(object,)

    def __init__(self, name, db, chart1, chart2):
        TableBase.__init__(self, name, db, chart1, chart2)

        self.model = ModelInvestment(self, db)

        self.table_view.setModel(self.model)

        self.qsettings = QSettings()

        cfg_widget = self.loader.load(self.module_path + "/ui/investment_chart_cfg.ui")

        chart_cfg_frame = cfg_widget.findChild(QFrame, "chart_cfg_frame")
        self.doublespinbox_income_tax = cfg_widget.findChild(QDoubleSpinBox, "doublespinbox_income_tax")

        self.main_widget.layout().addWidget(chart_cfg_frame)

        # effects

        chart_cfg_frame.setGraphicsEffect(self.card_shadow())

        # read qsettings values and initialize the interface

        self.qsettings.beginGroup(self.name)

        self.doublespinbox_income_tax.setValue(float(self.qsettings.value("income_tax", 0.0)))

        self.qsettings.endGroup()

        # signals

        self.doublespinbox_income_tax.valueChanged.connect(self.on_income_tax_changed)

    def recalculate_columns(self):
        self.calculate_total_contribution()

        self.qsettings.beginGroup(self.name)

        income_tax = float(self.qsettings.value("income_tax", 0.0))

        self.qsettings.endGroup()

        for n in range(self.model.rowCount()):
            total_contribution = self.model.record(n).value("total_contribution")

            gross_return = self.model.record(n).value("bank_balance") - total_contribution

            real_return = gross_return * (1.0 - 0.01 * income_tax)

            rec = self.model.record(n)

            rec.setGenerated("gross_return", True)
            rec.setGenerated("gross_return_perc", True)
            rec.setGenerated("real_return", True)
            rec.setGenerated("real_return_perc", True)

            rec.setValue("gross_return", float(gross_return))
            rec.setValue("real_return", float(real_return))

            if total_contribution > 0:
                rec.setValue("gross_return_perc", float(100 * gross_return / total_contribution))
                rec.setValue("real_return_perc", float(100 * real_return / total_contribution))

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

    def on_income_tax_changed(self, value):
        self.qsettings.beginGroup(self.name)

        self.qsettings.setValue("income_tax", value)

        self.qsettings.endGroup()

        self.qsettings.sync()
