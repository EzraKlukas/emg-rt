/*
 * Raw timestamped EMG sample ring buffer.
 *
 * This class stores the newest raw EMG samples in a fixed-size circular buffer.
 * Each sample contains:
 *
 *   - one timestamp
 *   - one value for each physical acquisition channel
 *
 * Conceptually, this is the boundary between acquisition and processing.
 *
 * In the current offline replay pipeline, samples are loaded from a binary file
 * and written into this buffer. In the future live system, a sensor/acquisition
 * thread should write samples into the same buffer. The decomposition pipeline
 * can then read from this buffer without caring whether the data came from disk
 * or hardware.
 *
 * The signal values are stored as converted floating-point microvolt values.
 * The input write functions currently accept raw uint16_t ADC samples and apply
 * the Intan/RHD conversion before storing them.
 *
 * Layout:
 *
 *   timestamps_[sample_index]
 *   signals_[sample_index * num_channels_ + channel]
 *
 * `write_head_` marks where the next incoming sample will be written.
 * `read_head_` marks where the decomposition cycle will next read.
 *
 * This class is currently a simple single-process data structure. If one thread
 * writes while another thread reads in the final live system, the ownership and
 * synchronization rules should be made explicit before relying on it as a
 * thread-safe producer/consumer queue.
 */

#ifndef SIGNAL_RING_BUFFER_H
#define SIGNAL_RING_BUFFER_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace emg_rt::buffer {
class AcquisitionRingBuffer {
public:
  AcquisitionRingBuffer(std::size_t size, std::size_t num_channels)
      : size_(size), num_channels_(num_channels), timestamps_(size, 0),
        signals_(size * num_channels, 0) {};

  std::size_t size() const { return size_; }
  std::size_t num_channels() const { return num_channels_; }
  std::vector<float> &signals() noexcept { return signals_; }
  std::vector<uint64_t> &timestamps() noexcept { return timestamps_; }
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
} // namespace emg_rt::buffer

#endif // SIGNAL_RING_BUFFER_H
