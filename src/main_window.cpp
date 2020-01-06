#include "main_window.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include "table_benchmarks.hpp"
#include "table_investments.hpp"

MainWindow::MainWindow(QMainWindow* parent) : QMainWindow(parent), qsettings(QSettings()) {
  QCoreApplication::setOrganizationName("wwmm");
  QCoreApplication::setApplicationName("ViewProfit");

  setupUi(this);

  // shadow effects

  frame_table_selection->setGraphicsEffect(card_shadow());
  button_add_investment->setGraphicsEffect(button_shadow());
  button_add_benchmark->setGraphicsEffect(button_shadow());
  button_database_file->setGraphicsEffect(button_shadow());
  button_remove_table->setGraphicsEffect(button_shadow());
  button_clear_table->setGraphicsEffect(button_shadow());
  button_save_table->setGraphicsEffect(button_shadow());

  // signals

  connect(button_add_benchmark, &QPushButton::clicked, this, &MainWindow::add_benchmark_table);
  connect(button_add_investment, &QPushButton::clicked, this, &MainWindow::add_investment_table);
  connect(button_database_file, &QPushButton::clicked, this, [&]() {
    auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDesktopServices::openUrl(path);
  });
  connect(listwidget_tables, &QListWidget::currentRowChanged, this, &MainWindow::on_listwidget_item_clicked);
  connect(listwidget_tables, &QListWidget::itemChanged, this, &MainWindow::on_listwidget_item_changed);
  connect(button_remove_table, &QPushButton::clicked, this, &MainWindow::on_remove_table);
  connect(button_clear_table, &QPushButton::clicked, this, &MainWindow::on_clear_table);
  connect(button_save_table, &QPushButton::clicked, this, &MainWindow::on_save_table_to_database);

  // apply custom stylesheet

  QFile styleFile(":/custom.css");

  styleFile.open(QFile::ReadOnly);

  QString style(styleFile.readAll());

  setStyleSheet(style);

  styleFile.close();

  // load the sqlite database file

  if (!QSqlDatabase::isDriverAvailable("QSQLITE")) {
    qCritical("sqlite driver is not available!");
  } else {
    db = QSqlDatabase::addDatabase("QSQLITE");

    auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);

    if (!QDir(path).exists()) {
      QDir().mkpath(path);
    }

    path += "/viewprofit.sqlite";

    qDebug("Database file: " + path.toLatin1());

    db.setDatabaseName(path);

    if (db.open()) {
      qDebug("The database file was opened!");

      auto portfolio_table = load_portfolio_table();

      load_inflation_table();

      load_saved_tables();

      portfolio_table->calculate();  // it has to be called after loading the other tables
    } else {
      qCritical("Failed to open the database file!");
    }
  }

  show();
}

QGraphicsDropShadowEffect* MainWindow::button_shadow() {
  auto effect = new QGraphicsDropShadowEffect(this);

  effect->setColor(QColor(0, 0, 0, 100));
  effect->setXOffset(1);
  effect->setYOffset(1);
  effect->setBlurRadius(5);

  return effect;
}

QGraphicsDropShadowEffect* MainWindow::card_shadow() {
  auto effect = new QGraphicsDropShadowEffect(this);

  effect->setColor(QColor(0, 0, 0, 100));
  effect->setXOffset(2);
  effect->setYOffset(2);
  effect->setBlurRadius(5);

  return effect;
}

