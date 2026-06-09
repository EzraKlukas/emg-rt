#ifndef DISTIME_H
#define DISTIME_H

#include <mdspan>
#include <vector>

void get_distime(std::vector<std::vector<std::size_t>> discharge_times,
                 std::mdspan<float, std::dextents<std::size_t, 2>> pulse_t,
                 std::mdspan<bool, std::dextents<std::size_t, 2>> spikes,
                 const std::vector<float> &noise_centroids,
                 const std::vector<float> &spike_centroids);

#endif
