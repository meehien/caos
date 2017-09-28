#include "ORAM.hpp"
#include "FileSystem.hpp"
#include "File.hpp"
#include "RAMStore.hpp"
#include "FileStore.hpp"
#include "Log.hpp"

#include <functional>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cmath> 

int main(int argc, char **argv)
{	
	AES::Setup();
	
	//bytes<Key> key {0};
	bytes<Key> key = {0xd1, 0xdc, 0xe2, 0xe9, 0xa5, 0xc0, 0x8, 0xaf, 0x6c, 0x11, 0xeb, 0xd, 0x9, 0x95, 0xad, 0x39, 0x82, 0x7e, 0xf9, 0xa8, 0x8a, 0x52, 0xc3, 0x90, 0x62, 0x31, 0x4f, 0x8e, 0xec, 0x7d, 0xd4, 0xb6};
	//Log::Write(Log::DEBUG, "here's a random: %zu", AES::GetRandom(10000));
	
	// depth of the tree
	// size of the store is 2^depth-1; 
	size_t depth = 13; //1GB

	size_t blockSize = 1024*16; // 16KiB
	size_t blockCount = Z*(pow(2, depth + 1) - 1);
	
	Log::Write(Log::INFO, "Store has: %zu blocks. Size is %zu MB", blockCount, (blockCount*blockSize)/1048576);

	size_t storeBlockSize = 0;
	size_t storeBlockCount = 0;
	
	//one bucket per store block
	storeBlockSize = IV + AES::GetCiphertextLength(Z*(sizeof(int32_t) + blockSize));
	storeBlockCount = blockCount/4;

	// Create the store
	BlockStore *store = nullptr;
	store = new FileStore("store.bin", storeBlockCount, storeBlockSize);
	
	// Create the secure store
	BlockStore *secureStore = nullptr;
	secureStore = new ORAM(store, depth, blockSize, key);

	FileSystem files(secureStore);
	files.Load("file.map");
	Log::Write(Log::INFO, "Space left %zu MB",files.GetFreeSpace());
	std::string helpout="\nUsage:\tpathORAM --add <filename> \t\t adds <filename> to ORAM\n\tpathORAM --get <filename1> <filename2> \t gets <filename1> from ORAM and saves it to <filename2>\n";
	switch (argc) {
		case 3:
			if (strcmp(argv[1], "--add") == 0){
				Log::Write(Log::INFO, "adding %s to oram", argv[2]);
				files.Add(argv[2]);
			} else {
				std::cout << helpout;
				exit(0);
			}
			break;
		case 4:
			if (strcmp(argv[1], "--get") == 0) {
				Log::Write(Log::INFO, "saving %s from oram to %s", argv[2], argv[3]);
				files.GetFile(argv[2], argv[3]);
			} else {
				std::cout << helpout;
				exit(0);
			}
			break;
		default:
			std::cout << helpout;
			exit(0);
	}
	
	Log::Write(Log::INFO, "Space left %zu MB",files.GetFreeSpace());	
	files.Save("file.map");
	
	delete secureStore;
	delete store;

	AES::Cleanup();
}

