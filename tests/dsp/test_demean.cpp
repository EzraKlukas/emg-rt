#include "emg-rt/dsp/demean.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <vector>

TEST_CASE(
    "initial_sums computes shifted history sums for each extension block") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {1.0F, 2.0F, 3.0F, 4.0F, 5.0F},
      {10.0F, 20.0F, 30.0F, 40.0F, 50.0F},
  });

  const std::vector<float> sums = emg_rt::dsp::initial_sums(signal, 3, 2);

  REQUIRE(sums.size() == 4);
  CHECK(sums[0] == doctest::Approx(12.0F));
  CHECK(sums[1] == doctest::Approx(120.0F));
  CHECK(sums[2] == doctest::Approx(9.0F));
  CHECK(sums[3] == doctest::Approx(90.0F));
}

TEST_CASE("initial_sums respects wrapped logical history") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {1.0F, 2.0F, 3.0F, 4.0F, 5.0F},
  });
  const float col6[] = {6.0F};
  const float col7[] = {7.0F};
  signal.write_column(col6);
  signal.write_column(col7);

  const std::vector<float> sums = emg_rt::dsp::initial_sums(signal, 3, 2);

  REQUIRE(sums.size() == 2);
  CHECK(sums[0] == doctest::Approx(18.0F));
  CHECK(sums[1] == doctest::Approx(15.0F));
}

TEST_CASE("incremental_demean subtracts the current per-row rolling mean") {
  auto extended = emg_rt::tests::matrix_from_rows<float>({
      {6.0F, 8.0F},
      {16.0F, 20.0F},
  });
  std::vector<float> sums = {4.0F, 8.0F};

  emg_rt::dsp::incremental_demean(extended, 2, sums);

  CHECK(extended(0, 0) == doctest::Approx(4.0F));
  CHECK(extended(0, 1) == doctest::Approx(6.0F));
  CHECK(extended(1, 0) == doctest::Approx(12.0F));
  CHECK(extended(1, 1) == doctest::Approx(16.0F));
}

TEST_CASE("incremental_demean turns constant rows into zero rows") {
  auto extended = emg_rt::tests::matrix_from_rows<float>({
      {5.0F, 5.0F, 5.0F},
      {-2.0F, -2.0F, -2.0F},
  });
  std::vector<float> sums = {15.0F, -6.0F};

  emg_rt::dsp::incremental_demean(extended, 3, sums);

  for (std::size_t col = 0; col < extended.cols; ++col) {
    CHECK(extended(0, col) == doctest::Approx(0.0F));
    CHECK(extended(1, col) == doctest::Approx(0.0F));
  }
}

TEST_CASE("incremental_demean does not mix means across rows") {
  auto extended = emg_rt::tests::matrix_from_rows<float>({
      {-3.0F, 1.0F, 5.0F},
      {10.0F, 20.0F, 30.0F},
  });
  std::vector<float> sums = {3.0F, 60.0F};

  emg_rt::dsp::incremental_demean(extended, 3, sums);

  CHECK(extended(0, 0) == doctest::Approx(-4.0F));
  CHECK(extended(0, 1) == doctest::Approx(0.0F));
  CHECK(extended(0, 2) == doctest::Approx(4.0F));
  CHECK(extended(1, 0) == doctest::Approx(-10.0F));
  CHECK(extended(1, 1) == doctest::Approx(0.0F));
  CHECK(extended(1, 2) == doctest::Approx(10.0F));
}
