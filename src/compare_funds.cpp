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
  label_months->hide();
  spinbox_months->hide();

  // shadow effects

  frame_chart->setGraphicsEffect(card_shadow());
  frame_resource_allocation->setGraphicsEffect(card_shadow());
  frame_net_return->setGraphicsEffect(card_shadow());
  frame_accumulated_net_return->setGraphicsEffect(card_shadow());
  button_reset_zoom->setGraphicsEffect(button_shadow());

  // chart settings

  chart->setTheme(QChart::ChartThemeLight);
  chart->setAcceptHoverEvents(true);
  // chart->setAnimationOptions(QChart::SeriesAnimations);

  chart_view->setChart(chart);
  chart_view->setRenderHint(QPainter::Antialiasing);
  chart_view->setRubberBand(QChartView::RectangleRubberBand);

  // signals

  connect(button_reset_zoom, &QPushButton::clicked, this, [&]() { chart->zoomReset(); });

  connect(radio_resource_allocation, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_net_balance, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_net_return, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_net_return_perc, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_accumulated_net_return, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_accumulated_net_return_perc, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);

  connect(spinbox_months, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) {
    clear_chart(chart);
    make_chart_accumulated_net_return();
  });
}

void CompareFunds::make_chart_resource_allocation() {
  chart->setTitle("");
  chart->legend()->hide();

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

void CompareFunds::make_chart_net_return() {
  chart->setTitle("Net Return");
  chart->legend()->setAlignment(Qt::AlignRight);
  chart->legend()->show();

  add_axes_to_chart(chart, "%");

  for (auto& table : tables) {
    auto s = add_series_to_chart(chart, table->model, table->name.toUpper(), "net_return_perc");

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
}

void CompareFunds::make_chart_accumulated_net_return() {
  chart->setTitle("Net Return");
  chart->legend()->setAlignment(Qt::AlignRight);
  chart->legend()->show();

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
  auto list_dates = get_unique_dates_from_db(db, tables, 13);

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

  if (radio_resource_allocation->isChecked()) {
    make_chart_resource_allocation();
  } else if (radio_net_balance->isChecked()) {
    make_chart_barseries("Net Balance", "net_balance");
  } else if (radio_net_return->isChecked()) {
    make_chart_barseries("Net Return", "net_return");
  } else if (radio_net_return_perc->isChecked()) {
    make_chart_net_return();
  } else if (radio_accumulated_net_return->isChecked()) {
    make_chart_barseries("Acumulated Net Return", "accumulated_net_return");
  } else if (radio_accumulated_net_return_perc->isChecked()) {
    make_chart_accumulated_net_return();
  }
}

void CompareFunds::on_chart_selection(const bool& state) {
  if (state) {
    if (radio_accumulated_net_return_perc->isChecked()) {
      label_months->show();
      spinbox_months->show();
    } else {
      label_months->hide();
      spinbox_months->hide();
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
                     .arg(name, qdt.toString("dd/MM/yyyy"), QString::number(point.y(), 'f', 2)));
    }

    c->setAnchor(point);

    c->setZValue(11);

    c->updateGeometry();

    c->show();
  } else {
    c->hide();
  }
}
