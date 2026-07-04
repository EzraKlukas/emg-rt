#include "emg-rt/utils/types.h"

struct MotorState {
  const int32_t actual_position;
  const int32_t actual_velocity;
  const int16_t actual_torque;
  const int32_t following_error;
  int32_t target_position;
  int32_t target_velocity;
  int16_t target_torque;
};

class EmgToMotor {
public:
  EmgToMotor() {};

  void emg_to_motor(MotorState &motor_state_src,
                    emg_rt::RingMatrix<float> &pulse_train,
                    std::size_t new_samples);
};
