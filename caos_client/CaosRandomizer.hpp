#pragma once

#include "Crypto.hpp"
#include "DataMap.hpp"
#include "BufferMap.hpp"

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>

using PositionMap = std::vector<int64_t>;
using BlockMap = std::unordered_map<int64_t, std::vector<size_t>>;
using ChangedMap = std::vector<byte_t>;

class CaosRandomizer {
	BlockStore *store;
	
	size_t blockCount, blockSize;

	bytes<Key> key;

	const int64_t NUM_CLIENTS;

	int64_t cont;

public:
	CaosRandomizer(BlockStore *store, size_t blockCount, size_t blockSize, bytes<Key> key, int64_t numClients, int64_t cont);
	~CaosRandomizer();

	bool Access(int64_t size);
	
	PositionMap LoadPositionMap();
	ChangedMap LoadChangedMap();

	size_t GetFreePosition(PositionMap &posMap, BlockMap &blockMap);
	size_t GetBlockPosition(int64_t bid, PositionMap &posMap, BlockMap &blockMap);
	void SavePositionMap(PositionMap posMap);

	void SaveChangedMap(ChangedMap changedMap);

	void SaveBuffer(std::ostream& os, BufferMap &buffer, size_t size);
	void LoadBuffer(std::istream& is, BufferMap &buffer);
	
	static BlockMap GenerateBlockMap(const PositionMap &posMap);
	
	size_t GetBlockCount();
	size_t GetBlockSize();
	size_t GetBlockDataSize();

	Block SyncPosition(int64_t bid, size_t pos, Block block, DataMap *map);
	Block DuplicateBlock(size_t pos, Block block, Block orgBlock, DataMap *map);
	
	bool WasSerialised();
};
