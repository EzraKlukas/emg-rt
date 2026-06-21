#ifndef ISLOCALMAX_H
#define ISLOCALMAX_H

#include "emg-rt/utils/types.h"

void is_local_max(const emg_rt::RingMatrix<float> &src,
                  emg_rt::RingMatrix<bool> &dst, std::size_t min_peak_distance);

#endif
