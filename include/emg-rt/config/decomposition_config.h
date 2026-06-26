#ifndef DECOMPOSITION_CONFIG_H
#define DECOMPOSITION_CONFIG_H

#include <cstddef>
#include <stdexcept>

// defining bounds on decomposition parameters (test / check sensibility later)
inline constexpr std::size_t WINDOW_SIZE_MAX = 1000;
inline constexpr std::size_t WINDOW_SIZE_MIN = 50;
inline constexpr float DECOMPOSITION_FREQUENCY_MAX = 2000.0F;
inline constexpr float DECOMPOSITION_FREQUENCY_MIN = 100.0F;

struct OnlineDecompositionConfig {
  float sampling_frequency;
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

#endif
