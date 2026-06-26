#ifndef SIGNAL_RING_BUFFER_H
#define SIGNAL_RING_BUFFER_H

#include <cstddef>
#include <cstdint>
#include <vector>

class SignalRingBuffer {
public:
  SignalRingBuffer(std::size_t size, std::size_t num_channels)
      : size_(size), num_channels_(num_channels), timestamps_(size, 0),
        signals_(size * num_channels, 0) {};

  std::size_t size() const { return size_; }
  std::size_t num_channels() const { return num_channels_; }
  std::vector<float> signals() const { return signals_; }
  std::vector<uint64_t> timestamps() const { return timestamps_; }
  std::size_t write_head() const { return write_head_; }
  std::size_t read_head() const { return read_head_; }

  std::size_t postfix_increment_read_head();
  std::size_t prefix_increment_write_head();

  void write_sample(uint64_t timestamp, const uint16_t *sample) noexcept;
  void write_samples(std::size_t num_to_write, const uint64_t *timestamps,
                     const uint16_t *samples) noexcept;

private:
  std::size_t size_;
  std::size_t num_channels_;
  std::vector<uint64_t> timestamps_;
  std::vector<float> signals_;
  std::size_t write_head_ = 0;
  std::size_t read_head_ = 0;
};

#endif // SIGNAL_RING_BUFFER_H
