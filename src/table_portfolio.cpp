#include "table_portfolio.hpp"
#include <QSqlError>
#include <QSqlQuery>
#include "chart_funcs.hpp"

TablePortfolio::TablePortfolio(QWidget* parent) {
  type = TableType::Portfolio;

  name = "portfolio";

  fund_cfg_frame->hide();
  button_add_row->hide();

  connect(spinbox_months, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
    clear_chart(chart2);
    make_chart2();
  });
}

void TablePortfolio::init_model() {
  model->setTable(name);
  model->setEditStrategy(QSqlTableModel::OnManualSubmit);
  model->setSort(1, Qt::DescendingOrder);

  auto currency = QLocale().currencySymbol();

  model->setHeaderData(1, Qt::Horizontal, "Date");
  model->setHeaderData(2, Qt::Horizontal, "Deposit " + currency);
  model->setHeaderData(3, Qt::Horizontal, "Withdrawal " + currency);
  model->setHeaderData(4, Qt::Horizontal, "Starting\n Balance " + currency);
  model->setHeaderData(5, Qt::Horizontal, "Ending\n Balance " + currency);
  model->setHeaderData(6, Qt::Horizontal, "Accumulated\nDeposits " + currency);
  model->setHeaderData(7, Qt::Horizontal, "Accumulated\nWithdrawals " + currency);
  model->setHeaderData(8, Qt::Horizontal, "Net\nDeposit " + currency);
  model->setHeaderData(9, Qt::Horizontal, "Net\nBalance " + currency);
  model->setHeaderData(10, Qt::Horizontal, "Net\nReturn " + currency);
  model->setHeaderData(11, Qt::Horizontal, "Net\nReturn %");
  model->setHeaderData(12, Qt::Horizontal, "Accumulated\nNet Return " + currency);
  model->setHeaderData(13, Qt::Horizontal, "Accumulated\nNet Return %");
  model->setHeaderData(14, Qt::Horizontal, "Real\nReturn %");
  model->setHeaderData(15, Qt::Horizontal, "Accumulated\nReal Return %");

  model->select();

  table_view->setModel(model);
  table_view->setColumnHidden(0, true);
}

