/*******************************************************************************
 * Copyright (C) 2019 ling-ban Ltd. All rights reserved.
 * 
 * file  : ACDCallNum.cpp
 * author: jzwu
 * date  : 2019-05-10
 * remark: 
 * 
 ******************************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <list>
#include <algorithm>
#include <json/writer.h>
#include <log4cplus/loggingmacros.h>
#include "ACDCallNum.h"
#include "MySql.h"
#include "cfgFile.h"
#ifndef WIN32
#include <iconv.h>
#endif
using namespace std;

LBRedis::LBRedis(std::string & path)
	: config(path)
{
	log = log4cplus::Logger::getInstance("LBRedis");
	LOG4CPLUS_DEBUG(log, "LBRedis ctor");

	connect();
}

LBRedis::~LBRedis()
{
	if (rc)
	{
		redisCommand(rc, "quit");
		redisFree(rc);
		rc = 0;
	}
}

redisReply * LBRedis::do_command(std::string & str_cmd)
{
	return (redisReply *)redisCommand(rc, str_cmd.c_str());
}

void LBRedis::connect()
{
	std::string host, auth;
	unsigned int port(6379); //db(1);

	std::size_t pos;
	std::string str_extension;
	if (std::string::npos != (pos = config.find_last_of('.')))
	{
		str_extension = config.substr(pos + 1);
	}

	if (0 == str_extension.compare("xml")) {
		ReadXMLConfig rxc;
		rxc.load_file(config.c_str());
		rxc.get_redis_config(host, port, auth);
	}
	else if (0 == str_extension.compare("cfg"))
	{
		char buffer[256] = { 0 };
		host = cfg_GetPrivateProfileStringEx("REDIS", "host", "", buffer, sizeof(buffer), config.c_str());
		port = cfg_GetPrivateProfileIntEx("REDIS", "port", 6379, config.c_str());
		auth = cfg_GetPrivateProfileStringEx("REDIS", "auth", "", buffer, sizeof(buffer), config.c_str());
	}
	else if (0 == str_extension.compare("json"))
	{
		ReadJSONConfig rjc;
		rjc.load_file(config.c_str());
		rjc.get_redis_config(host, port, auth);
	}
	LOG4CPLUS_DEBUG(log, " [" << host << ":" << port << "] [" << auth << "]");

	rc = redisConnect(host.c_str(), port);
	if (rc->err)
	{
		LOG4CPLUS_DEBUG(log, "LBRedis [redisConnect] " << rc->errstr);
		redisFree(rc);
		rc = 0;
	}
	else
	{
		if (auth.empty())
			return ;

		auth.insert(0, "auth ");
		if (redisCommand(rc, auth.c_str()))
		{
			LOG4CPLUS_DEBUG(log, " LBRedis connects to redis OK");
		}
		else
		{
			redisFree(rc);
			rc = 0;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////// 

MyMutex::MyMutex()
{
	mtx.lock();
}

MyMutex::~MyMutex()
{
	mtx.unlock();
}

//////////////////////////////////////////////////////////////////////////////// 

GatewayManager::GatewayManager()
{
	log = log4cplus::Logger::getInstance("GatewayManager");
	LOG4CPLUS_DEBUG(log, "GatewayManager ctor");
}

GatewayManager & GatewayManager::get_instance()
{
    static GatewayManager instance;
    return instance;
}

/*******************************************************************************
 * 数据库配置文件路径，含文件名
 ******************************************************************************/
void GatewayManager::set_sql_config(const std::string & path)
{
	db_config_path = path;
}

/*******************************************************************************
 * 根据主叫号码查询可用网关，放入json数组中
 ******************************************************************************/
