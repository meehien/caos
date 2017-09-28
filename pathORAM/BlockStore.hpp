#pragma once

#include "Types.hpp"

#include <vector>

using block = std::vector<byte_t>;

/*
 * An abstract class for interfacing with
 * various different block devices
 */
class BlockStore {
public:
	virtual ~BlockStore() = default;

	virtual block Read(size_t pos) = 0;
	virtual void Write(size_t pos, block b) = 0;

	virtual size_t GetBlockCount() = 0;
	virtual size_t GetBlockSize() = 0;

	virtual bool WasSerialised() = 0;
};
