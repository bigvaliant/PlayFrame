----------------------------------------
-- @file main.lua
-- @brief lua引擎入口函数 
-- @author fergus <zfengzhen@gmail.com>
-- @version 
-- @date 2014-05-03
----------------------------------------

-- @brief lua info级别打印日志
-- @author fergus
function LOG_INFO(...)
    C_LOG_INFO(string.format(...))
end

-- @brief lua error级别打印日志
-- @author fergus
function LOG_ERROR(...)
    C_LOG_ERROR(string.format(...))
end

-- @brief 执行函数后yield
-- @author fergus
function CO_YIELD()
    return coroutine.yield()
end

-- @brief 模块加载, 替代require
-- @author fergus
function REQUIRE_EX(name)
    if package.loaded[name] then
        LOG_INFO("require_ex module[%s] reload", name)
        package.loaded[name] = nil
    else
        LOG_INFO("require_ex module[%s] first load", name)
    end

    local ret = require(name)
    local ret_type = type(ret)

    if (ret_type == "boolean") then
        LOG_INFO("%s", "require_ex global")
    elseif (ret_type == "table") then
        LOG_INFO("%s", "require_ex table")
        _G[name] = ret
    else
        LOG_INFO("%s", "require_ex unkown format")
        error("require_ex unkown format")
    end
end

-- @brief lua模块热更新
-- @author fergus
function RELOAD()
    LOG_INFO("%s", "main.lua RELOAD...")
    LOG_INFO("%s", LUA_SCRIPT_PATH)
    local ls_cmd = "ls "
        .. LUA_SCRIPT_PATH .. "/*.lua "
        .. LUA_SCRIPT_PATH .. "/*.so "
    
    -- 遍历所有的模块
    for file in io.popen(ls_cmd):lines() do
        -- 去掉路径
        file = string.match(file, ".+/([^/]*%.%w+)$")
        -- 去掉后缀名
        local idx = string.match(file, ".+()%.%w+$")
        if (idx) then
            file = string.sub(file, 1, idx-1)
        end    
        -- 加载模块
        if (file ~= "main") then
            REQUIRE_EX(file)
        end
    end
end

