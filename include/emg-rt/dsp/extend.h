/*
 * Temporal extension interface.
 *
 * Temporal extension converts a physical-channel EMG signal into an extended
 * signal whose rows contain delayed copies of those channels. This allows a
 * motor-unit filter to operate on a short waveform history using a simple
 * matrix-vector or matrix-matrix projection.
 *
 * `incremental_extend` is the online version used by the real-time pipeline. It
 * updates only the newest samples and also maintains the rolling sums needed
 * for later demeaning.
 */

#ifndef EXTEND_H
#define EXTEND_H

#include "emg-rt/utils/types.h"

namespace emg_rt::dsp {
void incremental_extend(emg_rt::RingMatrix<float> &ext_signal,
                        emg_rt::RingMatrix<float> &signal,
                        std::size_t ex_factor, std::vector<float> &sums,
                        std::size_t new_samples,
                        std::size_t demean_window_size);
} // namespace emg_rt::dsp

#endif
