#include "compare_funds.hpp"

CompareFunds::CompareFunds(const QSqlDatabase& database, QWidget* parent) : db(database) {
  setupUi(this);
}