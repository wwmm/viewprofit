#include "compare_funds.hpp"

CompareFunds::CompareFunds(const QSqlDatabase& database, QWidget* parent) : db(database) {
  setupUi(this);

  // shadow effects

  frame_chart1->setGraphicsEffect(card_shadow());
  frame_chart2->setGraphicsEffect(card_shadow());
}

QGraphicsDropShadowEffect* CompareFunds::button_shadow() {
  auto effect = new QGraphicsDropShadowEffect(this);

  effect->setColor(QColor(0, 0, 0, 100));
  effect->setXOffset(1);
  effect->setYOffset(1);
  effect->setBlurRadius(5);

  return effect;
}

QGraphicsDropShadowEffect* CompareFunds::card_shadow() {
  auto effect = new QGraphicsDropShadowEffect(this);

  effect->setColor(QColor(0, 0, 0, 100));
  effect->setXOffset(2);
  effect->setYOffset(2);
  effect->setBlurRadius(5);

  return effect;
}

void CompareFunds::calculate() {
  qDebug("calculate");
}

// void process_fund_tables(const QVector<TableFund*>& tables) {}