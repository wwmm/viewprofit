#include "compare_funds.hpp"
#include <QSqlError>
#include <QSqlQuery>
#include <deque>
#include "chart_funcs.hpp"
#include "effects.hpp"

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

  connect(spinbox_months, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) { process_tables(); });
}

void CompareFunds::make_chart_net_balance_pie() {
  chart->setTitle("");

  auto series = new QPieSeries();

  series->setPieSize(0.6);

  std::deque<QPair<QString, double>> deque;

  for (auto& table : tables) {
    deque.push_back(QPair(table->name, table->model->record(0).value("net_balance").toDouble()));
  }

  std::sort(deque.begin(), deque.end(), [](auto a, auto b) { return a.second < b.second; });

  /*
    Now we alternate the slices. This will help to avoid label overlap.
  */

  bool pop_front = true;
  QVector<QString> names_added;

  while (deque.size() > 0) {
    double d;
    QString name;

    if (pop_front) {
      name = deque[0].first;
      d = deque[0].second;

      deque.pop_front();

      pop_front = false;
    } else {
      name = deque[deque.size() - 1].first;
      d = deque[deque.size() - 1].second;

      deque.pop_back();

      pop_front = true;
    }

    series->append(name, d);
  }

  bool explode = true;

  for (auto& slice : series->slices()) {
    slice->setLabelVisible(true);

    auto label = slice->label();

    slice->setLabel(QString("%1 %2%").arg(label, QString::number(100 * slice->percentage(), 'f', 2)));

    if (explode) {
      slice->setExploded(true);
      explode = false;
    } else {
      explode = true;
    }
  }

  chart->addSeries(series);
}

void CompareFunds::make_chart_net_return_pie() {
  chart->setTitle("");

  auto series = new QPieSeries();

  series->setPieSize(0.6);

  std::deque<QPair<QString, double>> deque;

  for (auto& table : tables) {
    deque.push_back(QPair(table->name, table->model->record(0).value("net_return").toDouble()));
  }

  std::sort(deque.begin(), deque.end(), [](auto a, auto b) { return a.second < b.second; });

  /*
    Now we alternate the slices. This will help to avoid label overlap.
  */

  bool pop_front = true;
  QVector<QString> names_added;

  while (deque.size() > 0) {
    double d;
    QString name;

    if (pop_front) {
      name = deque[0].first;
      d = deque[0].second;

      deque.pop_front();

      pop_front = false;
    } else {
      name = deque[deque.size() - 1].first;
      d = deque[deque.size() - 1].second;

      deque.pop_back();

      pop_front = true;
    }

    series->append(name, d);
  }

  bool explode = true;

  for (auto& slice : series->slices()) {
    slice->setLabelVisible(true);

    auto label = slice->label();

    slice->setLabel(QString("%1 %2%").arg(label, QString::number(100 * slice->percentage(), 'f', 2)));

    if (explode) {
      slice->setExploded(true);
      explode = false;
    } else {
      explode = true;
    }
  }

  chart->addSeries(series);
}

