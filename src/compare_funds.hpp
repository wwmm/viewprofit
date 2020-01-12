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

 private:
  QSqlDatabase db;

  QChart* chart;

  Callout* callout;

  QVector<TableFund*> last_tables;

  void make_chart_net_return(const QVector<TableFund*>& tables);
  void make_chart_accumulated_net_return(const QVector<TableFund*>& tables);

  void on_chart_selection(const bool& state);
  void on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name);
};

#endif