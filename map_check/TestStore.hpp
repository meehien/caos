#ifndef MAPCHECK_TESTSTORE_HPP
#define MAPCHECK_TESTSTORE_HPP


#include <unordered_map>
#include "BlockStore.hpp"
using BlockCache = std::unordered_map<size_t, blockData>;

class TestStore {
	BlockStore *store;
	BlockCache *cache;
public:
	TestStore(BlockStore *store);
	~TestStore() = default;
	blockData Read(size_t pos);
	void Write(size_t pos, blockData block);
};


#endif //MAPCHECK_TESTSTORE_HPP
