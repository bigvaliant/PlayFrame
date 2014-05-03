----------------------------------------
-- @file main.lua
-- @brief lua引擎入口函数 
-- @author fergus <zfengzhen@gmail.com>
-- @version 
-- @date 2014-05-03
----------------------------------------

-- @brief 模块加载, 替代require
-- @author fergus
function REQUIRE_EX(name)
    if package.loaded[name] then
        LOG_INFO("require_ex module[" .. name "] reload")
        package.loaded[name] = nil
    else
        LOG_INFO("require_ex module[" .. name .. "] first load")
    end

    local ret = require(name)
    local ret_type = type(ret)

    if (ret_type == "boolean") then
        LOG_INFO("require_ex global")
    elseif (ret_type == "table") then
        LOG_INFO("require_ex table")
        _G[name] = ret
    else
        LOG_INFO("require_ex unkown format")
        error("require_ex unkown format")
    end
end

-- @brief 异步TASK YIELD, 增加rand_id校验, 替代yield
-- @author fergus
function CO_YIELD(rand_id)
    local status = 1 
    while (status~= 0) do
        LOG_INFO("yield to cpp...")
        coroutine.yield()

        LOG_INFO("resume to lua...")
        if (CHECK_SIG ~= 0) then
            LOG_INFO("CHECK_SIG[" .. CHECK_SIG .. "] ~= 0")
            return CHECK_SIG 
        end 

        if (CHECK_RAND_ID ~= rand_id) then
            LOG_INFO("CHECK_RAND_ID[" .. CHECK_RAND_ID .. "] ~= rand_id[" .. rand_id .. "]")
        else
            status = 0 
            return 0
        end 
    end -- while
end

-- @brief lua模块热更新
-- @author fergus
function RELOAD()
    LOG_INFO("main.lua RELOAD...")
    LOG_INFO(LUA_SCRIPT_PATH)
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

