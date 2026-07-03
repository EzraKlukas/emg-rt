/*
 * Online EMG decomposition state and orchestration.
 *
 * This header defines the objects that connect raw incoming EMG samples to the
 * decomposition kernels.
 *
 * There are two levels:
 *
 *   1. `GridDecomposer`
 *
 *      Owns the trained data and working buffers for one EMG grid:
 *
 *        - active channel list
 *        - motor-unit filters
 *        - spike/noise centroids
 *        - inverse filter normalization values
 *        - per-grid signal, extended-signal, pulse-train, spike, and discharge
 *          buffers
 *
 *      A grid decomposer operates only on the channels belonging to its grid.
 *
 *   2. `MultiGridDecomposer`
 *
 *      Owns the global online configuration and a collection of
 *      `GridDecomposer` objects. It is the high-level object used by `main`.
 *
 * The data path is:
 *
 *   SignalRingBuffer
 *       -> GridDecomposer::workspace().raw_grid_window
 *       -> temporal extension
 *       -> demeaning
 *       -> pulse train projection
 *       -> local-maximum detection
 *       -> discharge classification
 *
 * `GridWorkspace` holds preallocated working memory for the real-time loop. The
 * goal is to avoid allocating temporary vectors or matrices during each
 * decomposition cycle.
 *
 * Naming note:
 *
 *   `AcquisitionRingBuffer` stores raw physical-channel samples from
 * acquisition. `GridWorkspace` stores internal per-grid decomposition
 * workspace. These are intentionally different layers of the pipeline.
 */

#ifndef ONLINE_DECOMPOSER_H
#define ONLINE_DECOMPOSER_H

#include "emg-rt/buffer/acquisition_ring_buffer.h"
#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/profiling/timer.h"
#include "emg-rt/utils/types.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace emg_rt {
inline constexpr std::size_t channels_per_grid = 64;

// Grid specific
struct GridWorkspace {
  emg_rt::RingMatrix<float> pulse_train;
  std::vector<float> pulse_train_maxima;
  emg_rt::RingMatrix<float> extended_grid_signal;
  emg_rt::RingMatrix<float> raw_grid_window;
  emg_rt::RingVector<size_t> indices;
  emg_rt::RingVector<uint64_t> timestamps;
  emg_rt::RingMatrix<bool> candidate_spikes;
  emg_rt::RingMatrix<bool> discharges;
  std::vector<float> demean_sums;

  GridWorkspace(std::size_t samples_per_cycle, std::size_t num_active_channels,
                std::size_t ex_factor, std::size_t num_filters,
                std::size_t demean_window_size, std::size_t min_lookback_samps)
      : pulse_train(num_filters, samples_per_cycle + min_lookback_samps),
        pulse_train_maxima(num_filters),
        extended_grid_signal(num_active_channels * ex_factor,
                             samples_per_cycle),
        raw_grid_window(num_active_channels, samples_per_cycle +
                                                 demean_window_size +
                                                 (ex_factor - 1)),
        indices(samples_per_cycle + demean_window_size + (ex_factor - 1)),
        timestamps(samples_per_cycle + demean_window_size + (ex_factor - 1)),
        candidate_spikes(num_filters, samples_per_cycle + min_lookback_samps),
        discharges(num_filters, samples_per_cycle + min_lookback_samps),
        demean_sums(num_active_channels * ex_factor) {}

  void advance_output_heads(std::size_t advance_dist);
};

class GridDecomposer {
public:
  GridDecomposer(std::size_t grid_id,
                 const std::vector<std::size_t> &active_channels,
                 const std::vector<std::size_t> &samples_onset,
                 std::vector<float> mu_filters,
                 std::vector<float> noise_centroids,
                 std::vector<float> spike_centroids,
                 std::vector<float> inv_filter_norms, std::size_t num_filters,
                 std::size_t ex_factor, std::size_t samples_per_cycle,
                 std::size_t demean_window_size, std::size_t min_lookback_samps,
                 buffer::AcquisitionMask acquisition_mask)
      : grid_id_(grid_id), active_channels_(active_channels),
        samples_onset_(samples_onset), mu_filters_(std::move(mu_filters)),
        noise_centroids_(std::move(noise_centroids)),
        spike_centroids_(std::move(spike_centroids)),
        inv_filter_norms_(std::move(inv_filter_norms)),
        num_filters_(num_filters), ex_factor_(ex_factor),
        num_extended_channels_(ex_factor_ * active_channels_.size()),
        acquisition_mask_(std::move(acquisition_mask)),
        workspace_(samples_per_cycle, active_channels_.size(), ex_factor_,
                   num_filters_, demean_window_size, min_lookback_samps) {
    validate_dimensions();
  }

  void
  init_workspace(emg_rt::buffer::AcquisitionRingBuffer &acquisition_buffer);
  void init_pulse_hist(emg_rt::config::OnlineDecompositionConfig &config);
  // something to initialize the mean and buffers, e.g. fill emg buffer.
  void decompose(emg_rt::config::OnlineDecompositionConfig &config);

