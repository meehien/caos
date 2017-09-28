#pragma once

#include "BlockStore.hpp"
#include "Crypto.hpp"
#include "Block.hpp"
#include "DataMap.hpp"
#include "Types.hpp"
#include "CaosClient.hpp"

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>

using PositionMap = std::vector<int64_t>;
using BlockMap = std::unordered_map<int64_t, std::vector<size_t>>;
using ChangedMap = std::vector<byte_t>;

class DataMap;

class CaosWClient : public CaosClient{ //TODO: removed blockstore inherit, ok?
	BlockStore *store;
	
	size_t blockCount, blockSize, clen, plen;
	const int64_t NUM_CLIENTS;

	int64_t cont;

	bytes<Key> key;

public:
	CaosWClient(BlockStore *store, size_t blockCount, size_t blockSize, bytes<Key> key, int64_t numClients, int64_t cont);
	~CaosWClient();

	enum Op {
		READ,
		WRITE
	};

	Block Access(Op op, int64_t bid, blockData data, DataMap *map);

	Block Read(size_t bid, DataMap *map);
	void Write(size_t bid, blockData b, DataMap *map);

	size_t GetBlockCount();
	size_t GetBlockSize();
	size_t GetBlockDataSize();
	
	bool WasSerialised();

	bool AquireFileLock(std::string filename);
	void ReleaseFileLock(std::string filename);

	Block SyncPosition(int64_t bid, size_t pos, Block block, DataMap *map);
	Block DuplicateBlock(size_t pos, Block block, Block orgBlock, DataMap *map);
	Block WriteBlock(blockData blockdata, int64_t bid, size_t pos, Block block, DataMap *map);

};