void GatewayManager::get_route_by_caller(const std::string & caller)
{
    LOG4CPLUS_DEBUG(log, "::" << __FUNCTION__ << " BEGIN");
	if (!json_callnum.isArray() 
	        || !json_callnum_pool.isArray() 
			|| !json_callnum_pool_callnum.isArray() 
			|| !json_gateway.isArray() )
	{
		LOG4CPLUS_ERROR(log, " invalid json value, not an array");
		return ;
	}

	if (caller.empty())
	{
		LOG4CPLUS_ERROR(log, " invalid caller, caller is empty");
		return ;
	}
	if (!json.empty())
	    json.clear();
	
	// 遍历数组，查找匹配的网关
	Json::ArrayIndex tno_callnum_pool = json_callnum_pool.size();
	Json::ArrayIndex tno_callnum_pool_callnum = json_callnum_pool_callnum.size();
	Json::ArrayIndex tno_callnum = json_callnum.size();
	Json::ArrayIndex tno_gateway = json_gateway.size();
	Json::ArrayIndex idx = 0U;
	Json::Value valid_callnum;
	std::vector<int> vec_callnum_ids;
	// 获取可用外显号码ID (O2)
	for (unsigned int i = 0U; i < tno_callnum_pool; ++i)
	{
		// 找到了一个符合条件的号码池ID
		if (json_callnum_pool[i]["caller"].isString() && caller == json_callnum_pool[i]["caller"].asString())
		{
			// 根据号码池ID查找号码ID
			for (unsigned int j = 0U; j < tno_callnum_pool_callnum; ++j)
			{
				if (json_callnum_pool_callnum[j]["callnum_pool_id"].isInt()
						&& json_callnum_pool[i]["id"].asInt() == json_callnum_pool_callnum[j]["callnum_pool_id"].asInt())
				{
					// 得到外显号码ID [j]["callnum_id"]
					vec_callnum_ids.push_back(json_callnum_pool_callnum[j]["callnum_id"].asInt());
				}
			}
		}
	}

	// 获取可用的外显号码记录 (O2)
	for (auto it = vec_callnum_ids.begin(); it != vec_callnum_ids.end(); ++it)
	{
		LOG4CPLUS_DEBUG(log, "callnum id: " << *it);
		for (unsigned int k = 0U; k < tno_callnum; ++k)
		{
			if (json_callnum[k]["id"].isInt()
					&& json_callnum[k]["id"].asInt() == *it)
			{
				valid_callnum[idx++] = json_callnum[k];
			}
		}
	}

	// 网关组
	idx = valid_callnum.size();
	vector<int> vi;
	for (Json::ArrayIndex i = 0u;i < idx; ++i)
	{
		// TODO
	}
	
	// 初步查找相关的网关
	idx = valid_callnum.size();
	Json::ArrayIndex ret_idx = 0U;
	for (unsigned int i = 0U; i < idx; ++i)
	{
		for (unsigned int j = 0U; j < tno_gateway; ++j)
		{
			if (valid_callnum[i]["gateway_id"].asInt() == json_gateway[j]["id"].asInt())
			{
				//json[ret_idx]["param"] = Json::Value();
				json[ret_idx]["callnum_id"]        = valid_callnum[i]["id"];
				json[ret_idx]["callee_prefix_del"] = valid_callnum[i]["called_prefix_del"];
				json[ret_idx]["callee_prefix_add"] = valid_callnum[i]["called_prefix_add"];
				json[ret_idx]["callee_suffix_del"] = valid_callnum[i]["called_tail_del"];
				json[ret_idx]["callee_suffix_add"] = valid_callnum[i]["called_tail_add"];
				json[ret_idx]["route_id"]          = json_gateway[j]["id"];
				json[ret_idx]["route_ip"]          = json_gateway[j]["ip"];
				json[ret_idx]["route_port"]        = json_gateway[j]["port"];
				json[ret_idx]["maxsessions"]       = json_gateway[j]["maxsessions"];
				json[ret_idx]["currentsessions"]   = json_gateway[j]["currentsessions"];
				json[ret_idx]["isregister"]        = json_gateway[j]["isregister"];
				json[ret_idx]["transport"]         = json_gateway[j]["transport"];
				json[ret_idx]["username"]          = json_gateway[j]["username"];
				json[ret_idx]["password"]          = json_gateway[j]["password"];
				json[ret_idx]["expire_seconds"]    = json_gateway[j]["expire_seconds"];
				json[ret_idx]["codec"]             = json_gateway[j]["codec"];
				json[ret_idx]["name"]              = json_gateway[j]["name"];
				json[ret_idx]["memo"]              = json_gateway[j]["memo"];
				json[ret_idx]["defaultcaller"]     = json_gateway[j]["defaultcaller"];
				json[ret_idx]["sipgwgroupid"]      = json_gateway[j]["sipgwgroupid"];
				++ret_idx;
			}
		}
	}
	// FOR TEST
	//Json::StyledWriter w;
	//Json::ArrayIndex xxx = json.size();
	//LOG4CPLUS_DEBUG(log, "JSON: [" << xxx << "]\n"<< w.write(json));
	//for (unsigned int i = 0U; i < xxx; ++i)
	//{
		//LOG4CPLUS_DEBUG(log, "查询结果 \n" << w.write(json[i]));
		//LOG4CPLUS_DEBUG(log, "name: [" << json[i]["name"].asString() << "]");
	//}
    LOG4CPLUS_DEBUG(log, "::" << __FUNCTION__ << " END");
}

