/*
 * Online decomposition configuration.
 *
 * `OnlineDecompositionConfig` stores the timing and buffer-size parameters that
 * control the online EMG decomposition loop. Some fields come directly from the
 * YAML configuration, while others are derived from those values.
 *
 * Direct configuration values:
 *
 *   - `sampling_frequency`
 *       EMG sample rate in samples per second.
 *
 *   - `decomposition_frequency`
 *       How often the online decomposition loop runs.
 *
 *   - `demean_window_size`
 *       Number of samples used for the rolling demeaning window.
 *
 *   - `tgt_ext_channels`
 *       Target number of temporally extended channels requested by the offline
 *       training/export pipeline.
 *
 * Derived values:
 *
 *   - `samples_per_cycle`
 *       Number of new EMG samples processed each decomposition cycle.
 *
 *   - `min_lookback_samps`
 *       Amount of pulse-train history retained so local-max and discharge
 *       decisions have enough past context.
 *
 *   - `min_lookahead_samps`
 *       Number of future samples to wait before confirming a local maximum.
 *
 * The validation bounds are intentionally conservative sanity checks. They are
 * not physiological laws; they exist to catch configuration mistakes before
 * buffers are allocated and the real-time loop begins.
 */

#ifndef DECOMPOSITION_CONFIG_H
#define DECOMPOSITION_CONFIG_H

#include <cstddef>
#include <stdexcept>

namespace emg_rt::config {
// defining bounds on decomposition parameters (test / check sensibility later)
inline constexpr std::size_t WINDOW_SIZE_MAX = 1000;
inline constexpr std::size_t WINDOW_SIZE_MIN = 50;
inline constexpr float DECOMPOSITION_FREQUENCY_MAX = 2000.0F;
inline constexpr float DECOMPOSITION_FREQUENCY_MIN = 100.0F;

struct OnlineDecompositionConfig {
  std::size_t sampling_frequency;
  float decomposition_frequency;
  std::size_t demean_window_size;

  std::size_t samples_per_cycle;
  std::size_t min_lookback_samps;
  std::size_t min_lookahead_samps;
  std::size_t tgt_ext_channels;

  void validate() const {
    if (decomposition_frequency > DECOMPOSITION_FREQUENCY_MAX ||
        decomposition_frequency < DECOMPOSITION_FREQUENCY_MIN) {
      throw std::runtime_error(
          "decomposition_frequency out of acceptable bounds");
    }

    if (demean_window_size > WINDOW_SIZE_MAX ||
        demean_window_size < WINDOW_SIZE_MIN) {
      throw std::runtime_error("demean_window_size out of acceptable bounds");
    }
  }
};
} // namespace emg_rt::config

#endif
