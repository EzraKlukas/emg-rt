#include "emg-rt/buffer/signal_ring_buffer.h"

#include <cassert>
#include <cstddef>
#include <vector>

using namespace std;

static constexpr float uV_per_count = 0.195F;
static constexpr int32_t adc_midscale = 32768;

static inline float rhd_u16_to_microvolts(uint16_t raw) {
  return static_cast<float>(static_cast<int32_t>(raw) - adc_midscale) *
         uV_per_count;
}

/*
 * Samples written to SignalRingBuffer are expected to contain all channels from
 * all grids.
 */
void SignalRingBuffer::write_sample(uint64_t timestamp,
                                    const uint16_t *sample) noexcept {
  timestamps_[write_head_] = timestamp;
  float *sig_dst = &signals_[write_head_ * num_channels_];

  for (size_t ch = 0; ch < num_channels_; ++ch) {
    sig_dst[ch] = rhd_u16_to_microvolts(sample[ch]);
  }

  prefix_increment_write_head();
}

void SignalRingBuffer::write_samples(size_t num_to_write,
                                     const uint64_t *timestamps,
                                     const uint16_t *samples) noexcept {
  uint64_t *ts_dst = &timestamps_[write_head_];
  float *sig_dst = &signals_[write_head_ * num_channels_];

  for (size_t sample = 0; sample < num_to_write; ++sample) {
    ts_dst[sample] = timestamps[sample];
    for (size_t ch = 0; ch < num_channels_; ++ch) {
      sig_dst[(sample * num_channels_) + ch] =
          rhd_u16_to_microvolts(samples[(sample * num_channels_) + ch]);
    }
    prefix_increment_write_head();
  }
}

// Increments, but returns current. (postfix increment)
size_t SignalRingBuffer::postfix_increment_read_head() {
  if (read_head_ == size_ - 1) {
    read_head_ = 0;
    return read_head_;
  }
  return read_head_++;
}

// Increments before returning. (prefix increment)
size_t SignalRingBuffer::prefix_increment_write_head() {
  if (++write_head_ == size_) {
    write_head_ = 0;
  }
  return write_head_;
}
