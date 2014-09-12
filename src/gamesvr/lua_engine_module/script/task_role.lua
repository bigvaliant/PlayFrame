----------------------------------------
-- @file task_role.lua
-- @brief 角色相关脚本
-- @author fergus <zfengzhen@gmail.com>
-- @version 
-- @date 2014-05-03
----------------------------------------

-- 登录
function login(task_id, uid, password_hash, seq, player_idx, res_cmd)
    LOG_INFO("task_id: %s", tostring(task_id))
    LOG_INFO("player_idx: %s", tostring(player_idx))
    LOG_INFO("res_cmd: %s", tostring(res_cmd))

    -- 必须用冒号表达式
    local player = OBJ_MGR_MODULE:get_player(player_idx)
    if (player == nil) then
        LOG_INFO("%s", "player is nil")
        player:send_failed_cs_res(seq, res_cmd, -1)
    end

    local old_player_idx = OBJ_MGR_MODULE:find_player_idx_by_uid(uid)
    if (old_player_idx > 0) then
        -- 踢下线
        local old_player = OBJ_MGR_MODULE:get_player(old_player_idx)
        old_player:send_server_kick_off_notify()
        local update_player_data_flag = 1
        local update_count = 0
        while (update_player_data_flag ~= 0 and update_count < 5) do
            old_player:do_update_player_data(task_id)
            update_player_data_flag = coroutine.yield()
        end

        OBJ_MGR_MODULE:del_player(old_player_idx)

        if (update_count >= 5) then
            player:send_failed_cs_res(seq, res_cmd, -1)
            return
        end
    end

    player:do_get_player_data(task_id, uid, password_hash);

    local get_player_data_flag = coroutine.yield()
    if (get_player_data_flag ~= 0) then
        player:send_failed_cs_res(seq, res_cmd, -1)
        return
    end

    player:set_uid(uid)
    player:set_password_hash(password_hash)

    player:send_ok_cs_login_res(seq)
    player:send_role_info_notify()

    return
end
