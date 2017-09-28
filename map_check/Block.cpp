#include <stdlib.h>
#include <ctime>
#include <cstring>
#include "BlockStore.hpp"
#include "Block.hpp"
#include "Log.hpp"
#include "Types.hpp"

Block::Block(int64_t bid, int64_t cns, time_t chg, blockData data) { ;
	this->bid = bid;
	this->cns = cns;
	this->chg = chg;
	this->data = data;
}

Block::Block(blockData bytes) {
	memcpy(&bid, bytes.data(), sizeof(int64_t));
	memcpy(&cns, bytes.data() + sizeof(int64_t), sizeof(int64_t));
	memcpy(&chg, bytes.data() + 2*(sizeof(int64_t)), sizeof(time_t));
	data.resize(bytes.size() - (2*sizeof(int64_t)+ sizeof(time_t)));
	memcpy(data.data(), bytes.data() + (2*sizeof(int64_t)+ sizeof(time_t)), bytes.size() - (2*sizeof(int64_t)+ sizeof(time_t)));

}

blockData Block::toBytes() {
	blockData bytes(2*sizeof(int64_t)+data.size()+ sizeof(time_t));

	memcpy(bytes.data(), &bid, sizeof(int64_t));
	memcpy(bytes.data()+ sizeof(int64_t), &cns, sizeof(int64_t));
	memcpy(bytes.data()+ 2*sizeof(int64_t), &chg, sizeof(time_t));
	memcpy(bytes.data()+ 2*sizeof(int64_t)+sizeof(time_t), data.data(), data.size());

	return bytes;
}