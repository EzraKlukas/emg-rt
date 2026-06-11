#include "emg-rt/config/decomposition_config.h"
#include "emg-rt/decomposition/get_discharge_t.h"
#include "emg-rt/decomposition/get_pulse_train.h"
#include "emg-rt/decomposition/is_local_max.h"
#include "emg-rt/dsp/demean.h"
#include "emg-rt/dsp/extend.h"
#include "emg-rt/utils/formatting.h"
#include "emg-rt/utils/types.h"

#include <iostream>

const std::string path_to_yaml = "offline_params/decomposition_config.yaml";

int main() {
  DecompositionParams config_params = get_online_params(path_to_yaml);
  config_params.grids[0].validate_dimensions();
  std::cout << format_online_params(config_params) << std::flush;
}
