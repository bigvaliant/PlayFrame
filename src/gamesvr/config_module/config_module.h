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
    inline int32_t GetTimerInitSize() const 
    {
        return timer_init_size_;
    }

    inline const char* GetConnsvrZmqAddr() const 
    {
        return connsvr_zmq_addr_.c_str();
    }

    inline const char* GetDatasvrZmqAddr() const 
    {
        return datasvr_zmq_addr_.c_str();
    }

	inline int32_t GetPlayerPoolShmKey() const 
    {
		return player_pool_shm_key_;
	}

	inline int32_t GetPlayerPoolSize() const 
    {
		return player_pool_size_;
	}
    
	inline int32_t GetCityPoolShmKey() const 
    {
		return city_pool_shm_key_;
	}

	inline int32_t GetCityPoolSize() const 
    {
		return city_pool_size_;
	}
    
private:
    std::string     conf_file_;
    LuaConfig       config_;

private:
    int32_t         timer_init_size_;
    std::string     connsvr_zmq_addr_;
    std::string     datasvr_zmq_addr_;
    int32_t         player_pool_shm_key_;
    int32_t         player_pool_size_;
    int32_t         city_pool_shm_key_;
    int32_t         city_pool_size_;
};

#endif // _CONFIG_MODULE_H_

