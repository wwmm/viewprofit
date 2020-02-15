#ifndef MODEL_BENCHMARK_HPP
#define MODEL_BENCHMARK_HPP

#include <QLocale>
#include <QSqlTableModel>

class Model : public QSqlTableModel {
 public:
  Model(const QSqlDatabase& db, QObject* parent = nullptr);

  using QSqlTableModel::flags;
  auto flags(const QModelIndex& index) -> Qt::ItemFlags;

  [[nodiscard]] auto data(const QModelIndex& index, int role = Qt::DisplayRole) const -> QVariant override;

  auto setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) -> bool override;

 private:
  QLocale locale;
};

#endif