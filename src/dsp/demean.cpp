//******************************************************************************
// Removes the mean of a signal.
//
// Input:
// signal: arbitrary-dimensioned mdspan viewer of an underlying contiguous
// vector containing the EMG signal.
//
// Output:
// void
//******************************************************************************

#include "emg-rt/dsp/demean.h"
#include "emg-rt/utils/types.h"

#include <mdspan>

using namespace emg_rt;

void demean(emg_rt::RingMatrix<float> ext_signal,
            const std::size_t &demean_window_size, std::vector<float> &sums,
            const std::size_t &new_samples) {
  std::size_t ext_channels = ext_signal.rows;
  std::size_t samples = ext_signal.cols;
  for (std::size_t sample = 0; sample < samples; ++sample) {
    for (std::size_t ext_channel = 0; ext_channel < ext_channels;
         ++ext_channel) {
      ext_signal[ext_channel, sample] -=
          sums[ext_channel] / (float)demean_window_size;
    }
  }
}
