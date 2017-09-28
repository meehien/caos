.PHONY: all cpp clean

all:    Main.cpp protobuf
	pkg-config --cflags protobuf
	g++ *.cpp message.pb.cc -o map_check -std=c++11 -lcrypto `pkg-config --cflags --libs protobuf`

clean:
	rm -f message.pb.cc message.pb.h
	rm -f map_check

protobuf: message.proto
	protoc --cpp_out=. message.proto

