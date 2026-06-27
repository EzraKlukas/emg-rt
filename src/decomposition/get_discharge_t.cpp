//******************************************************************************
// Identifies and separates spikes from non-spikes in pulse train according to
// predetermined spike and noise centroids.
// Implements matlab code:
// Distime = (abs(PulseT' .* spikes1 - noise_centroids) > abs(PulseT' .* spikes1
// - spike_centroids));
//
// Input:
// discharge_times: (M x N) boolean mask containing discharge times of each MU.
// pulse_t: pulse train (dimensions M x N)
// spikes: boolean mask (dimensions M x N) indicating maximal elements of
// pulse_t
// noise_centroids, spike_centroids: noise and spike centroids for each
// motor unit.
//
// Output: void
//******************************************************************************

#include "emg-rt/decomposition/get_discharge_t.h"
#include "emg-rt/utils/types.h"

#include <cassert>
#include <cstdlib>
#include <vector>

using namespace emg_rt;

void get_distime(RingMatrix<bool> &discharges, const RingMatrix<float> &pulse_t,
                 const RingMatrix<bool> &spikes,
                 const VectorView<float> &noise_centroids,
                 const VectorView<float> &spike_centroids,
                 std::size_t new_samples, std::size_t min_lookahead_samps) {
  std::size_t filters = pulse_t.rows;

  assert(discharges.rows == filters);
  assert(spikes.rows == filters && spikes.cols == pulse_t.cols);
  assert(filters == noise_centroids.size() &&
         filters == spike_centroids.size());

  for (std::size_t filter = 0; filter < filters; ++filter) {
    for (std::size_t sample = spikes.cols - (min_lookahead_samps + new_samples);
         sample < spikes.cols - min_lookahead_samps; ++sample) {
      const float sample_value =
          pulse_t(filter, sample) * static_cast<float>(spikes(filter, sample));

      const float noise_dist = std::abs(sample_value - noise_centroids(filter));
      const float spike_dist = std::abs(sample_value - spike_centroids(filter));

      if (noise_dist > spike_dist) {
        // discharge_times[filter].push_back(sample); // was vector of vectors
        discharges(filter, sample) = true;
      } else {
        discharges(filter, sample) = false;
      }
    }
  }
}
