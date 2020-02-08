#include "main_window.hpp"
#include <QCoreApplication>
#include <QDir>
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include "effects.hpp"
#include "table_benchmarks.hpp"
#include "table_fund.hpp"

MainWindow::MainWindow(QMainWindow* parent) : QMainWindow(parent), qsettings(QSettings()) {
  QCoreApplication::setOrganizationName("wwmm");
  QCoreApplication::setApplicationName("ViewProfit");

  setupUi(this);

  tab_widget->setCurrentIndex(0);

  // shadow effects

  frame_portfolio_pages->setGraphicsEffect(card_shadow());
  button_calculate_table_portfolio->setGraphicsEffect(button_shadow());
  button_clear_table_portfolio->setGraphicsEffect(button_shadow());
  button_save_table_portfolio->setGraphicsEffect(button_shadow());

  frame_table_selection_fund->setGraphicsEffect(card_shadow());
  button_add_fund->setGraphicsEffect(button_shadow());
  button_remove_table_fund->setGraphicsEffect(button_shadow());
  button_clear_table_fund->setGraphicsEffect(button_shadow());
  button_save_table_fund->setGraphicsEffect(button_shadow());
  button_calculate_table_fund->setGraphicsEffect(button_shadow());

  button_add_benchmark->setGraphicsEffect(button_shadow());
  frame_table_selection_benchmark->setGraphicsEffect(card_shadow());
  button_remove_table_benchmark->setGraphicsEffect(button_shadow());
  button_clear_table_benchmark->setGraphicsEffect(button_shadow());
  button_save_table_benchmark->setGraphicsEffect(button_shadow());
  button_calculate_table_benchmark->setGraphicsEffect(button_shadow());

  button_database_file->setGraphicsEffect(button_shadow());

  // signals

  connect(button_calculate_table_portfolio, &QPushButton::clicked, this, &MainWindow::on_calculate_portfolio);
  connect(button_save_table_portfolio, &QPushButton::clicked, this, &MainWindow::on_save_table_portfolio);
  connect(button_clear_table_portfolio, &QPushButton::clicked, this, &MainWindow::on_clear_table_portfolio);

  connect(button_add_benchmark, &QPushButton::clicked, this, &MainWindow::add_benchmark_table);
  connect(button_remove_table_benchmark, &QPushButton::clicked, this, &MainWindow::on_remove_table_benchmark);
  connect(button_clear_table_benchmark, &QPushButton::clicked, this, &MainWindow::on_clear_table_benchmark);
  connect(button_save_table_benchmark, &QPushButton::clicked, this, &MainWindow::on_save_table_benchmark);
  connect(button_calculate_table_benchmark, &QPushButton::clicked, this, [&]() {
    auto table =
        dynamic_cast<TableBenchmarks*>(stackedwidget_benchmarks->widget(stackedwidget_benchmarks->currentIndex()));

    table->calculate();
  });

  connect(button_add_fund, &QPushButton::clicked, this, &MainWindow::add_fund_table);
  connect(button_remove_table_fund, &QPushButton::clicked, this, &MainWindow::on_remove_table_fund);
  connect(button_clear_table_fund, &QPushButton::clicked, this, &MainWindow::on_clear_table_fund);
  connect(button_save_table_fund, &QPushButton::clicked, this, &MainWindow::on_save_table_fund);
  connect(button_calculate_table_fund, &QPushButton::clicked, this, [&]() {
    auto table = dynamic_cast<TableFund*>(stackedwidget_funds->widget(stackedwidget_funds->currentIndex()));

    table->calculate();
  });

  connect(button_database_file, &QPushButton::clicked, this, [&]() {
    auto path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDesktopServices::openUrl(path);
  });

  connect(listwidget_portfolio, &QListWidget::currentRowChanged, this,
          [&](int currentRow) { stackedwidget_portfolio->setCurrentIndex(currentRow); });

  connect(listwidget_tables_benchmarks, &QListWidget::currentRowChanged, this,
          [&](int currentRow) { stackedwidget_benchmarks->setCurrentIndex(currentRow); });
  connect(listwidget_tables_benchmarks, &QListWidget::itemChanged, this, [&](QListWidgetItem* item) {
    on_listwidget_item_changed(item, listwidget_tables_benchmarks, stackedwidget_benchmarks);
  });

  connect(listwidget_tables_funds, &QListWidget::currentRowChanged, this,
          [&](int currentRow) { stackedwidget_funds->setCurrentIndex(currentRow); });
  connect(listwidget_tables_funds, &QListWidget::itemChanged, this, [&](QListWidgetItem* item) {
    on_listwidget_item_changed(item, listwidget_tables_funds, stackedwidget_funds);
  });

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

    qDebug() << "Database file: " + path.toLatin1();

    db.setDatabaseName(path);

    if (db.open()) {
      qDebug("The database file was opened!");

      load_inflation_table();

      load_saved_tables();

      auto fund_tables = QVector<TableFund const*>();

      for (int n = 0; n < stackedwidget_funds->count(); n++) {
        fund_tables.append(dynamic_cast<TableFund const*>(stackedwidget_funds->widget(n)));
      }

      auto portfolio_table = load_portfolio_table();
      auto cf = load_compare_funds();
      auto fc = load_fund_correlation();
      auto fpca = load_fund_pca();

      // This has to be done after loading the other tables

      portfolio_table->process_fund_tables(fund_tables);
      cf->process(fund_tables, portfolio_table);
      fc->process(fund_tables, portfolio_table);
      fpca->process(fund_tables);
    } else {
      qCritical("Failed to open the database file!");
    }
  }

  show();
}

