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

  [[nodiscard]] auto boundingRect() const -> QRectF override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) override;

 private:
  QChart* const chart;

  QPointF anchor;
  QRectF rect;
  QString text;
  QRectF textRect;
};

#endif