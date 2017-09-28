#ifndef MAPCHECK_MAPCHECK_HPP
#define MAPCHECK_MAPCHECK_HPP


#include "NetworkStore.hpp"
#include "DataMap.hpp"
#include "FileStore.hpp"
#include "Crypto.hpp"

class MapCheck {
	BlockStore *store;
	std::string fileName;
	size_t blockCount, blockSize;
	bytes<Key> key;
	std::vector<size_t> *knownPoss;
	int changed;

	double Validity(int64_t bid, DataMap *map);
	int ListUnmapped(int64_t bid, DataMap *map);
	double HitRate(int64_t bid);

	Block DuplicateBlock(size_t pos, Block block, Block orgBlock, DataMap *map);
	Block SyncPosition(int64_t bid, size_t pos, Block block, DataMap *map);
public:
	MapCheck(BlockStore *store, std::string fileName, size_t blockCount, size_t blockSize, bytes<Key> key);
	void Check();
};


#endif //MAPCHECK_MAPCHECK_HPP
