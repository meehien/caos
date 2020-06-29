#pragma once

#include "BlockStore.hpp"
#include <string>

class FileStore: public BlockStore {
	int fd;

	byte_t *map;
	size_t count, size;

	bool wasSerialised;
	std::vector<size_t> *posLock;

public:
	FileStore(std::string filename, size_t count, size_t size);
	~FileStore();

	block Read(size_t pos);
	void Write(size_t pos, block b);

	size_t GetBlockCount();
	size_t GetBlockSize();

	bool WasSerialised();

	bool AquireLock(size_t pos);
	void ReleaseLock(size_t pos);
};
