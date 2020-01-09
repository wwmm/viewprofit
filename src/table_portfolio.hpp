#ifndef TABLE_PORTFOLIO_HPP
#define TABLE_PORTFOLIO_HPP

#include "table_base.hpp"

class TablePortfolio : public TableBase {
  Q_OBJECT
 public:
  explicit TablePortfolio(QWidget* parent = nullptr);

  void show_benchmark(const TableBase* btable);
  void process_fund_tables(const QVector<TableBase*>& tables);

  void init_model() override;
  void calculate() override;

 signals:
  void getFundTablesName();
  void getBenchmarkTables();

 private:
  void make_chart1();
  void make_chart2();
};

#endif