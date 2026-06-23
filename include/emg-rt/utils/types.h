#ifndef EMG_RT_TYPES_H
#define EMG_RT_TYPES_H

#include <cstddef>
#include <mdspan>
#include <span>
#include <vector>

/*
 * Strictly necessary for decomposition because it defines the type that is the
 * application interface for all dealing with matrices and vectors that are
 * underneath stored as 1D contiguous std::vectors.
 */

namespace emg_rt {

template <typename T>
using MatrixView = std::mdspan<T, std::dextents<std::size_t, 2>>;

template <typename T>
using ConstMatrixView = std::mdspan<const T, std::dextents<std::size_t, 2>>;

template <typename T>
using VectorView = std::mdspan<T, std::dextents<std::size_t, 1>>;

template <typename T>
using ConstVectorView = std::mdspan<const T, std::dextents<std::size_t, 1>>;

template <typename T> struct RingMatrix {
  std::size_t rows;
  std::size_t cols;     // window length
  std::size_t head = 0; // physical column for logical col 0, the oldest sample

  std::vector<T> data; // column-major: cols * rows

  RingMatrix(std::size_t rows, std::size_t cols)
      : rows(rows), cols(cols), data(rows * cols) {}

  typename std::vector<T>::reference operator[](std::size_t row,
                                                std::size_t logical_col) {
    return data[(((head + logical_col) % cols) * rows) + row];
  }

  typename std::vector<T>::const_reference
  operator[](std::size_t row, std::size_t logical_col) const {
    return data[(((head + logical_col) % cols) * rows) + row];
  }

  // Append one new column on the right, overwriting the oldest column.
  void push_column(std::span<const T> column) {
    std::size_t write_col = head; // overwrite oldest physical column

    for (std::size_t row = 0; row < rows; ++row) {
      data[(write_col * rows) + row] = column[row];
    }

    head = (head + 1) % cols;
  }
};

template <typename T> struct RingVector {
  std::size_t size;     // window length
  std::size_t head = 0; // physical column for logical idx 0, the oldest sample

  std::vector<T> data; // column-major: cols * rows

  RingVector(std::size_t size) : size(size), data(size) {}

  typename std::vector<T>::reference operator[](std::size_t logical_idx) {
    return data[(head + logical_idx) % size];
  }

  void insert(T value) {
    if (head == size) {
      head -= size;
    }
    data[head++] = value;
  }
};

} // namespace emg_rt

#endif // EMG_RT_TYPES_H
