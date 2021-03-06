#include "compare_funds.hpp"
#include <QSqlError>
#include <QSqlQuery>
#include "chart_funcs.hpp"
#include "effects.hpp"
#include "math.hpp"

CompareFunds::CompareFunds(const QSqlDatabase& database, QWidget* parent)
    : db(database), chart(new QChart()), callout(new Callout(chart)) {
  setupUi(this);

  callout->hide();
  spinbox_months->setDisabled(true);

  // shadow effects

  frame_chart->setGraphicsEffect(card_shadow());
  frame_resource_allocation->setGraphicsEffect(card_shadow());
  frame_net_return->setGraphicsEffect(card_shadow());
  frame_accumulated_net_return->setGraphicsEffect(card_shadow());
  frame_time_window->setGraphicsEffect(card_shadow());
  button_reset_zoom->setGraphicsEffect(button_shadow());

  // chart settings

  chart->setTheme(QChart::ChartThemeLight);
  chart->setAcceptHoverEvents(true);
  chart->legend()->setAlignment(Qt::AlignRight);
  chart->legend()->setShowToolTips(true);

  chart_view->setChart(chart);
  chart_view->setRenderHint(QPainter::Antialiasing);
  chart_view->setRubberBand(QChartView::RectangleRubberBand);

  // signals

  connect(button_reset_zoom, &QPushButton::clicked, this, [&]() { chart->zoomReset(); });

  connect(radio_net_balance_pie, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_net_balance, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_net_return_pie, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_net_return, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_net_return_perc, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_net_return_volatility, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_accumulated_net_return_pie, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_accumulated_net_return, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_accumulated_net_return_perc, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_accumulated_net_return_second_derivative, &QRadioButton::toggled, this,
          &CompareFunds::on_chart_selection);

  connect(spinbox_months, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) { process_tables(); });
}

void CompareFunds::make_chart_net_balance_pie() {
  std::deque<QPair<QString, double>> deque;

  for (auto& table : tables) {
    deque.emplace_back(table->name, table->model->record(0).value("net_balance").toDouble());
  }

  make_pie(deque);
}

void CompareFunds::make_chart_net_return_pie() {
  std::deque<QPair<QString, double>> deque;

  for (auto& table : tables) {
    double value = table->model->record(0).value("net_return").toDouble();

    if (value > 0) {
      deque.emplace_back(table->name, value);
    }
  }

  make_pie(deque);
}

