
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <iostream>  
#include <stdlib.h>
#include "../headers/remoteCom.hpp"
#include "../headers/messageInterpreter.hpp"


int quitPthread(pthread_mutex_t *mtx)
{
  switch(pthread_mutex_trylock(mtx)) {
    case 0: /* if we got the lock, unlock and return 1 (true) */
      pthread_mutex_unlock(mtx);
      return 1;
    case EBUSY: /* return 0 (false) if the mutex was locked */
      return 0;
  }
  return 1;
}

typedef struct  {
    pthread_t *thread;
    pthread_mutex_t * thread_terminator;
    linuxDomainSockerServer *socketserver;
}server_thread_management_obj;

typedef struct  {
    int idx;
    linuxDomainSockerServer *socketserver;
}client_thread_management_obj;

void *accepter_P_thread(void *accepterInfo){
    pthread_t *accepter = ((server_thread_management_obj *)accepterInfo)->thread;
    pthread_mutex_t *accepter_quit = ((server_thread_management_obj *)accepterInfo)->thread_terminator;
    linuxDomainSockerServer *server = ((server_thread_management_obj *)accepterInfo)->socketserver;
    while( !quitPthread(accepter_quit) ) {
        server->acceptNewConnections();
    } 
    pthread_exit(NULL);  
}

void startAccepter( server_thread_management_obj *accepterInfo){
    pthread_t *accepter = ((server_thread_management_obj *)accepterInfo)->thread;
    pthread_mutex_t *accepter_quit = ((server_thread_management_obj *)accepterInfo)->thread_terminator;
    linuxDomainSockerServer *server = ((server_thread_management_obj *)accepterInfo)->socketserver;
    pthread_mutex_init(accepter_quit,NULL);
    pthread_mutex_lock(accepter_quit);
    pthread_create(accepter,NULL,accepter_P_thread,(void *) accepterInfo);
    printf("accepter thread started\n");
}

void stopAccepter(server_thread_management_obj *accepterInfo){ 
    pthread_t *accepter = ((server_thread_management_obj *)accepterInfo)->thread;
    pthread_mutex_t *accepter_quit = ((server_thread_management_obj *)accepterInfo)->thread_terminator;
    linuxDomainSockerServer *server = ((server_thread_management_obj *)accepterInfo)->socketserver;  
    pthread_mutex_unlock(accepter_quit);     
    pthread_cancel(*accepter);
    pthread_join(*accepter,NULL);
    printf("accepter thread terminated\n");
}

void *clientCon_P_thread(void *clientInfo){
      
    linuxDomainSockerServer *server = ((client_thread_management_obj *)clientInfo)->socketserver;
    int clientId = ((client_thread_management_obj *)clientInfo)->idx;
    int ret = 0;
    int message_number = 0;

    string start_message = "client "+ to_string(clientId) + " thread starting \n";  
    printf("%s",start_message.c_str());

    ClientMessageInterpreter clientmessages(clientId);
    CilentMessageGenerator clientgenerator(clientId);

    while( ret >=0 || ret == 33)  {
        string message = "1";
        message += (char)((message_number >> 8) & 0xFF);
        message += (char)((message_number >> 8) & 0xFF);
        message += (char)((message_number >> 8) & 0xFF);
        message += (char)((message_number >> 8) & 0xFF);
        message += "server message: "+ to_string(message_number) +" to client " + to_string(clientId) ;
        clientgenerator.generateMessages(clientId, message_number, message);
        //message = clientgenerator.getOneTxQueueAndPop(); 
        cout << message << endl;
        server->setClientTxBuf( clientId, message);
        ret = server->sendToClient( clientId );
        if(ret >= 0 ){
            ret = server->readFromClient( clientId );
            message = server->getClientRxBuf(clientId ) ;
            clientmessages.insertMessage( clientId, message);
            cout << message << endl;
        }
        message_number++;
    }
    string termination_message = "client "+ to_string(clientId) + " thread terminating \n";
    printf("%s",termination_message.c_str());
    return 0;  
}


int main(int argc, char *argv[]) {
  

pthread_t accepter;
pthread_mutex_t accepter_quit;
linuxDomainSockerServer socketserver("soc");

server_thread_management_obj server_accepter_manager;

server_accepter_manager.thread = &accepter;
server_accepter_manager.thread_terminator = &accepter_quit;
server_accepter_manager.socketserver = &socketserver;

vector<client_thread_management_obj> clientobjs;

    cout << "server started" << endl;
  int loop = 1;
  string temp = "y";
   vector<int> connectsidx;
  startAccepter( &server_accepter_manager );

  while( loop < 100000 && temp =="y"){
    
    connectsidx = socketserver.getLiveSockets();
    if(connectsidx.size() > 0){
        for (int i = 0; i < connectsidx.size(); i++){
            int idx = connectsidx[i];
            if ( socketserver.getClientThreadStatus(idx) == 0){
                client_thread_management_obj tempObj;
                pthread_t tempThread;
                tempObj.socketserver = &socketserver;
                tempObj.idx = idx;
                socketserver.setClientPthread(idx, &tempThread);
                clientobjs.push_back(tempObj);
                pthread_create( socketserver.getClientPthread(idx),NULL,clientCon_P_thread, (void *) &clientobjs[clientobjs.size()-1] );
            }           
        }        
    }
    
    loop++;
    if(loop % 100 ==0){
        cout << "Run another loop (y/n)" << endl;
        cin >> temp;
    }
  }

  cout << "loop ended" <<endl;
  connectsidx = socketserver.getLiveSockets();
  
    for (int idx = 0; idx < connectsidx.size(); idx++){
        if ( socketserver.getClientThreadStatus(idx) == 1){
            pthread_join(*(socketserver.getClientPthread(idx)),NULL);
        }    
    }
    
    
  stopAccepter( &server_accepter_manager);
  return 0;
}


