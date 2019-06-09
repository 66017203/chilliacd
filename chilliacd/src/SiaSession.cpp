#include <stdint.h>
#include <string>
#include "../acdd.h"
#include "SiaManager.h"
#include "siaserver.h"
#include <sstream>
#include <log4cplus/loggingmacros.h>
#include "stringHelper.h"
#include <mutex>
#include "SiaSession.h"
#include "../tinyxml2/tinyxml2.h"
#include "tls.h"
#include "ACDCallNum.h"

std::mutex g_LuaMtx;

////////////////////////start TSessionLua ////////////////////
void verbose(char * type, char *fmt)
{
	static log4cplus::Logger log = log4cplus::Logger::getInstance("Session");
	if (std::string(type) == "warn")
		LOG4CPLUS_WARN(log, fmt);
	else if (std::string(type) == "error")
		LOG4CPLUS_ERROR(log,fmt);
	else
		LOG4CPLUS_INFO(log, fmt);
	return;
}

TSessionLua::TSessionLua(uint32_t id, const std::string & initial, TSiaManager *owner)
	:m_TimerOutId(0), m_initial(initial), /*m_luastate(NULL),*/ m_sessionId(id), m_owner(owner)
{
	Start();
    mysql.LoadConfig("../conf/acd.cfg");
    //LOG4CPLUS_TRACE(log, m_sessionId << " TSessionLua:" << this << " constructor.");
}

TSessionLua::~TSessionLua()
{
	Stop();
	//LOG4CPLUS_TRACE(log, m_sessionId << " TSessionLua:" << this << " desconstruct.");
}

void TSessionLua::reset()
{
	ClearEventToStateMap();
	m_Caller.clear();
	m_Called.clear();
	m_OrigCaller.clear();
	m_OrigCalled.clear();
	m_TimerOutId = 0;
	m_callclearedRegister.clear();
	m_hangupRegister.clear();

	LOG4CPLUS_DEBUG(log, m_sessionId << " <reset>");
}

//根据事件获取具体LUA函数名称
/*
1:先根据事件找函数。
2：找不到，就找通配符
3：再找不到，返回空
*/
const std::string TSessionLua::GetStateByEvent(const std::string & eventname)
{

	auto it = m_event_to_next_state_map.find(eventname);
	if (it == m_event_to_next_state_map.end()) {
		it = m_event_to_next_state_map.find("*");
		if (it == m_event_to_next_state_map.end()) {
			return "";
		}
		else {
			return it->second;
		}
	}
	else {
		return it->second;
	}
}

//检查是否订阅新事件
bool TSessionLua::IsExistSubscribeEvent()
{
	if (0 < m_event_to_next_state_map.size())
		return true;
	else
		return false;
}

bool TSessionLua::IsClose()
{
	return !IsExistSubscribeEvent();
}

void TSessionLua::ClearEventToStateMap()
{
	m_event_to_next_state_map.clear();
}

void TSessionLua::SetHangupFunction(const char *function_name)
{
	if (function_name)
		m_hangupRegister = function_name;
}

void TSessionLua::SetStopServiceFunction(const char *function_name)
{
	if (function_name)
		m_callclearedRegister = function_name;
}

// 订阅事件，跳转到具体函数 subscribe
void TSessionLua::AddSubscribeEvent(const char * event_name, const char *function_name)
{
	// 函数不能为空；事件为空，表示可以处理任意事件，用通配符"*"表示
	if (!function_name)
		return;

	if (event_name == NULL) {
		m_event_to_next_state_map["*"] = function_name;
	}
	else {
		m_event_to_next_state_map[event_name] = function_name;
	}
	return;
}

void TSessionLua::HandleUserMsg(std::shared_ptr<CMessage> &tranInfo)
{
	if (tranInfo->cmd.empty() || tranInfo->dataString.empty())
	{
		LOG4CPLUS_ERROR(log, m_sessionId << " <HandleUserMsg> gets json or evenname error, return");
		return ;
	}
	else
    {
		LOG4CPLUS_DEBUG(log, m_sessionId << " <HandleUserMsg> received [" << tranInfo->dataString << "]");
	}

    /*
	const std::string actionname = GetStateByEvent(tranInfo->cmd);
	if (actionname.empty())
	{
		LOG4CPLUS_WARN(log, m_sessionId << " <HandleUserMsg> received [" << tranInfo->cmd << "], no action, return");
		return ;
	}
    */

	ClearEventToStateMap();
    LOG4CPLUS_WARN(log, m_sessionId << " [" << tranInfo->cmd << "] [" << tranInfo->dataString << "]");
	// DoLuaAction(actionname.c_str(), tranInfo->dataString.c_str(), tranInfo->cmd.c_str());
    if (0 == tranInfo->cmd.compare("acdRoute"))
    {
        //acd_request_route(tranInfo->jsonData);
        acd_request_route(tranInfo->dataString);
    }
    else if (0 == tranInfo->cmd.compare("acdRouteEx"))
	{
		acd_request_route_ex(tranInfo->dataString);
	}
    else if (0 == tranInfo->cmd.compare("acdReRoute"))
	{
		acd_reroute(tranInfo->dataString);
	}
	else if (0 == tranInfo->cmd.compare("acdRouteUsed"))
	{
		acd_route_used(tranInfo->dataString);
	}
	else if (0 == tranInfo->cmd.compare("acdRouteEnd"))
	{
		acd_route_end(tranInfo->dataString);
	}
}

