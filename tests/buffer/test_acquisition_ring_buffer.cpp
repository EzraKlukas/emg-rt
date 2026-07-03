#include "emg-rt/buffer/acquisition_ring_buffer.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

using emg_rt::buffer::AcquisitionMask;
using emg_rt::buffer::AcquisitionRingBuffer;
using emg_rt::buffer::SensorBufferConfig;
using emg_rt::buffer::SensorType;

namespace {

SensorBufferConfig sensor_config(std::size_t buffer_size,
                                 std::size_t streams) {
  return SensorBufferConfig(SensorType::EMG, 1000, 1, streams, buffer_size);
}

std::vector<uint16_t> raw_count_samples(std::size_t samples,
                                        std::size_t streams) {
  std::vector<uint16_t> raw(samples * streams);
  for (std::size_t sample = 0; sample < samples; ++sample) {
    for (std::size_t stream = 0; stream < streams; ++stream) {
      raw[(sample * streams) + stream] =
          emg_rt::tests::raw_adc_from_counts(
              static_cast<int>((sample * 10) + stream));
    }
  }
  return raw;
}

} // namespace

TEST_CASE("AcquisitionRingBuffer construction derives size and stream count") {
  AcquisitionRingBuffer buffer(sensor_config(5, 3));

  CHECK(buffer.size() == 5);
  CHECK(buffer.num_streams() == 3);
  CHECK(buffer.write_head() == 0);
  CHECK(buffer.indices().size() == 5);
  CHECK(buffer.timestamps().size() == 5);
  CHECK(buffer.signal().size() == 15);
  CHECK(buffer.oldest_index() == 0);
  CHECK(buffer.newest_index() == 0);
}

TEST_CASE("write_sample stores index, timestamp, and converted stream values") {
  AcquisitionRingBuffer buffer(sensor_config(4, 2));
  const std::uint64_t timestamp = 42;
  const std::array<uint16_t, 2> raw = {
      emg_rt::tests::raw_adc_from_counts(1),
      emg_rt::tests::raw_adc_from_counts(-2),
  };

  buffer.write_sample(&timestamp, raw.data());

  CHECK(buffer.write_head() == 1);
  CHECK(buffer.indices()[0] == 0);
  CHECK(buffer.timestamps()[0] == 42);
  CHECK(buffer.signal()[0] ==
        doctest::Approx(emg_rt::tests::microvolts_from_counts(1)));
  CHECK(buffer.signal()[1] ==
        doctest::Approx(emg_rt::tests::microvolts_from_counts(-2)));
  CHECK(buffer.oldest_index() == 0);
  CHECK(buffer.newest_index() == 0);
}

TEST_CASE("write_samples preserves sample-major order across overwrite wrap") {
  AcquisitionRingBuffer buffer(sensor_config(4, 2));
  const std::array<std::size_t, 6> timestamps = {100, 101, 102,
                                                103, 104, 105};
  const std::vector<uint16_t> raw = raw_count_samples(6, 2);

  buffer.write_samples(timestamps.size(), timestamps.data(), raw.data());

  CHECK(buffer.write_head() == 2);
  CHECK(buffer.oldest_index() == 2);
  CHECK(buffer.newest_index() == 5);

  for (std::size_t logical = 0; logical < 4; ++logical) {
    const std::size_t sample_index = logical + 2;
    const std::size_t physical =
        (buffer.write_head() + logical) % buffer.size();
    CHECK(buffer.indices()[physical] == sample_index);
    CHECK(buffer.timestamps()[physical] == timestamps[sample_index]);
    CHECK(buffer.signal()[(physical * buffer.num_streams()) + 0] ==
          doctest::Approx(emg_rt::tests::microvolts_from_counts(
              static_cast<int>(sample_index * 10))));
    CHECK(buffer.signal()[(physical * buffer.num_streams()) + 1] ==
          doctest::Approx(emg_rt::tests::microvolts_from_counts(
              static_cast<int>((sample_index * 10) + 1))));
  }
}