auto MainWindow::load_portfolio_table() -> TablePortfolio* {
  auto query = QSqlQuery(db);

  query.prepare(
      "create table if not exists portfolio"
      " (id integer primary key, date int default (cast(strftime('%s','now') as int)),"
      " deposit real default 0.0, withdrawal real default 0.0, starting_balance real default 0.0,"
      " ending_balance real default 0.0, accumulated_deposit real default 0.0, accumulated_withdrawal real default 0.0,"
      " net_deposit real default 0.0, net_balance real default 0.0, net_return real default 0.0,"
      " net_return_perc real default 0.0, accumulated_net_return real default 0.0, "
      " accumulated_net_return_perc real default 0.0, real_return_perc real default 0.0,"
      " accumulated_real_return_perc real default 0.0)");

  if (!query.exec()) {
    qDebug("Failed to create table portfolio. Maybe it already exists.");
  }

  auto table = new TablePortfolio();

  table->set_database(db);
  table->init_model();

  connect(table, &TablePortfolio::getBenchmarkTables, this, [=]() {
    for (int n = 0; n < stackedwidget_benchmarks->count(); n++) {
      auto btable = dynamic_cast<TableBenchmarks*>(stackedwidget_benchmarks->widget(n));

      table->show_benchmark(btable);
    }
  });

  stackedwidget_portfolio->addWidget(table);

  listwidget_portfolio->addItem("Table");
  listwidget_portfolio->setCurrentRow(0);

  return table;
}

auto MainWindow::load_compare_funds() -> CompareFunds* {
  auto cf = new CompareFunds(db);

  stackedwidget_portfolio->addWidget(cf);

  listwidget_portfolio->addItem("Compare Funds");

  return cf;
}

auto MainWindow::load_fund_correlation() -> FundCorrelation* {
  auto fc = new FundCorrelation(db);

  stackedwidget_portfolio->addWidget(fc);

  listwidget_portfolio->addItem("Fund Correlation");

  return fc;
}

auto MainWindow::load_fund_pca() -> FundPCA* {
  auto fpca = new FundPCA(db);

  stackedwidget_portfolio->addWidget(fpca);

  listwidget_portfolio->addItem("Fund PCA");

  return fpca;
}

void MainWindow::load_inflation_table() {
  auto query = QSqlQuery(db);

  query.prepare(
      "create table if not exists inflation (id integer primary key,"
      " date int default (cast(strftime('%s','now') as int)),value real default 0.0,accumulated real default 0.0)");

  if (query.exec()) {
    auto* table = new TableBenchmarks();

    table->set_database(db);
    table->name = "inflation";
    table->init_model();

    stackedwidget_benchmarks->addWidget(table);

    listwidget_tables_benchmarks->addItem("inflation");

    table->calculate();
  } else {
    qDebug("Failed to create table inflation. Maybe it already exists.");
  }
}

