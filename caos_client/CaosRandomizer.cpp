#include "CaosRandomizer.hpp"
#include "FileStore.hpp"
#include "File.hpp"
#include "Log.hpp"
#include "DataMap.hpp"
#include "message.pb.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstring>
#include <ctime>
#include <cinttypes>
#include <cassert>
#include <iostream>

CaosRandomizer::CaosRandomizer(BlockStore *store, size_t blockCount, size_t blockSize, bytes<Key> key, int64_t numClients, int64_t cont)
: store(store), blockCount(blockCount), blockSize(blockSize), key(key), NUM_CLIENTS(numClients), cont(cont)
{
	size_t clen = IV + AES::GetCiphertextLength(GetBlockSize());
	
	if (store->GetBlockSize() != clen) {
		Log::Write(Log::FATAL,"store block size is too small");
	}

	if (store->GetBlockCount() < GetBlockCount()) {
		Log::Write(Log::FATAL,"store does not have enough blocks");
	}
}

CaosRandomizer::~CaosRandomizer()
{
	Log::Write(Log::DEBUG, "Unloaded Client");
}

void CaosRandomizer::LoadBuffer(std::istream& is, BufferMap &buffer)
{
	size_t bidsize = 0;
	is.read((char*)&bidsize, sizeof(bidsize));
	buffer.bids.resize(bidsize);
	is.read(reinterpret_cast<char*>(&buffer.bids[0]), bidsize * sizeof(int64_t));
	
	size_t possize = 0;
	is.read((char*)&possize, sizeof(possize));
	buffer.pos.resize(possize);
	is.read(reinterpret_cast<char*>(&buffer.pos[0]), possize * sizeof(int64_t));
}

void CaosRandomizer::SaveBuffer(std::ostream& os, BufferMap &buffer, size_t size)
{
	size_t bidsize = size;
	os.write((char*)&bidsize, sizeof(bidsize));
	os.write(reinterpret_cast<char*>(&buffer.bids[0]), buffer.bids.size() * sizeof(int64_t));
	
	size_t possize = size;
	os.write((char*)&possize, sizeof(possize));
	os.write(reinterpret_cast<char*>(&buffer.pos[0]), buffer.pos.size() * sizeof(int64_t));
}

