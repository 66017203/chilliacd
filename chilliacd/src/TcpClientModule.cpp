#include "TcpClientModule.h"
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <log4cplus/loggingmacros.h>
#include "SiaManager.h"

#ifdef WIN32
#include <WS2tcpip.h>
#endif


#define INT_SIZE                        sizeof(unsigned int)
#define PING_INTERVAL					10
#define CONNECT_TIMEOUT					30
#define CONNECT_DELAY					5
#define HEARTBEAT_TIMEOUT				2

union contextlen {
	char buffer[INT_SIZE];
	unsigned int  nlen;
	unsigned int  hlen;
};


class TCPData {
public:
	TCPData(const std::string & sessionid, const std::string & data) :m_sessionId(sessionid), m_data(data)
	{
	}

public:
	std::string m_sessionId;
	std::string m_data;
};
typedef TCPData* TCPDataPtr;

void TcpClientModule::connect_event_cb(struct bufferevent *bev, short event, void *arg)
{
	TcpClientModule * This = (TcpClientModule*) arg;

	if (event & BEV_EVENT_EOF) {
		This->onClosed();
	}
	else if (event & BEV_EVENT_ERROR) {
		This->onError(EVUTIL_SOCKET_ERROR());
	}
	else if (event & BEV_EVENT_CONNECTED) {
		This->onConnected();
	}
	else
		LOG4CPLUS_WARN(This->log, "Unkown event.");
}

void TcpClientModule::connect_read_cb(struct bufferevent* bev, void* arg)
{
	TcpClientModule * This = (TcpClientModule*)arg;

	contextlen hlen;
	struct evbuffer *buffer = bufferevent_get_input(bev);

	size_t len = evbuffer_get_length(buffer);
	if (len > 0) {
		std::vector<char>data(len);
		evbuffer_copyout(buffer, data.data(), data.size());
		LOG4CPLUS_TRACE(This->log, "receve data:" << std::string(data.begin(), data.end()));
	}

	while (evbuffer_get_length(buffer) >= INT_SIZE) {
		evbuffer_copyout(buffer, hlen.buffer, INT_SIZE);
		hlen.hlen = ntohl(hlen.nlen);

		if (evbuffer_get_length(buffer) >= hlen.hlen) {
			std::vector<char>data(hlen.hlen);
			evbuffer_remove(buffer, data.data(), hlen.hlen);
			std::string content = std::string(data.begin() + INT_SIZE, data.end());
			This->onReceived(content);
		}
		else {// 不够一个完整的包，跳出循环
			break;
		}
	}
}

void TcpClientModule::connect_write_cb(struct bufferevent* bev, void* arg)
{
	TcpClientModule * This = (TcpClientModule*)arg;
	struct evbuffer *output = bufferevent_get_output(bev);
	if (evbuffer_get_length(output) == 0) {
		//This->onWriteble();
		//LOG4CPLUS_DEBUG(This->log, "write data end.");
	}
}

void TcpClientModule::connect_timeout(evutil_socket_t fd, short event, void *arg)
{
	TcpClientModule * This = (TcpClientModule*)arg;
	This->onConnectTimeOut();
}

void TcpClientModule::connect_delay_timer(evutil_socket_t fd, short event, void *arg)
{
	TcpClientModule * This = (TcpClientModule*)arg;
	This->onConnectDelay();
}

void TcpClientModule::onwriteble(evutil_socket_t fd, short event, void *arg)
{
	TcpClientModule * This = (TcpClientModule*)arg;
	This->onWriteble();
}

void TcpClientModule::ping_timer(evutil_socket_t fd, short event, void *arg)
{
	TcpClientModule * This = (TcpClientModule*)arg;
	This->Ping();
	This->SetHeartbeat();
}

void TcpClientModule::heartbeatTimeout(evutil_socket_t fd, short event, void * arg)
{
	TcpClientModule * This = (TcpClientModule*)arg;
	This->HeartbeatTimeout();
}

