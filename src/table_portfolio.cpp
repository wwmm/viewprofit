#include "table_portfolio.hpp"
#include <QSqlError>
#include <QSqlQuery>

TablePortfolio::TablePortfolio(QWidget* parent) {
  type = TableType::Portfolio;

  name = "portfolio";

  investment_cfg_frame->hide();
  button_add_row->hide();
}

void TablePortfolio::init_model() {
  model->setTable(name);
  model->setEditStrategy(QSqlTableModel::OnManualSubmit);
  model->setSort(1, Qt::DescendingOrder);

  auto currency = QLocale().currencySymbol();

  model->setHeaderData(1, Qt::Horizontal, "Date");
  model->setHeaderData(2, Qt::Horizontal, "Deposit " + currency);
  model->setHeaderData(3, Qt::Horizontal, "Withdrawal " + currency);
  model->setHeaderData(4, Qt::Horizontal, "Starting\n Balance " + currency);
  model->setHeaderData(5, Qt::Horizontal, "Ending\n Balance " + currency);
  model->setHeaderData(6, Qt::Horizontal, "Accumulated\nDeposits " + currency);
  model->setHeaderData(7, Qt::Horizontal, "Accumulated\nWithdrawals " + currency);
  model->setHeaderData(8, Qt::Horizontal, "Net\nDeposit " + currency);
  model->setHeaderData(9, Qt::Horizontal, "Net\nBalance " + currency);
  model->setHeaderData(10, Qt::Horizontal, "Net\nReturn " + currency);
  model->setHeaderData(11, Qt::Horizontal, "Net\nReturn %");
  model->setHeaderData(12, Qt::Horizontal, "Accumulated\nNet Return " + currency);
  model->setHeaderData(13, Qt::Horizontal, "Accumulated\nNet Return %");
  model->setHeaderData(14, Qt::Horizontal, "Real\nReturn %");
  model->setHeaderData(15, Qt::Horizontal, "Accumulated\nReal Return %");

  model->select();

  table_view->setModel(model);
  table_view->setColumnHidden(0, true);
}

void TablePortfolio::calculate() {
  emit getInvestmentTablesName();
}

void TablePortfolio::process_investment_tables(const QVector<TableBase*>& tables) {
  // get date values in each investment tables

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

  // calculate columns

  auto qdt = QDateTime();

  for (auto& date : list_dates) {
    double deposit = 0.0, withdrawal = 0.0, starting_balance = 0.0, ending_balance = 0.0, accumulated_deposit = 0.0,
           accumulated_withdrawal = 0.0, net_deposit = 0.0, net_balance = 0.0, net_return = 0.0,
           accumulated_net_return = 0.0;

    qdt.setSecsSinceEpoch(date);

    QString date_month = qdt.toString("MM/yyyy");

    for (auto& table : tables) {
      for (int n = 0; n < table->model->rowCount(); n++) {
        QString tdate = table->model->record(n).value("date").toString();

        if (date_month == QDate::fromString(tdate, "dd/MM/yyyy").toString("MM/yyyy")) {
          deposit += table->model->record(n).value("deposit").toDouble();
          withdrawal += table->model->record(n).value("withdrawal").toDouble();
          starting_balance += table->model->record(n).value("starting_balance").toDouble();
          ending_balance += table->model->record(n).value("ending_balance").toDouble();
          accumulated_deposit += table->model->record(n).value("accumulated_deposit").toDouble();
          accumulated_withdrawal += table->model->record(n).value("accumulated_withdrawal").toDouble();
          net_deposit += table->model->record(n).value("net_deposit").toDouble();
          net_balance += table->model->record(n).value("net_balance").toDouble();
          net_return += table->model->record(n).value("net_return").toDouble();
          accumulated_net_return += table->model->record(n).value("accumulated_net_return").toDouble();

          break;
        }
      }
    }

    double net_return_perc = 100 * net_return / (starting_balance + deposit - withdrawal);

    double real_return_perc = net_return_perc;

    // get inflation values so we can update real_return_perc

    QVector<int> inflation_dates;
    QVector<double> inflation_values, inflation_accumulated;

    auto query = QSqlQuery(db);

    query.prepare("select distinct date,value,accumulated from inflation order by date desc");

    if (query.exec()) {
      while (query.next()) {
        inflation_dates.append(query.value(0).toInt());
        inflation_values.append(query.value(1).toDouble());
        inflation_accumulated.append(query.value(2).toDouble());
      }
    } else {
      qDebug("Failed to get inflation table values!");
    }

    for (int i = 0; i < inflation_dates.size(); i++) {
      qdt.setSecsSinceEpoch(inflation_dates[i]);

      if (qdt.toString("MM/yyyy") == date_month) {
        real_return_perc = 100.0 * (net_return_perc - inflation_values[i]) / (100.0 + inflation_values[i]);

        break;
      }
    }

    double accumulated_net_return_perc = 0, accumulated_real_return_perc = 0;

    query = QSqlQuery(db);

    query.prepare("insert or replace into " + name + " values ((select id from " + name +
                  " where date == ?),?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)");

    query.addBindValue(date);
    query.addBindValue(date);
    query.addBindValue(deposit);
    query.addBindValue(withdrawal);
    query.addBindValue(starting_balance);
    query.addBindValue(ending_balance);
    query.addBindValue(accumulated_deposit);
    query.addBindValue(accumulated_withdrawal);
    query.addBindValue(net_deposit);
    query.addBindValue(net_balance);
    query.addBindValue(net_return);
    query.addBindValue(net_return_perc);
    query.addBindValue(accumulated_net_return);
    query.addBindValue(accumulated_net_return_perc);
    query.addBindValue(real_return_perc);
    query.addBindValue(accumulated_real_return_perc);

    if (!query.exec()) {
      qDebug(model->lastError().text().toUtf8());
    }
  }

  model->select();

  if (model->rowCount() > 0) {
    calculate_accumulated_sum("net_return");
    calculate_accumulated_product("net_return_perc");
    calculate_accumulated_product("real_return_perc");

    clear_charts();

    make_chart1();
    make_chart2();
  }
}

void TablePortfolio::make_chart1() {
  chart1->setTitle(name.toUpper());

  add_axes_to_chart(chart1, QLocale().currencySymbol());
  add_series_to_chart(chart1, model, "Deposits", "net_deposit");
  add_series_to_chart(chart1, model, "Net Balance", "net_balance");
  add_series_to_chart(chart1, model, "Net Return", "accumulated_net_return");
}

void TablePortfolio::make_chart2() {
  chart2->setTitle(name.toUpper());

  add_axes_to_chart(chart2, "%");
  add_series_to_chart(chart2, model, "Net Return", "net_return_perc");
  add_series_to_chart(chart2, model, "Real Return", "real_return_perc");

  // ask the main window class for the benchmarks

  emit getBenchmarkTables();
}

void TablePortfolio::show_benchmark(const TableBase* btable) {
  add_series_to_chart(chart2, btable->model, btable->name, "accumulated");
}