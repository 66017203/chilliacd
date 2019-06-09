#pragma once
#include <log4cplus/logger.h>
#include <thread>
#include <atomic>
#include "json/value.h"
#include "ACDReadConfig.h"

namespace DataBase{

class MySql
{
public:
	explicit MySql();
	explicit MySql(std::string & id);
	virtual ~MySql(void);
    void set_id(const std::string & id);
	bool LoadConfig(const std::string & configContext);

	Json::Value executeQuery(const std::string & sql);

private:
//	helper::CEventBuffer<chilli::model::SQLEventType_t> m_SqlBuffer;
	std::string m_Host;
	uint32_t m_Port = 3306;
	std::string m_UserID;
	std::string m_Password;
	std::string m_DataBase;
	uint32_t m_connect_timeout = 5;
    std::string m_id;
    log4cplus::Logger log;
//	void executeSql();
};
}

