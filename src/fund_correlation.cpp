#include "fund_correlation.hpp"
#include "chart_funcs.hpp"
#include "effects.hpp"
#include "math.hpp"

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

void FundCorrelation::process(const QVector<TableFund const*>& tables, TablePortfolio const* portfolio) {
  this->tables = tables;
  this->portfolio = portfolio;

  const auto current_text = combo_fund->currentText();

  combo_fund->disconnect();
  combo_fund->clear();

  for (auto& table : tables) {
    combo_fund->addItem(table->name);
  }

  combo_fund->addItem(this->portfolio->name);

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

void FundCorrelation::process_fund_tables(const QVector<int>& dates) {
  QVector<double> values(dates.size(), 0.0);

  for (auto& table : tables) {
    if (table->name == combo_fund->currentText()) {
      int count = 0;

      for (auto& date : dates) {
        for (int n = 0; n < table->model->rowCount(); n++) {
          QString tdate = table->model->record(n).value("date").toString();

          auto qdt = QDateTime::fromString(tdate, "MM/yyyy");

          if (qdt.toSecsSinceEpoch() == date) {
            values[count] = table->model->record(n).value("net_return_perc").toDouble();
          }
        }

        count++;
      }

      break;
    }
  }

  for (auto& table : tables) {
    if (table->name != combo_fund->currentText()) {
      QVector<double> tvalues(dates.size(), 0.0);
      int count = 0;

      for (auto& date : dates) {
        for (int n = 0; n < table->model->rowCount(); n++) {
          QString tdate = table->model->record(n).value("date").toString();

          auto qdt = QDateTime::fromString(tdate, "MM/yyyy");

          if (qdt.toSecsSinceEpoch() == date) {
            tvalues[count] = table->model->record(n).value("net_return_perc").toDouble();
          }
        }

        count++;
      }

      auto s = add_series_to_chart(chart, dates, correlation_coefficient(values, tvalues), table->name);

      connect(s, &QLineSeries::hovered, this,
              [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
    }
  }

  // portfolio

  QVector<double> portfolio_values(dates.size(), 0.0);
  int count = 0;

  for (auto& date : dates) {
    for (int n = 0; n < portfolio->model->rowCount(); n++) {
      QString tdate = portfolio->model->record(n).value("date").toString();

      auto qdt = QDateTime::fromString(tdate, "MM/yyyy");

      if (qdt.toSecsSinceEpoch() == date) {
        portfolio_values[count] = portfolio->model->record(n).value("net_return_perc").toDouble();
      }
    }

    count++;
  }

  auto s = add_series_to_chart(chart, dates, correlation_coefficient(values, portfolio_values), portfolio->name);

  connect(s, &QLineSeries::hovered, this,
          [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
}

void FundCorrelation::process_portfolio_table(const QVector<int>& dates) {
  QVector<double> values(dates.size(), 0.0);
  int count = 0;

  for (auto& date : dates) {
    for (int n = 0; n < portfolio->model->rowCount(); n++) {
      QString tdate = portfolio->model->record(n).value("date").toString();

      auto qdt = QDateTime::fromString(tdate, "MM/yyyy");

      if (qdt.toSecsSinceEpoch() == date) {
        values[count] = portfolio->model->record(n).value("net_return_perc").toDouble();
      }
    }

    count++;
  }

  for (auto& table : tables) {
    QVector<double> tvalues(dates.size(), 0.0);
    int count = 0;

    for (auto& date : dates) {
      for (int n = 0; n < table->model->rowCount(); n++) {
        QString tdate = table->model->record(n).value("date").toString();

        auto qdt = QDateTime::fromString(tdate, "MM/yyyy");

        if (qdt.toSecsSinceEpoch() == date) {
          tvalues[count] = table->model->record(n).value("net_return_perc").toDouble();
        }
      }

      count++;
    }

    auto s = add_series_to_chart(chart, dates, correlation_coefficient(values, tvalues), table->name);

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
}

void FundCorrelation::process_tables() {
  clear_chart(chart);

  const auto dates = get_unique_months_from_db(db, tables, spinbox_months->value());

  if (dates.empty()) {
    return;
  }

  chart->setTitle("Correlation Coefficient");

  add_axes_to_chart(chart, "");

  if (combo_fund->currentText() != portfolio->name) {
    process_fund_tables(dates);
  } else {
    process_portfolio_table(dates);
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