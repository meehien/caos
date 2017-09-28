#include "TestStore.hpp"
#include "Log.hpp"

TestStore::TestStore(BlockStore *store):store(store) {
	cache = new std::unordered_map<size_t, blockData>();
}

blockData TestStore::Read(size_t pos) {
	if(cache->find(pos) != cache->end())
		return cache->find(pos)->second;
	else
		return store->Read(pos);
}

void TestStore::Write(size_t pos, blockData b){
	if(cache->find(pos) != cache->end())
		cache->erase(pos);

	cache->insert({pos, b});
}