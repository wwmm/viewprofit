#ifndef MODEL_BENCHMARK_HPP
#define MODEL_BENCHMARK_HPP

#include <QSqlTableModel>

class Model : public QSqlTableModel {
 public:
  Model(QSqlDatabase db, QObject* parent = nullptr);

  Qt::ItemFlags flags(const QModelIndex& index);

  QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

  bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
};

#endif