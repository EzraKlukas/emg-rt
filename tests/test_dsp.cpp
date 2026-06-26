#include "emg-rt/dsp/demean.h"

#include <doctest/doctest.h>

TEST_CASE("initial_sums returns one accumulator per signal row") {
  emg_rt::RingMatrix<float> signal(2, 2);
  signal[0, 0] = 1.0F;
  signal[0, 1] = 2.0F;
  signal[1, 0] = 10.0F;
  signal[1, 1] = 20.0F;

  const std::vector<float> sums = initial_sums(signal, 1, 1);

  CHECK(sums.size() == 2);
}
