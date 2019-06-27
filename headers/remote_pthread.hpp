#ifndef REMOTEPTHREADCOM_H
#define REMOTEPTHREADCOM_H

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>  
#include <stdlib.h>
#include "remoteCom.hpp"
#include "messageInterpreter.hpp"

typedef struct  {
    pthread_t *thread;
    pthread_mutex_t * thread_terminator;
    linuxDomainSockerServer *socketserver;
}server_thread_management_obj;

typedef struct  {
    int idx;
    linuxDomainSockerServer *socketserver;
}client_thread_management_obj;


int quitPthread(pthread_mutex_t *mtx);

void *accepter_P_thread(void *accepterInfo);

void startAccepter( server_thread_management_obj *accepterInfo);

void stopAccepter(server_thread_management_obj *accepterInfo);

void *clientCon_P_thread(void *clientInfo);

#endif