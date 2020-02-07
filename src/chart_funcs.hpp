#ifndef CHART_FUNCS_HPP
#define CHART_FUNCS_HPP

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QtCharts>
#include <tuple>
#include "model.hpp"
#include "table_fund.hpp"

void clear_chart(QChart* chart);

void add_axes_to_chart(QChart* chart, const QString& ytitle);

auto add_series_to_chart(QChart* chart, const Model* tmodel, const QString& series_name, const QString& column_name)
    -> QLineSeries*;

auto add_series_to_chart(QChart* chart,
                         const QVector<int>& dates,
                         const QVector<double>& values,
                         const QString& series_name) -> QLineSeries*;

auto add_tables_barseries_to_chart(QChart* chart,
                                   const QVector<TableFund const*>& tables,
                                   const QVector<int>& list_dates,
                                   const QString& series_name,
                                   const QString& column_name)
    -> std::tuple<QStackedBarSeries*, QVector<QBarSet*>, QStringList>;

auto get_unique_months_from_db(const QSqlDatabase& db,
                               const QVector<TableFund const*>& tables,
                               const int& last_n_months) -> QVector<int>;

#endif