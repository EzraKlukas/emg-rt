#ifndef DISTIME_H
#define DISTIME_H

#include "emg-rt/utils/types.h"

void get_distime(emg_rt::RingMatrix<bool> &discharges,
                 const emg_rt::RingMatrix<float> &pulse_t,
                 const emg_rt::RingMatrix<bool> &spikes,
                 const emg_rt::ConstVectorView<float> &noise_centroids,
                 const emg_rt::ConstVectorView<float> &spike_centroids);

#endif
