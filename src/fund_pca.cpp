#include "fund_pca.hpp"
#include <Eigen/Core>
#include <Eigen/Eigenvalues>
#include <QSqlError>
#include <QSqlQuery>
#include "chart_funcs.hpp"
#include "effects.hpp"

FundPCA::FundPCA(const QSqlDatabase& database, QWidget* parent)
    : db(database), chart(new QChart()), callout(new Callout(chart)) {
  setupUi(this);

  callout->hide();

  // shadow effects

  frame_chart->setGraphicsEffect(card_shadow());
  frame_time_window->setGraphicsEffect(card_shadow());
  frame_explained_variance->setGraphicsEffect(card_shadow());
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

  chart->setTitle("Net Return Pricipal Component Analysis");

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

  // Standardizing the data https://en.wikipedia.org/wiki/Feature_scaling#Standardization_(Z-score_Normalization)

  data = data.rowwise() - data.colwise().mean();

  Eigen::ArrayXd stddev = (data.array() * data.array()).colwise().sum().sqrt() / std::sqrt(data.rows() - 1);

  for (int n = 0; n < data.cols(); n++) {
    double std = stddev(n);

    for (int m = 0; m < data.rows(); m++) {
      if (std > 0.0001) {
        data(m, n) /= std;
      }
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

  label_pc1->setText(QString("PC1: %1%").arg(QString::number(pc1_explained_variance, 'f', 1)));
  label_pc2->setText(QString("PC2: %1%").arg(QString::number(pc2_explained_variance, 'f', 1)));

  // Projecting the data to the new space

  Eigen::MatrixXd projection_matrix(eigenvectors.rows(), 2);

  projection_matrix.col(0) = eigenvectors.col(eigenvectors.cols() - 1);
  projection_matrix.col(1) = eigenvectors.col(eigenvectors.cols() - 2);

  Eigen::MatrixXd pdata = data * projection_matrix;

  // Showing the data in the chart

  QFont serif_font("Sans");

  auto axis_x = new QValueAxis();

  axis_x->setTitleText("PC1");
  axis_x->setLabelFormat("%.2f");
  axis_x->setTitleFont(serif_font);

  auto axis_y = new QValueAxis();

  axis_y->setTitleText("PC2");
  axis_y->setLabelFormat("%.2f");
  axis_y->setTitleFont(serif_font);

  chart->addAxis(axis_x, Qt::AlignBottom);
  chart->addAxis(axis_y, Qt::AlignLeft);

  double xmin = 0.0, xmax = 0.0, ymin = 0.0, ymax = 0.0;

  for (int n = 0; n < pdata.rows(); n++) {
    double x = pdata(n, 0);
    double y = pdata(n, 1);

    if (n == 0) {
      xmin = x;
      xmax = x;
      ymin = y;
      ymax = y;
    } else {
      xmin = std::min(xmin, x);
      xmax = std::max(xmax, x);
      ymin = std::min(ymin, y);
      ymax = std::max(ymax, y);
    }

    auto series = new QScatterSeries();

    series->setName(tables[n]->name);

    series->append(x, y);

    chart->addSeries(series);

    series->attachAxis(chart->axes(Qt::Horizontal)[0]);
    series->attachAxis(chart->axes(Qt::Vertical)[0]);

    connect(series, &QLineSeries::hovered, this, [=](const QPointF& point, bool state) {
      if (state) {
        auto qdt = QDateTime();

        for (int n = 0; n < pdata.rows(); n++) {
          double x = pdata(n, 0);
          double y = pdata(n, 1);

          double dx = std::fabs(x - point.x());
          double dy = std::fabs(y - point.y());

          if (dx < 0.00001 && dy < 0.00001) {
            callout->setText(QString("Fund: %1").arg(tables[n]->name));

            break;
          }
        }

        callout->setAnchor(point);

        callout->setZValue(11);

        callout->updateGeometry();

        callout->show();
      } else {
        callout->hide();
      }
    });
  }

  chart->axes(Qt::Horizontal)[0]->setRange(xmin - 0.05 * fabs(xmin), xmax + 0.05 * fabs(xmax));
  chart->axes(Qt::Vertical)[0]->setRange(ymin - 0.05 * fabs(ymin), ymax + 0.05 * fabs(ymax));
}
