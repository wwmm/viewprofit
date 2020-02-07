#ifndef COMPARE_FUNDS_HPP
#define COMPARE_FUNDS_HPP

#include <QSqlDatabase>
#include <deque>
#include "callout.hpp"
#include "table_fund.hpp"
#include "table_portfolio.hpp"
#include "ui_compare_funds.h"

class CompareFunds : public QWidget, protected Ui::CompareFunds {
  Q_OBJECT
 public:
  explicit CompareFunds(const QSqlDatabase& database, QWidget* parent = nullptr);

  void process(const QVector<TableFund const*>& tables, TablePortfolio const* portfolio);

 private:
  QSqlDatabase db;

  QChart* const chart;

  Callout* const callout;

  QVector<TableFund const*> tables;

  TablePortfolio const* portfolio;

  void process_tables();

  void make_chart_net_balance_pie();
  void make_chart_net_return_pie();
  void make_chart_net_return();
  void make_chart_net_return_volatility();
  void make_chart_accumulated_net_return_pie();
  void make_chart_accumulated_net_return();
  void make_chart_accumulated_net_return_second_derivative();
  void make_chart_barseries(const QString& series_name, const QString& column_name);

  void make_pie(std::deque<QPair<QString, double>>& deque);

  void on_chart_selection(const bool& state);
  void on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name);
};

#endif