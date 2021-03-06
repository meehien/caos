#include "FileStore.hpp"
#include "Log.hpp"
#include <algorithm>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <cassert>
#include <string>

size_t GetFileSize(int fd)
{
	// Jump to the end of the file
	size_t size = lseek(fd, 0L, SEEK_END);
	
	// Return back to the start
	lseek(fd, 0L, SEEK_SET);
	
	return size;
}

FileStore::FileStore(std::string filename, size_t count, size_t size)
: count(count), size(size)
{
	posLock = new std::vector<size_t>;
	//Log::Write(Log::DEBUG, "Loading FileStore");
	// Open the file
	int oflags = O_RDWR | O_CREAT;
	fd = open(filename.c_str(), oflags, 0666);

	if (fd == -1) {
		perror("Error opening file");
		exit(1);
	}

	size_t currentSize = GetFileSize(fd);
	size_t totalSize = count*size;

	// The file was serialied if the size is
	// the same as the expected size
	wasSerialised = currentSize == totalSize;

	// Resize the file if needed
	if (currentSize != totalSize) {
		ftruncate(fd, totalSize);
	}

	// Map the file to memory
	int mflags = PROT_READ | PROT_WRITE;
	map = (byte_t *) mmap(nullptr, totalSize, mflags, MAP_SHARED, fd, 0);

	if (map == MAP_FAILED) {
		perror("Error mapping file");
		close(fd);
		exit(1);
	}
}

FileStore::~FileStore()
{
	if (munmap(map, count*size) == -1) {
		perror("Failed to unmap file");
		exit(1);
	}

	close(fd);
	//Log::Write(Log::DEBUG, "Unloaded FileStore");
}

block FileStore::Read(size_t pos)
{
	assert(pos < GetBlockCount());
	
	block b(size);
	std::copy(&map[pos*size], &map[pos*size] + size, b.begin());

	return b;
}

void FileStore::Write(size_t pos, block b)
{
	assert(pos < GetBlockCount());
	//TODO: fix block size
//	assert(b.size() == GetBlockSize());
	
	std::copy(b.begin(), b.end(), &map[pos*size]);
}

size_t FileStore::GetBlockCount()
{
	return count;
}

size_t FileStore::GetBlockSize()
{
	return size;
}

bool FileStore::WasSerialised()
{
	return wasSerialised;
}

bool FileStore::AquireLock(size_t pos)
{
	bool locked = std::find(posLock->begin(), posLock->end(), pos) != posLock->end();
	if(!locked)
		posLock->push_back(pos);
	else
		Log::Write(Log::WARNING, "Collision over position: %i", pos);

	return !locked;
}

void FileStore::ReleaseLock(size_t pos)
{
	posLock->erase(std::remove(posLock->begin(), posLock->end(), pos), posLock->end());
}
