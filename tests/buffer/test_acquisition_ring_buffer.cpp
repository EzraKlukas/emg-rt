#include "emg-rt/buffer/acquisition_ring_buffer.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <array>
#include <cstddef>
#include <cstdint>

using emg_rt::buffer::AcquisitionRingBuffer;

TEST_CASE("AcquisitionRingBuffer writes timestamped samples without wrap") {
  AcquisitionRingBuffer buffer(5, 2);
  const std::array<uint64_t, 3> timestamps = {100, 101, 102};
  const std::array<uint16_t, 6> samples = {
      emg_rt::tests::raw_adc_from_counts(1),
      emg_rt::tests::raw_adc_from_counts(2),
      emg_rt::tests::raw_adc_from_counts(3),
      emg_rt::tests::raw_adc_from_counts(4),
      emg_rt::tests::raw_adc_from_counts(5),
      emg_rt::tests::raw_adc_from_counts(6),
  };

  buffer.write_samples(timestamps.size(), timestamps.data(), samples.data());

  CHECK(buffer.write_head() == 3);
  CHECK(buffer.read_head() == 0);
  CHECK(buffer.indices()[0] == 100);
  CHECK(buffer.indices()[1] == 101);
  CHECK(buffer.indices()[2] == 102);
  CHECK(buffer.signal()[0] ==
        doctest::Approx(emg_rt::tests::microvolts_from_counts(1)));
  CHECK(buffer.signal()[1] ==
        doctest::Approx(emg_rt::tests::microvolts_from_counts(2)));
  CHECK(buffer.signal()[4] ==
        doctest::Approx(emg_rt::tests::microvolts_from_counts(5)));
  CHECK(buffer.signal()[5] ==
        doctest::Approx(emg_rt::tests::microvolts_from_counts(6)));
}

TEST_CASE(
    "AcquisitionRingBuffer exact wrap preserves channel/timestamp pairs") {
  AcquisitionRingBuffer buffer(4, 3);
  const std::array<uint64_t, 4> timestamps = {10, 11, 12, 13};
  const std::array<uint16_t, 12> samples = {
      emg_rt::tests::raw_adc_from_counts(10),
      emg_rt::tests::raw_adc_from_counts(11),
      emg_rt::tests::raw_adc_from_counts(12),
      emg_rt::tests::raw_adc_from_counts(20),
      emg_rt::tests::raw_adc_from_counts(21),
      emg_rt::tests::raw_adc_from_counts(22),
      emg_rt::tests::raw_adc_from_counts(30),
      emg_rt::tests::raw_adc_from_counts(31),
      emg_rt::tests::raw_adc_from_counts(32),
      emg_rt::tests::raw_adc_from_counts(40),
      emg_rt::tests::raw_adc_from_counts(41),
      emg_rt::tests::raw_adc_from_counts(42),
  };

  buffer.write_samples(timestamps.size(), timestamps.data(), samples.data());

  CHECK(buffer.write_head() == 0);
  for (std::size_t sample = 0; sample < timestamps.size(); ++sample) {
    CHECK(buffer.indices()[sample] == timestamps[sample]);
    for (std::size_t channel = 0; channel < buffer.num_streams(); ++channel) {
      const int counts =
          static_cast<int>((sample + 1) * 10 + static_cast<int>(channel));
      CHECK(buffer.signal()[(sample * buffer.num_streams()) + channel] ==
            doctest::Approx(emg_rt::tests::microvolts_from_counts(counts)));
    }
  }
}

TEST_CASE("AcquisitionRingBuffer keeps newest capacity after overwrite wrap") {
  AcquisitionRingBuffer buffer(4, 2);
  const std::array<uint64_t, 6> timestamps = {0, 1, 2, 3, 4, 5};
  const std::array<uint16_t, 12> samples = {
      emg_rt::tests::raw_adc_from_counts(0),
      emg_rt::tests::raw_adc_from_counts(100),
      emg_rt::tests::raw_adc_from_counts(1),
      emg_rt::tests::raw_adc_from_counts(101),
      emg_rt::tests::raw_adc_from_counts(2),
      emg_rt::tests::raw_adc_from_counts(102),
      emg_rt::tests::raw_adc_from_counts(3),
      emg_rt::tests::raw_adc_from_counts(103),
      emg_rt::tests::raw_adc_from_counts(4),
      emg_rt::tests::raw_adc_from_counts(104),
      emg_rt::tests::raw_adc_from_counts(5),
      emg_rt::tests::raw_adc_from_counts(105),
  };

  buffer.write_samples(timestamps.size(), timestamps.data(), samples.data());

  CHECK(buffer.write_head() == 2);
  const std::array<uint64_t, 4> newest_timestamps = {2, 3, 4, 5};
  for (std::size_t logical = 0; logical < newest_timestamps.size(); ++logical) {
    const std::size_t physical =
        (buffer.write_head() + logical) % buffer.size();
    CHECK(buffer.indices()[physical] == newest_timestamps[logical]);
    CHECK(buffer.signal()[(physical * buffer.num_streams()) + 0] ==
          doctest::Approx(emg_rt::tests::microvolts_from_counts(
              static_cast<int>(newest_timestamps[logical]))));
    CHECK(buffer.signal()[(physical * buffer.num_streams()) + 1] ==
          doctest::Approx(emg_rt::tests::microvolts_from_counts(
              static_cast<int>(100 + newest_timestamps[logical]))));
  }
}

TEST_CASE("AcquisitionRingBuffer read head wraps with postfix semantics") {
  AcquisitionRingBuffer buffer(3, 1);

  CHECK(buffer.postfix_increment_read_head() == 0);
  CHECK(buffer.read_head() == 1);
  CHECK(buffer.postfix_increment_read_head() == 1);
  CHECK(buffer.read_head() == 2);
  CHECK(buffer.postfix_increment_read_head() == 2);
  CHECK(buffer.read_head() == 0);
}
