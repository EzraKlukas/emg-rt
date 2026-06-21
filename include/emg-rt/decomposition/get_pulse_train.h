#ifndef PULSET_H
#define PULSET_H

#include "emg-rt/utils/types.h"

void get_pulse_train(emg_rt::RingMatrix<float> &pulse_t,
                     const emg_rt::RingMatrix<float> &emg_buffer,
                     emg_rt::ConstMatrixView<float> mu_filters,
                     emg_rt::ConstVectorView<float> norm);

#endif
