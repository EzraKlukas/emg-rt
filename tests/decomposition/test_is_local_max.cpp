#include "emg-rt/decomposition/is_local_max.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <vector>

TEST_CASE("incremental_is_local_max marks new values exceeding tracked maximum") {
  auto pulse_train = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 2.0F, 1.0F, 3.0F, 1.0F},
  });
  emg_rt::RingMatrix<bool> candidate_spikes(1, 5);
  std::vector<float> maxima = {2.0F};

  emg_rt::decomp::incremental_is_local_max(pulse_train, candidate_spikes, 2,
                                           maxima, 1);

  CHECK_FALSE(candidate_spikes(0, 2));
  CHECK(candidate_spikes(0, 3));
  CHECK_FALSE(candidate_spikes(0, 4));
}

TEST_CASE("incremental_is_local_max rejects older candidate when larger lookahead arrives") {
  auto pulse_train = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 2.0F, 1.0F, 3.0F},
  });
  emg_rt::RingMatrix<bool> candidate_spikes(1, 5);
  candidate_spikes(0, 3) = true;
  std::vector<float> maxima = {2.0F};

  emg_rt::decomp::incremental_is_local_max(pulse_train, candidate_spikes, 1,
                                           maxima, 1);

  CHECK_FALSE(candidate_spikes(0, 3));
  CHECK(candidate_spikes(0, 4));
}

TEST_CASE("incremental_is_local_max treats equal amplitude as not a new maximum") {
  auto pulse_train = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 1.0F, 5.0F, 5.0F},
  });
  emg_rt::RingMatrix<bool> candidate_spikes(1, 4);
  std::vector<float> maxima = {5.0F};

  emg_rt::decomp::incremental_is_local_max(pulse_train, candidate_spikes, 1,
                                           maxima, 1);

  CHECK_FALSE(candidate_spikes(0, 3));
}

TEST_CASE("incremental_is_local_max keeps filter rows independent") {
  auto pulse_train = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 1.0F, 1.0F, 4.0F},
      {0.0F, 1.0F, 7.0F, 4.0F},
  });
  emg_rt::RingMatrix<bool> candidate_spikes(2, 4);
  std::vector<float> maxima = {2.0F, 7.0F};

  emg_rt::decomp::incremental_is_local_max(pulse_train, candidate_spikes, 1,
                                           maxima, 1);

  CHECK(candidate_spikes(0, 3));
  CHECK_FALSE(candidate_spikes(1, 3));
}
