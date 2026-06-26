#ifndef ISLOCALMAX_H
#define ISLOCALMAX_H

#include "emg-rt/utils/types.h"

void incremental_is_local_max(const emg_rt::RingMatrix<float> &pulse_t,
                              emg_rt::RingMatrix<bool> &spikes,
                              std::size_t new_samples,
                              std::vector<float> &maxima,
                              std::size_t min_lookahead_dist);

void is_local_max(const emg_rt::RingMatrix<float> &src,
                  emg_rt::RingMatrix<bool> &dst, std::size_t min_peak_distance);

#endif
