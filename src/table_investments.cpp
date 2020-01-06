#include "table_investments.hpp"
#include <QSqlQuery>

TableInvestments::TableInvestments(QWidget* parent) : TableBase(parent), qsettings(QSettings()) {
  type = TableType::Investment;

  // shadow effects

  investment_cfg_frame->setGraphicsEffect(card_shadow());

  // signals

  connect(doublespinbox_income_tax, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [&](double value) {
    qsettings.beginGroup(name);

    qsettings.setValue("income_tax", value);

    qsettings.endGroup();

    qsettings.sync();
  });
}

void TableInvestments::init_model() {
  model->setTable(name);
  model->setEditStrategy(QSqlTableModel::OnManualSubmit);
  model->setSort(1, Qt::DescendingOrder);

  auto currency = QLocale().currencySymbol();

  model->setHeaderData(1, Qt::Horizontal, "Date");
  model->setHeaderData(2, Qt::Horizontal, "Deposit " + currency);
  model->setHeaderData(3, Qt::Horizontal, "Withdrawal " + currency);
  model->setHeaderData(4, Qt::Horizontal, "Bank Balance " + currency);
  model->setHeaderData(5, Qt::Horizontal, "Total Deposits " + currency);
  model->setHeaderData(6, Qt::Horizontal, "Total Withdrawals " + currency);
  model->setHeaderData(7, Qt::Horizontal, "Gross Return " + currency);
  model->setHeaderData(8, Qt::Horizontal, "Gross Return %");
  model->setHeaderData(9, Qt::Horizontal, "Net Return " + currency);
  model->setHeaderData(10, Qt::Horizontal, "Net Return %");
  model->setHeaderData(11, Qt::Horizontal, "Net Bank Balance " + currency);
  model->setHeaderData(12, Qt::Horizontal, "Real Return %");

  model->select();

  table_view->setModel(model);
  table_view->setColumnHidden(0, true);

  // initializing widgets with qsettings values

  qsettings.beginGroup(name);

  doublespinbox_income_tax->setValue(qsettings.value("income_tax", 0.0).toDouble());

  qsettings.endGroup();
}

void TableInvestments::calculate_totals(const QString& column_name) {
  QVector<double> list_values;

  for (int n = 0; n < model->rowCount(); n++) {
    list_values.append(model->record(n).value(column_name).toDouble());
  }

  if (list_values.size() > 0) {
    std::reverse(list_values.begin(), list_values.end());

    // cumulative sum

    std::partial_sum(list_values.begin(), list_values.end(), list_values.begin());

    std::reverse(list_values.begin(), list_values.end());

    for (int n = 0; n < model->rowCount(); n++) {
      auto rec = model->record(n);

      rec.setGenerated("total_" + column_name, true);

      rec.setValue("total_" + column_name, list_values[n]);

      model->setRecord(n, rec);
    }
  }
}

void TableInvestments::calculate() {
  if (model->rowCount() == 0) {
    return;
  }

  // get inflation values
  QVector<QPair<int, double>> inflation_vec;

  auto query = QSqlQuery(db);

  query.prepare("select distinct date,accumulated from inflation order by date desc");

  if (query.exec()) {
    while (query.next()) {
      inflation_vec.append(QPair(query.value(0).toInt(), query.value(1).toDouble()));
    }
  } else {
    qDebug("Failed to get inflation table values!");
  }

  calculate_totals("deposit");
  calculate_totals("withdrawal");

  qsettings.beginGroup(name);

  double income_tax = qsettings.value("income_tax", 0.0).toDouble();

  qsettings.endGroup();

  for (int n = 0; n < model->rowCount(); n++) {
    QString date = model->record(n).value("date").toString();
    double total_deposit = model->record(n).value("total_deposit").toDouble();
    double total_withdrawal = model->record(n).value("total_withdrawal").toDouble();

    double gross_return = model->record(n).value("bank_balance").toDouble() - (total_deposit - total_withdrawal);

    double net_return = gross_return * (1.0 - 0.01 * income_tax);

    double net_return_perc = 100 * net_return / (total_deposit - total_withdrawal);

    auto qdt = QDateTime();
    auto date_month = QDate::fromString(date, "dd/MM/yyyy").toString("MM/yyyy");
    double real_return_perc = 0.0;

    for (auto& p : inflation_vec) {
      qdt.setSecsSinceEpoch(p.first);

      if (qdt.toString("MM/yyyy") == date_month) {
        real_return_perc = 100.0 * (net_return_perc - p.second) / (100.0 + p.second);

        break;
      }
    }

    auto rec = model->record(n);

    rec.setGenerated("gross_return", true);
    rec.setGenerated("gross_return_perc", true);
    rec.setGenerated("net_return", true);
    rec.setGenerated("net_return_perc", true);
    rec.setGenerated("net_bank_balance", true);
    rec.setGenerated("real_return_perc", true);

    rec.setValue("gross_return", gross_return);
    rec.setValue("net_return", net_return);
    rec.setValue("net_bank_balance", total_deposit + net_return - total_withdrawal);
    rec.setValue("gross_return_perc", 100 * gross_return / (total_deposit - total_withdrawal));
    rec.setValue("net_return_perc", net_return_perc);
    rec.setValue("real_return_perc", real_return_perc);

    model->setRecord(n, rec);
  }

  clear_charts();

  make_chart1();
  make_chart2();
}

void TableInvestments::make_chart1() {
  chart1->setTitle(name.toUpper());

  add_axes_to_chart(chart1, QLocale().currencySymbol());
  add_series_to_chart(chart1, model, "Deposits", "total_deposit");
  add_series_to_chart(chart1, model, "Net Bank Balance", "net_bank_balance");

  bool show_withdrawals = false;

  for (int n = 0; n < model->rowCount(); n++) {
    double withdrawal = model->record(n).value("total_withdrawal").toDouble();

    if (fabs(withdrawal) > 0) {
      show_withdrawals = true;

      break;
    }
  }

  if (show_withdrawals) {
    add_series_to_chart(chart1, model, "Withdrawals", "total_withdrawal");
  }
}

void TableInvestments::make_chart2() {
  chart2->setTitle(name.toUpper());

  add_axes_to_chart(chart2, "%");
  add_series_to_chart(chart2, model, "Net Return", "net_return_perc");
  add_series_to_chart(chart2, model, "Real Return", "real_return_perc");

  // ask the main window class for the benchmarks

  emit getBenchmarkTables();
}

void TableInvestments::show_benchmark(const TableBase* btable) {
  add_series_to_chart(chart2, btable->model, btable->name, "accumulated");
}