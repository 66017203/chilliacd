#include "SSModule.h"
#include <log4cplus/loggingmacros.h>
#include "SiaManager.h"

SSMoudle::SSMoudle(TSiaManager * own) :TcpClientModule(own)
{
	log = log4cplus::Logger::getInstance("SSModule");
	LOG4CPLUS_TRACE(log, "construct");
}

SSMoudle::~SSMoudle()
{
	LOG4CPLUS_TRACE(log, "deconstructor");
}

void SSMoudle::Start(Json::Value address)
{
    Json::Value tmp;
	address[1U] = address[0U];
	address[1U].removeMember("redundance_ip", &tmp);
	address[1U]["ipaddr"] = tmp["redundance_ip"]; //address[1U].removeMember("redundance_ip");
	address[0U].removeMember("redundance_ip");
	TcpClientModule::Start(address);
}

void SSMoudle::Stop()
{
	TcpClientModule::Stop();
}

void SSMoudle::Login()
{
	Json::Value login;
	login["invokeID"] = std::to_string(++m_Own->m_invokeId);
	login["type"] = "request";
	login["request"] = "Connect";
	login["param"]["type"] = 5;//ivr type;
	login["param"]["LoginID"] = "acd";
	login["param"]["passWord"] = "***";
	login["param"]["version"] = "1.0.0.0";

	Json::FastWriter write;
	std::string data = write.write(login);
	Send(data.c_str(), data.length());
}

void SSMoudle::Ping()
{
	Json::Value ping;
	ping["invokeID"] = std::to_string(++m_Own->m_invokeId);
	ping["type"] = "request";
	ping["request"] = "HeartBeat";

	Json::FastWriter write;
	std::string data = write.write(ping);
	Send(data.c_str(), data.length());
}

void SSMoudle::onReceived(const std::string & data)
{
	LOG4CPLUS_DEBUG(log, "onReceived: [" << data << "]");
	Json::Value root;

    Json::CharReaderBuilder builder;
    builder["collectComments"] = false;
    JSONCPP_STRING errs;
    //Json::CharReader * reader = builder.newCharReader();
    Json::Reader reader;
    // 从data中读取数据到root
    //if (reader->parse(data.data(), data.data() + data.size(), &root, &errs))
	if (reader.parse(data, root) && root.isObject())
	{
		if (root["event"].asString() == "HeartBeat"){
			//LOG4CPLUS_DEBUG(log, "receive:" << data);
			this->ResetHearbeat();

			std::shared_ptr<CMessage> recvmsg(new CMessage());
			m_Own->SiaUnpackjson2Message(data, recvmsg);
			m_Own->SendMessageToMe(std::move(recvmsg));
		}
		else if (root["event"].asString() == "Connect")
		{
			if (root["status"].isInt() && root["status"].asInt() == 0) {
				SetLoginSuccess();
				return;
			}
			this->Close();
		}
		else {
			std::shared_ptr<CMessage> recvmsg(new CMessage());
			m_Own->SiaUnpackjson2Message(data, recvmsg);
			m_Own->SendMessageToMe(std::move(recvmsg));
		}
	}
	else {
		LOG4CPLUS_WARN(log, "abandon this message:" << data);
	}
}

