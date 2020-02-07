#include "effects.hpp"

auto button_shadow() -> QGraphicsDropShadowEffect* {
  const auto effect = new QGraphicsDropShadowEffect();

  effect->setColor(QColor(0, 0, 0, 100));
  effect->setXOffset(1);
  effect->setYOffset(1);
  effect->setBlurRadius(5);

  return effect;
}

auto card_shadow() -> QGraphicsDropShadowEffect* {
  const auto effect = new QGraphicsDropShadowEffect();

  effect->setColor(QColor(0, 0, 0, 100));
  effect->setXOffset(2);
  effect->setYOffset(2);
  effect->setBlurRadius(5);

  return effect;
}