#include "emg-rt/decomposition/online_decomposer.h"
#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/decomposition/get_discharge_t.h"
#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/decomposition/is_local_max.h"
#include "emg-rt/dsp/demean.h"
#include "emg-rt/dsp/extend.h"
#include "emg-rt/utils/types.h"

#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

void GridDecomposer::decompose(OnlineDecompositionConfig &config) {
  auto &buf = buffers();
  incremental_extend(buf.ext_signal, buf.signal, ex_factor_, buf.sums,
                     config.samples_per_cycle, config.demean_window_size);
  incremental_demean(buf.ext_signal, config.demean_window_size, buf.sums);
  get_pulse_train(buf.pulse_t, buf.ext_signal, mu_filters_view(),
                  filter_norms_view());
  is_local_max(buf.pulse_t, buf.spikes, config.min_peak_distance);
  get_distime(buf.discharges, buf.pulse_t, buf.spikes, noise_centroids_view(),
              spike_centroids_view());
}
