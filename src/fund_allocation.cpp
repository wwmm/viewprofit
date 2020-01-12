#include "fund_allocation.hpp"
#include <QSqlError>
#include <QSqlQuery>
#include <deque>
#include "chart_funcs.hpp"
#include "effects.hpp"

FundAllocation::FundAllocation(const QSqlDatabase& database, QWidget* parent)
    : db(database), chart1(new QChart()), chart2(new QChart()), callout(new Callout(chart2)) {
  setupUi(this);

  callout->hide();

  // shadow effects

  frame_chart->setGraphicsEffect(card_shadow());
  chart_cfg_frame->setGraphicsEffect(card_shadow());
  button_reset_zoom->setGraphicsEffect(button_shadow());

  // chart 1 settings

  chart1->setTheme(QChart::ChartThemeLight);
  chart1->legend()->hide();

  chart_view1->setChart(chart1);
  chart_view1->setRenderHint(QPainter::Antialiasing);

  // chart 2 settings

  chart2->setTheme(QChart::ChartThemeLight);
  chart2->setAcceptHoverEvents(true);
  chart2->legend()->setAlignment(Qt::AlignRight);

  chart_view2->setChart(chart2);
  chart_view2->setRenderHint(QPainter::Antialiasing);
  chart_view2->setRubberBand(QChartView::RectangleRubberBand);

  // signals

  connect(button_reset_zoom, &QPushButton::clicked, this, [&]() {
    if (radio_chart1->isChecked()) {
      chart1->zoomReset();
    }

    if (radio_chart2->isChecked()) {
      chart2->zoomReset();
    }
  });

  connect(radio_chart1, &QRadioButton::toggled, this, &FundAllocation::on_chart_selection);
  connect(radio_chart2, &QRadioButton::toggled, this, &FundAllocation::on_chart_selection);

  // select the default chart

  if (radio_chart1->isChecked()) {
    stackedwidget->setCurrentIndex(0);
  } else if (radio_chart2->isChecked()) {
    stackedwidget->setCurrentIndex(1);
  }
}

void FundAllocation::process_fund_tables(const QVector<TableFund*>& tables) {
  make_chart1(tables);
  make_chart2(tables);
}

void FundAllocation::on_chart_selection(const bool& state) {
  if (state) {
    if (radio_chart1->isChecked()) {
      stackedwidget->setCurrentIndex(0);
    } else if (radio_chart2->isChecked()) {
      stackedwidget->setCurrentIndex(1);
    }
  }
}

void FundAllocation::make_chart1(const QVector<TableFund*>& tables) {
  clear_chart(chart1);

  chart1->setTitle("Net Balance Contribution");

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

  chart1->addSeries(series);
}

void FundAllocation::make_chart2(const QVector<TableFund*>& tables) {
  clear_chart(chart2);

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

  chart2->setTitle("Net Balance History");

  chart2->addSeries(series);

  auto axis_x = new QBarCategoryAxis();

  axis_x->append(categories);

  chart2->addAxis(axis_x, Qt::AlignBottom);

  series->attachAxis(axis_x);

  auto axis_y = new QValueAxis();

  axis_y->setTitleText(QLocale().currencySymbol());
  axis_y->setTitleFont(serif_font);
  axis_y->setLabelFormat("%.2f");

  chart2->addAxis(axis_y, Qt::AlignLeft);

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
