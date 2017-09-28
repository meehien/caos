#pragma once

#include <array>

// The main type for passing around raw file data
using byte_t = uint8_t;

template <size_t N>
using bytes = std::array<byte_t, N>;
