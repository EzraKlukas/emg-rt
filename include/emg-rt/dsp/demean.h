/*
 * Demeaning interface for temporally extended EMG signals.
 *
 * The online pipeline subtracts a rolling row-wise mean from the extended EMG
 * signal before applying motor-unit filters. Because each extension row
 * represents a different physical channel and sample delay, each row needs its
 * own rolling mean.
 *
 * `initial_sums` computes the starting rolling sums before the first online
 * cycle.
 *
 * `incremental_demean` subtracts the current row-wise means from the extended
 * signal during each cycle. The sums themselves are updated by the extension
 * step as new samples enter and old samples leave the demeaning window.
 */

#ifndef DEMEAN_H
#define DEMEAN_H

#include "emg-rt/utils/types.h"

namespace emg_rt::dsp {
std::vector<float> initial_sums(emg_rt::RingMatrix<float> &signal,
                                std::size_t demean_window_size,
                                std::size_t ex_factor);

void incremental_demean(emg_rt::RingMatrix<float> &ext_signal,
                        std::size_t demean_window_size,
                        std::vector<float> &sums);
} // namespace emg_rt::dsp

#endif
