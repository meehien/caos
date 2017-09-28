#include "ORAM.hpp"
#include "Position.hpp"
#include "Log.hpp"
#include "Crypto.hpp"

#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstring>

ORAM::ORAM(BlockStore *store, size_t depth, size_t blockSize, bytes<Key> key)
: store(store), depth(depth), blockSize(blockSize), key(key), position(new int[GetBlockCount()])
{
	//if (blockSize != store->GetBlockSize()) {
	//	Log::Write(Log::DEBUG, "ORAM blocksize is %zu and store blocksize is %zu", blockSize, store->GetBlockSize());
	//}

	if (GetBucketCount() > store->GetBlockCount()) {
		Log::Write(Log::FATAL, "Store does not contain enough buckets");
	}

	bool stashExists = StashHelper::Load(oramstash, stash, blockSize);
	
	bool positionExists = PositionHelper::Load(orammap, position, GetBlockCount());
	
	wasSerialised = store->WasSerialised() && positionExists && stashExists;

	// Intialise state of ORAM is new
	if (!wasSerialised) {
		for (size_t i = 0; i < GetBucketCount(); i++) {
			Bucket bucket;

			for (int z = 0; z < Z; z++) {
				bucket[z].id = -1;
				bucket[z].data.resize(blockSize, 0);
			}

			WriteBucket(i, bucket);
		}
		
		// Initialise blocks with random paths
		for (size_t i = 0; i < GetBlockCount(); i++) {
			position[i] = RandomPath();
		}

		// Clear the stash
		stash.clear();
	}
	//Log::Write(Log::DEBUG, "Loading ORAM");
}

ORAM::~ORAM()
{
	PositionHelper::Save(orammap, position, GetBlockCount());
	StashHelper::Save(oramstash, stash, blockSize);
	//Log::Write(Log::DEBUG, "Unloaded ORAM");
}

// Generate a random path (a leaf node)
int ORAM::RandomPath()
{
	int rand=AES::GetRandom(GetBucketCount()/2);
	//Log::Write(Log::DEBUG, "random: %zu",rand);
	return rand;
}

// Fetches the array index a bucket
// that lise on a specific path
int ORAM::GetNodeOnPath(int leaf, int depth)
{
	leaf += GetBucketCount()/2;
	for (int d = GetDepth() - 1; d >= depth; d--) {
		leaf = (leaf + 1)/2 - 1;
	}
	
	return leaf;
}

static block int32_to_bytes(int32_t n)
{
	block b(sizeof (n));
	
	memcpy(b.data(), &n, sizeof (n));

	return b;
}

// Write bucket to a single block
block ORAM::SerialiseBucket(Bucket bucket)
{
	block buffer;

	for (int z = 0; z < Z; z++) {
		Block b = bucket[z];
	
		// Write block ID
		block idBlock = int32_to_bytes(b.id);
		buffer.insert(buffer.end(), idBlock.begin(), idBlock.end());

		assert(b.data.size() == blockSize);

		// Write block data
		buffer.insert(buffer.end(), b.data.begin(), b.data.end());
	}

	assert(buffer.size() == Z*(sizeof (int32_t) + blockSize));
	
	return buffer;
}

Bucket ORAM::DeserialiseBucket(block buffer)
{
	assert(buffer.size() == Z*(sizeof (int32_t) + blockSize));
	
	Bucket bucket;

	for (int z = 0; z < Z; z++) {
		Block &block = bucket[z];

		block.id = *((int32_t *) buffer.data());
		buffer.erase(buffer.begin(), buffer.begin() + sizeof (int32_t));

		block.data.assign(buffer.begin(), buffer.begin() + blockSize);
		buffer.erase(buffer.begin(), buffer.begin() + blockSize);
	}

	return bucket;	
}

Bucket ORAM::ReadBucket(int bid)
{
	block ciphertext = store->Read(bid);
	block buffer = AES::Decrypt(key, ciphertext);

	Bucket bucket = DeserialiseBucket(buffer);
	return bucket;
}

void ORAM::WriteBucket(int bid, Bucket bucket)
{
	block b = SerialiseBucket(bucket);

	block ciphertext = AES::Encrypt(key, b);
	store->Write(bid, ciphertext);
}

// Fetches blocks along a path, adding
// them to the stash
void ORAM::FetchPath(int x)
{
	for (size_t d = 0; d <= GetDepth(); d++) {
		Bucket bucket = ReadBucket(GetNodeOnPath(x, d));
		
		for (int z = 0; z < Z; z++) {
			Block &block = bucket[z];
		
			if (block.id != -1) { // It isn't a dummy block
				//Log::Write(Log::DEBUG, "stash block: %zu",block.id);
				stash.insert({block.id, block.data});	
			}
		}
	}
}

// Gets a list of blocks on the stash which can be placed at a
// specific point
std::vector<int> ORAM::GetIntersectingBlocks(int x, int depth)
{
	std::vector<int> validBlocks;
	
	int node = GetNodeOnPath(x, depth);
	
	for (auto b : stash) {
		int bid = b.first;

		if (GetNodeOnPath(position[bid], depth) == node) {
			validBlocks.push_back(bid);
		}
	}

	return validBlocks;
}

// Greedily writes blocks from the stash to the tree,
// pushing blocks as deep into the tree as possible
void ORAM::WritePath(int x)
{	
	// Write back the path
	for (int d = GetDepth(); d >= 0; d--) {
		// Find blocks that can be on this bucket
		int node = GetNodeOnPath(x, d);
		auto validBlocks = GetIntersectingBlocks(x, d);
		
		// Write blocks to tree
		Bucket bucket;
		for (int z = 0; z < std::min((int) validBlocks.size(), Z); z++) {
			Block &block = bucket[z];
			block.id = validBlocks[z];
			block.data = stash[block.id];
			
			// The block no longer needs to be in the stash
			stash.erase(block.id);
		}
		
		// Fill any empty spaces with dummy blocks
		for (int z = validBlocks.size(); z < Z; z++) {
			Block &block = bucket[z];

			block.id = -1;
			block.data.resize(blockSize, 0);
		}
		
		// Write bucket to tree
		WriteBucket(node, bucket);
	}
}

// Gets the data of a block in the stash
block ORAM::ReadData(int bid)
{	
	// Create block if it doesn't exist
	auto iter = stash.find(bid);
	if (iter == stash.end()) {
		// New blocks are zeroed out
		stash[bid].resize(blockSize, 0);
	}
	
	return stash[bid];
}

// Updates the data of a block in the stash
void ORAM::WriteData(int bid, block b)
{
	assert(b.size() == blockSize);

	stash[bid] = b;
}

// Fetches a block, allowing you to read and write 
// in a block
void ORAM::Access(Op op, int bid, block &b)
{
	int x = position[bid];
	position[bid] = RandomPath();
	
	FetchPath(x);
	
	if (op == READ) {
		b = ReadData(bid);
	} else {
		WriteData(bid, b);
	}
	
	WritePath(x);
}

block ORAM::Read(size_t bid)
{
	block b;
	Access(READ, bid, b);
	
	return b;
}

void ORAM::Write(size_t bid, block b)
{
	assert(blockSize == b.size());

	Access(WRITE, bid, b);
}

size_t ORAM::GetDepth()
{
	return depth;
}

size_t ORAM::GetBlockCount()
{
	return Z*GetBucketCount();
}

size_t ORAM::GetBlockSize()
{
	return blockSize;
}

size_t ORAM::GetBucketCount()
{
	size_t bucketCount = pow(2, GetDepth() + 1) - 1;
	return bucketCount;
}

bool ORAM::WasSerialised()
{
	return wasSerialised;
}
