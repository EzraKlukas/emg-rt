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
#include "emg-rt/utils/types.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <optional>

using namespace emg_rt;

void incremental_is_local_max(const RingMatrix<float> &pulse_t,
                              RingMatrix<bool> &spikes, std::size_t new_samples,
                              std::vector<float> &maxima,
                              std::size_t min_lookahead_dist) {
  std::size_t check_spike_idx = 0;
  for (std::size_t filter = 0; filter < pulse_t.rows; ++filter) {
    for (std::size_t new_sample = pulse_t.cols - new_samples;
         new_sample < pulse_t.cols; ++new_sample) {

      if (std::abs(pulse_t(filter, new_sample)) > maxima[filter]) {
        maxima[filter] = std::abs(pulse_t(filter, new_sample));
        spikes(filter, new_sample) = true;
      } else {
        spikes(filter, new_sample) = false;
      }

      check_spike_idx = new_sample - min_lookahead_dist;
      if (spikes(filter, check_spike_idx) &&
          std::abs(pulse_t(filter, check_spike_idx)) < maxima[filter]) {
        spikes(filter, check_spike_idx) = false;
      }

      if (std::abs(pulse_t(filter, new_sample)) == maxima[filter]) {
        maxima[filter] = 0.0;
        for (std::size_t idx = new_samples; idx < pulse_t.cols; ++idx) {
          maxima[filter] =
              std::max(std::abs(pulse_t(filter, idx)), maxima[filter]);
        }
      }
    }
  }
}

void is_local_max(const RingMatrix<float> &src, RingMatrix<bool> &dst,
                  const std::size_t min_peak_distance) {
  std::size_t rows = src.rows;
  std::size_t cols = src.cols;

  assert(rows == dst.rows);
  assert(cols == dst.cols);

  std::optional<std::size_t> last_maximum_col;

  for (std::size_t row = 0; row < src.rows; row++) {
    for (std::size_t col = 1; col < src.cols - 1; col++) { // no edges
      if ((src(row, col) >= src(row, col - 1)) &&
          (src(row, col) >= src(row, col + 1))) {
        if (last_maximum_col.has_value() &&
            (col - last_maximum_col.value() <= min_peak_distance)) {
          if (src(row, last_maximum_col.value()) < src(row, col)) {
            dst(row, col) = true;
            dst(row, last_maximum_col.value()) = false;
            last_maximum_col = col;
          }
        } else {
          dst(row, col) = true;
          last_maximum_col = col;
        }
      }
    }
    last_maximum_col.reset();
  }
}
