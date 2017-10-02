# Implementation of CAOS: Concurrent-Access Obfuscated Store.

> A secure cloud storage construction that acheives access pattern obfuscation in which both bandwidth overheads and client storage requirements are very low. Both the access pattern obfuscation and the concurrent access are provided without a trusted third party.

This is an implementation of the CAOS obfuscation scheme outlined in https://arxiv.org/abs/1709.10412.

This repository contains implementations of several storage algorithms, currently:

  - CAOS client, obfuscation client and server
  - Map test client and scripts for CAOS
  - An implementation of PathORAM

CAOS is implemented in a client-server model, while pathORAM is implemented as a local store only.
Both rely on the same underlying block filesystem.

## CAOS
Contained within ```/caos_client```,```/caos_server``` is a C++ implementation of the CAOS scheme.

### Requirements
  - GCC
  - Google Protobuf (libprotobuf-dev and protobuf-compiler in Ubuntu)
  - LibSSL (libssl-dev in Ubuntu)
  - pkg-config

### Usage

Easyest method is to start with the ```run_server.sh```, ```run_rwclient.sh``` and ```run_occontinous.sh``` scripts in ```/caos_testscripts```. If you want to add specific files read the client's commandline is outlined below.

#### Server:
To use CAOS start by compiling the server in ```/caos_server``` with ```make``` and run the resulting ```caos_server``` binary. command line flags are as follows:
```
	caos_server --listen	listens on port 12345 for connections. A custom port can be manually specified after listen if desired.
	aRAM --debug --*            runs Caos in debug mode
```

#### Client:
Compile the clinet located in ```caos_client``` using ```make```. The Caos client will use the ```localhost``` (127.0.0.1) IP address by default and port ```12345```. If one desires to use a different IP address (or port), this can be changed in Main.cpp inside ```caos_client```. The client's command line is as follows:


```
   caos_client --writer --add <filename> -> adds <filename> to Caos using RW client
   caos_client --writer --get<filename1> <filename2> -> gets <filename1> from Caos and saves it to <filename2> using RW client
   caos_client --reader --get <filename1> <filename2> -> gets <filename1> from Caos and saves it to <filename2> using RO client
   caos_client --agent <num1> <num2> -> runs Caos in agent mode <num1> times with <num2> buffer size
   caos_client --debug --* -> truns Caos in debug mode
```



### Configuration options
The Caos protocol has several the following configuration values which can be modified:
  - The block size of the store ``blockSize`` can be configured in ``Main.cpp`` *(values in KB)*
  - The size of the store ``storeSize`` can be configured in ``Main.cpp`` *(values in MB)*
  - The number of clients accessing the filesystem ``NUM_CLIENTS`` can be configured in ``Types.hpp``

These values must match between the client and the server versions (i.e. server only has ``blockSize`` and ``storeSize``).


## MapCheck
Contained within ```/map_check``` is a psuedo Caos client designed to test the status of a client's map with respect to it's store.

The program runs three tests on the map:

- Map Validity

	Checks how many of the positions in the map contain the block the map states that they should. This test checks each non-empty position from the store (according to the local map) and returns the percentage of valid positions out of the total checked.

- Map Knowledge

	This test determines how positions from the store are listed in the given map and weather the map lists the correct block for that position. This test differs from 1 as it is inclusive of non-empty positions from the store which are not listed at all in the map. The test returns a percentage of all the non-empty positions in the store which are listed in the map and are valid.

- Hit Rate

	This test determines how many attempts, on average, must be made by a client using a given map before it is able to successfully retrieve a block. This is determined in main by test 1, however as with each request clients improve their map knowledge of the store. The test accesses each block a fixed number of times (50 by default) and calculates the average number of attempts that were successfull for each access. To prevent the modification of the store an intermediate store is used to cache modified positions for the duration of each access.

Tests carried out by this program do not modify either the map or the file store.

### Requirements
  - GCC
  - Google Protobuf
  - LibSSL
  - pkg-config

### Setup
Compile the project with ```make``` and run the resulting ```map_check``` binary. Command line flags are as follows:

```
map_check [--debug] <map filename>
```

MapCheck requires access to the filesystem corresponding to the given map. Server must be running while ```map_check``` is executed.

### Configuration
The configuration values in ```map_check``` must match those of the client and server. These can be modified as follows:
  - The block size of the filesystem ``blockSize`` can be configured in ``Main.cpp`` *(values in KB)*
  - The size of the filesystem ``storeSize`` can be configured in ``Main.cpp`` *(values in MB)*
  - The type of filesystem can also be selected in ``Main.cpp`` *(uncomment desired filesystem)*