void CompareFunds::make_chart_net_return() {
  chart->setTitle("Net Return");

  add_axes_to_chart(chart, "%");

  for (auto& table : tables) {
    QVector<int> dates;
    QVector<double> values;

    for (int n = 0; n < table->model->rowCount() && n < spinbox_months->value(); n++) {
      auto qdt = QDateTime::fromString(table->model->record(n).value("date").toString(), "MM/yyyy");

      dates.append(qdt.toSecsSinceEpoch());
      values.append(table->model->record(n).value("net_return_perc").toDouble());
    }

    if (dates.size() < 2) {
      continue;
    }

    auto* const s = add_series_to_chart(chart, dates, values, table->name);

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }

  // portfolio

  QVector<int> dates;
  QVector<double> values;

  for (int n = 0; n < portfolio->model->rowCount() && n < spinbox_months->value(); n++) {
    auto qdt = QDateTime::fromString(portfolio->model->record(n).value("date").toString(), "MM/yyyy");

    dates.append(qdt.toSecsSinceEpoch());
    values.append(portfolio->model->record(n).value("net_return_perc").toDouble());
  }

  if (dates.size() < 2) {
    return;
  }

  auto* const s = add_series_to_chart(chart, dates, values, portfolio->name);

  connect(s, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
}

void CompareFunds::make_chart_net_return_volatility() {
  chart->setTitle("Standard Deviation");

  add_axes_to_chart(chart, "%");

  for (auto& table : tables) {
    QVector<int> dates;
    QVector<double> values;

    for (int n = 0; n < table->model->rowCount() && n < spinbox_months->value(); n++) {
      auto qdt = QDateTime::fromString(table->model->record(n).value("date").toString(), "MM/yyyy");

      dates.append(qdt.toSecsSinceEpoch());
      values.append(table->model->record(n).value("net_return_perc").toDouble());
    }

    if (dates.size() < 2) {
      continue;
    }

    std::reverse(dates.begin(), dates.end());
    std::reverse(values.begin(), values.end());

    auto* const s = add_series_to_chart(chart, dates, standard_deviation(values), table->name);

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }

  // portfolio

  QVector<int> dates;
  QVector<double> values;

  for (int n = 0; n < portfolio->model->rowCount() && n < spinbox_months->value(); n++) {
    auto qdt = QDateTime::fromString(portfolio->model->record(n).value("date").toString(), "MM/yyyy");

    dates.append(qdt.toSecsSinceEpoch());
    values.append(portfolio->model->record(n).value("net_return_perc").toDouble());
  }

  if (dates.size() < 2) {
    return;
  }

  std::reverse(dates.begin(), dates.end());
  std::reverse(values.begin(), values.end());

  auto* const s = add_series_to_chart(chart, dates, standard_deviation(values), portfolio->name);

  connect(s, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
}

void CompareFunds::make_chart_accumulated_net_return_pie() {
  std::deque<QPair<QString, double>> deque;

  for (auto& table : tables) {
    double value = table->model->record(0).value("accumulated_net_return").toDouble();

    if (value > 0) {
      deque.emplace_back(table->name, value);
    }
  }

  make_pie(deque);
}

void CompareFunds::make_chart_accumulated_net_return() {
  chart->setTitle("Net Return");

  add_axes_to_chart(chart, "%");

  for (auto& table : tables) {
    QVector<int> dates;
    QVector<double> net_return;
    QVector<double> accumulated_net_return;

    for (int n = 0; n < table->model->rowCount() && dates.size() < spinbox_months->value(); n++) {
      const auto qdt = QDateTime::fromString(table->model->record(n).value("date").toString(), "MM/yyyy");

      dates.append(qdt.toSecsSinceEpoch());
      net_return.append(table->model->record(n).value("net_return_perc").toDouble());
    }

    if (dates.size() < 2) {  // We need at least 2 points to show a line chart
      continue;
    }

    std::reverse(net_return.begin(), net_return.end());

    for (auto& value : net_return) {
      value = value * 0.01 + 1.0;
    }

    accumulated_net_return.resize(net_return.size());

    std::partial_sum(net_return.begin(), net_return.end(), accumulated_net_return.begin(), std::multiplies<>());

    for (auto& value : accumulated_net_return) {
      value = (value - 1.0) * 100;
    }

    std::reverse(accumulated_net_return.begin(), accumulated_net_return.end());

    auto* const s = add_series_to_chart(chart, dates, accumulated_net_return, table->name.toUpper());

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
}

void CompareFunds::make_chart_accumulated_net_return_second_derivative() {
  chart->setTitle("Accumulated Net Return Second Derivative");

  add_axes_to_chart(chart, "");

  for (auto& table : tables) {
    QVector<int> dates;
    QVector<double> net_return;
    QVector<double> accumulated_net_return;

    for (int n = 0; n < table->model->rowCount() && dates.size() < spinbox_months->value(); n++) {
      const auto qdt = QDateTime::fromString(table->model->record(n).value("date").toString(), "MM/yyyy");

      dates.append(qdt.toSecsSinceEpoch());
      net_return.append(table->model->record(n).value("net_return_perc").toDouble());
    }

    if (dates.size() < 3) {  // We need at least 3 points to calculate the second derivative
      continue;
    }

    std::reverse(net_return.begin(), net_return.end());

    for (auto& value : net_return) {
      value = value * 0.01 + 1.0;
    }

    accumulated_net_return.resize(net_return.size());

    std::partial_sum(net_return.begin(), net_return.end(), accumulated_net_return.begin(), std::multiplies<>());

    for (auto& value : accumulated_net_return) {
      value = (value - 1.0) * 100;
    }

    std::reverse(dates.begin(), dates.end());
    std::reverse(accumulated_net_return.begin(), accumulated_net_return.end());

    auto* const s = add_series_to_chart(chart, dates, second_derivative(accumulated_net_return), table->name);

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }

  // portfolio

  QVector<int> dates;
  QVector<double> accumulated_net_return;

  for (int n = 0; n < portfolio->model->rowCount() && dates.size() < spinbox_months->value(); n++) {
    const auto qdt = QDateTime::fromString(portfolio->model->record(n).value("date").toString(), "MM/yyyy");

    dates.append(qdt.toSecsSinceEpoch());
    accumulated_net_return.append(portfolio->model->record(n).value("accumulated_net_return_perc").toDouble());
  }

  if (dates.size() < 3) {  // We need at least 3 points to calculate the second derivative
    return;
  }

  std::reverse(dates.begin(), dates.end());
  std::reverse(accumulated_net_return.begin(), accumulated_net_return.end());

  auto* const s = add_series_to_chart(chart, dates, second_derivative(accumulated_net_return), portfolio->name);

  connect(s, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
}

void CompareFunds::make_chart_barseries(const QString& series_name, const QString& column_name) {
  const auto list_dates = get_unique_months_from_db(db, tables, spinbox_months->value());

  if (list_dates.empty()) {
    return;
  }

  // initialize the barsets

  QStackedBarSeries* series = nullptr;
  QVector<QBarSet*> barsets;
  QStringList categories;

  std::tie(series, barsets, categories) =
      add_tables_barseries_to_chart(chart, tables, list_dates, series_name, column_name);

  connect(series, &QStackedBarSeries::hovered, this, [=](bool status, int index, QBarSet* barset) {
    if (status) {
      double v = 0.5 * barset->at(index);

      for (int n = 1; n < barsets.size(); n++) {
        if (barsets[n] == barset) {
          for (int m = 0; m < n; m++) {
            auto bv = barsets[m]->at(index);

            if (v * bv > 0) {
              v += bv;
            }
          }

          break;
        }
      }

      callout->setText(QString(" Fund: %1\n Date: %2 \n Value: " + QLocale().currencySymbol() + " %3")
                           .arg(barset->label(), categories[index], QString::number(barset->at(index), 'f', 2)));

      callout->setAnchor(QPointF(index, v));

      callout->setZValue(11);

      callout->updateGeometry();

      callout->show();
    } else {
      callout->hide();
    }
  });
}

void CompareFunds::make_pie(std::deque<QPair<QString, double>>& deque) {
  const double pie_chart_size = 0.6;

  chart->setTitle("");

  auto* const series = new QPieSeries();

  series->setPieSize(pie_chart_size);

  std::sort(deque.begin(), deque.end(), [](auto a, auto b) { return std::fabs(a.second) < std::fabs(b.second); });

  /*
    Now we alternate the slices. This will help to avoid label overlap.
  */

  bool pop_front = true;
  QVector<QString> names_added;

  while (!deque.empty()) {
    if (pop_front) {
      series->append(deque[0].first, deque[0].second);

      deque.pop_front();
    } else {
      series->append(deque[deque.size() - 1].first, deque[deque.size() - 1].second);

      deque.pop_back();
    }

    pop_front = !pop_front;
  }

  bool explode = true;

  for (auto& slice : series->slices()) {
    slice->setLabelVisible(true);

    const auto label = slice->label();

    slice->setLabel(QString("%1 %2%").arg(label, QString::number(100 * slice->percentage(), 'f', 2)));

    slice->setExploded(explode);

    explode = !explode;
  }

  chart->addSeries(series);
}

void CompareFunds::process(const QVector<TableFund const*>& tables, TablePortfolio const* portfolio) {
  this->tables = tables;
  this->portfolio = portfolio;

  process_tables();
}

void CompareFunds::process_tables() {
  clear_chart(chart);

  if (radio_net_balance_pie->isChecked()) {
    make_chart_net_balance_pie();
  } else if (radio_net_balance->isChecked()) {
    make_chart_barseries("Net Balance", "net_balance");
  } else if (radio_net_return_pie->isChecked()) {
    make_chart_net_return_pie();
  } else if (radio_net_return->isChecked()) {
    make_chart_barseries("Net Return", "net_return");
  } else if (radio_net_return_perc->isChecked()) {
    make_chart_net_return();
  } else if (radio_net_return_volatility->isChecked()) {
    make_chart_net_return_volatility();
  } else if (radio_accumulated_net_return_pie->isChecked()) {
    make_chart_accumulated_net_return_pie();
  } else if (radio_accumulated_net_return->isChecked()) {
    make_chart_barseries("Acumulated Net Return", "accumulated_net_return");
  } else if (radio_accumulated_net_return_perc->isChecked()) {
    make_chart_accumulated_net_return();
  } else if (radio_accumulated_net_return_second_derivative->isChecked()) {
    make_chart_accumulated_net_return_second_derivative();
  }
}

void CompareFunds::on_chart_selection(const bool& state) {
  if (!state) {
    return;
  }

  if (radio_net_balance_pie->isChecked() || radio_net_return_pie->isChecked() ||
      radio_accumulated_net_return_pie->isChecked()) {
    spinbox_months->setDisabled(true);
  } else {
    spinbox_months->setDisabled(false);
  }

  process_tables();
}

void CompareFunds::on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name) {
  if (state) {
    const auto qdt = QDateTime::fromMSecsSinceEpoch(point.x());

    if (radio_net_return_perc->isChecked() || radio_accumulated_net_return_perc->isChecked()) {
      c->setText(QString("Fund: %1\nDate: %2\nReturn: %3%")
                     .arg(name, qdt.toString("MM/yyyy"), QString::number(point.y(), 'f', 2)));
    } else if (radio_net_return_volatility->isChecked()) {
      c->setText(QString("Fund: %1\nDate: %2\nValue: %3%")
                     .arg(name, qdt.toString("MM/yyyy"), QString::number(point.y(), 'f', 2)));
    } else if (radio_accumulated_net_return_second_derivative->isChecked()) {
      c->setText(QString("Fund: %1\nDate: %2\nValue: %3")
                     .arg(name, qdt.toString("MM/yyyy"), QString::number(point.y(), 'f', 2)));
    }

    c->setAnchor(point);

    c->setZValue(11);

    c->updateGeometry();

    c->show();
  } else {
    c->hide();
  }
}