void TSessionLua::HandleCallCleared(std::shared_ptr<CMessage> &tranInfo)
{
	LOG4CPLUS_DEBUG(log, m_sessionId << " receive :" << tranInfo->dataString);

	if (!m_callclearedRegister.empty())
	{
		ClearEventToStateMap();
	}
}

void TSessionLua::SetCaller(const char * caller)
{
	if (caller)
		m_Caller = caller;
}

void TSessionLua::SetCalled(const char * called)
{
	if (called)
		m_Called = called;
}

const char * TSessionLua::GetCaller()
{
	return m_Caller.c_str();
}

const char * TSessionLua::GetCalled()
{
	return m_Called.c_str();
}

const char * TSessionLua::GetScriptFile()
{
	return m_ScriptFile.c_str();
}

const char * TSessionLua::GetCurrentState()
{
	return m_CurrentState.c_str();
}

void TSessionLua::SetOrigCaller(const char * origcaller)
{
	if (origcaller)
		m_OrigCaller = origcaller;
}

void TSessionLua::SetOrigCalled(const char * origcalled)
{
	if (origcalled)
		m_OrigCalled = origcalled;
}

int TSessionLua::GetStationNo()
{
	return m_owner->m_stationno;
}

void TSessionLua::SetScriptFile(const char * file)
{
	if (file){
		m_ScriptFile = file;
	}
}

// log level
void TSessionLua::trace(const char * msg)
{
	LOG4CPLUS_TRACE(log, m_sessionId << " " << (msg ? msg : ""));
}

void TSessionLua::debug(const char * msg)
{
	LOG4CPLUS_DEBUG(log, m_sessionId << " " << (msg ? msg : ""));
}

void TSessionLua::info(const char * msg)
{
	LOG4CPLUS_INFO(log, m_sessionId << " " << (msg ? msg : ""));
}

void TSessionLua::warn(const char * msg)
{
	LOG4CPLUS_WARN(log, m_sessionId << " " << (msg ? msg : ""));
}

void TSessionLua::error(const char * msg)
{
	LOG4CPLUS_ERROR(log, m_sessionId << " " << (msg ? msg : ""));
}

void TSessionLua::acd_request_route(const std::string & data)
{
    LOG4CPLUS_DEBUG(log, m_sessionId << " " << __FUNCTION__ << " START [" << data << "]");
	if (!m_connection.empty())
    {
		Json::Reader reader;
		Json::Value tmp;
		if (reader.parse(data, tmp))
		{
			Json::Value json;
			Json::StyledWriter writer;
			LOG4CPLUS_DEBUG(log, m_sessionId << " : \n" << writer.write(tmp));
			std::string caller = tmp["param"]["caller"].asString();
			std::string callee = tmp["param"]["callee"].asString();

			GatewayManager * p = &GatewayManager::get_instance();
			p->get_data_from_redis();
			p->get_route_by_caller(caller);
			p->get_modified_callee_by_callee(callee);
			//p->update_current_sessions_by_route_id(1, 1);
			p->get_result(json, caller);

			Json::Value jsonData;
			jsonData["invokeID"] = this->getSessionID();
			jsonData["type"] = "response";
			jsonData["response"] = Evt_acdRoute;
			jsonData["status"] = 0;
			jsonData["param"][SER_Connection] = m_connection;
			jsonData["param"]["callee"] = callee;
			jsonData["param"]["route_ip"] = json[0U]["route_ip"];
			jsonData["param"]["route_port"] = json[0U]["route_port"];
			jsonData["param"]["route_id"] = json[0U]["route_id"];
			jsonData["param"]["caller"] = caller;
			jsonData["param"]["company_id"] = tmp["param"]["company"];

			m_owner->SendMessageToSS(std::to_string(this->getSessionID()), jsonData);
		}
		else
		{

		}
    }
    else
	{
		LOG4CPLUS_ERROR(log, m_sessionId <<  Evt_acdRoute << " not send, no callid");
	}

	return ;
}

