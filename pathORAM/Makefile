CC=g++
CFLAGS=-c -Wall --std=c++11
LDFLAGS=-lcrypto
SOURCES=$(wildcard *.cpp)
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=pathORAM

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf *.o *.bin *.map $(EXECUTABLE)
