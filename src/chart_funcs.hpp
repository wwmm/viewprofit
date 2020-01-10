#ifndef CHART_FUNCS_HPP
#define CHART_FUNCS_HPP

#include <QSqlRecord>
#include <QtCharts>
#include "model.hpp"

void clear_chart(QChart* chart);

template <typename T>
void add_axes_to_chart(T* chart, const QString& ytitle) {
  auto axis_x = new QDateTimeAxis();

  QFont serif_font("Sans");

  axis_x->setTitleText("Date");
  axis_x->setFormat("dd/MM/yyyy");
  axis_x->setLabelsAngle(-10);
  axis_x->setTitleFont(serif_font);

  auto axis_y = new QValueAxis();

  axis_y->setTitleText(ytitle);
  axis_y->setLabelFormat("%.2f");
  axis_y->setTitleFont(serif_font);

  chart->addAxis(axis_x, Qt::AlignBottom);
  chart->addAxis(axis_y, Qt::AlignLeft);
}

template <typename T>
QLineSeries* add_series_to_chart(T* chart,
                                 const Model* tmodel,
                                 const QString& series_name,
                                 const QString& column_name) {
  auto series = new QLineSeries();

  series->setName(series_name.toLower());

  double vmin = static_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0])->min();
  double vmax = static_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0])->max();

  for (int n = 0; n < tmodel->rowCount(); n++) {
    auto qdt = QDateTime::fromString(tmodel->record(n).value("date").toString(), "dd/MM/yyyy");

    auto epoch_in_ms = qdt.toMSecsSinceEpoch();

    double v = tmodel->record(n).value(column_name).toDouble();

    if (chart->series().size() > 0) {
      vmin = std::min(vmin, v);
      vmax = std::max(vmax, v);
    } else {
      if (n == 0) {
        vmin = v;
        vmax = v;
      } else {
        vmin = std::min(vmin, v);
        vmax = std::max(vmax, v);
      }
    }

    series->append(epoch_in_ms, v);
  }

  chart->addSeries(series);

  series->attachAxis(chart->axes(Qt::Horizontal)[0]);
  series->attachAxis(chart->axes(Qt::Vertical)[0]);

  chart->axes(Qt::Vertical)[0]->setRange(vmin - 0.05 * fabs(vmin), vmax + 0.05 * fabs(vmax));

  return series;
}

#endif