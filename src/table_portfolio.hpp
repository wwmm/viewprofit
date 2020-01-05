#ifndef TABLE_PORTFOLIO_HPP
#define TABLE_PORTFOLIO_HPP

#include "table_base.hpp"

class TablePortfolio : public TableBase {
  Q_OBJECT
 public:
  TablePortfolio(QWidget* parent = nullptr);

  void init_model();

  void show_benchmark(const TableBase* btable);

  void calculate() override;

  void process_investment_tables(const QVector<TableBase*>& tables);

 signals:
  void getInvestmentTablesName();
  void getBenchmarkTables();

 private:
  void make_chart1();
  void make_chart2();
};

#endif