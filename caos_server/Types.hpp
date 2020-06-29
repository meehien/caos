#pragma once

#include <array>
#include <inttypes.h>
#include <stdlib.h>

// The main type for passing around raw file data
using byte_t = uint8_t;

template <size_t N>
using bytes = std::array<byte_t, N>;
