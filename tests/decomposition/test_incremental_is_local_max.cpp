#include "emg-rt/decomposition/is_local_max.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <vector>

TEST_CASE(
    "incremental_is_local_max marks a new sample that exceeds running maxima") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 1.0F, 3.0F, 2.0F},
  });
  emg_rt::RingMatrix<bool> spikes(1, 4);
  std::vector<float> maxima = {1.0F};

  emg_rt::decomp::incremental_is_local_max(pulse, spikes, 2, maxima, 1);

  CHECK_FALSE(spikes(0, 0));
  CHECK_FALSE(spikes(0, 1));
  CHECK(spikes(0, 2));
  CHECK_FALSE(spikes(0, 3));
  CHECK(maxima[0] == doctest::Approx(3.0F));
}

TEST_CASE("incremental_is_local_max clears a delayed lower candidate") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 1.0F, 3.0F},
  });
  emg_rt::RingMatrix<bool> spikes(1, 4);
  spikes(0, 2) = true;
  std::vector<float> maxima = {2.0F};

  emg_rt::decomp::incremental_is_local_max(pulse, spikes, 1, maxima, 1);

  CHECK_FALSE(spikes(0, 2));
  CHECK(spikes(0, 3));
  CHECK(maxima[0] == doctest::Approx(3.0F));
}
