#ifndef COMPARE_FUNDS_HPP
#define COMPARE_FUNDS_HPP

#include <QSqlDatabase>
#include "table_fund.hpp"
#include "ui_compare_funds.h"

class CompareFunds : public QWidget, protected Ui::CompareFunds {
  Q_OBJECT
 public:
  explicit CompareFunds(const QSqlDatabase& database, QWidget* parent = nullptr);

  void calculate();
  void process_fund_tables(const QVector<TableFund*>& tables);

 signals:
  void getFundTablesName();

 private:
  QSqlDatabase db;

  QGraphicsDropShadowEffect* button_shadow();
  QGraphicsDropShadowEffect* card_shadow();
};

#endif