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
 *   - `postfix_increment_read_head` returns the current read position and then
 *     advances the read head.
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

/*
 * Samples written to SignalRingBuffer are expected to contain all channels from
 * all grids.
 */
void AcquisitionRingBuffer::write_sample(uint64_t timestamp,
                                         const uint16_t *sample) noexcept {
  timestamps_[write_head_] = timestamp;
  float *sig_dst = &signals_[write_head_ * num_channels_];

  for (size_t ch = 0; ch < num_channels_; ++ch) {
    sig_dst[ch] = rhd_u16_to_microvolts(sample[ch]);
  }

  prefix_increment_write_head();
}

void AcquisitionRingBuffer::write_samples(size_t num_to_write,
                                          const uint64_t *timestamps,
                                          const uint16_t *samples) noexcept {
  EMG_RT_PROFILE(emg_rt::prof::Section::ring_write);
  uint64_t *ts_dst = timestamps_.data();
  float *sig_dst = signals_.data();

  for (size_t sample = 0; sample < num_to_write; ++sample) {
    ts_dst[write_head_] = timestamps[sample];
    for (size_t ch = 0; ch < num_channels_; ++ch) {
      sig_dst[(write_head_ * num_channels_) + ch] =
          rhd_u16_to_microvolts(samples[(sample * num_channels_) + ch]);
    }
    prefix_increment_write_head();
  }
}

// Increments, but returns current. (postfix increment)
size_t AcquisitionRingBuffer::postfix_increment_read_head() {
  const std::size_t old = read_head_;

  if (++read_head_ == size_) {
    read_head_ = 0;
  }

  return old;
}

// Increments before returning. (prefix increment)
size_t AcquisitionRingBuffer::prefix_increment_write_head() {
  if (++write_head_ == size_) {
    write_head_ = 0;
  }
  return write_head_;
}