TcpClientModule::TcpClientModule(TSiaManager * own) :m_Connected(false), m_Running(false),
m_Own(own)
{
	this->log = log4cplus::Logger::getInstance("TcpClient");
}

TcpClientModule::~TcpClientModule()
{
	if (m_thread.joinable()){
		Stop();
	}
}

void TcpClientModule::Start(Json::Value address)
{
	if (m_thread.joinable())
		Stop();

	this->m_Running = true;

	this->m_tcpAddr = address;

	if (m_tcpAddr.isArray() && m_tcpAddr.size() > 0) {
		LOG4CPLUS_TRACE(log, "address: " << this->m_tcpAddr.toStyledString());
		m_thread = std::thread(&TcpClientModule::run, this);
	}
	else {
		LOG4CPLUS_ERROR(log, " address not array.");
	}
}


void TcpClientModule::run()
{
	LOG4CPLUS_DEBUG(log, "Starting...");

	while (m_Running)
	{
		m_pBase = event_base_new();
		LOG4CPLUS_DEBUG(log, "current method:" << event_base_get_method(m_pBase));

		m_tcpIp.clear();
		m_tcpPort = 0;

		m_tcpIndex = m_tcpIndex % m_tcpAddr.size();

		if (m_tcpAddr[m_tcpIndex]["ipaddr"].isString()) {
			m_tcpIp = m_tcpAddr[m_tcpIndex]["ipaddr"].asString();
		}

		if (m_tcpAddr[m_tcpIndex]["socketport"].isInt()) {
			m_tcpPort = m_tcpAddr[m_tcpIndex]["socketport"].asInt();
		}

		m_tcpIndex++;

		m_Writeble = event_new(m_pBase, -1, EV_PERSIST, onwriteble, this);
		m_connect_timer = evtimer_new(m_pBase, TcpClientModule::connect_timeout, this);
		m_connect_deley_timer = evtimer_new(m_pBase, TcpClientModule::connect_delay_timer, this);
		m_ping_timer = event_new(m_pBase, -1, EV_PERSIST, ping_timer, this);
		m_heartbeatTimeout = event_new(m_pBase, -1, EV_TIMEOUT, heartbeatTimeout, this);

		struct timeval tv;
		evutil_timerclear(&tv);
		tv.tv_usec = 50 * 1000;
		event_add(m_Writeble, &tv);

		evutil_timerclear(&tv);
		tv.tv_sec = CONNECT_TIMEOUT;
		event_add(m_connect_timer, &tv);

		this->m_bev = bufferevent_socket_new(m_pBase, -1,
			BEV_OPT_CLOSE_ON_FREE);

		bufferevent_socket_connect_hostname(m_bev, NULL, AF_INET, m_tcpIp.c_str(), m_tcpPort);

		bufferevent_setcb(m_bev, TcpClientModule::connect_read_cb, TcpClientModule::connect_write_cb, TcpClientModule::connect_event_cb, this);
		bufferevent_enable(m_bev, EV_READ | EV_WRITE);

		struct sockaddr_in  sa_local;
		socklen_t len = sizeof(sa_local);
		getsockname(bufferevent_getfd(m_bev), (sockaddr *)&sa_local, &len);
		LOG4CPLUS_DEBUG(log, "connecting to " << m_tcpIp << ":" << m_tcpPort << ", localport:"<< ntohs(sa_local.sin_port));

		event_base_dispatch(m_pBase);

		m_Connected = false;

		event_free(m_connect_timer);
		m_connect_timer = nullptr;

		event_free(m_connect_deley_timer);
		m_connect_deley_timer = nullptr;

		event_free(m_ping_timer);
		m_ping_timer = nullptr;

		event_free(m_heartbeatTimeout);
		m_heartbeatTimeout = nullptr;

		bufferevent_free(m_bev);
		m_bev = nullptr;

		event_free(m_Writeble);
		m_Writeble = nullptr;

		event_base_free(m_pBase);
		m_pBase = nullptr;

		if (m_Running){
			std::this_thread::sleep_for(std::chrono::seconds(5));
		}

	}

	LOG4CPLUS_DEBUG(log, "Stoped.");
	log4cplus::threadCleanup();
}

