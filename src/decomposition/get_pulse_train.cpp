/*
 * Compute motor-unit pulse trains from extended EMG samples.
 *
 * This function implements the online equivalent of the MATLAB expression:
 *
 *   PulseT = ((MUfilt * esample) .* abs(MUfilt * esample)) ./ norm'
 *
 * The computation has three conceptual steps:
 *
 *   1. Project the extended EMG signal through the trained motor-unit filters:
 *
 *        projection = MUfilt * emg_buffer
 *
 *   2. Sharpen the projection using a signed-square-like nonlinearity:
 *
 *        projection .* abs(projection)
 *
 *      This preserves sign while emphasizing large filter responses.
 *
 *   3. Apply one normalization value per motor-unit filter.
 *
 * `RingMatrix` stores logical matrices in circular buffers, so the logical
 * columns being processed may wrap around the physical end of the underlying
 * storage. Eigen expects contiguous matrix blocks, so this function splits the
 * operation into physically contiguous chunks whenever either the input
 * extended signal or the output pulse-train buffer would wrap.
 *
 * The motor-unit filter matrix is viewed as:
 *
 *   filters x extended_channels
 *
 * and each processed chunk of extended EMG is viewed as:
 *
 *   extended_channels x chunk_samples
 *
 * producing a pulse-train chunk of:
 *
 *   filters x chunk_samples
 */

#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/profiling/timer.h"
#include "emg-rt/utils/types.h"

#include <Eigen/Core>

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

void decomp::get_pulse_train(RingMatrix<float> &pulse_train,
                             const RingMatrix<float> &emg_buffer,
                             MatrixView<float> mu_filters,
                             VectorView<float> inv_filter_norms) {
  EMG_RT_PROFILE(prof::Section::pulse_train);
  const std::size_t filters = mu_filters.extent(0);
  const std::size_t samples = emg_buffer.cols;
  const std::size_t extended_channels = emg_buffer.rows;

  assert(inv_filter_norms.extent(0) == filters);
  assert(mu_filters.extent(1) == extended_channels);
  assert(pulse_train.rows == filters);
  assert(pulse_train.cols >= samples);

  // mu_filters is already stored column-major by your MatrixView convention:
  // index = col * rows + row.
  ConstEigenMatrixMap W(mu_filters.data_handle(), ei(filters),
                        ei(extended_channels));

  /*
   * We want:
   *
   *   projection = W * emg_buffer
   *   pulse_train    = projection .* abs(projection) .* inv_filter_norms
   *
   * But pulse_train and emg_buffer are RingMatrix objects, so logical columns
   * may wrap around physically. We split the operation into physically
   * contiguous chunks.
   */
  std::size_t done = 0;

  while (done < samples) {
    const std::size_t x_phys_col = (emg_buffer.head + done) % emg_buffer.cols;

    const std::size_t y_phys_col = (pulse_train.head + done) % pulse_train.cols;

    const std::size_t x_cols_until_wrap = emg_buffer.cols - x_phys_col;

    const std::size_t y_cols_until_wrap = pulse_train.cols - y_phys_col;

    const std::size_t chunk = std::min({
        samples - done,
        x_cols_until_wrap,
        y_cols_until_wrap,
    });

    ConstEigenMatrixMap X(&emg_buffer.data[x_phys_col * extended_channels],
                          ei(extended_channels), ei(chunk));

    EigenMatrixMap Y(&pulse_train.data[y_phys_col * filters], ei(filters),
                     ei(chunk));

    Eigen::Map<const Eigen::VectorXf, Eigen::Unaligned> inv_norm_map(
        inv_filter_norms.data_handle(), ei(filters));

    // Y = W * X
    Y.noalias() = W * X;

    // Y = Y .* abs(Y)
    Y.array() = Y.array() * Y.array().abs();

    // Y(row, :) *= inv_filter_norm(row)
    Y.array().colwise() *= inv_norm_map.array();

    done += chunk;
  }
}
