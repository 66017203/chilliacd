#include "TcpServerModule.h"
#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/listener.h>
#include <event2/util.h>
#include <event2/thread.h>
#include <log4cplus/loggingmacros.h>
#include <cstring>
#include "json/json.h"
#include <map>
#include <mutex>
#include <atomic>
#include <iostream>
#ifndef WIN32
#include <arpa/inet.h>
#endif

extern std::atomic<bool> g_Running;

#define INT_SIZE          sizeof(unsigned int)
#define CONNECT_TIMEOUT   30
#define LOGIN_TIMEOUT     10
#define HEARTBEAT_TIMEOUT 30

// 数据长度
union contextlen {
	char buffer[INT_SIZE];
	unsigned int  nlen;      // 网络字节序
	unsigned int  hlen;      // 本地字节序
};

static std::map<void *, struct bufferevent*> g_TCPClientMap;
static std::recursive_mutex g_TCPMutex;

// 连接超时 回调
static void connect_timer(evutil_socket_t fd, short event, void *arg)
{
	TcpClient * This = (TcpClient*)arg;
	std::unique_lock<std::recursive_mutex> lcx(g_TCPMutex);
	if (g_TCPClientMap.find(arg) != g_TCPClientMap.end())
		This->onError(ETIMEDOUT);
}

// 登录超时 回调
static void login_timer(evutil_socket_t fd, short event, void *arg)
{
	TcpClient * This = (TcpClient*)arg;
	std::unique_lock<std::recursive_mutex> lcx(g_TCPMutex);
	//if (g_TCPClientMap.find(arg) != g_TCPClientMap.end())
	if (g_TCPClientMap.find(arg) == g_TCPClientMap.end())
		This->onLoginTimeout();
} 
// 心跳超时 回调
static void heartbeat_timer(evutil_socket_t fd, short event, void *arg)
{
	TcpClient * This = (TcpClient*)arg;
	std::unique_lock<std::recursive_mutex> lcx(g_TCPMutex);
	if (g_TCPClientMap.find(arg) != g_TCPClientMap.end())
		This->onHeartbeatTimeout();
}

static std::atomic_ullong TcpClientId(0);
TcpClient::TcpClient(struct event_base * base)
    :m_pBase(base),m_Connected(false),m_Logined(false)
{
	this->log = log4cplus::Logger::getInstance("TcpClient");
	m_Id = ++TcpClientId;
	m_connect_timer = evtimer_new(m_pBase, connect_timer, this);
	m_login_timer = evtimer_new(m_pBase, login_timer, this);
	m_heartbeat_timer = evtimer_new(m_pBase, heartbeat_timer, this);

	std::unique_lock<std::recursive_mutex> lcx(g_TCPMutex);
	g_TCPClientMap.insert(std::make_pair(this, nullptr));
}

TcpClient::TcpClient(struct event_base * base, evutil_socket_t fd)
    :m_pBase(base), m_Connected(false), m_Logined(false)
{
	//TcpClient(base);
	this->log = log4cplus::Logger::getInstance("TcpClient");
	m_Id = ++TcpClientId;
	m_connect_timer = evtimer_new(m_pBase, connect_timer, this);
	m_login_timer = evtimer_new(m_pBase, login_timer, this);
	m_heartbeat_timer = evtimer_new(m_pBase, heartbeat_timer, this);

	m_bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
	if (!m_bev) {
		LOG4CPLUS_ERROR(log, m_Id << " Error constructing bufferevent!");
	}

	std::unique_lock<std::recursive_mutex> lcx(g_TCPMutex);
	g_TCPClientMap.insert(std::make_pair(this, m_bev));

	bufferevent_setcb(m_bev, TcpServerModule::conn_read_cb, TcpServerModule::conn_writecb, TcpServerModule::conn_eventcb, this);
	bufferevent_enable(m_bev, EV_WRITE | EV_READ | EV_ET);

	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_sec = LOGIN_TIMEOUT;
	event_add(m_login_timer, &tv);
}