/*******************************************************************************
 * 将Redis中的数据写入对象
 ******************************************************************************/
void GatewayManager::get_data_from_redis()
{
	MyMutex mmtx;
	json_callnum.clear();
	json_callnum_pool.clear();
	json_callnum_pool_callnum.clear();
	json_gateway.clear();

	redisReply * rr = 0;
	std::string str_cmd;

	Json::Reader reader;
	LBRedis redis(db_config_path);

	str_cmd = "get key_callnum";
	rr = redis.do_command(str_cmd);
	if (rr && rr->str) {
		if (reader.parse(rr->str, json_callnum)) {
			; // nops
		}
	}

	str_cmd = "get key_callnum_pool";
	rr = redis.do_command(str_cmd);
	if (rr && rr->str) {
		if (reader.parse(rr->str, json_callnum_pool)) {
			; // nops
		}
	}

	str_cmd = "get key_callnum_pool_callnum";
	rr = redis.do_command(str_cmd);
	if (rr && rr->str) {
		if (reader.parse(rr->str, json_callnum_pool_callnum)) {
			; // nops
		}
	}

	str_cmd = "get key_sip_gateway";
	rr = redis.do_command(str_cmd);
	if (rr && rr->str) {
		if (reader.parse(rr->str, json_gateway)) {
			; // nops
		}
	}

	str_cmd = "get key_sip_gwgroup";
	rr = redis.do_command(str_cmd);
	if (rr && rr->str) {
		if (reader.parse(rr->str, json_gwgroup)) {
			; // nops
		}
	}
}

void GatewayManager::sort()
{

}

/**
 * id 网关ID
 * t  增加或减少当前并发数量，1:增加，其他:减小
 */
void GatewayManager::update_current_sessions_by_route_id(int id, int t)
{
	int increment = (1 == t ? 1 : -1);
	Json::Reader reader;
	Json::Value  json_tmp;
	redisReply * rr = 0;
	LBRedis redis(db_config_path);

	std::string cmd("get key_sip_gateway");
	rr = redis.do_command(cmd);
	if (rr && rr->str)
	{
		if (reader.parse(rr->str, json_tmp))
		{
			Json::ArrayIndex idx = json_tmp.size();
			for (Json::ArrayIndex i = 0U; i < idx; ++i)
			{
				if (json_tmp[i]["id"].isInt() && id == json_tmp[i]["id"].asInt())
				{
					json_tmp[i]["currentsessions"] = json_tmp[i]["currentsessions"].asInt() + increment;
					LOG4CPLUS_DEBUG(log, " new currentsessions [" << json_tmp[i]["currentsessions"].asInt() << "]");
					break;
				}
			}
		}
		else
			LOG4CPLUS_DEBUG(log, "[" << __FUNCTION__ << "] line [" << __LINE__ << "] json parse ERROR ");
		
		Json::FastWriter writer;
		std::string str_cmd = "set key_sip_gateway ";
		str_cmd += writer.write(json_tmp);
		{
			MyMutex mtx;
			redisReply * rr = redis.do_command(str_cmd);
			if (rr && rr->str)
				LOG4CPLUS_DEBUG(log, " " << __FUNCTION__ << " OK");
			else
				LOG4CPLUS_DEBUG(log, " " << __FUNCTION__ << " FAILED id [" << id << "] type [" << t << "]");
		}
	}
}

// TODO 取每个网关的当前并发量
// 将参数 count 修改为 map
void GatewayManager::get_current_session_numbers(int & count)
{
    if (json_gateway.isArray())
    {
        int id, ss;
        std::map<int, int> map_sessions;
        auto cnt = json_gateway.size();
        for (Json::ArrayIndex i = 0; i < cnt; ++i)
        {
            id = json_gateway[i]["id"].asInt();
            ss = json_gateway[i]["currentsessions"].asInt();
            map_sessions.insert(std::pair<int, int>(id, ss));
        }
    }
    else
    {

    }
	count = 0;
}

/*******************************************************************************
 * 根据吃码、补码规则修改被叫号码
 ******************************************************************************/
