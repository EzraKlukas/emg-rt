#ifndef DISTIME_H
#define DISTIME_H

#include "emg-rt/utils/types.h"

void get_distime(emg_rt::RingMatrix<bool> &discharges,
                 const emg_rt::RingMatrix<float> &pulse_t,
                 const emg_rt::RingMatrix<bool> &spikes,
                 const emg_rt::VectorView<float> &noise_centroids,
                 const emg_rt::VectorView<float> &spike_centroids,
                 const std::vector<std::size_t> &samples_onset,
                 std::size_t new_samples, std::size_t min_lookahead_samps);

#endif
