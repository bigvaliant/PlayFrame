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
    gamesvr_zmq_addr_ = config_.Get<const char*>("gamesvr_zmq_addr");
    mysql_host_ = config_.Get<const char*>("mysql_host");
    mysql_user_ = config_.Get<const char*>("mysql_user");
    mysql_passwor_ = config_.Get<const char*>("mysql_password");
    mysql_dbname_ = config_.Get<const char*>("mysql_dbname");

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

