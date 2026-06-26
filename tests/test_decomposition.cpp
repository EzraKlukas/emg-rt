#include "emg-rt/decomposition/get_discharge_t.h"
#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/decomposition/is_local_max.h"

#include <doctest/doctest.h>

#include <array>
#include <vector>

TEST_CASE("is_local_max marks separated interior peaks") {
  emg_rt::RingMatrix<float> src(2, 5);
  src[0, 0] = 0.0F;
  src[0, 1] = 2.0F;
  src[0, 2] = 0.0F;
  src[0, 3] = 1.0F;
  src[0, 4] = 0.0F;
  src[1, 0] = 0.0F;
  src[1, 1] = 1.0F;
  src[1, 2] = 0.0F;
  src[1, 3] = 3.0F;
  src[1, 4] = 0.0F;

  emg_rt::RingMatrix<bool> dst(2, 5);

  is_local_max(src, dst, 1);

  CHECK_FALSE(dst[0, 0]);
  CHECK(dst[0, 1]);
  CHECK(dst[0, 3]);
  CHECK(dst[1, 1]);
  CHECK(dst[1, 3]);
  CHECK_FALSE(dst[1, 4]);
}

TEST_CASE("is_local_max keeps only the larger nearby peak") {
  emg_rt::RingMatrix<float> src(1, 5);
  src[0, 0] = 0.0F;
  src[0, 1] = 1.0F;
  src[0, 2] = 0.0F;
  src[0, 3] = 2.0F;
  src[0, 4] = 0.0F;

  emg_rt::RingMatrix<bool> dst(1, 5);

  is_local_max(src, dst, 2);

  CHECK_FALSE(dst[0, 1]);
  CHECK(dst[0, 3]);
}

TEST_CASE("get_pulse_train computes one-channel filtered pulse values") {
  emg_rt::RingMatrix<float> emg(1, 3);
  emg[0, 0] = 2.0F;
  emg[0, 1] = -3.0F;
  emg[0, 2] = 4.0F;

  std::vector<float> filters{2.0F};
  std::vector<float> norms{2.0F};
  emg_rt::RingMatrix<float> pulse(1, 3);

  emg_rt::ConstMatrixView<float> filter_view(filters.data(), 1, 1);
  emg_rt::ConstVectorView<float> norm_view(norms.data(), 1);

  get_pulse_train(pulse, emg, filter_view, norm_view);

  CHECK(pulse[0, 0] == doctest::Approx(8.0F));
  CHECK(pulse[0, 1] == doctest::Approx(-18.0F));
  CHECK(pulse[0, 2] == doctest::Approx(32.0F));
}

TEST_CASE("get_distime appends samples closer to spike centroid") {
  emg_rt::RingMatrix<float> pulse(2, 3);
  pulse[0, 0] = 0.1F;
  pulse[0, 1] = 0.9F;
  pulse[0, 2] = 1.2F;
  pulse[1, 0] = 0.2F;
  pulse[1, 1] = 2.8F;
  pulse[1, 2] = 3.1F;

  emg_rt::RingMatrix<bool> spikes(2, 3);
  spikes[0, 0] = false;
  spikes[0, 1] = true;
  spikes[0, 2] = true;
  spikes[1, 0] = false;
  spikes[1, 1] = true;
  spikes[1, 2] = true;

  std::vector<float> noise_centroids{0.0F, 0.0F};
  std::vector<float> spike_centroids{1.0F, 3.0F};
  emg_rt::RingMatrix<bool> discharges(2, 3);

  emg_rt::ConstVectorView<float> noise_view(noise_centroids.data(), 2);
  emg_rt::ConstVectorView<float> spike_view(spike_centroids.data(), 2);

  get_distime(discharges, pulse, spikes, noise_view, spike_view, 2, 0);

  CHECK_FALSE(discharges[0, 0]);
  CHECK(discharges[0, 1]);
  CHECK(discharges[0, 2]);
  CHECK_FALSE(discharges[1, 0]);
  CHECK(discharges[1, 1]);
  CHECK(discharges[1, 2]);
}
