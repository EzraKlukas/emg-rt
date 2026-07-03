#include "emg-rt/buffer/acquisition_ring_buffer.h"
#include "emg-rt/decomposition/classify_discharges.h"
#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/decomposition/is_local_max.h"
#include "emg-rt/decomposition/online_decomposer.h"
#include "emg-rt/dsp/demean.h"
#include "emg-rt/dsp/extend.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace {

void classify(emg_rt::RingMatrix<bool> &discharges,
              const emg_rt::RingMatrix<float> &pulse,
              const emg_rt::RingMatrix<bool> &candidates,
              std::vector<float> &noise_centroids,
              std::vector<float> &spike_centroids,
              const std::vector<std::size_t> &samples_onset,
              std::size_t new_samples) {
  emg_rt::VectorView<float> noise_view(noise_centroids.data(),
                                       noise_centroids.size());
  emg_rt::VectorView<float> spike_view(spike_centroids.data(),
                                       spike_centroids.size());
  emg_rt::decomp::classify_discharges(discharges, pulse, candidates, noise_view,
                                      spike_view, samples_onset, new_samples,
                                      0);
}

} // namespace

TEST_CASE("single-grid single-MU synthetic pipeline detects expected sample") {
  auto raw = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 0.0F, -2.0F, 4.0F, -2.0F},
  });
  emg_rt::RingMatrix<float> extended(1, 3);
  std::vector<float> sums = emg_rt::dsp::initial_sums(raw, 3, 1);

  emg_rt::dsp::incremental_extend(extended, raw, 1, sums, 3, 3);
  emg_rt::dsp::incremental_demean(extended, 3, sums);

  std::vector<float> filters = {1.0F};
  std::vector<float> inv_norms = {1.0F};
  emg_rt::RingMatrix<float> pulse(1, 3);
  emg_rt::MatrixView<float> filter_view(filters.data(), 1, 1);
  emg_rt::VectorView<float> norm_view(inv_norms.data(), inv_norms.size());
  emg_rt::decomp::get_pulse_train(pulse, extended, filter_view, norm_view);

  emg_rt::RingMatrix<bool> candidates(1, 3);
  std::vector<float> maxima = {0.0F};
  emg_rt::decomp::incremental_is_local_max(pulse, candidates, 3, maxima, 0);

  std::vector<float> noise_centroids = {0.0F};
  std::vector<float> spike_centroids = {16.0F};
  std::vector<std::size_t> samples_onset = {0};
  emg_rt::RingMatrix<bool> discharges(1, 3);
  classify(discharges, pulse, candidates, noise_centroids, spike_centroids,
           samples_onset, 3);

  CHECK_FALSE(discharges(0, 0));
  CHECK(discharges(0, 1));
  CHECK_FALSE(discharges(0, 2));
}

TEST_CASE("single-grid two-MU synthetic pipeline keeps filters separated") {
  auto raw = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 0.0F, -2.0F, 4.0F, -2.0F},
      {0.0F, 0.0F, 0.0F, -1.0F, 3.0F, -2.0F},
  });
  emg_rt::RingMatrix<float> extended(2, 3);
  std::vector<float> sums = emg_rt::dsp::initial_sums(raw, 3, 1);

  emg_rt::dsp::incremental_extend(extended, raw, 1, sums, 3, 3);
  emg_rt::dsp::incremental_demean(extended, 3, sums);

  std::vector<float> filters = {
      1.0F,
      0.0F,
      0.0F,
      1.0F,
  };
  std::vector<float> inv_norms = {1.0F, 1.0F};
  emg_rt::RingMatrix<float> pulse(2, 3);
  emg_rt::MatrixView<float> filter_view(filters.data(), 2, 2);
  emg_rt::VectorView<float> norm_view(inv_norms.data(), inv_norms.size());
  emg_rt::decomp::get_pulse_train(pulse, extended, filter_view, norm_view);

  emg_rt::RingMatrix<bool> candidates(2, 3);
  std::vector<float> maxima = {0.0F, 0.0F};
  emg_rt::decomp::incremental_is_local_max(pulse, candidates, 3, maxima, 0);

  std::vector<float> noise_centroids = {0.0F, 0.0F};
  std::vector<float> spike_centroids = {16.0F, 9.0F};
  std::vector<std::size_t> samples_onset = {0, 0};
  emg_rt::RingMatrix<bool> discharges(2, 3);
  classify(discharges, pulse, candidates, noise_centroids, spike_centroids,
           samples_onset, 3);

  CHECK_FALSE(discharges(0, 0));
  CHECK(discharges(0, 1));
  CHECK_FALSE(discharges(0, 2));
  CHECK_FALSE(discharges(1, 0));
  CHECK(discharges(1, 1));
  CHECK_FALSE(discharges(1, 2));
}

