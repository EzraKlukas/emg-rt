/*
 * Discharge-time classification interface.
 *
 * `classify_discharges` converts candidate pulse-train peaks into final
 * discharge decisions. Candidate peaks are compared against precomputed noise
 * and spike centroids for each motor unit, and are delayed according to their
 * onset calculated physiological delay.
 *
 * The output is a boolean discharge mask with one row per motor unit and one
 * column per retained pulse-train sample.
 */

#ifndef CLASSIFY_DISCHARGES_H
#define CLASSIFY_DISCHARGES_H

#include "emg-rt/utils/types.h"

namespace emg_rt::decomp {
void classify_discharges(emg_rt::RingMatrix<bool> &discharge_mask,
                         const emg_rt::RingMatrix<float> &pulse_train,
                         const emg_rt::RingMatrix<bool> &candidate_spikes,
                         const emg_rt::VectorView<float> &noise_centroids,
                         const emg_rt::VectorView<float> &spike_centroids,
                         const std::vector<std::size_t> &samples_onset,
                         std::size_t new_samples,
                         std::size_t min_lookahead_samps);
}

#endif
