#include "chart_funcs.hpp"
#include <QSqlError>
#include <QSqlQuery>

void clear_chart(QChart* chart) {
  chart->removeAllSeries();

  for (auto& axis : chart->axes()) {
    chart->removeAxis(axis);
  }
}

void add_axes_to_chart(QChart* chart, const QString& ytitle) {
  auto axis_x = new QDateTimeAxis();

  QFont serif_font("Sans");

  axis_x->setTitleText("Date");
  axis_x->setFormat("MM/yyyy");
  axis_x->setLabelsAngle(-10);
  axis_x->setTitleFont(serif_font);

  auto axis_y = new QValueAxis();

  axis_y->setTitleText(ytitle);
  axis_y->setLabelFormat("%.2f");
  axis_y->setTitleFont(serif_font);

  chart->addAxis(axis_x, Qt::AlignBottom);
  chart->addAxis(axis_y, Qt::AlignLeft);
}

auto add_series_to_chart(QChart* chart, const Model* tmodel, const QString& series_name, const QString& column_name)
    -> QLineSeries* {
  auto series = new QLineSeries();

  series->setName(series_name.toLower());

  double vmin = dynamic_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0])->min();
  double vmax = dynamic_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0])->max();

  for (int n = 0; n < tmodel->rowCount(); n++) {
    auto qdt = QDateTime::fromString(tmodel->record(n).value("date").toString(), "MM/yyyy");

    auto epoch_in_ms = qdt.toMSecsSinceEpoch();

    double v = tmodel->record(n).value(column_name).toDouble();

    if (!chart->series().empty()) {
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

auto add_series_to_chart(QChart* chart,
                         const QVector<int>& dates,
                         const QVector<double>& values,
                         const QString& series_name) -> QLineSeries* {
  auto series = new QLineSeries();

  series->setName(series_name.toLower());

  double vmin = dynamic_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0])->min();
  double vmax = dynamic_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0])->max();

  for (int n = 0; n < dates.size(); n++) {
    double v = values[n];

    if (!chart->series().empty()) {
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

    series->append((long long)(dates[n]) * 1000, v);
  }

  chart->addSeries(series);

  series->attachAxis(chart->axes(Qt::Horizontal)[0]);
  series->attachAxis(chart->axes(Qt::Vertical)[0]);

  chart->axes(Qt::Vertical)[0]->setRange(vmin - 0.05 * fabs(vmin), vmax + 0.05 * fabs(vmax));

  return series;
}

auto add_tables_barseries_to_chart(QChart* chart,
                                   const QVector<TableFund*>& tables,
                                   const QVector<int>& list_dates,
                                   const QString& series_name,
                                   const QString& column_name)
    -> std::tuple<QStackedBarSeries*, QVector<QBarSet*>, QStringList> {
  QVector<QBarSet*> barsets;

  for (auto& table : tables) {
    barsets.append(new QBarSet(table->name));
  }

  auto qdt = QDateTime();

  QStringList categories;

  for (auto& date : list_dates) {
    qdt.setSecsSinceEpoch(date);

    QString date_month = qdt.toString("MM/yyyy");

    categories.append(date_month);

    for (int m = 0; m < tables.size(); m++) {
      bool has_date = false;

      for (int n = tables[m]->model->rowCount() - 1; n >= 0; n--) {
        QString tdate = tables[m]->model->record(n).value("date").toString();

        if (date_month == tdate) {
          double v = tables[m]->model->record(n).value(column_name).toDouble();

          barsets[m]->append(v);

          has_date = true;

          break;
        }
      }

      if (!has_date) {
        barsets[m]->append(0.0);
      }
    }
  }

  auto series = new QStackedBarSeries();

  for (auto& bs : barsets) {
    series->append(bs);
  }

  QFont serif_font("Sans");

  chart->setTitle(series_name);
  chart->legend()->setAlignment(Qt::AlignRight);
  chart->legend()->show();

  chart->addSeries(series);

  auto axis_x = new QBarCategoryAxis();

  axis_x->append(categories);

  chart->addAxis(axis_x, Qt::AlignBottom);

  series->attachAxis(axis_x);

  auto axis_y = new QValueAxis();

  axis_y->setTitleText(QLocale().currencySymbol());
  axis_y->setTitleFont(serif_font);
  axis_y->setLabelFormat("%.2f");

  chart->addAxis(axis_y, Qt::AlignLeft);

  series->attachAxis(axis_y);

  return {series, barsets, categories};
}

auto get_unique_months_from_db(const QSqlDatabase& db, const QVector<TableFund*>& tables, const int& last_n_months)
    -> QVector<int> {
  QSet<int> set;
  auto qdt = QDateTime();

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