#include "remoteCom_client.hpp"

using namespace std;


linuxDomainSockerClient::linuxDomainSockerClient(const char* path){
	strncpy(socket_path, path, sizeof(path));
	socketfile = socket(AF_UNIX, SOCK_STREAM, 0);
	if ( socketfile == -1) {
		perror("socket error");
	}
	memset(&local, 0, sizeof(struct sockaddr_un));
	local.sun_family = AF_UNIX;
	strncpy(local.sun_path, socket_path, sizeof(local.sun_path) - 1);
}

linuxDomainSockerClient::~linuxDomainSockerClient(){
	
}

int linuxDomainSockerClient::Init(const char* path){
	strncpy(socket_path, path, sizeof(path));
	socketfile = socket(AF_UNIX, SOCK_STREAM, 0);
	if ( socketfile == -1) {
		perror("socket error");
	}
	memset(&local, 0, sizeof(struct sockaddr_un));
	local.sun_family = AF_UNIX;
	strncpy(local.sun_path, socket_path, sizeof(local.sun_path) - 1);
}

int linuxDomainSockerClient::connectToServer(){	
	int ret = connect(socketfile, (const struct sockaddr *)&local, sizeof(struct sockaddr_un));
	if ( ret == -1) {
        perror("connect");
    }
}

int linuxDomainSockerClient::closeConnection(){
	close(socketfile);
	return 0;
}


int linuxDomainSockerClient::sendToServer(){
	int ret = send(socketfile, txbuf, strlen(txbuf)+1, MSG_NOSIGNAL);
	//int ret = write(socketfile, txbuf, strlen(txbuf)+1);
	if (ret == -1) {
		perror("failed to send");        
	}
}

int linuxDomainSockerClient::readFromServer(){
	int t=recv(socketfile, rxbuf, R_BUFFER_SIZE, 0);	
	//int t=read(socketfile, rxbuf, R_BUFFER_SIZE);	
	if (t < 0) perror("recv");
	return t;
}

int linuxDomainSockerClient::setTxBuf( string str){	
	int templen = sizeof(str);
	if (templen > R_BUFFER_SIZE ){
		strncpy(txbuf,str.c_str(),R_BUFFER_SIZE);
	}
	else{
		strncpy(txbuf,str.c_str(),R_BUFFER_SIZE);
	}	
	return 0;	
}

string linuxDomainSockerClient::getRxBuf(){
	char tempbuf[R_BUFFER_SIZE];
	strncpy(tempbuf, rxbuf,R_BUFFER_SIZE);
	string tempstr = tempbuf;
	return tempstr;
}


