#include <iostream>
#include "SiaHttpServer.h"
#include "SiaManager.h"
#include "../acdd.h"
#include <log4cplus/loggingmacros.h>
#include "SiaTcpMonitor.h"

extern std::atomic<bool> g_Running;


SiaHttpServer::SiaHttpServer(const char *ip, const int port, TSiaManager *own)
	: m_pOwn(own), m_localip(ip), m_port(port), m_pBase(NULL), m_pHttp_server(NULL)
{
	log = log4cplus::Logger::getInstance("HttpServer");
}

//外部链接回调函数
void Sia_http_CallBack_fun(struct evhttp_request *req, void *arg)
{
	SiaHttpServer * handle = (SiaHttpServer *)arg;
	if (handle)
		handle->Http_Callback_handle(req);
	return	;
}
SiaHttpServer::~SiaHttpServer()
{
	if (m_listenthread.joinable()){
		Stop();
	}

	assert(m_pBase == NULL);
	assert(m_pHttp_server == NULL);
	assert(!m_listenthread.joinable());
}

void SiaHttpServer::Start()
{
	if (m_listenthread.joinable()){
		return;
	}
	//启动HTTP Server
	m_listenthread = std::thread(&SiaHttpServer::Http_Listen, this);

	return ;
}

void SiaHttpServer::Stop()
{
	if (m_listenthread.joinable()){
		
		if (m_pBase){
			event_base_loopbreak(m_pBase);
		}
		m_listenthread.join();
	}

}


void SiaHttpServer::Http_Listen()
{
	bool _exit = false;
	int ret = 0;
	LOG4CPLUS_DEBUG(log, "Start...");
	m_pBase = event_base_new();
	m_pHttp_server = evhttp_new(m_pBase);

	if (!m_pHttp_server){
		LOG4CPLUS_ERROR(log, " new http server failed");
		//m_pOwn->m_pMonitor->Alarm("SiaManager", "error", "listen ip or port error");
		goto done;
	}

	ret = evhttp_bind_socket(m_pHttp_server, m_localip.c_str(), m_port);
	if (ret != 0){
		LOG4CPLUS_ERROR(log, " bind ip=" << m_localip << ",port=" << m_port << " failed");
		//m_pOwn->m_pMonitor->Alarm("SiaManager", "error", "listen ip or port error");
		_exit = true;
		goto done;
	}

	evhttp_set_gencb(m_pHttp_server, Sia_http_CallBack_fun, this);

	LOG4CPLUS_INFO(log, "http server start OK! Bind IP=" << m_localip << ",Port=" << m_port);

	event_base_dispatch(m_pBase);

done:
	evhttp_free(m_pHttp_server);
	m_pHttp_server = NULL;

	event_base_free(m_pBase);
	m_pBase = NULL;

	if (_exit){
		LOG4CPLUS_ERROR(log, "System Exit...");
		g_Running = false;
		std::cin.clear();
	}
	LOG4CPLUS_DEBUG(log, "Stoped");
	log4cplus::threadCleanup();
	return;
}

void SiaHttpServer::Http_Callback_handle(struct evhttp_request *req)
{
	struct evbuffer * buf = evhttp_request_get_input_buffer(req);
	std::string strBuf;
	while (evbuffer_get_length(buf)) {
			int n;
			char cbuf[256];
			n = evbuffer_remove(buf, cbuf, sizeof(cbuf));
			if (n > 0)
				strBuf.append(cbuf, n);
	}

	if (!strBuf.empty())
	{
		LOG4CPLUS_TRACE(log, "recv msg,recv=" << strBuf);

		std::shared_ptr<CMessage> recvmsg(new CMessage());
		m_pOwn->SiaUnpackjson2Message(strBuf, recvmsg);

		m_pOwn->SendMessageToMe(std::move(recvmsg));
	}

	evhttp_send_reply(req, HTTP_OK, "OK",NULL);

}

