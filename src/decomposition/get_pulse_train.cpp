//******************************************************************************
// Performs matrix multiplication and element-wise division to obtain pulse
// train, implementing matlab line:
// PulseT = ((MUfilt * esample) .* abs(MUfilt * esample)) ./ norm';
//
// Input:
// pulse_t: pulse train (dimensions M x N)
// emg_buffer: extended EMG signal (dimensions CK x N)
// mu_filters: motor unit filters (dimensions M x CK)
// norm: encodes filter-wise norm (dimension M x 1);
//
// Output: void
//******************************************************************************

#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/utils/debug_views.h"
#include "emg-rt/utils/types.h"

#include <cassert>
#include <cstdlib>
#include <mdspan>

using namespace emg_rt;

void get_pulse_train(RingMatrix<float> &pulse_t,
                     const RingMatrix<float> &emg_buffer,
                     ConstMatrixView<float> mu_filters,
                     ConstVectorView<float> norm) {
  std::size_t filters = mu_filters.extent(0);
  std::size_t samples = emg_buffer.cols;
  std::size_t extended_channels = emg_buffer.rows;

  float tmp;

  assert(norm.extent(0) == filters);
  assert(mu_filters.extent(1) == extended_channels);

  for (std::size_t filter = 0; filter < filters; ++filter) {
    for (std::size_t sample = 0; sample < samples; ++sample) {
      for (std::size_t extended_channel = 0;
           extended_channel < extended_channels; ++extended_channel) {
        tmp = mu_filters[filter, extended_channel] *
              emg_buffer[extended_channel, sample];
        tmp *= std::abs(tmp);
        tmp /= norm[filter];

        pulse_t[filter, sample] += tmp;
      }
    }
  }
}
