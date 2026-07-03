#include "emg-rt/buffer/acquisition_ring_buffer.h"
#include "emg-rt/decomposition/classify_discharges.h"
#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/decomposition/is_local_max.h"
#include "emg-rt/dsp/extend.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

TEST_CASE("acquisition gather feeds temporal extension and pulse projection") {
  emg_rt::buffer::SensorBufferConfig cfg(
      emg_rt::buffer::SensorType::EMG, 1000, 1, 3, 5);
  emg_rt::buffer::AcquisitionRingBuffer acquisition(cfg);
  const std::array<std::size_t, 5> timestamps = {10, 11, 12, 13, 14};
  const std::array<uint16_t, 15> raw = {
      emg_rt::tests::raw_adc_from_counts(0),
      emg_rt::tests::raw_adc_from_counts(10),
      emg_rt::tests::raw_adc_from_counts(100),
      emg_rt::tests::raw_adc_from_counts(1),
      emg_rt::tests::raw_adc_from_counts(11),
      emg_rt::tests::raw_adc_from_counts(101),
      emg_rt::tests::raw_adc_from_counts(2),
      emg_rt::tests::raw_adc_from_counts(12),
      emg_rt::tests::raw_adc_from_counts(102),
      emg_rt::tests::raw_adc_from_counts(3),
      emg_rt::tests::raw_adc_from_counts(13),
      emg_rt::tests::raw_adc_from_counts(103),
      emg_rt::tests::raw_adc_from_counts(4),
      emg_rt::tests::raw_adc_from_counts(14),
      emg_rt::tests::raw_adc_from_counts(104),
  };
  acquisition.write_samples(timestamps.size(), timestamps.data(), raw.data());

  emg_rt::buffer::AcquisitionMask mask(emg_rt::buffer::SensorType::EMG, 1);
  mask.set_mask({1});
  emg_rt::RingVector<std::size_t> indices(4);
  emg_rt::RingVector<std::uint64_t> read_timestamps(4);
  emg_rt::RingMatrix<float> raw_grid_window(1, 4);
  acquisition.read_latest_samples(4, mask, indices, read_timestamps,
                                  raw_grid_window);

  emg_rt::RingMatrix<float> extended(1, 2);
  std::vector<float> sums(1, 0.0F);
  emg_rt::dsp::incremental_extend(extended, raw_grid_window, 1, sums, 2, 2);

  std::vector<float> filters = {1.0F};
  std::vector<float> inv_norms = {1.0F};
  emg_rt::RingMatrix<float> pulse(1, 2);
  emg_rt::decomp::get_pulse_train(
      pulse, extended, emg_rt::MatrixView<float>(filters.data(), 1, 1),
      emg_rt::VectorView<float>(inv_norms.data(), 1));

  emg_rt::tests::expect_vector_eq(indices, {1, 2, 3, 4});
  emg_rt::tests::expect_vector_eq<std::uint64_t>(read_timestamps,
                                                 {11, 12, 13, 14});
  emg_rt::tests::expect_matrix_approx(raw_grid_window, {
                                                           {2.145F, 2.34F,
                                                            2.535F, 2.73F},
                                                       });
  emg_rt::tests::expect_matrix_approx(extended, {
                                                    {2.535F, 2.73F},
                                                });
  CHECK(pulse(0, 0) == doctest::Approx(6.426225F));
  CHECK(pulse(0, 1) == doctest::Approx(7.4529F));
}

TEST_CASE("direct decomposition primitives classify a clear synthetic discharge") {
  auto pulse_train = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 1.0F, 9.0F, 1.0F},
  });
  emg_rt::RingMatrix<bool> candidates(1, 5);
  std::vector<float> maxima = {2.0F};
  emg_rt::decomp::incremental_is_local_max(pulse_train, candidates, 2, maxima,
                                           1);

  emg_rt::RingMatrix<bool> discharges(1, 5);
  std::vector<float> noise = {0.0F};
  std::vector<float> spike = {10.0F};
  std::vector<std::size_t> onset = {0};

  emg_rt::decomp::classify_discharges(
      discharges, pulse_train, candidates,
      emg_rt::VectorView<float>(noise.data(), 1),
      emg_rt::VectorView<float>(spike.data(), 1), onset, 2, 1);

  CHECK(candidates(0, 3));
  CHECK(discharges(0, 3));
}

TEST_CASE("streaming read_samples can continue after read_latest initialization") {
  emg_rt::buffer::SensorBufferConfig cfg(
      emg_rt::buffer::SensorType::EMG, 1000, 1, 1, 6);
  emg_rt::buffer::AcquisitionRingBuffer acquisition(cfg);
  const std::array<std::size_t, 6> initial_timestamps = {0, 1, 2, 3, 4, 5};
  std::vector<uint16_t> initial_raw;
  for (std::size_t i = 0; i < initial_timestamps.size(); ++i) {
    initial_raw.push_back(
        emg_rt::tests::raw_adc_from_counts(static_cast<int>(i)));
  }
  acquisition.write_samples(initial_timestamps.size(),
                            initial_timestamps.data(), initial_raw.data());

  emg_rt::buffer::AcquisitionMask mask(emg_rt::buffer::SensorType::EMG, 1);
  mask.set_mask({0});
  emg_rt::RingVector<std::size_t> indices(3);
  emg_rt::RingVector<std::uint64_t> timestamps(3);
  emg_rt::RingMatrix<float> signal(1, 3);
  acquisition.read_latest_samples(3, mask, indices, timestamps, signal);
  std::size_t last_index_read = 5;

  const std::array<std::size_t, 2> next_timestamps = {6, 7};
  const std::array<uint16_t, 2> next_raw = {
      emg_rt::tests::raw_adc_from_counts(6),
      emg_rt::tests::raw_adc_from_counts(7),
  };
  acquisition.write_samples(next_timestamps.size(), next_timestamps.data(),
                            next_raw.data());

  last_index_read =
      acquisition.read_samples(2, mask, last_index_read, indices, timestamps,
                               signal);

  CHECK(last_index_read == 7);
  emg_rt::tests::expect_vector_eq(indices, {6, 7, 5});
  CHECK(signal(0, 0) == doctest::Approx(0.975F));
  CHECK(signal(0, 1) == doctest::Approx(1.17F));
  CHECK(signal(0, 2) == doctest::Approx(1.365F));
}
