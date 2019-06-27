
# the compiler
CC=g++
# options 
#CFLAGS=-std=c++11 -c -I/usr/local/include/hs -L/usr/local/lib -lhs -lpthread -lpcap
CFLAGS=-std=c++11 -g -c -I/usr/local/include/hs

SOURCES_C=headers/circular_buffer.c headers/shared_memory_support.c
SOURCES_M=hs_db_compiler.cpp headers/RuleManager.cpp headers/remoteCom.cpp  headers/service_message_manager.cpp 
SOURCES_F=hs_filter.cpp headers/RuleManager.cpp headers/remoteCom.cpp  headers/service_message_manager.cpp 


OBJECTS_C=$(SOURCES_C:.c=.o)
OBJECTS_M=$(SOURCES_M:.cpp=.o)
OBJECTS_F=$(SOURCES_F:.cpp=.o)

#LDFLAGS=-std=c++11 -I/usr/local/include/hs -L/usr/local/lib -lhs -lpthread -lpcap
LDFLAGS=-L/usr/local/lib -lhs -lpthread -lpcap

EXECUTABLE_M=compiler
EXECUTABLE_F=filter

run_compiler: compiler
	./compiler snort_literals
run_filter: filter
	./filter ../../pcap_dump/2016_merged/2016_merge.pcap 


compiler: $(SOURCES_C) $(OBJECTS_M)  $(EXECUTABLE_M)

filter: $(SOURCES_C) $(OBJECTS_F)  $(EXECUTABLE_F)	

$(EXECUTABLE_M): $(OBJECTS_C) $(OBJECTS_M) 
	$(CC) $(OBJECTS_C) $(OBJECTS_M)  -o $@ $(LDFLAGS) 

$(EXECUTABLE_F): $(OBJECTS_C) $(OBJECTS_F) 
	$(CC) $(OBJECTS_C) $(OBJECTS_F)  -o $@ $(LDFLAGS) 

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

run_manager: clean all
	./manager 

debug_compile: 
	$(CC) -g -std=c++11 $(LDFLAGS) $(SOURCES) $(SOURCES_C) -o $@

run_gdb: clean $(EXECUTABLE_M)
	gdb --args $(EXECUTABLE_M)


clean:
	rm -f $(OBJECTS_M) $(OBJECTS_F)  $(OBJECTS_C) $(EXECUTABLE_M) $(EXECUTABLE_F)
	
compile_socket_s:
	g++ -std=c++11 -o server socketServer/socketserver.cpp headers/remoteCom.cpp headers/messageInterpreter.cpp -lpthread
compile_socket_c:
	g++ -std=c++11 -o client socketClient/socketclient.cpp headers/remoteCom.cpp headers/messageInterpreter.cpp -lpthread

create_files :
	echo "sample text" > fd  
	echo "sample text" > fd_1
	echo "sample text" > fd_controll