void MainWindow::add_benchmark_table() {
  auto name = QString("Benchmark%1").arg(stackedwidget_benchmarks->count());

  auto query = QSqlQuery(db);

  query.prepare("create table " + name + " (id integer primary key," +
                " date int default (cast(strftime('%s','now') as int))," + " value real default 0.0," +
                " accumulated real default 0.0)");

  if (query.exec()) {
    load_table<TableBenchmarks>(name, stackedwidget_benchmarks, listwidget_tables_benchmarks);

    stackedwidget_benchmarks->setCurrentIndex(stackedwidget_benchmarks->count() - 1);

    listwidget_tables_benchmarks->setCurrentRow(listwidget_tables_benchmarks->count() - 1);
  } else {
    qDebug() << "Failed to create table " + name.toUtf8() + ". Maybe it already exists.";
  }
}

void MainWindow::add_fund_table() {
  auto name = QString("Investment%1").arg(stackedwidget_funds->count());

  auto query = QSqlQuery(db);

  query.prepare(
      "create table " + name + " (id integer primary key, date int default (cast(strftime('%s','now') as int))," +
      " deposit real default 0.0, withdrawal real default 0.0, starting_balance real default 0.0," +
      " ending_balance real default 0.0, accumulated_deposit real default 0.0, accumulated_withdrawal real default 0.0,"
      " net_deposit real default 0.0, net_balance real default 0.0, net_return real default 0.0," +
      " net_return_perc real default 0.0, accumulated_net_return real default 0.0, "
      " accumulated_net_return_perc real default 0.0, real_return_perc real default 0.0," +
      " accumulated_real_return_perc real default 0.0)");

  if (query.exec()) {
    auto table = load_table<TableFund>(name, stackedwidget_funds, listwidget_tables_funds);

    stackedwidget_funds->setCurrentIndex(stackedwidget_funds->count() - 1);

    listwidget_tables_funds->setCurrentRow(listwidget_tables_funds->count() - 1);

    connect(table, &TableFund::getBenchmarkTables, this, [=]() {
      for (int n = 0; n < stackedwidget_benchmarks->count(); n++) {
        auto btable = dynamic_cast<TableBenchmarks*>(stackedwidget_benchmarks->widget(n));

        table->show_benchmark(btable);
      }
    });
  } else {
    qDebug() << "Failed to create table " + name.toUtf8() + ". Maybe it already exists.";
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
      qInfo() << "Found table: " + name.toUtf8();

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

    for (auto& name : investments) {
      auto table = load_table<TableFund>(name, stackedwidget_funds, listwidget_tables_funds);

      connect(table, &TableFund::getBenchmarkTables, this, [=]() {
        for (int n = 0; n < stackedwidget_benchmarks->count(); n++) {
          auto btable = dynamic_cast<TableBenchmarks*>(stackedwidget_benchmarks->widget(n));

          table->show_benchmark(btable);
        }
      });
    }

    for (auto& name : benchmarks) {
      load_table<TableBenchmarks>(name, stackedwidget_benchmarks, listwidget_tables_benchmarks);
    }

    for (int n = 0; n < stackedwidget_benchmarks->count(); n++) {
      auto table = dynamic_cast<TableBenchmarks*>(stackedwidget_benchmarks->widget(n));

      table->calculate();
    }

    for (int n = 0; n < stackedwidget_funds->count(); n++) {
      auto table = dynamic_cast<TableFund*>(stackedwidget_funds->widget(n));

      table->calculate();
    }

    if (listwidget_tables_funds->count() > 0) {
      listwidget_tables_funds->setCurrentRow(0);
    }

    if (listwidget_tables_benchmarks->count() > 0) {
      listwidget_tables_benchmarks->setCurrentRow(0);
    }
  } else {
    qDebug("Failed to get table names!");
  }
}

void MainWindow::on_listwidget_item_changed(QListWidgetItem* item, QListWidget* lw, QStackedWidget* sw) {
  if (item == lw->currentItem()) {
    QString new_name = item->text();

    auto table = dynamic_cast<TableBase*>(sw->widget(sw->currentIndex()));

    // finish any pending operation before changing the table name

    table->model->submitAll();

    auto query = QSqlQuery(db);

    query.prepare("alter table " + table->name + " rename to " + new_name);

    if (query.exec()) {
      table->name = new_name;

      lw->currentItem()->setText(new_name.toUpper());

      table->init_model();

      table->set_chart1_title(new_name);
      table->set_chart2_title(new_name);
    } else {
      qDebug() << "failed to rename table " + table->name.toUtf8();
    }
  }
}

