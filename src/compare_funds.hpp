#ifndef COMPARE_FUNDS_HPP
#define COMPARE_FUNDS_HPP

#include <QSqlDatabase>
#include "callout.hpp"
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

  QChart *chart1, *chart2;

  Callout *callout1, *callout2;

  void on_chart_selection(const bool& state);
  void on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name);
};

#endif