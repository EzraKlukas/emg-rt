#ifndef DECOMPOSITION_CONFIG_H
#define DECOMPOSITION_CONFIG_H

#include "emg-rt/utils/types.h"

#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

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

  void validate_dimensions() const {
    if (mu_filters.size() != num_filters * num_extended_channels) {
      throw std::runtime_error(std::format(
          "mu_filters size, {} does not match expected shape, {} by {}",
          mu_filters.size(), num_filters, num_extended_channels));
    }

    if (centroids.size() != num_filters * 2) {
      throw std::runtime_error("centroids size does not match expected shape");
    }

    if (filter_norms.size() != num_filters) {
      throw std::runtime_error("filter_norms size does not match num_filters");
    }
  }
};

struct DecompositionParams {
  float sampling_frequency;
  std::uint_fast16_t number_extended_channels;
  float min_peak_dist_factor;

  std::vector<GridDecompositionParams> grids;
};

DecompositionParams get_online_params(const std::string &path_to_yaml);

std::string format_online_params(const DecompositionParams &decomp_params);

#endif
