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
#include <cstdint>
#include <mdspan>

#define MS_PER_S 1000

using namespace emg_rt;

void extend(MatrixView<float> ext_signal, ConstMatrixView<float> signal,
            uint_fast16_t extension_t_ms, uint_fast16_t f_samp) {
  std::size_t channels = signal.extent(0);
  std::size_t samples = signal.extent(1);
  std::size_t ex_factor = (extension_t_ms * f_samp) / MS_PER_S;

  assert(ext_signal.extent(0) == channels * ex_factor);
  assert(ext_signal.extent(1) == samples + ex_factor);

  for (std::size_t block = 0; block < ex_factor; block++) {
    for (std::size_t channel = 0; channel < channels; channel++) {
      for (std::size_t sample = 0; sample < samples; sample++) {
        ext_signal[(block * channels) + channel, sample + block] =
            signal[channel, sample];
      }
    }
  }
}
