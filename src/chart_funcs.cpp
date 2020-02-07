#include "chart_funcs.hpp"
#include <QSqlError>
#include <QSqlQuery>
#include "qdatetime.h"
#include "qdatetimeaxis.h"
#include "qnamespace.h"

void clear_chart(QChart* chart) {
  chart->removeAllSeries();

  for (auto& axis : chart->axes()) {
    chart->removeAxis(axis);
  }
}

void add_axes_to_chart(QChart* chart, const QString& ytitle) {
  const auto axis_x = new QDateTimeAxis();

  const QFont serif_font("Sans");

  axis_x->setTitleText("Date");
  axis_x->setFormat("MM/yyyy");
  axis_x->setLabelsAngle(-10);
  axis_x->setTitleFont(serif_font);

  const auto axis_y = new QValueAxis();

  axis_y->setTitleText(ytitle);
  axis_y->setLabelFormat("%.2f");
  axis_y->setTitleFont(serif_font);

  chart->addAxis(axis_x, Qt::AlignBottom);
  chart->addAxis(axis_y, Qt::AlignLeft);
}

auto add_series_to_chart(QChart* chart, const Model* tmodel, const QString& series_name, const QString& column_name)
    -> QLineSeries* {
  const auto series = new QLineSeries();

  series->setName(series_name.toLower());

  double ymin = dynamic_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0])->min();
  double ymax = dynamic_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0])->max();
  qint64 xmin = dynamic_cast<QDateTimeAxis*>(chart->axes(Qt::Horizontal)[0])->min().toMSecsSinceEpoch();
  qint64 xmax = dynamic_cast<QDateTimeAxis*>(chart->axes(Qt::Horizontal)[0])->max().toMSecsSinceEpoch();

  for (int n = 0; n < tmodel->rowCount(); n++) {
    const auto qdt = QDateTime::fromString(tmodel->record(n).value("date").toString(), "MM/yyyy");

    const auto epoch_in_ms = qdt.toMSecsSinceEpoch();

    const double v = tmodel->record(n).value(column_name).toDouble();

    if (!chart->series().empty()) {
      ymin = std::min(ymin, v);
      ymax = std::max(ymax, v);
      xmin = std::min(xmin, epoch_in_ms);
      xmax = std::max(xmax, epoch_in_ms);
    } else {
      if (n == 0) {
        ymin = v;
        ymax = v;
        xmin = epoch_in_ms;
        xmax = epoch_in_ms;
      } else {
        ymin = std::min(ymin, v);
        ymax = std::max(ymax, v);
        xmin = std::min(xmin, epoch_in_ms);
        xmax = std::max(xmax, epoch_in_ms);
      }
    }

    series->append(epoch_in_ms, v);
  }

  chart->addSeries(series);

  series->attachAxis(chart->axes(Qt::Horizontal)[0]);
  series->attachAxis(chart->axes(Qt::Vertical)[0]);

  chart->axes(Qt::Vertical)[0]->setRange(ymin - 0.05 * fabs(ymin), ymax + 0.05 * fabs(ymax));
  chart->axes(Qt::Horizontal)[0]->setRange(QDateTime::fromMSecsSinceEpoch(xmin), QDateTime::fromMSecsSinceEpoch(xmax));

  return series;
}