TEST_CASE("read_latest_samples gathers newest samples through index mask") {
  AcquisitionRingBuffer buffer(sensor_config(5, 4));
  const std::array<std::size_t, 5> timestamps = {100, 101, 102, 103, 104};
  const std::vector<uint16_t> raw = raw_count_samples(5, 4);
  buffer.write_samples(timestamps.size(), timestamps.data(), raw.data());

  AcquisitionMask mask(SensorType::EMG, 2);
  mask.set_mask({3, 1});
  emg_rt::RingVector<std::size_t> indices(3);
  emg_rt::RingVector<std::uint64_t> read_timestamps(3);
  emg_rt::RingMatrix<float> signal(2, 3);

  buffer.read_latest_samples(3, mask, indices, read_timestamps, signal);

  emg_rt::tests::expect_vector_eq(indices, {2, 3, 4});
  emg_rt::tests::expect_vector_eq<std::uint64_t>(read_timestamps,
                                                 {102, 103, 104});
  emg_rt::tests::expect_matrix_approx(signal, {
                                                  {4.485F, 6.435F, 8.385F},
                                                  {4.095F, 6.045F, 7.995F},
                                              });
}

TEST_CASE("read_latest_samples can extract a full-capacity retained window") {
  AcquisitionRingBuffer buffer(sensor_config(4, 1));
  const std::array<std::size_t, 4> timestamps = {10, 11, 12, 13};
  const std::vector<uint16_t> raw = raw_count_samples(4, 1);
  buffer.write_samples(timestamps.size(), timestamps.data(), raw.data());

  AcquisitionMask mask(SensorType::EMG, 1);
  mask.set_mask({0});
  emg_rt::RingVector<std::size_t> indices(4);
  emg_rt::RingVector<std::uint64_t> read_timestamps(4);
  emg_rt::RingMatrix<float> signal(1, 4);

  buffer.read_latest_samples(4, mask, indices, read_timestamps, signal);

  emg_rt::tests::expect_vector_eq(indices, {0, 1, 2, 3});
  emg_rt::tests::expect_vector_eq<std::uint64_t>(read_timestamps,
                                                 {10, 11, 12, 13});
  emg_rt::tests::expect_matrix_approx(signal, {
                                                  {0.0F, 1.95F, 3.9F, 5.85F},
                                              });
}

TEST_CASE("read_samples returns the last copied index for an unwrapped window") {
  AcquisitionRingBuffer buffer(sensor_config(6, 1));
  const std::array<std::size_t, 6> timestamps = {10, 11, 12, 13, 14, 15};
  const std::vector<uint16_t> raw = raw_count_samples(6, 1);
  buffer.write_samples(timestamps.size(), timestamps.data(), raw.data());

  AcquisitionMask mask(SensorType::EMG, 1);
  mask.set_mask({0});
  emg_rt::RingVector<std::size_t> indices(2);
  emg_rt::RingVector<std::uint64_t> read_timestamps(2);
  emg_rt::RingMatrix<float> signal(1, 2);

  const std::size_t last_index =
      buffer.read_samples(2, mask, 3, indices, read_timestamps, signal);

  CHECK(last_index == 5);
  emg_rt::tests::expect_vector_eq(indices, {4, 5});
  emg_rt::tests::expect_vector_eq<std::uint64_t>(read_timestamps, {14, 15});
  emg_rt::tests::expect_matrix_approx(signal, {
                                                  {7.8F, 9.75F},
                                              });
}

TEST_CASE("read_samples returns the last copied index across physical wrap") {
  AcquisitionRingBuffer buffer(sensor_config(5, 1));
  const std::array<std::size_t, 7> timestamps = {10, 11, 12, 13,
                                                14, 15, 16};
  const std::vector<uint16_t> raw = raw_count_samples(7, 1);
  buffer.write_samples(timestamps.size(), timestamps.data(), raw.data());

  AcquisitionMask mask(SensorType::EMG, 1);
  mask.set_mask({0});
  emg_rt::RingVector<std::size_t> indices(2);
  emg_rt::RingVector<std::uint64_t> read_timestamps(2);
  emg_rt::RingMatrix<float> signal(1, 2);

  const std::size_t last_index =
      buffer.read_samples(2, mask, 4, indices, read_timestamps, signal);

  CHECK(last_index == 6);
  emg_rt::tests::expect_vector_eq(indices, {5, 6});
  emg_rt::tests::expect_vector_eq<std::uint64_t>(read_timestamps, {15, 16});
  emg_rt::tests::expect_matrix_approx(signal, {
                                                  {9.75F, 11.7F},
                                              });
}
