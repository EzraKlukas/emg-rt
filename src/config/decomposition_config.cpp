/*
 * Load online decomposition parameters and trained grid data from YAML.
 *
 * This file bridges the offline training/export pipeline and the C++ online
 * runtime. The YAML file describes the global online timing parameters and an
 * arbitrary number of EMG grids. For each grid, the loader reads metadata from
 * YAML and denser numeric arrays from binary files.
 *
 * Loaded per-grid data includes:
 *
 *   - active physical channels
 *   - motor-unit filters
 *   - spike/noise centroids
 *   - filter normalization values
 *   - sample-onset offsets
 *   - acquisition gather masks for each grid's active source streams
 *
 * The loader performs shape checks while constructing each `GridDecomposer`.
 * These checks are important because most decomposition data is stored as flat
 * binary arrays. Without explicit validation, a mismatch between the YAML
 * metadata and the binary files could silently produce incorrect matrix views.
 *
 * Active channels in YAML are grid-local. The loader offsets them into global
 * source stream indices and stores them in each grid's `AcquisitionMask`, which
 * is later used to gather from the full EMG acquisition sample into that
 * grid's dense workspace.
 *
 * The returned `MultiGridDecomposer` owns all loaded configuration, filters,
 * centroids, normalization values, acquisition masks, and per-grid decomposer
 * objects needed by the online loop.
 */

#include "emg-rt/buffer/acquisition_ring_buffer.h"
#include "emg-rt/decomposition/online_decomposer.h"
#include "emg-rt/utils/formatting.h"

#include <cmath>
#include <cstddef>
#include <format>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

#define MS_PER_S 1000

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

static std::string format_size_vector(const std::vector<std::size_t> &values) {
  std::ostringstream out;
  out << "[";

  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i > 0) {
      out << ", ";
    }

    out << values[i];
  }

  out << "]";
  return out.str();
}

static config::OnlineDecompositionConfig
parse_online_config(const YAML::Node &config) {
  if (!config["sampling_frequency"]) {
    throw std::runtime_error("Missing sampling_frequency");
  }

  if (!config["num_extended_channels"]) {
    throw std::runtime_error("Missing num_extended_channels");
  }

  if (!config["min_lookback_ms"]) {
    throw std::runtime_error("Missing min_lookback_ms");
  }

  if (!config["min_lookahead_ms"]) {
    throw std::runtime_error("Missing min_lookahead_ms");
  }

  if (!config["decomposition_frequency"]) {
    throw std::runtime_error("Missing decomposition_frequency");
  }

  if (!config["demean_window_size"]) {
    throw std::runtime_error("Missing demean_window_size");
  }

  config::OnlineDecompositionConfig online_config;

  online_config.sampling_frequency =
      config["sampling_frequency"].as<std::size_t>();
  online_config.decomposition_frequency =
      config["decomposition_frequency"].as<float>();
  online_config.demean_window_size =
      config["demean_window_size"].as<std::size_t>();
  online_config.tgt_ext_channels =
      config["num_extended_channels"].as<std::size_t>();

  online_config.samples_per_cycle = static_cast<std::size_t>(
      std::round((float)online_config.sampling_frequency /
                 online_config.decomposition_frequency));

  online_config.min_lookback_samps = static_cast<std::size_t>(
      std::round(config["min_lookback_ms"].as<float>() *
                 (float)online_config.sampling_frequency) /
      MS_PER_S);

  online_config.min_lookahead_samps = static_cast<std::size_t>(
      std::round(config["min_lookahead_ms"].as<float>() *
                 (float)online_config.sampling_frequency) /
      MS_PER_S);

  online_config.validate();
  return online_config;
}

static void split_centroids(const std::vector<float> &centroids,
                            std::size_t num_filters,
                            std::vector<float> &noise_centroids,
                            std::vector<float> &spike_centroids) {
  if (centroids.size() != 2 * num_filters) {
    throw std::runtime_error(std::format(
        "centroids size {} does not match expected size 2 * num_filters = {}",
        centroids.size(), 2 * num_filters));
  }

  noise_centroids.assign(centroids.begin(),
                         centroids.begin() + (long)num_filters);
  spike_centroids.assign(centroids.begin() + (long)num_filters,
                         centroids.end());
}

