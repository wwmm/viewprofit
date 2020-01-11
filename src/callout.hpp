#ifndef CALLOUT_HPP
#define CALLOUT_HPP

/*
    code adapted from https://doc.qt.io/qt-5/qtcharts-callout-example.html
*/

#include <QGraphicsItem>
#include <QtCharts>

class Callout : public QGraphicsItem {
 public:
  explicit Callout(QChart* parent = nullptr);

  void setText(const QString& value);
  void setAnchor(QPointF point);
  void updateGeometry();

  QRectF boundingRect() const;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget);

 private:
  QChart* chart;

  QPointF anchor;
  QRectF rect;
  QString text;
  QRectF textRect;
};

#endif