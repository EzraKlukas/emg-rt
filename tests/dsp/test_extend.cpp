#include "emg-rt/dsp/extend.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <vector>

TEST_CASE("incremental_extend stacks delay blocks in documented row order") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {10.0F, 11.0F, 12.0F, 13.0F, 14.0F},
      {20.0F, 21.0F, 22.0F, 23.0F, 24.0F},
  });
  emg_rt::RingMatrix<float> extended(6, 2);
  std::vector<float> sums(6, 0.0F);

  emg_rt::dsp::incremental_extend(extended, signal, 3, sums, 2, 1);

  emg_rt::tests::expect_matrix_approx(extended, {
                                                    {13.0F, 14.0F},
                                                    {23.0F, 24.0F},
                                                    {12.0F, 13.0F},
                                                    {22.0F, 23.0F},
                                                    {11.0F, 12.0F},
                                                    {21.0F, 22.0F},
                                                });
}

TEST_CASE("incremental_extend with extension factor one copies newest samples") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {7.0F, 8.0F, 9.0F},
  });
  emg_rt::RingMatrix<float> extended(1, 2);
  std::vector<float> sums(1, 0.0F);

  emg_rt::dsp::incremental_extend(extended, signal, 1, sums, 2, 1);

  emg_rt::tests::expect_matrix_approx(extended, {
                                                    {8.0F, 9.0F},
                                                });
}

TEST_CASE("incremental_extend preserves logical time order from wrapped input") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {1.0F, 2.0F, 3.0F, 4.0F, 5.0F},
  });
  const float newest[] = {6.0F};
  signal.write_column(newest, std::vector<std::size_t>{0});
  REQUIRE(signal.head == 1);

  emg_rt::RingMatrix<float> extended(2, 2);
  std::vector<float> sums(2, 0.0F);

  emg_rt::dsp::incremental_extend(extended, signal, 2, sums, 2, 2);

  emg_rt::tests::expect_matrix_approx(extended, {
                                                    {5.0F, 6.0F},
                                                    {4.0F, 5.0F},
                                                });
}

TEST_CASE("incremental_extend updates rolling sums by removing old delayed samples") {
  auto signal = emg_rt::tests::matrix_from_rows<float>({
      {1.0F, 2.0F, 3.0F, 4.0F},
  });
  emg_rt::RingMatrix<float> extended(2, 1);
  std::vector<float> sums = {100.0F, 200.0F};

  emg_rt::dsp::incremental_extend(extended, signal, 2, sums, 1, 2);

  emg_rt::tests::expect_matrix_approx(extended, {
                                                    {4.0F},
                                                    {3.0F},
                                                });
  emg_rt::tests::expect_vector_approx(sums, {102.0F, 202.0F});
}
