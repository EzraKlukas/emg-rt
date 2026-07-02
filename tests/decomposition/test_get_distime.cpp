#include "emg-rt/decomposition/classify_discharges.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <vector>

TEST_CASE("classify_discharges keeps spikes closer to spike centroid than "
          "noise centroid") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.1F, 0.9F, 1.2F},
      {0.2F, 2.8F, 3.1F},
  });
  auto spikes = emg_rt::tests::matrix_from_rows<bool>({
      {false, true, true},
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

  emg_rt::decomp::classify_discharges(discharges, pulse, spikes, noise_view,
                                      spike_view, samples_onset, 3, 0);

  CHECK_FALSE(discharges(0, 0));
  CHECK(discharges(0, 1));
  CHECK(discharges(0, 2));
  CHECK_FALSE(discharges(1, 0));
  CHECK(discharges(1, 1));
  CHECK(discharges(1, 2));
}

TEST_CASE("classify_discharges writes discharge at onset-adjusted sample") {
  auto pulse = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 0.9F, 0.1F},
  });
  auto spikes = emg_rt::tests::matrix_from_rows<bool>({
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

  emg_rt::decomp::classify_discharges(discharges, pulse, spikes, noise_view,
                                      spike_view, samples_onset, 2, 0);

  CHECK_FALSE(discharges(0, 0));
  CHECK(discharges(0, 1));
  CHECK_FALSE(discharges(0, 2));
  CHECK_FALSE(discharges(0, 3));
}
