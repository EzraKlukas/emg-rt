/*
 * Offline replay entry point for the real-time EMG decomposition pipeline.
 *
 * This executable currently uses recorded EMG data from disk instead of live
 * sensor input. The purpose is to exercise the same online pipeline that will
 * eventually run on the Jetson with live acquisition.
 *
 * The important architectural boundary is the acquisition buffer. Offline
 * replay writes into an `AcquisitionRingBuffer`; live acquisition is expected
 * to meet the decomposition code at the same boundary.
 *
 * The broader acquisition model also includes `SensorBufferConfig` and
 * `AcquisitionFrameBuffer`, which can own EMG, IMU, and ADC rings in parallel.
 * This replay harness currently constructs only the EMG ring used by the
 * decomposer.
 *
 * Offline replay and live acquisition should meet at this same interface:
 *
 *   1. A producer writes timestamped raw samples into an acquisition ring.
 *      Today, that producer is `pull_samples_from_bin`. Later, it should be the
 *      sensor/acquisition thread.
 *
 *   2. The decomposer reads indexed samples out of the acquisition ring.
 *      From this point onward, the pipeline should not care whether the
 *      samples came from a file or from hardware.
 *
 * The program flow is:
 *
 *   - Load decomposition parameters and trained motor-unit filters from YAML.
 *   - Allocate a raw sample ring buffer large enough to hold the recent history
 *     needed for demeaning and temporal extension.
 *   - Load offline EMG samples and generate replay timestamps.
 *   - Pre-fill the raw acquisition ring so the decomposer has enough history
 * before the first cycle.
 *   - Initialize each grid's internal decomposition workspace from the newest
 *     samples in the raw sample ring.
 *   - Repeatedly write new samples into the ring and run one online
 *     decomposition cycle.
 *
 * The command-line arguments are currently used for profiling experiments:
 *
 *   argv[1] : number of times to replay the offline data
 *   argv[2] : number of profiling histogram bins
 *   argv[3] : nanoseconds per profiling histogram bin
 *
 * This file is best understood as a temporary replay harness around the real
 * online decomposition code, not as the final live acquisition system. It does
 * not implement hardware timestamping, thread synchronization, or multi-rate
 * sensor alignment.
 */

#include "emg-rt/buffer/acquisition_ring_buffer.h"
#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/data_replay/offline_emg_source.h"
#include "emg-rt/decomposition/online_decomposer.h"
#include "emg-rt/profiling/timer.h"
#include <iostream>

const std::string path_to_yaml = "offline_params/decomposition_config.yaml";
const std::string path_to_sig = "offline_data/emg.bin";

const std::size_t buffer_duration_ms = 200;

using namespace emg_rt;

int main(int argc, char *argv[]) {
  // Process commandline arguments
  // num_data_replays: number of times to replay all the given offline data in a
  // test.
  size_t num_data_replays = 10;
  if (argc >= 2) {
    std::string str_arg_view(argv[1]);
    num_data_replays = static_cast<std::size_t>(std::stoull(str_arg_view));
  }

  if (argc >= 3) {
    std::string str_arg_view(argv[2]);
    prof::histogram_bins = static_cast<std::size_t>(std::stoull(str_arg_view));
  }

  if (argc >= 4) {
    std::string str_arg_view(argv[3]);
    prof::ns_per_bin = static_cast<std::size_t>(std::stoull(str_arg_view));
  }

  MultiGridDecomposer decompose = load_online_decomposer(path_to_yaml);
  // std::cout << format_online_params(decompose) << std::flush;

  buffer::SensorBufferConfig sensor_cfg = buffer::SensorBufferConfig(
      buffer::SensorType::EMG, decompose.config().sampling_frequency,
      decompose.grids().size(), buffer::chan_per_emg, buffer_duration_ms);

  buffer::AcquisitionRingBuffer acquisition_buffer =
      buffer::AcquisitionRingBuffer(sensor_cfg);

  std::vector<uint16_t> offline_raw_samples =
      replay::pull_samples_from_bin<uint16_t>(path_to_sig);
  std::vector<uint64_t> timestamps = replay::generate_replay_timestamps(
      offline_raw_samples.size() / acquisition_buffer.num_streams());

  // Pre-fill the acquisition ring before initializing grid workspaces.
  acquisition_buffer.write_samples(acquisition_buffer.size(), timestamps.data(),
                                   offline_raw_samples.data());
  decompose.init_grids(acquisition_buffer);

  std::size_t num_decomp_cycles = 0;
  if (num_decomp_cycles == 0) {
    num_decomp_cycles = timestamps.size() - acquisition_buffer.size();
  }

  for (size_t data_replay_count = 0; data_replay_count < num_data_replays;
       ++data_replay_count) {
    for (size_t emg_count = acquisition_buffer.size();
         emg_count < acquisition_buffer.size() + num_decomp_cycles;
         emg_count = emg_count + decompose.config().samples_per_cycle) {
      EMG_RT_PROFILE(prof::Section::cycle);

      acquisition_buffer.write_samples(
          decompose.config().samples_per_cycle, &timestamps[emg_count],
          &offline_raw_samples[emg_count * acquisition_buffer.num_streams()]);
      decompose.read_samples(acquisition_buffer,
                             decompose.config().samples_per_cycle);
      if (emg_count - acquisition_buffer.size() <
          decompose.config().min_lookback_samps) {
        decompose.init_pulse_hist();
      } else {
        decompose.decompose();
      }
    }
  }
#ifdef EMG_RT_ENABLE_PROFILING
  std::ofstream out("profiling_summary.txt");
  std::cout << prof::summarize_stats();
#endif
}
