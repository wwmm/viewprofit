#include "fund_correlation.hpp"
#include <QSqlError>
#include <QSqlQuery>
#include "chart_funcs.hpp"
#include "effects.hpp"

FundCorrelation::FundCorrelation(const QSqlDatabase& database, QWidget* parent)
    : db(database), chart(new QChart()), callout(new Callout(chart)) {
  setupUi(this);

  callout->hide();

  // shadow effects

  frame_chart->setGraphicsEffect(card_shadow());
  frame_fund_selection->setGraphicsEffect(card_shadow());
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
  connect(spinbox_months, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) { process_tables(); });
}

void FundCorrelation::process(const QVector<TableFund*>& tables) {
  this->tables = tables;

  auto current_text = combo_fund->currentText();

  combo_fund->disconnect();
  combo_fund->clear();

  for (auto& table : tables) {
    combo_fund->addItem(table->name);
  }

  for (int n = 0; n < combo_fund->count(); n++) {
    if (combo_fund->itemText(n) == current_text) {
      combo_fund->setCurrentIndex(n);

      break;
    }
  }

  connect(combo_fund, QOverload<const QString&>::of(&QComboBox::currentIndexChanged), this,
          [&]() { process_tables(); });

  process_tables();
}

void FundCorrelation::process_tables() {
  clear_chart(chart);

  auto dates = get_unique_months_from_db(db, tables, spinbox_months->value());

  if (dates.size() == 0) {
    return;
  }

  chart->setTitle("Correlation Coefficient");

  add_axes_to_chart(chart, "");

  QVector<double> values(dates.size(), 0.0);
  int count = 0;

  for (auto& date : dates) {
    auto query = QSqlQuery(db);

    query.prepare("select net_return_perc from " + combo_fund->currentText() +
                  " where strftime('%m/%Y', \"date\",'unixepoch')=?");

    auto qdt = QDateTime::fromSecsSinceEpoch(date);

    query.addBindValue(qdt.toString("MM/yyyy"));

    if (query.exec()) {
      if (query.next()) {
        values[count] = query.value(0).toDouble();
      }
    }

    count++;
  }

  for (auto& table : tables) {
    if (table->name != combo_fund->currentText()) {
      QVector<double> tvalues(dates.size(), 0.0);
      QVector<double> correlation(dates.size(), 0.0);
      int count = 0;

      for (auto& date : dates) {
        auto query = QSqlQuery(db);

        query.prepare("select net_return_perc from " + table->name +
                      " where strftime('%m/%Y', \"date\",'unixepoch')=?");

        auto qdt = QDateTime::fromSecsSinceEpoch(date);

        query.addBindValue(qdt.toString("MM/yyyy"));

        if (query.exec()) {
          if (query.next()) {
            tvalues[count] = query.value(0).toDouble();
          }
        }

        count++;
      }

      // calculating the Pearson correlation coefficient https://en.wikipedia.org/wiki/Pearson_correlation_coefficient

      for (int n = 0; n < correlation.size(); n++) {
        double avg = 0.0, tavg = 0.0;

        for (int m = 0; m <= n; m++) {
          avg += values[m];
          tavg += tvalues[m];
        }

        avg /= (n + 1);
        tavg /= (n + 1);

        double variance = 0.0, tvariance = 0.0;

        for (int m = 0; m <= n; m++) {
          variance += (values[m] - avg) * (values[m] - avg);
          tvariance += (tvalues[m] - tavg) * (tvalues[m] - tavg);
        }

        double stddev = std::sqrt(variance);
        double tstddev = std::sqrt(tvariance);

        for (int m = 0; m <= n; m++) {
          correlation[n] += (values[m] - avg) * (tvalues[m] - tavg);
        }

        if (stddev > 0.001 && tstddev > 0.001) {
          correlation[n] /= (stddev * tstddev);
        }
      }

      auto s = add_series_to_chart(chart, dates, correlation, table->name);

      connect(s, &QLineSeries::hovered, this,
              [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
    }
  }

  chart->axes(Qt::Vertical)[0]->setRange(-1.0, 1.0);
}

void FundCorrelation::on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name) {
  if (state) {
    auto qdt = QDateTime();

    qdt.setMSecsSinceEpoch(point.x());

    c->setText(QString("Fund: %1\nDate: %2\nCorrelation: %3")
                   .arg(name, qdt.toString("MM/yyyy"), QString::number(point.y(), 'f', 2)));

    c->setAnchor(point);

    c->setZValue(11);

    c->updateGeometry();

    c->show();
  } else {
    c->hide();
  }
}