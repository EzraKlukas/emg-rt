#include "emg-rt/data_replay/offline_emg_source.h"

#include <vector>

using namespace std;

vector<uint64_t> generate_timestamps(size_t size) {
  vector<uint64_t> timestamps(size);

  for (size_t sample = 0; sample < size; ++sample) {
    timestamps[sample] = (uint64_t)sample;
  }

  return timestamps;
}
