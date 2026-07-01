#include "emg-rt/utils/types.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

TEST_CASE("RingMatrix reads logical columns after wraparound writes") {
  auto matrix = emg_rt::tests::matrix_from_rows<float>({
      {1.0F, 2.0F, 3.0F},
      {10.0F, 20.0F, 30.0F},
  });

  const float new_column[] = {4.0F, 40.0F};
  matrix.write_column(new_column);

  CHECK(matrix.head == 1);
  CHECK(matrix(0, 0) == doctest::Approx(2.0F));
  CHECK(matrix(0, 1) == doctest::Approx(3.0F));
  CHECK(matrix(0, 2) == doctest::Approx(4.0F));
  CHECK(matrix(1, 0) == doctest::Approx(20.0F));
  CHECK(matrix(1, 1) == doctest::Approx(30.0F));
  CHECK(matrix(1, 2) == doctest::Approx(40.0F));
}
