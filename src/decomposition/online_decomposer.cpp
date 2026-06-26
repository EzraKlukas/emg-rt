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
#include <vector>

void GridDecomposer::init_buffers(std::size_t start_idx,
                                  SignalRingBuffer &live_signal) {
  size_t tot_channels = live_signal.num_channels();
  size_t num_active_channels = active_channels().size();
  size_t num_samples = buffers().signal.cols;
  size_t read_idx =
      (live_signal.read_head() + live_signal.size() - buffers().signal.cols) %
      live_signal.size();

  for (size_t sample_idx = 0; sample_idx < num_samples; ++sample_idx) {
    buffers().timestamps.data[sample_idx] = live_signal.timestamps()[read_idx];
    for (size_t chan_idx = 0; chan_idx < num_active_channels; ++chan_idx) {
      buffers().signal[chan_idx, sample_idx] =
          live_signal.signals()[(read_idx * tot_channels) + start_idx +
                                active_channels()[chan_idx]];
    }
    if (++read_idx == live_signal.size()) {
      read_idx = 0;
    }
  }
}

void GridBuffers::advance_output_heads(std::size_t advance_dist) {
  pulse_t.head += advance_dist;
  if (pulse_t.head >= pulse_t.cols) {
    pulse_t.head -= pulse_t.cols;
  }
  spikes.head = pulse_t.head;
  discharges.head = pulse_t.head;
}

void GridDecomposer::decompose(OnlineDecompositionConfig &config) {
  auto &buf = buffers();
  incremental_extend(buf.ext_signal, buf.signal, ex_factor_, buf.sums,
                     config.samples_per_cycle, config.demean_window_size);
  incremental_demean(buf.ext_signal, config.demean_window_size, buf.sums);
  get_pulse_train(buf.pulse_t, buf.ext_signal, mu_filters_view(),
                  filter_norms_view());
  incremental_is_local_max(buf.pulse_t, buf.spikes, config.samples_per_cycle,
                           buf.pulse_t_maxima, config.min_lookahead_samps);
  get_distime(buf.discharges, buf.pulse_t, buf.spikes, noise_centroids_view(),
              spike_centroids_view(), config.samples_per_cycle,
              config.min_lookahead_samps);
}

void MultiGridDecomposer::init_grids(SignalRingBuffer &live_signal) {
  std::size_t grid_idx = 0;
  for (auto &grid : grids_) {
    grid.init_buffers(grid_idx, live_signal);
    grid_idx += channels_per_grid;
    grid.buffers().sums = initial_sums(
        grid.buffers().signal, config_.demean_window_size, grid.ex_factor());
  }
}

void GridDecomposer::init_pulse_hist(OnlineDecompositionConfig &config) {
  auto &buf = buffers();
  incremental_extend(buf.ext_signal, buf.signal, ex_factor_, buf.sums,
                     config.samples_per_cycle, config.demean_window_size);
  incremental_demean(buf.ext_signal, config.demean_window_size, buf.sums);
  get_pulse_train(buf.pulse_t, buf.ext_signal, mu_filters_view(),
                  filter_norms_view());
  incremental_is_local_max(buf.pulse_t, buf.spikes, config.samples_per_cycle,
                           buf.pulse_t_maxima, config.min_lookahead_samps);
}

/*
 * Modifies:
 * live_signal.read_head() -> contractually the only place it's read!
 */
void MultiGridDecomposer::get_samples(SignalRingBuffer &live_signal,
                                      size_t num_to_get) {
  std::size_t grid_idx = 0;
  for (auto &grid : grids_) {
    for (size_t sample_idx = 0; sample_idx < num_to_get; ++sample_idx) {
      grid.buffers().timestamps.insert(
          live_signal.timestamps()[live_signal.read_head()]);
      grid.buffers().signal.write_column(
          &live_signal.signals()[(live_signal.postfix_increment_read_head() *
                                  live_signal.num_channels()) +
                                 grid_idx]);
    }
  }
}
