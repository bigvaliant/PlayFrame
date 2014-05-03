/**
 * @file app.cpp
 * @brief App实现
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "app.h"
#include "config_module.h"
#include "msg_module.h"
#include "mysql_module.h"

ServerFrame g_server("0.0.1");
ServerFrame* g_svr_frame = &g_server;

int32_t main(int argc, char * argv[])
{
    return g_server.Run(argc, argv);
}

//////////////////////////////////////////////////////////////////////////
App* App::instance_ = NULL;

const char * App::AppName() const
{
    // 名字必须跟exe名字一样
    static const std::string name = "datasvr";
    return name.c_str();
}

// 供在类ServerFrame中创建应用实例
AppBase* AppBase::GetInstance()
{
    if (App::instance_ == NULL) {
        App::instance_ = new App();
    }
    return App::instance_;
}

int32_t App::AppInit()
{
    // 初始化module
    AppModuleBase* app_module = NULL;

    app_module = ConfigModule::CreateModule(this, g_svr_frame->conf_name_);
    AddModule(ConfigModule::ModuleId(), app_module);

    app_module = MsgModule::CreateModule(this);
    AddModule(MsgModule::ModuleId(), app_module);

    app_module = MysqlModule::CreateModule(this);
    AddModule(MysqlModule::ModuleId(), app_module);

    LOG(INFO) << AppName() << " init succeed!";
    return 0;
}

int32_t App::AppRun()
{    
    static MsgModule* msg_module = GetModule<MsgModule>();

    msg_module->Run();
    return 0;
}

void App::AppClean()
{
}

void App::AppReload()
{
}

