#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QGraphicsDropShadowEffect>
#include <QMainWindow>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include "table_portfolio.hpp"
#include "ui_main_window.h"

class MainWindow : public QMainWindow, private Ui::MainWindow {
  Q_OBJECT
 public:
  explicit MainWindow(QMainWindow* parent = nullptr);

 private:
  QSettings qsettings;

  QSqlDatabase db;

  QGraphicsDropShadowEffect* button_shadow();
  QGraphicsDropShadowEffect* card_shadow();

  TablePortfolio* load_portfolio_table();
  void load_inflation_table();
  void add_benchmark_table();
  void add_fund_table();
  void load_saved_tables();
  void save_table(const QStackedWidget* sw);
  void clear_table(const QStackedWidget* sw);
  void remove_table(QListWidget* lw, QStackedWidget* sw);

  void on_calculate_table_fund();
  void on_save_table_fund();
  void on_clear_table_fund();
  void on_remove_table_fund();

  void on_calculate_table_benchmark();
  void on_save_table_benchmark();
  void on_clear_table_benchmark();
  void on_remove_table_benchmark();

  void on_calculate_table_portfolio();
  void on_save_table_portfolio();
  void on_clear_table_portfolio();

  void on_listwidget_item_changed(QListWidgetItem* item, QListWidget* lw, QStackedWidget* sw);

  template <class T>
  T* load_table(const QString& name, QStackedWidget* sw, QListWidget* lw) {
    auto table = new T();

    table->set_database(db);
    table->name = name;
    table->init_model();

    sw->addWidget(table);

    lw->addItem(name.toUpper());

    auto added_item = lw->item(lw->count() - 1);

    added_item->setFlags(added_item->flags() | Qt::ItemIsEditable);

    return table;
  }
};

#endif