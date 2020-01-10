#ifndef TABLE_PORTFOLIO_HPP
#define TABLE_PORTFOLIO_HPP

#include "table_benchmarks.hpp"
#include "table_fund.hpp"

class TablePortfolio : public TableBase {
  Q_OBJECT
 public:
  explicit TablePortfolio(QWidget* parent = nullptr);

  void show_benchmark(const TableBenchmarks* btable);
  void process_fund_tables(const QVector<TableFund*>& tables);

  void init_model() override;
  void calculate();

 signals:
  void getFundTablesName();
  void getBenchmarkTables();

 private:
  void make_chart1();
  void make_chart2();
};

#endif