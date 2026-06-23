#include "emg-rt/decomposition/online_decomposer.h"
#include "emg-rt/buffer/signal_ring_buffer.h"
#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/decomposition/get_discharge_t.h"
#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/decomposition/is_local_max.h"
#include "emg-rt/dsp/demean.h"
#include "emg-rt/dsp/extend.h"
#include "emg-rt/utils/types.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

static constexpr std::size_t chan_per_grid = 64;
static constexpr float uV_per_count = 0.195F;
static constexpr int32_t adc_midscale = 32768;

static inline float rhd_u16_to_microvolts(uint16_t raw) {
  return static_cast<float>(static_cast<int32_t>(raw) - adc_midscale) *
         uV_per_count;
}

void GridDecomposer::init_buffers(std::size_t start_idx,
                                  SignalRingBuffer &live_signal) {
  size_t num_chan = active_channels().size();
  size_t num_samples = buffers().signal.cols;
  size_t read_idx =
      (live_signal.read_head() - buffers().signal.cols) % live_signal.size();

  for (size_t sample_idx = 0; sample_idx < num_samples; ++sample_idx) {
    if (++read_idx == live_signal.size()) {
      read_idx -= live_signal.size();
    }
    buffers().timestamps.data[sample_idx] =
        live_signal.samples()[read_idx].timestamp;
    for (size_t chan_idx = 0; chan_idx < num_chan; ++chan_idx) {
      buffers().signal[chan_idx, sample_idx] = rhd_u16_to_microvolts(
          live_signal.samples()[read_idx]
              .signal[active_channels()[chan_idx] + start_idx]);
    }
  }

  live_signal.increment_read_head();
}

void GridDecomposer::decompose(OnlineDecompositionConfig &config) {
  auto &buf = buffers();
  incremental_extend(buf.ext_signal, buf.signal, ex_factor_, buf.sums,
                     config.samples_per_cycle, config.demean_window_size);
  incremental_demean(buf.ext_signal, config.demean_window_size, buf.sums);
  get_pulse_train(buf.pulse_t, buf.ext_signal, mu_filters_view(),
                  filter_norms_view());
  is_local_max(buf.pulse_t, buf.spikes, config.min_peak_distance);
  get_distime(buf.discharges, buf.pulse_t, buf.spikes, noise_centroids_view(),
              spike_centroids_view());
}

void MultiGridDecomposer::init_grids(SignalRingBuffer &live_signals) {
  std::size_t grid_idx = 0;
  for (auto &grid : grids_) {
    grid.init_buffers(grid_idx, live_signals);
    grid_idx += chan_per_grid;
    initial_sums(grid.buffers().signal);
  }
}
