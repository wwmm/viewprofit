#ifndef FUND_CORRELATION_HPP
#define FUND_CORRELATION_HPP

#include <QSqlDatabase>
#include "callout.hpp"
#include "table_fund.hpp"
#include "ui_fund_correlation.h"

class FundCorrelation : public QWidget, protected Ui::FundCorrelation {
  Q_OBJECT
 public:
  explicit FundCorrelation(const QSqlDatabase& database, QWidget* parent = nullptr);

  void process(const QVector<TableFund*>& tables);

 private:
  QSqlDatabase db;

  QChart* chart;

  Callout* callout;

  QVector<TableFund*> tables;

  void process_tables();

  void on_chart_selection(const bool& state);
  void on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name);
};

#endif