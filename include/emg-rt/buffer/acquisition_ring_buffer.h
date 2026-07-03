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

#ifndef ACQUISITION_RING_BUFFER_H
#define ACQUISITION_RING_BUFFER_H

#include "emg-rt/utils/types.h"
#include <cstddef>
#include <cstdint>
#include <vector>

#define MS_PER_S 1000

namespace emg_rt::buffer {

//
enum class PitayaImage : std::uint8_t {
  EIGHT_IMU_EIGHT_EMG,
  FOUR_IMU_TWELVE_EMG
};

enum class SensorType : std::uint8_t { EMG, IMU, ADC };

struct SensorBufferConfig {
  SensorType type_;
  std::size_t sampling_frequency_;
  std::size_t buffer_size_;
  std::size_t num_sensors_;
  std::size_t streams_per_sensor_;

  SensorBufferConfig(SensorType type, std::size_t sampling_frequency,
                     std::size_t num_sensors, std::size_t streams_per_sensor,
                     std::size_t buffer_duration_ms)
      : type_(type), sampling_frequency_(sampling_frequency),
        buffer_size_((sampling_frequency * buffer_duration_ms) / MS_PER_S),
        num_sensors_(num_sensors), streams_per_sensor_(streams_per_sensor) {}
};

inline constexpr std::size_t chan_per_imu = 0;
inline constexpr std::size_t chan_per_emg = 64;
inline constexpr std::size_t chan_per_adc = 0;

class AcquisitionMask {
public:
  AcquisitionMask(SensorType sensor_type, std::size_t num_active_streams)
      : mask_(num_active_streams), sensor_type_(sensor_type) {}

  void set_mask(const std::vector<std::size_t> &mask_indices);

  std::vector<std::size_t> mask() const { return mask_; }

private:
  std::vector<std::size_t> mask_;
  SensorType sensor_type_;
};

class AcquisitionRingBuffer {
public:
  AcquisitionRingBuffer(SensorBufferConfig sensor_cfg)
      : size_(sensor_cfg.buffer_size_),
        num_streams_(sensor_cfg.streams_per_sensor_ * sensor_cfg.num_sensors_),
        indices_(size(), 0), timestamps_(size(), 0),
        signal_(size() * num_streams(), 0) {};

  std::size_t size() const { return size_; }
  std::size_t num_streams() const { return num_streams_; }
  std::vector<float> &signal() noexcept { return signal_; }
  std::vector<size_t> &indices() noexcept { return indices_; }
  std::vector<uint64_t> &timestamps() noexcept { return timestamps_; }
  std::size_t write_head() const { return write_head_; }
  std::size_t oldest_index() const { return indices_[write_head_]; }
  std::size_t newest_index() const {
    if (write_head_ == 0) {
      return indices_[size_ - 1];
    }
    return indices_[write_head_ - 1];
  }

  std::size_t increment_heads();

  void write_sample(const std::uint64_t *timestamp_src,
                    const uint16_t *signal_src) noexcept;
  void write_samples(std::size_t num_to_write, const size_t *timestamps_src,
                     const uint16_t *signal_src) noexcept;

  std::size_t read_latest_samples(std::size_t num_to_read,
                                  const AcquisitionMask &acquisition_mask,
                                  RingVector<size_t> &indices_dst,
                                  RingVector<uint64_t> &timestamps_dst,
                                  RingMatrix<float> &signal_dst) noexcept;

  std::size_t read_samples(std::size_t num_to_read,
                           const AcquisitionMask &acquisition_mask,
                           std::size_t last_index_read,
                           RingVector<std::size_t> &indices_dst,
                           RingVector<uint64_t> &timestamps_dst,
                           RingMatrix<float> &signal_dst) noexcept;

private:
  std::size_t size_;
  std::size_t num_streams_;
  std::vector<std::size_t> indices_;
  std::vector<std::uint64_t> timestamps_;
  std::vector<float> signal_;
  std::size_t write_head_ = 0;
  std::size_t curr_index_ = 0;
};

class AcquisitionFrameBuffer {
public:
  AcquisitionFrameBuffer(const std::vector<SensorBufferConfig> &sensor_configs)
      : emg_(sensor_configs[static_cast<std::size_t>(SensorType::EMG)]),
        imu_(sensor_configs[static_cast<std::size_t>(SensorType::IMU)]),
        adc_(sensor_configs[static_cast<std::size_t>(SensorType::ADC)]) {};

  AcquisitionRingBuffer *emg_raw() { return &emg_; }
  AcquisitionRingBuffer *imu_raw() { return &imu_; }
  AcquisitionRingBuffer *adc_raw() { return &adc_; }

private:
  AcquisitionRingBuffer emg_;
  AcquisitionRingBuffer imu_;
  AcquisitionRingBuffer adc_;
};

} // namespace emg_rt::buffer

#endif // ACQUISITION_RING_BUFFER_H
