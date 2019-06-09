#ifndef __HTTP_SERVER_H_
#define __HTTP_SERVER_H_

#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/http.h"
#include <string>
#include "CEventBuffer.h"
#include <unordered_map>
#include <log4cplus/logger.h>
#include <thread>
#include <atomic>

class TSiaManager;

using namespace std;


class SiaHttpServer {
public:
	SiaHttpServer(const char *ip, const int port, TSiaManager *own);
	~SiaHttpServer();

	void Start();
	void Stop();

protected:
	
	//////////END/////////////
	std::thread m_listenthread;
	void Http_Listen();
	void Http_Callback_handle(struct evhttp_request *req);
	friend void Sia_http_CallBack_fun(struct evhttp_request *req, void *arg); //外部链接回调函数

private:
	TSiaManager *   m_pOwn;
	string          m_localip;
	uint32_t        m_port;
	log4cplus::Logger log;

	struct event_base * m_pBase ;
	struct evhttp * m_pHttp_server ;

};

#endif
