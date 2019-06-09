#ifndef __TCPCLIENT_MODULE_HEADER_
#define __TCPCLIENT_MODULE_HEADER_

#include <event2/event.h>
#include <log4cplus/logger.h>
#include "CEventBuffer.h"
#include "json/json.h"
#include <atomic>
#include <thread>

class TSiaManager;
class TcpClientModule {
public:
	explicit TcpClientModule(TSiaManager * own);
	virtual ~TcpClientModule();

	virtual void Start(Json::Value address);
	virtual void Stop();
	bool IsConnected();
	void PutMessage(const std::string & sessionid, const std::string & data);
	void Close();
protected:
	void run();
	virtual void onConnected();
	virtual void onClosed();
	virtual void onError(long err);
	virtual void onReceived(const std::string & data) = 0;
	virtual void onWriteble();
	void onConnectTimeOut();
	void onConnectDelay();
	virtual void Login() = 0;
	void SetLoginSuccess();
	virtual void Ping() = 0;
	virtual void HeartbeatTimeout();
	virtual void ResetHearbeat() final;
	virtual void SetHeartbeat()final;

	int Send(const char * data, unsigned int len, const std::string & sessionId = "");

	static void connect_event_cb(struct bufferevent *bev, short event, void *arg);
	static void connect_read_cb(struct bufferevent* bev, void* arg);
	static void connect_write_cb(struct bufferevent* bev, void* arg);
	static void connect_timeout(evutil_socket_t fd, short event, void *arg);
	static void connect_delay_timer(evutil_socket_t fd, short event, void *arg);
	static void onwriteble(evutil_socket_t fd, short event, void *arg);
	static void ping_timer(evutil_socket_t fd, short event, void *arg);
	static void heartbeatTimeout(evutil_socket_t fd, short event, void * arg);
protected:
	struct event_base * m_pBase = nullptr;
	struct bufferevent* m_bev = nullptr;
	struct event      * m_connect_timer = nullptr;
	struct event      * m_ping_timer = nullptr;
	struct event      * m_connect_deley_timer = nullptr;
	struct event      * m_Writeble = nullptr;
	struct event      * m_heartbeatTimeout = nullptr;
	uint32_t            m_TimeoutTimes = 0;
	std::atomic<bool>	m_Connected;

	std::thread m_thread;
	std::atomic_bool    m_Running;
	Json::Value m_tcpAddr;
	uint32_t	m_tcpIndex = 0;
	std::string m_tcpIp;
	int			m_tcpPort = 0;
	helper::CEventBuffer<class TCPData *> m_Datas;
protected:
	TSiaManager * m_Own = nullptr;
	log4cplus::Logger log;
};

#endif
