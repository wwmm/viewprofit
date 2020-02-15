#include "table_fund.hpp"
#include <QSqlQuery>
#include "chart_funcs.hpp"
#include "effects.hpp"

TableFund::TableFund(QWidget* parent) : TableBase(parent) {
  type = TableType::Investment;

  // shadow effects

  fund_cfg_frame->setGraphicsEffect(card_shadow());

  // signals

  connect(spinbox_months, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
    clear_chart(chart2);
    make_chart2();
  });
}

void TableFund::init_model() {
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

  // initializing widgets with qsettings values

  doublespinbox_income_tax->disconnect();

  qsettings.beginGroup(name);

  doublespinbox_income_tax->setValue(qsettings.value("income_tax", 0.0).toDouble());

  qsettings.endGroup();

  connect(doublespinbox_income_tax, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value) {
    qsettings.beginGroup(name);

    qsettings.setValue("income_tax", value);

    qsettings.endGroup();

    qsettings.sync();
  });
}

auto TableFund::process_benchmark(const QString& table_name, const int& oldest_date) const
    -> std::tuple<QVector<int>, QVector<double>, QVector<double>> {
  QVector<int> dates;
  QVector<double> values;
  QVector<double> accu;

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

  std::partial_sum(values.begin(), values.end(), accu.begin(), std::multiplies<>());

  for (auto& v : accu) {
    v = (v - 1.0) * 100;
  }

  for (auto& v : values) {
    v = (v - 1.0) * 100;
  }

  return {dates, values, accu};
}

void TableFund::calculate() {
  if (model->rowCount() == 0) {
    return;
  }

  // Tables are displayed in descending order. This is also the order the data has to be retrieved from the model

  QString oldest_investment_date = model->record(model->rowCount() - 1).value("date").toString();

  auto qdt = QDateTime::fromString(oldest_investment_date, "MM/yyyy");

  auto [inflation_dates, inflation_values, inflation_accumulated] =
      process_benchmark("inflation", qdt.toSecsSinceEpoch());

  qsettings.beginGroup(name);

  double income_tax = qsettings.value("income_tax", 0.0).toDouble();

  qsettings.endGroup();

  calculate_accumulated_sum("deposit");
  calculate_accumulated_sum("withdrawal");

  double gross_return_sum = 0.0;

  for (int n = model->rowCount() - 1; n >= 0; n--) {
    QString date = model->record(n).value("date").toString();
    double deposit = model->record(n).value("deposit").toDouble();
    double withdrawal = model->record(n).value("withdrawal").toDouble();
    double starting_balance = model->record(n).value("starting_balance").toDouble();
    double ending_balance = model->record(n).value("ending_balance").toDouble();
    double accumulated_deposit = model->record(n).value("accumulated_deposit").toDouble();
    double accumulated_withdrawal = model->record(n).value("accumulated_withdrawal").toDouble();

    double gross_return = ending_balance - starting_balance - deposit + withdrawal;

    gross_return_sum += gross_return;

    double net_return = gross_return * (1.0 - 0.01 * income_tax);

    double net_return_perc = 100 * net_return / (starting_balance + deposit - withdrawal);

    double real_return_perc = net_return_perc;

    for (int i = 0; i < inflation_dates.size(); i++) {
      qdt.setSecsSinceEpoch(inflation_dates[i]);

      if (qdt.toString("MM/yyyy") == date) {
        real_return_perc = 100.0 * (net_return_perc - inflation_values[i]) / (100.0 + inflation_values[i]);

        break;
      }
    }

    auto rec = model->record(n);

    rec.setGenerated("net_deposit", true);
    rec.setGenerated("net_return", true);
    rec.setGenerated("net_return_perc", true);
    rec.setGenerated("net_balance", true);
    rec.setGenerated("real_return_perc", true);

    rec.setValue("net_deposit", accumulated_deposit - accumulated_withdrawal);
    rec.setValue("gross_return", gross_return);
    rec.setValue("net_return", net_return);
    rec.setValue("net_balance", ending_balance - gross_return_sum * 0.01 * income_tax);
    rec.setValue("gross_return_perc", 100 * gross_return / (starting_balance + deposit - withdrawal));
    rec.setValue("net_return_perc", net_return_perc);
    rec.setValue("real_return_perc", real_return_perc);

    model->setRecord(n, rec);
  }

  calculate_accumulated_sum("net_return");
  calculate_accumulated_product("net_return_perc");
  calculate_accumulated_product("real_return_perc");

  clear_charts();

  make_chart1();
  make_chart2();
}

void TableFund::make_chart1() {
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

void TableFund::make_chart2() {
  chart2->setTitle(name.toUpper());

  add_axes_to_chart(chart2, "%");

  QVector<int> dates;
  QVector<double> accumulated_net_return;
  QVector<double> accumulated_real_return;

  for (int n = 0; n < model->rowCount() && dates.size() < spinbox_months->value(); n++) {
    auto qdt = QDateTime::fromString(model->record(n).value("date").toString(), "MM/yyyy");

    dates.append(qdt.toSecsSinceEpoch());
    accumulated_net_return.append(model->record(n).value("accumulated_net_return_perc").toDouble());
    accumulated_real_return.append(model->record(n).value("accumulated_real_return_perc").toDouble());
  }

  if (dates.empty()) {
    return;
  }

  perc_chart_oldest_date = dates[dates.size() - 1];

  auto s1 = add_series_to_chart(chart2, dates, accumulated_net_return, "Net Return");

  connect(s1, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout2, s1->name()); });

  auto s2 = add_series_to_chart(chart2, dates, accumulated_real_return, "Real Return");

  connect(s2, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout2, s2->name()); });

  // ask the main window class for the benchmarks

  emit getBenchmarkTables();
}

void TableFund::show_benchmark(const TableBase* btable) {
  auto [dates, values, accumulated] = process_benchmark(btable->name, perc_chart_oldest_date);

  if (chart2->axes().empty()) {
    return;
  }

  auto series = new QLineSeries();

  series->setName(btable->name.toLower());

  connect(series, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout2, series->name()); });

  double vmin = dynamic_cast<QValueAxis*>(chart2->axes(Qt::Vertical)[0])->min();
  double vmax = dynamic_cast<QValueAxis*>(chart2->axes(Qt::Vertical)[0])->max();

  for (int n = 0; n < dates.size(); n++) {
    auto qdt = QDateTime::fromSecsSinceEpoch(dates[n]);

    auto epoch_in_ms = qdt.toMSecsSinceEpoch();

    double v = accumulated[n];

    if (!chart2->series().empty()) {
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