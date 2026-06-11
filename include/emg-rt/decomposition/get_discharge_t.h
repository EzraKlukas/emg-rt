#ifndef DISTIME_H
#define DISTIME_H

#include "emg-rt/utils/types.h"
#include <vector>

void get_distime(std::vector<std::vector<std::size_t>> discharge_times,
                 emg_rt::MatrixView<float> pulse_t,
                 emg_rt::MatrixView<float> spikes,
                 emg_rt::ConstVectorView<float> &noise_centroids,
                 emg_rt::ConstVectorView<float> &spike_centroids);

#endif
