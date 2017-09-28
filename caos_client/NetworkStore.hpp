#include "MessageSender.hpp"
#include "BlockStore.hpp"
#include "message.pb.h"

class NetworkStore: public BlockStore {

	MessageSender<request, response> *sender;
	size_t storeBlockCount;
	size_t storeBlockSize;

public:
	NetworkStore(int host_port, const char* host_name, size_t storeBlockCount, size_t storeBlockSize);
	~NetworkStore();

	blockData Read(size_t pos);
	void Write(size_t pos, blockData b);

	size_t GetBlockCount();
	size_t GetBlockSize();

	bool WasSerialised();
};