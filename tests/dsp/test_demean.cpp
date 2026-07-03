#include "emg-rt/dsp/demean.h"
#include "emg-rt/dsp/extend.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <vector>

TEST_CASE("initial_sums computes row-wise delayed rolling windows") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {1.0F, 2.0F, 3.0F, 4.0F, 5.0F},
      {10.0F, 20.0F, 30.0F, 40.0F, 50.0F},
  });

  const std::vector<float> sums = emg_rt::dsp::initial_sums(signal, 2, 2);

  emg_rt::tests::expect_vector_approx(sums, {9.0F, 90.0F, 7.0F, 70.0F});
}

TEST_CASE("incremental_demean subtracts independent row-wise means") {
  auto extended = emg_rt::tests::matrix_from_rows<float>({
      {5.0F, 6.0F, 7.0F},
      {10.0F, 14.0F, 18.0F},
  });
  std::vector<float> sums = {6.0F, 12.0F};

  emg_rt::dsp::incremental_demean(extended, 3, sums);

  emg_rt::tests::expect_matrix_approx(extended, {
                                                    {3.0F, 4.0F, 5.0F},
                                                    {6.0F, 10.0F, 14.0F},
                                                });
}

TEST_CASE("constant signal becomes zero after extend and demean") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {5.0F, 5.0F, 5.0F, 5.0F},
  });
  emg_rt::RingMatrix<float> extended(1, 2);
  std::vector<float> sums = emg_rt::dsp::initial_sums(signal, 2, 1);

  emg_rt::dsp::incremental_extend(extended, signal, 1, sums, 2, 2);
  emg_rt::dsp::incremental_demean(extended, 2, sums);

  emg_rt::tests::expect_matrix_approx(extended, {
                                                    {0.0F, 0.0F},
                                                });
}

TEST_CASE("combined extend and demean matches explicit tiny expectation") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {4.0F, 4.0F, 1.0F, 3.0F, 1.0F},
  });
  emg_rt::RingMatrix<float> extended(1, 3);
  std::vector<float> sums = emg_rt::dsp::initial_sums(signal, 2, 1);

  emg_rt::dsp::incremental_extend(extended, signal, 1, sums, 3, 2);
  emg_rt::dsp::incremental_demean(extended, 2, sums);

  emg_rt::tests::expect_matrix_approx(extended, {
                                                    {1.0F, 3.0F, 1.0F},
                                                });
}
