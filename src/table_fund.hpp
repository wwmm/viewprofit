#ifndef TABLE_FUND_HPP
#define TABLE_FUND_HPP

#include "table_base.hpp"

class TableFund : public TableBase {
  Q_OBJECT
 public:
  explicit TableFund(QWidget* parent = nullptr);

  void show_benchmark(const TableBase* btable);

  void init_model() override;
  void calculate() override;

 signals:
  void getBenchmarkTables();

 private:
  QSettings qsettings;

  double chart2_vmin = 0.0, chart2_vmax = 0.0;

  std::tuple<QVector<int>, QVector<double>, QVector<double>> process_benchmark(const QString& table_name) const;
  void make_chart1();
  void make_chart2();
};

#endif