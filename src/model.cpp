#include "model.hpp"
#include <QColor>
#include <QDateTime>

Model::Model(QSqlDatabase db, QObject* parent) : QSqlTableModel(parent, db) {}

Qt::ItemFlags Model::flags(const QModelIndex& index) {
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QVariant Model::data(const QModelIndex& index, int role) const {
  if (role == Qt::BackgroundRole) {
    return QColor(Qt::white);
  }

  if (role == Qt::TextAlignmentRole) {
    return Qt::AlignRight;
  }

  if (role == Qt::DisplayRole || role == Qt::EditRole) {
    int column = index.column();

    if (column == 1) {
      auto v = QSqlTableModel::data(index, role);

      if (v.userType() == QMetaType::LongLong || v.userType() == QMetaType::Int) {
        auto qdt = QDateTime::fromSecsSinceEpoch(v.toInt());

        return qdt.toString("dd/MM/yyyy");
      } else {
        return v;
      }
    } else {
      return QSqlTableModel::data(index, role);
    }
  }

  return QSqlTableModel::data(index, role);
}

bool Model::setData(const QModelIndex& index, const QVariant& value, int role) {
  if (role != Qt::EditRole) {
    return false;
  }

  int column = index.column();

  if (column == 0) {
    return false;
  }

  if (column == 1) {
    if (value.type() == QVariant::String) {
      auto qdt = QDateTime::fromString(value.toString(), "dd/MM/yyyy");

      return QSqlTableModel::setData(index, qdt.toSecsSinceEpoch(), role);
    } else if (value.type() == QVariant::Int) {
      return QSqlTableModel::setData(index, value, role);
    } else {
      return false;
    }
  } else {
    return QSqlTableModel::setData(index, value, role);
  }
}