#ifndef TABLE_PORTFOLIO_HPP
#define TABLE_PORTFOLIO_HPP

#include "table_benchmarks.hpp"
#include "table_fund.hpp"

class TablePortfolio : public TableBase {
  Q_OBJECT
 public:
  explicit TablePortfolio(QWidget* parent = nullptr);

  void show_benchmark(const TableBenchmarks* btable);
  void process_fund_tables(const QVector<TableFund const*>& tables);

  void init_model() override;

 signals:
  void getBenchmarkTables();

 private:
  int perc_chart_oldest_date = 0;

  [[nodiscard]] auto process_benchmark(const QString& table_name, const int& oldest_date) const
      -> std::tuple<QVector<int>, QVector<double>, QVector<double>>;

  void make_chart1();
  void make_chart2();
};

#endif