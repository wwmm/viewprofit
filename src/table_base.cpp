#include "table_base.hpp"
#include <QGuiApplication>
#include <QSqlError>
#include <QSqlQuery>
#include <QSqlRecord>
#include "effects.hpp"

TableBase::TableBase(QWidget* parent)
    : QWidget(parent),
      chart1(new QChart()),
      chart2(new QChart()),
      callout1(new Callout(chart1)),
      callout2(new Callout(chart2)) {
  setupUi(this);

  callout1->hide();
  callout2->hide();

  table_view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
  table_view->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  table_view->installEventFilter(this);

  // shadow effects

  button_add_row->setGraphicsEffect(button_shadow());
  chart_cfg_frame->setGraphicsEffect(card_shadow());
  frame_chart->setGraphicsEffect(card_shadow());
  frame_tableview->setGraphicsEffect(card_shadow());
  button_reset_zoom->setGraphicsEffect(button_shadow());

  // signals

  connect(button_add_row, &QPushButton::clicked, this, &TableBase::on_add_row);
  connect(button_reset_zoom, &QPushButton::clicked, this, &TableBase::reset_zoom);
  connect(radio_chart1, &QRadioButton::toggled, this, &TableBase::on_chart_selection);
  connect(radio_chart2, &QRadioButton::toggled, this, &TableBase::on_chart_selection);

  // chart 1 settings

  chart1->setTheme(QChart::ChartThemeLight);
  chart1->setAcceptHoverEvents(true);

  chart_view1->setChart(chart1);
  chart_view1->setRenderHint(QPainter::Antialiasing);
  chart_view1->setRubberBand(QChartView::RectangleRubberBand);

  // chart 2 settings

  chart2->setTheme(QChart::ChartThemeLight);
  chart2->setAcceptHoverEvents(true);

  chart_view2->setChart(chart2);
  chart_view2->setRenderHint(QPainter::Antialiasing);
  chart_view2->setRubberBand(QChartView::RectangleRubberBand);

  // select the default chart

  if (radio_chart1->isChecked()) {
    stackedwidget->setCurrentIndex(0);

    label_months->hide();
    spinbox_months->hide();
  } else if (radio_chart2->isChecked()) {
    stackedwidget->setCurrentIndex(1);

    label_months->show();
    spinbox_months->show();
  }
}

void TableBase::set_database(const QSqlDatabase& database) {
  db = database;

  model = new Model(db);
}

void TableBase::set_chart1_title(const QString& title) {
  chart1->setTitle(title);
}

void TableBase::set_chart2_title(const QString& title) {
  chart2->setTitle(title);
}

bool TableBase::eventFilter(QObject* object, QEvent* event) {
  if (event->type() == QEvent::KeyPress) {
    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

    if (keyEvent->key() == Qt::Key_Delete) {
      remove_selected_rows();

      return true;
    } else if (keyEvent->matches(QKeySequence::Copy)) {
      auto s_model = table_view->selectionModel();

      if (s_model->hasSelection()) {
        auto selection_range = s_model->selection().constFirst();

        QString table_str;

        auto clipboard = QGuiApplication::clipboard();

        for (int i = selection_range.top(); i <= selection_range.bottom(); i++) {
          QString row_value;

          for (int j = selection_range.left(); j <= selection_range.right(); j++) {
            if (j < selection_range.right()) {
              row_value += s_model->model()->index(i, j).data().toString() + "\t";
            } else {
              row_value += s_model->model()->index(i, j).data().toString();
            }
          }

          table_str += row_value + "\n";
        }

        clipboard->setText(table_str);
      }

      return true;
    } else if (keyEvent->matches(QKeySequence::Paste)) {
      auto s_model = table_view->selectionModel();

      if (s_model->hasSelection()) {
        auto clipboard = QGuiApplication::clipboard();

        auto table_str = clipboard->text();
        auto table_rows = table_str.split(QRegExp("[\r\n]"), QString::SkipEmptyParts);

        auto selection_range = s_model->selection().constFirst();

        auto first_row = selection_range.top();
        auto first_col = selection_range.left();

        for (int i = 0; i < table_rows.size(); i++) {
          auto model_i = first_row + i;

          if (model_i < model->rowCount()) {
            auto row_cols = table_rows[i].split("\t");

            for (int j = 0; j < row_cols.size(); j++) {
              auto model_j = first_col + j;

              if (model_j < model->columnCount()) {
                model->setData(model->index(model_i, model_j), row_cols[j], Qt::EditRole);
              }
            }
          }
        }
      }

      return true;
    } else {
      return QObject::eventFilter(object, event);
    }
  } else {
    return QObject::eventFilter(object, event);
  }
}

void TableBase::remove_selected_rows() {
  auto s_model = table_view->selectionModel();

  if (s_model->hasSelection()) {
    auto index_list = s_model->selectedIndexes();

    QSet<int> row_set, column_set;

    for (auto& index : index_list) {
      row_set.insert(index.row());
      column_set.insert(index.column());
    }

    for (int idx = 1; idx < model->columnCount(); idx++) {
      if (!column_set.contains(idx)) {
        return;
      }
    }

    QList<int> row_list = row_set.values();

    if (row_list.size() > 0) {
      std::sort(row_list.begin(), row_list.end());
      std::reverse(row_list.begin(), row_list.end());

      for (auto& index : row_list) {
        model->removeRow(index);
      }
    }
  }
}

