#include "fund_allocation.hpp"
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

  // chart 1 settings

  chart1->setTheme(QChart::ChartThemeLight);
  chart1->legend()->setAlignment(Qt::AlignRight);

  chart_view1->setChart(chart1);
  chart_view1->setRenderHint(QPainter::Antialiasing);

  // chart 2 settings

  chart2->setTheme(QChart::ChartThemeLight);
  chart2->legend()->setAlignment(Qt::AlignRight);

  chart_view2->setChart(chart2);
  chart_view2->setRenderHint(QPainter::Antialiasing);

  // signals

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

  chart1->setTitle("Net Balance");

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

  chart2->setTitle("Net Balance History");

  // add_axes_to_chart(chart1, "%");
  // add_axes_to_chart(chart2, "%");

  // for (auto& table : tables) {
  //   auto s1 = add_series_to_chart(chart1, table->model, table->name.toUpper(), "net_return_perc");
  //   auto s2 = add_series_to_chart(chart2, table->model, table->name.toUpper(), "accumulated_net_return_perc");

  //   connect(s1, &QLineSeries::hovered, this,
  //           [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout1, s1->name()); });

  //   connect(s2, &QLineSeries::hovered, this,
  //           [=](const QPointF& point, bool state) { on_chart_mouse_hover(point, state, callout2, s2->name()); });
  // }
}
