#pragma once
#ifndef _TCPSERVER_MODULE_HEADER_
#define _TCPSERVER_MODULE_HEADER_
#include <log4cplus/logger.h>
#include <event2/event.h>
#include <event2/util.h>
#include <thread>
#include <atomic>
#include "json/json.h"

class TcpClient {
public:
	explicit TcpClient(struct event_base * base);
	explicit TcpClient(struct event_base * base, evutil_socket_t fd);
	virtual ~TcpClient();

	bool IsConnected();
	virtual void onConnected();
	virtual void onSend();
	virtual void onClosed();
	virtual void onError(long err);
	virtual void onReceived(const std::string & data);
	virtual bool onLogin(const std::string & data);
	void onLoginTimeout();
	void onLoginSuccess();
	void onHeartbeatTimeout();
	void ResetHeartbeatTimer();

	void Close();
	int Send(const char * data, unsigned int len, const std::string & sessionId);

private:
	struct event_base * m_pBase           = nullptr;
	struct bufferevent* m_bev             = nullptr;
	struct event      * m_connect_timer   = nullptr;
	struct event      * m_login_timer     = nullptr;
	struct event      * m_heartbeat_timer = nullptr;
	std::atomic<bool>   m_Connected;
	std::atomic_bool    m_Logined;
	friend class TcpServerModule;
protected:
	log4cplus::Logger log;
	int64_t			  m_Id;
};

class TcpServerModule
{
public:
	explicit TcpServerModule(const char * ip, int port);
	explicit TcpServerModule();
	virtual ~TcpServerModule(void);
	void Start();
	void Stop();
	virtual void PutMessage(int station, const std::string & sessionid, const std::string & data);
	virtual void OnWriteble();
	struct event_base * GetBase();
	virtual TcpClient * OnAccept(struct event_base * base, evutil_socket_t fd);
	static void listener_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
	static void conn_read_cb(struct bufferevent *bev, void *ctx);
	static void conn_writecb(struct bufferevent *, void *);
	static void conn_eventcb(struct bufferevent *, short, void *);

protected:
	log4cplus::Logger log;
private:
	std::thread m_thread;
	struct event_base * m_Base = nullptr;
	struct event      * m_Writeble = nullptr;
	std::string m_tcpIP;
	int m_tcpPort = -1;

	bool listenTCP(const std::string & ip, int port);

};
#endif // end TCP SERVER header

