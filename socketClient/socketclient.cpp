
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>  
#include <unistd.h>
#include "../headers/remoteCom.hpp"
#include "../headers/messageInterpreter.hpp"


int main(int argc, char *argv[]) {
  

  linuxDomainSockerClient socketclient("soc");
 cout << "Socket created" << endl;
  socketclient.connectToServer();
  cout << "Socket connected" << endl;
int loop = 1;
string temp;
//ClientMessageInterpreter clientmessages(0);
  while( loop < 12 ){//loop == "y") {
    loop++;
    //cout << "Read from Server" << endl;
    //cin >> temp;    
    socketclient.readFromServer( );
    string message = socketclient.getRxBuf();
    //printf("1");
    //clientmessages.insertMessage( 0, message);
    //printf("2");
    //message = clientmessages.completeRequestAndPop();
    //printf("3");
    cout <<  " recived " <<  message << endl;
    //printf("4");
    //cout << "Enter string to send" << endl;
    //cin >>temp;
    socketclient.setTxBuf(message);
    socketclient.sendToServer();
    //cout << "Run another loop (y/n)" << endl;
    //cin >> loop;

  }

  return 0;
}



