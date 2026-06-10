#include "emg-rt/config/decomposition_config.h"

#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <yaml-cpp/yaml.h>

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
    throw std::runtime_error(std::format(
        "Binary file size is not divisible by sizeof(float): {}", bin_path));
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

void get_online_params(DecompositionParams &online_params,
                       const std::string &path_to_yaml) {
  try {
    YAML::Node config = YAML::LoadFile(path_to_yaml);

    if (!config["server"]) {
      throw std::runtime_error(
          "Missing critical 'server' root node in configuration.\n");
    }
    std::string host = config["server"]["host"].as<std::string>();
    int port = config["server"]["port"].as<int>();

    std::cout << "Host: " << host << "\n";
    std::cout << "Port: " << port << "\n";

    // relevant scraping here
    online_params.sampling_frequency = config["sampling_frequency"].as<float>();
    online_params.num_extended_channels =
        config["num_extended_channels"].as<uint_fast16_t>();
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

      online_params.grids.push_back(grid);
    }
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
