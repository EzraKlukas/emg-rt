#ifndef EXTEND_H
#define EXTEND_H

#include <mdspan>

void extend(std::mdspan<float, std::dextents<std::size_t, 2>> &ext_signal,
            std::mdspan<float, std::dextents<std::size_t, 2>> &signal,
            std::size_t ex_factor);

#endif
