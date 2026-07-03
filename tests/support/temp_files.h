#ifndef EMG_RT_TESTS_TEMP_FILES_H
#define EMG_RT_TESTS_TEMP_FILES_H

#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace emg_rt::tests {

struct TempDir {
  std::filesystem::path path;

  TempDir() {
    const auto stamp =
        std::chrono::steady_clock::now().time_since_epoch().count();
    path = std::filesystem::temp_directory_path() /
           ("emg_rt_tests_" + std::to_string(stamp));
    std::filesystem::create_directories(path);
  }

  ~TempDir() { std::filesystem::remove_all(path); }

  TempDir(const TempDir &) = delete;
  TempDir &operator=(const TempDir &) = delete;
};

inline std::filesystem::path write_float_bin(const std::filesystem::path &path,
                                             const std::vector<float> &values) {
  std::ofstream out(path, std::ios::binary);
  if (!out) {
    throw std::runtime_error("failed to open test binary file for writing");
  }

  out.write(reinterpret_cast<const char *>(values.data()),
            static_cast<std::streamsize>(values.size() * sizeof(float)));
  return path;
}

inline std::filesystem::path write_text_file(const std::filesystem::path &path,
                                             const std::string &contents) {
  std::ofstream out(path);
  if (!out) {
    throw std::runtime_error("failed to open test text file for writing");
  }

  out << contents;
  return path;
}

} // namespace emg_rt::tests

#endif // EMG_RT_TESTS_TEMP_FILES_H
