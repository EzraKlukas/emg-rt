#ifndef EXTEND_H
#define EXTEND_H

#include <cstdint>
#include <mdspan>

void extend(std::mdspan<float, std::dextents<std::size_t, 2>> ext_signal,
            std::mdspan<float, std::dextents<std::size_t, 2>> signal,
            uint_fast16_t extension_t_ms, uint_fast16_t f_samp);

#endif
