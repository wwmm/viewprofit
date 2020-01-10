#include "chart_funcs.hpp"

void clear_chart(QChart* chart) {
  chart->removeAllSeries();

  for (auto& axis : chart->axes()) {
    chart->removeAxis(axis);
  }
}