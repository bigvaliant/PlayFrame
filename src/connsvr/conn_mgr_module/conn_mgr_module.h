/**
 * @file conn_mgr_module.h
 * @brief tcp连接管理类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _CONN_MGR_MODULE_H_
#define _CONN_MGR_MODULE_H_

#include "app_def.h"
#include "connection.h"

class ConnMgrModule : public AppModuleBase
{
public:
    typedef std::map<int32_t, Connection*> ConnectionMap;
    DISALLOW_COPY_AND_ASSIGN(ConnMgrModule);

	ConnMgrModule(App* app);
	virtual ~ConnMgrModule();

	virtual void            ModuleInit();
	virtual void            ModuleFini();
	virtual const char*     ModuleName() const;
	static int32_t          ModuleId();
	static AppModuleBase*   CreateModule(App* app);

public:
    Connection* CreateConn(int32_t conn_fd);
    Connection* GetConn(int32_t conn_fd);
    void ReleaseConn(int32_t conn_fd);
    int32_t GetCurConnMapSize() { return conn_map_.size(); }

private:
    ShmPool<Connection> conn_pool_;
    ConnectionMap       conn_map_;
};

#endif // _CONN_MGR_MODULE_H_

