// *****************************************************************************
// Pulls decomposition parameters from .yaml configuration file, including an
// arbitrary number of grids, pulling denser data from .bin files referenced in
// .yaml
// *****************************************************************************
#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/utils/formatting.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <yaml-cpp/yaml.h>

using namespace emg_rt;

static std::vector<float> get_vector_from_bin(const std::string &bin_path) {
  std::ifstream file(bin_path, std::ios::binary | std::ios::ate);

  if (!file) {
    throw std::runtime_error(std::format(
        "Failed to open binary decomposition data file at {}", bin_path));
  }

  const std::ifstream::pos_type end_pos = file.tellg();

  if (end_pos < 0) {
    throw std::runtime_error(
        std::format("Failed to determine size of binary file at {}", bin_path));
  }

  const auto size = static_cast<std::size_t>(end_pos);
  const std::size_t element_size = sizeof(float);

  if (size % element_size != 0) {
    throw std::runtime_error(
        std::format("Binary file size {} is not divisible by sizeof(float) {}. "
                    "Remainder: {}. Path: {}",
                    size, element_size, size % element_size, bin_path));
  }

  const std::size_t num_elements = size / element_size;

  std::vector<float> data(num_elements);

  file.seekg(0, std::ios::beg);

  if (!file.read(reinterpret_cast<char *>(data.data()),
                 static_cast<std::streamsize>(size))) {
    throw std::runtime_error(
        std::format("Error reading binary file at {}", bin_path));
  }

  return data;
}

// Purpose: extracts and returns all relevant online decomposition parameters
// from given .yaml path.
//
// inputs:
// path_to_yaml: the path to the .yaml file containing config parameters
//
// outputs:
// DecompositionParams: struct containing all relevant online decomposition
// parameters.
DecompositionParams get_online_params(const std::string &path_to_yaml) {
  try {
    YAML::Node config = YAML::LoadFile(path_to_yaml);

    if (!config["sampling_frequency"]) {
      throw std::runtime_error("Missing sampling_frequency");
    }

    if (!config["number_extended_channels"]) {
      throw std::runtime_error("Missing number_extended_channels");
    }

    if (!config["min_peak_dist_factor"]) {
      throw std::runtime_error("Missing min_peak_dist_factor");
    }

    if (!config["grids"]) {
      throw std::runtime_error("Missing grids");
    }

    DecompositionParams online_params;

    online_params.sampling_frequency = config["sampling_frequency"].as<float>();
    online_params.number_extended_channels =
        config["number_extended_channels"].as<uint_fast16_t>();
    online_params.min_peak_dist_factor =
        config["min_peak_dist_factor"].as<float>();

    YAML::Node grids = config["grids"];

    if (!grids || !grids.IsSequence()) {
      throw std::runtime_error("'grids' must be a YAML list.\n");
    }

    for (const YAML::Node &grid_node : grids) {
      GridDecompositionParams grid;

      grid.grid_id = grid_node["grid_id"].as<uint8_t>();

      grid.active_channels =
          grid_node["active_channels"].as<std::vector<std::size_t>>();

      std::string mu_filters_path =
          grid_node["mu_filters_path"].as<std::string>();
      std::string centroids_path =
          grid_node["centroids_path"].as<std::string>();
      std::string filter_norms_path =
          grid_node["filter_norms_path"].as<std::string>();

      // here call function that scrapes relevant binary
      grid.mu_filters = get_vector_from_bin(mu_filters_path);
      grid.centroids = get_vector_from_bin(centroids_path);
      grid.filter_norms = get_vector_from_bin(filter_norms_path);

      grid.num_filters = grid.filter_norms.size();
      grid.num_extended_channels = grid.mu_filters.size() / grid.num_filters;

      online_params.grids.push_back(grid);
    }
    return online_params;
  } catch (const YAML::BadFile &e) {
    // Intercept file-not-found issues and convert to a standard runtime error
    throw std::runtime_error(
        "Configuration file 'config.yaml' could not be opened or found.\n");

  } catch (const YAML::Exception &e) {
    // Intercept native parsing/casting failures and forward internal message
    throw std::runtime_error(
        "YAML structure validation failed: " + std::string(e.what()) + "\n");
  }
}

std::string format_online_params(const DecompositionParams &decomp_params) {
  std::string formatted_params;

  formatted_params +=
      std::format("Sampling frequency: {}\n", decomp_params.sampling_frequency);
  formatted_params += std::format("number_extended_channels: {}\n",
                                  decomp_params.number_extended_channels);
  formatted_params += std::format("min_peak_dist_factor: {}\n\n",
                                  decomp_params.min_peak_dist_factor);

  for (const auto &grid : decomp_params.grids) {
    formatted_params += std::format("grid_id: {}\n", grid.grid_id);
    formatted_params +=
        std::format("active_channels: {}\n", grid.active_channels);
    std::size_t num_filters = grid.filter_norms.size();
    formatted_params += std::format("Number of motor units: {}\n", num_filters);
    formatted_params += std::format("Noise centroids: {}\n",
                                    format_matrix(grid.centroids_view()));
    formatted_params += std::format("Filter norms: {}\n",
                                    format_vector(grid.filter_norms_view()));
    // formatted_params += std::format("Motor Unit Filters: {}\n",
    //                               format_matrix(grid.mu_filters_view()));
  }

  return formatted_params;
}
