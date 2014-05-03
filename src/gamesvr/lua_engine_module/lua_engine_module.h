/**
 * @file lua_engine_module.h
 * @brief Lua引擎模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _LUA_ENGINE_MODULE_H_
#define _LUA_ENGINE_MODULE_H_

#include "app_def.h"

#define REG_FUNC(CLASS_NAME, FUNC_NAME) \
    RegClassFunc<CLASS_NAME>(#FUNC_NAME, &CLASS_NAME::FUNC_NAME)

class LuaEngineModule : public AppModuleBase,
                        public LuaEngine
{
public:
	LuaEngineModule(App* app, const char* main_script_file);
	virtual ~LuaEngineModule();

	virtual void            ModuleInit();
	virtual void            ModuleFini();
	virtual const char*     ModuleName() const;
	static int32_t          ModuleId();
	static AppModuleBase*   CreateModule(App* app, const char* main_script_script);
};

#endif 

