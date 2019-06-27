# hyperscan runtime rule change and threaded sockets

## Description
This project includes following
- A method to change hyperscan rule database at runtime using shared memories and flags\. 
- A threaded Unix socket server that can accept and communicate with multiple clients simultaneously\.

## Getting Started

Headers : header files for support functions.
socketServer : unix socket server with multi threading
socketClient : unix socket client for testing.
hs_filter.cpp : hyperscan pcap filter. created using hyperscan testbench
hs_db_compiler.cpp : hyperscan runtime rule compiler and shared memory based database placer
Makefile : make file with all the make commands

### Prerequisites

Linux development enviornemt with Hyperscan, lpthread\.

### Installing
Compiling socket client and server
``` 
#create shm file handler
make create_files 
make compile_socket_s
make compile_socket_c
``` 
Above commands will create the server and client executables.

Compiling hyperscan filter and compiler

``` 
make compiler
make filter
``` 
## Running the tests
Runing unix socket server and client
``` 
make create_files 
./server
./client
``` 

Running hyperscan filter and compiler
Requires the to define snort_literals file location and pcap file location in Makefile

``` 
make compiler
make filter
``` 

## License
[MIT](https://choosealicense.com/licenses/mit/)
hs_filter.cpp contains a additional intel licence

## Acknowledgments

[Hyperscan](https://01.org/hyperscan) 
