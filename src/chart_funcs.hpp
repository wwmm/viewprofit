#ifndef CHART_FUNCS_HPP
#define CHART_FUNCS_HPP

#include <QSqlRecord>
#include <QtCharts>
#include "model.hpp"

void clear_chart(QChart* chart);

void add_axes_to_chart(QChart* chart, const QString& ytitle);

QLineSeries* add_series_to_chart(QChart* chart,
                                 const Model* tmodel,
                                 const QString& series_name,
                                 const QString& column_name);

#endif