#pragma once
#include <google/protobuf/message.h>
#include <netinet/in.h>
#include <resolv.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

template<class T, class Q>
class MessageReceiver{
	int listen_port;
	char *listen_ip;

	int hsock;
	int * p_int ;
	int err;

	socklen_t addr_size;
	int* csock;
	sockaddr_in sadr;
	pthread_t thread_id;

	char buffer[1024];
	int bytecount;
	int buffer_len;;

	int siz;
	char *pkt;

	struct sockaddr_in my_addr;

	void Initialize();
	T readBody(int csock,google::protobuf::uint32 siz);
	T SocketHandler(void* lp);

public:
	MessageReceiver(int listen_port);
	MessageReceiver(int listen_port, char *listen_ip);
	~MessageReceiver();

	google::protobuf::uint32 readHdr(char *buf);

	void SetListenPort(int listen_port);
	void SetListenIp(char *listen_ip);

	int GetListenPort();

	T Receive();

	void Listen(void* (*handler)(void* lp));
	void SendResponse(void* lp, Q message);

	void Serialize(Q *message, int siz, char *pkt);
};

#include "MessageReceiver.inc"