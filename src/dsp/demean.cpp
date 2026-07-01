/*
 * Demean the temporally extended EMG signal.
 *
 * The online decomposition pipeline does not work directly with the raw
 * physical-channel signal. First, the signal is temporally extended: delayed
 * copies of each physical channel are stacked as extra rows. This lets each
 * column of the extended signal contain a short window of recent EMG history.
 *
 * Because the extended signal contains delayed copies of the physical channels,
 * each extended row needs its own mean. For example, if `block == 0` represents
 * the most recent samples, then `block == 1` represents samples that are one
 * sample older, `block == 2` represents samples that are two samples older, and
 * so on. The mean for each row must therefore be computed from the
 * corresponding delayed history, not from the same time window for every row.
 *
 * This file provides two pieces of that process:
 *
 * 1. `initial_sums`
 *
 *    Before the first real-time decomposition cycle, we compute one rolling sum
 * of demean_window_size samples for each row of the extended signal. These sums
 * represent the historical baseline used for demeaning. The sum for each
 * extension block is shifted by that block's delay, so the mean lines up with
 * the delayed samples that will appear in that row of the extended signal.
 *
 * 2. `incremental_demean`
 *
 *    During each decomposition cycle, the extended signal has already been
 *    constructed and the rolling sums have already been updated. This function
 *    subtracts the current mean from every element of each extended row:
 *
 *        ext_signal(row, sample) -= sums[row] / demean_window_size
 *
 *    This removes slow baseline offsets while preserving the short-timescale
 *    waveform structure used by the motor-unit filters.
 *
 * The rolling sums are maintained incrementally instead of recomputing the full
 * mean from scratch each cycle. This keeps the real-time loop cheaper: updating
 * the sums happens as new samples enter and old samples leave the demeaning
 * window, while this file simply applies the current row-wise means.
 */

#include "emg-rt/dsp/demean.h"
#include "emg-rt/profiling/timer.h"
#include "emg-rt/utils/types.h"
#include <cassert>

using namespace emg_rt;

/*
 * Compute the initial rolling sums used to demean the extended signal.
 *
 * The returned vector has one entry per extended row:
 *
 *     signal.rows * ex_factor
 *
 * For each extension block, we sum a window of past samples shifted by that
 * block's delay. This matches the layout used by the temporally extended
 * signal: block 0 uses the most recent history, block 1 uses history delayed by
 * one sample, block 2 uses history delayed by two samples, and so on.
 */
std::vector<float> dsp::initial_sums(emg_rt::RingMatrix<float> &signal,
                                     std::size_t demean_window_size,
                                     std::size_t ex_factor) {
  std::vector<float> demean_sums(signal.rows * ex_factor);

  assert(signal.rows == demean_sums.size());
  assert(signal.cols >= demean_window_size + ex_factor);

  for (std::size_t block = 0; block < ex_factor; ++block) {
    for (std::size_t col = 0; col < demean_window_size; ++col) {
      for (std::size_t row = 0; row < signal.rows; ++row) {
        demean_sums[(block * signal.rows) + row] +=
            signal(row, (signal.cols - 1) - (col + block));
      }
    }
  }

  return demean_sums;
}

/*
 * Subtract the current rolling mean from each row of the extended signal.
 *
 * `sums[ext_channel]` stores the rolling sum for one extended row. Dividing by
 * `demean_window_size` gives that row's current mean. We subtract that same
 * row-wise mean from every newly constructed sample in `ext_signal`.
 *
 * This function assumes that `sums` has already been initialized by
 * `initial_sums` and updated for the current cycle by the extension step.
 */
void dsp::incremental_demean(RingMatrix<float> &ext_signal,
                             std::size_t demean_window_size,
                             std::vector<float> &demean_sums) {
  EMG_RT_PROFILE(emg_rt::prof::Section::demean);

  std::size_t ext_channels = ext_signal.rows;
  std::size_t samples = ext_signal.cols;

  for (std::size_t sample = 0; sample < samples; ++sample) {
    for (std::size_t ext_channel = 0; ext_channel < ext_channels;
         ++ext_channel) {
      const float mean =
          demean_sums[ext_channel] / static_cast<float>(demean_window_size);
      ext_signal(ext_channel, sample) -= mean;
    }
  }
}
