/**
 * @file lua_config.cpp
 * @brief 读取Lua配置类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#include "lua_config.h"

LuaConfig::LuaConfig()
{
    state_ = NULL;
}

LuaConfig::~LuaConfig()
{
    if (state_ != NULL)
        lua_close(state_);

    state_ = NULL;
}

void LuaConfig::Init(const char* lua_file)
{
    int32_t ret = 0;

    state_ = luaL_newstate();
    CHECK(state_ != NULL)
        << "luaL_newstate error!";

    luaL_openlibs(state_);

    ret = luaL_dofile(state_, lua_file);
    CHECK(ret == 0)
        << "luaL_dofile error!";
}

void LuaConfig::Close()
{
    if (state_ != NULL)
        lua_close(state_);
    state_ = NULL;
}
