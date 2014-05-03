/**
 * @file conn_mgr_module.cpp
 * @brief tcp连接管理类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "conn_mgr_module.h"
#include "app.h"
#include "config_module.h"

ConnMgrModule::ConnMgrModule(App* app)
	: AppModuleBase(app)
{}

ConnMgrModule::~ConnMgrModule()
{}

void ConnMgrModule::ModuleInit()
{
    ConfigModule* conf_module = FindModule<ConfigModule>(app_);
    int32_t conn_pool_shm_key = conf_module->conn_pool_shm_key();
    int32_t conn_pool_size = conf_module->conn_pool_size();

    char cmd[512] = {0};
    snprintf(cmd, 512, "ipcrm -M 0x%08x", conn_pool_shm_key);
    system(cmd);

    CHECK(conn_pool_.Init(conn_pool_size, conn_pool_shm_key) == 0)
        << "conn_pool init error!";
	LOG(INFO) << ModuleName() << " init ok!";
}

void ConnMgrModule::ModuleFini()
{
    for (ConnectionMap::iterator it = conn_map_.begin();
        it != conn_map_.end(); ++it) {
        close(it->first);
    }

	LOG(INFO) << ModuleName() << " fini completed!";
}

const char* ConnMgrModule::ModuleName() const
{
	static const std::string module_name = "ConnMgrModule";
	return module_name.c_str();
}

int32_t ConnMgrModule::ModuleId()
{
	return MODULE_ID_CONN_MGR;
}

AppModuleBase* ConnMgrModule::CreateModule(App* app)
{
	ConnMgrModule* conn_mgr_module = new ConnMgrModule(app);
	if (conn_mgr_module != NULL) {
        conn_mgr_module->ModuleInit();
	}

	return static_cast<AppModuleBase*>(conn_mgr_module);
}

Connection* ConnMgrModule::CreateConn(int32_t conn_fd)
{
    Connection* conn = (Connection*)conn_pool_.Alloc();
    conn->Init();
    conn_map_[conn_fd] = conn;
    LOG(INFO) << "create_conn conn_fd[" << conn_fd << "]";

    return conn;
}

Connection* ConnMgrModule::GetConn(int32_t conn_fd)
{
    ConnectionMap::const_iterator it = conn_map_.find(conn_fd);
    if (it == conn_map_.end()) {
        LOG(ERROR)
            << "conn fd[" << conn_fd
            << "] not find!";
        return NULL;
    }
    return conn_map_[conn_fd];
}

void ConnMgrModule::ReleaseConn(int32_t conn_fd)
{
    ConnectionMap::const_iterator it = conn_map_.find(conn_fd);
    if (it == conn_map_.end()) {
        return;
    }

    Connection* conn = conn_map_[conn_fd];
    if (conn != NULL && conn->conn_fd() == conn_fd) {
        LOG(INFO) << "release_conn[" << conn_fd << "]";
        conn_pool_.Release((void*)conn);
        conn_map_.erase(conn_fd);
        close(conn_fd);
        LOG(INFO) << "release_conn conn_fd[" << conn_fd << "]";
    }
}

