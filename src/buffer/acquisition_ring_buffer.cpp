/*
 * Implementation of the raw EMG sample ring buffer.
 *
 * The write functions convert raw unsigned 16-bit amplifier samples into
 * floating-point microvolts before storing them. This keeps downstream
 * decomposition code independent of the ADC representation used by the
 * acquisition hardware.
 *
 * The ring stores all channels from all grids in one sample-major layout:
 *
 *   signals_[sample * num_channels + channel]
 *
 * Higher-level decomposition code later selects the active channels belonging
 * to each grid and copies them into per-grid working buffers.
 *
 * `write_sample` writes one timestamped sample.
 * `write_samples` writes a block of consecutive timestamped samples.
 *
 * The read/write head helpers expose circular-buffer movement:
 *
 *   - `prefix_increment_write_head` advances the write head after a write.
 *
 * If this buffer is used for true live acquisition, block writes must preserve
 * circular-buffer semantics when the write crosses the physical end of the
 * underlying vectors.
 */

#include "emg-rt/buffer/acquisition_ring_buffer.h"
#include "emg-rt/profiling/timer.h"

#include <cassert>
#include <cstddef>
#include <vector>

using namespace std;
using namespace emg_rt::buffer;

static constexpr float uV_per_count = 0.195F;
static constexpr int32_t adc_midscale = 32768;

static inline float rhd_u16_to_microvolts(uint16_t raw) {
  return static_cast<float>(static_cast<int32_t>(raw) - adc_midscale) *
         uV_per_count;
}

void AcquisitionMask::set_mask(const vector<std::size_t> &mask_indices) {
  mask_ = mask_indices;
}

/*
 * Samples written to SignalRingBuffer are expected to contain all channels from
 * all grids.
 */
void AcquisitionRingBuffer::write_sample(const std::uint64_t *timestamp_src,
                                         const uint16_t *signal_src) noexcept {
  float *sig_dst = &signal_[write_head_ * num_streams_];

  indices_[write_head_] = curr_index_;
  timestamps_[write_head_] = timestamp_src[0];

  for (size_t ch = 0; ch < num_streams_; ++ch) {
    sig_dst[ch] = rhd_u16_to_microvolts(signal_src[ch]);
  }

  increment_heads();
}

void AcquisitionRingBuffer::write_samples(size_t num_to_write,
                                          const uint64_t *timestamps_src,
                                          const uint16_t *signal_src) noexcept {
  EMG_RT_PROFILE(emg_rt::prof::Section::ring_write);
  uint64_t *timestamps_dst = timestamps_.data();
  float *signal_dst = signal_.data();

  for (size_t sample = 0; sample < num_to_write; ++sample) {
    indices_[write_head_] = curr_index_;
    timestamps_dst[write_head_] = timestamps_src[sample];
    for (size_t ch = 0; ch < num_streams_; ++ch) {
      signal_dst[(write_head_ * num_streams_) + ch] =
          rhd_u16_to_microvolts(signal_src[(sample * num_streams_) + ch]);
    }

    increment_heads();
  }
}

/*
 * Reads the latest num_to_read samples out of the acquisition ring buffer, and
 * returns the timestamp of the last sample read. This makes it easy for readers
 * to initialize buffers when they haven't yet kept track of timestamps, while
 * allowing the next read to draw samples starting at the first timestamp the
 * reader hasn't read from yet, as opposed to unwisely trusting that drawing N
 * newest samples will always yield data without partition between reads, and
 * without missed timestamps.
 *
 * Later: change from sample_dst to emg_dst, imu_dst, and adc_dst, or smth
 * similar.
 */
uint64_t AcquisitionRingBuffer::read_latest_samples(
    std::size_t num_to_read, const AcquisitionMask &acquisition_mask,
    RingVector<size_t> &indices_dst, RingVector<uint64_t> &timestamps_dst,
    RingMatrix<float> &signal_dst) noexcept {
  EMG_RT_PROFILE(emg_rt::prof::Section::ring_read_latest);
  size_t *indices_src = indices_.data();
  uint64_t *timestamps_src = timestamps_.data();
  float *signal_src = signal_.data();

  assert(num_to_read < size_);

  std::size_t read_head;
  if (write_head_ < num_to_read) {
    read_head = (write_head_ + size_) - num_to_read;
  } else {
    read_head = write_head_ - num_to_read;
  }

  for (size_t sample = 0; sample < num_to_read; ++sample) {
    indices_dst(sample) = indices_src[read_head];
    timestamps_dst(sample) = timestamps_src[read_head];
    signal_dst.write_column(&signal_src[read_head * num_streams_],
                            acquisition_mask.mask());
    if (++read_head == size_) {
      read_head = 0;
    }
  }

  return timestamps_src[read_head];
}

/*
 * Given the last timestamp that a reader read from and the amount of new data
 * it would like, the acquisition ring buffer confirms that the queried data
 * exists, copies its data into the reader, and returns the last timestamp that
 * the reader read from, as an argument to this function the next time it would
 * like to read.
 */
std::size_t AcquisitionRingBuffer::read_samples(
    std::size_t num_to_read, const AcquisitionMask &acquisition_mask,
    uint64_t last_index_read, RingVector<size_t> &indices_dst,
    RingVector<uint64_t> &timestamps_dst,
    RingMatrix<float> &signal_dst) noexcept {
  EMG_RT_PROFILE(emg_rt::prof::Section::ring_read);
  size_t *indices_src = indices_.data();
  uint64_t *timestamps_src = timestamps_.data();
  float *signal_src = signal_.data();

  assert(num_to_read < size_);
  assert(oldest_index() < last_index_read);
  assert(newest_index() - last_index_read >= num_to_read);

  std::size_t read_head;
  if (write_head_ < newest_index() - last_index_read) {
    read_head = (write_head_ + size_) + (last_index_read - newest_index());
  } else {
    read_head = write_head_ + (last_index_read - newest_index());
  }

  for (size_t sample = 0; sample < num_to_read; ++sample) {
    indices_dst(sample) = indices_src[read_head];
    timestamps_dst(sample) = timestamps_src[read_head];
    signal_dst.write_column(&signal_src[read_head * num_streams_],
                            acquisition_mask.mask());
    if (++read_head == size_) {
      read_head = 0;
    }
  }

  return indices_src[read_head];
}

// Increments before returning. (prefix increment)
size_t AcquisitionRingBuffer::increment_heads() {
  if (++write_head_ == size_) {
    write_head_ = 0;
  }
  ++curr_index_;
  return write_head_;
}