auto add_series_to_chart(QChart* chart,
                         const QVector<int>& dates,
                         const QVector<double>& values,
                         const QString& series_name) -> QLineSeries* {
  const auto series = new QLineSeries();

  series->setName(series_name.toLower());

  double ymin = dynamic_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0])->min();
  double ymax = dynamic_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0])->max();
  qint64 xmin = dynamic_cast<QDateTimeAxis*>(chart->axes(Qt::Horizontal)[0])->min().toMSecsSinceEpoch();
  qint64 xmax = dynamic_cast<QDateTimeAxis*>(chart->axes(Qt::Horizontal)[0])->max().toMSecsSinceEpoch();

  for (int n = 0; n < dates.size(); n++) {
    const double v = values[n];
    const qint64 d = static_cast<qint64>(dates[n]) * 1000;

    if (!chart->series().empty()) {
      ymin = std::min(ymin, v);
      ymax = std::max(ymax, v);
      xmin = std::min(xmin, d);
      xmax = std::max(xmax, d);
    } else {
      if (n == 0) {
        ymin = v;
        ymax = v;
        xmin = d;
        xmax = d;
      } else {
        ymin = std::min(ymin, v);
        ymax = std::max(ymax, v);
        xmin = std::min(xmin, d);
        xmax = std::max(xmax, d);
      }
    }

    series->append(d, v);
  }

  chart->addSeries(series);

  series->attachAxis(chart->axes(Qt::Horizontal)[0]);
  series->attachAxis(chart->axes(Qt::Vertical)[0]);

  chart->axes(Qt::Vertical)[0]->setRange(ymin - 0.05 * fabs(ymin), ymax + 0.05 * fabs(ymax));
  chart->axes(Qt::Horizontal)[0]->setRange(QDateTime::fromMSecsSinceEpoch(xmin), QDateTime::fromMSecsSinceEpoch(xmax));

  return series;
}

auto add_tables_barseries_to_chart(QChart* chart,
                                   const QVector<TableFund const*>& tables,
                                   const QVector<int>& list_dates,
                                   const QString& series_name,
                                   const QString& column_name)
    -> std::tuple<QStackedBarSeries*, QVector<QBarSet*>, QStringList> {
  QVector<QBarSet*> barsets;

  for (auto& table : tables) {
    barsets.append(new QBarSet(table->name));
  }

  QStringList categories;

  for (auto& date : list_dates) {
    const auto qdt = QDateTime::fromSecsSinceEpoch(date);

    const QString date_month = qdt.toString("MM/yyyy");

    categories.append(date_month);

    for (int m = 0; m < tables.size(); m++) {
      bool has_date = false;

      for (int n = tables[m]->model->rowCount() - 1; n >= 0; n--) {
        const QString tdate = tables[m]->model->record(n).value("date").toString();

        if (date_month == tdate) {
          barsets[m]->append(tables[m]->model->record(n).value(column_name).toDouble());

          has_date = true;

          break;
        }
      }

      if (!has_date) {
        barsets[m]->append(0.0);
      }
    }
  }

  const auto series = new QStackedBarSeries();

  for (auto& bs : barsets) {
    series->append(bs);
  }

  const QFont serif_font("Sans");

  chart->setTitle(series_name);
  chart->legend()->setAlignment(Qt::AlignRight);
  chart->legend()->show();

  chart->addSeries(series);

  const auto axis_x = new QBarCategoryAxis();

  axis_x->append(categories);

  chart->addAxis(axis_x, Qt::AlignBottom);

  series->attachAxis(axis_x);

  const auto axis_y = new QValueAxis();

  axis_y->setTitleText(QLocale().currencySymbol());
  axis_y->setTitleFont(serif_font);
  axis_y->setLabelFormat("%.2f");

  chart->addAxis(axis_y, Qt::AlignLeft);

  series->attachAxis(axis_y);

  return {series, barsets, categories};
}

auto get_unique_months_from_db(const QSqlDatabase& db,
                               const QVector<TableFund const*>& tables,
                               const int& last_n_months) -> QVector<int> {
  QSet<int> set;

  for (auto& table : tables) {
    if (set.size() == last_n_months) {
      break;
    }

    // making sure all the latest data was saved to the database

    table->model->submitAll();

    auto query = QSqlQuery(db);

    query.prepare("select distinct date from " + table->name + " order by date desc");

    if (query.exec()) {
      while (query.next() && set.size() < last_n_months) {
        set.insert(query.value(0).toInt());
      }
    } else {
      qDebug() << table->model->lastError().text().toUtf8();
    }
  }

  QVector<int> list = QVector<int>::fromList(set.values());

  std::sort(list.begin(), list.end());

  return list;
}