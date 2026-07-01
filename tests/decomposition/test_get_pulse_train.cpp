#include "emg-rt/decomposition/get_pulse_train.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <vector>

TEST_CASE(
    "get_pulse_train computes squared signed projection divided by norm") {
  auto emg = emg_rt::tests::matrix_from_rows<float>({
      {2.0F, -3.0F, 4.0F},
  });

  std::vector<float> filters = {2.0F};
  std::vector<float> norms = {2.0F};
  emg_rt::RingMatrix<float> pulse(1, 3);

  emg_rt::MatrixView<float> filter_view(filters.data(), 1, 1);
  emg_rt::VectorView<float> norm_view(norms.data(), 1);

  emg_rt::decomp::get_pulse_train(pulse, emg, filter_view, norm_view);

  CHECK(pulse(0, 0) == doctest::Approx(8.0F));
  CHECK(pulse(0, 1) == doctest::Approx(-18.0F));
  CHECK(pulse(0, 2) == doctest::Approx(32.0F));
}

TEST_CASE("get_pulse_train honors logical ring order for wrapped buffers") {
  auto emg = emg_rt::tests::matrix_from_rows<float>({
      {1.0F, 2.0F, 3.0F},
  });
  const float new_column[] = {4.0F};
  emg.write_column(new_column);

  std::vector<float> filters = {1.0F};
  std::vector<float> norms = {1.0F};
  emg_rt::RingMatrix<float> pulse(1, 3);

  emg_rt::MatrixView<float> filter_view(filters.data(), 1, 1);
  emg_rt::VectorView<float> norm_view(norms.data(), 1);

  emg_rt::decomp::get_pulse_train(pulse, emg, filter_view, norm_view);

  CHECK(pulse(0, 0) == doctest::Approx(4.0F));
  CHECK(pulse(0, 1) == doctest::Approx(9.0F));
  CHECK(pulse(0, 2) == doctest::Approx(16.0F));
}