TcpClient::~TcpClient()
{
	std::unique_lock<std::recursive_mutex> lcx(g_TCPMutex);
	g_TCPClientMap.erase(this);

	if (m_bev) {
		bufferevent_free(m_bev);
		m_bev = nullptr;
	}
	
	event_free(m_connect_timer);
	m_connect_timer = nullptr;

	event_free(m_login_timer);
	m_login_timer = nullptr;
	
	event_free(m_heartbeat_timer);
	m_heartbeat_timer = nullptr;
}

void TcpClient::Close()
{
	if (m_bev) {
		LOG4CPLUS_WARN(log, m_Id << " CLOSE SOCKET.");
		evutil_closesocket(bufferevent_getfd(m_bev));
		this->m_Connected = false;
	}

	event_del(m_connect_timer);
	event_del(m_login_timer);
	event_del(m_heartbeat_timer);
}

bool TcpClient::IsConnected()
{
	return m_Connected;
}

void TcpClient::onConnected()
{
	event_del(m_connect_timer);
	m_Connected = true;
	LOG4CPLUS_DEBUG(log, m_Id << " onConnected");
}

void TcpClient::onClosed()
{
	m_Connected = false;
	
	event_del(m_connect_timer);
	event_del(m_login_timer);
	event_del(m_heartbeat_timer);

	LOG4CPLUS_DEBUG(log, m_Id << " onClosed");
}

void TcpClient::onError(long err)
{
	m_Connected = false;
	
	event_del(m_connect_timer);
	event_del(m_login_timer);
	event_del(m_heartbeat_timer);

	LOG4CPLUS_DEBUG(log, m_Id << " onError [" << err << "]");
}

void TcpClient::onReceived(const std::string & data)
{
	LOG4CPLUS_DEBUG(log, m_Id << " onReceived:" << data);
}

void TcpClient::onLoginTimeout()
{
	LOG4CPLUS_DEBUG(log, m_Id << " onLoginTimeout:");
	this->Close();
	delete this;
}

void TcpClient::onLoginSuccess()
{
	LOG4CPLUS_DEBUG(log, m_Id << " onLoginSuccess:");
	event_del(m_login_timer);

	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_sec = HEARTBEAT_TIMEOUT;
	event_add(m_heartbeat_timer, &tv);
}

void TcpClient::onHeartbeatTimeout()
{
	LOG4CPLUS_DEBUG(log, m_Id << " onHeartbeatTimeout:");
	this->Close();
	delete this;
}

void TcpClient::ResetHeartbeatTimer()
{
	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_sec = HEARTBEAT_TIMEOUT;
	event_add(m_heartbeat_timer, &tv);
}

bool TcpClient::onLogin(const std::string & data)
{
	LOG4CPLUS_ERROR(log, m_Id << " onLogin " << data);
	//return false;
	return true;
}

void TcpClient::onSend()
{
	LOG4CPLUS_DEBUG(log, m_Id << " onSend");
}

int TcpClient::Send(const char * data, unsigned int len, const std::string & sessionId)
{
	if (m_Connected) {
		contextlen nlen;
		std::string buffer;
		nlen.nlen = htonl(len + INT_SIZE);
		buffer.append(nlen.buffer, INT_SIZE);
		buffer.append(data, len);
		LOG4CPLUS_TRACE(log, m_Id << " " << sessionId << " send to :" << buffer);
		return bufferevent_write(m_bev, buffer.c_str(), buffer.length());
	}
	else
		LOG4CPLUS_ERROR(log, m_Id << " no connected.");
	return -1;
}

static void onWriteble(evutil_socket_t fd, short event, void *arg)
{
    //log4cplus::Logger log;
	//log = log4cplus::Logger::getInstance("static||||||||||||||||");
	//LOG4CPLUS_DEBUG(log, "<>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
	TcpServerModule * This = reinterpret_cast<TcpServerModule *>(arg);
	This->OnWriteble();
}

TcpServerModule::TcpServerModule(const char * ip, int port) :
	m_tcpIP(ip), m_tcpPort(port)
{
	log = log4cplus::Logger::getInstance("TcpServerModule");
	LOG4CPLUS_DEBUG(log, "Constructs a TcpServerModule instance.");
}