emg_rt::MultiGridDecomposer
emg_rt::load_online_decomposer(const std::string &path_to_yaml) {
  try {
    YAML::Node config = YAML::LoadFile(path_to_yaml);

    if (!config["grids"]) {
      throw std::runtime_error("Missing grids");
    }

    const config::OnlineDecompositionConfig online_config =
        parse_online_config(config);

    YAML::Node grids_node = config["grids"];

    if (!grids_node || !grids_node.IsSequence()) {
      throw std::runtime_error("'grids' must be a YAML list.");
    }

    std::vector<emg_rt::GridDecomposer> grids;
    grids.reserve(grids_node.size());

    std::size_t grid_idx = 0;

    for (const YAML::Node &grid_node : grids_node) {
      const std::size_t grid_id = grid_node["grid_id"].as<std::size_t>();

      std::vector<std::size_t> active_channels =
          grid_node["active_channels"].as<std::vector<std::size_t>>();
      std::vector<std::size_t> samples_onset =
          grid_node["samples_onset"].as<std::vector<std::size_t>>();

      const std::string mu_filters_path =
          grid_node["mu_filters_path"].as<std::string>();
      const std::string filter_norms_path =
          grid_node["filter_norms_path"].as<std::string>();

      std::vector<float> mu_filters = get_vector_from_bin(mu_filters_path);
      std::vector<float> filter_norms = get_vector_from_bin(filter_norms_path);
      std::vector<float> inv_filter_norms(filter_norms.size());

      // invert filter_norms, will rename inv_norms on host
      for (std::size_t i = 0; i < filter_norms.size(); ++i) {
        inv_filter_norms[i] = 1.0F / filter_norms[i];
      }

      const std::size_t num_filters = inv_filter_norms.size();

      if (num_filters == 0) {
        throw std::runtime_error(
            std::format("Grid {} has zero filters", grid_id));
      }

      if (mu_filters.size() % num_filters != 0) {
        throw std::runtime_error(std::format(
            "Grid {} mu_filters size {} is not divisible by num_filters {}",
            grid_id, mu_filters.size(), num_filters));
      }

      const std::size_t num_extended_channels = mu_filters.size() / num_filters;

      if (active_channels.empty()) {
        throw std::runtime_error(
            std::format("Grid {} has no active channels", grid_id));
      }

      if (num_extended_channels % active_channels.size() != 0) {
        throw std::runtime_error(std::format(
            "Grid {} num_extended_channels {} is not divisible by "
            "active_channels size {}",
            grid_id, num_extended_channels, active_channels.size()));
      }

      const std::size_t ex_factor =
          num_extended_channels / active_channels.size();

      std::vector<float> noise_centroids;
      std::vector<float> spike_centroids;

      if (grid_node["noise_centroids_path"] &&
          grid_node["spike_centroids_path"]) {
        noise_centroids = get_vector_from_bin(
            grid_node["noise_centroids_path"].as<std::string>());
        spike_centroids = get_vector_from_bin(
            grid_node["spike_centroids_path"].as<std::string>());
      } else if (grid_node["centroids_path"]) {
        const std::vector<float> centroids =
            get_vector_from_bin(grid_node["centroids_path"].as<std::string>());
        split_centroids(centroids, num_filters, noise_centroids,
                        spike_centroids);
      } else {
        throw std::runtime_error(
            std::format("Grid {} is missing either "
                        "noise_centroids_path/spike_centroids_path "
                        "or centroids_path",
                        grid_id));
      }

      buffer::AcquisitionMask acquisition_mask = buffer::AcquisitionMask(
          buffer::SensorType::EMG, active_channels.size());

      std::vector<std::size_t> global_active_channels = active_channels;
      for (std::size_t i = 0; i < active_channels.size(); ++i) {
        global_active_channels[i] += grid_idx;
      }

      acquisition_mask.set_mask(global_active_channels);

      grids.emplace_back(
          grid_id, std::move(active_channels), std::move(samples_onset),
          std::move(mu_filters), std::move(noise_centroids),
          std::move(spike_centroids), std::move(inv_filter_norms), num_filters,
          ex_factor, online_config.samples_per_cycle,
          online_config.demean_window_size, online_config.min_lookback_samps,
          std::move(acquisition_mask));

      grid_idx += channels_per_grid;
    }

    return emg_rt::MultiGridDecomposer{online_config, std::move(grids)};

  } catch (const YAML::BadFile &) {
    throw std::runtime_error(std::format(
        "Configuration file '{}' could not be opened or found.", path_to_yaml));

  } catch (const YAML::Exception &e) {
    throw std::runtime_error("YAML structure validation failed: " +
                             std::string(e.what()));
  }
}

std::string
emg_rt::format_online_params(const emg_rt::MultiGridDecomposer &decomposer) {
  std::string formatted_params;

  const config::OnlineDecompositionConfig &config = decomposer.config();

  formatted_params +=
      std::format("Sampling frequency: {}\n", config.sampling_frequency);
  formatted_params +=
      std::format("Target extended channels: {}\n", config.tgt_ext_channels);
  formatted_params += std::format("Decomposition frequency: {}\n",
                                  config.decomposition_frequency);
  formatted_params +=
      std::format("Demean window size: {}\n", config.demean_window_size);
  formatted_params +=
      std::format("Samples per cycle: {}\n", config.samples_per_cycle);
  formatted_params +=
      std::format("Min lookback distance: {}\n\n", config.min_lookback_samps);
  formatted_params +=
      std::format("Min lookahead distance: {}\n\n", config.min_lookahead_samps);

  for (const auto &grid : decomposer.grids()) {
    formatted_params += std::format("grid_id: {}\n", grid.grid_id());
    formatted_params += std::format("active_channels: {}\n",
                                    format_size_vector(grid.active_channels()));
    formatted_params +=
        std::format("Number of motor units: {}\n", grid.num_filters());
    formatted_params += std::format("Extension factor: {}\n", grid.ex_factor());
    formatted_params +=
        std::format("Extended channels: {}\n", grid.num_extended_channels());

    formatted_params += std::format("Noise centroids: {}\n",
                                    format_vector(grid.noise_centroids_view()));
    formatted_params += std::format("Spike centroids: {}\n",
                                    format_vector(grid.spike_centroids_view()));
    formatted_params += std::format(
        "Filter norms: {}\n", format_vector(grid.inv_filter_norms_view()));
    // formatted_params += std::format("Motor Unit Filters: {}\n",
    //                                format_matrix(grid.mu_filters_view()));
  }

  return formatted_params;
}
