#ifndef EMG_RT_TESTS_MATRIX_HELPERS_H
#define EMG_RT_TESTS_MATRIX_HELPERS_H

#include "emg-rt/utils/types.h"

#include <doctest/doctest.h>

#include <cstddef>
#include <initializer_list>

namespace emg_rt::tests {

template <typename T>
RingMatrix<T>
matrix_from_rows(std::initializer_list<std::initializer_list<T>> rows) {
  REQUIRE_FALSE(rows.size() == 0);

  const std::size_t row_count = rows.size();
  const std::size_t col_count = rows.begin()->size();
  REQUIRE_FALSE(col_count == 0);

  RingMatrix<T> out(row_count, col_count);

  std::size_t row = 0;
  for (const auto &values : rows) {
    REQUIRE(values.size() == col_count);

    std::size_t col = 0;
    for (const T &value : values) {
      out(row, col) = value;
      ++col;
    }

    ++row;
  }

  return out;
}

} // namespace emg_rt::tests

#endif // EMG_RT_TESTS_MATRIX_HELPERS_H
