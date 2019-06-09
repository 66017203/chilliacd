#include "SiaManager.h"
#include "../acdd.h"
#include "SiaSession.h"
#include "SiaHttpServer.h"
#include "SiaTcpMonitor.h"
#include <sstream>
#include <log4cplus/loggingmacros.h>
#include "HttpRequest.h"
#include "json/json.h"
#include "SSModule.h"
#include "SiaTcpServer.h"
#include "StatModule.h"
#include "cfgFile.h"
#include "stringHelper.h"

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#else
#include <mysql.h>
#include <errmsg.h>
#endif

TSiaManager *g_siaManager = NULL;
extern const char *siaversion;
extern std::atomic<bool> g_Running;

#define MIN_SESSION_NUM			1U
#define MIN_THREAD_NUM			1U

void siaSleep(int millsecond)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(millsecond));
}

void CSIATimer::run()
{
	LOG4CPLUS_DEBUG(log, "Running...");
	m_Base = event_base_new();

	LOG4CPLUS_DEBUG(log, "current method [" << event_base_get_method(m_Base) << "]");

	struct event * ev_addTimer = event_new(m_Base, -1, EV_PERSIST, addTimer, this);

	struct timeval tv;
	evutil_timerclear(&tv);
	tv.tv_usec = 50 * 1000;
	event_add(ev_addTimer, &tv);

	event_base_dispatch(m_Base);

	event_del(ev_addTimer);
	event_free(ev_addTimer);

	event_base_free(m_Base);
	m_Base = NULL;

	LOG4CPLUS_DEBUG(log, "Stoped");
	log4cplus::threadCleanup();
}

uint32_t CSIATimer::timerSet(uint32_t value, uint32_t lefttime)
{
	uint32_t id = m_timerId++;
	CTimerElementPtr timerEle = new CTimerElement(this, id, value, lefttime);
	m_AddTimerMap.Put(timerEle);
	return id;
}

uint32_t CSIATimer::timerClear(uint32_t timerid)
{
	std::unique_lock<std::mutex> lck(m_mtx);
	m_TimerMap.erase(timerid);
	return timerid;
}

void CSIATimer::SendMessage(const CTimerElement * ctimer)
{
	if (ctimer)
	{
		std::shared_ptr<CMessage> tranInfo(new CMessage(ctimer->m_value, ctimer->m_value, Evt_TIMEOUT));

		Json::Value timer;
		Json::FastWriter writer;
		timer["timerid"] = ctimer->m_id;

		std::string jsonstr = writer.write(timer);
		tranInfo->dataString = jsonstr;
		tranInfo->jsonData = timer;

		m_theSender->HandleMessage(tranInfo);
	}
	return ;
}

void CSIATimer::processTimer(evutil_socket_t fd, short event, void *arg)
{
	CTimerElement * e = (CTimerElement *)arg;
	CSIATimer *This = e->m_CTimer;
	return This->ProcessTimer(e);
}

void CSIATimer::addTimer(evutil_socket_t fd, short event, void * arg)
{
	CSIATimer *This = (CSIATimer *)arg;
	return This->AddTimer();
}

void CSIATimer::ProcessTimer(CTimerElementPtr e)
{
	m_mtx.lock();
	bool bFind = m_TimerMap.erase(e->m_id) != 0;
	m_mtx.unlock();

	if (bFind)
		this->SendMessage(e);

	delete e;
}

void CSIATimer::AddTimer()
{
	CTimerElementPtr it;
	while (m_AddTimerMap.Get(it, 0))
    {
		struct event * etimer = evtimer_new(m_Base, processTimer, it);
		it->m_eventtimer = etimer;

		struct timeval tv;
		evutil_timerclear(&tv);
		tv.tv_sec = it->m_interval / 1000;
		tv.tv_usec = (it->m_interval % 1000) * 1000;
		evtimer_add(etimer, &tv);
		std::unique_lock<std::mutex> lck(m_mtx);
		m_TimerMap[it->m_id] = it;
	}
}

TSiaManager::TSiaManager( const char *profile)
	:m_Timer(this), m_invokeId(0), m_stationno(0), m_max_session_num(MIN_SESSION_NUM)
{
	log = log4cplus::Logger::getInstance("SiaManager");
	g_siaManager = this;

	memset(m_profile, 0, MAX_PATH);
	if (profile)
		strncpy(m_profile, profile, MAX_PATH - 1);
}

TSiaManager::~TSiaManager()
{
	if (m_MainThread.joinable())
    {
		Stop();
	}

	assert(m_Sessions.size() == 0);
	assert(m_Callid2SessionMap.size() == 0);

    g_siaManager = NULL;
}

