#ifndef EXTEND_H
#define EXTEND_H

#include "emg-rt/utils/types.h"
#include <cstdint>

void extend(emg_rt::MatrixView<float> ext_signal,
            emg_rt::ConstMatrixView<float> signal, uint_fast16_t extension_t_ms,
            uint_fast16_t f_samp);

#endif
