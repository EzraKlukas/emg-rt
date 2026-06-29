#include "emg-rt/buffer/signal_ring_buffer.h"
#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/data_replay/offline_emg_source.h"
#include "emg-rt/decomposition/online_decomposer.h"
#include "emg-rt/profiling/timer.h"
#include <iostream>

const std::string path_to_yaml = "offline_params/decomposition_config.yaml";
const std::string path_to_sig = "offline_data/emg.bin";

int main(int argc, char *argv[]) {
  std::size_t num_decomp_cycles = 0;
  if (argc >= 2) {
    std::string str_arg_view(argv[1]);
    num_decomp_cycles = static_cast<std::size_t>(std::stoull(str_arg_view));
  }

  if (argc >= 3) {
      std::string str_arg_view(argv[2]);
      emg_rt::prof::histogram_bins = static_cast<std::size_t>(std::stoull(str_arg_view));
  }

  if (argc >= 4) {
      std::string str_arg_view(argv[3]);
      emg_rt::prof::ns_per_bin = static_cast<std::size_t>(std::stoull(str_arg_view));
  }

  MultiGridDecomposer decompose = load_online_decomposer(path_to_yaml);
  // std::cout << format_online_params(decompose) << std::flush;

  SignalRingBuffer live_signal =
      SignalRingBuffer(decompose.config().demean_window_size * 2,
                       decompose.grids().size() * channels_per_grid);

  std::vector<uint16_t> signal_source =
      pull_samples_from_bin<uint16_t>(path_to_sig);
  std::vector<uint64_t> timestamps =
      generate_timestamps(signal_source.size() / live_signal.num_channels());

  // fully load live_signal initially and then perform decompose.init_grids.
  live_signal.write_samples(live_signal.size(), timestamps.data(),
                            signal_source.data());
  decompose.init_grids(live_signal);

  if (num_decomp_cycles == 0) {
    num_decomp_cycles = timestamps.size() - live_signal.size();
  }

  for (size_t emg_count = live_signal.size();
       emg_count < live_signal.size() + num_decomp_cycles;
       emg_count = emg_count + decompose.config().samples_per_cycle) {
    emg_rt::prof::ScopedTimer cycle_timer(emg_rt::prof::Section::cycle);

    live_signal.write_samples(
        decompose.config().samples_per_cycle, &timestamps[emg_count],
        &signal_source[emg_count * live_signal.num_channels()]);
    decompose.get_samples(live_signal, decompose.config().samples_per_cycle);
    if (emg_count - live_signal.size() <
        decompose.config().min_lookback_samps) {
      decompose.init_pulse_hist();
    } else {
      decompose.decompose();
    }
  }
  std::cout << emg_rt::prof::summarize_stats();
}
