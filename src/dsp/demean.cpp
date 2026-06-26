#include "emg-rt/dsp/demean.h"
#include "emg-rt/utils/types.h"

using namespace emg_rt;

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

/*
 * Runs before the first decomposition cycle to calculate historical running
 * mean.
 *
 * Because each extension block is translated one sample to the right of the
 * last one in the extended signal, each subsequent block in real-time will
 * contain data that's one sample older than the last one. Hence, we shift the
 * subset of our initial signal used to calculate the sum for a given row
 * according to the extension block it's in.
 */
std::vector<float> initial_sums(emg_rt::RingMatrix<float> &signal,
                                std::size_t demean_window_size,
                                std::size_t ex_factor) {
  std::vector<float> sums(signal.rows * ex_factor);

  for (std::size_t block = 0; block < ex_factor; ++block) {
    for (std::size_t col = 0; col < demean_window_size; ++col) {
      for (std::size_t row = 0; row < signal.rows; ++row) {
        sums[(block * signal.rows) + row] +=
            signal[row, (signal.cols - 1) - (col + block)];
      }
    }
  }

  return sums;
}

// To run every cycle
void incremental_demean(RingMatrix<float> &ext_signal,
                        std::size_t demean_window_size,
                        std::vector<float> &sums) {
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
