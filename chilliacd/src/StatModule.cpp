#include "StatModule.h"
#include <log4cplus/loggingmacros.h>
#include "SiaManager.h"


StatMoudle::StatMoudle(TSiaManager * own) :TcpClientModule(own)
{
	log = log4cplus::Logger::getInstance("StatMoudle");
	LOG4CPLUS_TRACE(log, "CONSTRUCT");
}

StatMoudle::~StatMoudle()
{
	LOG4CPLUS_TRACE(log, "DECONSTRUCTOR");
}

void StatMoudle::Start(Json::Value address)
{
	TcpClientModule::Start(address);
}

void StatMoudle::Stop()
{
	TcpClientModule::Stop();
}

void StatMoudle::Login()
{
#define _STRVERSION(val) #val
#define STRVERSION(val) _STRVERSION(val)

	Json::Value login;
	login["cmd"] = "login";
	login["station_no"] = m_Own->m_stationno;
	login["reason"] = "Sia module login";
	login["version"] = STRVERSION(SIAVERSION);

	Json::FastWriter write;
	std::string data = write.write(login);
	Send(data.c_str(), data.length());
}

void StatMoudle::Ping()
{
	Json::Value ping;
	ping["cmd"] = "ping";
	ping["station_no"] = m_Own->m_stationno;

	Json::FastWriter write;
	std::string data = write.write(ping);
	Send(data.c_str(), data.length());
}

void StatMoudle::onReceived(const std::string & data)
{
	Json::Reader reader;
	Json::Value root;
	if (reader.parse(data, root) && root.isObject() && root["cmd"].isString())
	{
		if (root["cmd"].asString() == "ack"){
			//LOG4CPLUS_DEBUG(log, "receive:" << data);
			this->ResetHearbeat();
		}
		else if (root["cmd"].asString() == "login")
		{
			if (root["status"].isString() && root["status"].asString() == "0")
				SetLoginSuccess();
		}
		else {
			std::shared_ptr<CMessage> recvmsg(new CMessage());
			m_Own->SiaUnpackjson2Message(data, recvmsg);

			m_Own->SendMessageToMe(std::move(recvmsg));
		}
	}
}

