#include "emg-rt/utils/types.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

#include <cstddef>
#include <vector>

TEST_CASE("MatrixView uses documented column-major indexing") {
  std::vector<float> storage = {
      1.0F, 10.0F,
      2.0F, 20.0F,
      3.0F, 30.0F,
  };
  emg_rt::MatrixView<float> view(storage.data(), 2, 3);

  CHECK(view(0, 0) == doctest::Approx(1.0F));
  CHECK(view(1, 0) == doctest::Approx(10.0F));
  CHECK(view(0, 1) == doctest::Approx(2.0F));
  CHECK(view(1, 2) == doctest::Approx(30.0F));
}

TEST_CASE("RingMatrix write_column gathers by source index and advances head") {
  auto matrix = emg_rt::tests::matrix_from_rows<int>({
      {1, 2, 3},
      {10, 20, 30},
  });

  const int first_source[] = {100, 101, 102, 103};
  matrix.write_column(first_source, std::vector<std::size_t>{3, 1});

  CHECK(matrix.head == 1);
  emg_rt::tests::expect_matrix_eq(matrix, {
                                              {2, 3, 103},
                                              {20, 30, 101},
                                          });

  const int second_source[] = {200, 201, 202, 203};
  matrix.write_column(second_source, std::vector<std::size_t>{0, 2});

  CHECK(matrix.head == 2);
  emg_rt::tests::expect_matrix_eq(matrix, {
                                              {3, 103, 200},
                                              {30, 101, 202},
                                          });
}

TEST_CASE("RingVector insert preserves logical oldest-to-newest order") {
  emg_rt::RingVector<int> vector(3);

  vector.insert(1);
  vector.insert(2);
  vector.insert(3);
  emg_rt::tests::expect_vector_eq(vector, {1, 2, 3});

  vector.insert(4);
  emg_rt::tests::expect_vector_eq(vector, {2, 3, 4});

  vector.insert(5);
  emg_rt::tests::expect_vector_eq(vector, {3, 4, 5});
}