TablePortfolio* MainWindow::load_portfolio_table() {
  auto query = QSqlQuery(db);

  query.prepare(
      "create table if not exists portfolio (id integer primary key, date int, deposit real, withdrawal real,"
      " net_bank_balance real, net_return real, net_return_perc real, real_return_perc real)");

  if (!query.exec()) {
    qDebug("Failed to create table portfolio. Maybe it already exists.");
  }

  auto table = new TablePortfolio();

  table->set_database(db);
  table->init_model();

  connect(table, &TablePortfolio::getInvestmentTablesName, this, [=]() {
    auto tables = QVector<TableBase*>();

    for (int n = 0; n < stackedwidget->count(); n++) {
      auto btable = dynamic_cast<TableBase*>(stackedwidget->widget(n));

      if (btable->type == TableType::Investment) {
        tables.append(btable);
      }
    }

    table->process_investment_tables(tables);
  });

  connect(table, &TablePortfolio::getBenchmarkTables, this, [=]() {
    for (int n = 0; n < stackedwidget->count(); n++) {
      auto btable = dynamic_cast<TableBase*>(stackedwidget->widget(n));

      if (btable->type == TableType::Benchmark) {
        table->show_benchmark(btable);
      }
    }
  });

  stackedwidget->addWidget(table);

  listwidget_tables->addItem(table->name.toUpper());
  listwidget_tables->setCurrentRow(0);

  return table;
}

void MainWindow::load_inflation_table() {
  auto query = QSqlQuery(db);

  query.prepare(
      "create table if not exists inflation (id integer primary key,"
      " date int default (cast(strftime('%s','now') as int)),value real default 0.0,accumulated real default 0.0)");

  if (query.exec()) {
    auto table = new TableBenchmarks();

    table->set_database(db);
    table->name = "inflation";
    table->init_model();

    stackedwidget->addWidget(table);

    listwidget_tables->addItem("INFLATION");
  } else {
    qDebug("Failed to create table inflation. Maybe it already exists.");
  }
}

void MainWindow::add_benchmark_table() {
  auto name = QString("Benchmark%1").arg(stackedwidget->count());

  auto query = QSqlQuery(db);

  query.prepare("create table " + name + " (id integer primary key," +
                " date int default (cast(strftime('%s','now') as int))," + " value real default 0.0," +
                " accumulated real default 0.0)");

  if (query.exec()) {
    load_table<TableBenchmarks>(name);

    stackedwidget->setCurrentIndex(stackedwidget->count() - 1);

    listwidget_tables->setCurrentRow(listwidget_tables->count() - 1);
  } else {
    qDebug("Failed to create table " + name.toUtf8() + ". Maybe it already exists.");
  }
}

void MainWindow::add_investment_table() {
  auto name = QString("Investment%1").arg(stackedwidget->count());

  auto query = QSqlQuery(db);

  query.prepare(
      "create table " + name + " (id integer primary key, date int default (cast(strftime('%s','now') as int))," +
      " deposit real default 0.0, withdrawal real default 0.0, bank_balance real default 0.0," +
      " total_deposit real default 0.0, total_withdrawal real default 0.0, gross_return real default 0.0," +
      " gross_return_perc real default 0.0," + " net_return real default 0.0, net_return_perc real default 0.0," +
      " net_bank_balance real default 0.0," + " real_return_perc real default 0.0)");

  if (query.exec()) {
    auto table = load_table<TableInvestments>(name);

    stackedwidget->setCurrentIndex(stackedwidget->count() - 1);

    listwidget_tables->setCurrentRow(listwidget_tables->count() - 1);

    connect(table, &TableInvestments::getBenchmarkTables, this, [=]() {
      for (int n = 0; n < stackedwidget->count(); n++) {
        auto btable = dynamic_cast<TableBase*>(stackedwidget->widget(n));

        if (btable->type == TableType::Benchmark) {
          table->show_benchmark(btable);
        }
      }
    });
  } else {
    qDebug("Failed to create table " + name.toUtf8() + ". Maybe it already exists.");
  }
}

