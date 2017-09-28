#pragma once

#include "Types.hpp"
#include "Block.hpp"

#include <vector>

using blockData = std::vector<byte_t>;

/*
 * An abstract class for interfacing with
 * various different block devices
 */
class BlockStore {
public:
	virtual ~BlockStore() = default;

	virtual void Write(size_t pos, blockData b) = 0;
	virtual blockData Read(size_t pos) = 0;

	virtual size_t GetBlockCount() = 0;
	virtual size_t GetBlockSize() = 0;

	virtual bool WasSerialised() = 0;
};
