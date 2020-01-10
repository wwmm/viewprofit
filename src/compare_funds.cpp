#include "compare_funds.hpp"
#include "chart_funcs.hpp"

CompareFunds::CompareFunds(const QSqlDatabase& database, QWidget* parent)
    : db(database), chart1(new QChart()), chart2(new QChart()) {
  setupUi(this);

  // shadow effects

  frame_chart1->setGraphicsEffect(card_shadow());
  frame_chart2->setGraphicsEffect(card_shadow());
  button_chart2_reset_zoom->setGraphicsEffect(button_shadow());

  // chart 1 settings

  chart1->setTheme(QChart::ChartThemeLight);
  chart1->setAcceptHoverEvents(true);

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

  connect(button_chart2_reset_zoom, &QPushButton::clicked, this, [&]() { chart2->zoomReset(); });
}

QGraphicsDropShadowEffect* CompareFunds::button_shadow() {
  auto effect = new QGraphicsDropShadowEffect(this);

  effect->setColor(QColor(0, 0, 0, 100));
  effect->setXOffset(1);
  effect->setYOffset(1);
  effect->setBlurRadius(5);

  return effect;
}

QGraphicsDropShadowEffect* CompareFunds::card_shadow() {
  auto effect = new QGraphicsDropShadowEffect(this);

  effect->setColor(QColor(0, 0, 0, 100));
  effect->setXOffset(2);
  effect->setYOffset(2);
  effect->setBlurRadius(5);

  return effect;
}

void CompareFunds::process_fund_tables(const QVector<TableFund*>& tables) {
  clear_chart(chart2);

  chart2->setTitle("Monthly Net Return");

  add_axes_to_chart(chart2, "%");

  for (auto& table : tables) {
    // add_series_to_chart(chart2, table->model, table->name.toUpper(), "accumulated_net_return_perc");
    add_series_to_chart(chart2, table->model, table->name.toUpper(), "net_return_perc");
  }
}