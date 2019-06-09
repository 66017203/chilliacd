/*******************************************************************************
 * Copyright (C) 2019 ling-ban Ltd. All rights reserved.
 * 
 * file  : ACDReadConfig.cpp
 * author: jzwu
 * date  : 2019-03-28
 * remark: 
 * 
 ******************************************************************************/

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "ACDReadConfig.h"
#include "base.h"
using namespace std;
using namespace tinyxml2;

/********************************ReadXMLConfig*********************************/
ReadXMLConfig::ReadXMLConfig()
{
}

bool ReadXMLConfig::load_file(const char * xml_file)
{
    IFNULLRETURNFALSE(xml_file, __LINE__, __FUNCTION__, STR(xml_file));

    if (XML_SUCCESS != xmldoc.LoadFile(xml_file))
    {
        throw std::string("XMLDocument LoadFile error");
        return false;
    }

    return true;
}

bool ReadXMLConfig::get_acd_config(std::string & str_ip, unsigned int & port) const
{
    const XMLElement * root = xmldoc.RootElement();                      // 获取根元素
    IFNULLRETURNFALSE(root, __LINE__, __FUNCTION__, STR(root));
    const XMLElement * element = root->FirstChildElement("self");        // 获取 self 元素
    IFNULLRETURNFALSE(element, __LINE__, __FUNCTION__, STR(element));

    const XMLElement * value = element->FirstChildElement("ip");         // 获取 ip 元素
    IFNULLRETURNFALSE(value, __LINE__, __FUNCTION__, STR(value));
    str_ip = value->GetText();

    //value = value->NextSiblingElement();
    value = element->FirstChildElement("port");
    IFNULLRETURNFALSE(value, __LINE__, __FUNCTION__, STR(value));
    port = std::stoi(value->GetText());

    return true;
}

bool ReadXMLConfig::get_cm_config(std::string & str_ip, unsigned int & port) const
{
    const XMLElement * root = xmldoc.RootElement();
    IFNULLRETURNFALSE(root, __LINE__, __FUNCTION__, STR(root));
    const XMLElement * element = root->FirstChildElement("cm");
    IFNULLRETURNFALSE(element, __LINE__, __FUNCTION__, STR(element));

    const XMLElement * value = element->FirstChildElement("ip");
    IFNULLRETURNFALSE(value, __LINE__, __FUNCTION__, STR(value));
    str_ip = value->GetText();

    //value = value->NextSiblingElement();
    value = element->FirstChildElement("port");
    IFNULLRETURNFALSE(value, __LINE__, __FUNCTION__, STR(value));
    port = std::stoi(value->GetText());

    return true;
}

bool ReadXMLConfig::get_mysql_config(std::string & str_host, unsigned int & port, std::string & str_db, std::string & str_user, std::string & str_passwd) const
{
    const XMLElement * root = xmldoc.RootElement();
    IFNULLRETURNFALSE(root, __LINE__, __FUNCTION__, STR(root));
    const XMLElement * element = root->FirstChildElement("mysql");
    IFNULLRETURNFALSE(element, __LINE__, __FUNCTION__, STR(element));

    const XMLElement * value = element->FirstChildElement("host");
    IFNULLRETURNFALSE(value, __LINE__, __FUNCTION__, STR(value));
    str_host = value->GetText();

    //value = value->NextSiblingElement();
    value = element->FirstChildElement("port");
    IFNULLRETURNFALSE(value, __LINE__, __FUNCTION__, STR(value));
    port = std::stoi(value->GetText());

    //value = value->NextSiblingElement();
    value = element->FirstChildElement("db");
    IFNULLRETURNFALSE(value, __LINE__, __FUNCTION__, STR(value));
    str_db = value->GetText();

    //value = value->NextSiblingElement();
    value = element->FirstChildElement("user");
    IFNULLRETURNFALSE(value, __LINE__, __FUNCTION__, STR(value));
    str_user = value->GetText();

    //value = value->NextSiblingElement();
    value = element->FirstChildElement("passwd");
    IFNULLRETURNFALSE(value, __LINE__, __FUNCTION__, STR(value));
    str_passwd = value->GetText();

    return true;
}

bool ReadXMLConfig::get_redis_config(std::string & host, unsigned int & port, std::string & auth) const
{
    const XMLElement * root = xmldoc.RootElement();
    IFNULLRETURNFALSE(root, __LINE__, __FUNCTION__, STR(root));
    const XMLElement * element = root->FirstChildElement("redis");
    IFNULLRETURNFALSE(element, __LINE__, __FUNCTION__, STR(element));

    const XMLElement * value = element->FirstChildElement("host");
    IFNULLRETURNFALSE(value, __LINE__, __FUNCTION__, STR(value));
    host = value->GetText();

    value = element->FirstChildElement("port");
    IFNULLRETURNFALSE(value, __LINE__, __FUNCTION__, STR(value));
    port = std::stoi(value->GetText());

    value = element->FirstChildElement("auth");
    IFNULLRETURNFALSE(value, __LINE__, __FUNCTION__, STR(value));
	auth = value->GetText();

	return true;
}

bool ReadXMLConfig::get_config(CONFIG who, Json::Value & json) const
{
	// TODO
	switch (who)
	{
		case LB_SIA:
			break;
		case LB_SELF:
			break;
		case LB_CM:
			break;
		case LB_MYSQL:
			break;
		case LB_REDIS:
			break;
		case LB_PROMETHEUS:
			break;
		default:
			break;
	}

	return true;
}

