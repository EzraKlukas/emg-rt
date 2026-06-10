#ifndef DECOMPOSITION_CONFIG_H
#define DECOMPOSITION_CONFIG_H

#include <cstdint>
#include <string>
#include <vector>

struct GridDecompositionParams {
  uint8_t grid_id;
  std::vector<std::size_t> active_channels;

  std::vector<float> mu_filters;
  std::vector<float> centroids;
  std::vector<float> filter_norms;
};

struct DecompositionParams {
  float sampling_frequency;
  std::uint_fast16_t num_extended_channels;
  float min_peak_dist_factor;

  std::vector<GridDecompositionParams> grids;
};

void get_online_params(DecompositionParams online_params,
                       std::string path_to_yaml);

#endif
