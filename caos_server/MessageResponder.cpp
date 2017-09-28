#include <sys/socket.h>
#include <sys/errno.h>
#include "MessageResponder.hpp"
#include <sstream>


MessageResponder::MessageResponder(BlockStore *store, MessageReceiver<request, response> *receiver) {
	this->store = store;
	this->receiver = receiver;
}

MessageResponder::~MessageResponder() { }

void MessageResponder::readBody(int csock, google::protobuf::uint32 siz)
{
	int bytecount;
	request payload;
	char buffer [siz+4];//size of the payload and hdr
	//Read the entire buffer including the hdr
	if((bytecount = recv(csock, (void *)buffer, 4+siz, MSG_WAITALL))== -1){
		fprintf(stderr, "Error receiving data %d\n", errno);
		close(csock);
	}
	//Assign ArrayInputStream with enough memory
	google::protobuf::io::ArrayInputStream ais(buffer,siz+4);
	google::protobuf::io::CodedInputStream coded_input(&ais);
	//Read an unsigned integer with Varint encoding, truncating to 32 bits.
	coded_input.ReadVarint32(&siz);
	//After the message's length is read, PushLimit() is used to prevent the CodedInputStream
	//from reading beyond that length.Limits are used when parsing length-delimited
	//embedded messages
	google::protobuf::io::CodedInputStream::Limit msgLimit = coded_input.PushLimit(siz);
	//De-Serialize
	payload.ParseFromCodedStream(&coded_input);
	//Once the embedded message has been parsed, PopLimit() is called to undo the limit
	coded_input.PopLimit(msgLimit);

	if(payload.type() == MessageType::READ) {
		Log::Write(Log::DEBUG,"Read request received for position %i", payload.pos());

		receiver->SendResponse(&csock, ConstructReadResponse(payload));
	}
	else if(payload.type() == MessageType::WRITE) {
		Log::Write(Log::DEBUG,"Write request received for position %i", payload.pos());
		assert(payload.pos() < store->GetBlockCount());
		assert(payload.block_size() == store->GetBlockSize());

		receiver->SendResponse(&csock, ConstructWriteResponse(payload));
	}
	else{
		Log::Write(Log::WARNING, "Invalid message received");
	}

}

response MessageResponder::ConstructReadResponse(request payload){
	response respPayload;
	respPayload.set_type(MessageType::READ);
	respPayload.set_pos(payload.pos());
	respPayload.set_res(false);

	if(payload.pos() < store->GetBlockCount() && store->AquireLock(payload.pos()))
	{
		block b = store->Read(payload.pos());
		store->ReleaseLock(payload.pos());

		respPayload.set_block_size(store->GetBlockSize());

		std::stringstream byteString;
		std::copy(b.begin(), b.end(), std::ostream_iterator<byte_t>(byteString, ""));
		respPayload.set_block(byteString.str());
		respPayload.set_res(true);
	}

	return respPayload;
}

response MessageResponder::ConstructWriteResponse(request payload){

	block b;
	b.assign(payload.block().begin(), payload.block().end());

	response respPayload;
	respPayload.set_type(MessageType::WRITE);
	respPayload.set_pos(payload.pos());
	respPayload.set_res(false);

	if(store->AquireLock(payload.pos()))
	{
		store->Write(payload.pos(), b);
		store->ReleaseLock(payload.pos());

		respPayload.set_res(true);
	}

	return respPayload;
}