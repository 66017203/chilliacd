#ifndef __HTTPCLIENTMODULE_HEADER__
#define __HTTPCLIENTMODULE_HEADER__
#include <event2/event.h>
#include <log4cplus/logger.h>
#include <thread>
#include <chrono>
#include "CEventBuffer.h"
#include "HttpRequest.h"
#include <atomic>

class TSiaManager;
class HttpClientThread {
public:
	explicit HttpClientThread(class HttpClientModule * own);
	virtual ~HttpClientThread();
private:
	log4cplus::Logger log;
	struct event_base * m_Base;
	std::thread			m_thread;
	static void processRequest(evutil_socket_t fd, short event, void *arg);
	void ProcessRequest();
	void Run();

	HttpClientModule * const m_Own = nullptr;
	friend class HttpClientModule;
};

class HttpClientModule{
public:
	explicit HttpClientModule(TSiaManager * own);
	virtual ~HttpClientModule();

	void Start();
	void Stop();
	void PutMessage(const std::string & uri, const std::string & sessionid, const std::string & cmd, const std::string & data, HttpRequest::http_method method, const HttpRequest::HeaderType & header, int timeout);

protected:
	TSiaManager		  * m_pOwn;
private:
	const uint32_t m_MaxThreadSize = 2048;
	std::vector<std::shared_ptr<HttpClientThread>> m_threads;
	std::atomic<uint32_t> m_Idlethread;
	helper::CEventBuffer<HttpRequestPtr> m_HttpRequests;
protected:
	void DelayRequest(HttpRequestPtr req);
	log4cplus::Logger log;
	friend class AsynHttpRequest;
	friend class HttpClientThread;

private:
	HttpClientModule(const HttpClientModule &) = delete;
	HttpClientModule & operator=(const HttpClientModule &) = delete;
};

#endif
