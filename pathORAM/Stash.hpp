#pragma once

#include "BlockStore.hpp"

#include <unordered_map>
#include <cstdint>

using Stash = std::unordered_map<int32_t, block>;

struct StashHelper {
	static bool Load(std::string filename, Stash &stash, size_t blockSize);
	static void Save(std::string filename, Stash &stash, size_t blockSize);
};
