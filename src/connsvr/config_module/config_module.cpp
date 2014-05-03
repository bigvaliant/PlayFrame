/**
 * @file config_module.cpp
 * @brief 配置模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "config_module.h"
#include "app.h"

ConfigModule::ConfigModule(App* app, const char* lua_conf_file)
	: AppModuleBase(app),
      conf_file_(lua_conf_file)
{
    app_ = app;
}

ConfigModule::~ConfigModule()
{}

void ConfigModule::ModuleInit()
{
    config_.Init(conf_file_.c_str());

    // 获取各配置项
	listen_port_ = config_.Get<int>("listen_port");
	conn_pool_shm_key_ = config_.Get<int>("conn_pool_shm_key");
	conn_pool_size_ = config_.Get<int>("conn_pool_size");
    gamesvr_zmq_addr_ = config_.Get<const char*>("gamesvr_zmq_addr");

	LOG(INFO) << ModuleName() << " init ok!";
}

void ConfigModule::ModuleFini()
{
	LOG(INFO) << ModuleName() << " fini completed!";
}

const char* ConfigModule::ModuleName() const
{
	static const std::string ModuleName = "ConfigModule";
	return ModuleName.c_str();
}

int32_t ConfigModule::ModuleId()
{
	return MODULE_ID_CONFIG;
}

AppModuleBase* ConfigModule::CreateModule(App* app, const char* conf_file)
{
	ConfigModule* conf_module = new ConfigModule(app, conf_file);
	if (conf_module != NULL) {
        conf_module->ModuleInit();
	}

	return static_cast<AppModuleBase*>(conf_module);
}

