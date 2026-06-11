#ifndef ISLOCALMAX_H
#define ISLOCALMAX_H

#include "emg-rt/utils/types.h"

void islocalmax(emg_rt::ConstMatrixView<float> src,
                emg_rt::MatrixView<bool> dst, std::size_t min_peak_distance);

#endif
