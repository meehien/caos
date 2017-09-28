#include <google/protobuf/stubs/common.h>
#include "BlockStore.hpp"
#include "message.pb.h"
#include "MessageReceiver.hpp"
#include "MessageHandler.hpp"

class MessageResponder : public MessageHandler{
	BlockStore *store;
	MessageReceiver<request, response> *receiver;
	response ConstructReadResponse(request payload);
	response ConstructWriteResponse(request payload);

public:
	MessageResponder(BlockStore *store, MessageReceiver<request, response> *receiver);
	~MessageResponder();

	void readBody(int csock, google::protobuf::uint32 siz);
};