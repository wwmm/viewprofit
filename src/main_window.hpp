#ifndef MAIN_WINDOW_HPP
#define MAIN_WINDOW_HPP

#include <QGraphicsDropShadowEffect>
#include <QMainWindow>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
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

  void add_benchmark_table();
  void add_investment_table();
  void load_saved_tables();

  void on_listwidget_item_clicked(int currentRow);
  void on_remove_table();

  template <class T>
  T* load_table(const QString& name) {
    auto table = new T();

    table->set_database(db);
    table->name = name;
    table->init_model();

    connect(table, &T::tableNameChanged, this, [=](QString new_name) {
      // finish any pending operation before changing the table name

      table->model->submitAll();

      auto query = QSqlQuery(db);

      query.prepare("alter table " + table->name + " rename to " + new_name);

      if (query.exec()) {
        table->name = new_name;

        listwidget_tables->currentItem()->setText(new_name.toUpper());

        table->init_model();
      } else {
        qDebug("failed to rename table " + name.toUtf8());
      }
    });

    // tab_widget->addTab(table, name);
    stackedwidget->addWidget(table);

    listwidget_tables->addItem(name.toUpper());

    return table;
  }
};

#endif