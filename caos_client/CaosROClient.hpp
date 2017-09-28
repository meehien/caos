#ifndef CSWCaosCLIENT_CaosROCLIENT_H
#define CSWCaosCLIENT_CaosROCLIENT_H

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

class CaosROClient : public CaosClient{
    BlockStore *store;

    size_t blockCount, blockSize;

    bytes<Key> key;

    const int64_t NUM_CLIENTS;

    int64_t cont;

public:
    CaosROClient(BlockStore *store, size_t blockCount, size_t blockSize, bytes<Key> key, int64_t numClients, int64_t cont);
    ~CaosROClient();

    enum Op {
        READ,
        WRITE
    };

    Block Access(Op op, int64_t bid1, blockData blockdata);

    Block Read(size_t bid);
    void Write(size_t bid, blockData b);

    void SaveBuffer(std::ostream& os, BufferMap &buffer, size_t size);
    void LoadBuffer(std::istream& is, BufferMap &buffer);

    size_t GetBlockCount();
    size_t GetBlockSize();
    size_t GetBlockDataSize();

    Block SyncPosition(int64_t bid, size_t pos, Block block, DataMap *map);
    Block DuplicateBlock(size_t pos, Block block, Block orgBlock, DataMap *map);

    bool WasSerialised();
};



#endif //CSWCaosCLIENT_CaosROCLIENT_H
