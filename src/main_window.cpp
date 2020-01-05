#include "main_window.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QSqlRecord>
#include <QStandardPaths>
#include "table_benchmarks.hpp"
#include "table_investments.hpp"
#include "table_portfolio.hpp"

MainWindow::MainWindow(QMainWindow* parent) : QMainWindow(parent), qsettings(QSettings()) {
  QCoreApplication::setOrganizationName("wwmm");
  QCoreApplication::setApplicationName("ViewProfit");

  setupUi(this);

  // shadow effects

  tab_widget->setGraphicsEffect(card_shadow());
  button_add_investment->setGraphicsEffect(button_shadow());
  button_add_benchmark->setGraphicsEffect(button_shadow());

  // signals

  connect(button_add_benchmark, &QPushButton::clicked, this, &MainWindow::add_benchmark_table);
  connect(button_add_investment, &QPushButton::clicked, this, &MainWindow::add_investment_table);

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

      auto query = QSqlQuery(db);

      query.prepare(
          "create table if not exists portfolio (id integer primary key,date int,total_contribution real,"
          "real_bank_balance real,real_return real,real_return_perc real)");

      if (!query.exec()) {
        qDebug("Failed to create table portfolio. Maybe it already exists.");
      }

      // load the portfolio tab

      auto table = new TablePortfolio();

      table->set_database(db);
      table->init_model();

      connect(table, &TablePortfolio::getInvestmentTablesName, this, [=]() {
        auto tables = QVector<TableBase*>();

        for (int n = 1; n < tab_widget->count(); n++) {
          auto btable = dynamic_cast<TableBase*>(tab_widget->widget(n));

          if (btable->type == TableType::Investment) {
            tables.append(btable);
          }
        }

        table->process_investment_tables(tables);
      });

      connect(table, &TablePortfolio::getBenchmarkTables, this, [=]() {
        for (int n = 1; n < tab_widget->count(); n++) {
          auto btable = dynamic_cast<TableBase*>(tab_widget->widget(n));

          if (btable->type == TableType::Benchmark) {
            table->show_benchmark(btable);
          }
        }
      });

      tab_widget->addTab(table, table->name);

      load_saved_tables();

      table->calculate();  // it has to be called after loading the other tables
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

void MainWindow::add_benchmark_table() {
  auto name = QString("Benchmark%1").arg(tab_widget->count());

  auto query = QSqlQuery(db);

  query.prepare("create table " + name + " (id integer primary key," +
                " date int default (cast(strftime('%s','now') as int))," + " value real default 0.0," +
                " accumulated real default 0.0)");

  if (query.exec()) {
    load_table<TableBenchmarks>(name);

    tab_widget->setCurrentIndex(tab_widget->count() - 1);
  } else {
    qDebug("Failed to create table " + name.toUtf8() + ". Maybe it already exists.");
  }
}

void MainWindow::add_investment_table() {
  auto name = QString("Investment%1").arg(tab_widget->count());

  auto query = QSqlQuery(db);

  query.prepare(
      "create table " + name + " (id integer primary key," + " date int default (cast(strftime('%s','now') as int))," +
      " contribution real default 0.0," + " bank_balance real default 0.0," + " total_contribution real default 0.0," +
      " gross_return real default 0.0," + " gross_return_perc real default 0.0," + " real_return real default 0.0," +
      " real_return_perc real default 0.0," + " real_bank_balance real default 0.0)");

  if (query.exec()) {
    load_table<TableInvestments>(name);

    tab_widget->setCurrentIndex(tab_widget->count() - 1);
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

      if (name != "portfolio") {
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
        for (int n = 1; n < tab_widget->count(); n++) {
          auto btable = dynamic_cast<TableBase*>(tab_widget->widget(n));

          if (btable->type == TableType::Benchmark) {
            table->show_benchmark(btable);
          }
        }
      });
    }

    for (auto& name : benchmarks) {
      load_table<TableBenchmarks>(name);
    }

    for (int n = 1; n < tab_widget->count(); n++) {
      auto btable = dynamic_cast<TableBase*>(tab_widget->widget(n));

      btable->calculate();
    }
  } else {
    qDebug("Failed to get table names!");
  }
}