bool CaosRandomizer::Access(int64_t fullsize)
{

	if (fullsize <= 1)
		Log::Write(Log::FATAL, "There is no use running the agent with a buffer size of 1.");

	DataMap *map = new DataMap(store->GetBlockCount());

	//initialize buffer
	BufferMap bufferMap;
	std::ifstream in(buffermap, std::ios::out | std::ios::binary);
	if(in.good()){
		Log::Write(Log::DEBUG, "Loading buffer map...");
		LoadBuffer(in, bufferMap);
		in.close();
	}
	else{
		bufferMap.pos = {};
		bufferMap.bids = {};
	}

	FileStore *bufferData;
	bufferData = new FileStore(bufferdata, fullsize, GetBlockSize());
	size_t bufferBlocks=bufferData->GetBlockCount();

	//buffer has one extra element. buffermap loses an element when is ejected from buffer.
	if ((bufferBlocks-1)<bufferMap.bids.size()){
		delete map;
		delete bufferData;
		std::ofstream out(buffermap, std::ios::out | std::ios::binary);
		out.close();
		Log::Write(Log::FATAL, "Map size was too big for store. Buffer has been reset as some data might not have been addressable.");
	}

	//filling buffer
	size_t caossize=GetBlockCount();
	Log::Write(Log::DEBUG, "BufferMap size: %zu, Buffer size: %zu, Written size: %zu",bufferMap.bids.size(),
			   fullsize, caossize);

	int64_t bid1 = -1;
	size_t p1;

	if(map->GetMapSize() > (int)bufferMap.bids.size()){
		size_t positionsize;
		do {
			bid1=AES::GetRandom(caossize);
			positionsize = map->Positions(bid1).size();
		} while (((positionsize == 0) || (std::find(bufferMap.bids.begin(), bufferMap.bids.end(), bid1) != bufferMap.bids.end())));

		//read from caos a random position for the bid selected.
		p1 = map->GetRandBlockPosition(bid1);
	}
	else{
		p1 = AES::GetRandom(GetBlockCount());
		int ibc = static_cast<int>(GetBlockCount());
		for(int i = -1; i < ibc; i++){
			std::vector<size_t> blockPositions = map->Positions(i);
			if(std::find(blockPositions.begin(), blockPositions.end(),p1)!=blockPositions.end()) {
				bid1 = i;
				break;
			}
		}
	}

	size_t p2 = AES::GetRandom(GetBlockCount());
	int ibc = static_cast<int>(GetBlockCount());
	int64_t bid2 = -1;
	for(int i = -1; i < ibc; i++){
		std::vector<size_t> blockPositions = map->Positions(i);
		if(std::find(blockPositions.begin(), blockPositions.end(),p2)!=blockPositions.end()) {
			bid2 = i;
			break;
		}
	}

	Block rblk1, rblk2, syncb1, syncb2, wblk1, wblk2, bblk1, bblk2;


	rblk1 = Block(AES::Decrypt(key, store->Read(p1), GetBlockDataSize()));
	rblk2 = Block(AES::Decrypt(key, store->Read(p2), GetBlockDataSize()));
	Log::Write(Log::DEBUG, "BID1: %i", bid1);
	Log::Write(Log::DEBUG, "BID2: %i", bid2);

	syncb1 = SyncPosition(bid1, p1, rblk1, map);
	syncb2 = SyncPosition(bid2, p2, rblk2, map);

	if(bufferMap.bids.size() != (bufferBlocks-1) && (std::find(bufferMap.bids.begin(), bufferMap.bids.end(), syncb1.bid) == bufferMap.bids.end())){
		if(syncb1.toBytes().size() != GetBlockSize())
			return false;
		size_t randBufferPos;
		do {
			randBufferPos = AES::GetRandom(fullsize);
		} while ((std::find(bufferMap.pos.begin(), bufferMap.pos.end(), randBufferPos) != bufferMap.pos.end()) && (bufferMap.pos.size()<fullsize)/*TODO: why?*/);
		bufferMap.bids.push_back(syncb1.bid);
		Log::Write(Log::DEBUG, "BID1: %i PUSH BACK POS %i", syncb1.bid, randBufferPos);
		bufferMap.pos.push_back(randBufferPos);
		bufferData->Write(randBufferPos,syncb1.toBytes());
	}

	if(bufferMap.bids.size() != (bufferBlocks-1) && (std::find(bufferMap.bids.begin(), bufferMap.bids.end(), syncb2.bid) == bufferMap.bids.end())){
		if(syncb2.toBytes().size() != GetBlockSize())
			return false;
		size_t randBufferPos;
		do {
			randBufferPos = AES::GetRandom(fullsize);
		} while ((std::find(bufferMap.pos.begin(), bufferMap.pos.end(), randBufferPos) != bufferMap.pos.end()) && (bufferMap.pos.size()<fullsize));
		bufferMap.bids.push_back(syncb2.bid);
		Log::Write(Log::DEBUG, "BID2: %i PUSH BACK POS %i", syncb2.bid, randBufferPos);
		bufferMap.pos.push_back(randBufferPos);
		bufferData->Write(randBufferPos,syncb2.toBytes());
	}

	size_t rPos1, rPos2;
	wblk1 = syncb1;
	wblk2 = syncb2;


	if(bufferMap.bids.size() == (bufferBlocks-1)){
		rPos1 = AES::GetRandom(bufferMap.pos.size());
		bblk1 = bufferData->Read(bufferMap.pos[rPos1]);
		Log::Write(Log::DEBUG, "Selected Buffer item 1: bid %i, pos %i", bblk1.bid, bufferMap.pos[rPos1]);
		wblk1 = DuplicateBlock(p1, syncb1, bblk1, map);

		rPos2 = AES::GetRandom(bufferMap.pos.size());
		bblk2 = bufferData->Read(bufferMap.pos[rPos1]);
		Log::Write(Log::DEBUG, "Selected Buffer item 2: bid %i, pos %i", bblk2.bid, bufferMap.pos[rPos2]);
		wblk2 = DuplicateBlock(p2, syncb2, bblk2, map);
	}


	if(wblk1.bid != syncb1.bid){
		Log::Write(Log::DEBUG, "Removing (1) bid %i pos %i", bblk1.bid, bufferMap.pos[rPos1]);
		bufferMap.bids.erase(std::remove(bufferMap.bids.begin(), bufferMap.bids.end(), bufferMap.bids[rPos1]),
							 bufferMap.bids.end());
		bufferMap.pos.erase(std::remove(bufferMap.pos.begin(), bufferMap.pos.end(), bufferMap.pos[rPos1]),
							bufferMap.pos.end());
	}
	if(wblk2.bid != syncb2.bid){
		Log::Write(Log::DEBUG, "Removing (2) bid %i pos %i", bblk2.bid, bufferMap.pos[rPos2]);
		bufferMap.bids.erase(std::remove(bufferMap.bids.begin(), bufferMap.bids.end(), bufferMap.bids[rPos2]),
							 bufferMap.bids.end());
		bufferMap.pos.erase(std::remove(bufferMap.pos.begin(), bufferMap.pos.end(), bufferMap.pos[rPos2]),
							bufferMap.pos.end());
	}

	store->Write(p1, AES::Encrypt(key, wblk1.toBytes()));
	store->Write(p2, AES::Encrypt(key, wblk2.toBytes()));

	map->SaveMap();
	Log::Write(Log::DEBUG, "BID SIZE: %i POS SIZE: %i", (int)bufferMap.bids.size(), (int)bufferMap.pos.size());
	if (bufferMap.bids.size()<bufferBlocks){
			std::ofstream out(buffermap, std::ios::out | std::ios::binary);
			SaveBuffer(out, bufferMap, bufferMap.bids.size());
			out.close();
	} else {
		delete map;
		delete bufferData;
		Log::Write(Log::FATAL, "BufferMap size error.");
	}

	Log::Write(Log::DEBUG, "Saved buffer map with %zu blocks.", bufferMap.bids.size());
	delete map;
	delete bufferData;
	return true;
}

