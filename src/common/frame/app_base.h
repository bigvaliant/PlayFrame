/**
 * @file app_base.h
 * @brief App以及AppModule基类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
/*
                ---------------
                | ServerFrame |
                ---------------
                       |
                       |  加载
                  ---------
                  |  App  |
                  ---------
                    /   \
                   /     \  一个App拥有多个AppModule
                  /       \
        -------------  -------------
        | AppModule |  | AppModule |
        -------------  -------------
*/
// ServerFrame 加载一个App, App继承AppBase
// App实现AppBase中的方法, 实现GetInstance单例的方法
// ServerFrame 会调用GetInstance方法用于加载App
// ServerFrame 会调用AppRun, 用于整个App的主循环
// ServerFrame 会调用AppInit, 用于App的初始化, 主要用于加载各个AppModule
// ServerFrame 收到reload指令时, 会调用AppReload
// ServerFrame 收到stop指令时, 会调用AppStop
// ServerFrame 收到clean指令时, 会调用AppClean
//
// App通过AddModule加载AppModule
// App通过FindModule索引AppModule
//
// AppModule实现CreateModule静态方法创建AppModule, 第一个参数为AppBase*指针
// AppModule实现ModuleId静态方法, 用于保存AppModule的唯一Id
// 该AppBase*指针用于AppModule索引App
// AppModule实现ModuleInit用于Module的初始化
// AppModule实现ModuleFini用于Module的结束处理
// AppModule实现ModuleName, 用于保存AppModule的名字

#ifndef _APP_BASE_H_
#define _APP_BASE_H_

#include "comm_def.h"

class AppBase;

class AppModuleBase
{
public:
    explicit AppModuleBase(AppBase* app) : app_(app) {}
    virtual ~AppModuleBase() {};
    virtual void ModuleInit() = 0;
    virtual void ModuleFini() = 0;
    virtual const char* ModuleName() const = 0;

    AppBase* app_;
};

class AppBase
{
public:
    DISALLOW_COPY_AND_ASSIGN(AppBase);

    AppBase() {}

    virtual ~AppBase() {}; 

    // 子类实现
    virtual int32_t AppInit() = 0;
    virtual const char* AppName() const = 0;
    virtual int32_t AppRun() = 0;
    static AppBase* GetInstance();

    virtual void AppStop()
    {
        if(module_map_.empty())
            return;

        LOG(INFO) << AppName() << " now stoping";

        //按创建相反的顺序来析构，从而可以解决析构中有相互依赖的情况
        ModuleMap::const_reverse_iterator it = module_map_.rbegin();
        while (it != module_map_.rend()) {
            AppModuleBase* temp = it->second;
            if (temp != NULL) {
                temp->ModuleFini();
                delete temp;
                temp = NULL;
            }
            ++it;
        }

        module_map_.clear();

        LOG(INFO) << AppName() << " stop completed!";
        sleep(1);
    }

    virtual void AppReload()
    {
        LOG(INFO) << " AppBase recv reload signal!";
    }

    virtual void AppClean()
    {
        LOG(INFO) << " AppBase recv clean signal!";
    }

    void AddModule(uint64_t module_id, AppModuleBase* app_module) {
        module_map_[module_id] = app_module;
    }

    AppModuleBase* FindModule(int32_t module_id) const
    {
        AppModuleBase* module;
        ModuleMap::const_iterator it = module_map_.find(module_id);
        if(it == module_map_.end())
            module = NULL;
        else
            module = it->second;

        CHECK(module != NULL)
            << "module[" << module_id
            << "]  not register!";
        return module;
    }

    template<typename T>
    T* GetModule() const {
        ModuleMap::const_iterator it = module_map_.find(T::ModuleId());
        CHECK(it != module_map_.end())
            << "get_module error!";
        return dynamic_cast<T*>(it->second);
    }

private:
    typedef std::map<uint64_t, AppModuleBase*> ModuleMap;
    ModuleMap module_map_;
};

template<class T>
T* FindModule(const AppBase* app_base)
{
    AppModuleBase* module = app_base->FindModule(T::ModuleId());
    return static_cast<T*>(module);
}

#endif //_APP_BASE_H_

