#ifndef SIGNAL_RING_BUFFER_H
#define SIGNAL_RING_BUFFER_H

#include "emg-rt/utils/types.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct Sample {
  uint64_t timestamp;
  std::vector<uint16_t> signal;

  Sample(uint64_t timestamp, std::size_t num_channels)
      : timestamp(timestamp), signal(num_channels) {};
};

class SignalRingBuffer {
public:
  SignalRingBuffer(std::size_t size) : size_(size) { samples_.reserve(size); };

  std::size_t size() const { return size_; }
  std::vector<Sample> samples() const { return samples_; }
  std::size_t write_head() const { return write_head_; }
  std::size_t read_head() const { return read_head_; }

  void increment_read_head();

  void add_sample(Sample sample);
  Sample get_sample(); // can only get most recent unread sample
  void add_samples(std::vector<Sample> &src);

private:
  std::size_t size_;
  std::vector<Sample> samples_;
  std::size_t write_head_ = 0;
  std::size_t read_head_ = 0;
};

#endif // SIGNAL_RING_BUFFER_H
