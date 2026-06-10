#ifndef ISLOCALMAX_H
#define ISLOCALMAX_H

#include <mdspan>

void islocalmax(std::mdspan<float, std::dextents<std::size_t, 2>> src,
                std::mdspan<bool, std::dextents<std::size_t, 2>> dst,
                std::size_t min_peak_distance);

#endif
