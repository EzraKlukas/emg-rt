#ifndef DEMEAN_H
#define DEMEAN_H

#include "emg-rt/utils/types.h"

void demean(emg_rt::RingMatrix<float> ext_signal,
            const std::size_t &demean_window_size, std::vector<float> &sums,
            const std::size_t &new_samples);

#endif
