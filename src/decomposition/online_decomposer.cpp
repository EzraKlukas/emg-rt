/*
 * Online decomposition orchestration.
 *
 * This file implements the movement of data through the online decomposition
 * pipeline.
 *
 * Initialization:
 *
 *   - `GridDecomposer::init_buffers` copies enough recent raw samples from the
 *     acquisition ring into each grid's active-channel signal buffer.
 *
 *   - `MultiGridDecomposer::init_grids` initializes every grid and computes the
 *     initial rolling sums used for demeaning.
 *
 * Per-cycle operation:
 *
 *   - `MultiGridDecomposer::get_samples` reads new raw samples from
 *     `SignalRingBuffer` into each grid's working signal buffer.
 *
 *   - `GridDecomposer::decompose` runs the actual decomposition stages:
 *
 *       1. temporally extend the active-channel signal
 *       2. subtract rolling row-wise means
 *       3. project motor-unit filters to produce pulse trains
 *       4. find candidate local maxima
 *       5. classify candidate maxima as discharges or non-discharges
 *
 *   - `GridBuffers::advance_output_heads` advances the circular output buffers
 *     after each cycle so pulse trains, spike masks, and discharge masks remain
 *     aligned.
 *
 * `init_pulse_hist` runs the early pipeline stages before enough lookback
 * history is available to make valid discharge decisions. Once sufficient
 * pulse-train history exists, the normal `decompose` path can classify
 * discharges.
 */

#include "emg-rt/decomposition/online_decomposer.h"
#include "emg-rt/buffer/acquisition_ring_buffer.h"
#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/decomposition/classify_discharges.h"
#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/decomposition/is_local_max.h"
#include "emg-rt/dsp/demean.h"
#include "emg-rt/dsp/extend.h"
#include "emg-rt/profiling/timer.h"
#include "emg-rt/utils/types.h"

#include <cstddef>
#include <vector>

using namespace emg_rt;

void GridDecomposer::init_workspace(
    std::size_t start_idx, buffer::AcquisitionRingBuffer &acquisition_buffer) {
  size_t tot_channels = acquisition_buffer.num_channels();
  size_t num_active_channels = active_channels().size();
  size_t num_samples = workspace().raw_grid_window.cols;
  size_t read_idx =
      (acquisition_buffer.read_head() + acquisition_buffer.size() -
       workspace().raw_grid_window.cols) %
      acquisition_buffer.size();

  for (size_t sample_idx = 0; sample_idx < num_samples; ++sample_idx) {
    workspace().timestamps.data[sample_idx] =
        acquisition_buffer.timestamps()[read_idx];
    for (size_t chan_idx = 0; chan_idx < num_active_channels; ++chan_idx) {
      workspace().raw_grid_window(chan_idx, sample_idx) =
          acquisition_buffer.signals()[(read_idx * tot_channels) + start_idx +
                                       active_channels()[chan_idx]];
    }
    if (++read_idx == acquisition_buffer.size()) {
      read_idx = 0;
    }
  }
}

void GridWorkspace::advance_output_heads(std::size_t advance_dist) {
  pulse_train.head += advance_dist;
  if (pulse_train.head >= pulse_train.cols) {
    pulse_train.head -= pulse_train.cols;
  }
  candidate_spikes.head = pulse_train.head;
  discharges.head = pulse_train.head;
}

void GridDecomposer::decompose(config::OnlineDecompositionConfig &config) {
  auto &ws = workspace();
  dsp::incremental_extend(ws.extended_grid_signal, ws.raw_grid_window,
                          ex_factor_, ws.demean_sums, config.samples_per_cycle,
                          config.demean_window_size);
  dsp::incremental_demean(ws.extended_grid_signal, config.demean_window_size,
                          ws.demean_sums);
  decomp::get_pulse_train(ws.pulse_train, ws.extended_grid_signal,
                          mu_filters_view(), inv_filter_norms_view());
  decomp::incremental_is_local_max(
      ws.pulse_train, ws.candidate_spikes, config.samples_per_cycle,
      ws.pulse_train_maxima, config.min_lookahead_samps);
  decomp::classify_discharges(
      ws.discharges, ws.pulse_train, ws.candidate_spikes,
      noise_centroids_view(), spike_centroids_view(), samples_onset(),
      config.samples_per_cycle, config.min_lookahead_samps);
}

void MultiGridDecomposer::init_grids(
    buffer::AcquisitionRingBuffer &live_signal) {
  std::size_t grid_idx = 0;
  for (auto &grid : grids_) {
    grid.init_workspace(grid_idx, live_signal);
    grid_idx += channels_per_grid;
    grid.workspace().demean_sums =
        dsp::initial_sums(grid.workspace().raw_grid_window,
                          config_.demean_window_size, grid.ex_factor());
  }
}

void GridDecomposer::init_pulse_hist(
    config::OnlineDecompositionConfig &config) {
  auto &buf = workspace();
  dsp::incremental_extend(buf.extended_grid_signal, buf.raw_grid_window,
                          ex_factor_, buf.demean_sums, config.samples_per_cycle,
                          config.demean_window_size);
  dsp::incremental_demean(buf.extended_grid_signal, config.demean_window_size,
                          buf.demean_sums);
  decomp::get_pulse_train(buf.pulse_train, buf.extended_grid_signal,
                          mu_filters_view(), inv_filter_norms_view());
  decomp::incremental_is_local_max(
      buf.pulse_train, buf.candidate_spikes, config.samples_per_cycle,
      buf.pulse_train_maxima, config.min_lookahead_samps);
}

/*
 * Modifies:
 * live_signal.read_head() -> contractually the only place it's read!
 */
void MultiGridDecomposer::get_samples(
    buffer::AcquisitionRingBuffer &live_signal, size_t num_to_get) {
  EMG_RT_PROFILE(prof::Section::samp_from_ring);
  std::size_t grid_idx = 0;
  for (auto &grid : grids_) {
    for (size_t sample_idx = 0; sample_idx < num_to_get; ++sample_idx) {
      grid.workspace().timestamps.insert(
          live_signal.timestamps()[live_signal.read_head()]);
      grid.workspace().raw_grid_window.write_column(
          &live_signal.signals()[(live_signal.postfix_increment_read_head() *
                                  live_signal.num_channels()) +
                                 grid_idx]);
    }
    grid_idx += channels_per_grid;
  }
}
