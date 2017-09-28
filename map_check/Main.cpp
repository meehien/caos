#ifndef MAPCHECK_MAIN
#define MAPCHECK_MAIN

#include "File.hpp"
#include "FileStore.hpp"
#include "Log.hpp"
#include "Crypto.hpp"
#include "Types.hpp"
#include <functional>
#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstring>
#include <cmath>
#include "NetworkStore.hpp"
#include "MapCheck.hpp"

/*
 * This program is a psuedo Caos client designed to test the status of a client's map with respect to it's store. The
 * program runs three tests on the map:
 *
 * 			1. Map Validity
 * 				This test determines how many of the positions listed in the given map remain in the store and that they
 * 				each contain the block the map states that they should. This test reads each non-empty position from the
 * 				store which is listed in the map and returns a percentage of positions listed in the map which remain
 * 				valid.
 *
 * 			2. Map Knowledge
 * 				This test determines how many of the positions in the store are listed in the given map and that the map
 * 				lists the correct block for that position. This test differs from 1 as it is inclusive of non-empty
 * 				positions from the store which are not listed at all in the map. The test reads every position from the
 * 				store and returns a percentage of all the non-empty positions in the store which are listed in the map
 * 				and remain valid.
 *
 * 			3. Hit Rate
 * 				This test determines how many attempts, on average, must be made by a client using the given map before
 * 				it is able to successfully retrieve a given block. This is determined in main by test 1 however as with
 * 				each request clients improve their map knowledge of the store this must be accounted for and so cannot
 * 				simply be calculated by the result of test 1. The test accesses each block a fixed number of times (50
 * 				by default) and calculates the average number of attempts that were required for each access. To prevent
 * 				the modification of the store an intermediate store is used to cache modified positions for the duration
 * 				of each access.
 *
 * Tests carried out by this program do not modify either the map or the store.
 */

int main(int argc, char **argv)
{
	AES::Setup();

	//bytes<Key> key {0};
	bytes<Key> key = {0xd1, 0xdc, 0xe2, 0xe9, 0xa5, 0xc0, 0x8, 0xaf, 0x6c, 0x11, 0xeb, 0xd, 0x9, 0x95, 0xad, 0x39, 0x82, 0x7e, 0xf9, 0xa8, 0x8a, 0x52, 0xc3, 0x90, 0x62, 0x31, 0x4f, 0x8e, 0xec, 0x7d, 0xd4, 0xb6};

	size_t blockSize = 1024*64; // 64KiB
	size_t storeSize = 32768; //32GiB
	size_t blockCount = (storeSize*1048576)/blockSize;

	Log::Write(Log::INFO, "Store has: %zu blocks. Size is %zu MB", blockCount, storeSize);
	size_t storeBlockSize = IV + AES::GetCiphertextLength(Block::getSerializedSize(blockSize));
	Log::Write(Log::DEBUG, "SIZE %i", storeBlockSize);
	size_t storeBlockCount = 2*blockCount;

	// Create the store
	BlockStore *store = nullptr;
	store = new FileStore("store.bin", storeBlockCount, storeBlockSize);

	// Create the secure store
	std::string helpout="\nUsage:\t./map_check [--debug] <map filename>\n";

	int i = 1;
	if(argc == 3 && strcmp(argv[i], "--debug") == 0){
		Log::TurnOnDebugMode();
		i++;

		MapCheck *mc = new MapCheck(store, argv[i], blockCount, blockSize, key);
		mc->Check();
		delete mc;
	}
	else if(argc == 2){
		MapCheck *mc = new MapCheck(store, argv[i], blockCount, blockSize, key);
		mc->Check();
		delete mc;
	}
	else{
		std::cout << helpout;
		exit(0);
	}

	delete store;

	AES::Cleanup();
}

#endif //MAPCHECK_MAIN
