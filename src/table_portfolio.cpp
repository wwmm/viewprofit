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
  model->setHeaderData(4, Qt::Horizontal, "Net Bank Balance " + currency);
  model->setHeaderData(5, Qt::Horizontal, "Net Return " + currency);
  model->setHeaderData(6, Qt::Horizontal, "Net Return %");
  model->setHeaderData(7, Qt::Horizontal, "Real Return %");

  model->select();

  table_view->setModel(model);
  table_view->setColumnHidden(0, true);
}

void TablePortfolio::calculate() {
  emit getInvestmentTablesName();
}

void TablePortfolio::process_investment_tables(const QVector<TableBase*>& tables) {
  QSet<int> list_set;
  QVector<QPair<int, double>> inflation_vec;

  // get inflation values
  auto query = QSqlQuery(db);

  query.prepare("select distinct date,accumulated from inflation order by date desc");

  if (query.exec()) {
    while (query.next()) {
      inflation_vec.append(QPair(query.value(0).toInt(), query.value(1).toDouble()));
    }
  } else {
    qDebug("Failed to get inflation table values!");
  }

  // get date values in the investment tables

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

  auto qdt = QDateTime();

  for (auto& date : list_dates) {
    double total_deposit = 0.0, total_withdrawal = 0.0, net_bank_balance = 0.0;

    qdt.setSecsSinceEpoch(date);

    QString date_month = qdt.toString("MM/yyyy");

    for (auto& table : tables) {
      for (int n = 0; n < table->model->rowCount(); n++) {
        QString tdate = table->model->record(n).value("date").toString();

        if (date_month == QDate::fromString(tdate, "dd/MM/yyyy").toString("MM/yyyy")) {
          total_deposit += table->model->record(n).value("total_deposit").toDouble();
          total_withdrawal += table->model->record(n).value("total_withdrawal").toDouble();
          net_bank_balance += table->model->record(n).value("net_bank_balance").toDouble();

          break;
        }
      }
    }

    double net_return = net_bank_balance - (total_deposit - total_withdrawal);
    double net_return_perc = 100 * net_return / (total_deposit - total_withdrawal);

    double real_return_perc = 0.0;

    for (auto& p : inflation_vec) {
      qdt.setSecsSinceEpoch(p.first);

      if (qdt.toString("MM/yyyy") == date_month) {
        real_return_perc = 100.0 * (net_return_perc - p.second) / (100.0 + p.second);

        break;
      }
    }

    auto query = QSqlQuery(db);

    query.prepare("insert or replace into " + name + " values ((select id from " + name +
                  " where date == ?),?,?,?,?,?,?,?)");

    query.addBindValue(date);
    query.addBindValue(date);
    query.addBindValue(total_deposit);
    query.addBindValue(total_withdrawal);
    query.addBindValue(net_bank_balance);
    query.addBindValue(net_return);
    query.addBindValue(net_return_perc);
    query.addBindValue(real_return_perc);

    if (!query.exec()) {
      qDebug(model->lastError().text().toUtf8());
    }
  }

  model->select();

  if (model->rowCount() > 0) {
    clear_charts();

    make_chart1();
    make_chart2();
  }
}

void TablePortfolio::make_chart1() {
  chart1->setTitle(name.toUpper());

  add_axes_to_chart(chart1, QLocale().currencySymbol());
  add_series_to_chart(chart1, model, "Deposits", "deposit");
  add_series_to_chart(chart1, model, "Net Bank Balance", "net_bank_balance");

  bool show_withdrawals = false;

  for (int n = 0; n < model->rowCount(); n++) {
    double withdrawal = model->record(n).value("withdrawal").toDouble();

    if (fabs(withdrawal) > 0) {
      show_withdrawals = true;

      break;
    }
  }

  if (show_withdrawals) {
    add_series_to_chart(chart1, model, "Withdrawals", "withdrawal");
  }
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