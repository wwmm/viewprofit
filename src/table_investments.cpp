#include "table_investments.hpp"

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
  model->setHeaderData(2, Qt::Horizontal, "Contribution " + currency);
  model->setHeaderData(3, Qt::Horizontal, "Bank Balance " + currency);
  model->setHeaderData(4, Qt::Horizontal, "Total Contribution " + currency);
  model->setHeaderData(5, Qt::Horizontal, "Gross Return " + currency);
  model->setHeaderData(6, Qt::Horizontal, "Gross Return %");
  model->setHeaderData(7, Qt::Horizontal, "Real Return " + currency);
  model->setHeaderData(8, Qt::Horizontal, "Real Return %");
  model->setHeaderData(9, Qt::Horizontal, "Real Bank Balance " + currency);

  model->select();

  table_view->setModel(model);
  table_view->setColumnHidden(0, true);

  // initializing widgets with qsettings values

  qsettings.beginGroup(name);

  doublespinbox_income_tax->setValue(qsettings.value("income_tax", 0.0).toDouble());

  qsettings.endGroup();
}

void TableInvestments::calculate_total_contribution() {
  QVector<double> list_values;

  for (int n = 0; n < model->rowCount(); n++) {
    list_values.append(model->record(n).value("contribution").toDouble());
  }

  if (list_values.size() > 0) {
    std::reverse(list_values.begin(), list_values.end());

    // cumulative sum

    std::partial_sum(list_values.begin(), list_values.end(), list_values.begin());

    std::reverse(list_values.begin(), list_values.end());

    for (int n = 0; n < model->rowCount(); n++) {
      auto rec = model->record(n);

      rec.setGenerated("total_contribution", true);

      rec.setValue("total_contribution", list_values[n]);

      model->setRecord(n, rec);
    }
  }
}

void TableInvestments::calculate() {
  if (model->rowCount() == 0) {
    return;
  }

  calculate_total_contribution();

  qsettings.beginGroup(name);

  double income_tax = qsettings.value("income_tax", 0.0).toDouble();

  qsettings.endGroup();

  for (int n = 0; n < model->rowCount(); n++) {
    double total_contribution = model->record(n).value("total_contribution").toDouble();

    double gross_return = model->record(n).value("bank_balance").toDouble() - total_contribution;

    double real_return = gross_return * (1.0 - 0.01 * income_tax);

    auto rec = model->record(n);

    rec.setGenerated("gross_return", true);
    rec.setGenerated("gross_return_perc", true);
    rec.setGenerated("real_return", true);
    rec.setGenerated("real_return_perc", true);
    rec.setGenerated("real_bank_balance", true);

    rec.setValue("gross_return", gross_return);
    rec.setValue("real_return", real_return);
    rec.setValue("real_bank_balance", total_contribution + real_return);

    if (total_contribution > 0) {
      rec.setValue("gross_return_perc", 100 * gross_return / total_contribution);
      rec.setValue("real_return_perc", 100 * real_return / total_contribution);
    }

    model->setRecord(n, rec);
  }

  clear_charts();

  make_chart1();
  make_chart2();
}

void TableInvestments::make_chart1() {
  chart1->setTitle(name.toUpper());

  add_axes_to_chart(chart1, QLocale().currencySymbol());
  add_series_to_chart(chart1, model, "Total Contribution", "total_contribution");
  add_series_to_chart(chart1, model, "Real Bank Balance", "real_bank_balance");
}

void TableInvestments::make_chart2() {
  chart2->setTitle(name.toUpper());

  add_axes_to_chart(chart2, "%");
  add_series_to_chart(chart2, model, "Real Return", "real_return_perc");

  // ask the main window class for the benchmarks

  emit getBenchmarkTables();
}

void TableInvestments::show_benchmark(const TableBase* btable) {
  add_series_to_chart(chart2, btable->model, btable->name, "accumulated");
}