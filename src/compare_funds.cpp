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

  // shadow effects

  frame_chart->setGraphicsEffect(card_shadow());
  frame_resource_allocation->setGraphicsEffect(card_shadow());
  chart_cfg_frame->setGraphicsEffect(card_shadow());
  button_reset_zoom->setGraphicsEffect(button_shadow());

  // chart settings

  chart->setTheme(QChart::ChartThemeLight);
  chart->setAcceptHoverEvents(true);
  chart->setAnimationOptions(QChart::SeriesAnimations);

  chart_view->setChart(chart);
  chart_view->setRenderHint(QPainter::Antialiasing);
  chart_view->setRubberBand(QChartView::RectangleRubberBand);

  // signals

  connect(button_reset_zoom, &QPushButton::clicked, this, [&]() { chart->zoomReset(); });

  connect(radio_resource_allocation, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_net_balance, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_net_return_perc, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_accumulated_net_return_perc, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
}

void CompareFunds::make_chart_resource_allocation() {
  chart->setTitle("");
  chart->legend()->hide();

  auto series = new QPieSeries();

  /*
    The code below appends data to the chart alternating the biggestand the smallest slices. THis helps to avoid label
    overlap.
  */

  std::deque<double> deque;

  for (auto& table : tables) {
    deque.push_back(table->model->record(0).value("net_balance").toDouble());
  }

  std::sort(deque.begin(), deque.end());

  bool pop_front = true;
  QVector<QString> names_added;
  double max_value = deque[deque.size() - 1];

  while (deque.size() > 0) {
    double d;

    if (pop_front) {
      d = deque[0];

      deque.pop_front();

      pop_front = false;
    } else {
      d = deque[deque.size() - 1];

      deque.pop_back();

      pop_front = true;
    }

    for (auto& table : tables) {
      bool skip = false;

      for (auto& name : names_added) {
        if (name == table->name) {
          skip = true;

          break;
        }
      }

      if (!skip) {
        double v = table->model->record(0).value("net_balance").toDouble();

        if (v == d) {
          series->append(table->name, v);

          names_added.push_back(table->name);

          break;
        }
      }
    }
  }

  for (auto& slice : series->slices()) {
    slice->setLabelVisible(true);

    auto label = slice->label();

    slice->setLabel(QString("%1 %2%").arg(label, QString::number(100 * slice->percentage(), 'f', 2)));

    if (slice->value() == max_value) {
      slice->setExploded(true);
    }
  }

  chart->addSeries(series);
}

void CompareFunds::make_chart_net_balance() {
  // get date values in each investment tables

  QSet<int> list_set;

  for (auto& table : tables) {
    // making sure all the latest data was saved to the database

    table->model->submitAll();

    auto query = QSqlQuery(db);

    query.prepare("select distinct date from " + table->name + " order by date desc");

    if (query.exec()) {
      while (query.next() && list_set.size() < 13) {  // show only the last 12 months
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

  // initialize the barsets

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

        if (date_month == QDate::fromString(tdate, "dd/MM/yyyy").toString("MM/yyyy")) {
          double v = tables[m]->model->record(n).value("net_balance").toDouble();

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

  chart->setTitle("Net Balance");
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

      callout->setText(QString("Fund: %1\nDate: %2\nBalance: " + QLocale().currencySymbol() + " %3")
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

void CompareFunds::make_chart_net_return() {
  chart->setTitle("Monthly Net Return");
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
  chart->setTitle("Accumulated Net Return");
  chart->legend()->setAlignment(Qt::AlignRight);
  chart->legend()->show();

  add_axes_to_chart(chart, "%");

  for (auto& table : tables) {
    auto s = add_series_to_chart(chart, table->model, table->name.toUpper(), "accumulated_net_return_perc");

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
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
    make_chart_net_balance();
  } else if (radio_net_return_perc->isChecked()) {
    make_chart_net_return();
  } else if (radio_accumulated_net_return_perc->isChecked()) {
    make_chart_accumulated_net_return();
  }
}

void CompareFunds::on_chart_selection(const bool& state) {
  if (state) {
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
