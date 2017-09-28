#include "CaosWClient.hpp"
#include "FileStore.hpp"
#include "File.hpp"
#include "Log.hpp"
#include "Block.hpp"
#include "BlockStore.hpp"
#include "DataMap.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cinttypes>
#include <cassert>

CaosWClient::CaosWClient(BlockStore *store, size_t blockCount, size_t blockSize, bytes<Key> key, int64_t numClients, int64_t cont)
: store(store), blockCount(blockCount), blockSize(blockSize), key(key), NUM_CLIENTS(numClients), cont(cont)
{
	plen = GetBlockSize();
	clen = IV + AES::GetCiphertextLength(plen);
}

CaosWClient::~CaosWClient()
{
	Log::Write(Log::DEBUG, "Unloaded Client");
}

Block CaosWClient::Access(Op op, int64_t bid1, blockData blockdata, DataMap *map)
{
	size_t p1 = map->GetRandBlockPosition(bid1);
	size_t p2 = AES::GetRandom(GetBlockCount());

	int ibc = static_cast<int>(GetBlockCount());
	int64_t bid2 = -1;
	for(int i = 0; i < ibc; i++){
		std::vector<size_t> blockPositions = map->Positions(i);
		if(std::find(blockPositions.begin(), blockPositions.end(),p2)!=blockPositions.end()) {
			bid2 = i;
			break;
		}
	}

	Block blk1, blk2, syncb1, syncb2, wblk1, wblk2;

	bool read_first = AES::GetRandom(2);

	if(read_first){
		blk1 = Block(AES::Decrypt(key, store->Read(p1), GetBlockDataSize()));
		blk2 = Block(AES::Decrypt(key, store->Read(p2), GetBlockDataSize()));
	}
	else{
		blk2 = Block(AES::Decrypt(key, store->Read(p2), GetBlockDataSize()));
		blk1 = Block(AES::Decrypt(key, store->Read(p1), GetBlockDataSize()));
	}

	syncb1 = SyncPosition(bid1, p1, blk1, map);
	syncb2 = SyncPosition(bid2, p2, blk2, map);

	Block retBlock;

	if(op == READ && syncb1.bid == bid1)
		retBlock = syncb1;

	wblk1 = syncb1;
	if(op == WRITE)
		wblk1 = WriteBlock(blockdata, bid1, p1, syncb1, map);

	wblk2 = DuplicateBlock(p2, syncb2, syncb1, map);

	store->Write(p1, AES::Encrypt(key, wblk1.toBytes()));
	store->Write(p2, AES::Encrypt(key, wblk2.toBytes()));

	return retBlock;
}

Block CaosWClient::Read(size_t bid, DataMap *map)
{
	Block b;
	do{
		 b = Access(READ, bid, {}, map);
	} while(bid != b.bid);
	return b;
}

void CaosWClient::Write(size_t bid, blockData b, DataMap *map)
{
	assert(GetBlockDataSize() == b.size());
	int bidCount = (int)map->Positions(bid).size();
	while(bidCount == (int)map->Positions(bid).size()){
		Access(WRITE, bid, b, map);
	}

}

size_t CaosWClient::GetBlockCount()
{
	return blockCount;
}

size_t CaosWClient::GetBlockSize()
{
	return Block::getSerializedSize(blockSize);
}

size_t CaosWClient::GetBlockDataSize()
{
	return blockSize;
}

bool CaosWClient::WasSerialised()
{
	return store->WasSerialised();
}

bool CaosWClient::AquireFileLock(std::string filename){
	std::ifstream infile(filename+".lock");
	if(!infile.good()){
		std::fstream file;
		file.open(filename+".lock", std::ios::out | std::ios::trunc | std::ios::binary);

		file.write("", 0);
		file.close();
		return true;
	}
	return false;
}

void CaosWClient::ReleaseFileLock(std::string filename){
	if(std::remove((filename+".lock").c_str()) != 0)
		Log::Write(Log::WARNING, "Unable to release lock on %s", filename.c_str());
}

Block CaosWClient::SyncPosition(int64_t bid, size_t pos, Block block, DataMap *map){

	if(block.chg < map->LatestLocalVersion(block.bid)){
		map->RemovePosition(block.bid, pos);
		map->RemoveVFPosition(block.bid, pos);
		map->AddPosition(-1, pos);

		block.bid = -1;
		block.cns = cont;
		block.chg = time(nullptr);
	}
	else if(block.cns < NUM_CLIENTS){
		if(bid != block.bid){
			block.cns += cont;
			map->RemovePosition(bid, pos);
			map->RemoveVFPosition(bid, pos);

			if(block.chg > map->LatestLocalVersion(block.bid)){
				map->SetLocalVersion(block.bid, block.chg);
				map->SetPositions(block.bid, {pos});
				map->SetVFPositions(block.bid, {});

			}
			else if(block.chg == map->LatestLocalVersion(block.bid)){
				map->AddPosition(block.bid, pos);

			}
		}
		else if(block.chg > map->LatestLocalVersion(block.bid)){
			block.cns += cont;
			map->SetLocalVersion(block.bid, block.chg);
			map->SetPositions(block.bid, {pos});
			map->SetVFPositions(block.bid, {});

		}
	}

	if(block.cns == NUM_CLIENTS){
		map->AddVFPosition(block.bid, pos);
	}

	return block;
}

Block CaosWClient::DuplicateBlock(size_t pos, Block block, Block orgBlock, DataMap *map){

	if(block.cns == NUM_CLIENTS && (int)map->VFPositions(block.bid).size() > NUM_CLIENTS){

		map->SetVFPositions(block.bid, {});
		map->RemovePosition(block.bid, pos);
		block = orgBlock;
		block.cns = cont;
		map->AddPosition(orgBlock.bid, pos);

	}
	else if(block.cns == 1 && (int)map->Positions(block.bid).size() > 1){

		map->RemovePosition(block.bid, pos);
		block = orgBlock;
		block.cns = cont;
		map->AddPosition(orgBlock.bid, pos);

	}

	return block;
}

Block CaosWClient::WriteBlock(blockData blockdata, int64_t bid, size_t pos, Block block, DataMap *map){
	if(block.bid == -1 || block.bid == bid){
		block.bid = bid;
		block.chg = time(nullptr);
		block.cns = cont;
		block.data = blockdata;

		map->SetLocalVersion(block.bid, block.chg);
		map->SetPositions(block.bid, {pos});
		map->SetVFPositions(block.bid, {});
	}
	return block;
}