  std::size_t grid_id() const { return grid_id_; }
  std::size_t num_filters() const { return num_filters_; }
  std::size_t ex_factor() const { return ex_factor_; }
  std::size_t num_extended_channels() const { return num_extended_channels_; }

  buffer::AcquisitionMask &acquisition_mask() { return acquisition_mask_; }

  GridWorkspace &workspace() { return workspace_; }
  const GridWorkspace &workspace() const { return workspace_; }

  const std::vector<std::size_t> &active_channels() const {
    return active_channels_;
  }

  const std::vector<std::size_t> &samples_onset() const {
    return samples_onset_;
  }

  emg_rt::MatrixView<float> mu_filters_view() {
    return emg_rt::MatrixView<float>{
        mu_filters_.data(),
        num_filters_,
        num_extended_channels_,
    };
  }

  emg_rt::ConstMatrixView<float> mu_filters_view() const {
    return emg_rt::ConstMatrixView<float>{
        mu_filters_.data(),
        num_filters_,
        num_extended_channels_,
    };
  }

  emg_rt::VectorView<float> noise_centroids_view() {
    return emg_rt::VectorView<float>{
        noise_centroids_.data(),
        noise_centroids_.size(),
    };
  }

  emg_rt::ConstVectorView<float> noise_centroids_view() const {
    return emg_rt::ConstVectorView<float>{
        noise_centroids_.data(),
        noise_centroids_.size(),
    };
  }

  emg_rt::VectorView<float> spike_centroids_view() {
    return emg_rt::VectorView<float>{
        spike_centroids_.data(),
        spike_centroids_.size(),
    };
  }

  emg_rt::ConstVectorView<float> spike_centroids_view() const {
    return emg_rt::ConstVectorView<float>{
        spike_centroids_.data(),
        spike_centroids_.size(),
    };
  }

  emg_rt::VectorView<float> inv_filter_norms_view() {
    return emg_rt::VectorView<float>{
        inv_filter_norms_.data(),
        inv_filter_norms_.size(),
    };
  }

  emg_rt::ConstVectorView<float> inv_filter_norms_view() const {
    return emg_rt::ConstVectorView<float>{
        inv_filter_norms_.data(),
        inv_filter_norms_.size(),
    };
  }

private:
  void validate_dimensions() const {
    if (mu_filters_.size() != num_filters_ * num_extended_channels_) {
      throw std::runtime_error("mu_filters size does not match expected shape");
    }

    if (spike_centroids_.size() != num_filters_) {
      throw std::runtime_error(
          "spike_centroids size does not match num_filters");
    }

    if (noise_centroids_.size() != num_filters_) {
      throw std::runtime_error(
          "noise_centroids size does not match num_filters");
    }

    if (inv_filter_norms_.size() != num_filters_) {
      throw std::runtime_error("filter_norms size does not match num_filters");
    }
  }

  std::size_t grid_id_;
  std::vector<std::size_t> active_channels_;
  std::vector<std::size_t> samples_onset_;

  std::vector<float> mu_filters_;
  std::vector<float> noise_centroids_;
  std::vector<float> spike_centroids_;
  std::vector<float> inv_filter_norms_;

  std::size_t num_filters_;
  std::size_t ex_factor_;
  std::size_t num_extended_channels_;

  buffer::AcquisitionMask acquisition_mask_;
  GridWorkspace workspace_;
};

class MultiGridDecomposer {
public:
  std::size_t last_index_read = 0;

  MultiGridDecomposer(emg_rt::config::OnlineDecompositionConfig config,
                      std::vector<GridDecomposer> grids)
      : config_(config), grids_(std::move(grids)) {
    config_.validate();
    for (GridDecomposer &grid : grids) {
      grid.acquisition_mask().set_mask(grid.active_channels());
    }
  }

  void decompose() {
    for (auto &grid : grids_) {
      EMG_RT_PROFILE(emg_rt::prof::Section::decompose);
      grid.decompose(config_);
      grid.workspace().advance_output_heads(config_.samples_per_cycle);
    }
  }

  void init_grids(emg_rt::buffer::AcquisitionRingBuffer &acquisition_buffer);
  void init_pulse_hist() {
    for (auto &grid : grids_) {
      EMG_RT_PROFILE(emg_rt::prof::Section::init_pulse_train);
      grid.init_pulse_hist(config_);
      grid.workspace().advance_output_heads(config_.samples_per_cycle);
    }
  }
  void read_samples(emg_rt::buffer::AcquisitionRingBuffer &acquisition_buffer,
                    std::size_t num_to_read);

  const emg_rt::config::OnlineDecompositionConfig &config() const {
    return config_;
  }

  std::vector<GridDecomposer> &grids() { return grids_; }
  const std::vector<GridDecomposer> &grids() const { return grids_; }

private:
  config::OnlineDecompositionConfig config_;
  std::vector<GridDecomposer> grids_;
};

MultiGridDecomposer load_online_decomposer(const std::string &path_to_yaml);

std::string format_online_params(const MultiGridDecomposer &decomposer);
} // namespace emg_rt

#endif
