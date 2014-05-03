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
{}

ConfigModule::~ConfigModule()
{}

void ConfigModule::ModuleInit()
{
    config_.Init(conf_file_.c_str());

    // 获取各配置项
    timer_init_size_ = config_.Get<int>("timer_init_size");
    connsvr_zmq_addr_ = config_.Get<const char*>("connsvr_zmq_addr");
    datasvr_zmq_addr_ = config_.Get<const char*>("datasvr_zmq_addr");
	player_pool_shm_key_ = config_.Get<int>("player_pool_shm_key");
	player_pool_size_ = config_.Get<int>("player_pool_size");
	city_pool_shm_key_ = config_.Get<int>("city_pool_shm_key");
	city_pool_size_ = config_.Get<int>("city_pool_size");

	LOG(INFO) << ModuleName() << " init ok!";
}

void ConfigModule::ModuleFini()
{
	LOG(INFO) << ModuleName() << " fini completed!";
}

const char* ConfigModule::ModuleName() const
{
	static const std::string module_name = "ConfigModule";
	return module_name.c_str();
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

