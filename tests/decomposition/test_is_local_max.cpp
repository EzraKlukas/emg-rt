#include "emg-rt/decomposition/is_local_max.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <vector>

TEST_CASE("incremental_is_local_max keeps an interior maximum") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 1.0F, 4.0F, 2.0F, 0.0F},
  });
  emg_rt::RingMatrix<bool> spikes(1, 5);
  std::vector<float> maxima = {1.0F};

  emg_rt::decomp::incremental_is_local_max(pulse, spikes, 3, maxima, 1);

  CHECK_FALSE(spikes(0, 0));
  CHECK_FALSE(spikes(0, 1));
  CHECK(spikes(0, 2));
  CHECK_FALSE(spikes(0, 3));
  CHECK_FALSE(spikes(0, 4));
}

TEST_CASE("incremental_is_local_max clears an older candidate when a larger "
          "future value arrives inside lookahead") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 3.0F, 5.0F},
  });
  emg_rt::RingMatrix<bool> spikes(1, 4);
  spikes(0, 2) = true;
  std::vector<float> maxima = {4.0F};

  emg_rt::decomp::incremental_is_local_max(pulse, spikes, 1, maxima, 1);

  CHECK_FALSE(spikes(0, 2));
  CHECK(spikes(0, 3));
}

TEST_CASE("incremental_is_local_max does not replace a candidate on an equal "
          "plateau value") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 3.0F, 3.0F},
  });
  emg_rt::RingMatrix<bool> spikes(1, 4);
  spikes(0, 2) = true;
  std::vector<float> maxima = {3.0F};

  emg_rt::decomp::incremental_is_local_max(pulse, spikes, 1, maxima, 1);

  CHECK(spikes(0, 2));
  CHECK_FALSE(spikes(0, 3));
}

TEST_CASE("incremental_is_local_max uses absolute pulse-train magnitude") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 1.0F, -4.0F, 2.0F},
  });
  emg_rt::RingMatrix<bool> spikes(1, 4);
  std::vector<float> maxima = {1.0F};

  emg_rt::decomp::incremental_is_local_max(pulse, spikes, 2, maxima, 1);

  CHECK(spikes(0, 2));
  CHECK_FALSE(spikes(0, 3));
}

TEST_CASE("incremental_is_local_max processes rows independently") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 1.0F, 4.0F, 2.0F},
      {0.0F, 8.0F, 3.0F, 9.0F},
  });
  emg_rt::RingMatrix<bool> spikes(2, 4);
  std::vector<float> maxima = {1.0F, 8.0F};

  emg_rt::decomp::incremental_is_local_max(pulse, spikes, 2, maxima, 1);

  CHECK(spikes(0, 2));
  CHECK_FALSE(spikes(0, 3));
  CHECK_FALSE(spikes(1, 2));
  CHECK(spikes(1, 3));
}
