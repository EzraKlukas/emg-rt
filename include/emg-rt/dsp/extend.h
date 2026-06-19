#ifndef EXTEND_H
#define EXTEND_H

#include "emg-rt/utils/types.h"

void extend(emg_rt::RingMatrix<float> &ext_signal,
            emg_rt::RingMatrix<float> &signal, const std::size_t &ex_factor,
            std::vector<float> &sums, const std::size_t &new_samples);

#endif
