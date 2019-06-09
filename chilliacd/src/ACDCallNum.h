/*******************************************************************************
 * Copyright (C) 2019 ling-ban Ltd. All rights reserved.
 *
 * file  : ACDCallNum.h
 * author: jzwu
 * date  : 2019-05-10
 * remark:
 *
 ******************************************************************************/

#ifndef MY_ACDCALLNUM_H_
#define MY_ACDCALLNUM_H_

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <log4cplus/logger.h>
#include "MySql.h"
#include "json/value.h"
#include "../include/hiredis/hiredis.h"
//#include "hiredis/hiredis.h"

/* 外显号码 */
struct CallNum {
    long long    id;                  // identifier
    std::string  callnum;             // 外显号码
    int          gateway_id;          // 网关ID
    unsigned int ctime;               // 创建时间，10位时间戳
    unsigned int utime;               // 更新时间，10位时间戳
    std::string  desc;                // 备注
    int          callee_prefix_del;   // 被叫前面吃码数
    std::string  callee_prefix_add;   // 被叫前面补码
    int          callee_suffix_del;   // 被叫后面吃码数
    std::string  callee_suffix_add;   // 被叫后面补码
};

/* 外显号码池 */
struct CallNumPool {
    int          id;                  // identifier
    std::string  caller;              // 主叫号码，后台选择用
    std::string  desc;                // 备注
    unsigned int ctime;               // 创建时间，10位时间戳
    unsigned int utime;               // 更新时间，10位时间戳
    int ruleid;                       // 号码轮询规则id

};

/* 外显号码池与号码关联表 */
struct CallNumPoolCallNum {
    int          id;                  // identifier
    int          callnum_id;          // 外显号码id
    int          callnum_pool_id;     // 外显号码池id
    unsigned int ctime;               // 创建时间，10位时间戳
    unsigned int utime;               // 更新时间，10位时间戳
};

/* 网关配置表，配置外部SIP网关 */
struct SipGateway {
    int         id;                   // identifier
    std::string ip;                   // 网关IP地址
    int         port;                 // 网关端口
    int         maxsessions;          // 最大并发数量
    int         currentsessions;      // 当前并发数，由其他模块更新
    int         isregister;           // 是否注册
    std::string transport;            // 传输协议，udp or tcp
    std::string username;             // 注册用户名
    std::string password;             // 注册密码
    int         expire_seconds;       // 注册超时时间，单位秒 DEFAULT 60
    std::string codec;                // 支持的音频编码 PCMA,PCMU
    std::string name;                 // 网关名称
    std::string memo;                 // 描述
    std::string defaultcaller;        // 默认显示的主叫号码
    int         sipgwgroupid;         // 网关分组id
};

/* 网关组 */
struct SipGWGroup {
	int         id;                   // 记录ID，自增
	std::string algorithm;            // 选择网关策略：0：轮询，1：随机
	std::string name;                 // 网关名称
	std::string memo;                 // 描述
};

// UNUSED
struct CallNumSipGateway {
    long long    id;                  // identifier
    int          callee_prefix_del;   // 被叫前面吃码数
    std::string  callee_prefix_add;   // 被叫前面补码
    int          callee_suffix_del;   // 被叫后面吃码数
    std::string  callee_suffix_add;   // 被叫后面补码
	SipGateway gateway;
};

/*******************************************************************************
 * Redis 操作
 ******************************************************************************/
class LBRedis {
public:
	LBRedis(std::string & path);
	~LBRedis();

	redisReply * do_command(std::string & str_cmd);

private:
	void connect();

private:
    log4cplus::Logger log;
	std::string       config;
	redisContext *    rc;
};

class MyMutex {
public:
	MyMutex();
	~MyMutex();
private:
	std::mutex mtx;
};

/*******************************************************************************
 * 读取Redis中的配置
 ******************************************************************************/
class GatewayManager {
public:
    static GatewayManager & get_instance();

	///! 配置文件路径a
	void set_sql_config(const std::string & path);
	///! 根据主叫查询所有网关a
    void get_route_by_caller(const std::string & caller);
    ///! 被叫吃补码a
    void get_modified_callee_by_callee(std::string & callee);
    ///! 从redis中取数据a
	void get_data_from_redis();

	///! 选择路径a
	void select_route(Json::Value & out, int policy = 0);
	///! 更新当前并发量a
	void update_current_sessions_by_route_id(int id, int t);	
	void get_current_session_numbers(int & count);
	void get_result(Json::Value & jv, const std::string & caller);

private:
	void sort();

private:
	GatewayManager();
    GatewayManager(const GatewayManager &) = delete;
    GatewayManager operator=(const GatewayManager &) = delete;

private:
    log4cplus::Logger log;
	std::string db_config_path;

	Json::Value json_callnum;                // redis 中的外显号码记录a
	Json::Value json_callnum_pool;           // redis 中的外显号码池记录a
	Json::Value json_callnum_pool_callnum;   // redis 号码池与号码关系a
	Json::Value json_gwgroup;                // 网关组a
	Json::Value json_gateway;                // redis 中的网关配置信息a
    Json::Value json;                        // 匹配后的结果a
	Json::Value json_available_routes;       // 可用网关结果a
};

/*******************************************************************************
 * 将MySQL中的配置信息读入Redis中a
 * 加载方式:a
 *   1. 启动时加载(必须)a
 *   2. 收到加载请求时加载(TODO)a
 *   3. 定时加载(TODO)a
 ******************************************************************************/
class ReadConfig2Redis {
public:
	static ReadConfig2Redis & get_instance();
	void set_sql_config(const std::string & path);
    void load_data_from_mysql();
    void save_data_2_redis();

private:
	ReadConfig2Redis();
	ReadConfig2Redis(const ReadConfig2Redis &) = delete;
	ReadConfig2Redis operator=(const ReadConfig2Redis &) = delete;

private:
	redisContext * get_hiredis_context();

private:
    log4cplus::Logger log;
	std::string db_config_path;
	DataBase::MySql mysql;
	//redisContext * rc;

	Json::Value json_callnum;
	Json::Value json_callnum_pool;
	Json::Value json_callnum_pool_callnum;
	Json::Value json_gateway;
	Json::Value json_gwgroup;
};

#endif  // MY_ACDCALLNUM_H

