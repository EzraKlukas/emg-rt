#ifndef PULSET_H
#define PULSET_H

#include <mdspan>

void get_pulse_train(
    std::mdspan<float, std::dextents<std::size_t, 2>> pulse_t,
    std::mdspan<float, std::dextents<std::size_t, 2>> emg_buffer,
    std::mdspan<float, std::dextents<std::size_t, 2>> mu_filters,
    std::mdspan<float, std::dextents<std::size_t, 1>> norm);

#endif
