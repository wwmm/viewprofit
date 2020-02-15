#include "model.hpp"
#include <QColor>
#include <QDateTime>

Model::Model(const QSqlDatabase& db, QObject* parent) : QSqlTableModel(parent, db) {}

auto Model::flags(const QModelIndex& index) -> Qt::ItemFlags {
  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

auto Model::data(const QModelIndex& index, int role) const -> QVariant {
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

        return qdt.toString("MM/yyyy");
      }

      return v;
    }

    return QSqlTableModel::data(index, role);
  }

  return QSqlTableModel::data(index, role);
}

auto Model::setData(const QModelIndex& index, const QVariant& value, int role) -> bool {
  if (role != Qt::EditRole) {
    return false;
  }

  int column = index.column();

  if (column == 0) {
    return false;
  }

  if (column == 1) {
    if (value.userType() == QMetaType::QString) {
      auto qdt = QDateTime::fromString(value.toString(), "MM/yyyy");

      return QSqlTableModel::setData(index, qdt.toSecsSinceEpoch(), role);
    }

    if (value.userType() == QMetaType::Int) {
      return QSqlTableModel::setData(index, value, role);
    }

    return false;
  }

  if (value.userType() == QMetaType::Double) {
    return QSqlTableModel::setData(index, value, role);
  }

  if (value.userType() == QMetaType::QString) {
    return QSqlTableModel::setData(index, locale.toDouble(value.toString()), role);
  }

  return false;
}