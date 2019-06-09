#include "MySql.h"
#include <log4cplus/loggingmacros.h>
#include "../tinyxml2/tinyxml2.h"
#include <json/config.h>
#include <json/json.h>
#if (defined(_WIN32) || defined(_WIN64))
#include <WinSock2.h>
#include <mysql.h>
#include <errmsg.h>
#else
#include <mysql/mysql.h>
#include <mysql/errmsg.h>
#endif
#include "cfgFile.h"
#include "ACDReadConfig.h"


namespace DataBase {

	Json::Value fieldtoJson(const MYSQL_FIELD & field,
            const char * row,
            const log4cplus::Logger & log,
            const std::string & logId )
	{
		Json::Value result;
		try
		{
			if (field.type == MYSQL_TYPE_DECIMAL) {
				if (row)
					result = std::stod(row);
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_TINY) {
				if (row)
					result = std::stoi(row);
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_SHORT) {
				if (row)
					result = std::stoi(row);
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_LONG) {
				if (row)
					//result = std::stoll(row);
					result = std::stoi(row);
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_FLOAT) {
				if (row)
					result = std::stof(row);
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_DOUBLE) {
				if (row)
					result = std::stod(row);
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_NULL) {
				if (row)
					result = Json::nullValue;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_TIMESTAMP) {
				if (row)
					//result = std::stoull(row);
					result = std::stoi(row);
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_LONGLONG) {
				if (row)
					//result = std::stoll(row);
					result = std::stoi(row);
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_INT24) {
				if (row)
					result = std::stoi(row);
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_DATE) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_TIME) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_DATETIME) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_YEAR) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_NEWDATE) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_VARCHAR) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_BIT) {
				if (row)
					result = *row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_NEWDECIMAL) {
				if (row)
					result = std::stod(row);
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_ENUM) {
				if (row)
					result = std::stoi(row);
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_SET) {
				if (row)
					result = std::stoi(row);
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_TINY_BLOB) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_MEDIUM_BLOB) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_LONG_BLOB) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_BLOB) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_VAR_STRING) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_STRING) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else if (field.type == MYSQL_TYPE_GEOMETRY) {
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
			else {
				LOG4CPLUS_WARN(log, logId << " " << field.table << "." << field.name << " unkown type " << field.type);
				if (row)
					result = row;
				else
					result = Json::nullValue;
			}
		}
		catch (std::exception* e)
		{
			LOG4CPLUS_ERROR(log, logId << " " << field.table << e->what());
		}
		return result;
	}

	MySql::MySql()
        : m_id("load_00000000")
	{
		log = log4cplus::Logger::getInstance("MySql");
		LOG4CPLUS_DEBUG(log, m_id << " Constuction a module.");
	}

	MySql::MySql(std::string & id)
        : m_id(id)
	{
		log = log4cplus::Logger::getInstance("MySql");
		LOG4CPLUS_DEBUG(log, m_id << " Constuction a module.");
	}

	MySql::~MySql(void)
	{
		LOG4CPLUS_DEBUG(log, m_id << " Destruction a module.");
	}

    void MySql::set_id(const std::string & id)
    {
        m_id = id;
    }

	bool MySql::LoadConfig(const std::string & configContext)
	{
        std::size_t pos;
        std::string str_extension;
        if (std::string::npos != (pos = configContext.find_last_of('.')))
        {
            str_extension = configContext.substr(pos + 1);
        }

        //if (0 == configContext.compare(pos + 1, 3, "xml")
        //if (0 == configContext.compare(pos + 1, 3, "cfg")
        //if (0 == configContext.compare(pos + 1, 4, "json")

        if (0 == str_extension.compare("xml")) {
            /*
            using namespace tinyxml2;
            tinyxml2::XMLDocument config;
            if (XMLError::XML_SUCCESS != config.Parse(configContext.c_str()) ) {
                LOG4CPLUS_ERROR(log, m_id << " load config error:" << config.ErrorName() << ":" << config.GetErrorStr1());
                return false;
            }
            XMLElement * mysql = config.FirstChildElement("mysql");
            const char * host = mysql->Attribute("host");
            const char * port = mysql->Attribute("port");
            const char * userId = mysql->Attribute("user");
            const char * password = mysql->Attribute("passwd");
            const char * db = mysql->Attribute("db");
            m_Host = host ? host : std::string();
            if (port) {
                try{
                m_Port = std::stoul(port);
                }
                catch (...)
                {
                }
            }

            m_UserID = userId ? userId : std::string();
            m_Password = password ? password : std::string();
            m_DataBase = db ? db : std::string();
            */
            ReadXMLConfig rxc;
            rxc.load_file(configContext.c_str());
            rxc.get_mysql_config(m_Host, m_Port, m_DataBase, m_UserID, m_Password);
        }
        else if (0 == str_extension.compare("cfg"))
        {
            char buffer[256] = { 0 };
            m_Host = cfg_GetPrivateProfileStringEx("MYSQL", "host", "", buffer, sizeof(buffer), configContext.c_str());
	        m_Port = cfg_GetPrivateProfileIntEx("MYSQL", "port", 3306, configContext.c_str());
            m_DataBase = cfg_GetPrivateProfileStringEx("MYSQL", "db", "cc", buffer, sizeof(buffer), configContext.c_str());
            m_UserID = cfg_GetPrivateProfileStringEx("MYSQL", "user", "", buffer, sizeof(buffer), configContext.c_str());
            m_Password = cfg_GetPrivateProfileStringEx("MYSQL", "passwd", "", buffer, sizeof(buffer), configContext.c_str());
        }
        else if (0 == str_extension.compare("json"))
        {
            ReadJSONConfig rjc;
            rjc.load_file(configContext.c_str());
            rjc.get_mysql_config(m_Host, m_Port, m_DataBase, m_UserID, m_Password);
        }
        LOG4CPLUS_ERROR(log, m_id << " [" << m_Host << ":" << m_Port << "] " << m_DataBase << "-" << m_UserID << "-" << m_Password);

		return true;
	}

    /*
	void MySql::run()
	{
		return executeSql();
	}
    */

    /*
	void MySql::execute(helper::CEventBuffer<model::EventType_t>* eventQueue)
	{
		fsm::threadIdle();
		fsm::threadCleanup();
		log4cplus::threadCleanup();
	}
    */

	Json::Value MySql::executeQuery(const std::string & sql)
	{
		MYSQL mysql;
		MYSQL_RES * res = nullptr;       //查询结果集，结构类型
		MYSQL_FIELD * fd = nullptr;     //包含字段信息的结构
		MYSQL_ROW row = nullptr;       //存放一行查询结果的字符串数组
		mysql_init(&mysql);
		mysql.options.connect_timeout = this->m_connect_timeout;

		Json::Value result;

		if (mysql_real_connect(&mysql, m_Host.c_str(), m_UserID.c_str(), m_Password.c_str(), m_DataBase.c_str(), m_Port, nullptr, 0) == nullptr)
		{
			LOG4CPLUS_ERROR(log, m_id << " ERROR: " << mysql_error(&mysql) << " (MySQL error code: " << mysql_errno(&mysql) << ", SQLState: " << mysql_sqlstate(&mysql) << ")");
			return result;
		}

		mysql_set_character_set(&mysql, "utf8");

		if (mysql_query(&mysql, sql.c_str())) {
			LOG4CPLUS_ERROR(log, m_id << " ERROR: " << mysql_error(&mysql) << " (MySQL error code: " << mysql_errno(&mysql) << ", SQLState: " << mysql_sqlstate(&mysql) << ")");
			mysql_close(&mysql);
			return result;
		}

		if (nullptr == (res = mysql_store_result(&mysql))) {
			LOG4CPLUS_ERROR(log, m_id << " ERROR: " << mysql_error(&mysql) << " (MySQL error code: " << mysql_errno(&mysql) << ", SQLState: " << mysql_sqlstate(&mysql) << ")");
			mysql_close(&mysql);
			return result;
		}

		uint32_t num_fields = mysql_num_fields(res);
		MYSQL_FIELD *fields = nullptr;
		fields = mysql_fetch_fields(res);

		while ((row = mysql_fetch_row(res)))
		{
			//unsigned long *lengths;
			//lengths = mysql_fetch_lengths(res);
			Json::Value jrow;
			for (uint32_t i = 0; i < num_fields; i++)
			{
				jrow[fields[i].name] = fieldtoJson(fields[i], row[i], this->log, m_id);
			}
			result.append(jrow);
		}


		mysql_free_result(res);
		mysql_close(&mysql);
		mysql_thread_end();

		return result;
	}

    /*
	void MySql::fireSend(const fsm::FireDataType & fireData, const void * param)
	{
		LOG4CPLUS_TRACE(log, m_id << " fireSend:" << fireData.event);

		const std::string & eventName = fireData.event;
		const std::string & typeName = fireData.type;
		const std::string & dest = fireData.dest;
		const std::string & from = fireData.from;


		if (typeName != "cmd") {
			return;
		}

		std::string sql;

		if (fireData.param["sql"].isString())
			sql = fireData.param["sql"].asString();

		chilli::model::SQLEventType_t event(sql, from);
		LOG4CPLUS_TRACE(log, m_id << " sql:" << event.m_sql);
		m_SqlBuffer.Put(event);

	}
    */

    /*
	void MySql::executeSql()
	{
		LOG4CPLUS_INFO(log, m_id << " Starting...");
		while (m_bRunning)
		{
			MYSQL mysql;
			MYSQL_RES *res = nullptr;      //查询结果集，结构类型
			MYSQL_FIELD *fd = nullptr;     //包含字段信息的结构
			MYSQL_ROW row = nullptr;       //存放一行查询结果的字符串数组
			mysql_init(&mysql);

			mysql.options.connect_timeout = this->m_connect_timeout;
			Json::Value result;

			if (mysql_real_connect(&mysql, m_Host.c_str(), m_UserID.c_str(), m_Password.c_str(), m_DataBase.c_str(), m_Port, nullptr, 0) == nullptr)
			{
				LOG4CPLUS_ERROR(log, m_id << " ERROR: " << mysql_error(&mysql) << " (MySQL error code: " << mysql_errno(&mysql) << ", SQLState: " << mysql_sqlstate(&mysql) << ")");
				std::this_thread::sleep_for(std::chrono::seconds(5));
				continue;
			}
			mysql_set_character_set(&mysql, "utf8");

			LOG4CPLUS_INFO(log, m_id << " New client character set:"<< mysql_character_set_name(&mysql));

			LOG4CPLUS_DEBUG(log, m_id << " " << mysql_get_server_info(&mysql));

			model::SQLEventType_t Event;
			while (m_SqlBuffer.Get(Event) && !Event.m_sql.empty())
			{
				LOG4CPLUS_DEBUG(log, m_id << " " << Event.m_Id << " executeSql:" << Event.m_sql);
				if (Event.m_sql.empty())
					break;

				Json::Value result;
				result["cause"] = 0;
				result["reason"] = "success";



				if (mysql_query(&mysql, Event.m_sql.c_str())) {
					LOG4CPLUS_ERROR(log, m_id << " ERROR: " << mysql_error(&mysql) << " (MySQL error code: " << mysql_errno(&mysql) << ", SQLState: " << mysql_sqlstate(&mysql) << ")");
					if (Event.m_times++ < 5) {
						m_SqlBuffer.Put(Event);
						continue;
					}
					else {
						result["id"] = Event.m_Id;
						result["event"] = "SQL";
						result["type"] = "result";
						result["cause"] = mysql_errno(&mysql);
						result["reason"] = mysql_error(&mysql);

						chilli::model::EventType_t resultEvent(new model::_EventType(result));
						this->PushEvent(resultEvent);
						break;
					}
				}

				if ((res = mysql_store_result(&mysql)) != nullptr) {

					Json::Value data;

					uint32_t num_fields = mysql_num_fields(res);
					MYSQL_FIELD *fields = nullptr;
					fields = mysql_fetch_fields(res);

					while ((row = mysql_fetch_row(res)))
					{
						//unsigned long *lengths;
						//lengths = mysql_fetch_lengths(res);
						Json::Value jrow;
						for (uint32_t i = 0; i < num_fields; i++)
						{
							jrow[fields[i].name] = fieldtoJson(fields[i], row[i], this->log, m_id);
						}
						data.append(jrow);
					}

					result["UpdateCount"] = (uint32_t)res->row_count;
					mysql_free_result(res);

					result["data"] = data;
				}

				result["id"] = Event.m_Id;
				result["event"] = "SQL";
				result["type"] = "event";

				chilli::model::EventType_t resultEvent(new model::_EventType(result));
				auto pe = getPerformElementByGlobal(Event.m_Id);
				if(pe != nullptr){
					pe->PushEvent(resultEvent);
				}
			}

			mysql_close(&mysql);
		}


		LOG4CPLUS_INFO(log, m_id << " Stoped.");
		mysql_thread_end();
		log4cplus::threadCleanup();
	}
    */
}
