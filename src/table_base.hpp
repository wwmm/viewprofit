#ifndef TABLE_BASE_HPP
#define TABLE_BASE_HPP

#include <QSqlDatabase>
#include <QSqlRecord>
#include <QSqlTableModel>
#include <QTableView>
#include <QtCharts>
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
  virtual void calculate() = 0;

 signals:
  void hideProgressBar();
  void newChartMouseHover(const QPointF& point);

 protected:
  QSqlDatabase db;
  QChart* chart1;
  QChart* chart2;

  QGraphicsDropShadowEffect* button_shadow();
  QGraphicsDropShadowEffect* card_shadow();

  bool eventFilter(QObject* object, QEvent* event);
  void remove_selected_rows();
  void save_image();
  void reset_zoom();
  void calculate_accumulated_sum(const QString& column_name);
  void calculate_accumulated_product(const QString& column_name);

  void on_chart_mouse_hover(const QPointF& point, bool state);
  void on_chart_selection(const bool& state);

  template <typename T>
  void add_axes_to_chart(T* chart, const QString& ytitle) const {
    auto axis_x = new QDateTimeAxis();

    QFont serif_font("Sans");

    axis_x->setTitleText("Date");
    axis_x->setFormat("dd/MM/yyyy");
    axis_x->setLabelsAngle(-10);
    axis_x->setTitleFont(serif_font);

    auto axis_y = new QValueAxis();

    axis_y->setTitleText(ytitle);
    axis_y->setLabelFormat("%.2f");
    axis_y->setTitleFont(serif_font);

    chart->addAxis(axis_x, Qt::AlignBottom);
    chart->addAxis(axis_y, Qt::AlignLeft);
  }

  template <typename T>
  void add_series_to_chart(T* chart,
                           const Model* tmodel,
                           const QString& series_name,
                           const QString& column_name) const {
    if (chart->axes().size() == 0) {
      return;
    }

    auto series = new QLineSeries();

    series->setName(series_name.toLower());

    connect(series, &QLineSeries::hovered, this, &TableBase::on_chart_mouse_hover);

    double vmin = static_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0])->min();
    double vmax = static_cast<QValueAxis*>(chart->axes(Qt::Vertical)[0])->max();

    for (int n = 0; n < tmodel->rowCount(); n++) {
      auto qdt = QDateTime::fromString(tmodel->record(n).value("date").toString(), "dd/MM/yyyy");

      auto epoch_in_ms = qdt.toMSecsSinceEpoch();

      double v = tmodel->record(n).value(column_name).toDouble();

      if (chart->series().size() > 0) {
        vmin = std::min(vmin, v);
        vmax = std::max(vmax, v);
      } else {
        if (n == 0) {
          vmin = v;
          vmax = v;
        } else {
          vmin = std::min(vmin, v);
          vmax = std::max(vmax, v);
        }
      }

      series->append(epoch_in_ms, v);
    }

    chart->addSeries(series);

    series->attachAxis(chart->axes(Qt::Horizontal)[0]);
    series->attachAxis(chart->axes(Qt::Vertical)[0]);

    chart->axes(Qt::Vertical)[0]->setRange(vmin - 0.05 * fabs(vmin), vmax + 0.05 * fabs(vmax));
  }

 private:
  void on_add_row();
};

#endif