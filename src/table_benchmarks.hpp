#ifndef TABLE_BENCHMARKS_HPP
#define TABLE_BENCHMARKS_HPP

#include "table_base.hpp"

class TableBenchmarks : public TableBase {
 public:
  explicit TableBenchmarks(QWidget* parent = nullptr);

  void init_model();

  void calculate() override;

 private:
  void show_chart();
};

#endif