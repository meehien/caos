#pragma once

#include <string>

struct PositionHelper {
	static bool Load(std::string filename, int *position, size_t size);
	static void Save(std::string filename, int *position, size_t size);
};
