/*
 * Classify candidate pulse-train peaks as motor-unit discharges.
 *
 * Earlier pipeline stages produce:
 *
 *   - `pulse_train`: continuous pulse-train values for each motor unit
 *   - `candidate_spikes`: a boolean mask of candidate local maxima
 *
 * This function decides whether each candidate maximum is closer to that motor
 * unit's spike centroid or noise centroid. It implements the online equivalent
 * of the MATLAB decision:
 *
 *   abs(PulseT .* spikes - noise_centroid)
 *       >
 *   abs(PulseT .* spikes - spike_centroid)
 *
 * If the candidate is closer to the spike centroid than the noise centroid, it
 * is marked as a discharge.
 *
 * `samples_onset` applies a motor-unit-specific timing correction. The pulse
 * train may peak slightly after the physiological onset of the discharge, so
 * the final discharge mark is shifted backward by the corresponding onset
 * offset.
 *
 * Because local maxima require future samples to confirm, this function only
 * classifies samples that are far enough behind the newest data to have the
 * required lookahead context.
 */

#include "emg-rt/decomposition/classify_discharges.h"
#include "emg-rt/profiling/timer.h"
#include "emg-rt/utils/types.h"

#include <cassert>
#include <cstdlib>
#include <vector>

using namespace emg_rt;

void decomp::classify_discharges(RingMatrix<bool> &discharge_mask,
                                 const RingMatrix<float> &pulse_train,
                                 const RingMatrix<bool> &candidate_spikes,
                                 const VectorView<float> &noise_centroids,
                                 const VectorView<float> &spike_centroids,
                                 const std::vector<std::size_t> &samples_onset,
                                 std::size_t new_samples,
                                 std::size_t min_lookahead_samps) {
  EMG_RT_PROFILE(emg_rt::prof::Section::thresholding);

  std::size_t filters = pulse_train.rows;

  assert(discharge_mask.rows == filters);
  assert(spikes.rows == filters && spikes.cols == pulse_train.cols);
  assert(filters == noise_centroids.size() &&
         filters == spike_centroids.size());

  for (std::size_t filter = 0; filter < filters; ++filter) {
    for (std::size_t sample =
             candidate_spikes.cols - (min_lookahead_samps + new_samples);
         sample < candidate_spikes.cols - min_lookahead_samps; ++sample) {
      const float sample_value =
          pulse_train(filter, sample) *
          static_cast<float>(candidate_spikes(filter, sample));

      const float noise_dist = std::abs(sample_value - noise_centroids(filter));
      const float spike_dist = std::abs(sample_value - spike_centroids(filter));

      // - samples_onset[filter] is the offline-measured sample delay between
      // when a spike is triggered, and when physiologically for our purposes
      // it's most accurate to say the discharge 'began'.
      if (noise_dist > spike_dist) {
        discharge_mask(filter, sample - samples_onset[filter]) = true;
      }
    }
  }
}
