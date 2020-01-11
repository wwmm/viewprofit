#ifndef TABLE_BASE_HPP
#define TABLE_BASE_HPP

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
  QChart *chart1, *chart2;

  Callout *callout1, *callout2;

  QGraphicsDropShadowEffect* button_shadow();
  QGraphicsDropShadowEffect* card_shadow();

  bool eventFilter(QObject* object, QEvent* event);
  void remove_selected_rows();
  void reset_zoom();
  void calculate_accumulated_sum(const QString& column_name);
  void calculate_accumulated_product(const QString& column_name);

  void on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name);
  void on_chart_selection(const bool& state);

 private:
  void on_add_row();
};

#endif