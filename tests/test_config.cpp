#include "emg-rt/config/decomposition_config.h"

#include <doctest/doctest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace {

void write_floats(const std::filesystem::path &path,
                  const std::vector<float> &values) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  REQUIRE(file.good());
  file.write(reinterpret_cast<const char *>(values.data()),
             static_cast<std::streamsize>(values.size() * sizeof(float)));
  REQUIRE(file.good());
}

} // namespace

TEST_CASE("get_online_params loads a small valid config") {
  const auto dir = std::filesystem::temp_directory_path() / "emg_rt_tests";
  std::filesystem::create_directories(dir);

  const auto mu_filters_path = dir / "mu_filters.bin";
  const auto centroids_path = dir / "centroids.bin";
  const auto filter_norms_path = dir / "filter_norms.bin";
  const auto config_path = dir / "small_config.yaml";

  write_floats(mu_filters_path, {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F});
  write_floats(centroids_path, {0.0F, 1.0F, 0.0F, 3.0F});
  write_floats(filter_norms_path, {2.0F, 4.0F});

  std::ofstream config(config_path, std::ios::trunc);
  REQUIRE(config.good());
  config << "sampling_frequency: 2000\n"
         << "num_extended_channels: 3\n"
         << "min_peak_dist_factor: 0.02\n"
         << "decomposition_frequency: 1000\n"
         << "window_size: 100\n"
         << "grids:\n"
         << "  - grid_id: 0\n"
         << "    active_channels: [0, 2]\n"
         << "    mu_filters_path: " << mu_filters_path.string() << "\n"
         << "    centroids_path: " << centroids_path.string() << "\n"
         << "    filter_norms_path: " << filter_norms_path.string() << "\n";
  config.close();

  const DecompositionParams params = get_online_params(config_path.string());

  CHECK(params.sampling_frequency == doctest::Approx(2000.0F));
  CHECK(params.num_extended_channels == 3);
  CHECK(params.grids.size() == 1);
  REQUIRE(params.grids[0].active_channels == std::vector<std::size_t>{0, 2});
  CHECK(params.grids[0].num_filters == 2);
  CHECK(params.grids[0].num_extended_channels == 3);
  CHECK(params.grids[0].mu_filters.size() == 6);
  CHECK(params.grids[0].filter_norms[1] == doctest::Approx(4.0F));
  CHECK_NOTHROW(params.validate_dimensions());
}
