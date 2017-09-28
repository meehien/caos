#pragma once

#include "BlockStore.hpp"

class RAMStore: public BlockStore {
	std::vector<block> store;
	size_t size;

public:
	RAMStore(size_t num, size_t size);
	~RAMStore();

	block Read(size_t pos);
	void Write(size_t pos, block b);

	size_t GetBlockCount();
	size_t GetBlockSize();

	bool WasSerialised();
};
