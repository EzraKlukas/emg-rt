#include "emg-rt/utils/types.h"

#include <doctest/doctest.h>

#include <array>

TEST_CASE("RingMatrix push_column preserves logical order after wrap") {
  emg_rt::RingMatrix<float> matrix(2, 3);

  const std::array<float, 2> first{1.0F, 10.0F};
  const std::array<float, 2> second{2.0F, 20.0F};
  const std::array<float, 2> third{3.0F, 30.0F};
  const std::array<float, 2> fourth{4.0F, 40.0F};

  matrix.push_column(first);
  matrix.push_column(second);
  matrix.push_column(third);
  matrix.push_column(fourth);

  CHECK(matrix[0, 0] == doctest::Approx(2.0F));
  CHECK(matrix[1, 0] == doctest::Approx(20.0F));
  CHECK(matrix[0, 1] == doctest::Approx(3.0F));
  CHECK(matrix[1, 1] == doctest::Approx(30.0F));
  CHECK(matrix[0, 2] == doctest::Approx(4.0F));
  CHECK(matrix[1, 2] == doctest::Approx(40.0F));
}
