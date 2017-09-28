#pragma once

#include "BlockStore.hpp"
#include <string>

class FileStore: public BlockStore {
	int fd;

	byte_t *map;
	size_t count, size;

	bool wasSerialised;

public:
	FileStore(std::string filename, size_t count, size_t size);
	~FileStore();

	blockData Read(size_t pos);
	void Write(size_t pos, blockData b);

	size_t GetBlockCount();
	size_t GetBlockSize();

	bool WasSerialised();
};
