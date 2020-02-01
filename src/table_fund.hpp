#ifndef TABLE_FUND_HPP
#define TABLE_FUND_HPP

#include "table_base.hpp"

class TableFund : public TableBase {
  Q_OBJECT
 public:
  explicit TableFund(QWidget* parent = nullptr);

  void show_benchmark(const TableBase* btable);

  void init_model() override;
  void calculate();

 signals:
  void getBenchmarkTables();

 private:
  QSettings qsettings;

  int perc_chart_oldest_date = 0;

  [[nodiscard]] auto process_benchmark(const QString& table_name, const int& oldest_date) const
      -> std::tuple<QVector<int>, QVector<double>, QVector<double>>;
  void make_chart1();
  void make_chart2();
};

#endif