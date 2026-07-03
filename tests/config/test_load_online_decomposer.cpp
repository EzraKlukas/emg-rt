#include "emg-rt/decomposition/online_decomposer.h"

#include "support/temp_files.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <format>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string path_string(const std::filesystem::path &path) {
  return path.string();
}

std::filesystem::path write_single_grid_config(
    emg_rt::tests::TempDir &dir, const std::vector<float> &filters,
    const std::vector<float> &norms, const std::vector<float> &noise,
    const std::vector<float> &spike,
    const std::vector<std::size_t> &active_channels = {1, 3}) {
  const auto filters_path =
      emg_rt::tests::write_float_bin(dir.path / "filters.bin", filters);
  const auto norms_path =
      emg_rt::tests::write_float_bin(dir.path / "norms.bin", norms);
  const auto noise_path =
      emg_rt::tests::write_float_bin(dir.path / "noise.bin", noise);
  const auto spike_path =
      emg_rt::tests::write_float_bin(dir.path / "spike.bin", spike);

  std::string active = "[";
  for (std::size_t i = 0; i < active_channels.size(); ++i) {
    if (i != 0) {
      active += ", ";
    }
    active += std::to_string(active_channels[i]);
  }
  active += "]";

  const std::string yaml = std::format(
      "sampling_frequency: 1000\n"
      "num_extended_channels: 2\n"
      "min_lookback_ms: 3\n"
      "min_lookahead_ms: 1\n"
      "decomposition_frequency: 500\n"
      "demean_window_size: 50\n"
      "grids:\n"
      "  - grid_id: 7\n"
      "    active_channels: {}\n"
      "    samples_onset: [0]\n"
      "    mu_filters_path: '{}'\n"
      "    filter_norms_path: '{}'\n"
      "    noise_centroids_path: '{}'\n"
      "    spike_centroids_path: '{}'\n",
      active, path_string(filters_path), path_string(norms_path),
      path_string(noise_path), path_string(spike_path));

  return emg_rt::tests::write_text_file(dir.path / "config.yaml", yaml);
}

} // namespace

TEST_CASE("load_online_decomposer loads valid single-grid config") {
  emg_rt::tests::TempDir dir;
  const auto config_path =
      write_single_grid_config(dir, {1.0F, 2.0F}, {2.0F}, {0.0F}, {10.0F});

  emg_rt::MultiGridDecomposer decomposer =
      emg_rt::load_online_decomposer(path_string(config_path));

  REQUIRE(decomposer.grids().size() == 1);
  CHECK(decomposer.config().sampling_frequency == 1000);
  CHECK(decomposer.config().samples_per_cycle == 2);
  CHECK(decomposer.config().min_lookback_samps == 3);
  CHECK(decomposer.config().min_lookahead_samps == 1);
  CHECK(decomposer.grids()[0].grid_id() == 7);
  CHECK(decomposer.grids()[0].active_channels() ==
        std::vector<std::size_t>{1, 3});
  CHECK(decomposer.grids()[0].num_filters() == 1);
  CHECK(decomposer.grids()[0].ex_factor() == 1);
  CHECK(decomposer.grids()[0].inv_filter_norms_view()(0) ==
        doctest::Approx(0.5F));
  CHECK(decomposer.grids()[0].acquisition_mask().mask() ==
        std::vector<std::size_t>{1, 3});
}

TEST_CASE("load_online_decomposer rejects missing required global field") {
  emg_rt::tests::TempDir dir;
  const auto config_path = emg_rt::tests::write_text_file(
      dir.path / "missing.yaml",
      "sampling_frequency: 1000\n"
      "num_extended_channels: 2\n"
      "min_lookback_ms: 3\n"
      "decomposition_frequency: 500\n"
      "demean_window_size: 50\n"
      "grids: []\n");

  CHECK_THROWS_WITH_AS(emg_rt::load_online_decomposer(path_string(config_path)),
                       doctest::Contains("Missing min_lookahead_ms"),
                       std::runtime_error);
}