void GatewayManager::get_modified_callee_by_callee(std::string & callee)
{
	LOG4CPLUS_DEBUG(log, "");
	Json::ArrayIndex idx = json.size();
	for (Json::ArrayIndex i = 0U; i < idx; ++i)
	{
		if (json[i]["currentsessions"].isInt() && json[i]["maxsessions"].isInt()
				&& json[i]["currentsessions"].asInt() < json[i]["maxsessions"].asInt())
		{
			if (json[i]["callee_prefix_del"].isInt())
				callee.erase(0, json[i]["callee_prefix_del"].asInt());

			if (json[i]["callee_prefix_add"].isString())
				callee.insert(0, json[i]["callee_prefix_add"].asString());

			if (json[i]["callee_suffix_del"].isInt())
				callee.erase(callee.length() - json[i]["callee_suffix_del"].asInt(), json[i]["callee_suffix_del"].asInt());

			if (json[i]["callee_suffix_add"].isString())
				callee.append(json[i]["callee_suffix_add"].asString());
			
			LOG4CPLUS_DEBUG(log, "AFTER EAT/ADD, CALLEE [" << callee << "]");
			// break;
		}
	}
}

void GatewayManager::get_result(Json::Value & ret, const std::string & caller)
{
    ret = json;
}

////////////////////////////////////////////////////////////////////////////////

ReadConfig2Redis::ReadConfig2Redis()
{
	log = log4cplus::Logger::getInstance("ReadConfig2Redis");
	LOG4CPLUS_DEBUG(log, "ReadConfig2Redis ctor");
}

ReadConfig2Redis & ReadConfig2Redis::get_instance()
{
	static ReadConfig2Redis instance;
	return instance;
}

/*******************************************************************************
 * 数据库配置文件路径，含文件名
 ******************************************************************************/
void ReadConfig2Redis::set_sql_config(const std::string & path)
{
	db_config_path = path;
}

/*******************************************************************************
 * 将配置全部读入对象内存
 ******************************************************************************/
void ReadConfig2Redis::load_data_from_mysql()
{
	std::string sql_callnum = "SELECT\n"
		"	T.id,\n"
		"	T.callnum,\n"
		"	T.gateway_id,\n"
		"	T.ctime,\n"
		"	T.utime,\n"
		"	T.descr,\n"
		"	T.called_prefix_del,\n"
		"	T.called_prefix_add,\n"
		"	T.called_tail_del,\n"
		"	T.called_tail_add\n"
		"FROM\n"
		"	callnum T";
	std::string sql_callnum_pool = "SELECT\n"
		"	T.id,\n"
		"	T.caller,\n"
		"	T.descr,\n"
		"	T.ctime,\n"
		"	T.utime,\n"
		"	T.ruleid\n"
		"FROM\n"
		"	callnum_pool T";
	std::string sql_callnum_pool_callnum = "SELECT\n"
		"	T.id,\n"
		"	T.callnum_id,\n"
		"	T.callnum_pool_id,\n"
		"	T.ctime,\n"
		"	T.utime\n"
		"FROM\n"
		"	callnum_pool_callnum T";
	std::string sql_gateway = "SELECT\n"
		"	T.`id`,\n"
		"	T.`ip`,\n"
		"	T.`port`,\n"
		"	T.`maxsessions`,\n"
		"	T.`currentsessions`,\n"
		"	T.`isregister`,\n"
		"	T.`transport`,\n"
		"	T.`username`,\n"
		"	T.`password`,\n"
		"	T.`expire_seconds`,\n"
		"	T.`codec`,\n"
		"	T.`name`,\n"
		"	T.`memo`,\n"
		"	T.`defaultcaller`,\n"
		"	T.`sipgwgroupid` \n"
		"FROM\n"
		"	sip_gateway T";
	std::string sql_gwgroup = "SELECT\n"
		"	T.`id`,\n"
		"	T.`algorithm`,\n"
		"	T.`name`,\n"
		"	T.`memo`\n"
		"FROM\n"
		"	sip_gwgroup T";

	// RAII
	mysql.LoadConfig(db_config_path);
	json_callnum = mysql.executeQuery(sql_callnum);
	json_callnum_pool = mysql.executeQuery(sql_callnum_pool);
	json_callnum_pool_callnum = mysql.executeQuery(sql_callnum_pool_callnum);
	json_gateway = mysql.executeQuery(sql_gateway);
	json_gwgroup = mysql.executeQuery(sql_gwgroup);
}

