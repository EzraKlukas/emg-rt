/*
 * Acquisition buffer types for timestamped sensor samples.
 *
 * The acquisition layer is split into small ownership pieces:
 *
 *   - `SensorType` names the supported sensor domains.
 *   - `SensorBufferConfig` describes one fixed-duration sensor buffer.
 *   - `AcquisitionRingBuffer` owns the circular data for one sensor type.
 *   - `AcquisitionFrameBuffer` owns the parallel EMG, IMU, and ADC buffers.
 *   - `AcquisitionMask` stores selected source stream indices used when
 *     gathering a full acquisition sample into a dense grid workspace.
 *
 * Each `AcquisitionRingBuffer` is configured from `SensorBufferConfig`.
 * `num_streams_` is computed as:
 *
 *   streams_per_sensor_ * num_sensors_
 *
 * The ring stores sample-major signal data plus one timestamp and one
 * monotonically increasing acquisition index per sample:
 *
 *   signal_[sample_index * num_streams_ + stream]
 *   timestamps_[sample_index]
 *   indices_[sample_index]
 *
 * `write_sample` and `write_samples` accept raw `uint16_t` values and currently
 * apply the Intan/RHD microvolt conversion before storing floats. That
 * conversion is appropriate for the present EMG replay path; callers should not
 * assume it is generally valid for every sensor type without revisiting the
 * conversion policy.
 *
 * Reads are consumer-index based. A consumer initializes from the newest
 * samples with `read_latest_samples`, then tracks its own `last_index_read` and
 * calls `read_samples` to copy the next unread samples. The buffer exposes
 * `oldest_index()` and `newest_index()` so callers can reason about whether
 * requested data is still retained.
 *
 * These classes do not enforce thread safety or multi-rate synchronization. If
 * live acquisition writes while decomposition reads, ownership, locking or
 * lock-free rules, timestamp meaning, and sensor-rate alignment need to be
 * defined outside this buffer first.
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
