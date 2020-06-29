#pragma once

#include <cstddef>
#include <unordered_map>
#include <list>
#include <vector>
#include <string>

struct FileInfo {
	size_t length;
	std::vector<size_t> blocks;
};

class BlockStore;

class FileSystem {
	BlockStore *store;
	std::unordered_map<std::string, FileInfo> filesystem;
	
	int GetAvailableID();

	std::pair<std::string, FileInfo> LoadFileInfo(std::string line);
	std::string SaveFileInfo(std::string filename, FileInfo info);

public:
	FileSystem(BlockStore *store);
	~FileSystem();
	
	int GetFreeSpace();
	bool Add(std::string filename);
	bool GetFile(std::string inputFile, std::string outputFile);
	bool Remove(std::string filename);
	
	void Load(std::string storemap);
	void Save(std::string storemap);
	
	BlockStore *GetBlockStore();

	FileInfo GetFileInfo(std::string filename);
};
