/*
 * Contiguous matrix, vector, and ring-buffer utility types.
 *
 * The decomposition code stores numeric data in `std::vector` and exposes it
 * through lightweight views. `MatrixView` and `VectorView` are non-owning; the
 * caller must keep the underlying storage alive and avoid concurrent mutation
 * without synchronization.
 *
 * `RingMatrix` and `RingVector` own fixed-size circular windows. Logical index
 * zero is the oldest retained element and `head` points at its physical
 * location. `RingMatrix` stores column-major data so one logical sample column
 * is contiguous in memory.
 *
 * `RingMatrix::write_column(column, mask)` appends one logical column by
 * gathering from source stream indices: destination row `stream` receives
 * `column[mask[stream]]`. The mask is therefore an index map, not a boolean
 * mask. After writing, the ring head advances and the oldest logical column is
 * overwritten.
 *
 * These types allocate in their constructors and do not allocate during indexed
 * access or column writes, but they do not provide thread safety.
 */

#ifndef EMG_RT_TYPES_H
#define EMG_RT_TYPES_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace emg_rt {

template <typename T> struct MatrixView {
  T *ptr = nullptr;
  std::size_t rows = 0;
  std::size_t cols = 0;

  MatrixView() = default;

  MatrixView(T *data, std::size_t rows, std::size_t cols)
      : ptr(data), rows(rows), cols(cols) {}

  T &operator()(std::size_t row, std::size_t col) const {
    // column-major: index = col * rows + row
    return ptr[(col * rows) + row];
  }

  T *data_handle() const { return ptr; }

  std::size_t extent(std::size_t dim) const { return dim == 0 ? rows : cols; }

  std::size_t size() const { return rows * cols; }
};

template <typename T> struct VectorView {
  T *ptr = nullptr;
  std::size_t n = 0;

  VectorView() = default;

  VectorView(T *data, std::size_t n) : ptr(data), n(n) {}

  T &operator()(std::size_t i) const { return ptr[i]; }

  T *data_handle() const { return ptr; }

  std::size_t extent(std::size_t dim) const { return dim == 0 ? n : 1; }

  std::size_t size() const { return n; }
};

template <typename T> using ConstMatrixView = MatrixView<const T>;

template <typename T> using ConstVectorView = VectorView<const T>;

template <typename T> struct RingMatrix {
  std::size_t rows;
  std::size_t cols;     // window length
  std::size_t head = 0; // physical column for logical col 0, the oldest sample

  std::vector<T> data; // column-major: cols * rows

  RingMatrix(std::size_t rows, std::size_t cols)
      : rows(rows), cols(cols), data(rows * cols) {}

  typename std::vector<T>::reference operator()(std::size_t row,
                                                std::size_t logical_col) {
    return data[(((head + logical_col) % cols) * rows) + row];
  }

  typename std::vector<T>::const_reference
  operator()(std::size_t row, std::size_t logical_col) const {
    return data[(((head + logical_col) % cols) * rows) + row];
  }

  // Append one gathered column on the right, overwriting the oldest column.
  void write_column(const T *column, const std::vector<std::size_t> &mask) {
    std::size_t write_col = head; // overwrite oldest physical column

    T *dst_col = &data[write_col * rows];

    for (std::size_t stream = 0; stream < mask.size(); ++stream) {
      dst_col[stream] = column[mask[stream]];
    }

    increment_head();
  }

private:
  std::size_t increment_head() {
    if (++head == cols) {
      head = 0;
    }
    return head;
  }
};

template <typename T> struct RingVector {
  std::size_t size;     // window length
  std::size_t head = 0; // physical column for logical idx 0, the oldest sample

  std::vector<T> data; // column-major: cols * rows

  RingVector(std::size_t size) : size(size), data(size) {}

  typename std::vector<T>::reference operator()(std::size_t logical_idx) {
    return data[(head + logical_idx) % size];
  }

  void insert(T value) {
    if (head == size) {
      head -= size;
    }
    data[head++] = value;
  }
};

inline std::size_t abs_diff(std::size_t a, std::size_t b) {
  return (a > b) ? (a - b) : (b - a);
}

} // namespace emg_rt

#endif // EMG_RT_TYPES_H
