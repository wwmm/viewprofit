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
  void add_investment_table();
  void load_saved_tables();

  void on_listwidget_item_clicked(int currentRow);
  void on_listwidget_item_changed(QListWidgetItem* item);
  void on_remove_table();
  void on_clear_table();
  void on_save_table_to_database();

  template <class T>
  T* load_table(const QString& name) {
    auto table = new T();

    table->set_database(db);
    table->name = name;
    table->init_model();

    stackedwidget->addWidget(table);

    listwidget_tables->addItem(name.toUpper());

    auto added_item = listwidget_tables->item(listwidget_tables->count() - 1);

    added_item->setFlags(added_item->flags() | Qt::ItemIsEditable);

    return table;
  }
};

#endif