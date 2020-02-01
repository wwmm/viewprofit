#include "table_benchmarks.hpp"
#include <QSqlQuery>
#include "chart_funcs.hpp"

TableBenchmarks::TableBenchmarks(QWidget* parent) : TableBase(parent) {
  type = TableType::Benchmark;

  fund_cfg_frame->hide();

  radio_chart2->setText("Accumulated");
  radio_chart1->setText("Monthly Value");

  connect(spinbox_months, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) { show_chart(); });
}

void TableBenchmarks::init_model() {
  model->setTable(name);
  model->setEditStrategy(QSqlTableModel::OnManualSubmit);
  model->setSort(1, Qt::DescendingOrder);
  model->setHeaderData(1, Qt::Horizontal, "Date");
  model->setHeaderData(2, Qt::Horizontal, "Monthly Value %");
  model->setHeaderData(3, Qt::Horizontal, "Accumulated %");
  model->select();

  table_view->setModel(model);
  table_view->setColumnHidden(0, true);
}

void TableBenchmarks::calculate() {
  QVector<double> list_values;
  QVector<double> accu;

  for (int n = 0; n < model->rowCount(); n++) {
    list_values.append(model->record(n).value("value").toDouble());
  }

  if (!list_values.empty()) {
    std::reverse(list_values.begin(), list_values.end());

    for (auto& value : list_values) {
      value = value * 0.01 + 1.0;
    }

    // cumulative product

    accu.resize(list_values.size());

    std::partial_sum(list_values.begin(), list_values.end(), accu.begin(), std::multiplies<>());

    for (auto& value : accu) {
      value = (value - 1.0) * 100;
    }

    std::reverse(accu.begin(), accu.end());

    for (int n = 0; n < model->rowCount(); n++) {
      auto rec = model->record(n);

      rec.setGenerated("accumulated", true);

      rec.setValue("accumulated", accu[n]);

      model->setRecord(n, rec);
    }

    show_chart();
  }
}

void TableBenchmarks::show_chart() {
  clear_charts();

  chart1->setTitle(name.toUpper());
  chart2->setTitle(name.toUpper());

  add_axes_to_chart(chart1, "%");
  add_axes_to_chart(chart2, "%");

  auto s1 = add_series_to_chart(chart1, model, "Monthly Value", "value");

  connect(s1, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout1, s1->name()); });

  model->submitAll();

  QVector<int> dates;
  QVector<double> values;
  QVector<double> accu;

  auto query = QSqlQuery(db);

  query.prepare("select distinct date,value from " + name + " order by date desc");

  if (query.exec()) {
    while (query.next() && dates.size() < spinbox_months->value()) {
      dates.append(query.value(0).toInt());
      values.append(query.value(1).toDouble());
    }
  } else {
    qDebug("Failed to get inflation table values!");
  }

  if (dates.empty()) {
    qDebug("oi");
    return;
  }

  std::reverse(dates.begin(), dates.end());
  std::reverse(values.begin(), values.end());

  for (auto& v : values) {
    v = v * 0.01 + 1.0;
  }

  // cumulative product

  accu.resize(values.size());

  std::partial_sum(values.begin(), values.end(), accu.begin(), std::multiplies<>());

  for (auto& v : accu) {
    v = (v - 1.0) * 100;
  }

  auto s2 = add_series_to_chart(chart2, dates, accu, "Accumulated");

  connect(s2, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout2, s2->name()); });
}