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

#include <cassert>
#include <mdspan>

void extend(std::mdspan<float, std::dextents<std::size_t, 2>> &ext_signal,
            std::mdspan<float, std::dextents<std::size_t, 2>> &signal,
            std::size_t ex_factor) {
  std::size_t channels = signal.extent(0);
  std::size_t samples = signal.extent(1);

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
