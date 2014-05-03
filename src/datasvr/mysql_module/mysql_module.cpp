/**
 * @file mysql_module.cpp
 * @brief mysql模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "mysql_module.h"
#include "app.h"
#include "config_module.h"

MysqlModule::MysqlModule(App* app)
	: AppModuleBase(app)
{}

MysqlModule::~MysqlModule()
{}

void MysqlModule::ModuleInit()
{
    ConfigModule* conf_module = FindModule<ConfigModule>(app_);
    Init(E_MAX_MYSQL_CLIENT_COUNT,
        conf_module->mysql_host(),
        conf_module->mysql_user(),
        conf_module->mysql_password(),
        conf_module->mysql_dbname());

	LOG(INFO) << ModuleName() << " init ok!";
}

void MysqlModule::ModuleFini()
{
	LOG(INFO) << ModuleName() << " fini completed!";
}

const char* MysqlModule::ModuleName() const
{
	static const std::string ModuleName = "MysqlModule";
	return ModuleName.c_str();
}

int32_t MysqlModule::ModuleId()
{
	return MODULE_ID_MYSQL;
}

AppModuleBase* MysqlModule::CreateModule(App* app)
{
	MysqlModule* module = new MysqlModule(app);
	if (module != NULL) {
        module->ModuleInit();
	}

	return static_cast<AppModuleBase*>(module);
}

std::string MysqlModule::ConvertBinToString(MYSQL* conn, const char* bin, int len)
{
    // 单线程函数, 不要在多线程中使用
    static char bin_data[10240];
    bin_data[0] = '\0';
    mysql_real_escape_string(conn, bin_data, bin, len);
    return std::string(bin_data);
}