TEST_CASE("load_online_decomposer rejects missing config file") {
  CHECK_THROWS_WITH_AS(
      emg_rt::load_online_decomposer("/tmp/emg_rt_missing_config.yaml"),
      doctest::Contains("could not be opened or found"), std::runtime_error);
}

TEST_CASE("load_online_decomposer validates centroid counts") {
  emg_rt::tests::TempDir dir;
  const auto config_path =
      write_single_grid_config(dir, {1.0F, 2.0F}, {2.0F}, {0.0F, 1.0F},
                               {10.0F});

  CHECK_THROWS_WITH_AS(emg_rt::load_online_decomposer(path_string(config_path)),
                       doctest::Contains("noise_centroids size does not match"),
                       std::runtime_error);
}

TEST_CASE("load_online_decomposer validates filter shape against active channels") {
  emg_rt::tests::TempDir dir;
  const auto config_path =
      write_single_grid_config(dir, {1.0F, 2.0F, 3.0F}, {2.0F}, {0.0F},
                               {10.0F});

  CHECK_THROWS_WITH_AS(
      emg_rt::load_online_decomposer(path_string(config_path)),
      doctest::Contains("num_extended_channels 3 is not divisible"),
      std::runtime_error);
}

TEST_CASE("load_online_decomposer offsets acquisition masks for multiple grids") {
  emg_rt::tests::TempDir dir;
  const auto f0 = emg_rt::tests::write_float_bin(dir.path / "f0.bin", {1.0F});
  const auto n0 = emg_rt::tests::write_float_bin(dir.path / "n0.bin", {1.0F});
  const auto z0 = emg_rt::tests::write_float_bin(dir.path / "z0.bin", {0.0F});
  const auto s0 = emg_rt::tests::write_float_bin(dir.path / "s0.bin", {5.0F});
  const auto f1 = emg_rt::tests::write_float_bin(dir.path / "f1.bin", {2.0F});
  const auto n1 = emg_rt::tests::write_float_bin(dir.path / "n1.bin", {2.0F});
  const auto z1 = emg_rt::tests::write_float_bin(dir.path / "z1.bin", {0.0F});
  const auto s1 = emg_rt::tests::write_float_bin(dir.path / "s1.bin", {6.0F});

  const std::string yaml = std::format(
      "sampling_frequency: 1000\n"
      "num_extended_channels: 1\n"
      "min_lookback_ms: 3\n"
      "min_lookahead_ms: 1\n"
      "decomposition_frequency: 500\n"
      "demean_window_size: 50\n"
      "grids:\n"
      "  - grid_id: 0\n"
      "    active_channels: [2]\n"
      "    samples_onset: [0]\n"
      "    mu_filters_path: '{}'\n"
      "    filter_norms_path: '{}'\n"
      "    noise_centroids_path: '{}'\n"
      "    spike_centroids_path: '{}'\n"
      "  - grid_id: 1\n"
      "    active_channels: [1]\n"
      "    samples_onset: [0]\n"
      "    mu_filters_path: '{}'\n"
      "    filter_norms_path: '{}'\n"
      "    noise_centroids_path: '{}'\n"
      "    spike_centroids_path: '{}'\n",
      path_string(f0), path_string(n0), path_string(z0), path_string(s0),
      path_string(f1), path_string(n1), path_string(z1), path_string(s1));
  const auto config_path =
      emg_rt::tests::write_text_file(dir.path / "multi.yaml", yaml);

  emg_rt::MultiGridDecomposer decomposer =
      emg_rt::load_online_decomposer(path_string(config_path));

  REQUIRE(decomposer.grids().size() == 2);
  CHECK(decomposer.grids()[0].acquisition_mask().mask() ==
        std::vector<std::size_t>{2});
  CHECK(decomposer.grids()[1].acquisition_mask().mask() ==
        std::vector<std::size_t>{65});
}