void MainWindow::load_saved_tables() {
  auto query = QSqlQuery(db);

  query.prepare("select name from sqlite_master where type='table'");

  if (query.exec()) {
    auto names = QVector<QString>();

    while (query.next()) {
      auto name = query.value(0).toString();

      if (name != "portfolio" && name != "inflation") {
        names.append(name);
      }
    }

    std::sort(names.begin(), names.end());

    auto benchmarks = QVector<QString>();
    auto investments = QVector<QString>();

    for (auto& name : names) {
      qInfo("Found table: " + name.toUtf8());

      auto query = QSqlQuery(db);

      query.prepare("select * from " + name);

      if (query.exec()) {
        int n_cols = query.record().count();

        if (n_cols == 4) {
          benchmarks.append(name);
        } else {
          investments.append(name);
        }
      }
    }

    // put investment tables first

    for (auto& name : investments) {
      auto table = load_table<TableInvestments>(name);

      connect(table, &TableInvestments::getBenchmarkTables, this, [=]() {
        for (int n = 0; n < stackedwidget->count(); n++) {
          auto btable = dynamic_cast<TableBase*>(stackedwidget->widget(n));

          if (btable->type == TableType::Benchmark) {
            table->show_benchmark(btable);
          }
        }
      });
    }

    for (auto& name : benchmarks) {
      load_table<TableBenchmarks>(name);
    }

    for (int n = 0; n < stackedwidget->count(); n++) {
      auto btable = dynamic_cast<TableBase*>(stackedwidget->widget(n));

      btable->calculate();
    }
  } else {
    qDebug("Failed to get table names!");
  }
}

void MainWindow::on_listwidget_item_clicked(int currentRow) {
  stackedwidget->setCurrentIndex(currentRow);
}

void MainWindow::on_listwidget_item_changed(QListWidgetItem* item) {
  if (item == listwidget_tables->currentItem()) {
    QString new_name = item->text();

    auto table = dynamic_cast<TableBase*>(stackedwidget->widget(stackedwidget->currentIndex()));

    // finish any pending operation before changing the table name

    table->model->submitAll();

    auto query = QSqlQuery(db);

    query.prepare("alter table " + table->name + " rename to " + new_name);

    if (query.exec()) {
      table->name = new_name;

      listwidget_tables->currentItem()->setText(new_name.toUpper());

      table->init_model();

      table->set_chart1_title(new_name);
      table->set_chart2_title(new_name);
    } else {
      qDebug("failed to rename table " + table->name.toUtf8());
    }
  }
}

void MainWindow::on_remove_table() {
  if (listwidget_tables->currentRow() < 2) {
    return;
  }

  auto box = QMessageBox(this);

  box.setText("Remove the selected table from the database?");
  box.setInformativeText("This action cannot be undone!");
  box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  box.setDefaultButton(QMessageBox::Yes);

  auto r = box.exec();

  if (r == QMessageBox::Yes) {
    auto table = dynamic_cast<TableBase*>(stackedwidget->widget(stackedwidget->currentIndex()));

    stackedwidget->removeWidget(stackedwidget->widget(stackedwidget->currentIndex()));

    auto it = listwidget_tables->takeItem(listwidget_tables->currentRow());

    if (it != nullptr) {
      delete it;
    }

    qsettings.beginGroup(table->name);

    qsettings.remove("");

    qsettings.endGroup();

    auto query = QSqlQuery(db);

    query.prepare("drop table if exists " + table->name);

    if (!query.exec()) {
      qDebug("Failed remove table " + table->name.toUtf8() + ". Maybe has already been removed.");
    }
  }
}

void MainWindow::on_clear_table() {
  if (listwidget_tables->count() == 0) {
    return;
  }

  auto box = QMessageBox(this);

  box.setText("Remove all rows from the selected table?");
  box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  box.setDefaultButton(QMessageBox::Yes);

  auto r = box.exec();

  if (r == QMessageBox::Yes) {
    auto table = dynamic_cast<TableBase*>(stackedwidget->widget(stackedwidget->currentIndex()));

    auto query = QSqlQuery(db);

    query.prepare("delete from " + table->name);

    if (query.exec()) {
      table->model->select();

      table->clear_charts();
    } else {
      qDebug(table->model->lastError().text().toUtf8());
    }
  }
}

void MainWindow::on_save_table_to_database() {
  auto table = dynamic_cast<TableBase*>(stackedwidget->widget(stackedwidget->currentIndex()));

  if (!table->model->submitAll()) {
    qDebug("failed to save table " + table->name.toUtf8() + " to the database");

    qDebug(table->model->lastError().text().toUtf8());
  }
}