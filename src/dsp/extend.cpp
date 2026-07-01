/*
 * Build a temporally extended EMG signal matrix.
 *
 * Motor-unit filters are not applied to a single instantaneous EMG sample.
 * They are applied to a short window of recent signal history. To make that
 * possible in a simple matrix-vector projection, we represent time history as
 * additional rows: each physical channel is repeated at several sample delays.
 *
 * If the original signal has `channels` physical channels and the extension
 * factor is `ex_factor`, then the extended signal has:
 *
 *     channels * ex_factor
 *
 * rows. Rows are arranged in delay blocks:
 *
 *     row = block * channels + channel
 *
 * where `block == 0` is the most recent sample, `block == 1` is one sample
 * older, `block == 2` is two samples older, and so on.
 *
 * Example with two physical channels and ex_factor = 5:
 *
 *     signal history:
 *
 *         timestamp:    7      8      9
 *         ch 0:       s_07   s_08   s_09
 *         ch 1:       s_17   s_18   s_19
 *
 *     extended representation:
 *
 *         delay 0, ch 0:   s_07   s_08   s_09
 *         delay 0, ch 1:   s_17   s_18   s_19
 *         delay 1, ch 0:   s_06   s_07   s_08
 *         delay 1, ch 1:   s_16   s_17   s_18
 *         delay 2, ch 0:   s_05   s_06   s_07
 *         delay 2, ch 1:   s_15   s_16   s_17
 *         delay 3, ch 0:   s_04   s_05   s_06
 *         delay 3, ch 1:   s_14   s_15   s_16
 *         delay 4, ch 0:   s_03   s_04   s_05
 *         delay 4, ch 1:   s_13   s_14   s_15
 *
 * Each column of `ext_signal` therefore contains the current multi-channel
 * sample plus several previous multi-channel samples. Projecting a column of
 * this matrix against an MU filter is equivalent to comparing the filter
 * against a short recent waveform, rather than against one instantaneous
 * sample.
 *
 * This function also updates `sums`, the rolling sums used to demean the
 * extended signal. Each extended row has its own rolling sum. As new samples
 * enter the extended representation, the corresponding old samples are removed
 * from the sum and the new samples are added. This keeps the demeaning window
 * length fixed over time without recomputing each mean from scratch.
 *
 * `demean_window_size` controls how much history contributes to each rolling
 * mean. The caller is responsible for ensuring that `signal` contains enough
 * past samples for both the requested extension and the demeaning window.
 */

#include "emg-rt/dsp/extend.h"
#include "emg-rt/profiling/timer.h"
#include "emg-rt/utils/types.h"

#include <cassert>

#define MS_PER_S 1000

using namespace emg_rt;

void dsp::incremental_extend(RingMatrix<float> &ext_signal,
                             RingMatrix<float> &signal, std::size_t ex_factor,
                             std::vector<float> &sums, std::size_t new_samples,
                             std::size_t demean_window_size) {
  EMG_RT_PROFILE(prof::Section::extend);
  std::size_t channels = signal.rows;

  assert(ext_signal.rows == channels * ex_factor);
  assert(ext_signal.cols == new_samples);

  for (std::size_t new_sample = 0; new_sample < new_samples; ++new_sample) {
    for (std::size_t block = 0; block < ex_factor; ++block) {
      for (std::size_t channel = 0; channel < channels; ++channel) {
        const std::size_t ext_row = (block * channels) + channel;

        // Remove the sample that just left this row's demeaning window.
        sums[ext_row] -=
            signal(channel, (signal.cols - 1) - (new_sample + block) -
                                demean_window_size);

        // Copy the delayed physical-channel sample into the extended matrix.
        ext_signal(ext_row, (new_samples - 1) - new_sample) =
            signal(channel, (signal.cols - 1) - (new_sample + block));

        // Add the new sample to this row's demeaning sum.
        sums[ext_row] +=
            signal(channel, (signal.cols - 1) - (new_sample + block));
      }
    }
  }
}
