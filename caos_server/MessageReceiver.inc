#include "message.pb.h"
#include "Log.hpp"
#include <errno.h>
#include <iostream>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include <unistd.h>

template<class T, class Q>
MessageReceiver<T, Q>::MessageReceiver(int listen_port) {
	this->listen_port = listen_port;
}

template<class T, class Q>
MessageReceiver<T, Q>::MessageReceiver(int listen_port, char* listen_ip) {
	this->listen_port = listen_port;
	this->listen_ip = listen_ip;
}

template<class T, class Q>
MessageReceiver<T, Q>::~MessageReceiver() { }

template<class T, class Q>
void MessageReceiver<T, Q>::SetListenPort(int listen_port) {
	this->listen_port = listen_port;
}

template<class T, class Q>
void MessageReceiver<T, Q>::SetListenIp(char *listen_ip) {
	this->listen_ip = listen_ip;
}

template<class T, class Q>
int MessageReceiver<T, Q>::GetListenPort() {
	return listen_port;
}

template<class T, class Q>
void MessageReceiver<T, Q>::Initialize(){
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1){
		Log::Write(Log::WARNING, "Error initializing socket %d", errno);
		throw;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;

	if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )||
		(setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) ){
		Log::Write(Log::WARNING,"Error setting options %d", errno);
		free(p_int);
		throw;
	}
	free(p_int);

	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(listen_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY ;

	if( bind( hsock, (sockaddr*)&my_addr, sizeof(my_addr)) == -1 ){
		Log::Write(Log::WARNING,"Error binding to socket, make sure nothing else is listening on this port %d",errno);
		throw;
	}
	if(listen( hsock, 10) == -1 ){
		Log::Write(Log::WARNING, "Error listening %d",errno);
		throw;
	}

	addr_size = sizeof(sockaddr_in);
}

template<class T, class Q>
void MessageReceiver<T, Q>::Listen(void* (*process)(void* lp)){

	Initialize();
	Log::Write(Log::INFO,"Listening for connections");

	while(1){
		//Log::Write(Log::INFO,"Listening for connections");
		csock = (int*)malloc(sizeof(int));
		if((*csock = accept(hsock, (sockaddr*)&sadr, &addr_size))!= -1){
				//Log::Write(Log::INFO,"Client connection from %s",inet_ntoa(sadr.sin_addr));
				pthread_create(&thread_id,0,process, (void*)csock);
				pthread_detach(thread_id);
		}
		else{
				Log::Write(Log::WARNING, "Error accepting %d", errno);
				close(*csock);
		}
	}
}

template<class T, class Q>
T MessageReceiver<T, Q>::Receive() {

	Initialize();
	Log::Write(Log::INFO,"Listening for connections");

	while(1){
		//Log::Write(Log::INFO,"Listening for connections");
		csock = (int*)malloc(sizeof(int));
		if(!strcmp(inet_ntoa(sadr.sin_addr), listen_ip) && (*csock = accept( hsock, (sockaddr*)&sadr, &addr_size))!= -1){
				return SocketHandler((void*)csock);
		}
		else{
				Log::Write(Log::WARNING, "Error accepting %d", errno);
				close(*csock);
		}
	}
}

template<class T, class Q>
google::protobuf::uint32 MessageReceiver<T, Q>::readHdr(char *buf)
{
	google::protobuf::uint32 size;
	google::protobuf::io::ArrayInputStream ais(buf,4);
	google::protobuf::io::CodedInputStream coded_input(&ais);
	coded_input.ReadVarint32(&size);//Decode the HDR and get the size
	return size;
}

template<class T, class Q>
T MessageReceiver<T, Q>::readBody(int csock,google::protobuf::uint32 siz)
{

	int bytecount;
	T payload;
	char buffer [siz+4];//size of the payload and hdr
	//Read the entire buffer including the hdr
	if((bytecount = recv(csock, (void *)buffer, 4+siz, MSG_WAITALL))== -1){
		Log::Write(Log::WARNING, "Error receiving data %d", errno);
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
	//Print the message
	return payload;
}

template<class T, class Q>
T MessageReceiver<T, Q>::SocketHandler(void* lp){
	int *csock = (int*)lp;

	char buffer[4];
	int bytecount=0;
	std::string output,pl;

	memset(buffer, '\0', 4);

	while (1) {
		//Peek into the socket and get the packet size
		if((bytecount = recv(*csock, buffer, 4, MSG_PEEK))== -1){
				Log::Write(Log::WARNING, "Error receiving data %d", errno);
				close(*csock);
		}else if (bytecount == 0)
				break;
		return readBody(*csock,readHdr(buffer));
	}
}

template<class T, class Q>
void MessageReceiver<T, Q>::SendResponse(void* lp, Q message){
	int *csock = (int*)lp;

	siz = message.ByteSize()+4;
	pkt = new char [siz];

	Serialize(&message, siz, pkt);

	Log::Write(Log::DEBUG, "Responding to request");
	buffer_len=0;
	if( (bytecount=send(*csock, (void *) pkt,siz,0))== -1 ) {
		Log::Write(Log::WARNING, "Error sending data %d", errno);
		close(*csock);
	}
	usleep(1);
//    delete pkt;
}

template<class T, class Q>
void MessageReceiver<T, Q>::Serialize(Q *message, int siz, char *pkt){
	google::protobuf::io::ArrayOutputStream aos(pkt,siz);
	google::protobuf::io::CodedOutputStream *coded_output = new google::protobuf::io::CodedOutputStream(&aos);
	coded_output->WriteVarint32(message->ByteSize());
	message->SerializeToCodedStream(coded_output);
}