
#include "remote_pthread.hpp"

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
