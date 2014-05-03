/**
 * @file timer_mgr_module.h
 * @brief 定时器管理模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _TIMER_MGR_MODULE_H_
#define _TIMER_MGR_MODULE_H_

#include "app_def.h"

class TimerMgrModule : public AppModuleBase
{
public:
    DISALLOW_COPY_AND_ASSIGN(TimerMgrModule);

	TimerMgrModule(App* app);
	virtual ~TimerMgrModule();

	virtual void            ModuleInit();
	virtual void            ModuleFini();
	virtual const char*     ModuleName() const;
	static int32_t          ModuleId();
	static AppModuleBase*   CreateModule(App* app);

public:
    void    Poll();
    TimerHeap* GetTimerHeap()
    {
        return timer_heap_;
    }

private:
    TimerHeap*      timer_heap_;
};

#endif // _TIMER_MGR_MODULE_H_

