/*******************************************************************************
 * Copyright (C) 2019 ling-ban Ltd. All rights reserved.
 * 
 * file  : ACDReadConfig.h
 * author: jzwu
 * date  : 2019-03-28
 * remark: 
 * 
 ******************************************************************************/

#ifndef MY_ACDREADCONFIG_H_
#define MY_ACDREADCONFIG_H_

#include <string>
#include <fstream>
#include "../tinyxml2/tinyxml2.h"
#include "json/value.h"
#include "json/reader.h"

enum CONFIG {
	LB_SIA,
	LB_SELF,
	LB_CM,
	LB_MYSQL,
	LB_REDIS,
	LB_PROMETHEUS
};
/*******************************************************************************
 * 产品类接口
 ******************************************************************************/
class ReadConfig {
public:
    ReadConfig(){}
    ~ReadConfig(){}

    ///< 加载 xml 格式的配置文件
    virtual bool load_file(const char * xml_file) = 0;
    ///< 读取 acd 的配置
    virtual bool get_acd_config(std::string &, unsigned int &) const = 0;
    ///< 读取 cm 的配置
    virtual bool get_cm_config(std::string &, unsigned int &) const = 0;
    ///< 读取 mysql 的配置
    virtual bool get_mysql_config(std::string &, unsigned int &, std::string &, std::string &, std::string &) const = 0;
    virtual bool get_redis_config(std::string &, unsigned int &, std::string &) const = 0;
    virtual bool get_config(CONFIG, Json::Value &) const = 0;
    ///< 对象所属的类名
    virtual std::string belongs_to(void) const = 0;
};

/*******************************************************************************
 * 产品类 读取XML格式的配置文件
 ******************************************************************************/
class ReadXMLConfig: public ReadConfig {
public:
    ReadXMLConfig();

    ///< 加载 xml 格式的配置文件a
    bool load_file(const char * xml_file);

    ///< 读取 acd 的配置a
    bool get_acd_config(std::string & str_ip, unsigned int & port) const;
    ///< 读取 cm 的配置a
    bool get_cm_config(std::string & str_ip, unsigned int & port) const;
    ///< 读取 mysql 的配置a
    bool get_mysql_config(std::string & str_host, unsigned int & port, std::string & str_db, std::string & str_user, std::string & str_passwd) const;
    bool get_redis_config(std::string & host, unsigned int & port, std::string & auth) const;
    bool get_config(CONFIG who, Json::Value & json) const;
    std::string belongs_to(void) const;

private:
    ReadXMLConfig(const ReadXMLConfig &) = delete;
    ReadXMLConfig & operator=(const ReadXMLConfig &) = delete;

private:
    tinyxml2::XMLDocument xmldoc;
};

/*******************************************************************************
 * 产品类 读取JSON格式的配置文件a
 ******************************************************************************/
class ReadJSONConfig : public ReadConfig {
public:
    ReadJSONConfig();
    ReadJSONConfig(const ReadJSONConfig &) = delete;
    ReadJSONConfig & operator=(const ReadJSONConfig &) = delete;
    ~ReadJSONConfig();

    bool load_file(const char * json_file) throw(std::string);
    bool get_acd_config(std::string & str_ip, unsigned int & port) const;
    bool get_acd_config(Json::Value & json) const;
    bool get_cm_config(std::string & str_ip, unsigned int & port) const;
    bool get_cm_config(Json::Value & json) const;
    bool get_mysql_config(std::string & str_host, unsigned int & port, std::string & str_db, std::string & str_user, std::string & str_passwd) const;
    bool get_mysql_config(Json::Value & json) const;
    bool get_redis_config(std::string & host, unsigned int & port, std::string & auth) const;
    bool get_config(CONFIG who, Json::Value & json) const;
    std::string belongs_to(void) const;

private:
    std::ifstream in;
    Json::Value value;
    Json::Reader reader;
};

enum OBJECT {
    XML,
    JSON
};

// simple factory
class ConfigObject {
public:
    ~ConfigObject();
    static ConfigObject & get_instance() {
        static ConfigObject instance;
        return instance;
    }

    ReadConfig * get_config_object(OBJECT obj);

private:
    ConfigObject();
    ConfigObject(const ConfigObject &) = delete;
    ConfigObject & operator=(const ConfigObject &) = delete;
};

#endif  // MY_ACDREADCONFIG_H