redisContext * ReadConfig2Redis::get_hiredis_context()
{
	if (db_config_path.empty())
	{
		LOG4CPLUS_ERROR(log, "config path is not found");
		return NULL;
	}

	redisContext * rc = 0;
	std::size_t pos;
	unsigned int rds_port(6379);
	std::string rds_host, rds_auth;
	std::string str_extension;
	if (std::string::npos != (pos = db_config_path.find_last_of('.')))
	{
		str_extension = db_config_path.substr(pos + 1);
	}

	if (0 == str_extension.compare("xml")) {
		ReadXMLConfig rxc;
		rxc.load_file(db_config_path.c_str());
		rxc.get_redis_config(rds_host, rds_port, rds_auth);
	}
	else if (0 == str_extension.compare("cfg"))
	{
		char buffer[256] = { 0 };
		rds_host = cfg_GetPrivateProfileStringEx("REDIS", "host", "", buffer, sizeof(buffer), db_config_path.c_str());
		rds_port = cfg_GetPrivateProfileIntEx("REDIS", "port", 6379, db_config_path.c_str());
		rds_auth = cfg_GetPrivateProfileStringEx("REDIS", "auth", "", buffer, sizeof(buffer), db_config_path.c_str());
	}
	else if (0 == str_extension.compare("json"))
	{
		ReadJSONConfig rjc;
		rjc.load_file(db_config_path.c_str());
		rjc.get_redis_config(rds_host, rds_port, rds_auth);
	}
	LOG4CPLUS_DEBUG(log, " [" << rds_host << ":" << rds_port << "] [" << rds_auth << "]");

	rc = redisConnect(rds_host.c_str(), rds_port);
	if (rc->err)
	{
		LOG4CPLUS_DEBUG(log, "CONNECT 2 REDIS ERROR [" << rc->errstr << "]");
		redisFree(rc);
		rc = 0;
	}
	else
	{
		if (!rds_auth.empty())
		{
			rds_auth.insert(0, "auth ");
			// XXX no-blocking always return NULL
			if (NULL == redisCommand(rc, rds_auth.c_str()))
			{
				redisFree(rc);
				rc = 0;
			}
			//redisCommand(rc, rds_auth.c_str());
		}
	}

	return rc;
}

// FIXME
void ReadConfig2Redis::save_data_2_redis()
{
	redisReply * rr = 0;
	redisContext * rc = get_hiredis_context();
	if (NULL == rc)
	{
		return ;
	}

	Json::FastWriter writer;
	std::string str_cmd;

	str_cmd = "set key_callnum ";
	str_cmd += writer.write(json_callnum);
	rr = (redisReply*)redisCommand(rc, str_cmd.c_str());

	str_cmd.clear();
	str_cmd = "set key_callnum_pool ";
	str_cmd += writer.write(json_callnum_pool);
	rr = (redisReply*)redisCommand(rc, str_cmd.c_str());

	str_cmd.clear();
	str_cmd = "set key_callnum_pool_callnum ";
	str_cmd += writer.write(json_callnum_pool_callnum);
	rr = (redisReply*)redisCommand(rc, str_cmd.c_str());

	str_cmd.clear();
	str_cmd = "set key_sip_gateway ";
	str_cmd += writer.write(json_gateway);
	rr = (redisReply*)redisCommand(rc, str_cmd.c_str());

	str_cmd.clear();
	str_cmd = "set key_sip_gwgroup";
	str_cmd += writer.write(json_gwgroup);
	rr = (redisReply*)redisCommand(rc, str_cmd.c_str());
	/*
	LOG4CPLUS_DEBUG(log, "BEFORE: " << str_cmd);
	const char * encTo = "UTF-8//IGNORE", *encFrom = "UNICODE";
	iconv_t cd = iconv_open(encTo, encFrom);
	if ((iconv_t)-1 == cd)
	{
		LOG4CPLUS_ERROR(log, "OPEN iconv_open FAILED");
	}
	char inbuf[1024 << 2] = {0};
	char outbuf[1024 << 2] = {0};
	size_t srclen = sizeof(inbuf), outlen = sizeof(outbuf); 
	strncpy(inbuf, str_cmd.c_str(), str_cmd.length());
	char * strstart = inbuf, *tmpoutbuf = outbuf;
	size_t ret = iconv(cd, &strstart, &srclen, &tmpoutbuf, &outlen);
	if ((size_t)-1 == ret)
	{
		LOG4CPLUS_ERROR(log, "exec iconv FAILED");
	}
	LOG4CPLUS_DEBUG(log, "AFTER: " << outbuf);
	iconv_close(cd);
	*/

	redisCommand(rc, "quit");
	redisFree(rc);
}

