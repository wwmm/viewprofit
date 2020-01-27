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

  QVector<int> dates;
  QVector<double> values;

  auto query = QSqlQuery(db);

  query.prepare("select distinct date,net_return_perc from " + combo_fund->currentText() + " order by date desc");

  if (query.exec()) {
    while (query.next() && dates.size() < spinbox_months->value()) {
      dates.append(query.value(0).toInt());
      values.append(query.value(1).toDouble());
    }
  }

  if (dates.size() == 0) {
    return;
  }

  std::reverse(dates.begin(), dates.end());
  std::reverse(values.begin(), values.end());

  for (auto& table : tables) {
    if (table->name != combo_fund->currentText()) {
      auto query = QSqlQuery(db);
      QVector<double> tvalues(dates.size(), 0.0);
      int count = 0;

      for (auto& date : dates) {
        auto qdt = QDateTime();

        qdt.setSecsSinceEpoch(date);

        query.prepare("select net_return_perc from " + table->name +
                      " where strftime('%m/%Y', date(\"date\",'unixepoch'))=?");

        query.addBindValue(qdt.toString("MM/yyyy"));

        if (query.exec()) {
          if (query.next()) {
            tvalues[count] = query.value(0).toDouble();

            qDebug(qdt.toString("MM/yyyy").toUtf8());
          }
        } else {
          qDebug(table->model->lastError().text().toUtf8());
        }

        count++;
      }
    }
  }
}