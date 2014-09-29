/**
 * @file lua_engine.cpp
 * @brief 框架Lua引擎, 用于异步事件管理, 调用Lua函数
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#include "lua_engine.h"

LuaEngine::LuaEngine()
    :master_state_(NULL), heap_timer_(NULL), rand_maker_(0)
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

void LuaEngine::Fini()
{
    CoroutineMap::const_reverse_iterator it = co_map_.rbegin();
    while (it != co_map_.rend()) {
        lua_State* co= it->second;
        if (co != NULL) {
            co = NULL;
        }
        ++it;
    }
}

void LuaEngine::Init(const char* main_script_file)
{
    Fini();
    int ret = 0;

    // TODO: 定时器个数采用宏配置
    heap_timer_ = new HeapTimer(1024);
    CHECK(heap_timer_ != NULL)
        << "timer heap init error!";

    master_state_ = luaL_newstate();
    CHECK(master_state_ != NULL)
        << "luaL_newstate error!";

    luaL_openlibs(master_state_);
    lua_tinker::init(master_state_);
    lua_tinker::init_s64(master_state_);
    lua_tinker::init_u64(master_state_);

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
    RegFunc("C_LOG_INFO", &LuaEngine::LogInfo);
    RegFunc("C_LOG_ERROR", &LuaEngine::LogError);
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

    // 设置检查信号宏
    SetGlobal<int>("CHECK_SIG_OK", CHECK_SIG_OK);
    SetGlobal<int>("CHECK_SIG_TIMEOUT", CHECK_SIG_TIMEOUT);

    // 设置定时器
    SetHandler(this, &LuaEngine::OnTimer);
}

int32_t LuaEngine::OnTimer(int64_t task_id, void* data)
{
    UNUSE_ARG(data);
    LOG(INFO) << "OnTimer: task_id[" << task_id << "] TIMEOUT!!!!";

    CloseTask(task_id);
    return -1;
}

int64_t LuaEngine::CreateTask(const char* lua_func, time_t task_delay_secs)
{
    PrintMemSize("before create_task");
    // 分配定时器
    int64_t task_id = heap_timer_->RegisterTimer(
        TimeValue(task_delay_secs),
        TimeValue(task_delay_secs),
        this,
        NULL);

    // task_id 分配失败
    if (task_id <= 0) {
        LOG(ERROR) << "alloc timer failed!";
        return -1;
    }

    // There is no explicit function to close or to destroy a thread. 
    // Threads are subject to garbage collection, like any Lua object.
    lua_State* co = lua_newthread(master_state_);
    if (co == NULL) {
        LOG(ERROR) << "lua_newthread error!"; 
        return -1;
    }
    lua_pop(master_state_, 1);

    co_map_[task_id] = co;

    // 将lua_func压入新创建的协程
    lua_getglobal(co, lua_func);
    if (lua_isfunction(co, -1) == 0) {
        LOG(ERROR) << "[" << lua_func << "] is not a function";
        CloseTask(task_id);
        return 0;
    }

    LOG(INFO) << "create task[" << task_id << "]";
    LOG(INFO) << "TaskSize[" << GetTaskSize() << "]";

    return task_id;
}

void LuaEngine::CloseTask(int64_t task_id)
{
    if (task_id > 0 && co_map_[task_id] != NULL) {
        LOG(INFO)
            << "close task[" << task_id
            << "]";
        co_map_.erase(task_id);
        heap_timer_->UnregisterTimer(task_id);
    }

    PrintMemSize("after close_task");
    lua_tinker::enum_stack(master_state_);
    LOG(INFO) << "TaskSize[" << GetTaskSize() << "]";
    return;
}

void LuaEngine::Resume(int64_t task_id)
{
    LOG(INFO) << "resume task[" << task_id << "]";

    if (task_id > 0 && co_map_[task_id] != NULL) {
        lua_State* co = co_map_[task_id];

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

lua_State* LuaEngine::FindTask(int64_t task_id)
{
    CoroutineMap::const_iterator it = co_map_.find(task_id);
    if (it == co_map_.end()) {
        return NULL;
    } else {
        return it->second;
    }
}

int32_t LuaEngine::GetTaskSize()
{
    return (int32_t)co_map_.size();
}
