/**
 * @file lua_engine.h
 * @brief 框架Lua引擎, 用于异步事件管理, 调用Lua函数
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#ifndef  _LUA_ENGINE_H
#define  _LUA_ENGINE_H

#include "comm_def.h"
#include "callback.h"
#include "heap_timer.h"
#include "random.h"
#include "lua_tinker.h"

enum CHECK_SIG {
    CHECK_SIG_OK = 0,
    CHECK_SIG_TIMEOUT = 1,
};

class LuaEngine: public CallbackObject
{
public:
    DISALLOW_COPY_AND_ASSIGN(LuaEngine);

    LuaEngine();
    virtual ~LuaEngine();

public:

    void AddRequirePath(const char* str);
    void AddRequireCPath(const char* str);
    void Init(const char* main_script_file);
    void Fini();
    int32_t OnTimer(int64_t timer_id, void* data);

    void Run()
    {
        TimeValue now = TimeValue::Time();
        heap_timer_->TimerPoll(now);
    }

    int32_t GetTaskSize();

    template<typename T>
    T GetGlobal(const char* name)
    {
        return lua_tinker::get<T>(master_state_, name);
    }

    template<typename T>
    void SetGlobal(const char* name, T object)
    {
        lua_tinker::set<T>(master_state_, name, object);
    }

    //导入全局函数
    template<typename F>
    void RegFunc(const char* name, F func)
    {
        lua_tinker::def<F>(master_state_, name, func);
    }

    //导入class
    template<typename Class>
    void RegClass(const char *name) {
        lua_tinker::class_add<Class>(master_state_, name);
    }

    template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
    void RegClassCon() {
        lua_tinker::class_con<T>(master_state_, lua_tinker::constructor<T, T1, T2, T3, T4, T5>);
    }

    template<typename T, typename T1, typename T2, typename T3, typename T4>
    void RegClassCon() {
        lua_tinker::class_con<T>(master_state_, lua_tinker::constructor<T, T1, T2, T3, T4>);
    }

    template<typename T, typename T1, typename T2, typename T3>
    void RegClassCon() {
        lua_tinker::class_con<T>(master_state_, lua_tinker::constructor<T, T1, T2, T3>);
    }

    template<typename T, typename T1, typename T2>
    void RegClassCon() {
        lua_tinker::class_con<T>(master_state_, lua_tinker::constructor<T, T1, T2>);
    }

    template<typename T, typename T1>
    void RegClassCon() {
        lua_tinker::class_con<T>(master_state_, lua_tinker::constructor<T, T1>);
    }

    template<typename T>
    void RegClassCon() {
        lua_tinker::class_con<T>(master_state_, lua_tinker::constructor<T>);
    }

    template<typename T, typename P>
    void RegClassInh()
    {
        lua_tinker::class_inh<T, P>(master_state_);
    }

    template<typename Class, typename F>
    void RegClassFunc(const char* name, F func) {
        lua_tinker::class_def<Class>(master_state_, name, func);
    }

    template<typename Class, typename BASE, typename VAR>
    void RegClassVar(const char* name, VAR BASE::*val) {
        lua_tinker::class_mem<Class>(master_state_, name, val);
    }

public:

    template <typename R>
    inline R CallLua(const char *func)
    {
        return lua_tinker::call<R>(master_state_, func);
    }

    template <typename R, typename T1>
    inline R CallLua(const char *func, T1 t1)
    {
        return lua_tinker::call<R, T1>(master_state_, func, t1);
    }

    template <typename R, typename T1, typename T2>
    inline R CallLua(const char *func, T1 t1, T2 t2)
    {
        return lua_tinker::call<R, T1, T2>(master_state_, func, t1, t2);
    }

    template <typename R, typename T1, typename T2, typename T3>
    inline R CallLua(const char *func, T1 t1, T2 t2, T3 t3)
    {
        return lua_tinker::call<R, T1, T2, T3>(master_state_, func, t1, t2, t3);
    }

    template <typename R, typename T1, typename T2, typename T3, typename T4>
    inline R CallLua(const char *func, T1 t1, T2 t2, T3 t3, T4 t4)
    {
        return lua_tinker::call<R, T1, T2, T3, T4>(master_state_, func, t1, t2, t3, t4);
    }

    template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
    inline R CallLua(const char *func, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
    {
        return lua_tinker::call<R, T1, T2, T3, T4, T5>(master_state_, func, t1, t2, t3, t4, t5);
    }

    int64_t CreateTask(const char* lua_func, time_t task_delay_secs);
    void Resume(int64_t task_id);

    template<class T1>
    void Resume(int64_t task_id, T1 t1)
    {
        LOG(INFO) << "resume task[" << task_id << "]";

        if (task_id > 0 && co_map_[task_id] != NULL) {
            lua_State* co = co_map_[task_id];

            lua_tinker::push(co, t1);
            int32_t ret = lua_resume(co, 1);
            if (ret != LUA_YIELD && ret != 0) {
                LOG(ERROR) << "lua_resume failed. ret[" << ret << "]";
                lua_tinker::enum_stack(co);
            }
            if (ret != LUA_YIELD)
                CloseTask(task_id);
        }
    }

    template<class T1, class T2>
    void Resume(int64_t task_id, T1 t1, T2 t2)
    {
        LOG(INFO) << "resume task[" << task_id << "]";

        if (task_id > 0 && co_map_[task_id] != NULL) {
            lua_State* co = co_map_[task_id];

            lua_tinker::push(co, t1);
            lua_tinker::push(co, t2);
            int32_t ret = lua_resume(co, 2);
            if (ret != LUA_YIELD && ret != 0) {
                LOG(ERROR) << "lua_resume failed. ret[" << ret << "]";
                lua_tinker::enum_stack(co);
            }
            if (ret != LUA_YIELD)
                CloseTask(task_id);
        }
    }

    template<class T1, class T2, class T3>
    void Resume(int64_t task_id, T1 t1, T2 t2, T3 t3)
    {
        LOG(INFO) << "resume task[" << task_id << "]";

        if (task_id > 0 && co_map_[task_id] != NULL) {
            lua_State* co = co_map_[task_id];

            lua_tinker::push(co, t1);
            lua_tinker::push(co, t2);
            lua_tinker::push(co, t3);
            int32_t ret = lua_resume(co, 3);
            if (ret != LUA_YIELD && ret != 0) {
                LOG(ERROR) << "lua_resume failed. ret[" << ret << "]";
                lua_tinker::enum_stack(co);
            }
            if (ret != LUA_YIELD)
                CloseTask(task_id);
        }
    }

    template<class T1, class T2, class T3, class T4>
    void Resume(int64_t task_id, T1 t1, T2 t2, T3 t3, T4 t4)
    {
        LOG(INFO) << "resume task[" << task_id << "]";

        if (task_id > 0 && co_map_[task_id] != NULL) {
            lua_State* co = co_map_[task_id];

            lua_tinker::push(co, t1);
            lua_tinker::push(co, t2);
            lua_tinker::push(co, t3);
            lua_tinker::push(co, t4);
            int32_t ret = lua_resume(co, 4);
            if (ret != LUA_YIELD && ret != 0) {
                LOG(ERROR) << "lua_resume failed. ret[" << ret << "]";
                lua_tinker::enum_stack(co);
            }
            if (ret != LUA_YIELD)
                CloseTask(task_id);
        }
    }

    template<class T1, class T2, class T3, class T4, class T5>
    void Resume(int64_t task_id, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5)
    {
        LOG(INFO) << "resume task[" << task_id << "]";

        if (task_id > 0 && co_map_[task_id] != NULL) {
            lua_State* co = co_map_[task_id];

            lua_tinker::push(co, t1);
            lua_tinker::push(co, t2);
            lua_tinker::push(co, t3);
            lua_tinker::push(co, t4);
            lua_tinker::push(co, t5);
            int32_t ret = lua_resume(co, 5);
            if (ret != LUA_YIELD && ret != 0) {
                LOG(ERROR) << "lua_resume failed. ret[" << ret << "]";
                lua_tinker::enum_stack(co);
            }
            if (ret != LUA_YIELD)
                CloseTask(task_id);
        }
    }

    template<class T1, class T2, class T3, class T4, class T5, class T6>
    void Resume(int64_t task_id, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6)
    {
        LOG(INFO) << "resume task[" << task_id << "]";

        if (task_id > 0 && co_map_[task_id] != NULL) {
            lua_State* co = co_map_[task_id];

            lua_tinker::push(co, t1);
            lua_tinker::push(co, t2);
            lua_tinker::push(co, t3);
            lua_tinker::push(co, t4);
            lua_tinker::push(co, t5);
            lua_tinker::push(co, t6);
            int32_t ret = lua_resume(co, 6);
            if (ret != LUA_YIELD && ret != 0) {
                LOG(ERROR) << "lua_resume failed. ret[" << ret << "]";
                lua_tinker::enum_stack(co);
            }
            if (ret != LUA_YIELD)
                CloseTask(task_id);
        }
    }

    template<class T1, class T2, class T3, class T4, class T5, class T6, class T7>
    void Resume(int64_t task_id, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7)
    {
        LOG(INFO) << "resume task[" << task_id << "]";

        if (task_id > 0 && co_map_[task_id] != NULL) {
            lua_State* co = co_map_[task_id];

            lua_tinker::push(co, t1);
            lua_tinker::push(co, t2);
            lua_tinker::push(co, t3);
            lua_tinker::push(co, t4);
            lua_tinker::push(co, t5);
            lua_tinker::push(co, t6);
            lua_tinker::push(co, t7);
            int32_t ret = lua_resume(co, 7);
            if (ret != LUA_YIELD && ret != 0) {
                LOG(ERROR) << "lua_resume failed. ret[" << ret << "]";
                lua_tinker::enum_stack(co);
            }
            if (ret != LUA_YIELD)
                CloseTask(task_id);
        }
    }

    template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
    void Resume(int64_t task_id, T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8)
    {
        LOG(INFO) << "resume task[" << task_id << "]";

        if (task_id > 0 && co_map_[task_id] != NULL) {
            lua_State* co = co_map_[task_id];

            lua_tinker::push(co, t1);
            lua_tinker::push(co, t2);
            lua_tinker::push(co, t3);
            lua_tinker::push(co, t4);
            lua_tinker::push(co, t5);
            lua_tinker::push(co, t6);
            lua_tinker::push(co, t7);
            lua_tinker::push(co, t8);
            int32_t ret = lua_resume(co, 8);
            if (ret != LUA_YIELD && ret != 0) {
                LOG(ERROR) << "lua_resume failed. ret[" << ret << "]";
                lua_tinker::enum_stack(co);
            }
            if (ret != LUA_YIELD)
                CloseTask(task_id);
        }
    }

    void Reload();
    void PrintMemSize(const char* extra=NULL);
    void LuaGarbageCollect();
    static void LogInfo(const char* str);
    static void LogError(const char* str);

private:
    lua_State* FindTask(int64_t task_id);
    void CloseTask(int64_t task_id);
private:
    typedef std::map<int64_t, lua_State*> CoroutineMap;
    CoroutineMap        co_map_;
    lua_State*          master_state_;
    HeapTimer*          heap_timer_;
    Random              rand_maker_;
    std::ostringstream  oss_require_path_;
    std::ostringstream  oss_require_cpath_;
};

#endif
