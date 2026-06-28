#include "emg-rt/dsp/extend.h"
#include "emg-rt/profiling/timer.h"
#include "emg-rt/utils/types.h"

#include <cassert>

#define MS_PER_S 1000

using namespace emg_rt;

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

/*
 * Here signal is a whole block.
 */
void extend(RingMatrix<float> &ext_signal, const RingMatrix<float> &signal,
            std::size_t &ex_factor) {
  std::size_t channels = signal.rows;
  std::size_t samples = signal.cols;

  assert(ext_signal.rows == channels * ex_factor);
  assert(ext_signal.cols == samples + ex_factor);

  for (std::size_t block = 0; block < ex_factor; block++) {
    for (std::size_t channel = 0; channel < channels; channel++) {
      for (std::size_t sample = 0; sample < samples; sample++) {
        ext_signal((block * channels) + channel, sample + block) =
            signal(channel, sample);
      }
    }
  }
}

/*
 * signal is just the new samples coming in?
 */
void incremental_extend(RingMatrix<float> &ext_signal,
                        RingMatrix<float> &signal, std::size_t ex_factor,
                        std::vector<float> &sums, std::size_t new_samples,
                        std::size_t demean_window_size) {
  emg_rt::prof::ScopedTimer extend_timer(emg_rt::prof::Section::extend);
  std::size_t channels = signal.rows;

  assert(ext_signal.rows == channels * ex_factor);
  assert(ext_signal.cols == new_samples);

  for (std::size_t new_sample = 0; new_sample < new_samples; ++new_sample) {
    for (std::size_t block = 0; block < ex_factor; ++block) {
      for (std::size_t channel = 0; channel < channels; ++channel) {
        sums[(block * channels) + channel] -=
            signal(channel, (signal.cols - 1) - (new_sample + block) -
                                demean_window_size);
        ext_signal((block * channels) + channel,
                   (new_samples - 1) - new_sample) =
            signal(channel, (signal.cols - 1) - (new_sample + block));
        // next line is subtle; that this mean represents the mean of a larger
        // samplesize is encoded in the length of signal: demean_window_size.
        sums[(block * channels) + channel] +=
            signal(channel, (signal.cols - 1) - (new_sample + block));
      }
    }
  }
}
