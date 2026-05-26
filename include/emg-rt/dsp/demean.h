#ifndef DEMEAN_H
#define DEMEAN_H

#include <mdspan>

void demean(std::mdspan<float, std::dextents<std::size_t, 2>> &signal);

#endif