size_t CaosRandomizer::GetBlockCount()
{
	return blockCount;
}

size_t CaosRandomizer::GetBlockSize()
{
	return Block::getSerializedSize(blockSize);
}

size_t CaosRandomizer::GetBlockDataSize()
{
	return blockSize;
}

bool CaosRandomizer::WasSerialised()
{
	return store->WasSerialised();
}

Block CaosRandomizer::SyncPosition(int64_t bid, size_t pos, Block block, DataMap *map){
	if(block.chg < map->LatestLocalVersion(block.bid)){
		map->RemovePosition(block.bid, pos);
		map->RemoveVFPosition(block.bid, pos);
		map->AddPosition(-1, pos);

		block.bid = -1;
		Log::Write(Log::DEBUG, "block %i pos %i set cns to %i", block.bid, pos, cont);
		block.cns = cont;
		block.chg = time(nullptr);

	}
	else if(block.cns < NUM_CLIENTS){
		if(bid != block.bid){
			Log::Write(Log::DEBUG, "block %i pos %i inc cns by %i to %i because bid (%i) != block.bid (%i)", block.bid, pos, cont, block.cns+1, bid, block.bid);
			block.cns += cont;
			map->RemovePosition(bid, pos);
			map->RemoveVFPosition(bid, pos);

			if(block.chg > map->LatestLocalVersion(block.bid)){
				map->SetLocalVersion(block.bid, block.chg);
				map->SetPositions(block.bid, {pos});
				map->SetVFPositions(block.bid, {});

			}
			else if(block.chg == map->LatestLocalVersion(block.bid)){
				Log::Write(Log::DEBUG, "LLV %i %i", block.chg, map->LatestLocalVersion(block.bid));
				map->AddPosition(block.bid, pos);
			}
		}
		else if(block.chg > map->LatestLocalVersion(block.bid)){
			Log::Write(Log::DEBUG, "block %i pos %i inc cns by %i to %i because chg (%i) newer than map (%i)", block.bid, pos, cont, block.cns+1, block.chg, map->LatestLocalVersion(block.bid));
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

Block CaosRandomizer::DuplicateBlock(size_t pos, Block block, Block orgBlock, DataMap *map){
	Log::Write(Log::DEBUG, "Duplicate %i into %i", orgBlock.bid, block.bid);
	Log::Write(Log::DEBUG, "cns: %i vf: %i cont: %i", block.cns, (int)map->VFPositions(block.bid).size(), cont);
	if(block.cns == NUM_CLIENTS && (int)map->VFPositions(block.bid).size() > NUM_CLIENTS){
		Log::Write(Log::DEBUG, "D1");
		map->SetVFPositions(block.bid, {});
		map->RemovePosition(block.bid, pos);
		block = orgBlock;
		block.cns = cont;
		map->AddPosition(orgBlock.bid, pos);
		Log::Write(Log::DEBUG, "LLV %i %i", block.chg, map->LatestLocalVersion(block.bid));
	}
	else if(block.cns == 1 && (int)map->Positions(block.bid).size() > 1){
		Log::Write(Log::DEBUG, "D2");
		map->RemovePosition(block.bid, pos);
		block = orgBlock;
		block.cns = cont;
		map->AddPosition(orgBlock.bid, pos);
		Log::Write(Log::DEBUG, "LLV %i %i", block.chg, map->LatestLocalVersion(block.bid));
	}
	Log::Write(Log::DEBUG, "Resulting in %i", block.bid);
	return block;
}