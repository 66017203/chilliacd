#include "SiaTcpServer.h"
#include "SiaManager.h"

SiaTcpClient::SiaTcpClient(SiaTCPServer * server, TSiaManager * own, struct event_base * base, evutil_socket_t fd)
	:TcpClient(base, fd), m_module(own), m_Server(server)
{
	this->log = log4cplus::Logger::getInstance("SiaTcpClient");
	LOG4CPLUS_TRACE(log, m_Id << " " << this <<" construction");
}

void SiaTcpClient::onClosed()
{
	TcpClient::onClosed();
	const auto & it = m_Server->m_Connects.find(this->m_station);
	if (it != m_Server->m_Connects.end()) {
		m_Server->m_Connects.erase(it);
		LOG4CPLUS_DEBUG(log, m_Id << " rmove connection station:" << m_station << "," << this);
	}
	delete this;
}

void SiaTcpClient::onError(long err)
{
	TcpClient::onError(err);
	const auto & it = m_Server->m_Connects.find(this->m_station);
	if (it != m_Server->m_Connects.end()) {
		m_Server->m_Connects.erase(it);
		LOG4CPLUS_DEBUG(log, m_Id << " rmove connection station:" << m_station << "," << this);
	}
	delete this;
}

void SiaTcpClient::onReceived(const std::string & data)
{
	TcpClient::onReceived(data);
	std::shared_ptr<CMessage> recvmsg(new CMessage());
	if (m_module->SiaUnpackjson2Message(data, recvmsg)) {
		recvmsg->station = m_station;
		m_module->SendMessageToMe(std::move(recvmsg));
	}
}

bool SiaTcpClient::onLogin(const std::string & data)
{
	Json::Reader reader;
	Json::Value root;
	
    LOG4CPLUS_DEBUG(log, m_Id << " add connection station:" << "," << this);
	if (reader.parse(data, root) && root.isObject()) {
		if (root["cmd"].isString() && root["cmd"].asString() == "cmdconnect") {
			int station = -1;
			if (root["asstation"].isUInt()){
				station = root["asstation"].asUInt();
			}
			else if (root["asstation"].isInt()){
				station = root["asstation"].asInt();
			}
			if (station != -1){
				LOG4CPLUS_DEBUG(log, m_Id << " add connection station:" << station << "," << this);

				if (m_Server->m_Connects.find(station) != m_Server->m_Connects.end()){
					m_Server->m_Connects[station]->Close();
					delete m_Server->m_Connects[station];
				}

				m_Server->m_Connects[station] = this;
				this->m_station = station;
				return true;
			}
		}
	}
	else {
		LOG4CPLUS_ERROR(log, m_Id << ", parse json error " << data);
	}
	
	return false;
}

SiaTcpClient::~SiaTcpClient()
{
	const auto & it = m_Server->m_Connects.find(this->m_station);
	if (it != m_Server->m_Connects.end()) {
		m_Server->m_Connects.erase(it);
		LOG4CPLUS_DEBUG(log, m_Id << " rmove connection station:" << m_station << "," << this);
	}
	LOG4CPLUS_TRACE(log, m_Id << " " << this << " deconstruct");
}

//std::atomic_ullong station(1);
TcpClient * SiaTCPServer::OnAccept(struct event_base * base, evutil_socket_t fd)
{
	// add begin
    static int station = 10;
	if (station != -1)
	{
		LOG4CPLUS_DEBUG(log, " add connection station:" << station << "," << this);

		if (this->m_Connects.find(station) != this->m_Connects.end()){
			m_Connects[station]->Close();
			delete m_Connects[station];
		}
	}
	// add end
	TcpClient * wsc = new SiaTcpClient(this, m_Own, base, fd);
	// add begin
	m_Connects[station++] = wsc;
	// add end
	return wsc;
}

void SiaTCPServer::PutMessage(int station, const std::string & sessionid, const std::string & data)
{
	if (station == -1){
		LOG4CPLUS_ERROR(log, sessionid << " station id == -1.");
		return;
	}

	LOG4CPLUS_DEBUG(log, sessionid << " PutMessage [" << station << " " << sessionid << " " << data);
	SiaTcpData TcpData(sessionid, data);
	std::unique_lock<std::mutex> lck(m_DataMtx);
	m_Datas.insert(std::make_pair(station,TcpData));
	LOG4CPLUS_DEBUG(log, "SiaTCPServer::PutMessage " << m_Datas.size());
}

void SiaTCPServer::OnWriteble()
{
	std::unique_lock<std::mutex> lck(m_DataMtx);
	//LOG4CPLUS_DEBUG(log, "SiaTCPServer::OnWriteble " << m_Connects.size());
	for (auto & it : m_Connects)
	{
		int station = it.first;
		TcpClient * tcp = it.second;
		const auto & itDatas= m_Datas.equal_range(station);
		//auto itdata;
		for (auto itdata = itDatas.first; itdata != itDatas.second;)
		{
			tcp->Send(itdata->second.m_data.c_str(), itdata->second.m_data.length(), itdata->second.m_sessionId);
			m_Datas.erase(itdata++);
		}
	}
}