TEST_CASE(
    "wrapped raw buffer gives same single-MU output as contiguous input") {
  auto raw = emg_rt::tests::matrix_from_rows<float>({
      {9.0F, 8.0F, 7.0F, 6.0F, 5.0F, 4.0F},
  });
  const float junk0[] = {99.0F};
  const float junk1[] = {88.0F};
  const float c0[] = {0.0F};
  const float c1[] = {0.0F};
  const float c2[] = {0.0F};
  const float c3[] = {-2.0F};
  const float c4[] = {4.0F};
  const float c5[] = {-2.0F};
  raw.write_column(junk0);
  raw.write_column(junk1);
  raw.write_column(c0);
  raw.write_column(c1);
  raw.write_column(c2);
  raw.write_column(c3);
  raw.write_column(c4);
  raw.write_column(c5);

  emg_rt::RingMatrix<float> extended(1, 3);
  std::vector<float> sums = emg_rt::dsp::initial_sums(raw, 3, 1);
  emg_rt::dsp::incremental_extend(extended, raw, 1, sums, 3, 3);
  emg_rt::dsp::incremental_demean(extended, 3, sums);

  CHECK(raw.head == 2);
  CHECK(extended(0, 0) == doctest::Approx(-2.0F));
  CHECK(extended(0, 1) == doctest::Approx(4.0F));
  CHECK(extended(0, 2) == doctest::Approx(-2.0F));
}

TEST_CASE("non-contiguous active-channel copy preserves physical channel map") {
  emg_rt::buffer::AcquisitionRingBuffer buffer(3, 6);
  const std::array<uint64_t, 3> timestamps = {10, 11, 12};
  std::array<uint16_t, 18> samples{};

  for (std::size_t sample = 0; sample < timestamps.size(); ++sample) {
    for (std::size_t channel = 0; channel < 6; ++channel) {
      samples[(sample * 6) + channel] =
          emg_rt::tests::raw_adc_from_counts(static_cast<int>(100 + channel));
    }
  }
  samples[(0 * 6) + 5] = emg_rt::tests::raw_adc_from_counts(-2);
  samples[(1 * 6) + 5] = emg_rt::tests::raw_adc_from_counts(4);
  samples[(2 * 6) + 5] = emg_rt::tests::raw_adc_from_counts(-2);
  buffer.write_samples(timestamps.size(), timestamps.data(), samples.data());

  const std::vector<std::size_t> active_channels = {0, 2, 5};
  emg_rt::RingMatrix<float> raw_grid(active_channels.size(), 3);
  for (std::size_t sample = 0; sample < 3; ++sample) {
    for (std::size_t active = 0; active < active_channels.size(); ++active) {
      raw_grid(active, sample) =
          buffer.signal()[(sample * buffer.num_streams()) +
                          active_channels[active]];
    }
  }

  CHECK(raw_grid(2, 0) ==
        doctest::Approx(emg_rt::tests::microvolts_from_counts(-2)));
  CHECK(raw_grid(2, 1) ==
        doctest::Approx(emg_rt::tests::microvolts_from_counts(4)));
  CHECK(raw_grid(2, 2) ==
        doctest::Approx(emg_rt::tests::microvolts_from_counts(-2)));
}

TEST_CASE("MultiGridDecomposer get_samples reads the same acquisition samples "
          "for each grid") {
  emg_rt::config::OnlineDecompositionConfig config{
      .sampling_frequency = 1000.0F,
      .decomposition_frequency = 1000.0F,
      .demean_window_size = 50,
      .samples_per_cycle = 1,
      .min_lookback_samps = 0,
      .min_lookahead_samps = 0,
      .tgt_ext_channels = 1,
  };

  emg_rt::GridDecomposer grid0(0, {0}, {0}, {1.0F}, {0.0F}, {1.0F}, {1.0F}, 1,
                               1, 1, 50, 0);
  emg_rt::GridDecomposer grid1(1, {0}, {0}, {1.0F}, {0.0F}, {1.0F}, {1.0F}, 1,
                               1, 1, 50, 0);
  std::vector<emg_rt::GridDecomposer> grids;
  grids.push_back(std::move(grid0));
  grids.push_back(std::move(grid1));
  emg_rt::MultiGridDecomposer decomposer(config, std::move(grids));

  emg_rt::buffer::AcquisitionRingBuffer acquisition(2, 128);
  std::array<uint64_t, 2> timestamps = {100, 101};
  std::array<uint16_t, 256> samples{};
  for (auto &sample : samples) {
    sample = emg_rt::tests::raw_adc_from_counts(0);
  }
  samples[0] = emg_rt::tests::raw_adc_from_counts(1);
  samples[64] = emg_rt::tests::raw_adc_from_counts(2);
  samples[128] = emg_rt::tests::raw_adc_from_counts(9);
  samples[192] = emg_rt::tests::raw_adc_from_counts(9);
  acquisition.write_samples(2, timestamps.data(), samples.data());

  decomposer.read_samples(acquisition, 1);

  CHECK(decomposer.grids()[0].workspace().timestamps(0) == 100);
  CHECK(decomposer.grids()[1].workspace().timestamps(0) == 100);
  CHECK(decomposer.grids()[0].workspace().raw_grid_window(0, 0) ==
        doctest::Approx(emg_rt::tests::microvolts_from_counts(1)));
  CHECK(decomposer.grids()[1].workspace().raw_grid_window(0, 0) ==
        doctest::Approx(emg_rt::tests::microvolts_from_counts(2)));
}
