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

#include <mdspan>

void demean(std::mdspan<float, std::dextents<std::size_t, 2>> signal) {
  std::size_t channels = signal.extent(0);
  std::size_t samples = signal.extent(1);
  float mean = 0.0;
  for (std::size_t channel = 0; channel < channels; channel++) {
    float sum = 0.0;
    for (std::size_t sample = 0; sample < samples; sample++) {
      sum += signal[channel, sample];
    }
    mean = sum / (float)samples;
    for (std::size_t sample = 0; sample < samples; sample++) {
      signal[channel, sample] -= mean;
    }
  }
}
