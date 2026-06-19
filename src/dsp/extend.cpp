//******************************************************************************
// Extends a signal by continually shifting the signal forward by one column,
// and pasting below until the number of extended channels is reached.
//
// Input:
// ext_signal: extended dimension (CK x (N+K)) emg signal encapsulated in mdspan
// viewer.
// signal: extended dimension (C x N) emg input signal
// ex_factor: K
//
// Output: void
//******************************************************************************

#include "emg-rt/dsp/extend.h"
#include "emg-rt/utils/types.h"

#include <cassert>

using namespace emg_rt;

void extend(RingMatrix<float> &ext_signal, RingMatrix<float> &signal,
            const std::size_t &ex_factor, std::vector<float> &sums,
            const std::size_t &new_samples) {
  std::size_t channels = signal.rows;
  std::size_t samples = signal.cols;

  assert(ext_signal.rows == channels * ex_factor);
  assert(ext_signal.cols == samples + ex_factor);

  for (std::size_t new_sample = 0; new_sample < new_samples; ++new_sample) {
    for (std::size_t block = 0; block < ex_factor; ++block) {
      for (std::size_t channel = 0; channel < channels; ++channel) {
        sums[(block * channels) + channel] -=
            ext_signal[(block * channels) + channel, new_samples - new_sample];
        ext_signal[(block * channels) + channel, new_samples - new_sample] =
            signal[channel, signal.cols - (new_sample + block)];
        sums[(block * channels) + channel] +=
            signal[channel, signal.cols - (new_sample + block)];
      }
    }
  }
}
