#include "emg-rt/decomposition/online_decomposer.h"

#include "support/temp_files.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <string>
#include <vector>

namespace {

std::string base_config_yaml(const std::string &grids_yaml) {
  return "sampling_frequency: 1000\n"
         "num_extended_channels: 4\n"
         "min_lookback_ms: 0\n"
         "min_lookahead_ms: 0\n"
         "decomposition_frequency: 1000\n"
         "demean_window_size: 50\n"
         "grids:\n" +
         grids_yaml;
}

std::string quote_path(const std::filesystem::path &path) {
  return "\"" + path.string() + "\"";
}

} // namespace

TEST_CASE("load_online_decomposer parses a single-grid config") {
  emg_rt::tests::TempDir temp;
  const auto filters =
      emg_rt::tests::write_float_bin(temp.path / "filters.bin",
                                     {1.0F, 0.0F, 0.0F, 2.0F});
  const auto norms =
      emg_rt::tests::write_float_bin(temp.path / "norms.bin", {2.0F, 4.0F});
  const auto centroids = emg_rt::tests::write_float_bin(
      temp.path / "centroids.bin", {0.1F, 0.2F, 1.1F, 1.2F});

  const std::string grids_yaml =
      "  - grid_id: 7\n"
      "    active_channels: [0, 2]\n"
      "    samples_onset: [0, 1]\n"
      "    mu_filters_path: " +
      quote_path(filters) +
      "\n"
      "    filter_norms_path: " +
      quote_path(norms) +
      "\n"
      "    centroids_path: " +
      quote_path(centroids) + "\n";
  const auto yaml =
      emg_rt::tests::write_text_file(temp.path / "config.yaml",
                                     base_config_yaml(grids_yaml));

  const emg_rt::MultiGridDecomposer decomposer =
      emg_rt::load_online_decomposer(yaml.string());

  REQUIRE(decomposer.grids().size() == 1);
  const auto &config = decomposer.config();
  CHECK(config.sampling_frequency == doctest::Approx(1000.0F));
  CHECK(config.decomposition_frequency == doctest::Approx(1000.0F));
  CHECK(config.samples_per_cycle == 1);
  CHECK(config.demean_window_size == 50);
  CHECK(config.tgt_ext_channels == 4);

  const auto &grid = decomposer.grids().front();
  CHECK(grid.grid_id() == 7);
  CHECK((grid.active_channels() == std::vector<std::size_t>{0, 2}));
  CHECK((grid.samples_onset() == std::vector<std::size_t>{0, 1}));
  CHECK(grid.num_filters() == 2);
  CHECK(grid.ex_factor() == 1);
  CHECK(grid.num_extended_channels() == 2);
  CHECK(grid.inv_filter_norms_view()(0) == doctest::Approx(0.5F));
  CHECK(grid.inv_filter_norms_view()(1) == doctest::Approx(0.25F));
  CHECK(grid.noise_centroids_view()(0) == doctest::Approx(0.1F));
  CHECK(grid.noise_centroids_view()(1) == doctest::Approx(0.2F));
  CHECK(grid.spike_centroids_view()(0) == doctest::Approx(1.1F));
  CHECK(grid.spike_centroids_view()(1) == doctest::Approx(1.2F));
}

