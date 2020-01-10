#ifndef COMPARE_FUNDS_HPP
#define COMPARE_FUNDS_HPP

#include <QSqlDatabase>
#include "table_fund.hpp"
#include "ui_compare_funds.h"

class CompareFunds : public QWidget, protected Ui::CompareFunds {
  Q_OBJECT
 public:
  explicit CompareFunds(const QSqlDatabase& database, QWidget* parent = nullptr);

  void process_fund_tables(const QVector<TableFund*>& tables);

 signals:
  void getFundTablesName();

 private:
  QSqlDatabase db;

  QChart* chart1;
  QChart* chart2;

  QGraphicsDropShadowEffect* button_shadow();
  QGraphicsDropShadowEffect* card_shadow();

  void on_chart_selection(const bool& state);
};

#endif