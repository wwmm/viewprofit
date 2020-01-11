#ifndef FUND_ALLOCATION_HPP
#define FUND_ALLOCATION_HPP

#include <QSqlDatabase>
#include "callout.hpp"
#include "table_fund.hpp"
#include "ui_fund_allocation.h"

class FundAllocation : public QWidget, protected Ui::FundAllocation {
  Q_OBJECT
 public:
  explicit FundAllocation(const QSqlDatabase& database, QWidget* parent = nullptr);

  void process_fund_tables(const QVector<TableFund*>& tables);

 private:
  QSqlDatabase db;

  QChart *chart1, *chart2;

  Callout *callout1, *callout2;

  void make_chart1(const QVector<TableFund*>& tables);
  void make_chart2(const QVector<TableFund*>& tables);

  void on_chart_selection(const bool& state);
};

#endif