/*******************************************************************************
 * 读取配置信息
 * 配置文件: ../conf/sia.cfg
 ******************************************************************************/
void TSiaManager::LoadSiaConfig()
{
	char buffer[MAX_PATH] = { 0 };
    // 读取相关配置
	m_max_session_num = cfg_GetPrivateProfileIntEx("SIA", "session_num", MIN_SESSION_NUM, m_profile);
	m_max_session_num = max(m_max_session_num, MIN_SESSION_NUM);
	LOG4CPLUS_DEBUG(log, "max session:" << m_max_session_num);

    m_SiaServerIp = cfg_GetPrivateProfileStringEx("SELF", "ipv4", "", buffer, sizeof(buffer), m_profile);
    m_TcpPort = cfg_GetPrivateProfileIntEx("SELF", "port", 10, m_profile);
    m_FlowPath = cfg_GetPrivateProfileStringEx("SELF", "scriptdir", "./sys_scripts/", buffer, sizeof(buffer), m_profile);
    m_CMServerIp = cfg_GetPrivateProfileStringEx("CM", "ipv4", "", buffer, sizeof(buffer), m_profile);
    m_CMPort = cfg_GetPrivateProfileIntEx("CM", "port", 10, m_profile);

	LOG4CPLUS_DEBUG(log, "CM  tcp [" << m_CMServerIp << ":" << m_CMPort << "]");
	LOG4CPLUS_DEBUG(log, "SELF tcp:[" << m_SiaServerIp << ":" << m_TcpPort << "]");
	LOG4CPLUS_DEBUG(log, "Sia flow path: [" << m_FlowPath << "]");
}

void TSiaManager::Start()
{
	//启动Manager线程
	mysql_library_init(0, NULL, NULL);
	if (true == OnConnected())
    {
		m_Timer.Start();
		m_MainThread = std::thread(&TSiaManager::run, this);
		//m_pMonitor->Alarm("SiaManager", "info", "start.");
		LOG4CPLUS_DEBUG(log, " Start...");
	}
	else
    {
		LOG4CPLUS_ERROR(log, " Start error.");
	}
}

void TSiaManager::Stop()
{
	g_Running = false;
	std::cin.clear();

	if (m_MainThread.joinable())
	{
		std::shared_ptr<CMessage> tranInfo(new CMessage(0, 0, Evt_Manager_Stop));
		m_mainqueue.Put(std::move(tranInfo));

		m_MainThread.join();
	}

	m_Timer.Stop();

	//m_pMonitor->Alarm("SiaManager", "error", "stop.");
	OnDisconnect();
	LOG4CPLUS_INFO(log, "Stoped.");
}

void TSiaManager::run()
{
	std::shared_ptr<CMessage> tranInfo;

	while (true)
	{
		if (m_mainqueue.Get(tranInfo, 1000 * 60))
        {
			LOG4CPLUS_DEBUG(log, " queue size:" << m_mainqueue.size());
			if (Evt_Manager_Stop == tranInfo->cmd)
				break;
			else
				HandleMessage(tranInfo);
		}
	}

	log4cplus::threadCleanup();
}

bool TSiaManager::OnConnected(void)
{
	m_stationno = cfg_GetPrivateProfileIntEx("SIA", "stationno", 10, m_profile);
	m_version = siaversion;
	//m_pMonitor = new SiaTcpMonitor(this);
	LoadSiaConfig();

/////////////////
	m_TCPServerModule = new SiaTCPServer(this, m_SiaServerIp.c_str(), m_TcpPort);
	m_TCPServerModule->Start();

	//Json::Value address;
	//address[0u]["ipaddr"] = m_CMServerIp;
	//address[0u]["socketport"] = m_CMPort;
	//m_CMClientModule = new SSMoudle(this);
	//m_CMClientModule->Start(address);
//////////////////

	return true;
}

void TSiaManager::OnDisconnect(void)
{
	//for (auto it = m_Thread_Pool.begin(); it != m_Thread_Pool.end(); it++){
	//	if (it->joinable())
	//		it->join();
	//}
	//m_Thread_Pool.clear();

	m_Sessions.clear();
	m_Callid2SessionMap.clear();

	if (m_TCPServerModule) {
		m_TCPServerModule->Stop();
		delete m_TCPServerModule;
		m_TCPServerModule = nullptr;
	}

	std::this_thread::sleep_for(std::chrono::milliseconds(200));

	return ;
}

/*******************************************************************************
 *
 * TODO
 ******************************************************************************/
