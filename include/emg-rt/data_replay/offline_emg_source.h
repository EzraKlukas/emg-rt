#ifndef OFFLINE_EMG_SOURCE_H
#define OFFLINE_EMG_SOURCE_H

#include <format>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

template <typename T>
std::vector<T> pull_samples_from_bin(const std::string &bin_path) {
  static_assert(std::is_trivially_copyable_v<T>,
                "get_vector_from_bin<T> requires T to be trivially copyable");

  std::ifstream file(bin_path, std::ios::binary | std::ios::ate);

  if (!file) {
    throw std::runtime_error(
        std::format("Failed to open binary data file at {}", bin_path));
  }

  const std::ifstream::pos_type end_pos = file.tellg();

  if (end_pos < 0) {
    throw std::runtime_error(
        std::format("Failed to determine size of binary file at {}", bin_path));
  }

  const auto size = static_cast<size_t>(end_pos);
  const size_t element_size = sizeof(T);

  if (size % element_size != 0) {
    throw std::runtime_error(
        std::format("Binary file size {} is not divisible by sizeof(T) {}. "
                    "Remainder: {}. Path: {}",
                    size, element_size, size % element_size, bin_path));
  }

  if (size > static_cast<size_t>(std::numeric_limits<std::streamsize>::max())) {
    throw std::runtime_error(std::format(
        "Binary file too large to read in one operation. Size: {}. Path: {}",
        size, bin_path));
  }

  const size_t num_elements = size / element_size;

  std::vector<T> data(num_elements);

  file.seekg(0, std::ios::beg);

  if (size > 0 && !file.read(reinterpret_cast<char *>(data.data()),
                             static_cast<std::streamsize>(size))) {
    throw std::runtime_error(
        std::format("Error reading binary file at {}", bin_path));
  }

  return data;
}

std::vector<uint64_t> generate_timestamps(std::size_t size);

#endif