std::string ReadXMLConfig::belongs_to(void) const
{
    return std::string("ReadXMLConfig");
}

/********************************ReadJSONConfig********************************/
ReadJSONConfig::ReadJSONConfig()
{
}

ReadJSONConfig::~ReadJSONConfig()
{
    if (in.is_open())
        in.close();
}

bool ReadJSONConfig::load_file(const char * json_file) throw(std::string)
{
    if (!json_file)
    {
        throw std::string("json_file is NULL");
        return false;
    }

    in.open(json_file);
    if (in.is_open())
    {
        if (reader.parse(in, value))
        {
            return true;
        }
        else
        {
            throw std::string("parse json file failed");
            return false;
        }
    }

    throw std::string("load_file open failed");
    return false;
}

bool ReadJSONConfig::get_acd_config(std::string & str_ip, unsigned int & port) const
{
    if (value["self"].isObject())
    {
        if (value["self"]["ip"].isString())
            str_ip = value["self"]["ip"].asString();

        if (value["self"]["port"].isInt())
            port = value["self"]["port"].asUInt();

        return true;
    }

    return false;
}

bool ReadJSONConfig::get_acd_config(Json::Value & json) const
{
    if (value["self"].isObject())
    {
        json = value["self"];
        return true;
    }

    return false;
}

bool ReadJSONConfig::get_cm_config(std::string & str_ip, unsigned int & port) const
{
    if (value["cm"].isObject())
    {
        if (value["cm"]["ip"].isString())
            str_ip = value["cm"]["ip"].asString();

        if (value["cm"]["port"].isInt())
            port = value["cm"]["port"].asUInt();

        return true;
    }

    return false;
}

bool ReadJSONConfig::get_cm_config(Json::Value & json) const
{
    if (value["cm"].isObject())
    {
        json = value["cm"];
        return true;
    }

    return false;
}

bool ReadJSONConfig::get_mysql_config(std::string & str_ip, unsigned int &port, std::string & str_db, std::string & str_user, std::string & str_passwd) const
{
    if (value["mysql"].isObject())
    {
        if (value["mysql"]["host"].isString())
            str_ip = value["mysql"]["host"].asString();

        if (value["mysql"]["port"].isInt())
            port = value["mysql"]["port"].asUInt();

        if (value["mysql"]["db"].isString())
            str_db = value["mysql"]["db"].asString();

        if (value["mysql"]["user"].isString())
            str_user = value["mysql"]["user"].asString();

        if (value["mysql"]["passwd"].isString())
            str_passwd = value["mysql"]["passwd"].asString();

        return true;
    }

    return false;
}

bool ReadJSONConfig::get_mysql_config(Json::Value & json) const
{
    if (value["mysql"].isObject())
    {
        json = value["mysql"];
        return true;
    }

    return false;
}

bool ReadJSONConfig::get_redis_config(std::string & host, unsigned int & port, std::string & auth) const
{
	if (value["redis"].isObject())
	{
		if (value["redis"]["host"].isString())
			host = value["redis"]["host"].asString();

		if (value["redis"]["port"].isUInt())
			port = value["redis"]["port"].asUInt();

		if (value["redis"]["auth"].isString())
			auth = value["redis"]["auth"].asString();
	}

	return true;
}

bool ReadJSONConfig::get_config(CONFIG who, Json::Value & json) const
{
	// TODO
	switch (who)
	{
		case LB_SIA:
			break;
		case LB_SELF:
			break;
		case LB_CM:
			break;
		case LB_MYSQL:
			break;
		case LB_REDIS:
			break;
		case LB_PROMETHEUS:
			break;
		default:
			break;
	}

	return true;
}

std::string ReadJSONConfig::belongs_to(void) const
{
    return std::string("ReadJSONConfig");
}

/*********************************ConfigObject*********************************/
ConfigObject::ConfigObject()
{
}

ConfigObject::~ConfigObject()
{
}

ReadConfig * ConfigObject::get_config_object(OBJECT obj)
{
    switch (obj)
    {
        case XML:
			{
				static ReadXMLConfig config;
				return &config; 
			}            
        case JSON:            
			{
				static ReadJSONConfig config2;
				return &config2;
			}
        defalut:
            return nullptr;
    }

}

/*
int main(int argc, char ** argv)
{
    if (2 > argc)
    {
        cout << "usage\n\t" << argv[0] << " config_file" << endl;
        exit(1);
    }

    std::string ip, ip2;
    unsigned int port, port2;

    ConfigObject obj, obj2;
    ReadConfig * config = ConfigObject::get_instance().get_config_object(XML);
    config->load_file(argv[1]);
    config->get_cm_config(ip, port);
    std::cout << ip << ":" << port << std::endl;
    std::cout<< "config belongs to " << config->belongs_to() << std::endl;

    config = ConfigObject::get_instance().get_config_object(JSON);
    std::cout<< "config belongs to " << config->belongs_to() << std::endl;
    try {
        std::cout << "load: " << config->load_file(argv[2]) << std::endl;;
        std::cout << config->get_acd_config(ip2, port2) << std::endl;
        std::cout << ip2 << ":" << port2 << std::endl;
    } catch(string & e) {
        std::cout << e << std::endl;
    }


    return 0;
}
*/

// End Of File

