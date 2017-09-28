.PHONY: all cpp clean

all:    Main.cpp protobuf
	pkg-config --cflags protobuf
	g++ *.cpp message.pb.cc -o caos_server -std=c++11 -lcrypto `pkg-config --cflags --libs protobuf`

clean:
	rm -f server client
	rm -f message.pb.cc message.pb.h
	rm -f store.bin
	rm -f caos_server

cleanmeta:
	rm -f store.bin

protobuf: message.proto
	protoc --cpp_out=. message.proto
