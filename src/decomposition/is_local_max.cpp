//******************************************************************************
// islocalmax matlab implementation in C++, with min separation.
//
// Input:
// src: the input array (e.g. row-wise collection of pulse trains).
// dst: the binary mask of satisfactory local max in src.
// min_peak_distance: the minimal required distance between maxima.
//
// Output: void
//******************************************************************************

#include "emg-rt/decomposition/is_local_max.h"

#include <cassert>
#include <mdspan>
#include <optional>

void islocalmax(const std::mdspan<float, std::dextents<std::size_t, 2>> src,
                std::mdspan<bool, std::dextents<std::size_t, 2>> dst,
                const std::size_t min_peak_distance = 1) {
  std::size_t rows = src.extent(0);
  std::size_t cols = src.extent(1);

  assert(rows == dst.extent(0));
  assert(cols == dst.extent(1));

  std::optional<std::size_t> last_maximum_col;

  for (std::size_t row = 0; row < src.extent(0); row++) {
    for (std::size_t col = 1; col < src.extent(1) - 1; col++) { // no edges
      if ((src[row, col] >= src[row, col - 1]) &&
          (src[row, col] >= src[row, col + 1])) {
        if (last_maximum_col.has_value() &&
            (col - last_maximum_col.value() <= min_peak_distance)) {
          if (src[row, last_maximum_col.value()] < src[row, col]) {
            dst[row, col] = true;
            dst[row, last_maximum_col.value()] = false;
            last_maximum_col = col;
          }
        } else {
          dst[row, col] = true;
          last_maximum_col = col;
        }
      }
    }
    last_maximum_col.reset();
  }
}
