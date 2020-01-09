#ifndef COMPARE_FUNDS_HPP
#define COMPARE_FUNDS_HPP

#include <QSqlDatabase>
#include "ui_compare_funds.h"

class CompareFunds : public QWidget, protected Ui::CompareFunds {
  Q_OBJECT
 public:
  explicit CompareFunds(const QSqlDatabase& database, QWidget* parent = nullptr);

 signals:
  void getFundTablesName();

 private:
  QSqlDatabase db;
};

#endif