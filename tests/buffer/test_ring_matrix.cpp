#include "emg-rt/utils/types.h"

#include "support/matrix_helpers.h"

#include <doctest/doctest.h>

TEST_CASE("RingMatrix preserves logical order before wraparound") {
  const auto matrix = emg_rt::tests::matrix_from_rows<int>({
      {1, 2, 3},
      {10, 20, 30},
  });

  CHECK(matrix.head == 0);
  emg_rt::tests::expect_matrix_eq<int>(matrix, {
                                                   {1, 2, 3},
                                                   {10, 20, 30},
                                               });
}

TEST_CASE("RingMatrix exact-capacity writes leave logical order contiguous") {
  emg_rt::RingMatrix<int> matrix(2, 3);

  const int col0[] = {1, 10};
  const int col1[] = {2, 20};
  const int col2[] = {3, 30};

  matrix.write_column(col0);
  matrix.write_column(col1);
  matrix.write_column(col2);

  CHECK(matrix.head == 0);
  emg_rt::tests::expect_matrix_eq<int>(matrix, {
                                                   {1, 2, 3},
                                                   {10, 20, 30},
                                               });
}

TEST_CASE("RingMatrix returns newest capacity after multiple overwrites") {
  emg_rt::RingMatrix<int> matrix(2, 3);

  const int col0[] = {1, 10};
  const int col1[] = {2, 20};
  const int col2[] = {3, 30};
  const int col3[] = {4, 40};
  const int col4[] = {5, 50};

  matrix.write_column(col0);
  matrix.write_column(col1);
  matrix.write_column(col2);
  matrix.write_column(col3);
  matrix.write_column(col4);

  CHECK(matrix.head == 2);
  emg_rt::tests::expect_matrix_eq<int>(matrix, {
                                                   {3, 4, 5},
                                                   {30, 40, 50},
                                               });

  CHECK(matrix.data[0] == 4);
  CHECK(matrix.data[2] == 5);
  CHECK(matrix.data[4] == 3);
}

TEST_CASE("RingVector insert wraps logical indexing independently of storage") {
  emg_rt::RingVector<int> vector(3);

  vector.insert(10);
  vector.insert(20);
  vector.insert(30);
  vector.insert(40);
  vector.insert(50);

  CHECK(vector.head == 2);
  CHECK(vector(0) == 30);
  CHECK(vector(1) == 40);
  CHECK(vector(2) == 50);
}
