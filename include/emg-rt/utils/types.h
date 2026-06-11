#ifndef EMG_RT_TYPES_H
#define EMG_RT_TYPES_H

#include <mdspan>

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

} // namespace emg_rt

#endif // EMG_RT_TYPES_H
