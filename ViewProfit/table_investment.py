# -*- coding: utf-8 -*-


import numpy as np
from PySide2.QtCharts import QtCharts
from PySide2.QtCore import QDateTime, QLocale, QSettings, Qt, Signal
from PySide2.QtSql import QSqlTableModel
from PySide2.QtWidgets import QDoubleSpinBox, QFrame

from ViewProfit.model_investment import ModelInvestment
from ViewProfit.table_base_ib import TableBaseIB


class TableInvestment(TableBaseIB):
    new_mouse_coords = Signal(object,)

    def __init__(self, app, name):
        TableBaseIB.__init__(self, app, name)

        self.model = ModelInvestment(self, app.db)

        self.init_model()

        self.qsettings = QSettings()

        cfg_widget = self.loader.load(self.module_path + "/ui/investment_chart_cfg.ui")

        chart_cfg_frame = cfg_widget.findChild(QFrame, "chart_cfg_frame")
        self.doublespinbox_income_tax = cfg_widget.findChild(QDoubleSpinBox, "doublespinbox_income_tax")

        self.main_widget.layout().addWidget(chart_cfg_frame)

        self.lineedit_name.setText(name)

        # effects

        chart_cfg_frame.setGraphicsEffect(self.card_shadow())

        # read qsettings values and initialize the interface

        self.qsettings.beginGroup(self.name)

        self.doublespinbox_income_tax.setValue(float(self.qsettings.value("income_tax", 0.0)))

        self.qsettings.endGroup()

        # signals

        self.doublespinbox_income_tax.valueChanged.connect(self.on_income_tax_changed)

    def init_model(self):
        self.model.setTable(self.name)
        self.model.setEditStrategy(QSqlTableModel.OnManualSubmit)
        self.model.setSort(1, Qt.SortOrder.DescendingOrder)

        currency = QLocale().currencySymbol()

        self.model.setHeaderData(1, Qt.Horizontal, "Date")
        self.model.setHeaderData(2, Qt.Horizontal, "Contribution " + currency)
        self.model.setHeaderData(3, Qt.Horizontal, "Bank Balance " + currency)
        self.model.setHeaderData(4, Qt.Horizontal, "Total Contribution " + currency)
        self.model.setHeaderData(5, Qt.Horizontal, "Gross Return " + currency)
        self.model.setHeaderData(6, Qt.Horizontal, "Gross Return %")
        self.model.setHeaderData(7, Qt.Horizontal, "Real Return " + currency)
        self.model.setHeaderData(8, Qt.Horizontal, "Real Return %")
        self.model.setHeaderData(9, Qt.Horizontal, "Real Bank Balance " + currency)

        self.model.select()

        self.table_view.setModel(self.model)
        self.table_view.setColumnHidden(0, True)  # do no show the id column

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
            rec.setGenerated("real_bank_balance", True)

            rec.setValue("gross_return", float(gross_return))
            rec.setValue("real_return", float(real_return))
            rec.setValue("real_bank_balance", float(total_contribution + real_return))

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

    def on_income_tax_changed(self, value):
        self.qsettings.beginGroup(self.name)

        self.qsettings.setValue("income_tax", value)

        self.qsettings.endGroup()

        self.qsettings.sync()