void TSessionLua::acd_request_route(const Json::Value & data)
{
	if (!m_connection.empty())
    {
        Json::StyledWriter writer;
        writer.write(data);
        LOG4CPLUS_DEBUG(log, m_sessionId << " acd_route " << writer.write(data));

		Json::Value jsonData;
        m_owner->SendMessageToSS(std::to_string(this->getSessionID()), jsonData);
    }
    else
	{
		LOG4CPLUS_ERROR(log, m_sessionId << "acdRequestRoute not send, no callid " << data);
	}

	return ;
}

void TSessionLua::acd_request_route_ex(const std::string & data)
{
    LOG4CPLUS_DEBUG(log, m_sessionId << " " << __FUNCTION__ << " START [" << data << "]");
	if (!m_connection.empty())
    {
		Json::Reader reader;
		Json::Value tmp;
		if (reader.parse(data, tmp))
		{
			Json::Value json, json2;
			Json::StyledWriter writer;
			LOG4CPLUS_DEBUG(log, m_sessionId << " : \n" << writer.write(tmp));
			std::string caller = tmp["param"]["caller"].asString();
			std::string callee = tmp["param"]["callee"].asString();
			std::string caller2 = callee;
			std::string callee2 = caller;

			GatewayManager * p = &GatewayManager::get_instance();
			p->get_data_from_redis();
			p->get_route_by_caller(caller);
			p->get_modified_callee_by_callee(callee);
			p->get_result(json, caller);

			p->get_route_by_caller(caller2);
			p->get_modified_callee_by_callee(callee2);
			p->get_result(json2, caller2);

			Json::Value jsonData;
			jsonData["invokeID"] = this->getSessionID();
			jsonData["type"] = "response";
			jsonData["response"] = Evt_acdRoute;
			jsonData["status"] = 0;
			jsonData["param"][SER_Connection] = m_connection;
			jsonData["param"]["callee"] = callee;
			jsonData["param"]["route_ip"] = json[0U]["route_ip"];
			jsonData["param"]["route_port"] = json[0U]["route_port"];
			jsonData["param"]["route_id"] = json[0U]["route_id"];
			jsonData["param"]["caller"] = caller;
			jsonData["param"]["company_id"] = tmp["param"]["company"];
			jsonData["param"]["callee2"] = callee2;
			jsonData["param"]["route_ip2"] = json2[0U]["route_ip"];
			jsonData["param"]["route_port2"] = json2[0U]["route_port"];
			jsonData["param"]["route_id2"] = json2[0U]["route_id"];
			jsonData["param"]["caller2"] = caller2;

			m_owner->SendMessageToSS(std::to_string(this->getSessionID()), jsonData);
		}
		else
		{

		}
    }
    else
		LOG4CPLUS_ERROR(log, m_sessionId <<  Evt_acdRouteEx << " not send, no callid");

	return ;
}

// TODO
void TSessionLua::acd_reroute(const std::string & data)
{
	if (!m_connection.empty())
	{
		Json::Reader reader;
		Json::Value  json;

		Json::Value jsonData;
        m_owner->SendMessageToSS(std::to_string(this->getSessionID()), jsonData);
	}
	else
	{

	}

	return ;
}

// TODO
void TSessionLua::acd_route_used(const std::string & data)
{
    LOG4CPLUS_DEBUG(log, m_sessionId << " " << __FUNCTION__ << " " << data);
	if (!m_connection.empty())
	{
		Json::Reader reader;
		Json::Value  json;
		if (reader.parse(data, json)) {
			GatewayManager * p = &GatewayManager::get_instance();
			p->get_data_from_redis();
			p->update_current_sessions_by_route_id(json["param"]["route_id"].asInt(), 1);


			Json::Value jsonData;
			jsonData["invokeID"] = this->getSessionID();
			jsonData["type"] = "response";
			jsonData["response"] = Evt_acdRouteUsed;
			jsonData["status"] = 0;
			jsonData["param"][SER_Connection] = m_connection;

			m_owner->SendMessageToSS(std::to_string(this->getSessionID()), jsonData);
		}
	}
	else
	{

	}

	return ;
}

