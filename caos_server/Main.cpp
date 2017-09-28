#include "FileStore.hpp"
#include "Log.hpp"
#include "MessageReceiver.hpp"
#include "MessageResponder.hpp"

#include <functional>
#include <fstream>
#include <iostream>
#include <cmath>

void* handler(void* lp);
MessageReceiver<request, response> *receiver;
BlockStore *store = nullptr;
MessageHandler *process = nullptr;

int main(int argc, char **argv)
{
	size_t blockSize = 1024*64; // 64KiB
	size_t storeSize = 32768;   // 32GiB
	size_t blockCount = (storeSize*1048576)/blockSize;
	size_t iv = 16;
	size_t aesPadding = 16;
	size_t metadata = 16;

	//iv + aesPadding + blockSize + metadata
	size_t storeBlockSize = 65584;
	Log::Write(Log::DEBUG, "Store block size: %i", storeBlockSize);
	Log::Write(Log::INFO, "Store has: %zu blocks. Size is %zu MB", blockCount, storeSize);

	size_t storeBlockCount = 2*blockCount;

	// Create the store
	store = new FileStore("store.bin", storeBlockCount, storeBlockSize);

	std::string helpout="\nUsage:\tcaos_server --listen [<port>] -> listens on <port> for connections, defaults to 12345\n\tcaos_server --debug --* -> truns CAOS debug mode on\n";

	int port=12345;

	switch (argc) {
		case 2:
			if (strcmp(argv[1], "--listen") == 0){
				Log::Write(Log::INFO, "listening for clients on port 12345");
				receiver = new MessageReceiver<request, response>(port);
				process = new MessageResponder(store, receiver);
				receiver->Listen(&handler);
			} else{
				std::cout << helpout;
				exit(0);
			}
			break;
		case 3:
			if (strcmp(argv[1], "--listen") == 0){
				port = atoi(argv[2]);
				Log::Write(Log::INFO, "listening for clients on port", argv[2]);
				receiver = new MessageReceiver<request, response>(port);
				process = new MessageResponder(store, receiver);
				receiver->Listen(&handler);
			} else{
				std::cout << helpout;
				exit(0);
			}
			break;
		case 4:
			if (strcmp(argv[1], "--debug") + strcmp(argv[2], "--listen") == 0){
				Log::TurnOnDebugMode();
				Log::Write(Log::INFO, "listening for clients on port", argv[3]);
				receiver = new MessageReceiver<request, response>(atoi(argv[3]));
				process = new MessageResponder(store, receiver);
				receiver->Listen(&handler);
			} else{
				std::cout << helpout;
				exit(0);
			}
			break;
		default:
			std::cout << helpout;
			exit(0);
	}

	delete store;
	delete process;
}

void* handler(void *lp){
	int *csock = (int*)lp;

	char buffer[4];
	int bytecount=0;

	memset(buffer, '\0', 4);

	while (1) {
		//Peek into the socket and get the packet size
		if((bytecount = recv(*csock,
							 buffer,
							 4, MSG_PEEK))== -1){
			fprintf(stderr, "Error receiving data %d\n", errno);
			close(*csock);
		}else if (bytecount == 0)
			break;

		process->readBody(*csock,receiver->readHdr(buffer));
	}
	close(*csock);
}
