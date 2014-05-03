/**
 * @file config_module.h
 * @brief 配置模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _CONFIG_MODULE_H_
#define _CONFIG_MODULE_H_

#include "app_def.h"

class ConfigModule : public AppModuleBase
{
public:
	ConfigModule(App* app, const char* lua_conf_file);
	virtual ~ConfigModule();

	virtual void            ModuleInit();
	virtual void            ModuleFini();
	virtual const char*     ModuleName() const;
	static int32_t          ModuleId();
	static AppModuleBase*   CreateModule(App* app, const char* conf_file);

public:
    inline const char* gamesvr_zmq_addr() const
    {
        return gamesvr_zmq_addr_.c_str();
    }

    inline const char* mysql_host() const
    {
        return mysql_host_.c_str();
    }

    inline const char* mysql_user() const
    {
        return mysql_user_.c_str();
    }

    inline const char* mysql_password() const
    {
        return mysql_passwor_.c_str();
    }

    inline const char* mysql_dbname() const
    {
        return mysql_dbname_.c_str();
    }

private:
    std::string     conf_file_;
    LuaConfig       config_;

private:
    std::string     gamesvr_zmq_addr_;
    std::string     mysql_host_;
    std::string     mysql_user_;
    std::string     mysql_passwor_;
    std::string     mysql_dbname_;
};

#endif // _CONFIG_MODULE_H_

