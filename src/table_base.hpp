#ifndef TABLE_BASE_HPP
#define TABLE_BASE_HPP

#include <QLocale>
#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QTableView>
#include <QtCharts>
#include "callout.hpp"
#include "model.hpp"
#include "table_type.hpp"
#include "ui_table_base.h"

class TableBase : public QWidget, protected Ui::TableBase {
  Q_OBJECT
 public:
  explicit TableBase(QWidget* parent = nullptr);

  QString name;
  TableType type;
  Model* model;

  void set_database(const QSqlDatabase& database);
  void set_chart1_title(const QString& title);
  void set_chart2_title(const QString& title);
  void clear_charts();

  virtual void init_model() = 0;

 signals:
  void hideProgressBar();
  void newChartMouseHover(const QPointF& point);

 protected:
  QSqlDatabase db;

  QChart* const chart1;
  QChart* const chart2;

  Callout* const callout1;
  Callout* const callout2;

  auto eventFilter(QObject* object, QEvent* event) -> bool override;
  void remove_selected_rows();
  void reset_zoom();
  void calculate_accumulated_sum(const QString& column_name);
  void calculate_accumulated_product(const QString& column_name);

  static void on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name);
  void on_chart_selection(const bool& state);

 private:
  QLocale locale;

  void on_add_row();
};

#endif