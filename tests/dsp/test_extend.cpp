#include "emg-rt/dsp/demean.h"
#include "emg-rt/dsp/extend.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <vector>

TEST_CASE("incremental_extend builds delayed rows for one channel") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F},
  });
  emg_rt::RingMatrix<float> extended(3, 2);
  std::vector<float> sums = {100.0F, 200.0F, 300.0F};

  emg_rt::dsp::incremental_extend(extended, signal, 3, sums, 2, 2);

  CHECK(extended(0, 0) == doctest::Approx(6.0F));
  CHECK(extended(0, 1) == doctest::Approx(7.0F));
  CHECK(extended(1, 0) == doctest::Approx(5.0F));
  CHECK(extended(1, 1) == doctest::Approx(6.0F));
  CHECK(extended(2, 0) == doctest::Approx(4.0F));
  CHECK(extended(2, 1) == doctest::Approx(5.0F));

  CHECK(sums[0] == doctest::Approx(104.0F));
  CHECK(sums[1] == doctest::Approx(204.0F));
  CHECK(sums[2] == doctest::Approx(304.0F));
}

TEST_CASE("incremental_extend orders delay blocks before channel rows") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F},
      {10.0F, 20.0F, 30.0F, 40.0F, 50.0F, 60.0F},
  });
  emg_rt::RingMatrix<float> extended(4, 2);
  std::vector<float> sums(4, 0.0F);

  emg_rt::dsp::incremental_extend(extended, signal, 2, sums, 2, 2);

  CHECK(extended(0, 0) == doctest::Approx(5.0F));
  CHECK(extended(0, 1) == doctest::Approx(6.0F));
  CHECK(extended(1, 0) == doctest::Approx(50.0F));
  CHECK(extended(1, 1) == doctest::Approx(60.0F));
  CHECK(extended(2, 0) == doctest::Approx(4.0F));
  CHECK(extended(2, 1) == doctest::Approx(5.0F));
  CHECK(extended(3, 0) == doctest::Approx(40.0F));
  CHECK(extended(3, 1) == doctest::Approx(50.0F));
}

TEST_CASE("incremental_extend reads logical history after signal wraparound") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {1.0F, 2.0F, 3.0F, 4.0F, 5.0F},
  });
  const float col6[] = {6.0F};
  const float col7[] = {7.0F};
  signal.write_column(col6);
  signal.write_column(col7);
  emg_rt::RingMatrix<float> extended(2, 2);
  std::vector<float> sums(2, 0.0F);

  emg_rt::dsp::incremental_extend(extended, signal, 2, sums, 2, 2);

  CHECK(signal.head == 2);
  CHECK(extended(0, 0) == doctest::Approx(6.0F));
  CHECK(extended(0, 1) == doctest::Approx(7.0F));
  CHECK(extended(1, 0) == doctest::Approx(5.0F));
  CHECK(extended(1, 1) == doctest::Approx(6.0F));
}

TEST_CASE("extend then demean matches hand-computed extended matrix") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {0.0F, 0.0F, 0.0F, -2.0F, 4.0F, -2.0F},
  });
  emg_rt::RingMatrix<float> extended(1, 3);
  std::vector<float> sums = emg_rt::dsp::initial_sums(signal, 3, 1);

  emg_rt::dsp::incremental_extend(extended, signal, 1, sums, 3, 3);

  CHECK(extended(0, 0) == doctest::Approx(-2.0F));
  CHECK(extended(0, 1) == doctest::Approx(4.0F));
  CHECK(extended(0, 2) == doctest::Approx(-2.0F));
  CHECK(sums[0] == doctest::Approx(0.0F));

  emg_rt::dsp::incremental_demean(extended, 3, sums);

  CHECK(extended(0, 0) == doctest::Approx(-2.0F));
  CHECK(extended(0, 1) == doctest::Approx(4.0F));
  CHECK(extended(0, 2) == doctest::Approx(-2.0F));
}

TEST_CASE("incremental_extend handles a one-sample output window") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {2.0F, 4.0F, 6.0F},
      {20.0F, 40.0F, 60.0F},
  });
  emg_rt::RingMatrix<float> extended(4, 1);
  std::vector<float> sums(4, 0.0F);

  emg_rt::dsp::incremental_extend(extended, signal, 2, sums, 1, 1);

  CHECK(extended(0, 0) == doctest::Approx(6.0F));
  CHECK(extended(1, 0) == doctest::Approx(60.0F));
  CHECK(extended(2, 0) == doctest::Approx(4.0F));
  CHECK(extended(3, 0) == doctest::Approx(40.0F));
}
