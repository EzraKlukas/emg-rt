#include "emg-rt/buffer/signal_ring_buffer.h"
#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/decomposition/get_discharge_t.h"
#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/decomposition/is_local_max.h"
#include "emg-rt/decomposition/online_decomposer.h"
#include "emg-rt/dsp/demean.h"
#include "emg-rt/dsp/extend.h"
#include "emg-rt/utils/formatting.h"
#include "emg-rt/utils/types.h"

#include <iostream>

const std::string path_to_yaml = "offline_params/decomposition_config.yaml";

int main() {
  MultiGridDecomposer decompose = load_online_decomposer(path_to_yaml);
  std::cout << format_online_params(decompose) << std::flush;

  // have decomposition function / external file with function that essentially
  // contains the getonlinespikes function essentially.
}
