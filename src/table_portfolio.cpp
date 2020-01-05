#include "table_portfolio.hpp"
#include <QSqlError>
#include <QSqlQuery>

TablePortfolio::TablePortfolio(QWidget* parent) {
  type = TableType::Portfolio;

  name = "portfolio";

  investment_cfg_frame->hide();
  label_table_name->hide();
  lineedit_table_name->hide();
  button_update_name->hide();
  button_add_row->hide();
  button_remove_row->hide();
  button_save_table->hide();

  button_remove_table->disconnect();
  button_remove_table->setText("Clear Table");

  connect(button_remove_table, &QPushButton::clicked, this, [&]() {
    auto query = QSqlQuery(db);

    query.prepare("delete from " + name);

    if (query.exec()) {
      model->select();

      clear_charts();
    } else {
      qDebug(model->lastError().text().toUtf8());
    }
  });
}

void TablePortfolio::init_model() {
  model->setTable(name);
  model->setEditStrategy(QSqlTableModel::OnManualSubmit);
  model->setSort(1, Qt::DescendingOrder);

  auto currency = QLocale().currencySymbol();

  model->setHeaderData(1, Qt::Horizontal, "Date");
  model->setHeaderData(2, Qt::Horizontal, "Total Contribution " + currency);
  model->setHeaderData(3, Qt::Horizontal, "Real Bank Balance " + currency);
  model->setHeaderData(4, Qt::Horizontal, "Real Return " + currency);
  model->setHeaderData(5, Qt::Horizontal, "Real Return %");

  model->select();

  table_view->setModel(model);
  table_view->setColumnHidden(0, true);
}

void TablePortfolio::calculate() {
  emit getInvestmentTablesName();
}

void TablePortfolio::process_investment_tables(const QVector<TableBase*>& tables) {
  QSet<int> list_set;

  for (auto& table : tables) {
    // making sure all the latest data was saved to the database

    table->model->submitAll();

    auto query = QSqlQuery(db);

    query.prepare("select distinct date from " + table->name + " order by date");

    if (query.exec()) {
      while (query.next()) {
        list_set.insert(query.value(0).toInt());
      }
    } else {
      qDebug(table->model->lastError().text().toUtf8());
    }
  }

  if (list_set.size() == 0) {
    return;
  }

  QList<int> list_dates = list_set.values();

  std::sort(list_dates.begin(), list_dates.end());

  for (auto& date : list_dates) {
    double total_contribution = 0.0;
    double real_bank_balance = 0.0;

    for (auto& table : tables) {
      auto query = QSqlQuery(db);

      /*
        345600 seconds = 4 days. If the time difference is smaller than that we consider that the dates
        refer to the same period. In other words they will show the same values in the Portfolio tab
      */

      query.prepare("select distinct total_contribution,real_bank_balance from " + table->name +
                    " where abs(date - ?) < 345600");

      query.addBindValue(date);

      if (query.exec()) {
        while (query.next()) {
          total_contribution += query.value(0).toDouble();
          real_bank_balance += query.value(1).toDouble();
        }
      } else {
        qDebug(table->model->lastError().text().toUtf8());
      }
    }

    double real_return = real_bank_balance - total_contribution;
    double real_return_perc = 100 * real_return / total_contribution;

    auto query = QSqlQuery(db);

    query.prepare("insert or replace into " + name + " values ((select id from " + name +
                  " where date == ?),?,?,?,?,?)");

    query.addBindValue(date);
    query.addBindValue(date);
    query.addBindValue(total_contribution);
    query.addBindValue(real_bank_balance);
    query.addBindValue(real_return);
    query.addBindValue(real_return_perc);

    if (!query.exec()) {
      qDebug(model->lastError().text().toUtf8());
    }
  }

  model->select();

  clear_charts();

  make_chart1();
  make_chart2();
}

void TablePortfolio::make_chart1() {
  chart1->setTitle(name);

  add_axes_to_chart(chart1, QLocale().currencySymbol());
  add_series_to_chart(chart1, model, "Total Contribution", "total_contribution");
  add_series_to_chart(chart1, model, "Real Bank Balance", "real_bank_balance");
}

void TablePortfolio::make_chart2() {
  chart2->setTitle(name);

  add_axes_to_chart(chart2, "%");
  add_series_to_chart(chart2, model, "Real Return", "real_return_perc");

  // ask the main window class for the benchmarks

  emit getBenchmarkTables();
}

void TablePortfolio::show_benchmark(const TableBase* btable) {
  add_series_to_chart(chart2, btable->model, btable->name, "accumulated");
}