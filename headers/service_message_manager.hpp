#ifndef SERV_MSG_MAN_H
#define SERV_MSG_MAN_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <cstring>
#include <chrono>
#include <fstream>
#include <vector>
#include <string>   
#include <algorithm>    
#include <iostream>     
#include <sstream>  
#include <pthread.h>
#include "door_docker_api.h"


using namespace std;

#define MSG_BUFFER_SIZE 1024

enum msg_state {IN_USE, COMPLETED, EXECUTING, EXECUTED};

typedef struct service_request{
    int clientID;
    int state;
    int type;
    string request;
}service_request;

typedef struct app_detail{
    int appId;
    int key; 
    const char * serv_file_loc;   
    char *serv_mmap;
    circular_buffer *serv_buffer;
}app_detail;

class serviceMessageManager{
private:
    pthread_mutex_t service_request_lock;
    pthread_mutex_t service_reply_lock;
    vector<service_request> service_requests;    
    vector<service_request> service_replys;
    vector<app_detail> app_details;
public:
    serviceMessageManager();
    ~serviceMessageManager();
    int insertRequest(int inputClientID, int type, string inputbuffer);
    int getRequest(service_request *serv_request, int *idx);
    int changeState(int idx, int state);
    int removeRequest(int idx);
    int insertReply(int inputClientID, int state, int type, string inputbuffer);
    int getReply(int clientID, service_request *serv_reply, int *idx);
    int removeReply(int idx);
    int addAppDetail(app_detail app_det);
    int getNoOfApps();
    int initServBuffer(int appId);
    circular_buffer * getServBuffer(int appId);
};

#endif