TEST_CASE("load_online_decomposer parses structurally separate grids") {
  emg_rt::tests::TempDir temp;
  const auto filters0 =
      emg_rt::tests::write_float_bin(temp.path / "filters0.bin", {1.0F});
  const auto norms0 =
      emg_rt::tests::write_float_bin(temp.path / "norms0.bin", {2.0F});
  const auto noise0 =
      emg_rt::tests::write_float_bin(temp.path / "noise0.bin", {0.0F});
  const auto spike0 =
      emg_rt::tests::write_float_bin(temp.path / "spike0.bin", {1.0F});
  const auto filters1 =
      emg_rt::tests::write_float_bin(temp.path / "filters1.bin", {3.0F});
  const auto norms1 =
      emg_rt::tests::write_float_bin(temp.path / "norms1.bin", {4.0F});
  const auto noise1 =
      emg_rt::tests::write_float_bin(temp.path / "noise1.bin", {0.2F});
  const auto spike1 =
      emg_rt::tests::write_float_bin(temp.path / "spike1.bin", {2.0F});

  const std::string grids_yaml =
      "  - grid_id: 0\n"
      "    active_channels: [0]\n"
      "    samples_onset: [0]\n"
      "    mu_filters_path: " +
      quote_path(filters0) +
      "\n"
      "    filter_norms_path: " +
      quote_path(norms0) +
      "\n"
      "    noise_centroids_path: " +
      quote_path(noise0) +
      "\n"
      "    spike_centroids_path: " +
      quote_path(spike0) +
      "\n"
      "  - grid_id: 1\n"
      "    active_channels: [1]\n"
      "    samples_onset: [0]\n"
      "    mu_filters_path: " +
      quote_path(filters1) +
      "\n"
      "    filter_norms_path: " +
      quote_path(norms1) +
      "\n"
      "    noise_centroids_path: " +
      quote_path(noise1) +
      "\n"
      "    spike_centroids_path: " +
      quote_path(spike1) + "\n";
  const auto yaml =
      emg_rt::tests::write_text_file(temp.path / "config.yaml",
                                     base_config_yaml(grids_yaml));

  const emg_rt::MultiGridDecomposer decomposer =
      emg_rt::load_online_decomposer(yaml.string());

  REQUIRE(decomposer.grids().size() == 2);
  CHECK(decomposer.grids()[0].grid_id() == 0);
  CHECK((decomposer.grids()[0].active_channels() ==
         std::vector<std::size_t>{0}));
  CHECK(decomposer.grids()[0].inv_filter_norms_view()(0) ==
        doctest::Approx(0.5F));
  CHECK(decomposer.grids()[1].grid_id() == 1);
  CHECK((decomposer.grids()[1].active_channels() ==
         std::vector<std::size_t>{1}));
  CHECK(decomposer.grids()[1].inv_filter_norms_view()(0) ==
        doctest::Approx(0.25F));
}

TEST_CASE("load_online_decomposer rejects missing required global fields") {
  emg_rt::tests::TempDir temp;
  const auto yaml = emg_rt::tests::write_text_file(
      temp.path / "config.yaml",
      "num_extended_channels: 1\n"
      "min_lookback_ms: 0\n"
      "min_lookahead_ms: 0\n"
      "decomposition_frequency: 1000\n"
      "demean_window_size: 50\n"
      "grids: []\n");

  CHECK_THROWS_WITH_AS(emg_rt::load_online_decomposer(yaml.string()),
                       doctest::Contains("Missing sampling_frequency"),
                       std::runtime_error);
}

TEST_CASE("load_online_decomposer rejects invalid per-grid dimensions") {
  emg_rt::tests::TempDir temp;
  const auto filters =
      emg_rt::tests::write_float_bin(temp.path / "filters.bin", {1.0F, 2.0F});
  const auto norms =
      emg_rt::tests::write_float_bin(temp.path / "norms.bin", {1.0F, 2.0F});
  const auto centroids =
      emg_rt::tests::write_float_bin(temp.path / "centroids.bin", {0.0F});

  const std::string grids_yaml =
      "  - grid_id: 0\n"
      "    active_channels: [0]\n"
      "    samples_onset: [0, 0]\n"
      "    mu_filters_path: " +
      quote_path(filters) +
      "\n"
      "    filter_norms_path: " +
      quote_path(norms) +
      "\n"
      "    centroids_path: " +
      quote_path(centroids) + "\n";
  const auto yaml =
      emg_rt::tests::write_text_file(temp.path / "config.yaml",
                                     base_config_yaml(grids_yaml));

  CHECK_THROWS_WITH_AS(emg_rt::load_online_decomposer(yaml.string()),
                       doctest::Contains("centroids size"),
                       std::runtime_error);
}

TEST_CASE("load_online_decomposer rejects empty active channel lists") {
  emg_rt::tests::TempDir temp;
  const auto filters =
      emg_rt::tests::write_float_bin(temp.path / "filters.bin", {1.0F});
  const auto norms =
      emg_rt::tests::write_float_bin(temp.path / "norms.bin", {1.0F});
  const auto centroids =
      emg_rt::tests::write_float_bin(temp.path / "centroids.bin",
                                     {0.0F, 1.0F});

  const std::string grids_yaml =
      "  - grid_id: 0\n"
      "    active_channels: []\n"
      "    samples_onset: [0]\n"
      "    mu_filters_path: " +
      quote_path(filters) +
      "\n"
      "    filter_norms_path: " +
      quote_path(norms) +
      "\n"
      "    centroids_path: " +
      quote_path(centroids) + "\n";
  const auto yaml =
      emg_rt::tests::write_text_file(temp.path / "config.yaml",
                                     base_config_yaml(grids_yaml));

  CHECK_THROWS_WITH_AS(emg_rt::load_online_decomposer(yaml.string()),
                       doctest::Contains("has no active channels"),
                       std::runtime_error);
}
