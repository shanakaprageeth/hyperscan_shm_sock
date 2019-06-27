#include "service_message_manager.hpp"
serviceMessageManager::serviceMessageManager(){
	
}
serviceMessageManager::~serviceMessageManager(){
	
}
int serviceMessageManager::insertRequest(int inputClientID, int type, string inputbuffer){
	service_request req;
	req.clientID = inputClientID;
	req.state =  IN_USE;
	req.request = inputbuffer;
	req.type = type;
	pthread_mutex_lock(&service_request_lock);
	service_requests.push_back(req);
	pthread_mutex_unlock(&service_request_lock);
}

int serviceMessageManager::getRequest(service_request *serv_request, int *idx){
	for (int i=0; i<service_requests.size(); i++) {		
		if (service_requests[i].state == IN_USE){
			*serv_request = service_requests[i];
			*idx = i;
			return 0;
		}
	}
	return 1;
}

int serviceMessageManager::changeState(int idx, int state){
	
	pthread_mutex_lock(&service_request_lock);	
	service_requests[idx].state = state;
	pthread_mutex_unlock(&service_request_lock);
		
}

int serviceMessageManager::removeRequest(int idx){
	if(service_requests[idx].state == EXECUTED){
		pthread_mutex_lock(&service_request_lock);	
		service_requests.erase(next(begin(service_requests), idx));
		pthread_mutex_unlock(&service_request_lock);
	}	
}

int serviceMessageManager::insertReply(int inputClientID, int state, int type, string inputbuffer){
	service_request req;
	req.clientID = inputClientID;
	req.state =  state;
	req.type =  type;
	req.request = inputbuffer;
	pthread_mutex_lock(&service_reply_lock);
	service_replys.push_back(req);
	pthread_mutex_unlock(&service_reply_lock);
}

int serviceMessageManager::getReply(int clientID, service_request *serv_reply, int *idx){			
	for (int i =0 ; i < service_replys.size(); i++){
		*idx = i;
		*serv_reply = service_replys[i];
		return 0;
	}	
	return 1;
}

int serviceMessageManager::removeReply(int idx){	
	pthread_mutex_lock(&service_reply_lock);	
	service_replys.erase( next(begin(service_replys), idx));
	pthread_mutex_unlock(&service_reply_lock);		
}

int serviceMessageManager::addAppDetail(app_detail app_det){	
	while  (app_det.appId > app_details.size()){
		app_detail tempdet;
		app_details.push_back(tempdet);
	}
	if(app_det.appId < app_details.size())
		app_details[app_det.appId] = app_det;
	else
		app_details.push_back(app_det);

}

int serviceMessageManager::getNoOfApps(){
	return app_details.size();
}

int serviceMessageManager::initServBuffer(int appId){
	int options = PROT_READ | PROT_WRITE;
    int mode = MAP_SHARED;
    int file_size = sizeof(circular_buffer);                
    app_details[appId].serv_mmap = (char *)set_up_mmap(app_details[appId].serv_file_loc, file_size, options, mode); 
    app_details[appId].serv_buffer= (circular_buffer *)(app_details[appId].serv_mmap);
    CB_init(app_details[appId].serv_buffer); 
}

circular_buffer * serviceMessageManager::getServBuffer(int appId){
	return  app_details[appId].serv_buffer;
}