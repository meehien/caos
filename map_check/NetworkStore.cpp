#include "Log.hpp"
#include "NetworkStore.hpp"
#include "Block.hpp"
#include "Types.hpp"
#include "Crypto.hpp"
#include <sstream>

NetworkStore::NetworkStore(int host_port, const char* host_name, size_t storeBlockCount, size_t storeBlockSize){
//    Log::Write(Log::DEBUG, "Loading NetworkStore");
	this->sender = new MessageSender<request, response>(host_port, host_name);
	this->storeBlockCount = storeBlockCount;
	this->storeBlockSize = storeBlockSize;
}

NetworkStore::~NetworkStore(){
	delete(sender);
}

blockData NetworkStore::Read(size_t pos){
//    Log::Write(Log::DEBUG, "Block read at Position: %i", pos);
	assert(pos < GetBlockCount());
	request *payload = new request();
	payload->set_type(MessageType::READ);
	payload->set_pos(pos);

	bool success = false;
	response respPayload;

	int attempt = 0;
	while(!success){
		if(attempt == 3){
				delete payload;
				Log::Write(Log::FATAL, "Unable to establish connection to server");
		}


		attempt++;
		respPayload = sender->ReceiveResponse(sender->SendMessage(payload));
		success = respPayload.res() && respPayload.block_size() == GetBlockSize();
	}

	blockData b;

	b.assign(respPayload.block().begin(), respPayload.block().end());
	delete(payload);
	return b;
}

void NetworkStore::Write(size_t pos, blockData b){
//    Log::Write(Log::DEBUG, "Block Write at Position: %i", pos);
	assert(pos < GetBlockCount());

	request *payload = new request();
	payload->set_type(MessageType::WRITE);
	payload->set_pos(pos);

	std::stringstream byteString;
	std::copy(b.begin(), b.end(), std::ostream_iterator<byte_t>(byteString, ""));
	payload->set_block(byteString.str());

	//TODO: issue with block size sometimes too small sometimes too large
	payload->set_block_size(b.size());

	bool success = false;
	response respPayload;

	int attempt = 0;
	while(!success) {
		if(attempt == 3){
				delete payload;
				Log::Write(Log::FATAL, "Unable to establish connection to server");
		}


		attempt++;
		respPayload = sender->ReceiveResponse(sender->SendMessage(payload));
		success = respPayload.res();
	}

	if(!(respPayload.type() == MessageType::WRITE && respPayload.pos() == pos && respPayload.res()))
		Log::Write(Log::WARNING, "Write to position %i failed, response position %i", pos);
//    else{}
//        Log::Write(Log::INFO, "Write to position %i successful", pos);
	delete payload;
}

size_t NetworkStore::GetBlockCount()
{
	return storeBlockCount;
}

size_t NetworkStore::GetBlockSize()
{
	return storeBlockSize;
}

bool NetworkStore::WasSerialised() {
	//TODO: What to do about serialised
	return true;
}