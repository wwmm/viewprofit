#include "effects.hpp"

QGraphicsDropShadowEffect* button_shadow() {
  auto effect = new QGraphicsDropShadowEffect();

  effect->setColor(QColor(0, 0, 0, 100));
  effect->setXOffset(1);
  effect->setYOffset(1);
  effect->setBlurRadius(5);

  return effect;
}

QGraphicsDropShadowEffect* card_shadow() {
  auto effect = new QGraphicsDropShadowEffect();

  effect->setColor(QColor(0, 0, 0, 100));
  effect->setXOffset(2);
  effect->setYOffset(2);
  effect->setBlurRadius(5);

  return effect;
}