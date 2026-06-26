#ifndef DEBUG_VIEWS_H
#define DEBUG_VIEWS_H

#include <cstddef>
#include <mdspan>

#include "types.h"

#if defined(__GNUC__) || defined(__clang__)
#define USERSPACE_NOINLINE __attribute__((noinline))
#define USERSPACE_USED __attribute__((used))
#else
#define USERSPACE_NOINLINE
#define USERSPACE_USED
#endif

namespace userspace::debug {

// ---------------- MatrixView<T> ----------------

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED T *matrix_data(emg_rt::MatrixView<T> view) {
  return view.data_handle();
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED const T *
matrix_data(emg_rt::ConstMatrixView<T> view) {
  return view.data_handle();
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED std::size_t
matrix_rows(emg_rt::MatrixView<T> view) {
  return view.extent(0);
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED std::size_t
matrix_rows(emg_rt::ConstMatrixView<T> view) {
  return view.extent(0);
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED std::size_t
matrix_cols(emg_rt::MatrixView<T> view) {
  return view.extent(1);
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED std::size_t
matrix_cols(emg_rt::ConstMatrixView<T> view) {
  return view.extent(1);
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED std::size_t
matrix_index(emg_rt::MatrixView<T> view, std::size_t row, std::size_t col) {
  return view.mapping()(row, col);
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED std::size_t
matrix_index(emg_rt::ConstMatrixView<T> view, std::size_t row,
             std::size_t col) {
  return view.mapping()(row, col);
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED T &
matrix_at(emg_rt::MatrixView<T> view, std::size_t row, std::size_t col) {
  return view.data_handle()[view.mapping()(row, col)];
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED const T &
matrix_at(emg_rt::ConstMatrixView<T> view, std::size_t row, std::size_t col) {
  return view.data_handle()[view.mapping()(row, col)];
}

// ---------------- VectorView<T> ----------------

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED T *vector_data(emg_rt::VectorView<T> view) {
  return view.data_handle();
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED const T *
vector_data(emg_rt::ConstVectorView<T> view) {
  return view.data_handle();
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED std::size_t
vector_size(emg_rt::VectorView<T> view) {
  return view.extent(0);
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED std::size_t
vector_size(emg_rt::ConstVectorView<T> view) {
  return view.extent(0);
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED T &vector_at(emg_rt::VectorView<T> view,
                                               std::size_t i) {
  return view.data_handle()[view.mapping()(i)];
}

template <typename T>
USERSPACE_NOINLINE USERSPACE_USED const T &
vector_at(emg_rt::ConstVectorView<T> view, std::size_t i) {
  return view.data_handle()[view.mapping()(i)];
}

} // namespace userspace::debug

#undef USERSPACE_NOINLINE
#undef USERSPACE_USED

#endif
