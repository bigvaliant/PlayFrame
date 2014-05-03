/**
 * @file lua_engine.cpp
 * @brief 框架Lua引擎, 用于异步事件管理, 调用Lua函数
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#include "lua_engine.h"

LuaEngine::LuaEngine()
    :master_state_(NULL), timer_heap_(NULL), rand_maker_(0)
{
}

LuaEngine::~LuaEngine()
{
    if (master_state_ != NULL)
        lua_close(master_state_);

    master_state_ = NULL;
}

void LuaEngine::LogInfo(const char* str)
{
    LOG(INFO) << "[LUA-PRINT]" << str;
}

void LuaEngine::LogError(const char* str)
{
    LOG(ERROR) << "[LUA-PRINT]" << str;
}

void LuaEngine::AddRequirePath(const char* str)
{
    oss_require_path_ << str << "//?.lua;";
}

void LuaEngine::AddRequireCPath(const char* str)
{
    oss_require_cpath_ << str << "//?.so;";
}

void LuaEngine::Init(const char* main_script_file)
{
    int ret = 0;

    // TODO: 定时器个数采用宏配置
    timer_heap_ = new TimerHeap(1024);
    CHECK(timer_heap_ != NULL)
        << "timer heap init error!";

    master_state_ = luaL_newstate();
    CHECK(master_state_ != NULL)
        << "luaL_newstate error!";

    luaL_openlibs(master_state_);
    lua_tinker::init(master_state_);

    // 将script_main.lua所在文件夹路径加入require路径搜索
    char script_path[256] = {0};
    realpath(main_script_file, script_path);
    strncpy(script_path, dirname(script_path), 256);

    AddRequirePath(script_path);
    AddRequireCPath(script_path);

    // package.path 处理
    std::ostringstream oss_require;
    oss_require
        << "package.path = \"" << oss_require_path_.str() << "\"";

    ret = luaL_dostring(master_state_, oss_require.str().c_str());
    LOG(INFO) << "luaL_dostring [" << oss_require.str().c_str() << "]";
    CHECK(ret == 0)
        << "luaL_dostring [" << oss_require.str().c_str()
        << "] error!";

    oss_require.str("");
    oss_require_path_.str("");

    // package.cpath 处理
    oss_require
        << "package.cpath = \"" << oss_require_cpath_.str() << "\"";

    ret = luaL_dostring(master_state_, oss_require.str().c_str());
    LOG(INFO) << "luaL_dostring [" << oss_require.str().c_str() << "]";
    CHECK(ret == 0)
        << "luaL_dostring [" << oss_require.str().c_str()
        << "] error!";

    oss_require.str("");
    oss_require_cpath_.str("");

    // 设置LUA_SCRIPT_PATH
    SetGlobal("LUA_SCRIPT_PATH", script_path);

    // 注册log函数
    RegFunc("LOG_INFO", &LuaEngine::LogInfo);
    RegFunc("LOG_ERROR", &LuaEngine::LogError);
    // lua_tinker中日志函数
    RegFunc("_ALERT", &LuaEngine::LogInfo);

    ret = luaL_dofile(master_state_, main_script_file);
    if (ret != 0) {
        LOG(ERROR) << lua_tostring(master_state_, -1);
    }
    CHECK(ret == 0)
        << "luaL_dofile error!";

    // 第一次加载模块
    Reload();

    // 创建协程TABLE
    lua_newtable(master_state_);
    lua_setglobal(master_state_, "CO_TABLE");

    // 设置检查信号宏
    SetGlobal<int>("CHECK_SIG_OK", CHECK_SIG_OK);
    SetGlobal<int>("CHECK_SIG_TIMEOUT", CHECK_SIG_TIMEOUT);

    // 设置定时器
    SetHandler(this, &LuaEngine::OnTimer);
}

int32_t LuaEngine::OnTimer(int32_t timer_id, void* data)
{
    UNUSE_ARG(data);
    LOG(INFO)
        << "on_timer: timer_id[" << timer_id
        << "]";

    int32_t rand_id = 0;
    uint64_t task_id = ((uint64_t)timer_id << 32) | ((uint64_t)rand_id);

    // 传入TIMEOUT信号，lua逻辑做超时处理
    Resume(task_id, CHECK_SIG_TIMEOUT);
    // 返回-1 定时器释放
    return -1;
}

uint64_t LuaEngine::CreateTask(const char* lua_func, time_t task_delay_secs)
{
    PrintMemSize("before create_task");
    lua_tinker::enum_stack(master_state_);
    // 分配定时器
    int32_t timer_id = timer_heap_->RegisterTimer(
        TimeValue(task_delay_secs),
        TimeValue(task_delay_secs),
        this,
        NULL);

    // timer_id 分配失败
    if (timer_id <= 0) {
        LOG(ERROR) << "alloc timer failed!";
        return 0;
    }

    int rand_id = rand_maker_.GetRand();
    uint64_t task_id = ((uint64_t)timer_id << 32) | ((uint64_t)rand_id);

    // 以timer_id为lua中CO_TABLE的索引
    // 创建协程
    lua_getglobal(master_state_, "CO_TABLE");
    lua_pushinteger(master_state_, timer_id);
    lua_State *co = lua_newthread(master_state_);
    lua_settable(master_state_, -3);

    // 弹出CO_TABLE
    lua_pop(master_state_, 1);

    // 压入task_id lua逻辑session保存
    SetGlobal<uint64_t>("TASK_ID", task_id);
    SetGlobal<int32_t>("RAND_ID", rand_id);
    SetGlobal<int32_t>("TIMER_ID", timer_id);

    // 将lua_func压入新创建的协程
    lua_getglobal(co, lua_func);
    if (lua_isfunction(co, -1) == 0) {
        LOG(ERROR) << "[" << lua_func << "] is not a function";
        CloseTask(task_id);
        return 0;
    }

    LOG(INFO)
        << "create task[" << task_id
        << "] timer_id[" << timer_id
        << "] rand_id[" << rand_id
        << "]";

    return task_id;
}

void LuaEngine::CloseTask(uint64_t task_id)
{
    int32_t timer_id = (int32_t)(task_id >> 32);
    int32_t rand_id = (int32_t)(task_id & 0xffffffff);

    // 根据time_id去CO_TABLE查找相应的协程
    // 置为nil释放
    // 释放定时器
    if (timer_id > 0) {
        LOG(INFO)
            << "close task[" << task_id
            << "] timer_id[" << timer_id
            << "] rand_id[" << rand_id
            << "]";

        lua_getglobal(master_state_, "CO_TABLE");
        lua_pushinteger(master_state_, timer_id);
        lua_pushnil(master_state_);
        lua_settable(master_state_, -3);

        // 弹出CO_TABLE
        lua_pop(master_state_, 1);

        timer_heap_->UnregisterTimer(timer_id);

        // lua_garbage_collect();
    }

    PrintMemSize("after close_task");
    lua_tinker::enum_stack(master_state_);
    return;
}

void LuaEngine::Resume(uint64_t task_id, int32_t check_sig)
{
    int32_t timer_id = (int32_t)(task_id >> 32);
    int32_t rand_id = (int32_t)(task_id & 0xffffffff);

    LOG(INFO)
        << "resume task[" << task_id
        << "] timer_id[" << timer_id
        << "] rand_id[" << rand_id
        << "] check_sig[" << check_sig
        << "]";

    if (timer_id > 0) {
        lua_getglobal(master_state_, "CO_TABLE");
        lua_pushinteger(master_state_, timer_id);
        lua_gettable(master_state_, -2);

        if (lua_isthread(master_state_, -1) == 0) {
            LOG(ERROR)
                << "CO_TABLE[" << timer_id
                << "] is not thread!";
            return;
        }

        lua_tinker::enum_stack(master_state_);
        lua_State* co = lua_tothread(master_state_, -1);
        if (co == NULL) {
            LOG(ERROR)
                << "CO_TABLE[" << timer_id
                << "] lua_tothread is NULL";
            return;
        }

        lua_pop(master_state_, 2);

        // 设置异常信号以及RAND_ID校验
        SetGlobal<int>("CHECK_SIG", check_sig);
        SetGlobal<int>("CHECK_RAND_ID", rand_id);

        int32_t ret = lua_resume(co, 0);
        if (ret != LUA_YIELD && ret != 0) {
            LOG(ERROR) << "lua_resume failed. ret[" << ret << "]";
            lua_tinker::enum_stack(co);
        }
        if (ret != LUA_YIELD)
            CloseTask(task_id);
    }
}

void LuaEngine::Reload()
{
    lua_getglobal(master_state_, "RELOAD");
    if (lua_isfunction(master_state_, -1) == 0) {
        LOG(ERROR) << "[RELOAD] is not a function";
        return;
    }

    lua_pcall(master_state_, 0, 0, 0);
}

void LuaEngine::PrintMemSize(const char* extra)
{
    int32_t mem_kb = lua_gc(master_state_, LUA_GCCOUNT, 0);
    int32_t mem_m = mem_kb >> 10;
    int32_t mem_b = lua_gc(master_state_, LUA_GCCOUNTB, 0);
    mem_kb = mem_kb - (mem_m << 10);

    LOG(INFO) << "[LUA-MEM] ["
            << mem_m << "M "
            << mem_kb << "KB "
            << mem_b << "B]. "
            << extra;
}

void LuaEngine::LuaGarbageCollect()
{
    lua_gc(master_state_, LUA_GCCOLLECT, 0);
}

