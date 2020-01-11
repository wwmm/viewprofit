#include "compare_funds.hpp"
#include "chart_funcs.hpp"

CompareFunds::CompareFunds(const QSqlDatabase& database, QWidget* parent)
    : db(database),
      chart1(new QChart()),
      chart2(new QChart()),
      callout1(new Callout(chart1)),
      callout2(new Callout(chart2)) {
  setupUi(this);

  // shadow effects

  frame_chart->setGraphicsEffect(card_shadow());
  chart_cfg_frame->setGraphicsEffect(card_shadow());
  button_reset_zoom->setGraphicsEffect(button_shadow());

  // chart 1 settings

  chart1->setTheme(QChart::ChartThemeLight);
  chart1->setAcceptHoverEvents(true);

  chart_view1->setChart(chart1);
  chart_view1->setRenderHint(QPainter::Antialiasing);
  chart_view1->setRubberBand(QChartView::RectangleRubberBand);

  // chart 2 settings

  chart2->setTheme(QChart::ChartThemeLight);
  chart2->setAcceptHoverEvents(true);
  chart2->legend()->setShowToolTips(true);
  // chart2->legend()->detachFromChart();
  // chart2->legend()->setAlignment(Qt::AlignTop);

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

  connect(radio_chart1, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);
  connect(radio_chart2, &QRadioButton::toggled, this, &CompareFunds::on_chart_selection);

  // select the default chart

  if (radio_chart1->isChecked()) {
    stackedwidget->setCurrentIndex(0);
  } else if (radio_chart2->isChecked()) {
    stackedwidget->setCurrentIndex(1);
  }
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
  clear_chart(chart1);
  clear_chart(chart2);

  chart1->setTitle("Monthly Net Return");
  chart2->setTitle("Accumulated Net Return");

  add_axes_to_chart(chart1, "%");
  add_axes_to_chart(chart2, "%");

  for (auto& table : tables) {
    auto s1 = add_series_to_chart(chart1, table->model, table->name.toUpper(), "net_return_perc");
    auto s2 = add_series_to_chart(chart2, table->model, table->name.toUpper(), "accumulated_net_return_perc");

    connect(s1, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout1, table->name); });

    connect(s2, &QLineSeries::hovered, this,
            [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout2, table->name); });
  }
}

void CompareFunds::on_chart_selection(const bool& state) {
  if (state) {
    if (radio_chart1->isChecked()) {
      stackedwidget->setCurrentIndex(0);
    } else if (radio_chart2->isChecked()) {
      stackedwidget->setCurrentIndex(1);
    }
  }
}

void CompareFunds::on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name) {
  if (state) {
    auto qdt = QDateTime();

    qdt.setMSecsSinceEpoch(point.x());

    c->setText(QString("%1\n\nDate: %2\ny: %3").arg(name, qdt.toString("dd/MM/yyyy"), QString::number(point.y())));

    c->setAnchor(point);

    c->setZValue(11);

    c->updateGeometry();

    c->show();
  } else {
    c->hide();
  }
}