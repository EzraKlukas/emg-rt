/*
 * Incrementally identify candidate pulse-train peaks.
 *
 * This is an online approximation of MATLAB's `islocalmax` behavior with a
 * minimum separation/lookahead distance. It operates row-wise, where each row
 * is one motor-unit pulse train.
 *
 * For each new pulse-train sample:
 *
 *   - If the new absolute value is larger than the current tracked maximum for
 *     that filter, mark it as a candidate spike.
 *
 *   - After enough lookahead samples have passed, revisit the older candidate.
 *     If a larger value appeared within the lookahead region, reject the older
 *     candidate.
 *
 *   - When the current tracked maximum reaches the newest sample, recompute the
 *     maximum over the retained history so future comparisons remain valid.
 *
 * The output `candidate_spikes` is not yet the final discharge decision. It is
 * a mask of candidate local maxima that will later be classified using each
 * motor unit's spike and noise centroids.
 */

#include "emg-rt/decomposition/is_local_max.h"
#include "emg-rt/profiling/timer.h"
#include "emg-rt/utils/types.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>

using namespace emg_rt;

void decomp::incremental_is_local_max(const RingMatrix<float> &pulse_train,
                                      RingMatrix<bool> &candidate_spikes,
                                      std::size_t new_samples,
                                      std::vector<float> &maxima,
                                      std::size_t min_lookahead_dist) {
  EMG_RT_PROFILE(emg_rt::prof::Section::is_local_max);

  std::size_t check_spike_idx = 0;
  for (std::size_t filter = 0; filter < pulse_train.rows; ++filter) {
    for (std::size_t new_sample = pulse_train.cols - new_samples;
         new_sample < pulse_train.cols; ++new_sample) {

      if (std::abs(pulse_train(filter, new_sample)) > maxima[filter]) {
        maxima[filter] = std::abs(pulse_train(filter, new_sample));
        candidate_spikes(filter, new_sample) = true;
      } else {
        candidate_spikes(filter, new_sample) = false;
      }

      check_spike_idx = new_sample - min_lookahead_dist;
      if (candidate_spikes(filter, check_spike_idx) &&
          std::abs(pulse_train(filter, check_spike_idx)) < maxima[filter]) {
        candidate_spikes(filter, check_spike_idx) = false;
      }

      if (std::abs(pulse_train(filter, new_sample)) == maxima[filter]) {
        maxima[filter] = 0.0;
        for (std::size_t idx = new_samples; idx < pulse_train.cols; ++idx) {
          maxima[filter] =
              std::max(std::abs(pulse_train(filter, idx)), maxima[filter]);
        }
      }
    }
  }
}
