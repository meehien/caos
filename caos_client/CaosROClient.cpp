//
// Created by Luke Taher on 11/07/2016.
//

#include "CaosROClient.hpp"
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

CaosROClient::CaosROClient(BlockStore *store, size_t blockCount, size_t blockSize, bytes<Key> key, int64_t numClients, int64_t cont)
        : store(store), blockCount(blockCount), blockSize(blockSize), key(key), NUM_CLIENTS(numClients), cont(cont)
{
    size_t clen = IV + AES::GetCiphertextLength(GetBlockSize());

    if (store->GetBlockSize() != clen) {
        Log::Write(Log::FATAL,"store block size is too small");
    }

    if (store->GetBlockCount() < GetBlockCount()) {
        Log::Write(Log::FATAL,"store does not have enough blocks");
    }
    //Log::Write(Log::DEBUG, "Loading Client");
}

CaosROClient::~CaosROClient()
{
    //Log::Write(Log::DEBUG, "Unloaded Client");
}

void CaosROClient::LoadBuffer(std::istream& is, BufferMap &buffer)
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

void CaosROClient::SaveBuffer(std::ostream& os, BufferMap &buffer, size_t size)
{
    //size_t bidsize = buffer.bids.size();
    size_t bidsize = size;
    os.write((char*)&bidsize, sizeof(bidsize));
    os.write(reinterpret_cast<char*>(&buffer.bids[0]), buffer.bids.size() * sizeof(int64_t));

    //size_t possize = buffer.pos.size();
    size_t possize = size;
    os.write((char*)&possize, sizeof(possize));
    os.write(reinterpret_cast<char*>(&buffer.pos[0]), buffer.pos.size() * sizeof(int64_t));
}

Block CaosROClient::Access(Op op, int64_t bid1, blockData blockdata)
{

    DataMap *map = new DataMap(GetBlockCount());

    size_t p1 = map->GetRandBlockPosition(bid1);
    size_t p2 = AES::GetRandom(GetBlockCount()); //random pos or random set pos? ask server or local?

    Log::Write(Log::DEBUG, "BlockSize: %zu. MapSize: %zu",GetBlockSize(),GetBlockCount());

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
//	size_t storeBlockSize=store->GetBlockSize();

    FileStore *bufferData;
    bufferData = new FileStore(bufferdata, GetBlockCount()/*TODO:wrong?*/, GetBlockSize());
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
               bufferBlocks, caossize);

    //TODO: improve? start at -1?
    int ibc = static_cast<int>(GetBlockCount());
    int64_t bid2;
    for(int i = -1; i < ibc; i++){
        std::vector<size_t> blockPositions = map->Positions(i);
        if(std::find(blockPositions.begin(), blockPositions.end(),p2)!=blockPositions.end()) {
            bid2 = i;
            break;
        }
    }

    Block rblk1, rblk2, syncb1, syncb2, wblk1, wblk2, bblk1, bblk2;

    //TODO: just block size?
    bool read_first = AES::GetRandom(2);

    if(read_first){
        rblk1 = Block(AES::Decrypt(key, store->Read(p1), GetBlockDataSize()));
        rblk2 = Block(AES::Decrypt(key, store->Read(p2), GetBlockDataSize()));
    }
    else{
        rblk2 = Block(AES::Decrypt(key, store->Read(p2), GetBlockDataSize()));
        rblk1 = Block(AES::Decrypt(key, store->Read(p1), GetBlockDataSize()));
    }


    //TODO: issue with bid2 not set [see for loop above]
    syncb1 = SyncPosition(bid1, p1, rblk1, map);
    syncb2 = SyncPosition(bid2, p2, rblk2, map);


    if(bufferMap.bids.size() != GetBlockCount()){ //TODO: ifs same
        bufferMap.bids.push_back(bid1);
        bufferMap.pos.push_back(p1);
        bufferData->Write(p1,syncb1.toBytes());
    }
    if(bufferMap.bids.size() != GetBlockCount()){
        bufferMap.bids.push_back(bid2);
        bufferMap.pos.push_back(p2);
        bufferData->Write(p2,syncb2.toBytes());
    }

    size_t rPos1, rPos2;//TODO: not always defined
    wblk1 = syncb1;//TODO: right?
    wblk2 = syncb2;//TODO: right?
    if(bufferMap.bids.size() == GetBlockCount()){ //TODO: seperate 2 ifs?
        rPos1 = AES::GetRandom(bufferMap.pos.size()); //TODO: is this right?
        bblk1 = bufferData->Read(bufferMap.pos[rPos1]);
        wblk1 = DuplicateBlock(p1, syncb1, bblk1, map);

        rPos2 = AES::GetRandom(bufferMap.pos.size()); //TODO: is this right?
        bblk2 = bufferData->Read(bufferMap.pos[rPos2]);
        wblk2 = DuplicateBlock(p2, syncb2, bblk2, map);
    }

    //TODO: this ok?
    if(wblk1.bid != syncb1.bid){
        bufferMap.bids.erase(std::remove(bufferMap.bids.begin(), bufferMap.bids.end(), wblk1.bid),
                             bufferMap.bids.end());
        bufferMap.pos.erase(std::remove(bufferMap.pos.begin(), bufferMap.pos.end(), rPos1),
                            bufferMap.pos.end());
    }
    if(wblk2.bid != syncb2.bid){
        bufferMap.bids.erase(std::remove(bufferMap.bids.begin(), bufferMap.bids.end(), wblk2.bid),
                             bufferMap.bids.end());
        bufferMap.pos.erase(std::remove(bufferMap.pos.begin(), bufferMap.pos.end(), rPos2),
                            bufferMap.pos.end());
    }

    store->Write(p1, AES::Encrypt(key, wblk1.toBytes()));
    store->Write(p2, AES::Encrypt(key, wblk2.toBytes()));

    map->SaveMap();

    if (bufferMap.bids.size()<GetBlockCount()/*TODO:wrong?*/){
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
    return rblk1;
}

Block CaosROClient::Read(size_t bid)
{
    return Access(READ, bid, {});
}

void CaosROClient::Write(size_t bid, blockData b)
{
	Log::Write(Log::FATAL, "Read only client cannot write");
}

size_t CaosROClient::GetBlockCount()
{
    return blockCount;
}

size_t CaosROClient::GetBlockSize()
{
    return Block::getSerializedSize(blockSize);
}

size_t CaosROClient::GetBlockDataSize()
{
    return blockSize;
}

bool CaosROClient::WasSerialised()
{
    return store->WasSerialised();
}

Block CaosROClient::SyncPosition(int64_t bid, size_t pos, Block block, DataMap *map){

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

Block CaosROClient::DuplicateBlock(size_t pos, Block block, Block orgBlock, DataMap *map){
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