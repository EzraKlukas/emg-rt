/*
 * Implementation of acquisition ring buffers.
 *
 * `AcquisitionRingBuffer` stores one sensor type in fixed-size circular
 * storage. Incoming samples are written in sample-major order and tagged with a
 * monotonically increasing acquisition index. The current write path converts
 * raw unsigned 16-bit values using the Intan/RHD microvolt scale before storing
 * floats; that matches the present EMG replay data and should be revisited
 * before using the same path for non-EMG sensor streams.
 *
 * Layout:
 *
 *   signal_[sample * num_streams_ + stream]
 *   timestamps_[sample]
 *   indices_[sample]
 *
 * `AcquisitionMask` is a gather list, not a boolean mask. When a read copies a
 * full acquisition sample into a per-grid `RingMatrix`, each destination row
 * receives `sample[mask[row]]`, and the destination ring advances by one
 * logical column.
 *
 * Reads are based on acquisition indices instead of a shared read head.
 * `read_latest_samples` initializes a consumer from the newest retained
 * samples. `read_samples` uses a caller-owned `last_index_read` to copy the
 * next unread samples into destination index, timestamp, and signal rings.
 *
 * The implementation is single-object storage only. It does not provide
 * synchronization between producer and consumer threads.
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

// Samples written to the ring contain every stream for this sensor type.
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
 * Copy the newest retained samples into caller-owned ring buffers.
 *
 * This is used to initialize a consumer that does not yet have a
 * `last_index_read`. The caller is responsible for recording the acquisition
 * index it wants to use for subsequent incremental reads.
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
 * Copy the next unread retained samples after `last_index_read`.
 *
 * The destination rings receive acquisition indices, timestamps, and gathered
 * stream values. The returned index is the value the caller should retain for
 * the next incremental read.
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

  // decrement read_head to obtain final index read.
  if (read_head == 0) {
    read_head = size_ - 1;
  } else {
    --read_head;
  }

  return indices_src[read_head];
}

// Advance the write position and the monotonically increasing acquisition
// index.
size_t AcquisitionRingBuffer::increment_heads() {
  if (++write_head_ == size_) {
    write_head_ = 0;
  }
  ++curr_index_;
  return write_head_;
}
