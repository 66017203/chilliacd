#ifndef __SIA_SESSION_H_
#define __SIA_SESSION_H_

//#include "lua.hpp"
#include "CMessage.h"
#include <string>
#include <map>
#include <log4cplus/logger.h>
#include <memory>
#include <thread>
#include "CEventBuffer.h"
#include "json/json.h"
#include "MySql.h"

class TSiaManager;

using namespace std;


class TSessionLua {
public:
	TSessionLua(uint32_t id, const std::string & initial, TSiaManager *owner);
	~TSessionLua();


	uint32_t getSessionID() const {
		return m_sessionId;
	}

	const Json::Value & getConnection() const {
		return m_connection;
	}

	const char * getScriptPath();

	////////////lua action ///////////////////
	/*
	void preanswer();
	void answer();
	void clearcall(const char * data);
	void senddtmf(const char *dtmf, const char * device = NULL);
	void gentones(const char *dtmf, const char * device = NULL);
	*/
	uint32_t settimer(uint32_t m_second);
	void cleartimer();
	void endconsultservice(const char * userdata = nullptr);
	void stopservice();
	////////////end lua action////////////////

	void HandleMessage(std::shared_ptr<CMessage> &tranInfo);

	void ClearEventToStateMap();
	void SetHangupFunction(const char *function_name);
	void SetStopServiceFunction(const char *function_name);
	void AddSubscribeEvent(const char * event_name, const char *function_name);
	bool IsExistSubscribeEvent();   //检查是否订阅新事件
	bool IsClose();

	void SetCaller(const char * caller);
	void SetCalled(const char * called);
	void SetOrigCaller(const char * origcaller);
	void SetOrigCalled(const char * origcalled);
	void SetScriptFile(const char * file);
	const char * GetCaller();
	const char * GetCalled();
	const char * GetScriptFile();
	const char * GetCurrentState();
	int GetStationNo();

	void trace(const char * msg);
	void debug(const char * msg);
	void info(const char * msg);
	void warn(const char * msg);
	void error(const char * msg);

public:
	// for acd
    void acd_request_route(const std::string & data);
    void acd_request_route(const Json::Value & data);
    void acd_request_route_ex(const std::string & data);
    void acd_reroute(const std::string & data);
    void acd_route_used(const std::string & data);
    void acd_route_end(const std::string & data);

protected:
	bool LoadStartScripts();
	bool ExecLuaInitFun();
	void HandleUserMsg(std::shared_ptr<CMessage> &tranInfo);
	void HandleCallCleared(std::shared_ptr<CMessage> &tranInfo);
	void DoLuaAction(const char * actionname, const char *p_json, const char *evname);
	const std::string GetStateByEvent(const std::string & eventname);


	void reset();
	std::thread	     m_thread;
	void Start();
	void Stop();
	void run();

	//action
	//end
protected:
	Json::Value       m_connection;
	std::string       m_Caller;
	std::string       m_Called;
	std::string       m_OrigCaller;
	std::string       m_OrigCalled;
	std::string       m_ScriptFile;
	std::string       m_CurrentState;
	uint32_t          m_TimerOutId;
	const std::string m_initial;
protected:
	//lua_State *       m_luastate;

	const uint32_t    m_sessionId;
	int               m_fromStation = -1;
	log4cplus::Logger log;
	TSiaManager      *m_owner;
	friend class TSiaManager;

	std::map<string, string> m_event_to_next_state_map;  //脚本事件跳转表,脚本的下一个状态表
	string              m_hangupRegister;                //挂机回调函数
	string              m_callclearedRegister;           //停止服务回调函数
	helper::CEventBuffer<std::shared_ptr<CMessage>>  m_EventQueue;   //业务线程在此获取消息，并执行
private:
    DataBase::MySql mysql;
private:
    TSessionLua(const TSessionLua &) = delete;
    TSessionLua & operator=(const TSessionLua &) = delete;
};

typedef std::shared_ptr<TSessionLua> TSessionLuaPtr;

void verbose(char * type, char *fmt);
void siaSleep(int millsecond);

#endif
