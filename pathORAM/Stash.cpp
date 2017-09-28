#include "Stash.hpp"
#include "File.hpp"

#include <fstream>
#include <cstring>

static block int32_to_bytes(int32_t n)
{
	block b(sizeof (n));
	
	memcpy(b.data(), &n, sizeof (n));

	return b;
}

bool StashHelper::Load(std::string filename, Stash &stash, size_t blockSize)
{
	std::fstream file;
	file.open(filename, std::ios::in | std::ios::binary);
	
	if (!file.good()) {
		return false;
	}
	
	size_t length = File::GetLength(file);
	
	for (size_t i = 0; i < length; i += sizeof (int32_t) + blockSize) {
		int32_t id;
		block b(blockSize);
		
		File::Read(file, (byte_t *) &id, sizeof (int32_t));
		File::Read(file, b.data(), b.size());
		
		// Place block into the stash
		stash[id] = b;
	}
	
	file.close();
	return true;
}

void StashHelper::Save(std::string filename, Stash &stash, size_t blockSize)
{
	std::fstream file;
	file.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
	
	for (auto s : stash) {
		block b(sizeof (int32_t) + blockSize);
	
		// Copy data to slab
		block idBlock = int32_to_bytes(s.first);
		b.insert(b.end(), idBlock.begin(), idBlock.end());
		b.insert(b.end(), s.second.begin(), s.second.end());

		File::Write(file, b.data(), b.size());
	}
	
	file.close();
}
