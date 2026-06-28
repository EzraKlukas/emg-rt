#ifndef ONLINE_DECOMPOSER_H
#define ONLINE_DECOMPOSER_H

#include "emg-rt/buffer/signal_ring_buffer.h"
#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/profiling/timer.h"
#include "emg-rt/utils/types.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

inline constexpr std::size_t channels_per_grid = 64;

// Grid specific
struct GridBuffers {
  emg_rt::RingMatrix<float> pulse_t;
  std::vector<float> pulse_t_maxima;
  emg_rt::RingMatrix<float> ext_signal;
  emg_rt::RingMatrix<float> signal;
  emg_rt::RingVector<uint64_t> timestamps;
  emg_rt::RingMatrix<bool> spikes;
  emg_rt::RingMatrix<bool> discharges;
  std::vector<float> sums;

  GridBuffers(std::size_t samples_per_cycle, std::size_t num_active_channels,
              std::size_t ex_factor, std::size_t num_filters,
              std::size_t demean_window_size, std::size_t min_lookback_ms)
      : pulse_t(num_filters, samples_per_cycle + min_lookback_ms),
        pulse_t_maxima(num_filters),
        ext_signal(num_active_channels * ex_factor, samples_per_cycle),
        signal(num_active_channels,
               samples_per_cycle + demean_window_size + (ex_factor - 1)),
        timestamps(samples_per_cycle + demean_window_size + (ex_factor - 1)),
        spikes(num_filters, samples_per_cycle + min_lookback_ms),
        discharges(num_filters, samples_per_cycle + min_lookback_ms),
        sums(num_active_channels * ex_factor) {}

  void advance_output_heads(std::size_t advance_dist);
};

class GridDecomposer {
public:
  GridDecomposer(std::size_t grid_id, std::vector<std::size_t> active_channels,
                 std::vector<float> mu_filters,
                 std::vector<float> noise_centroids,
                 std::vector<float> spike_centroids,
                 std::vector<float> filter_norms, std::size_t num_filters,
                 std::size_t ex_factor, std::size_t samples_per_cycle,
                 std::size_t demean_window_size, std::size_t min_lookback_ms)
      : grid_id_(grid_id), active_channels_(std::move(active_channels)),
        mu_filters_(std::move(mu_filters)),
        noise_centroids_(std::move(noise_centroids)),
        spike_centroids_(std::move(spike_centroids)),
        filter_norms_(std::move(filter_norms)), num_filters_(num_filters),
        ex_factor_(ex_factor),
        num_extended_channels_(ex_factor_ * active_channels_.size()),
        buffers_(samples_per_cycle, active_channels_.size(), ex_factor_,
                 num_filters_, demean_window_size, min_lookback_ms) {
    validate_dimensions();
  }

  void init_buffers(std::size_t start_idx, SignalRingBuffer &live_signal);
  void init_pulse_hist(OnlineDecompositionConfig &config);
  // something to initialize the mean and buffers, e.g. fill emg buffer.
  void decompose(OnlineDecompositionConfig &config);

  std::size_t grid_id() const { return grid_id_; }
  std::size_t num_filters() const { return num_filters_; }
  std::size_t ex_factor() const { return ex_factor_; }
  std::size_t num_extended_channels() const { return num_extended_channels_; }

  GridBuffers &buffers() { return buffers_; }
  const GridBuffers &buffers() const { return buffers_; }

  const std::vector<std::size_t> &active_channels() const {
    return active_channels_;
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

  emg_rt::VectorView<float> filter_norms_view() {
    return emg_rt::VectorView<float>{
        filter_norms_.data(),
        filter_norms_.size(),
    };
  }

  emg_rt::ConstVectorView<float> filter_norms_view() const {
    return emg_rt::ConstVectorView<float>{
        filter_norms_.data(),
        filter_norms_.size(),
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

    if (filter_norms_.size() != num_filters_) {
      throw std::runtime_error("filter_norms size does not match num_filters");
    }
  }

  std::size_t grid_id_;
  std::vector<std::size_t> active_channels_;

  std::vector<float> mu_filters_;
  std::vector<float> noise_centroids_;
  std::vector<float> spike_centroids_;
  std::vector<float> filter_norms_;

  std::size_t num_filters_;
  std::size_t ex_factor_;
  std::size_t num_extended_channels_;

  GridBuffers buffers_;
};

class MultiGridDecomposer {
public:
  MultiGridDecomposer(OnlineDecompositionConfig config,
                      std::vector<GridDecomposer> grids)
      : config_(config), grids_(std::move(grids)) {
    config_.validate();
  }

  void decompose() {
    for (auto &grid : grids_) {
      emg_rt::prof::ScopedTimer decomp_timer(emg_rt::prof::Section::decompose);
      grid.decompose(config_);
      grid.buffers().advance_output_heads(config_.samples_per_cycle);
    }
  }

  void init_grids(SignalRingBuffer &live_signal);
  void init_pulse_hist() {
    for (auto &grid : grids_) {
      emg_rt::prof::ScopedTimer init_timer(emg_rt::prof::Section::init_pulse_t);
      grid.init_pulse_hist(config_);
      grid.buffers().advance_output_heads(config_.samples_per_cycle);
    }
  }
  void get_samples(SignalRingBuffer &live_signal, std::size_t num_to_get);

  const OnlineDecompositionConfig &config() const { return config_; }

  std::vector<GridDecomposer> &grids() { return grids_; }
  const std::vector<GridDecomposer> &grids() const { return grids_; }

private:
  OnlineDecompositionConfig config_;
  std::vector<GridDecomposer> grids_;
};

MultiGridDecomposer load_online_decomposer(const std::string &path_to_yaml);

std::string format_online_params(const MultiGridDecomposer &decomposer);

#endif
