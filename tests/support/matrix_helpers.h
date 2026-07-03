#ifndef EMG_RT_TESTS_MATRIX_HELPERS_H
#define EMG_RT_TESTS_MATRIX_HELPERS_H

#include "emg-rt/utils/types.h"

#include <doctest/doctest.h>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>

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

template <typename T>
void expect_matrix_eq(const RingMatrix<T> &matrix,
                      std::initializer_list<std::initializer_list<T>> rows) {
  REQUIRE(matrix.rows == rows.size());
  REQUIRE_FALSE(rows.size() == 0);

  const std::size_t expected_cols = rows.begin()->size();
  REQUIRE(matrix.cols == expected_cols);

  std::size_t row = 0;
  for (const auto &values : rows) {
    REQUIRE(values.size() == expected_cols);

    std::size_t col = 0;
    for (const T &expected : values) {
      CHECK(matrix(row, col) == expected);
      ++col;
    }

    ++row;
  }
}

inline void
expect_matrix_approx(const RingMatrix<float> &matrix,
                     std::initializer_list<std::initializer_list<float>> rows) {
  REQUIRE(matrix.rows == rows.size());
  REQUIRE_FALSE(rows.size() == 0);

  const std::size_t expected_cols = rows.begin()->size();
  REQUIRE(matrix.cols == expected_cols);

  std::size_t row = 0;
  for (const auto &values : rows) {
    REQUIRE(values.size() == expected_cols);

    std::size_t col = 0;
    for (const float expected : values) {
      CHECK(matrix(row, col) == doctest::Approx(expected));
      ++col;
    }

    ++row;
  }
}

template <typename T, typename U>
void expect_vector_eq(RingVector<T> &vector,
                      std::initializer_list<U> expected_values) {
  REQUIRE(vector.size == expected_values.size());

  std::size_t idx = 0;
  for (const U &expected : expected_values) {
    CHECK(vector(idx) == static_cast<T>(expected));
    ++idx;
  }
}

inline void expect_vector_approx(const std::vector<float> &actual,
                                 std::initializer_list<float> expected_values) {
  REQUIRE(actual.size() == expected_values.size());

  std::size_t idx = 0;
  for (const float expected : expected_values) {
    CHECK(actual[idx] == doctest::Approx(expected));
    ++idx;
  }
}

inline uint16_t raw_adc_from_counts(int counts_from_midscale) {
  return static_cast<uint16_t>(32768 + counts_from_midscale);
}

inline float microvolts_from_counts(int counts_from_midscale) {
  return static_cast<float>(counts_from_midscale) * 0.195F;
}

} // namespace emg_rt::tests

#endif // EMG_RT_TESTS_MATRIX_HELPERS_H
