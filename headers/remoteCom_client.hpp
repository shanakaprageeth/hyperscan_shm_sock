#ifndef REMOTECOMCLIENT_H
#define REMOTECOMCLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <cstring>
#include <unistd.h>
#include <string> 
#include <iostream>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <vector> 
#define R_BUFFER_SIZE 200

using namespace std;

class linuxDomainSockerClient{
private:
    struct sockaddr_un local;    
    int socketfile;
    char rxbuf[R_BUFFER_SIZE];
    char txbuf[R_BUFFER_SIZE];
    char socket_path[108] = "";
public:
    linuxDomainSockerClient();
    linuxDomainSockerClient(const char*  path);
    ~linuxDomainSockerClient();
    int Init(const char*  path);
    int connectToServer();
    int closeConnection();
    int sendToServer( );
    int readFromServer( );
    int setTxBuf(string str);
    string getRxBuf();
};


#endif