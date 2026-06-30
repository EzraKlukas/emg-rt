//******************************************************************************
// Performs matrix multiplication and element-wise division to obtain pulse
// train, implementing matlab line:
// PulseT = ((MUfilt * esample) .* abs(MUfilt * esample)) ./ norm';
//
// Input:
// pulse_t: pulse train (dimensions M x N)
// emg_buffer: extended EMG signal (dimensions CK x N)
// mu_filters: motor unit filters (dimensions M x CK)
// norm: encodes filter-wise norm (dimension M x 1);
//
// Output: void
//******************************************************************************

#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/profiling/timer.h"
#include "emg-rt/utils/types.h"

#include <eigen3/Eigen/Core>

#include <algorithm>
#include <cassert>
#include <cstddef>

using namespace emg_rt;

namespace {

using EigenMatrix =
    Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor>;

using ConstEigenMatrixMap = Eigen::Map<const EigenMatrix, Eigen::Unaligned>;

using EigenMatrixMap = Eigen::Map<EigenMatrix, Eigen::Unaligned>;

inline Eigen::Index ei(std::size_t x) { return static_cast<Eigen::Index>(x); }

} // namespace

void get_pulse_train(RingMatrix<float> &pulse_t,
                     const RingMatrix<float> &emg_buffer,
                     MatrixView<float> mu_filters, VectorView<float> norm) {
  const std::size_t filters = mu_filters.extent(0);
  const std::size_t samples = emg_buffer.cols;
  const std::size_t extended_channels = emg_buffer.rows;

  assert(norm.extent(0) == filters);
  assert(mu_filters.extent(1) == extended_channels);
  assert(pulse_t.rows == filters);
  assert(pulse_t.cols >= samples);

  emg_rt::prof::ScopedTimer pulse_train_timer(
      emg_rt::prof::Section::pulse_train);

  // mu_filters is already stored column-major by your MatrixView convention:
  // index = col * rows + row.
  ConstEigenMatrixMap W(mu_filters.data_handle(), ei(filters),
                        ei(extended_channels));

  /*
   * We want:
   *
   *   projection = W * emg_buffer
   *   pulse_t    = projection .* abs(projection) ./ norm
   *
   * But pulse_t and emg_buffer are RingMatrix objects, so logical columns
   * may wrap around physically. We split the operation into physically
   * contiguous chunks.
   */
  std::size_t done = 0;

  while (done < samples) {
    const std::size_t x_phys_col = (emg_buffer.head + done) % emg_buffer.cols;

    const std::size_t y_phys_col = (pulse_t.head + done) % pulse_t.cols;

    const std::size_t x_cols_until_wrap = emg_buffer.cols - x_phys_col;

    const std::size_t y_cols_until_wrap = pulse_t.cols - y_phys_col;

    const std::size_t chunk = std::min({
        samples - done,
        x_cols_until_wrap,
        y_cols_until_wrap,
    });

    ConstEigenMatrixMap X(&emg_buffer.data[x_phys_col * extended_channels],
                          ei(extended_channels), ei(chunk));

    EigenMatrixMap Y(&pulse_t.data[y_phys_col * filters], ei(filters),
                     ei(chunk));

    // Y = W * X
    Y.noalias() = W * X;

    // Y = Y .* abs(Y)
    Y.array() = Y.array() * Y.array().abs();

    // Y(row, :) /= norm(row)
    for (std::size_t filter = 0; filter < filters; ++filter) {
      Y.row(ei(filter)) *= norm(filter);
    }

    done += chunk;
  }
}