void TcpClientModule::Stop()
{
	m_Running = false;
	if (m_pBase) {
		event_base_loopbreak(m_pBase);
	}

	if(m_thread.joinable() && std::this_thread::get_id() != m_thread.get_id()){
		m_thread.join();
	}
}

bool TcpClientModule::IsConnected()
{
	return m_Connected;
}

void TcpClientModule::onConnected()
{
	event_del(m_connect_timer);
	LOG4CPLUS_DEBUG(this->log, "the client has connected");
	this->Login();
	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_sec = PING_INTERVAL;
	event_add(m_ping_timer, &tv);
}

void TcpClientModule::onClosed()
{
	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_sec = CONNECT_DELAY;
	event_add(m_connect_deley_timer, &tv);
	event_del(m_ping_timer);
	event_del(m_heartbeatTimeout);
	m_Connected = false;
	LOG4CPLUS_WARN(this->log, "connection closed");
}

void TcpClientModule::onError(long err)
{
	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_sec = CONNECT_DELAY;
	event_add(m_connect_deley_timer, &tv);
	event_del(m_ping_timer);
	event_del(m_heartbeatTimeout);
	m_Connected = false;
	LOG4CPLUS_ERROR(this->log, "error:" << err);
}

void TcpClientModule::onConnectTimeOut()
{
	LOG4CPLUS_ERROR(this->log, "connect " << m_tcpIp << ":" << m_tcpPort << " timeout:");
	event_base_loopbreak(this->m_pBase);
}

void TcpClientModule::onConnectDelay()
{
	event_base_loopbreak(this->m_pBase);
}

void TcpClientModule::SetLoginSuccess()
{
	m_Connected = true;
}

void TcpClientModule::HeartbeatTimeout()
{
	if (++m_TimeoutTimes >= 2) {
		m_Connected = false;
		LOG4CPLUS_ERROR(log, "HeartbeatTimeout");
		event_base_loopbreak(this->m_pBase);
	}
}

void TcpClientModule::ResetHearbeat()
{
	event_del(m_heartbeatTimeout);
	m_TimeoutTimes = 0;
}

void TcpClientModule::SetHeartbeat()
{
	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_sec = HEARTBEAT_TIMEOUT;
	event_add(m_heartbeatTimeout, &tv);
}

int TcpClientModule::Send(const char * data, unsigned int len, const std::string & sessionId)
{

	contextlen nlen;
	std::string buffer;
	nlen.nlen = htonl(len + INT_SIZE);
	buffer.append(nlen.buffer, INT_SIZE);
	buffer.append(data, len);

	LOG4CPLUS_TRACE(log, sessionId << " send to :" << buffer);
	return bufferevent_write(m_bev, buffer.c_str(), buffer.length());

	return -1;
}

void TcpClientModule::PutMessage(const std::string & sessionid, const std::string & data)
{
	if (m_Datas.size() < 100000) {
		TCPDataPtr _data = new TCPData(sessionid, data);
		m_Datas.Put(_data);
	}
}

void TcpClientModule::Close()
{
	if (m_bev) {
		LOG4CPLUS_DEBUG(log, "close... connection.");
		evutil_closesocket(bufferevent_getfd(m_bev));
	}
}


void TcpClientModule::onWriteble()
{
	while (m_Datas.size() > 0 && m_Connected) {
		TCPDataPtr data = nullptr;
		m_Datas.Get(data);
		LOG4CPLUS_TRACE(log, "queue size:" << m_Datas.size());
		if (this->Send(data->m_data.c_str(), data->m_data.size(), data->m_sessionId) != 0) {
			LOG4CPLUS_ERROR(log, data->m_sessionId << " send :" << data->m_data);
		}
		delete data;
	}
}
