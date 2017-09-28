#include "CaosWClient.hpp"
#include "FileSystem.hpp"
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
#include "CaosRandomizer.hpp"
#include "CaosROClient.hpp"

int main(int argc, char **argv)
{
	AES::Setup();

	//hardcoded AES key
	bytes<Key> key = {0xd1, 0xdc, 0xe2, 0xe9, 0xa5, 0xc0, 0x8, 0xaf, 0x6c, 0x11, 0xeb, 0xd, 0x9, 0x95, 0xad, 0x39, 0x82, 0x7e, 0xf9, 0xa8, 0x8a, 0x52, 0xc3, 0x90, 0x62, 0x31, 0x4f, 0x8e, 0xec, 0x7d, 0xd4, 0xb6};

	size_t blockSize = 1024*64; // 64KiB
	size_t storeSize = 32768;   //32GiB in MiB
	size_t blockCount = (storeSize*1048576)/blockSize;

	Log::Write(Log::INFO, "Store has: %zu blocks. Size is %zu MB", blockCount, storeSize);
	size_t storeBlockSize = IV + AES::GetCiphertextLength(Block::getSerializedSize(blockSize));
	Log::Write(Log::DEBUG, "SIZE %i", storeBlockSize);
	size_t storeBlockCount = 2*blockCount;

	// Create the store
	BlockStore *store = nullptr;


	std::string helpout="\nUsage:\tcaos_client --writer --add <filename> -> adds <filename> to Caos using RW client\n\tcaos_client --writer --get<filename1> <filename2> -> gets <filename1> from Caos and saves it to <filename2> using RW client\n\tcaos_client --reader --get <filename1> <filename2> -> gets <filename1> from Caos and saves it to <filename2> using RO client\n\tcaos_client --agent <num1> <num2> -> runs Caos in agent mode <num1> times with <num2> buffer size\n\tcaos_client --debug --* -> truns Caos in debug mode\n";

	if(argc < 2){
		std::cout << helpout;
		exit(0);
	}
	int i = 1;
	if(strcmp(argv[i], "--debug") == 0){
		Log::TurnOnDebugMode();
		i++;
	}

	//hardcoded IP address, port and NUM clients. Change to actual server.
	char const *address = "127.0.0.1";
	int const port = 12345;
	// Create the secure store
	int numClients = NUM_CLIENTS;

	store = new NetworkStore(port, address, storeBlockCount, storeBlockSize);

	if(strcmp(argv[i], "--writer") == 0){
		if(strcmp(argv[i+1], "--add") == 0){
			if(argc == i+3){
				CaosWClient *secureStore = new CaosWClient(store, blockCount, blockSize, key, numClients, 1);
				FileSystem files(secureStore);
				files.Load("file.map");
				Log::Write(Log::INFO, "Space left %zu MB",files.GetFreeSpace());

				Log::Write(Log::INFO, "adding %s to Caos", argv[i+2]);
				files.Add(argv[i+2]);

				Log::Write(Log::INFO, "Space left %zu MB",files.GetFreeSpace());
				files.Save("file.map");
				delete secureStore;
			}
			else if(argc == i+4){
				CaosWClient *secureStore = new CaosWClient(store, blockCount, blockSize, key, numClients, atoi(argv[i+3]));
				FileSystem files(secureStore);
				files.Load("file.map");
				Log::Write(Log::INFO, "Space left %zu MB",files.GetFreeSpace());

				Log::Write(Log::INFO, "adding %s to Caos", argv[i+2]);
				files.Add(argv[i+2]);

				Log::Write(Log::INFO, "Space left %zu MB",files.GetFreeSpace());
				files.Save("file.map");
				delete secureStore;
			}
			else{
				std::cout << helpout;
				exit(0);
			}
		}
		else if(strcmp(argv[i+1], "--get") == 0){
			if(argc == i+4){
				CaosWClient *secureStore = new CaosWClient(store, blockCount, blockSize, key, numClients, 1);
				FileSystem files(secureStore);
				files.Load("file.map");
				Log::Write(Log::INFO, "Space left %zu MB",files.GetFreeSpace());

				Log::Write(Log::INFO, "saving %s from Caos to %s", argv[i+2], argv[i+3]);
				files.GetFile(argv[i+2], argv[i+3]);

				Log::Write(Log::INFO, "Space left %zu MB",files.GetFreeSpace());
				files.Save("file.map");
				delete secureStore;
			}
			else if(argc == i+5){
				CaosWClient *secureStore = new CaosWClient(store, blockCount, blockSize, key, numClients, atoi(argv[i+4]));
				FileSystem files(secureStore);
				files.Load("file.map");
				Log::Write(Log::INFO, "Space left %zu MB",files.GetFreeSpace());

				Log::Write(Log::INFO, "saving %s from Caos to %s", argv[i+2], argv[i+3]);
				files.GetFile(argv[i+2], argv[i+3]);

				Log::Write(Log::INFO, "Space left %zu MB",files.GetFreeSpace());
				files.Save("file.map");
				delete secureStore;
			}
			else{
				std::cout << helpout;
				exit(0);
			}
		}
		else{
			std::cout << helpout;
			exit(0);
		}
	}

	else if(strcmp(argv[i], "--agent") == 0){
		if(argc == i+3){
			CaosRandomizer *randomizer = nullptr;
			randomizer = new CaosRandomizer(store, blockCount, blockSize, key, numClients, 1);
			for(int j=0;j<strtol(argv[i+1], nullptr, 10);j++){
				randomizer->Access(strtol(argv[i+2], nullptr, 10));
			}
			delete randomizer;
		}
		else if(argc == i+4){
			CaosRandomizer *randomizer = nullptr;
			randomizer = new CaosRandomizer(store, blockCount, blockSize, key, numClients, atoi(argv[i+3]));
			for(int j=0;j<strtol(argv[i+1], nullptr, 10);j++){
				randomizer->Access(strtol(argv[i+2], nullptr, 10));
			}
			delete randomizer;
		}
		else{
			std::cout << helpout;
			exit(0);
		}
	}
	else{
		std::cout << helpout;
		exit(0);
	}

	delete store;

	AES::Cleanup();
}
