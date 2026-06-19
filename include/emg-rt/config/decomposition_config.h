#ifndef DECOMPOSITION_CONFIG_H
#define DECOMPOSITION_CONFIG_H

#include "emg-rt/utils/types.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// defining bounds on decomposition parameters (test / check sensibility later)
#define WINDOW_SIZE_MAX 1000
#define WINDOW_SIZE_MIN 50
#define DECOMPOSITION_FREQUENCY_MAX 2000
#define DECOMPOSITION_FREQUENCY_MIN 100 // not sure what should go into this?

struct GridDecompositionParams {
  uint8_t grid_id;
  std::vector<std::size_t> active_channels;

  std::vector<float> mu_filters;
  std::vector<float> centroids;
  std::vector<float> filter_norms;

  std::size_t num_filters;
  std::size_t num_extended_channels;

  emg_rt::MatrixView<float> mu_filters_view() {
    return emg_rt::MatrixView<float>{
        mu_filters.data(),
        num_filters,
        num_extended_channels,
    };
  }

  emg_rt::ConstMatrixView<float> mu_filters_view() const {
    return emg_rt::ConstMatrixView<float>{
        mu_filters.data(),
        num_filters,
        num_extended_channels,
    };
  }

  emg_rt::MatrixView<float> centroids_view() {
    return emg_rt::MatrixView<float>{
        centroids.data(),
        num_filters,
        2,
    };
  }

  emg_rt::ConstMatrixView<float> centroids_view() const {
    return emg_rt::ConstMatrixView<float>{
        centroids.data(),
        2,
        num_filters,
    };
  }

  emg_rt::VectorView<float> filter_norms_view() {
    return emg_rt::VectorView<float>{
        filter_norms.data(),
        filter_norms.size(),
    };
  }

  emg_rt::ConstVectorView<float> filter_norms_view() const {
    return emg_rt::ConstVectorView<float>{
        filter_norms.data(),
        filter_norms.size(),
    };
  }
};

struct DecompositionParams {
  float sampling_frequency;
  std::size_t num_extended_channels;
  float min_peak_dist_factor;
  float decomposition_frequency;
  std::size_t window_size;

  std::vector<GridDecompositionParams> grids;

  void validate_dimensions() const {
    for (GridDecompositionParams grid : grids) {
      if (grid.mu_filters.size() !=
          grid.num_filters * grid.num_extended_channels) {
        throw std::runtime_error(std::format(
            "mu_filters size, {} does not match expected shape, {} by {}",
            grid.mu_filters.size(), grid.num_filters,
            grid.num_extended_channels));
      }

      if (grid.centroids.size() != grid.num_filters * 2) {
        throw std::runtime_error(
            "centroids size does not match expected shape");
      }

      if (grid.filter_norms.size() != grid.num_filters) {
        throw std::runtime_error(
            "filter_norms size does not match num_filters");
      }
    }
    if (decomposition_frequency > DECOMPOSITION_FREQUENCY_MAX ||
        decomposition_frequency < DECOMPOSITION_FREQUENCY_MIN) {
      throw std::runtime_error(std::format(
          "decomposition_frequency out of acceptable bounds, must be between "
          "{} and {} Hz.",
          DECOMPOSITION_FREQUENCY_MIN, DECOMPOSITION_FREQUENCY_MAX));
    }

    if (window_size > WINDOW_SIZE_MAX || window_size < WINDOW_SIZE_MIN) {
      throw std::runtime_error(
          std::format("window_size out of acceptable bounds, must be "
                      "between {} and {} samples.",
                      WINDOW_SIZE_MIN, WINDOW_SIZE_MAX));
    }
  }
};

DecompositionParams get_online_params(const std::string &path_to_yaml);

std::string format_online_params(const DecompositionParams &decomp_params);

#endif
