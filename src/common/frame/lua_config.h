/**
 * @file lua_config.h
 * @brief 读取Lua配置类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#ifndef _LUA_CONFIG_
#define _LUA_CONFIG_

#include "comm_def.h"
#include "lua_tinker.h"

class LuaConfig
{
public:
    DISALLOW_COPY_AND_ASSIGN(LuaConfig);
    LuaConfig();
    ~LuaConfig();

    void Init(const char* lua_file);
    void Close();

    template<typename T>
    T Get(const char* name)
    {
        return lua_tinker::get<T>(state_, name);
    }

private:
    lua_State* state_;
};

#endif 
