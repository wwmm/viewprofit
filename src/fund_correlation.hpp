#ifndef FUND_CORRELATION_HPP
#define FUND_CORRELATION_HPP

#include <QSqlDatabase>
#include "callout.hpp"
#include "table_fund.hpp"
#include "table_portfolio.hpp"
#include "ui_fund_correlation.h"

class FundCorrelation : public QWidget, protected Ui::FundCorrelation {
  Q_OBJECT
 public:
  explicit FundCorrelation(const QSqlDatabase& database, QWidget* parent = nullptr);

  void process(const QVector<TableFund const*>& tables, TablePortfolio const* portfolio);

 private:
  QSqlDatabase db;

  QChart* const chart;

  Callout* const callout;

  QVector<TableFund const*> tables;

  TablePortfolio const* portfolio = nullptr;

  void process_tables();
  void process_fund_tables(const QVector<int>& dates);
  void process_portfolio_table(const QVector<int>& dates);

  static void on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name);
};

#endif