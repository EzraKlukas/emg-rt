#include "emg-rt/dsp/demean.h"
#include "emg-rt/dsp/extend.h"

#include <doctest/doctest.h>

TEST_CASE("demean subtracts each channel mean in place") {
  emg_rt::RingMatrix<float> signal(2, 3);
  signal[0, 0] = 1.0F;
  signal[0, 1] = 2.0F;
  signal[0, 2] = 3.0F;
  signal[1, 0] = 2.0F;
  signal[1, 1] = 4.0F;
  signal[1, 2] = 6.0F;

  demean(signal);

  CHECK(signal[0, 0] == doctest::Approx(-1.0F));
  CHECK(signal[0, 1] == doctest::Approx(0.0F));
  CHECK(signal[0, 2] == doctest::Approx(1.0F));
  CHECK(signal[1, 0] == doctest::Approx(-2.0F));
  CHECK(signal[1, 1] == doctest::Approx(0.0F));
  CHECK(signal[1, 2] == doctest::Approx(2.0F));
}

TEST_CASE("extend copies delayed channel blocks") {
  emg_rt::RingMatrix<float> signal(2, 3);
  signal[0, 0] = 1.0F;
  signal[0, 1] = 2.0F;
  signal[0, 2] = 3.0F;
  signal[1, 0] = 10.0F;
  signal[1, 1] = 20.0F;
  signal[1, 2] = 30.0F;

  emg_rt::RingMatrix<float> extended(4, 4);

  extend(extended, signal, 2);

  CHECK(extended[0, 0] == doctest::Approx(1.0F));
  CHECK(extended[0, 2] == doctest::Approx(3.0F));
  CHECK(extended[1, 1] == doctest::Approx(20.0F));
  CHECK(extended[2, 0] == doctest::Approx(0.0F));
  CHECK(extended[2, 1] == doctest::Approx(1.0F));
  CHECK(extended[2, 3] == doctest::Approx(3.0F));
  CHECK(extended[3, 1] == doctest::Approx(10.0F));
  CHECK(extended[3, 3] == doctest::Approx(30.0F));
}
