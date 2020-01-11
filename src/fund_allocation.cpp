#include "fund_allocation.hpp"
#include <QSqlError>
#include <QSqlQuery>
#include "chart_funcs.hpp"
#include "effects.hpp"

FundAllocation::FundAllocation(const QSqlDatabase& database, QWidget* parent)
    : db(database),
      chart1(new QChart()),
      chart2(new QChart()),
      callout1(new Callout(chart1)),
      callout2(new Callout(chart2)) {
  setupUi(this);

  callout1->hide();
  callout2->hide();

  // shadow effects

  frame_chart->setGraphicsEffect(card_shadow());
  chart_cfg_frame->setGraphicsEffect(card_shadow());
  button_reset_zoom->setGraphicsEffect(button_shadow());

  // chart 1 settings

  chart1->setTheme(QChart::ChartThemeLight);
  chart1->legend()->setAlignment(Qt::AlignRight);

  chart_view1->setChart(chart1);
  chart_view1->setRenderHint(QPainter::Antialiasing);
  chart_view1->setRubberBand(QChartView::RectangleRubberBand);

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

  for (auto& table : tables) {
    double max_net_balance = 0.0;

    for (int n = 0; n < table->model->rowCount(); n++) {
      double v = table->model->record(n).value("net_balance").toDouble();

      max_net_balance = std::max(max_net_balance, v);
    }

    series->append(table->name, max_net_balance);
  }

  for (auto& slice : series->slices()) {
    slice->setLabelVisible(true);

    auto label = slice->label();

    slice->setLabel(QString("%1 %2%").arg(label, QString::number(100 * slice->percentage(), 'f', 2)));
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
      callout2->setText(QString("Fund: %1\nDate: %2\nReturn: " + QLocale().currencySymbol() + " %3")
                            .arg(barset->label(), categories[index], QString::number(barset->at(index), 'f', 2)));

      double v = barset->at(index);

      for (int n = 1; n < barsets.size(); n++) {
        if (barsets[n] == barset) {
          for (int m = 0; m < n; m++) {
            v += barsets[m]->at(index);
          }

          break;
        }
      }

      callout2->setAnchor(QPointF(index, v));

      callout2->setZValue(11);

      callout2->updateGeometry();

      callout2->show();
    } else {
      callout2->hide();
    }
  });
}
