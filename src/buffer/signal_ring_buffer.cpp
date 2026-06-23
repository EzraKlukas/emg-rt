#include "emg-rt/buffer/signal_ring_buffer.h"
#include "emg-rt/decomposition/online_decomposer.h"
#include "emg-rt/utils/types.h"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <utility>
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
void SignalRingBuffer::add_sample(Sample sample) {
  samples_[write_head_] = std::move(sample);
  ++write_head_;
  if (write_head_ == size_) {
    write_head_ -= size_;
  }
}

/*
 * Problem is Sample's not really useful outside of this context so won't use.
 */
Sample SignalRingBuffer::get_sample() {
  if (read_head == size_) {
    read_head -= size_;
  }
  return samples_[read_head++];
}

/*
 * Check later if this is efficient enough.
 */
void SignalRingBuffer::add_samples(std::vector<Sample> &src) {
  for (const Sample &sample : src) {
    add_sample(sample);
  }
}

void SignalRingBuffer::increment_read_head() {
  if (read_head_ == size_) {
    read_head_ -= size_;
  }
  ++read_head_;
}