void CompareFunds::make_chart_net_return() {
  chart->setTitle("Net Return");

  add_axes_to_chart(chart, "%");

  for (auto& table : tables) {
    auto s = add_series_to_chart(chart, table->model, table->name.toUpper(), "net_return_perc");

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
}

void CompareFunds::make_chart_net_return_volatility() {
  chart->setTitle("Standard Deviation");

  add_axes_to_chart(chart, "%");

  QVector<QString> names{"portfolio"};

  for (auto& table : tables) {
    names.push_back(table->name);
  }

  for (auto& name : names) {
    QVector<int> dates;
    QVector<double> values, accu, stddev;

    auto query = QSqlQuery(db);

    query.prepare("select distinct date,net_return_perc from " + name + " order by date desc");

    if (query.exec()) {
      while (query.next() && dates.size() < spinbox_months->value()) {
        dates.append(query.value(0).toInt());
        values.append(query.value(1).toDouble());
      }
    }

    if (dates.size() == 0) {
      break;
    }

    std::reverse(dates.begin(), dates.end());
    std::reverse(values.begin(), values.end());

    // cumulative sum

    accu.resize(values.size());

    std::partial_sum(values.begin(), values.end(), accu.begin());

    stddev.resize(values.size());

    /*
      Calculating the standard deviation https://en.wikipedia.org/wiki/Standard_deviation
    */

    for (int n = 0; n < stddev.size(); n++) {
      double sum = 0.0;
      double avg = accu[n] / (n + 1);

      for (int m = 0; m < n; m++) {
        sum += (values[m] - avg) * (values[m] - avg);
      }

      sum = (n > 0) ? sum / n : sum;

      stddev[n] = std::sqrt(sum);
    }

    auto s = add_series_to_chart(chart, dates, stddev, name);

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
}

void CompareFunds::make_chart_accumulated_net_return_pie() {
  chart->setTitle("");

  auto series = new QPieSeries();

  series->setPieSize(0.6);

  std::deque<QPair<QString, double>> deque;

  for (auto& table : tables) {
    deque.push_back(QPair(table->name, table->model->record(0).value("accumulated_net_return").toDouble()));
  }

  std::sort(deque.begin(), deque.end(), [](auto a, auto b) { return a.second < b.second; });

  /*
    Now we alternate the slices. This will help to avoid label overlap.
  */

  bool pop_front = true;
  QVector<QString> names_added;

  while (deque.size() > 0) {
    double d;
    QString name;

    if (pop_front) {
      name = deque[0].first;
      d = deque[0].second;

      deque.pop_front();

      pop_front = false;
    } else {
      name = deque[deque.size() - 1].first;
      d = deque[deque.size() - 1].second;

      deque.pop_back();

      pop_front = true;
    }

    series->append(name, d);
  }

  bool explode = true;

  for (auto& slice : series->slices()) {
    slice->setLabelVisible(true);

    auto label = slice->label();

    slice->setLabel(QString("%1 %2%").arg(label, QString::number(100 * slice->percentage(), 'f', 2)));

    if (explode) {
      slice->setExploded(true);
      explode = false;
    } else {
      explode = true;
    }
  }

  chart->addSeries(series);
}

void CompareFunds::make_chart_accumulated_net_return() {
  chart->setTitle("Net Return");

  add_axes_to_chart(chart, "%");

  for (auto& table : tables) {
    QVector<int> dates;
    QVector<double> values, accu;

    auto query = QSqlQuery(db);

    query.prepare("select distinct date,net_return_perc from " + table->name + " order by date desc");

    if (query.exec()) {
      while (query.next() && dates.size() < spinbox_months->value()) {
        dates.append(query.value(0).toInt());
        values.append(query.value(1).toDouble());
      }
    }

    if (dates.size() == 0) {
      break;
    }

    std::reverse(dates.begin(), dates.end());
    std::reverse(values.begin(), values.end());

    for (auto& v : values) {
      v = v * 0.01 + 1.0;
    }

    // cumulative product

    accu.resize(values.size());

    std::partial_sum(values.begin(), values.end(), accu.begin(), std::multiplies<double>());

    for (auto& v : accu) {
      v = (v - 1.0) * 100;
    }

    auto s = add_series_to_chart(chart, dates, accu, table->name.toUpper());

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
}

void CompareFunds::make_chart_barseries(const QString& series_name, const QString& column_name) {
  auto list_dates = get_unique_months_from_db(db, tables, spinbox_months->value());

  if (list_dates.size() == 0) {
    return;
  }

  // initialize the barsets

  QStackedBarSeries* series;
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
            v += barsets[m]->at(index);
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

void CompareFunds::process(const QVector<TableFund*>& tables) {
  this->tables = tables;

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
  }
}

void CompareFunds::on_chart_selection(const bool& state) {
  if (state) {
    if (radio_net_balance_pie->isChecked() || radio_net_return_pie->isChecked() ||
        radio_accumulated_net_return_pie->isChecked()) {
      spinbox_months->setDisabled(true);
    } else {
      spinbox_months->setDisabled(false);
    }

    process_tables();
  }
}

void CompareFunds::on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name) {
  if (state) {
    auto qdt = QDateTime();

    qdt.setMSecsSinceEpoch(point.x());

    if (radio_net_return_perc->isChecked() || radio_accumulated_net_return_perc->isChecked()) {
      c->setText(QString("Fund: %1\nDate: %2\nReturn: %3%")
                     .arg(name, qdt.toString("MM/yyyy"), QString::number(point.y(), 'f', 2)));
    } else if (radio_net_return_volatility->isChecked()) {
      c->setText(QString("Fund: %1\nDate: %2\nValue: %3%")
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
