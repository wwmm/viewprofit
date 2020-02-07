/*
    code adapted from https://doc.qt.io/qt-5/qtcharts-callout-example.html
*/

#include "callout.hpp"

Callout::Callout(QChart* parent) : QGraphicsItem(parent), chart(parent) {}

auto Callout::boundingRect() const -> QRectF {
  const QPointF a = mapFromParent(chart->mapToPosition(anchor));
  QRectF r;

  r.setLeft(qMin(rect.left(), a.x()));
  r.setRight(qMax(rect.right(), a.x()));
  r.setTop(qMin(rect.top(), a.y()));
  r.setBottom(qMax(rect.bottom(), a.y()));

  return r;
}

void Callout::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
  Q_UNUSED(option)
  Q_UNUSED(widget)

  QPainterPath path;

  path.addRoundedRect(rect, 5, 5);

  const QPointF a = mapFromParent(chart->mapToPosition(anchor));

  if (!rect.contains(a)) {
    QPointF point1;
    QPointF point2;

    // establish the position of the anchor point in relation to rect

    const bool above = a.y() <= rect.top();
    const bool aboveCenter = a.y() > rect.top() && a.y() <= rect.center().y();
    const bool belowCenter = a.y() > rect.center().y() && a.y() <= rect.bottom();
    const bool below = a.y() > rect.bottom();

    const bool onLeft = a.x() <= rect.left();
    const bool leftOfCenter = a.x() > rect.left() && a.x() <= rect.center().x();
    const bool rightOfCenter = a.x() > rect.center().x() && a.x() <= rect.right();
    const bool onRight = a.x() > rect.right();

    // get the nearest rect corner.

    const qreal x = (static_cast<int>(onRight) + static_cast<int>(rightOfCenter)) * rect.width();
    const qreal y = (static_cast<int>(below) + static_cast<int>(belowCenter)) * rect.height();

    const bool cornerCase = (above && onLeft) || (above && onRight) || (below && onLeft) || (below && onRight);
    const bool vertical = qAbs(a.x() - x) > qAbs(a.y() - y);

    const qreal x1 = x + static_cast<int>(leftOfCenter) * 10 - static_cast<int>(rightOfCenter) * 20 +
                     static_cast<int>(cornerCase) * static_cast<int>(!vertical) *
                         (static_cast<int>(onLeft) * 10 - static_cast<int>(onRight) * 20);

    const qreal y1 = y + static_cast<int>(aboveCenter) * 10 - static_cast<int>(belowCenter) * 20 +
                     static_cast<int>(cornerCase) * static_cast<int>(vertical) *
                         (static_cast<int>(above) * 10 - static_cast<int>(below) * 20);

    point1.setX(x1);
    point1.setY(y1);

    const qreal x2 = x + static_cast<int>(leftOfCenter) * 20 - static_cast<int>(rightOfCenter) * 10 +
                     static_cast<int>(cornerCase) * static_cast<int>(!vertical) *
                         (static_cast<int>(onLeft) * 20 - static_cast<int>(onRight) * 10);

    const qreal y2 = y + static_cast<int>(aboveCenter) * 20 - static_cast<int>(belowCenter) * 10 +
                     static_cast<int>(cornerCase) * static_cast<int>(vertical) *
                         (static_cast<int>(above) * 20 - static_cast<int>(below) * 10);

    point2.setX(x2);
    point2.setY(y2);

    path.moveTo(point1);
    path.lineTo(a);
    path.lineTo(point2);

    path = path.simplified();
  }

  painter->setBrush(QColor(255, 255, 255));
  painter->drawPath(path);
  painter->drawText(textRect, text);
}

void Callout::setText(const QString& value) {
  text = value;

  QFont font;
  QFontMetrics metrics(font);

  textRect = metrics.boundingRect(QRect(0, 0, 150, 150), Qt::AlignLeft, text);

  textRect.translate(5, 5);

  prepareGeometryChange();

  rect = textRect.adjusted(-5, -5, 5, 5);
}

void Callout::setAnchor(QPointF point) {
  anchor = point;
}

void Callout::updateGeometry() {
  prepareGeometryChange();

  setPos(chart->mapToPosition(anchor) + QPoint(-80, -100));
}
