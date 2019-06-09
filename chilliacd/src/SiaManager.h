#ifndef __SIA_MANAGER_H_
#define __SIA_MANAGER_H_

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include "json/json.h"
#include <memory>
#include <map>
#include <thread>
#include "SiaSession.h"
#include "CEventBuffer.h"
#include <atomic>
#include "CMessage.h"
#include "event2/event.h"

#ifndef MAX_PATH
#define MAX_PATH 260
#endif // !MAX_PATH

class modlua;
class SiaTcpMonitor;
class SSMoudle;
class TcpServerModule;
class TcpClientModule;
class StatMoudle;

typedef helper::CEventBuffer<std::shared_ptr<CMessage>>  MANAGERQUEUE;
typedef uint32_t	SESSIONIDTYPE;


class CSIATimer;
class CTimerElement
{
private:
	CSIATimer*		m_CTimer;
	uint32_t		m_value; 
	uint32_t		m_id;
	uint32_t		m_interval;
	struct event *	m_eventtimer = nullptr;
public:
	CTimerElement(CSIATimer* CTimer, uint32_t id, uint32_t value, uint32_t interval):m_CTimer(CTimer)
		,m_value(value), m_id(id) ,m_interval(interval)
	{
	}

	~CTimerElement() {
		if (m_eventtimer) {
			event_free(m_eventtimer);
			m_eventtimer = nullptr;
		}
	}
	friend class CSIATimer;
};

typedef CTimerElement* CTimerElementPtr;

class CSIATimer
{
private:
	TSiaManager *       m_theSender;
	struct event_base * m_Base;
	std::thread			m_thread;
	std::map<uint32_t, CTimerElementPtr> m_TimerMap;
	helper::CEventBuffer<CTimerElementPtr> m_AddTimerMap;
	std::atomic<uint32_t> m_timerId;
	std::mutex m_mtx;
	log4cplus::Logger log;
public:

	explicit CSIATimer(TSiaManager* aSender):m_theSender(aSender), m_Base(nullptr),m_timerId(1)
	{
		this->log = log4cplus::Logger::getInstance("CSIATimer");
	}

	~CSIATimer() {
		if (m_thread.joinable()){
			Stop();
		}
		assert(m_Base == NULL);
	}

	void Stop() {
		for (auto it : m_TimerMap){
			delete it.second;
		}
		m_TimerMap.clear();
		if (m_thread.joinable()) {
			event_base_loopbreak(m_Base);
			m_thread.join();
		}
	}

	void Start()
	{
		if (m_thread.joinable()) {
			return;
		}
	
		m_thread = std::thread(&CSIATimer::run, this);
	}

	uint32_t timerSet(uint32_t value, uint32_t lefttime);
	uint32_t timerClear(uint32_t timerid);
private:
	void run();
	virtual void SendMessage(const CTimerElement* );
	static void processTimer(evutil_socket_t fd, short event, void *arg);
	static void addTimer(evutil_socket_t fd, short event, void *arg);
	void ProcessTimer(CTimerElementPtr e);
	void AddTimer();
};

class TSiaManager  {
public:
	explicit TSiaManager( const char *profile);
	virtual ~TSiaManager();

	void Start();
	void Stop();

	void   SendMessageToMe(std::shared_ptr<CMessage>&& tranInfo);

	bool SiaUnpackjson2Message(const std::string & data, std::shared_ptr<CMessage>& tranInfo);
	static SESSIONIDTYPE GenerationSessionId();

	const std::map<SESSIONIDTYPE, TSessionLuaPtr>& GetAllSession() const;

protected:
	uint32_t  timerClear(uint32_t timerID) { return m_Timer.timerClear(timerID); }
	uint32_t  timerSet(uint32_t value, uint32_t lefttime) { return m_Timer.timerSet(value, lefttime); }
	void   SendMessageToStation(int station, const std::string & sessionid, const std::string & cmd, const Json::Value & data);
	void   SendMessageToStation(int station, const std::string & sessionid, const std::string & cmd, const std::string & data);
	void   SendMessageToSS(const std::string & sessionid, const Json::Value & data);
	void   SendMessageToSS(const std::string & sessionid, const std::string & data);

	TSessionLuaPtr GetSessionByID(const SESSIONIDTYPE & id);
	SESSIONIDTYPE NewSession();
	TSessionLuaPtr RemoveSession(const SESSIONIDTYPE & sessionId);//session 使用的是智能指针，此函数并不是delete session，只是释放对session的引用计数。

	SESSIONIDTYPE GetSessionByCallId(const char * callid);
	SESSIONIDTYPE NewSessionByCallId(const char * callid);
	void AssociateSessionByCallId(SESSIONIDTYPE sessionId, const char * callid);

	const char * GetCallId(std::shared_ptr<CMessage>& tranInfo);

	virtual  bool  OnConnected(void);
	virtual  void OnDisconnect(void);
	bool   HandleMessage(std::shared_ptr<CMessage>& tranInfo);

	void DispatchThreadMessage(std::shared_ptr<CMessage>& tranInfo, SESSIONIDTYPE & session);

	std::thread m_MainThread;
	void run();

private:
	void LoadSiaConfig();

public:
	uint64_t m_invokeId;

private:
	MANAGERQUEUE      m_mainqueue;
	//helper::CEventBuffer<TSessionLuaPtr> m_ProcessSessions;
	log4cplus::Logger log;
	//std::vector<std::thread> m_Thread_Pool;
	char              m_profile[MAX_PATH];   //配置文件

	std::map<SESSIONIDTYPE, TSessionLuaPtr> m_Sessions;
	std::recursive_mutex	m_sessionMtx;
	//std::list<TSessionLuaPtr> m_IdleSessions;
	CSIATimer         m_Timer;

	std::map<std::string, SESSIONIDTYPE>   m_Callid2SessionMap;

	std::string       m_SiaServerIp;
	uint32_t          m_TcpPort = -1;
	TcpServerModule * m_TCPServerModule = nullptr;
	TcpClientModule * m_CMClientModule = nullptr;
	uint32_t          m_stationno;
	std::string       m_version;
	std::string       m_serveruri;
	std::string	      m_FlowPath;

    // add begin
    std::string       m_IPv4;
    uint32_t          m_IPv4PORT;
    std::string       m_CMServerIp;
    uint32_t          m_CMPort;
    // add end

	//uint32_t		  m_max_thread_num;
	uint32_t		  m_max_session_num;
	friend class TSessionLua;
	friend class SiaTcpMonitor;
	friend class SSMoudle;
	friend class StatMoudle;
	friend class CSIATimer;
};

#endif
