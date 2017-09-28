#include "FileSystem.hpp"
#include "BlockStore.hpp"
#include "File.hpp"
#include "Log.hpp"

#include <sstream>
#include <algorithm>
#include <iostream>

FileSystem::FileSystem(BlockStore *store)
: store(store), filesystem()
{
	//Log::Write(Log::DEBUG, "Loading FileSystem");
}

FileSystem::~FileSystem()
{
	//Log::Write(Log::DEBUG, "Unloaded FileSystem");
}

int FileSystem::GetAvailableID()
{
	std::vector<size_t> usedblks;
	
	for (auto f : filesystem) {
		FileInfo fileinfo = f.second;
		for(std::vector<size_t>::iterator blkid = fileinfo.blocks.begin(); blkid != fileinfo.blocks.end(); ++blkid){
			usedblks.push_back(*blkid);
		}
	}
	std::sort(usedblks.begin(),usedblks.end());
	
	size_t var=0;
	if (!usedblks.empty()){
   	var = usedblks.back();
   	if (var + 1 >= store->GetBlockCount()) {
			Log::Write(Log::FATAL, "out of space");
		}
   	var++;
	}
	
	return var;
}

int FileSystem::GetFreeSpace()
{
	std::vector<size_t> usedblks;
	
	for (auto f : filesystem) {
		FileInfo fileinfo = f.second;
		for(std::vector<size_t>::iterator blkid = fileinfo.blocks.begin(); blkid != fileinfo.blocks.end(); ++blkid){
			usedblks.push_back(*blkid);
		}
	}
	std::sort(usedblks.begin(),usedblks.end());
	
	size_t var=0;
	if (!usedblks.empty()){
   	var = usedblks.back();
   	var++;
	}
	
	size_t maxblocks = store->GetBlockCount();
	size_t curmax = var;
	int spaceleft = ((maxblocks-curmax)*store->GetBlockSize())/1048576;
	
	//Log::Write(Log::INFO, "There are %zu blocks free. Size left is about %zu MB",maxblocks-curmax,((maxblocks-curmax)*store->GetBlockSize())/1048576);
	
	return spaceleft;
}

bool FileSystem::Add(std::string filename)
{	
	Log::Write(Log::DEBUG, "Adding File");
	if (filesystem.find(filename) != filesystem.end()) {
		Log::Write(Log::INFO, "file already exists");
		return false;
	}
	
	std::fstream file;
	file.open(filename, std::fstream::in | std::fstream::binary);
	
	if (!file) {
		Log::Write(Log::WARNING, "failed to open file");
		return false;
	}
	
	// Keep track of file info
	FileInfo &info = filesystem[filename];
	info.length = File::GetLength(file);
	
	size_t blockSize = store->GetBlockSize();
		
	for (size_t i = 0; i < info.length; i += blockSize) {
		size_t readLength = std::min(blockSize, info.length - i);
		
		block b(blockSize, 0);
		File::Read(file, b.data(), readLength);
		
		// get blockIDs
		size_t bid = GetAvailableID();
		info.blocks.push_back(bid);
		store->Write(bid, b);	
	}
	file.close();
	return true;
}

bool FileSystem::GetFile(std::string inputFile, std::string outputFile)
{	
	Log::Write(Log::DEBUG, "Getting File");
	if (!(filesystem.find(inputFile) != filesystem.end())) {
		Log::Write(Log::INFO, "file does not exist");
		return false;
	}
	
	std::fstream file;
	file.open(outputFile, std::ios::out | std::ios::binary | std::ios::trunc);
	
	if (!file) {
		Log::Write(Log::WARNING, "failed to open file");
		return false;
	}

	// metadata
	FileInfo info = filesystem[inputFile];

	size_t blockSize = store->GetBlockSize();
	for (size_t i = 0; i < info.blocks.size(); i++) {
		size_t writeLength = std::min(blockSize, info.length - blockSize*i);
		size_t pos = info.blocks[i];
		block buffer = store->Read(pos);
		file.write((char *) buffer.data(), writeLength);
		fflush(stdout);
	}
	file.close();
	
	return true;
}


bool FileSystem::Remove(std::string filename)
{
	if (filesystem.find(filename) == filesystem.end()) {
		Log::Write(Log::WARNING, "file not in database");
		return false;
	}
	
	// TODO: This doesn't actually free the IDs
	filesystem.erase(filename);
	return true;
}

std::pair<std::string, FileInfo> FileSystem::LoadFileInfo(std::string line)
{		
	std::istringstream sstream(line);
	std::string filename;
	sstream >> filename;
	
	FileInfo info;
	//we need to know how much to read.
	sstream >> info.length;
	size_t blockSize = store->GetBlockSize();
	for (size_t i = 0; i < info.length; i += blockSize) {
		size_t bid;
		sstream >> bid;
		info.blocks.push_back(bid);
	}
	return std::make_pair(filename, info);
}

void FileSystem::Load(std::string storemap)
{
	std::ifstream map(storemap);
	
	if (!map) {
		return;
	}
	std::string line;
	while (std::getline(map, line)) {
		filesystem.insert(LoadFileInfo(line));
	}
	map.close();
}

//saves file to map
std::string FileSystem::SaveFileInfo(std::string filename, FileInfo info)
{
	std::ostringstream sstream;
	
	sstream << filename;
	sstream << ' ';
	sstream << info.length;

	for (size_t i = 0; i < info.blocks.size(); i++) {
		sstream << ' ';
		sstream << info.blocks[i];
	}
	return sstream.str();
}

void FileSystem::Save(std::string storemap)
{
	std::ofstream map(storemap);
	for (auto f : filesystem) {
		map << SaveFileInfo(f.first, f.second);
		map << '\n';
	}
	map.close();
}

BlockStore *FileSystem::GetBlockStore()
{
	return store;
}

FileInfo FileSystem::GetFileInfo(std::string filename)
{
	return filesystem[filename];
}
