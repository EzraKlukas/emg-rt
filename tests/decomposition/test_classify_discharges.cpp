#include "emg-rt/decomposition/classify_discharges.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <vector>

TEST_CASE("classify_discharges accepts candidates closer to spike centroid") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.4F, 0.6F, 1.1F, 0.0F},
  });
  auto candidates = emg_rt::tests::matrix_from_rows<bool>({
      {false, true, true, true, false},
  });

  std::vector<float> noise_centroids = {0.0F};
  std::vector<float> spike_centroids = {1.0F};
  std::vector<std::size_t> samples_onset = {0};
  emg_rt::RingMatrix<bool> discharges(1, 5);

  emg_rt::VectorView<float> noise_view(noise_centroids.data(),
                                       noise_centroids.size());
  emg_rt::VectorView<float> spike_view(spike_centroids.data(),
                                       spike_centroids.size());

  emg_rt::decomp::classify_discharges(discharges, pulse, candidates,
                                      noise_view, spike_view, samples_onset, 3,
                                      1);

  CHECK_FALSE(discharges(0, 1));
  CHECK(discharges(0, 2));
  CHECK(discharges(0, 3));
}

TEST_CASE("classify_discharges rejects exact centroid midpoint ties") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.5F},
  });
  auto candidates = emg_rt::tests::matrix_from_rows<bool>({
      {false, true},
  });

  std::vector<float> noise_centroids = {0.0F};
  std::vector<float> spike_centroids = {1.0F};
  std::vector<std::size_t> samples_onset = {0};
  emg_rt::RingMatrix<bool> discharges(1, 2);

  emg_rt::VectorView<float> noise_view(noise_centroids.data(),
                                       noise_centroids.size());
  emg_rt::VectorView<float> spike_view(spike_centroids.data(),
                                       spike_centroids.size());

  emg_rt::decomp::classify_discharges(discharges, pulse, candidates,
                                      noise_view, spike_view, samples_onset, 1,
                                      0);

  CHECK_FALSE(discharges(0, 1));
}

TEST_CASE("classify_discharges writes onset-adjusted discharge samples") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 0.9F, 0.0F},
  });
  auto candidates = emg_rt::tests::matrix_from_rows<bool>({
      {false, false, true, false},
  });

  std::vector<float> noise_centroids = {0.0F};
  std::vector<float> spike_centroids = {1.0F};
  std::vector<std::size_t> samples_onset = {1};
  emg_rt::RingMatrix<bool> discharges(1, 4);

  emg_rt::VectorView<float> noise_view(noise_centroids.data(),
                                       noise_centroids.size());
  emg_rt::VectorView<float> spike_view(spike_centroids.data(),
                                       spike_centroids.size());

  emg_rt::decomp::classify_discharges(discharges, pulse, candidates,
                                      noise_view, spike_view, samples_onset, 2,
                                      0);

  CHECK_FALSE(discharges(0, 0));
  CHECK(discharges(0, 1));
  CHECK_FALSE(discharges(0, 2));
  CHECK_FALSE(discharges(0, 3));
}

TEST_CASE("classify_discharges keeps motor-unit rows independent") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 1.2F, 0.0F},
      {0.0F, 0.4F, 2.8F},
  });
  auto candidates = emg_rt::tests::matrix_from_rows<bool>({
      {false, true, false},
      {false, true, true},
  });

  std::vector<float> noise_centroids = {0.0F, 0.0F};
  std::vector<float> spike_centroids = {1.0F, 3.0F};
  std::vector<std::size_t> samples_onset = {0, 0};
  emg_rt::RingMatrix<bool> discharges(2, 3);

  emg_rt::VectorView<float> noise_view(noise_centroids.data(),
                                       noise_centroids.size());
  emg_rt::VectorView<float> spike_view(spike_centroids.data(),
                                       spike_centroids.size());

  emg_rt::decomp::classify_discharges(discharges, pulse, candidates,
                                      noise_view, spike_view, samples_onset, 2,
                                      0);

  CHECK(discharges(0, 1));
  CHECK_FALSE(discharges(0, 2));
  CHECK_FALSE(discharges(1, 1));
  CHECK(discharges(1, 2));
}
