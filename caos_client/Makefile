.PHONY: all cpp clean

all:    Main.cpp protobuf
	pkg-config --cflags protobuf
	g++ *.cpp message.pb.cc -o caos_client -std=c++11 -fopenmp -lcrypto `pkg-config --cflags --libs protobuf`

clean:
	rm -f server client
	rm -f message.pb.cc message.pb.h
	rm -f *.map
	rm -f buffer.bin
	rm -f store.bin
	rm -f caos_client

cleanmeta:
	rm -f *.map
	rm -f buffer.bin
	rm -f store.bin

protobuf: message.proto
	protoc --cpp_out=. message.proto

