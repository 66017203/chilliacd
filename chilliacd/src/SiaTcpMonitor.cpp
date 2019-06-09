#include <iostream>
#include "SiaTcpMonitor.h"
#include <log4cplus/loggingmacros.h>
#include "json/json.h"
#include "SiaManager.h"


extern std::atomic<bool> g_Running;
extern TSiaManager * g_sia;



SiaTcpMonitor::SiaTcpMonitor(TSiaManager *own) :TcpClientModule(own)
{
	std::string logname = "OAM";
	this->log = log4cplus::Logger::getInstance(logname);

	LOG4CPLUS_TRACE(log, "construct");
}

SiaTcpMonitor::~SiaTcpMonitor()
{
	LOG4CPLUS_TRACE(log, "deconstrctor");
}

void SiaTcpMonitor::Login()
{
	Json::Value login;
	login["cmd"] = "login";
	login["uuid"] = " ";
	login["content"] = " ";
	login["version"] = m_Own->m_version;
	login["station_no"] = m_Own->m_stationno;

	Json::FastWriter write;
	std::string data = write.write(login);
	this->Send(data.c_str(), data.length());
}

void SiaTcpMonitor::Ping()
{
	Json::Value login;
	login["cmd"] = "ping";
	login["uuid"] = " ";
	login["content"] = " ";
	login["station_no"] = m_Own->m_stationno;

	Json::FastWriter write;
	std::string data = write.write(login);
	this->Send(data.c_str(), data.length());
}

void SiaTcpMonitor::onReceived(const std::string & data)
{
	this->ProcessRecv(data);
}

void SiaTcpMonitor::ProcessRecv(const std::string & data)
{
	//LOG4CPLUS_DEBUG(log, "<ProcessRecv> Data " << data);

	Json::Value root;
	Json::Reader reader;
	if (!reader.parse(data, root) && root.isObject()){
		LOG4CPLUS_ERROR(log, "not json data.");
		return;
	}

	std::string cmd = root["cmd"].isString() ? root["cmd"].asString() : "";
	if(cmd =="login"){
		std::string content = root["content"].isString() ? root["content"].asString() : "";
		if (content == "kick")
		{
			LOG4CPLUS_WARN(log, "Login resp was kick off");
			g_Running = false;
			std::cin.clear();
		}
		else {
			SetLoginSuccess();
		}
	}
	else if (cmd == "stopsession"){

		if (root["sessionid"].isUInt()){
			SESSIONIDTYPE sessionid = root["content"].asUInt();
			TSessionLuaPtr session = g_sia->GetSessionByID(sessionid);
			if (session){
				session->stopservice();
			}
		}

	}
	else if (cmd == "ack")
	{
		this->ResetHearbeat();
	}
	else {
		LOG4CPLUS_DEBUG(log, "receive:" << data);
	}
	return;
}

void SiaTcpMonitor::Alarm(const std::string & sessionid, const std::string & level, const std::string & content)
{
	Json::Value root;
	root["cmd"] = "alarm";
	root["from_station"] = m_Own->m_stationno;
	using namespace std::chrono;
	time_point<system_clock, seconds> now = time_point_cast<seconds>(system_clock::now());
	root["timestamp"] = (uint32_t)now.time_since_epoch().count();
	root["level"] = level;
	root["content"] = content;
	Json::FastWriter writer;
	std::string strData = writer.write(root);
	this->PutMessage(sessionid, strData);
}