// TODO
void TSessionLua::acd_route_end(const std::string & data)
{
    LOG4CPLUS_DEBUG(log, m_sessionId << " " << __FUNCTION__ << " START " << data);
	if (!m_connection.empty())
	{
		Json::Reader reader;
		Json::Value  json;
		if (reader.parse(data, json))
		{
			GatewayManager * p = &GatewayManager::get_instance();
			p->get_data_from_redis();
			p->update_current_sessions_by_route_id(json["param"]["route_id"].asInt(), 0);

			Json::Value jsonData;
			jsonData["invokeID"] = this->getSessionID();
			jsonData["type"] = "response";
			jsonData["response"] = Evt_acdRouteEnd;
			jsonData["status"] = 0;
			jsonData["param"][SER_Connection] = m_connection;

			m_owner->SendMessageToSS(std::to_string(this->getSessionID()), jsonData);
		}
	}
	else
	{

	}

    LOG4CPLUS_DEBUG(log, m_sessionId << " " << __FUNCTION__ << " END " << data);
	return ;
}

void TSessionLua::endconsultservice(const char * userdata)
{
}

void TSessionLua::stopservice()
{
	/*
	 *if (m_connection.empty())
	 *{
	 *    LOG4CPLUS_ERROR(log, m_sessionId << "ClearCall not send,no callid");
	 *    return;
	 *}
	 *Json::Value jsonData;
	 *jsonData["invokeID"] = this->getSessionID();
	 *jsonData["type"] = "request";
	 *jsonData["param"][SER_Connection] = m_connection;
	 *jsonData["request"] = "ClearCall";
	 *LOG4CPLUS_DEBUG(log, m_sessionId << " ClearCall ");
	 *m_owner->SendMessageToSS(std::to_string(this->getSessionID()), jsonData);
	 *return;
	 */
}

void TSessionLua::HandleMessage(std::shared_ptr<CMessage> &tranInfo)
{
	if (-1 != tranInfo->station) {
		this->m_fromStation = tranInfo->station;
	}

	if (tranInfo->cmd == Evt_CallCleared) {
		HandleCallCleared(tranInfo);
		return;
	}

	LOG4CPLUS_DEBUG(log, m_sessionId << " [\n" << tranInfo->jsonData["param"] << "\n]");
	if (tranInfo->jsonData.isObject()) {
		if (tranInfo->jsonData["param"].isMember(SER_Connection))
			this->m_connection = tranInfo->jsonData["param"][SER_Connection];
	}

	HandleUserMsg(tranInfo);
	return;
}











uint32_t TSessionLua::settimer(uint32_t m_second)
{
	m_TimerOutId = m_owner->timerSet(m_sessionId, m_second);
	LOG4CPLUS_DEBUG(log, m_sessionId << " set timerid:" << m_TimerOutId << "," << m_second << "ms");
	return m_TimerOutId;
}

void TSessionLua::cleartimer()
{
	if (m_TimerOutId)
		m_owner->timerClear(m_TimerOutId);
	m_TimerOutId = 0;
}

void TSessionLua::Start()
{
	if (m_thread.joinable())
		return;
	m_thread = std::thread(&TSessionLua::run, this);
}

void TSessionLua::Stop()
{
	if (m_thread.joinable()){
		std::shared_ptr<CMessage> info(new CMessage(0, 0, Evt_Manager_Stop));
		this->m_EventQueue.Put(info);
		m_thread.join();
	}
}

void TSessionLua::run()
{
	TSessionLuaPtr session;
	std::shared_ptr<CMessage> tranInfo;
	this->log = log4cplus::Logger::getInstance("Session");
	LOG4CPLUS_DEBUG(log, m_sessionId << " start running.");
	tls_key_type tls_key = tls_init(nullptr);
#if WIN32
	LOG4CPLUS_DEBUG(log, m_sessionId << "tls_key:" << tls_key);
#else
	if (tls_key) {
		LOG4CPLUS_DEBUG(log, m_sessionId << "tls_key:" << *tls_key);
		tls_cleanup(tls_key);
	}
	else
		LOG4CPLUS_ERROR(log, m_sessionId << "tls_key error");
#endif

	try {
		bool no_timeout = true;

		while ((no_timeout = this->m_EventQueue.Get(tranInfo, 1000 * 60 * 60 * 1))) { // 执行完现有队列里的event
			if (Evt_Manager_Stop == tranInfo->cmd)
				break;

			this->HandleMessage(tranInfo);

			if (this->IsClose()){
				this->m_thread.detach();
				session = m_owner->RemoveSession(this->m_sessionId);
				break;
			}
		}

		if (no_timeout == false){
			this->stopservice();
			this->m_thread.detach();
			session = m_owner->RemoveSession(this->m_sessionId);
		}
	} catch (const std::exception & e) {
		LOG4CPLUS_ERROR(log, m_sessionId << " " << e.what());
	}

	LOG4CPLUS_DEBUG(log, m_sessionId << " stoped.");
	log4cplus::threadCleanup();
}

