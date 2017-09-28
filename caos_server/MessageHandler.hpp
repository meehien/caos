#include <google/protobuf/stubs/common.h>
#include "BlockStore.hpp"

class MessageHandler{
	BlockStore *store;

public:
	virtual ~MessageHandler() = default;

	virtual void readBody(int csock, google::protobuf::uint32 siz) = 0;
};