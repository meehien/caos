#include "Log.hpp"
#include <unistd.h>
#include "message.pb.h"
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <google/protobuf/io/coded_stream.h>

template<class T, class Q>
MessageSender<T,Q>::MessageSender(int host_port, const char* host_name){
	this->host_port = host_port;
	this->host_name = host_name;
}

template<class T, class Q>
MessageSender<T,Q>::~MessageSender(){};

template<class T, class Q>
void MessageSender<T,Q>::SetHostPort(int host_port){
	this->host_port = host_port;
}

template<class T, class Q>
void MessageSender<T,Q>::SetHostName(const char* host_name){
	this->host_name = host_name;
}

template<class T, class Q>
int MessageSender<T,Q>::GetHostPort(){
	return this->host_port;
}

template<class T, class Q>
const char* MessageSender<T,Q>::GetHostName(){
	return this->host_name;
}

template<class T, class Q>
int MessageSender<T,Q>::SendMessage(T *message){

	siz = message->ByteSize()+4;
	pkt = new char [siz];

	Serialize(message, siz, pkt);

	buffer_len=0;

	int hsock = socket(AF_INET, SOCK_STREAM, 0);
	if (hsock == -1) {
		Log::Write(Log::WARNING, "Error initializing socket %d", errno);
		CloseConnection(hsock);
	}

	p_int = (int *) malloc(sizeof(int));
	*p_int = 1;

	if ((setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char *) p_int, sizeof(int)) == -1) ||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char *) p_int, sizeof(int)) == -1)) {
		Log::Write(Log::WARNING, "Error setting options %d", errno);
		free(p_int);
		CloseConnection(hsock);
	}
	free(p_int);

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = inet_addr(host_name);
	if (connect(hsock, (struct sockaddr *) &my_addr, sizeof(my_addr)) == -1) {
		if ((err = errno) != EINPROGRESS) {
			Log::Write(Log::WARNING, "Error connecting socket %d", errno);
			CloseConnection(hsock);
		}
	}

	if ((bytecount = send(hsock, (void *) pkt, siz, 0)) == -1) {
		Log::Write(Log::WARNING, "Error sending data %d", errno);
		CloseConnection(hsock);
	}

	usleep(1);

	delete pkt;
	return hsock;
}

template<class T, class Q>
Q MessageSender<T,Q>::ReceiveResponse(int hsock){

	int *bytecount = (int*)malloc(sizeof(int));
	Q payload;
	char buffer [4];//size of the payload and hdr
	//Read the entire buffer including the hdr
	if((*bytecount = recv(hsock, (void *)buffer, 4, MSG_PEEK))== -1){
		Log::Write(Log::WARNING, "Error receiving response %d", errno);
	}

	payload = readBody(hsock,readHdr(buffer));
	CloseConnection(hsock);
	return payload;
};

template<class T, class Q>
void MessageSender<T,Q>::Serialize(T *message, int siz, char *pkt){
	google::protobuf::io::ArrayOutputStream aos(pkt,siz);
	google::protobuf::io::CodedOutputStream *coded_output = new google::protobuf::io::CodedOutputStream(&aos);
	coded_output->WriteVarint32(message->ByteSize());
	message->SerializeToCodedStream(coded_output);
	delete coded_output;
}

template<class T, class Q>
void MessageSender<T,Q>::CloseConnection(int hsock){
	Log::Write(Log::DEBUG, "Closing socket");
	close(hsock);
}

template<class T, class Q>
Q MessageSender<T, Q>::readBody(int csock,google::protobuf::uint32 siz)
{
	int bytecount = 0;
	Q payload;
	char buffer [siz+4];//size of the payload and hdr
	//Read the entire buffer including the hdr
	if((bytecount = recv(csock, (void *)buffer, siz+4, MSG_WAITALL))== -1){
		Log::Write(Log::WARNING, "Error receiving data %d", errno);
	}
	//Assign ArrayInputStream with enough memory
	//TODO: +4?
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

	return payload;
}

template<class T, class Q>
Q MessageSender<T, Q>::SocketHandler(void* lp){
	int *csock = (int*)lp;

	char buffer[4];
	int bytecount=0;
	std::string output,pl;

	memset(buffer, '\0', 4);

	while (1) {
		//Peek into the socket and get the packet size
		if((bytecount = recv(*csock,
							 buffer,
							 4, MSG_PEEK))== -1){
			Log::Write(Log::WARNING, "Error receiving data %d", errno);
		}else if (bytecount == 0)
			break;

		return readBody(*csock,readHdr(buffer));
	}
}

template<class T, class Q>
google::protobuf::uint32 MessageSender<T, Q>::readHdr(char *buf)
{
	google::protobuf::uint32 size;
	google::protobuf::io::ArrayInputStream ais(buf,4);
	google::protobuf::io::CodedInputStream coded_input(&ais);
	coded_input.ReadVarint32(&size);//Decode the HDR and get the size

	return size;
}