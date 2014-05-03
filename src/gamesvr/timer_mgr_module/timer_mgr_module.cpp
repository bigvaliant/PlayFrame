/**
 * @file timer_mgr_module.cpp
 * @brief 定时器管理模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "timer_mgr_module.h"
#include "config_module.h"
#include "app.h"

TimerMgrModule::TimerMgrModule(App* app)
	: AppModuleBase(app),
      timer_heap_(NULL)
{}

TimerMgrModule::~TimerMgrModule()
{}

void TimerMgrModule::ModuleInit()
{
    ConfigModule* conf_module = FindModule<ConfigModule>(app_);

    // timer_heap 初始化
    timer_heap_ = new TimerHeap(conf_module->GetTimerInitSize());

    CHECK(timer_heap_ != NULL)
        << "timer heap init error!";

	LOG(INFO) << ModuleName() << " init ok!";
}

void TimerMgrModule::ModuleFini()
{
    if (timer_heap_ != NULL) {
        delete timer_heap_;
        timer_heap_ = NULL;
    }

	LOG(INFO) << ModuleName() << " fini completed!";
}

const char* TimerMgrModule::ModuleName() const
{
	static const std::string ModuleName = "TimerMgrModule";
	return ModuleName.c_str();
}

int32_t TimerMgrModule::ModuleId()
{
	return MODULE_ID_TIMER_MGR;
}

AppModuleBase* TimerMgrModule::CreateModule(App* app)
{
	TimerMgrModule* module = new TimerMgrModule(app);
	if (module != NULL) {
        module->ModuleInit();
	}

	return static_cast<AppModuleBase*>(module);
}

void TimerMgrModule::Poll()
{
    TimeValue now = TimeValue::Time();
    timer_heap_->TimerPoll(now);
}