void TablePortfolio::process_fund_tables(const QVector<TableFund*>& tables) {
  // get date values in each investment tables

  QSet<int> list_set;

  for (auto& table : tables) {
    // making sure all the latest data was saved to the database

    table->model->submitAll();

    auto query = QSqlQuery(db);

    query.prepare("select distinct date from " + table->name + " order by date");

    if (query.exec()) {
      while (query.next()) {
        list_set.insert(query.value(0).toInt());
      }
    } else {
      qDebug(table->model->lastError().text().toUtf8());
    }
  }

  if (list_set.size() == 0) {
    return;
  }

  QList<int> list_dates = list_set.values();

  std::sort(list_dates.begin(), list_dates.end());

  // calculate columns

  auto qdt = QDateTime();

  for (auto& date : list_dates) {
    double deposit = 0.0, withdrawal = 0.0, starting_balance = 0.0, ending_balance = 0.0, accumulated_deposit = 0.0,
           accumulated_withdrawal = 0.0, net_deposit = 0.0, net_balance = 0.0, net_return = 0.0,
           accumulated_net_return = 0.0;

    qdt.setSecsSinceEpoch(date);

    QString date_month = qdt.toString("MM/yyyy");

    for (auto& table : tables) {
      for (int n = 0; n < table->model->rowCount(); n++) {
        QString tdate = table->model->record(n).value("date").toString();

        if (date_month == QDate::fromString(tdate, "dd/MM/yyyy").toString("MM/yyyy")) {
          deposit += table->model->record(n).value("deposit").toDouble();
          withdrawal += table->model->record(n).value("withdrawal").toDouble();
          starting_balance += table->model->record(n).value("starting_balance").toDouble();
          ending_balance += table->model->record(n).value("ending_balance").toDouble();
          accumulated_deposit += table->model->record(n).value("accumulated_deposit").toDouble();
          accumulated_withdrawal += table->model->record(n).value("accumulated_withdrawal").toDouble();
          net_deposit += table->model->record(n).value("net_deposit").toDouble();
          net_balance += table->model->record(n).value("net_balance").toDouble();
          net_return += table->model->record(n).value("net_return").toDouble();
          accumulated_net_return += table->model->record(n).value("accumulated_net_return").toDouble();

          break;
        }
      }
    }

    double net_return_perc = 100 * net_return / (starting_balance + deposit - withdrawal);

    double real_return_perc = net_return_perc;

    // get inflation values so we can update real_return_perc

    QVector<int> inflation_dates;
    QVector<double> inflation_values, inflation_accumulated;

    auto query = QSqlQuery(db);

    query.prepare("select distinct date,value,accumulated from inflation order by date");

    if (query.exec()) {
      while (query.next()) {
        inflation_dates.append(query.value(0).toInt());
        inflation_values.append(query.value(1).toDouble());
        inflation_accumulated.append(query.value(2).toDouble());
      }
    } else {
      qDebug("Failed to get inflation table values!");
    }

    for (int i = 0; i < inflation_dates.size(); i++) {
      qdt.setSecsSinceEpoch(inflation_dates[i]);

      if (qdt.toString("MM/yyyy") == date_month) {
        real_return_perc = 100.0 * (net_return_perc - inflation_values[i]) / (100.0 + inflation_values[i]);

        break;
      }
    }

    double accumulated_net_return_perc = 0, accumulated_real_return_perc = 0;

    query = QSqlQuery(db);

    query.prepare("insert or replace into " + name + " values ((select id from " + name +
                  " where date == ?),?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");

    query.addBindValue(date);
    query.addBindValue(date);
    query.addBindValue(deposit);
    query.addBindValue(withdrawal);
    query.addBindValue(starting_balance);
    query.addBindValue(ending_balance);
    query.addBindValue(accumulated_deposit);
    query.addBindValue(accumulated_withdrawal);
    query.addBindValue(net_deposit);
    query.addBindValue(net_balance);
    query.addBindValue(net_return);
    query.addBindValue(net_return_perc);
    query.addBindValue(accumulated_net_return);
    query.addBindValue(accumulated_net_return_perc);
    query.addBindValue(real_return_perc);
    query.addBindValue(accumulated_real_return_perc);

    if (!query.exec()) {
      qDebug(model->lastError().text().toUtf8());
    }
  }

  model->select();

  if (model->rowCount() > 0) {
    calculate_accumulated_sum("net_return");
    calculate_accumulated_product("net_return_perc");
    calculate_accumulated_product("real_return_perc");

    clear_charts();

    make_chart1();
    make_chart2();
  }
}

void TablePortfolio::make_chart1() {
  chart1->setTitle(name.toUpper());

  add_axes_to_chart(chart1, QLocale().currencySymbol());

  auto s1 = add_series_to_chart(chart1, model, "Net Deposit", "net_deposit");
  auto s2 = add_series_to_chart(chart1, model, "Net Balance", "net_balance");
  auto s3 = add_series_to_chart(chart1, model, "Net Return", "accumulated_net_return");

  connect(s1, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout1, s1->name()); });
  connect(s2, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout1, s2->name()); });
  connect(s3, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout1, s3->name()); });
}

