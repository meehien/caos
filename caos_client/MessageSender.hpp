#include <google/protobuf/message.h>
#include <netinet/in.h>

template<class T, class Q>
class MessageSender{
	int host_port;
	const char* host_name;

    int siz;
    char *pkt;

    struct sockaddr_in my_addr;
    char buffer[1024];
    int bytecount;
    int buffer_len;

    int * p_int;
    int err;
	int hsock;

	void Serialize(T *message, int siz, char *pkt);

    Q readBody(int csock,google::protobuf::uint32 siz);
    Q SocketHandler(void* lp);
    google::protobuf::uint32 readHdr(char *buf);

public:
	MessageSender(int host_port, const char* host_name);
	~MessageSender();

	void SetHostPort(int host_port);
	void SetHostName(const char* host_name);

	int GetHostPort();
	const char* GetHostName();

	int SendMessage(T *message);
    Q ReceiveResponse(int hsock);
	void CloseConnection(int hsock);
};

#include "MessageSender.inc"