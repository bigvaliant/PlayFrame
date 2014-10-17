/**
 * @file lua_engine_module.cpp
 * @brief Lua引擎模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "lua_engine_module.h"
#include "obj_mgr_module.h"
#include "player.h"
#include "app.h"

LuaEngineModule::LuaEngineModule(App* app,
    const char* main_script_file)
	: AppModuleBase(app)
{
    Init(main_script_file);
}

LuaEngineModule::~LuaEngineModule()
{
    while (GetTaskSize() != 0) {
        LOG(INFO) << "wait for [" << GetTaskSize() << "] to finish!";
        Run();
        sleep(1);  
    }
}

void LuaEngineModule::ModuleInit()
{
    // 初始化要调用的类
    // ObjMgrModule
    RegClass<ObjMgrModule>("ObjMgrModule");
    REG_FUNC(ObjMgrModule, find_player_idx_by_uid);
    REG_FUNC(ObjMgrModule, add_player);
    REG_FUNC(ObjMgrModule, get_player);
    REG_FUNC(ObjMgrModule, del_player);

    //Utils
    RegFunc("gen_uid_num", &Utils::GenUidNum);
    RegFunc("fnv_64a_hash", &Utils::Fnv64aHash);
    RegFunc("check_account", &Utils::CheckAccount);
    RegFunc("check_password", &Utils::CheckPassword);

    // Player
    // Player 自身属性
    RegClass<Player>("Player");
    REG_FUNC(Player, set_player_by_db);
    REG_FUNC(Player, get_player_idx);
    REG_FUNC(Player, set_uid);
    REG_FUNC(Player, get_uid);
    REG_FUNC(Player, set_password_hash);
    REG_FUNC(Player, get_password_hash);
    // Player 客户端交互
    REG_FUNC(Player, send_failed_cs_res);
    REG_FUNC(Player, send_ok_cs_quick_reg_res);
    REG_FUNC(Player, send_ok_cs_normal_reg_res);
    REG_FUNC(Player, send_ok_cs_login_res);
    REG_FUNC(Player, send_role_info_notify);
    REG_FUNC(Player, send_server_kick_off_notify);
    // Player datasvr交互
    REG_FUNC(Player, do_account_reg);
    REG_FUNC(Player, do_account_verify);
    REG_FUNC(Player, do_get_player_data);
    REG_FUNC(Player, do_update_player_data);

    // 设置要调用的module
    ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(app_);
    SetGlobal<ObjMgrModule*>("OBJ_MGR_MODULE", obj_mgr_module);

	LOG(INFO) << ModuleName() << " init ok!";
}

void LuaEngineModule::ModuleFini()
{
	LOG(INFO) << ModuleName() << " fini completed!";
}

const char* LuaEngineModule::ModuleName() const
{
	static const std::string ModuleName = "LuaEngineModule";
	return ModuleName.c_str();
}

int32_t LuaEngineModule::ModuleId()
{
	return MODULE_ID_LUA_ENGINE;
}

AppModuleBase* LuaEngineModule::CreateModule(App* app,
    const char* main_script_file)
{
	LuaEngineModule* module = new LuaEngineModule(app, main_script_file);
	if (module != NULL) {
        module->ModuleInit();
	}

	return static_cast<AppModuleBase*>(module);
}

