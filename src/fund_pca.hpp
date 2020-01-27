#ifndef FUND_PCA_HPP
#define FUND_PCA_HPP

#include <QSqlDatabase>
#include "callout.hpp"
#include "table_fund.hpp"
#include "ui_fund_pca.h"

class FundPCA : public QWidget, protected Ui::FundPCA {
  Q_OBJECT
 public:
  explicit FundPCA(const QSqlDatabase& database, QWidget* parent = nullptr);

  void process(const QVector<TableFund*>& tables);

 private:
  QSqlDatabase db;

  QChart* chart;

  Callout* callout;

  QVector<TableFund*> tables;

  void process_tables();

  void on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name);
};

#endif