TcpServerModule::TcpServerModule()
{
	TcpServerModule("", -1);
}


TcpServerModule::~TcpServerModule(void)
{
	if (m_thread.joinable()){
		Stop();
	}
	LOG4CPLUS_DEBUG(log, "Destructs a TcpServerModule instance.");
}

void TcpServerModule::Start()
{
	if (!m_thread.joinable()) {

		m_thread = std::thread(&TcpServerModule::listenTCP, this, this->m_tcpIP, this->m_tcpPort);
	}
}

void TcpServerModule::Stop(void)
{
	if (m_thread.joinable()) {
		event_base_loopbreak(m_Base);
		m_thread.join();
	}
}

void TcpServerModule::PutMessage(int station, const std::string & sessionid, const std::string & data)
{
	LOG4CPLUS_ERROR(log, "PutMessage not implement.");
}

void TcpServerModule::OnWriteble()
{
	LOG4CPLUS_ERROR(log, "OnWriteble not implement.");
}

struct event_base * TcpServerModule::GetBase()
{
	return m_Base;
}

TcpClient * TcpServerModule::OnAccept(event_base * base, evutil_socket_t fd)
{
	TcpClient * client = new TcpClient(base, fd);
	return client;
}

void TcpServerModule::listener_cb(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *sa, int socklen, void *user_data)
{
	TcpServerModule * This = reinterpret_cast<TcpServerModule *>(user_data);

	struct event_base *base = evconnlistener_get_base(listener);
	TcpClient * client = This->OnAccept(base, fd);
	sockaddr_in * sa_in = (sockaddr_in *)sa;
	LOG4CPLUS_DEBUG(This->log, "accept client:" << client << ":" << inet_ntoa(sa_in->sin_addr) << ":" << ntohs(sa_in->sin_port));
	client->onConnected();
}

void TcpServerModule::conn_read_cb(struct bufferevent *bev, void * user_data)
{
	g_TCPMutex.lock();
	if (g_TCPClientMap.find(user_data) == g_TCPClientMap.end()) {
		LOG4CPLUS_ERROR(log4cplus::Logger::getRoot(), user_data << ", tcp client is freed.");
		g_TCPMutex.unlock();
		return;
	}
	g_TCPMutex.unlock();

	TcpClient * This = (TcpClient*)user_data;
	contextlen hlen;
	struct evbuffer *buffer = bufferevent_get_input(bev);

	size_t len = evbuffer_get_length(buffer);
	if (len > 0) {
		std::vector<char> data(len);
		evbuffer_copyout(buffer, data.data(), data.size());
		LOG4CPLUS_TRACE(This->log, This->m_Id << " receve data:" << std::string(data.begin(), data.end()));
		std::string str_data = std::string(data.begin(), data.end());
		//This->onReceived(str_data);
	}

	while (evbuffer_get_length(buffer) >= INT_SIZE) {
		evbuffer_copyout(buffer, hlen.buffer, INT_SIZE);
		LOG4CPLUS_DEBUG(This->log, This->m_Id << " nlen " << hlen.nlen);
		hlen.hlen = ntohl(hlen.nlen);
		LOG4CPLUS_DEBUG(This->log, This->m_Id << " hlen " << hlen.hlen);

		if (evbuffer_get_length(buffer) >= hlen.hlen) {
			std::vector<char>data(hlen.hlen);
			evbuffer_remove(buffer, data.data(), hlen.hlen);
			std::string content = std::string(data.begin() + INT_SIZE, data.end());
			if (true /*This->m_Logined*/) {
				Json::Reader reader;
				Json::Value root;
				bool isHeartbeat = false;
				if (reader.parse(content, root) && root.isObject()) {
					if (root["cmd"].isString() && root["cmd"].asString() == "ping") {
						isHeartbeat = true;
					}
				}

				if (isHeartbeat) {
					This->ResetHeartbeatTimer();
				}
				else
					This->onReceived(content);
			}
			else {
				This->m_Logined = This->onLogin(content);
				if (This->m_Logined)
					This->onLoginSuccess();
			}
		}
		else {// 不够一个完整的包，跳出循环   
			break;
		}
	}
}

