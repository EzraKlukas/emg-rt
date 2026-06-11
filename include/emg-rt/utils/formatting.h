#ifndef EMG_RT_FORMATTING_H
#define EMG_RT_FORMATTING_H

#include <cstddef>
#include <format>
#include <string>
#include <string_view>
#include <vector>

/*
 * Important for debugging / profiling / viewing, not strictly necessary for
 * decomposition.
 */

namespace emg_rt {

template <typename T>
[[nodiscard]] std::string format_std_vector(const std::vector<T> &vec,
                                            std::string_view separator = " ") {
  std::string result;

  for (std::size_t i = 0; i < vec.size(); ++i) {
    result += std::format("{}", vec[i]);

    if (i + 1 < vec.size()) {
      result += separator;
    }
  }

  return result;
}

template <typename Vector>
[[nodiscard]] std::string format_vector(const Vector &view,
                                        std::string_view separator = " ") {
  std::string result;

  for (std::size_t i = 0; i < view.extent(0); ++i) {
    result += std::format("{}", view[i]);

    if (i + 1 < view.extent(0)) {
      result += separator;
    }
  }

  return result;
}

template <typename Matrix>
[[nodiscard]] std::string format_matrix(const Matrix &view,
                                        std::string_view column_separator = " ",
                                        std::string_view row_separator = "\n") {
  std::string result;

  for (std::size_t i = 0; i < view.extent(0); ++i) {
    for (std::size_t j = 0; j < view.extent(1); ++j) {
      result += std::format("{}", view[i, j]);

      if (j + 1 < view.extent(1)) {
        result += column_separator;
      }
    }

    if (i + 1 < view.extent(0)) {
      result += row_separator;
    }
  }

  return result;
}

} // namespace emg_rt

#endif // EMG_RT_FORMATTING_H
