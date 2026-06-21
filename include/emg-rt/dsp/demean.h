#ifndef DEMEAN_H
#define DEMEAN_H

#include "emg-rt/utils/types.h"

std::vector<float> initial_sums(emg_rt::RingMatrix<float> &signal);

void incremental_demean(emg_rt::RingMatrix<float> &ext_signal,
                        const std::size_t &demean_window_size,
                        std::vector<float> &sums);

#endif
