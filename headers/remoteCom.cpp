#include "remoteCom.hpp"

using namespace std;



linuxDomainSockerServer::linuxDomainSockerServer(const char* path, int waitng_connections){
	strncpy(socket_path, path, sizeof(path));
	unlink(socket_path);
	socketfile = socket(AF_UNIX, SOCK_STREAM, 0);
	if ( socketfile == -1) {
		perror("socket error");
	}
	memset(&local, 0, sizeof(struct sockaddr_un));
	local.sun_family = AF_UNIX;
	strncpy(local.sun_path, socket_path, sizeof(local.sun_path) - 1);
	int ret =  bind(socketfile, (const struct sockaddr*)&local, sizeof(struct sockaddr_un));
	if ( ret == -1) {
		perror("bind error");
	}
	ret = listen(socketfile, waitng_connections);
	if ( ret ) {
		perror("listen error");
	}
	clientdata tmpclientdata;
	allclientdata.push_back(tmpclientdata);

}

linuxDomainSockerServer::~linuxDomainSockerServer(){
	printf("unlinking server \n");
	for (int idx =0; idx<allclientdata.size();idx++ ){
		if(allclientdata[idx].con_live){
			if(allclientdata[idx].threadAssigned){
				allclientdata[idx].threadAssigned = 0;
				allclientdata[idx].con_live = 0;
				pthread_cancel(*(allclientdata[idx].client_thread));	
				pthread_join(*(allclientdata[idx].client_thread),NULL);	
				allclientdata[idx].client_thread = NULL;	
				printf("thread stopped \n");
			}
			close(allclientdata[idx].socketCon);
		}
	}
	unlink(socket_path);
}

int linuxDomainSockerServer::acceptNewConnections(){
	int temp_free_element = -1;
	int additem = 0;
	for (int idx =0 ; idx < allclientdata.size(); idx++){
		if(allclientdata[idx].con_live == 0){
			temp_free_element = idx;
			if( temp_free_element == allclientdata.size() -1 ){
				additem = 1;
			}
			break;
		}
	}
	
	allclientdata[temp_free_element].socketCon = accept(socketfile, (struct sockaddr *)&(allclientdata[temp_free_element].remote), &(allclientdata[temp_free_element].length));
	if( allclientdata[temp_free_element].socketCon == -1){	
		perror("failed to accept ");	
		return -1;
	}
	else{
		printf("new connection accepted %d \n",temp_free_element);
		allclientdata[temp_free_element].con_live = 1;
		if(additem)		{
			clientdata tmpclientdata;
			allclientdata.push_back(tmpclientdata);
		}		
		return temp_free_element;
	}
}



vector<int> linuxDomainSockerServer::getLiveSockets(){
	vector<int> allidx;	
	for (int idx =0 ; idx < allclientdata.size(); idx++){
		if(allclientdata[idx].con_live == 1){
			allidx.push_back(idx);
			//printf("size %d idx %d \r",allclientdata.size(), idx);
			//fflush(stdout);
		}
	}
	return allidx; 
}

int linuxDomainSockerServer::closeConnection(int idx){
	if (allclientdata[idx].con_live){
		if(allclientdata[idx].threadAssigned){
			allclientdata[idx].threadAssigned = 0;
		}
		close(allclientdata[idx].socketCon);
	}
	allclientdata[idx].con_live = 0;
	return 0;
}


int linuxDomainSockerServer::sendToClient(int idx){
	int ret = send(allclientdata[idx].socketCon, allclientdata[idx].txbuf, R_BUFFER_SIZE, MSG_NOSIGNAL);
	//int ret = write(allclientdata[idx].socketCon, allclientdata[idx].txbuf, R_BUFFER_SIZE);
	if (ret == -1) {
		closeConnection(idx); 
		perror("failed to send");
		return -1;   
	}
	return 0;
}

int linuxDomainSockerServer::readFromClient(int idx){
	int t=recv(allclientdata[idx].socketCon, allclientdata[idx].rxbuf, R_BUFFER_SIZE, 0);
	//int t=read(allclientdata[idx].socketCon, allclientdata[idx].rxbuf, R_BUFFER_SIZE);
	if (t == -1) {
		closeConnection(idx);
		perror("receive:");	
		return -1;
	}
	return t;
}

int linuxDomainSockerServer::setClientTxBuf(int idx, string str){	
	int templen = str.size();
	if (templen > R_BUFFER_SIZE ){
		strncpy(allclientdata[idx].txbuf,str.c_str(),R_BUFFER_SIZE);
	}
	else{
		strncpy(allclientdata[idx].txbuf,str.c_str(),templen);
	}
	return 0;	
}

int linuxDomainSockerServer::setClientPthread(int idx, pthread_t *inclient_thread){	
	allclientdata[idx].client_thread = inclient_thread;
	allclientdata[idx].threadAssigned = 1;
	return 0;
}

int linuxDomainSockerServer::quitClientPthread(int idx){		
	allclientdata[idx].threadAssigned = 0;
	return 0;
}

pthread_t * linuxDomainSockerServer::getClientPthread(int idx){	
	return allclientdata[idx].client_thread;
}

int linuxDomainSockerServer::getClientThreadStatus(int idx){
	return allclientdata[idx].threadAssigned;
}


string linuxDomainSockerServer::getClientRxBuf(int idx){
	char tempbuf[R_BUFFER_SIZE];
	strncpy(tempbuf,allclientdata[idx].rxbuf,R_BUFFER_SIZE);
	string tempstr = tempbuf;
	return tempstr;
}

linuxDomainSockerClient::linuxDomainSockerClient(){
}

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


