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

#include <cassert>
#include <cstdlib>
#include <mdspan>
#include <vector>

void get_distime(
    std::vector<std::vector<std::size_t>> discharge_times,
    const std::mdspan<float, std::dextents<std::size_t, 2>> pulse_t,
    const std::mdspan<bool, std::dextents<std::size_t, 2>> spikes,
    const std::vector<float> &noise_centroids,
    const std::vector<float> &spike_centroids) {
  std::size_t filters = pulse_t.extent(0);
  std::size_t samples = pulse_t.extent(1);

  assert(discharge_times.size() == filters);
  assert(spikes.extent(0) == filters && spikes.extent(1) == samples);
  assert(filters == noise_centroids.size() &&
         filters == spike_centroids.size());

  for (std::size_t filter = 0; filter < filters; filter++) {
    for (std::size_t sample = 0; sample < samples; sample++) {
      const float sample_value =
          pulse_t[filter, sample] * static_cast<float>(spikes[filter, sample]);

      const float noise_dist = std::abs(sample_value - noise_centroids[filter]);
      const float spike_dist = std::abs(sample_value - spike_centroids[filter]);

      if (noise_dist > spike_dist) {
        discharge_times[filter].push_back(sample);
      }
    }
  }
}
