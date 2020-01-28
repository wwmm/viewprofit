#include "fund_pca.hpp"
#include <Eigen/Dense>
#include <QSqlError>
#include <QSqlQuery>
#include <iostream>
#include "chart_funcs.hpp"
#include "effects.hpp"

FundPCA::FundPCA(const QSqlDatabase& database, QWidget* parent)
    : db(database), chart(new QChart()), callout(new Callout(chart)) {
  setupUi(this);

  callout->hide();

  // shadow effects

  frame_chart->setGraphicsEffect(card_shadow());
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

void FundPCA::process(const QVector<TableFund*>& tables) {
  this->tables = tables;

  process_tables();
}

void FundPCA::process_tables() {
  clear_chart(chart);

  chart->setTitle("Pricipal Component Analysis");

  add_axes_to_chart(chart, "");

  Eigen::MatrixXd data = Eigen::MatrixXd::Zero(tables.size(), spinbox_months->value());

  for (int k = 0; k < tables.size(); k++) {
    auto query = QSqlQuery(db);
    QVector<double> values;

    query.prepare("select net_return_perc from " + tables[k]->name + " order by date desc");

    if (query.exec()) {
      while (query.next() && values.size() < spinbox_months->value()) {
        values.append(query.value(0).toDouble());
      }
    }

    if (values.size() == 0) {
      break;
    }

    for (int n = 0; n < values.size(); n++) {
      data(k, n) = values[n];
    }
  }

  std::cout << data << std::endl;
}

void FundPCA::on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name) {
  if (state) {
    auto qdt = QDateTime();

    qdt.setMSecsSinceEpoch(point.x());

    c->setText(QString("Fund: %1\nDate: %2").arg(name, qdt.toString("dd/MM/yyyy")));

    c->setAnchor(point);

    c->setZValue(11);

    c->updateGeometry();

    c->show();
  } else {
    c->hide();
  }
}