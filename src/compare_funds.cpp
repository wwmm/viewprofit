#include "compare_funds.hpp"
#include "chart_funcs.hpp"
#include "effects.hpp"

CompareFunds::CompareFunds(const QSqlDatabase& database, QWidget* parent)
    : db(database), chart(new QChart()), callout(new Callout(chart)) {
  setupUi(this);

  callout->hide();

  // shadow effects

  frame_chart->setGraphicsEffect(card_shadow());
  chart_cfg_frame->setGraphicsEffect(card_shadow());
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

  connect(radio_net_return_perc, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_accumulated_net_return_perc, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
}

void CompareFunds::make_chart_net_return(const QVector<TableFund*>& tables) {
  chart->setTitle("Monthly Net Return");

  add_axes_to_chart(chart, "%");

  for (auto& table : tables) {
    auto s = add_series_to_chart(chart, table->model, table->name.toUpper(), "net_return_perc");

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
}

void CompareFunds::make_chart_accumulated_net_return(const QVector<TableFund*>& tables) {
  chart->setTitle("Accumulated Net Return");

  add_axes_to_chart(chart, "%");

  for (auto& table : tables) {
    auto s = add_series_to_chart(chart, table->model, table->name.toUpper(), "accumulated_net_return_perc");

    connect(s, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout, s->name()); });
  }
}

void CompareFunds::process_fund_tables(const QVector<TableFund*>& tables) {
  last_tables = tables;

  clear_chart(chart);

  if (radio_net_return_perc->isChecked()) {
    make_chart_net_return(tables);
  } else if (radio_accumulated_net_return_perc->isChecked()) {
    make_chart_accumulated_net_return(tables);
  }
}

void CompareFunds::on_chart_selection(const bool& state) {
  if (state) {
    process_fund_tables(last_tables);
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
