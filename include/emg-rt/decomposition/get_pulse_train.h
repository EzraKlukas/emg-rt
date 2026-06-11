#ifndef PULSET_H
#define PULSET_H

#include "emg-rt/utils/types.h"

void get_pulse_train(emg_rt::MatrixView<float> pulse_t,
                     emg_rt::ConstMatrixView<float> emg_buffer,
                     emg_rt::ConstMatrixView<float> mu_filters,
                     emg_rt::ConstVectorView<float> norm);

#endif