void TablePortfolio::make_chart2() {
  chart2->setTitle(name.toUpper());

  add_axes_to_chart(chart2, "%");

  QVector<int> dates;
  QVector<double> net_return, real_return, accumulated_net_return, accumulated_real_return;

  auto query = QSqlQuery(db);

  query.prepare("select distinct date,net_return_perc,real_return_perc from " + name + " order by date desc");

  if (query.exec()) {
    while (query.next() && dates.size() < spinbox_months->value()) {
      dates.append(query.value(0).toInt());
      net_return.append(query.value(1).toDouble());
      real_return.append(query.value(2).toDouble());
    }
  }

  if (dates.size() == 0) {
    return;
  }

  perc_chart_oldest_date = dates[dates.size() - 1];

  std::reverse(dates.begin(), dates.end());
  std::reverse(net_return.begin(), net_return.end());
  std::reverse(real_return.begin(), real_return.end());

  for (int n = 0; n < dates.size(); n++) {
    net_return[n] = net_return[n] * 0.01 + 1.0;
    real_return[n] = real_return[n] * 0.01 + 1.0;
  }

  // cumulative product

  accumulated_net_return.resize(net_return.size());
  accumulated_real_return.resize(real_return.size());

  std::partial_sum(net_return.begin(), net_return.end(), accumulated_net_return.begin(), std::multiplies<double>());
  std::partial_sum(real_return.begin(), real_return.end(), accumulated_real_return.begin(), std::multiplies<double>());

  for (int n = 0; n < dates.size(); n++) {
    accumulated_net_return[n] = (accumulated_net_return[n] - 1.0) * 100;
    accumulated_real_return[n] = (accumulated_real_return[n] - 1.0) * 100;
  }

  auto s1 = add_series_to_chart(chart2, dates, accumulated_net_return, "Net Return");

  connect(s1, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout2, s1->name()); });

  auto s2 = add_series_to_chart(chart2, dates, accumulated_real_return, "Real Return");

  connect(s2, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout2, s2->name()); });

  // ask the main window class for the benchmarks

  emit getBenchmarkTables();
}

void TablePortfolio::show_benchmark(const TableBenchmarks* btable) {
  auto [dates, values, accumulated] = process_benchmark(btable->name, perc_chart_oldest_date);

  if (chart2->axes().size() == 0) {
    return;
  }

  auto series = new QLineSeries();

  series->setName(btable->name.toLower());

  connect(series, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout2, series->name()); });

  double vmin = static_cast<QValueAxis*>(chart2->axes(Qt::Vertical)[0])->min();
  double vmax = static_cast<QValueAxis*>(chart2->axes(Qt::Vertical)[0])->max();

  for (int n = 0; n < dates.size(); n++) {
    auto qdt = QDateTime::fromSecsSinceEpoch(dates[n]);

    auto epoch_in_ms = qdt.toMSecsSinceEpoch();

    double v = accumulated[n];

    if (chart2->series().size() > 0) {
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

  chart2->addSeries(series);

  series->attachAxis(chart2->axes(Qt::Horizontal)[0]);
  series->attachAxis(chart2->axes(Qt::Vertical)[0]);

  chart2->axes(Qt::Vertical)[0]->setRange(vmin - 0.05 * fabs(vmin), vmax + 0.05 * fabs(vmax));
}

std::tuple<QVector<int>, QVector<double>, QVector<double>> TablePortfolio::process_benchmark(
    const QString& table_name,
    const int& oldest_date) const {
  QVector<int> dates;
  QVector<double> values, accu;

  auto query = QSqlQuery(db);

  query.prepare("select distinct date,value from " + table_name + " where date >= ? order by date");

  query.addBindValue(oldest_date);

  if (query.exec()) {
    while (query.next()) {
      dates.append(query.value(0).toInt());
      values.append(query.value(1).toDouble());
    }
  } else {
    qDebug("Failed to get inflation table values!");
  }

  for (auto& v : values) {
    v = v * 0.01 + 1.0;
  }

  // cumulative product

  accu.resize(values.size());

  std::partial_sum(values.begin(), values.end(), accu.begin(), std::multiplies<double>());

  for (auto& v : accu) {
    v = (v - 1.0) * 100;
  }

  for (auto& v : values) {
    v = (v - 1.0) * 100;
  }

  return {dates, values, accu};
}
