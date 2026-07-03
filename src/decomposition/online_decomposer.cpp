/*
 * Online decomposition orchestration.
 *
 * This file implements the movement of data through the online decomposition
 * pipeline.
 *
 * Initialization:
 *
 *   - `GridDecomposer::init_workspace` copies enough recent raw samples from
 *     the acquisition ring into each grid's active-channel signal buffer using
 *     that grid's acquisition gather mask. It also initializes the grid's
 *     acquisition index and timestamp windows.
 *
 *   - `MultiGridDecomposer::init_grids` initializes every grid and computes the
 *     initial rolling sums used for demeaning.
 *
 * Per-cycle operation:
 *
 *   - `MultiGridDecomposer::read_samples` reads the next unread raw samples
 *     from `AcquisitionRingBuffer` into each grid's working signal buffer.
 *     It uses `last_index_read` rather than advancing a shared acquisition
 *     read head.
 *
 *   - `GridDecomposer::decompose` runs the actual decomposition stages:
 *
 *       1. temporally extend the active-channel signal
 *       2. subtract rolling row-wise means
 *       3. project motor-unit filters to produce pulse trains
 *       4. find candidate local maxima
 *       5. classify candidate maxima as discharges or non-discharges
 *
 *   - `GridWorkspace::advance_output_heads` advances the circular output buffers
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
    buffer::AcquisitionRingBuffer &acquisition_buffer) {
  GridWorkspace &ws = workspace();
  size_t num_samples = ws.raw_grid_window.cols;
  acquisition_buffer.read_latest_samples(num_samples, acquisition_mask(),
                                         ws.indices, ws.timestamps,
                                         ws.raw_grid_window);
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
    buffer::AcquisitionRingBuffer &acquisition_buffer) {
  for (auto &grid : grids_) {
    GridWorkspace &ws = grid.workspace();
    grid.init_workspace(acquisition_buffer);
    ws.demean_sums = dsp::initial_sums(
        ws.raw_grid_window, config_.demean_window_size, grid.ex_factor());
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
 * Reads the same acquisition-index range into every grid workspace.
 *
 * Each grid gathers its own source streams from the full acquisition sample.
 * `last_index_read` is updated once per batch and reused for all grids so their
 * workspaces stay aligned to the same acquisition indices.
 */
void MultiGridDecomposer::read_samples(
    buffer::AcquisitionRingBuffer &acquisition_buffer, size_t num_to_read) {
  EMG_RT_PROFILE(prof::Section::samp_from_ring);
  std::size_t old_last_index_read = last_index_read;
  for (auto &grid : grids_) {
    last_index_read = acquisition_buffer.read_samples(
        num_to_read, grid.acquisition_mask(), old_last_index_read,
        grid.workspace().indices, grid.workspace().timestamps,
        grid.workspace().raw_grid_window);
  }
}
