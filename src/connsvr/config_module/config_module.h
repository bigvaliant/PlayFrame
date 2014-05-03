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
	inline int32_t listen_port() const 
    {
		return listen_port_;
	}

	inline int32_t conn_pool_shm_key() const
    {
		return conn_pool_shm_key_;
	}

	inline int32_t conn_pool_size() const
    {
		return conn_pool_size_;
	}

    inline const char* gamesvr_zmq_addr() const
    {
        return gamesvr_zmq_addr_.c_str();
    }

private:
    std::string     conf_file_;
    LuaConfig       config_;

private:
    int32_t         listen_port_;
    int32_t         conn_pool_shm_key_;
    int32_t         conn_pool_size_;
    std::string     gamesvr_zmq_addr_;
};

#endif // _CONFIG_MODULE_H_

