#include "emg-rt/dsp/extend.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <vector>

TEST_CASE("incremental_extend copies newest delayed samples and updates sums") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F},
  });
  emg_rt::RingMatrix<float> extended(2, 2);
  std::vector<float> sums = {10.0F, 20.0F};

  emg_rt::dsp::incremental_extend(extended, signal, 2, sums, 2, 2);

  CHECK(extended(0, 0) == doctest::Approx(5.0F));
  CHECK(extended(0, 1) == doctest::Approx(6.0F));
  CHECK(extended(1, 0) == doctest::Approx(4.0F));
  CHECK(extended(1, 1) == doctest::Approx(5.0F));

  CHECK(sums[0] == doctest::Approx(14.0F));
  CHECK(sums[1] == doctest::Approx(24.0F));
}
