#include "fund_pca.hpp"
#include <Eigen/Dense>
#include <QSqlError>
#include <QSqlQuery>
#include <iostream>
#include "chart_funcs.hpp"
#include "effects.hpp"

FundPCA::FundPCA(const QSqlDatabase& database, QWidget* parent)
    : db(database), chart(new QChart()), callout(new Callout(chart)) {
  setupUi(this);

  callout->hide();

  // shadow effects

  frame_chart->setGraphicsEffect(card_shadow());
  frame_time_window->setGraphicsEffect(card_shadow());
  button_reset_zoom->setGraphicsEffect(button_shadow());

  // chart settings

  chart->setTheme(QChart::ChartThemeLight);
  chart->setAcceptHoverEvents(true);
  chart->legend()->setAlignment(Qt::AlignRight);

  chart_view->setChart(chart);
  chart_view->setRenderHint(QPainter::Antialiasing);
  chart_view->setRubberBand(QChartView::RectangleRubberBand);

  // signals

  connect(button_reset_zoom, &QPushButton::clicked, this, [&]() { chart->zoomReset(); });
  connect(spinbox_months, QOverload<int>::of(&QSpinBox::valueChanged), [&](int value) { process_tables(); });
}

void FundPCA::process(const QVector<TableFund*>& tables) {
  this->tables = tables;

  process_tables();
}

void FundPCA::process_tables() {
  clear_chart(chart);

  chart->setTitle("Pricipal Component Analysis");

  add_axes_to_chart(chart, "");

  Eigen::MatrixXd data = Eigen::MatrixXd::Zero(tables.size(), spinbox_months->value());

  for (int k = 0; k < tables.size(); k++) {
    auto query = QSqlQuery(db);
    QVector<double> values;

    query.prepare("select net_return_perc from " + tables[k]->name + " order by date desc");

    if (query.exec()) {
      while (query.next() && values.size() < spinbox_months->value()) {
        values.append(query.value(0).toDouble());
      }
    }

    if (values.size() == 0) {
      break;
    }

    for (int n = 0; n < values.size(); n++) {
      data(k, n) = values[n];
    }
  }

  // https://en.wikipedia.org/wiki/Sample_mean_and_covariance#Sample_covariance

  Eigen::MatrixXd covariance =
      (data.rowwise() - data.colwise().mean()).transpose() * (data.rowwise() - data.colwise().mean());

  if (data.rows() > 1) {
    covariance /= (data.rows() - 1);
  }

  // Finding eigenvalues and eigenvectors. We use SelfAdjointEigenSolver because the covariance matrix is symmetric.

  Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> solver;

  solver.compute(covariance);

  Eigen::VectorXd eigenvalues = solver.eigenvalues();
  Eigen::MatrixXd eigenvectors = solver.eigenvectors();

  // Calculating the explained variance

  double eigenvalues_sum = eigenvalues.sum();
  Eigen::VectorXd percentage = 100 * eigenvalues / eigenvalues_sum;

  double pc1_explained_variance = percentage[percentage.size() - 1];
  double pc2_explained_variance = percentage[percentage.size() - 2];

  qDebug(" ");
  qDebug("Explained variances:");
  qDebug(QString("PC1 -> %1%").arg(QString::number(pc1_explained_variance, 'f', 1)).toUtf8());
  qDebug(QString("PC2 -> %1%").arg(QString::number(pc2_explained_variance, 'f', 1)).toUtf8());

  // std::cout << percentage << std::endl;
}

void FundPCA::on_chart_mouse_hover(const QPointF& point, bool state, Callout* c, const QString& name) {
  if (state) {
    auto qdt = QDateTime();

    qdt.setMSecsSinceEpoch(point.x());

    c->setText(QString("Fund: %1\nDate: %2").arg(name, qdt.toString("dd/MM/yyyy")));

    c->setAnchor(point);

    c->setZValue(11);

    c->updateGeometry();

    c->show();
  } else {
    c->hide();
  }
}