bool TSiaManager::HandleMessage(std::shared_ptr<CMessage>& tranInfo)
{
	LOG4CPLUS_DEBUG(log, " IN FUNCTION HandleMessage, RECEIVES DATA ["
			<< tranInfo->dataString << "]\n cmd ["
			<< tranInfo->cmd << "]");
	SESSIONIDTYPE sessionId;

	if (tranInfo->receiver) { // 消息有sessionID，直接派送。
		sessionId = tranInfo->receiver;
        LOG4CPLUS_DEBUG(log, "event: [" << tranInfo->cmd << "]");
		goto dipatch;
	}

	LOG4CPLUS_DEBUG(log, "CMD [" << tranInfo->cmd << "]");

    if (tranInfo->cmd == Evt_Received)
    {
		sessionId = NewSession();
		AssociateSessionByCallId(sessionId, GetCallId(tranInfo));
	}
    else if (tranInfo->cmd == Evt_acdRoute)
    {
		sessionId = NewSession();
		AssociateSessionByCallId(sessionId, GetCallId(tranInfo));
    }
    else if (tranInfo->cmd == Evt_acdRouteEx)
	{
		sessionId = NewSession();
		AssociateSessionByCallId(sessionId, GetCallId(tranInfo));
	}
    else if (tranInfo->cmd == Evt_acdReRoute)
	{
		sessionId = NewSession();
		AssociateSessionByCallId(sessionId, GetCallId(tranInfo));
	}
    else if (tranInfo->cmd == Evt_acdRouteUsed)
	{
		sessionId = NewSession();
		AssociateSessionByCallId(sessionId, GetCallId(tranInfo));
	}
    else if (tranInfo->cmd == Evt_acdRouteEnd)
	{
		sessionId = NewSession();
		AssociateSessionByCallId(sessionId, GetCallId(tranInfo));
	}
    else if (tranInfo->cmd == "HeartBeat")
    {
		sessionId = NewSession();
		AssociateSessionByCallId(sessionId, GetCallId(tranInfo));
    }
    else
    {
		sessionId = GetSessionByCallId(GetCallId(tranInfo));
	}

dipatch:
	if (sessionId) {
		DispatchThreadMessage(tranInfo, sessionId);
	}
	else {
		LOG4CPLUS_WARN(log, "CAN NOT FIND SESSION:" << tranInfo->dataString);
	}

	return true;
}

void TSiaManager::DispatchThreadMessage(std::shared_ptr<CMessage>& tranInfo, SESSIONIDTYPE & sessionId)
{
	std::lock_guard<std::recursive_mutex> lck(m_sessionMtx);
	const auto & it = m_Sessions.find(sessionId);
	if (it != m_Sessions.end()) {
		it->second->m_EventQueue.Put(tranInfo);
	}
	return;
}

TSessionLuaPtr TSiaManager::GetSessionByID(const SESSIONIDTYPE & id)
{
	std::lock_guard<std::recursive_mutex> lck(m_sessionMtx);
	const auto & it  = m_Sessions.find(id);
	if (it != m_Sessions.end()){
		return it->second;
	}
	return nullptr;
}

SESSIONIDTYPE TSiaManager::NewSession()
{
	m_sessionMtx.lock();
	if (m_Sessions.size() >= m_max_session_num){
		LOG4CPLUS_ERROR(log, "Session count:" << m_Sessions.size() << ",  have reached the maximum session.");
		//m_pMonitor->Alarm("SiaManager", "error", "have reached the maximum session");
		m_sessionMtx.unlock();
		return 0;
	}

	SESSIONIDTYPE id = GenerationSessionId();

	Json::FastWriter writer;
	//std::string initial = writer.write(m_companyConfig);
	//initial = "company=JSON.decode(\"" + helper::string::replaceString(initial, "\"", "\\\"") + "\");";
	TSessionLuaPtr session(new TSessionLua(id, "", this));

	m_Sessions.insert(std::make_pair(id, session));
	m_sessionMtx.unlock();
	LOG4CPLUS_DEBUG(log, "session size:" << m_Sessions.size());
    return session->m_sessionId;

}

TSessionLuaPtr TSiaManager::RemoveSession(const SESSIONIDTYPE & sessionId)
{
	LOG4CPLUS_DEBUG(log, "begin session size: [" << m_Sessions.size() << "]");
	std::lock_guard<std::recursive_mutex> lck(m_sessionMtx);
	TSessionLuaPtr session;
	do{
		const auto & it = m_Sessions.find(sessionId);
		if (it != m_Sessions.end()){
			session = it->second;
			const auto & it2 = m_Callid2SessionMap.find(it->second->getConnection()["callID"].asString());
			if (it2 != m_Callid2SessionMap.end())
			{
				m_Callid2SessionMap.erase(it2);
			}
			m_Sessions.erase(it);
		}
	} while (0);

	LOG4CPLUS_DEBUG(log, "session size: [" << m_Sessions.size() << "]");

	return session;
}

