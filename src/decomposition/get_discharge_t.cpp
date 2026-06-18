//******************************************************************************
// Identifies and separates spikes from non-spikes in pulse train according to
// predetermined spike and noise centroids.
// Implements matlab code:
// Distime = (abs(PulseT' .* spikes1 - noise_centroids) > abs(PulseT' .* spikes1
// - spike_centroids));
//
// Input:
// discharge_times: M vectors containing discharge times of each motor unit.
// pulse_t: pulse train (dimensions M x N)
// spikes: boolean mask (dimensions M x N) indicating maximal elements of
// pulse_t
// noise_centroids, spike_centroids: noise and spike centroids for each
// motor unit.
//
// Output: void
//******************************************************************************

#include "emg-rt/decomposition/get_discharge_t.h"
#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/utils/types.h"

#include <cassert>
#include <cstdlib>
#include <mdspan>
#include <vector>

using namespace emg_rt;

void get_distime(RaggedArray<std::size_t> &discharge_times,
                 MatrixView<float> pulse_t, MatrixView<float> spikes,
                 ConstVectorView<float> &noise_centroids,
                 ConstVectorView<float> &spike_centroids) {
  std::size_t filters = pulse_t.extent(0);
  std::size_t samples = pulse_t.extent(1);

  assert(discharge_times.offsets.size() - 1 == filters);
  assert(spikes.extent(0) == filters && spikes.extent(1) == samples);
  assert(filters == noise_centroids.size() &&
         filters == spike_centroids.size());

  discharge_times.offsets[0] = 0; // Can I assume and preserve this?

  for (std::size_t filter = 0; filter < filters; ++filter) {
    for (std::size_t sample = 0; sample < samples; ++sample) {
      const float sample_value =
          pulse_t[filter, sample] * static_cast<float>(spikes[filter, sample]);

      const float noise_dist = std::abs(sample_value - noise_centroids[filter]);
      const float spike_dist = std::abs(sample_value - spike_centroids[filter]);

      if (noise_dist > spike_dist) {
        discharge_times.data.push_back(sample);
      }
    }
    discharge_times.offsets[filter + 1] =
        discharge_times.data.size(); // next filter starts at data[data.size()]
  }
}