void TcpServerModule::conn_writecb(struct bufferevent *bev, void *user_data)
{
	g_TCPMutex.lock();
	if (g_TCPClientMap.find(user_data) == g_TCPClientMap.end()) {
		g_TCPMutex.unlock();
		return;
	}

	TcpClient * This = (TcpClient*)user_data;
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_get_length(output) == 0) {
		This->onSend();
		//LOG4CPLUS_DEBUG(This->log, "write data end.");
	}
	//g_TCPMutex.unlock();
}

void TcpServerModule::conn_eventcb(struct bufferevent *bev, short events, void * user_data)
{
	g_TCPMutex.lock();
	if (g_TCPClientMap.find(user_data) == g_TCPClientMap.end()) {
		g_TCPMutex.unlock();
		return;
	}

	TcpClient * This = (TcpClient*)user_data;

	if (events & BEV_EVENT_EOF) {
		if (typeid(*This) == typeid(TcpClient)) {
			This->onClosed();
			delete This;
		}
		else
			This->onClosed();
	}
	else if (events & BEV_EVENT_ERROR) {
		if (typeid(*This) == typeid(TcpClient)) {
			This->onError(EVUTIL_SOCKET_ERROR());
			delete This;
		}
		else
			This->onError(EVUTIL_SOCKET_ERROR());
	}
	else if (events & BEV_EVENT_CONNECTED) {
		This->onConnected();
	}
	else if (events & BEV_EVENT_TIMEOUT)
	{
		LOG4CPLUS_WARN(This->log, This->m_Id << " time out");
	}
	else
		LOG4CPLUS_WARN(This->log, This->m_Id << " Unkown event.");

}

bool TcpServerModule::listenTCP(const std::string & ip, int port)
{
	struct sockaddr_in sin;
	struct evconnlistener *listener = nullptr;

	LOG4CPLUS_INFO(log, "Starting...");
#ifdef WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

    // 创建反应器
	m_Base = event_base_new();
	if (!m_Base) {
		LOG4CPLUS_ERROR(log, "Could not initialize libevent!");
		goto done;
	}

	LOG4CPLUS_INFO(log, " libevent current method [" << event_base_get_method(m_Base) << "]");

    // 创建事件 event_new(pBase, fd, what, cb, arg)
	m_Writeble = event_new(m_Base, -1, EV_PERSIST, onWriteble, this);
	/*const char ** methods = event_get_supported_methods();
	for (int i = 0; methods[i] != nullptr; ++i) {
		//LOG4CPLUS_INFO(log, ",libevent supported method:" << methods[i]);
	}*/

	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_usec = 50 * 1000;
	// 添加事件
	event_add(m_Writeble, &tv);

	if (!m_tcpIP.empty() && m_tcpPort != -1){
		memset(&sin, 0, sizeof(sin));
		sin.sin_family = AF_INET;
		sin.sin_addr.s_addr = htonl(0);
		sin.sin_port = htons(port);

		listener = evconnlistener_new_bind(m_Base, listener_cb, this,
			LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE | LEV_OPT_THREADSAFE, -1,
			(struct sockaddr*)&sin,
			sizeof(sin));

		if (!listener) {
			LOG4CPLUS_ERROR(log, m_tcpIP << ":" << port << ", Could not create a listener!");
			goto done;
		}
		LOG4CPLUS_INFO(log, " start listen tcp [" << m_tcpIP << "]:[" << port << "]");
	}

    // 分发事件
	event_base_dispatch(m_Base);

done:
	g_Running = false;
	std::cin.clear();
	if (listener){
		evconnlistener_free(listener);
		listener = nullptr;
	}

	if (m_Writeble) {
		event_free(m_Writeble);  // 释放事件
		m_Writeble = nullptr;
	}

	if (m_Base) {
		event_base_free(m_Base); // 释放反应器
		m_Base = nullptr;
	}

#ifdef WIN32
	WSACleanup();
#endif
	LOG4CPLUS_INFO(log, "Stoped.");
	log4cplus::threadCleanup();
	return true;
}