void MainWindow::remove_table(QListWidget* lw, QStackedWidget* sw) {
  if (lw->currentRow() < 1) {
    return;
  }

  auto box = QMessageBox(this);

  box.setText("Remove the selected table from the database?");
  box.setInformativeText("This action cannot be undone!");
  box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  box.setDefaultButton(QMessageBox::Yes);

  auto r = box.exec();

  if (r == QMessageBox::Yes) {
    auto table = dynamic_cast<TableBase*>(sw->widget(sw->currentIndex()));

    sw->removeWidget(sw->widget(sw->currentIndex()));

    auto it = lw->takeItem(lw->currentRow());

    delete it;

    qsettings.beginGroup(table->name);

    qsettings.remove("");

    qsettings.endGroup();

    auto query = QSqlQuery(db);

    query.prepare("drop table if exists " + table->name);

    if (!query.exec()) {
      qDebug() << "Failed remove table " + table->name.toUtf8() + ". Maybe has already been removed.";
    }
  }
}

void MainWindow::on_remove_table_fund() {
  remove_table(listwidget_tables_funds, stackedwidget_funds);
}

void MainWindow::on_remove_table_benchmark() {
  remove_table(listwidget_tables_benchmarks, stackedwidget_benchmarks);
}

void MainWindow::clear_table(const QStackedWidget* sw) {
  if (sw->count() < 1) {
    return;
  }

  auto box = QMessageBox(this);

  box.setText("Remove all rows from the selected table?");
  box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  box.setDefaultButton(QMessageBox::Yes);

  auto r = box.exec();

  if (r == QMessageBox::Yes) {
    auto table = dynamic_cast<TableBase*>(sw->widget(sw->currentIndex()));

    auto query = QSqlQuery(db);

    query.prepare("delete from " + table->name);

    if (query.exec()) {
      table->model->select();

      table->clear_charts();
    } else {
      qDebug() << table->model->lastError().text().toUtf8();
    }
  }
}

void MainWindow::on_clear_table_fund() {
  clear_table(stackedwidget_funds);
}

void MainWindow::on_clear_table_benchmark() {
  clear_table(stackedwidget_benchmarks);
}

void MainWindow::on_clear_table_portfolio() {
  auto box = QMessageBox(this);

  box.setText("Clear the portfolio table?");
  box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
  box.setDefaultButton(QMessageBox::Yes);

  auto r = box.exec();

  if (r == QMessageBox::Yes) {
    auto table = dynamic_cast<TableBase*>(stackedwidget_portfolio->widget(stackedwidget_portfolio->currentIndex()));

    auto query = QSqlQuery(db);

    query.prepare("delete from " + table->name);

    if (query.exec()) {
      table->model->select();

      table->clear_charts();
    } else {
      qDebug() << table->model->lastError().text().toUtf8();
    }
  }
}

void MainWindow::on_save_table_portfolio() {
  auto table = dynamic_cast<TableBase*>(stackedwidget_portfolio->widget(0));

  if (!table->model->submitAll()) {
    qDebug() << "failed to save table " + table->name.toUtf8() + " to the database";

    qDebug() << table->model->lastError().text().toUtf8();
  }
}

void MainWindow::save_table(const QStackedWidget* sw) {
  auto table = dynamic_cast<TableBase*>(sw->widget(sw->currentIndex()));

  if (!table->model->submitAll()) {
    qDebug() << "failed to save table " + table->name.toUtf8() + " to the database";

    qDebug() << table->model->lastError().text().toUtf8();
  }
}

void MainWindow::on_save_table_fund() {
  save_table(stackedwidget_funds);
}

void MainWindow::on_save_table_benchmark() {
  save_table(stackedwidget_benchmarks);
}

void MainWindow::on_calculate_portfolio() {
  auto fund_tables = QVector<TableFund const*>();

  for (int n = 0; n < stackedwidget_funds->count(); n++) {
    fund_tables.append(dynamic_cast<TableFund const*>(stackedwidget_funds->widget(n)));
  }

  auto portfolio_table = dynamic_cast<TablePortfolio*>(stackedwidget_portfolio->widget(0));

  portfolio_table->process_fund_tables(fund_tables);

  auto cf = dynamic_cast<CompareFunds*>(stackedwidget_portfolio->widget(1));

  cf->process(fund_tables, portfolio_table);

  auto fc = dynamic_cast<FundCorrelation*>(stackedwidget_portfolio->widget(2));

  fc->process(fund_tables, portfolio_table);

  auto fpca = dynamic_cast<FundPCA*>(stackedwidget_portfolio->widget(3));

  fpca->process(fund_tables);
}