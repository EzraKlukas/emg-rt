/*
 * Generate replay timestamps for offline EMG data.
 *
 * The current replay path does not have hardware timestamps, so each sample is
 * assigned a monotonically increasing integer timestamp:
 *
 *   timestamps[i] = i
 *
 * This is sufficient for testing buffer movement and decomposition logic. It is
 * not a substitute for hardware-side timestamping in the final live acquisition
 * system.
 */

#include "emg-rt/data_replay/offline_emg_source.h"

#include <vector>

using namespace std;

vector<uint64_t> emg_rt::replay::generate_replay_timestamps(size_t size) {
  vector<uint64_t> timestamps(size);

  for (size_t sample = 0; sample < size; ++sample) {
    timestamps[sample] = (uint64_t)sample;
  }

  return timestamps;
}
