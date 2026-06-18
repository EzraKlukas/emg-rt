#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/decomposition/get_discharge_t.h"
#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/decomposition/is_local_max.h"
#include "emg-rt/utils/types.h"

#include <cassert>
#include <cstdlib>
#include <mdspan>
#include <vector>

using namespace emg_rt;

/*
 * function [PulseT, Distime, esample2] = getspikesonline(EMGtmp,
 * extensionfactor, esample2, MUfilt, norm, noise_centroids, spike_centroids,
 * nsamp, fsamp)
 * Before-hand, I already know the shape of PulseT, esample2, and Distime, in
 * that it's a vector of vectors.
 */
void get_spikes_online(MatrixView<float> pulse_t,
                       RaggedArray<std::size_t> &discharge_times,
                       MatrixView<float> extended_signal);
