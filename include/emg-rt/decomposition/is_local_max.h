/*
 * Incremental local-maximum detection for pulse trains.
 *
 * Pulse trains are processed online, so local maxima cannot always be confirmed
 * immediately. A sample that looks maximal now may be exceeded by a nearby
 * future sample. This interface marks candidate maxima while keeping enough
 * lookahead history to later reject candidates that are not true local maxima.
 *
 * The resulting boolean mask is used by the discharge classification step.
 */

#ifndef ISLOCALMAX_H
#define ISLOCALMAX_H

#include "emg-rt/utils/types.h"

namespace emg_rt::decomp {
void incremental_is_local_max(const emg_rt::RingMatrix<float> &pulse_train,
                              emg_rt::RingMatrix<bool> &candidate_spikes,
                              std::size_t new_samples,
                              std::vector<float> &maxima,
                              std::size_t min_lookahead_dist);
}

#endif
