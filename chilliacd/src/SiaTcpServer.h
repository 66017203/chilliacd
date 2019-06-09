#pragma once
#ifndef _SIA_TCPSERVER_HEADER_
#define _SIA_TCPSERVER_HEADER_
#include "TcpServerModule.h"
#include <map>
#include <mutex>

class TSiaManager;

class SiaTcpData {
public:
	SiaTcpData(const std::string & sessionid, const std::string & data) :m_sessionId(sessionid), m_data(data)
	{
	}

public:
	std::string m_sessionId;
	std::string m_data;
};

class SiaTCPServer;
class SiaTcpClient :public TcpClient
{
public:
	explicit SiaTcpClient(SiaTCPServer * server, TSiaManager * own, struct event_base * base, evutil_socket_t fd);
	virtual void onClosed() override;
	virtual void onError(long err) override;
	virtual void onReceived(const std::string & data)override;
	virtual bool onLogin(const std::string & data) override;

	~SiaTcpClient();
private:
	TSiaManager * m_module = nullptr;
	int32_t m_station = -1;
	SiaTCPServer * m_Server = nullptr;
};

class SiaTCPServer :public TcpServerModule
{
public:
	explicit SiaTCPServer(TSiaManager * own, const char * ip, int port)
		:TcpServerModule(ip, port), m_Own(own)
	{};
	virtual TcpClient * OnAccept(struct event_base * base, evutil_socket_t fd) override;
	virtual void PutMessage(int station, const std::string & sessionid, const std::string & data) override;
	virtual void OnWriteble() override;
private:
	TSiaManager * m_Own = nullptr;
	std::map<int, TcpClient *> m_Connects;
	std::multimap<int, SiaTcpData> m_Datas;
	std::mutex m_DataMtx;
	friend class SiaTcpClient;
};

#endif // end TCP SERVER header

