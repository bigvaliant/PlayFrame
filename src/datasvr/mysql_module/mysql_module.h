/**
 * @file mysql_module.h
 * @brief mysql模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _MYSQL_MODULE_H_
#define _MYSQL_MODULE_H_

#include "app_def.h"

class MysqlModule : public AppModuleBase,
                    public MysqlMgr 
{
public:
    DISALLOW_COPY_AND_ASSIGN(MysqlModule);

	MysqlModule(App* app);
	virtual ~MysqlModule();

	virtual void            ModuleInit();
	virtual void            ModuleFini();
	virtual const char*     ModuleName() const;
	static int32_t          ModuleId();
	static AppModuleBase*   CreateModule(App* app);

public:
    static std::string ConvertBinToString(MYSQL* conn, const char* bin, int len);
};

#endif 

