#ifndef REMOTECOM_H
#define REMOTECOM_H

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
#include <pthread.h> //for threading , link with lpthread
#define R_BUFFER_SIZE 200

using namespace std;

typedef struct{
    int socketCon;
    socklen_t length;
    struct sockaddr_un remote;
    char rxbuf[R_BUFFER_SIZE];
    char txbuf[R_BUFFER_SIZE];
    int con_live=0;
    int threadAssigned=0;
    pthread_t *client_thread;
}clientdata;


class linuxDomainSockerServer{
private:
    struct sockaddr_un local;    
    int socketfile;
    vector<clientdata> allclientdata;    
    char socket_path[108] = "";
public:
    linuxDomainSockerServer(const char*  path, int waitng_connections = 5);
    ~linuxDomainSockerServer();
    int acceptNewConnections();
    vector<int> getLiveSockets();  
    int closeConnection(int idx);
    int sendToClient(int idx);
    int readFromClient(int idx);
    int setClientTxBuf(int idx, string str);
    int setClientPthread(int idx, pthread_t *inclient_thread);
    pthread_t * getClientPthread(int idx);
    int quitClientPthread(int idx);
    int getClientThreadStatus(int idx);
    string getClientRxBuf(int idx);
};

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