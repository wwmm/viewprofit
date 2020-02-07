#ifndef MATH_HPP
#define MATH_HPP

#include <QVector>

template <class T>
auto second_derivative(const QVector<T>& input) -> QVector<T> {
  QVector<T> output(input.size(), 0.0);

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

#endif