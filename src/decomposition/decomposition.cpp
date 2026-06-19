#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/decomposition/get_discharge_t.h"
#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/decomposition/is_local_max.h"
#include "emg-rt/dsp/extend.h"
#include "emg-rt/utils/types.h"

#include <cassert>
#include <cstdlib>
#include <mdspan>
#include <vector>

using namespace emg_rt;

#define MS_PER_S 1000

/*
 * function [PulseT, Distime, esample2] = getspikesonline(EMGtmp,
 * extensionfactor, esample2, MUfilt, norm, noise_centroids, spike_centroids,
 * nsamp, fsamp)
 * Before-hand, I already know the shape of PulseT, esample2, and Distime, in
 * that it's a vector of vectors.
 *
 * Signal we keep as a queue (easily removable back) maybe is always:
 * num_channels * (demean_sample_size + (sampling_frequency) /
 * (decomposition_frequency) + extension_factor - 1).
 */
void get_spikes_online(MatrixView<float> pulse_t,
                       RaggedArray<std::size_t> &discharge_times,
                       RingMatrix<float> &ext_signal, std::vector<float> &sums,
                       RingMatrix<float> &signal, std::size_t &channels,
                       const std::size_t &ex_factor,
                       const std::size_t &demean_window_size,
                       const std::size_t &new_samples) {
  extend(ext_signal, signal, ex_factor, sums, new_samples);

  for (std::size_t new_sample = 0; new_sample < new_samples; ++new_sample) {
    for (std::size_t block = 0; block < ex_factor; ++block) {
      for (std::size_t channel = 0; channel < channels; ++channel) {
        ext_signal[(block * channels) + channel, new_samples - new_sample] -=
            sums[(block * channels) + channel] / (float)demean_window_size;
      }
    }
  }
}
