#ifndef CaosCLIENT_BLOCK_H
#define CaosCLIENT_BLOCK_H


#include <cinttypes>
#include <vector>
#include "BlockStore.hpp"

using blockData = std::vector<byte_t>;

class Block {
public:
	int64_t bid;
	int64_t cns;
	time_t chg;
	blockData data;

	Block() = default;
	Block(int64_t bid, int64_t cns, time_t chg, blockData data);
	Block(blockData bytes);
	~Block() = default;
	blockData toBytes();

	static unsigned long getSerializedSize(unsigned long dataSize){
		return dataSize + (2* sizeof(int64_t)) + sizeof(time_t);
	};
};


#endif //CaosCLIENT_BLOCK_H