void TableBase::clear_charts() {
  chart1->removeAllSeries();
  chart2->removeAllSeries();

  for (auto& axis : chart1->axes()) {
    chart1->removeAxis(axis);
  }

  for (auto& axis : chart2->axes()) {
    chart2->removeAxis(axis);
  }
}

void TableBase::on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name) {
  if (state) {
    auto qdt = QDateTime();

    qdt.setMSecsSinceEpoch(point.x());

    c->setText(QString("Curve: %1\nDate: %2\nReturn: %3")
                   .arg(name, qdt.toString("MM/yyyy"), QString::number(point.y(), 'f', 2)));

    c->setAnchor(point);

    c->setZValue(11);

    c->updateGeometry();

    c->show();
  } else {
    c->hide();
  }
}

void TableBase::reset_zoom() {
  if (radio_chart1->isChecked()) {
    chart1->zoomReset();
  }

  if (radio_chart2->isChecked()) {
    chart2->zoomReset();
  }
}

void TableBase::on_chart_selection(const bool& state) {
  if (state) {
    if (radio_chart1->isChecked()) {
      stackedwidget->setCurrentIndex(0);

      label_months->hide();
      spinbox_months->hide();
    } else if (radio_chart2->isChecked()) {
      stackedwidget->setCurrentIndex(1);

      label_months->show();
      spinbox_months->show();
    }
  }
}

void TableBase::on_add_row() {
  auto rec = model->record();

  rec.setGenerated("id", false);

  rec.setValue("date", int(QDateTime().currentSecsSinceEpoch()));

  // table benchmark
  rec.setGenerated("value", true);
  rec.setGenerated("accumulated", true);

  rec.setValue("value", 0.0);
  rec.setValue("accumulated", 0.0);

  // table investment
  rec.setGenerated("deposit", true);
  rec.setGenerated("withdrawal", true);
  rec.setGenerated("starting_balance", true);
  rec.setGenerated("ending_balance", true);
  rec.setGenerated("accumulated_deposit", true);
  rec.setGenerated("accumulated_withdrawal", true);
  rec.setGenerated("net_deposit", true);
  rec.setGenerated("net_balance", true);
  rec.setGenerated("net_return", true);
  rec.setGenerated("net_return_perc", true);
  rec.setGenerated("accumulated_net_return", true);
  rec.setGenerated("accumulated_net_return_perc", true);
  rec.setGenerated("real_return_perc", true);
  rec.setGenerated("accumulated_real_return_perc", true);

  rec.setValue("deposit", 0.0);
  rec.setValue("withdrawal", 0.0);
  rec.setValue("starting_balance", 0.0);
  rec.setValue("ending_balance", 0.0);
  rec.setValue("accumulated_deposit", 0.0);
  rec.setValue("accumulated_withdrawal", 0.0);
  rec.setValue("net_deposit", 0.0);
  rec.setValue("net_balance", 0.0);
  rec.setValue("net_return", 0.0);
  rec.setValue("net_return_perc", 0.0);
  rec.setValue("accumulated_net_return", 0.0);
  rec.setValue("accumulated_net_return_perc", 0.0);
  rec.setValue("real_return_perc", 0.0);
  rec.setValue("accumulated_real_return_perc", 0.0);

  if (!model->insertRecord(0, rec)) {
    qDebug("failed to add row to table " + name.toUtf8());

    qDebug(model->lastError().text().toUtf8());
  }
}

void TableBase::calculate_accumulated_sum(const QString& column_name) {
  QVector<double> list_values, accu;

  for (int n = 0; n < model->rowCount(); n++) {
    list_values.append(model->record(n).value(column_name).toDouble());
  }

  if (list_values.size() > 0) {
    std::reverse(list_values.begin(), list_values.end());

    // cumulative sum

    accu.resize(list_values.size());

    std::partial_sum(list_values.begin(), list_values.end(), accu.begin());

    std::reverse(accu.begin(), accu.end());

    for (int n = 0; n < model->rowCount(); n++) {
      auto rec = model->record(n);

      rec.setGenerated("accumulated_" + column_name, true);

      rec.setValue("accumulated_" + column_name, accu[n]);

      model->setRecord(n, rec);
    }
  }
}

void TableBase::calculate_accumulated_product(const QString& column_name) {
  QVector<double> list_values, accu;

  for (int n = 0; n < model->rowCount(); n++) {
    list_values.append(model->record(n).value(column_name).toDouble());
  }

  if (list_values.size() > 0) {
    std::reverse(list_values.begin(), list_values.end());

    for (auto& value : list_values) {
      value = value * 0.01 + 1.0;
    }

    // cumulative product

    accu.resize(list_values.size());

    std::partial_sum(list_values.begin(), list_values.end(), accu.begin(), std::multiplies<double>());

    for (auto& value : accu) {
      value = (value - 1.0) * 100;
    }

    std::reverse(accu.begin(), accu.end());

    for (int n = 0; n < model->rowCount(); n++) {
      auto rec = model->record(n);

      rec.setGenerated("accumulated_" + column_name, true);

      rec.setValue("accumulated_" + column_name, accu[n]);

      model->setRecord(n, rec);
    }
  }
}