void TSiaManager::SendMessageToMe(std::shared_ptr<CMessage>&& tranInfo)
{
	m_mainqueue.Put(std::move(tranInfo));
}

void TSiaManager::SendMessageToStation(int station, const std::string & sessionid, const std::string & cmd, const Json::Value & jsonData)
{
	Json::FastWriter writer;
	std::string strData = writer.write(jsonData);
	return SendMessageToStation(station, sessionid, cmd, strData);
}

void TSiaManager::SendMessageToStation(int station, const std::string & sessionid, const std::string & cmd, const std::string & data)
{
	m_TCPServerModule->PutMessage(station, sessionid, data);
}

void TSiaManager::SendMessageToSS(const std::string & sessionid, const Json::Value & jsonData)
{
	//LOG4CPLUS_DEBUG(log, sessionid << " ++++++++++++++++++++++++++++++++++++++++++++++++ ");
	Json::FastWriter writer;
	std::string strData = writer.write(jsonData);
	return SendMessageToSS(sessionid, strData);
}

void TSiaManager::SendMessageToSS(const std::string & sessionid, const std::string & strData)
{
	//return m_CMClientModule->PutMessage(sessionid, strData);
	LOG4CPLUS_DEBUG(log, sessionid << " [" << strData << "]");
	return m_TCPServerModule->PutMessage(10, sessionid, strData);
}

SESSIONIDTYPE TSiaManager::GetSessionByCallId(const char * callid)
{
	if (callid == NULL){
		return 0;
	}

	std::lock_guard<std::recursive_mutex> lck(m_sessionMtx);

	const auto & it = m_Callid2SessionMap.find(std::string(callid));
	if (m_Callid2SessionMap.end() != it){
		return it->second;
	}
	LOG4CPLUS_WARN(log, "callid:" << callid << " not find session.");
	return 0;
}

SESSIONIDTYPE TSiaManager::NewSessionByCallId(const char * callid)
{
	if (callid == NULL){
		return 0;
	}

	std::lock_guard<std::recursive_mutex> lck(m_sessionMtx);
	const auto & it = m_Callid2SessionMap.find(std::string(callid));
	if (m_Callid2SessionMap.end() != it){
		return it->second;
	}

	SESSIONIDTYPE sessionId = NewSession();
	if (sessionId)
		m_Callid2SessionMap.insert(std::make_pair(callid, sessionId));

	return sessionId;
}

void TSiaManager::AssociateSessionByCallId(SESSIONIDTYPE sessionId, const char * callid)
{
	if (callid == NULL) {
		return ;
	}

	std::lock_guard<std::recursive_mutex> lck(m_sessionMtx);
	const auto & it = m_Callid2SessionMap.find(std::string(callid));
	if (m_Callid2SessionMap.end() != it) {
		return ;
	}

	m_Callid2SessionMap.insert(std::make_pair(callid, sessionId));
}

const char * TSiaManager::GetCallId(std::shared_ptr<CMessage>& tranInfo)
{
	const char *p_callid = NULL;
	if (tranInfo->jsonData["param"][SER_CallID].isString()){
		p_callid = tranInfo->jsonData["param"][SER_CallID].asCString();
	}

	return p_callid;
}

bool TSiaManager::SiaUnpackjson2Message(const std::string & data, std::shared_ptr<CMessage>& tranInfo)
{
	Json::Reader reader;
	tranInfo->dataString = data;

	if (!data.empty() && tranInfo->jsonData.isNull()){
		reader.parse(data, tranInfo->jsonData);
	}

	if (!tranInfo->cmd.empty())
		return true;

	Json::Value & json = tranInfo->jsonData;

	if(json.isObject() && json["event"].isString()){
		tranInfo->cmd = json["event"].asString();
	}

	if (json.isObject() && json["event"].isString()) {
		tranInfo->cmd = json["event"].asString();
		if(json["invokeID"].isUInt())
			tranInfo->receiver = json["invokeID"].asUInt();
	}

	return true;
}

SESSIONIDTYPE TSiaManager::GenerationSessionId()
{
	static std::atomic_ulong sessionid(10000);
	return sessionid++;
}

const std::map<SESSIONIDTYPE, TSessionLuaPtr> & TSiaManager::GetAllSession()const
{
	return m_Sessions;
}

//////////////////////end TSiaManager/////////////////////

