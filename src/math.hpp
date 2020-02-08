#ifndef MATH_HPP
#define MATH_HPP

#include <QVector>
#include <cmath>

template <class T>
auto second_derivative(const QVector<T>& input) -> QVector<T> {
  QVector<T> output(input.size(), 0);

  // https://en.wikipedia.org/wiki/Finite_difference

  for (int n = 0; n < input.size(); n++) {
    if (n == 0) {  // second order forward using dt = 1 month
      output[n] = input[2] - 2 * input[1] + input[0];
    } else if (n == input.size() - 1) {  // second order backward
      output[n] = input[n] - 2 * input[n - 1] + input[n - 2];
    } else {  // second order central
      output[n] = input[n + 1] - 2 * input[n] + input[n - 1];
    }
  }

  return output;
}

template <class T>
auto standard_deviation(const QVector<T>& input) -> QVector<T> {
  QVector<T> output(input.size(), 0);

  // Calculating the standard deviation https://en.wikipedia.org/wiki/Standard_deviation

  T accumulated = 0.0;

  for (int n = 0; n < input.size(); n++) {
    accumulated += input[n];

    T avg = accumulated / (n + 1);

    T sum = 0.0;

    for (int m = 0; m < n; m++) {
      sum += (input[m] - avg) * (input[m] - avg);
    }

    sum = (n > 0) ? sum / n : sum;

    output[n] = std::sqrt(sum);
  }

  return output;
}

template <class T>
auto correlation_coefficient(const QVector<T>& a, const QVector<T>& b) -> QVector<T> {
  QVector<T> output(a.size(), 0);
  T sum_a = 0;
  T sum_b = 0;

  // calculating the Pearson correlation coefficient https://en.wikipedia.org/wiki/Pearson_correlation_coefficient

  for (int n = 0; n < a.size(); n++) {
    sum_a += a[n];
    sum_b += b[n];

    T avg_a = sum_a / (n + 1);
    T avg_b = sum_b / (n + 1);

    T variance_a = 0;
    T variance_b = 0;

    for (int m = 0; m <= n; m++) {
      variance_a += (a[m] - avg_a) * (a[m] - avg_a);
      variance_b += (b[m] - avg_b) * (b[m] - avg_b);
    }

    T stddev = std::sqrt(variance_a);
    T tstddev = std::sqrt(variance_b);

    for (int m = 0; m <= n; m++) {
      output[n] += (a[m] - avg_a) * (b[m] - avg_b);
    }

    const float tol = 0.001F;

    if (stddev > tol && tstddev > tol) {
      output[n] /= (stddev * tstddev);
    }
  }

  return output;
}

#endif