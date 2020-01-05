#ifndef TABLE_INVESTMENTS_HPP
#define TABLE_INVESTMENTS_HPP

#include "table_base.hpp"

class TableInvestments : public TableBase {
  Q_OBJECT
 public:
  explicit TableInvestments(QWidget* parent = nullptr);

  void init_model();

  void show_benchmark(const TableBase* btable);

  void calculate() override;

 signals:
  void getBenchmarkTables();

 private:
  QSettings qsettings;

  double chart2_vmin = 0.0, chart2_vmax = 0.0;

  void calculate_total_contribution();
  void make_chart1();
  void make_chart2();
};

#endif