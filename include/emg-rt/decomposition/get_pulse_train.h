/*
 * Pulse-train projection interface.
 *
 * `get_pulse_train` applies trained motor-unit filters to a temporally extended
 * EMG signal buffer. The output is one pulse train per motor-unit filter.
 *
 * Inputs:
 *
 *   - `emg_buffer`
 *       Temporally extended, demeaned EMG signal.
 *
 *   - `mu_filters`
 *       Trained motor-unit filters. Each row corresponds to one motor unit.
 *
 *   - `inv_filter_norms`
 *       Filter-wise inverse normalization values. These values
 *       are stored as inverse norms so the real-time code can multiply rather
 *       than divide.
 *
 * Output:
 *
 *   - `pulse_train`
 *       Pulse trains, with one row per motor-unit filter and one column per
 *       processed sample.
 */

#ifndef PULSET_H
#define PULSET_H

#include "emg-rt/utils/types.h"

namespace emg_rt::decomp {
void get_pulse_train(emg_rt::RingMatrix<float> &pulse_train,
                     const emg_rt::RingMatrix<float> &emg_buffer,
                     emg_rt::MatrixView<float> mu_filters,
                     emg_rt::VectorView<float> inv_filter_norms);
}

#endif
