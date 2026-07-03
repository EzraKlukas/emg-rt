#include "emg-rt/decomposition/classify_discharges.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <vector>

TEST_CASE("classify_discharges marks candidates closer to spike centroid") {
  auto pulse_train = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 0.0F, 9.0F, 1.0F, 0.0F},
      {0.0F, 0.0F, 0.0F, 1.0F, -7.0F, 0.0F},
  });
  emg_rt::RingMatrix<bool> candidates(2, 6);
  candidates(0, 3) = true;
  candidates(0, 4) = true;
  candidates(1, 4) = true;
  emg_rt::RingMatrix<bool> discharges(2, 6);

  std::vector<float> noise = {0.0F, 0.0F};
  std::vector<float> spike = {10.0F, -8.0F};
  std::vector<std::size_t> onset = {1, 0};

  emg_rt::decomp::classify_discharges(
      discharges, pulse_train, candidates,
      emg_rt::VectorView<float>(noise.data(), noise.size()),
      emg_rt::VectorView<float>(spike.data(), spike.size()), onset, 2, 1);

  CHECK(discharges(0, 2));
  CHECK_FALSE(discharges(0, 3));
  CHECK(discharges(1, 4));
}

TEST_CASE("classify_discharges does not mark non-candidates or noise-like candidates") {
  auto pulse_train = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 0.0F, 1.0F, 2.0F},
  });
  emg_rt::RingMatrix<bool> candidates(1, 5);
  candidates(0, 3) = true;
  emg_rt::RingMatrix<bool> discharges(1, 5);

  std::vector<float> noise = {0.0F};
  std::vector<float> spike = {10.0F};
  std::vector<std::size_t> onset = {0};

  emg_rt::decomp::classify_discharges(
      discharges, pulse_train, candidates,
      emg_rt::VectorView<float>(noise.data(), noise.size()),
      emg_rt::VectorView<float>(spike.data(), spike.size()), onset, 2, 1);

  CHECK_FALSE(discharges(0, 3));
  CHECK_FALSE(discharges(0, 4));
}

TEST_CASE("classify_discharges leaves exact noise/spike tie unmarked") {
  auto pulse_train = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 0.0F, 5.0F, 0.0F},
  });
  emg_rt::RingMatrix<bool> candidates(1, 5);
  candidates(0, 3) = true;
  emg_rt::RingMatrix<bool> discharges(1, 5);

  std::vector<float> noise = {0.0F};
  std::vector<float> spike = {10.0F};
  std::vector<std::size_t> onset = {0};

  emg_rt::decomp::classify_discharges(
      discharges, pulse_train, candidates,
      emg_rt::VectorView<float>(noise.data(), noise.size()),
      emg_rt::VectorView<float>(spike.data(), spike.size()), onset, 2, 1);

  CHECK_FALSE(discharges(0, 3));
}
