----------------------------------------
-- @file task_role.lua
-- @brief 角色相关脚本
-- @author fergus <zfengzhen@gmail.com>
-- @version 
-- @date 2014-05-03
----------------------------------------

-- 登录
function login()
    local task_id = TASK_ID
    local rand_id = RAND_ID
    local timer_id = TIMER_ID
    local uid = UID
    local password_hash = PASSWORD_HASH
    local seq = SEQ
    local player_idx = PLAYER_IDX
    local res_cmd = RES_CMD

    -- 必须用冒号表达式
    local player = OBJ_MGR_MODULE:get_player(player_idx)
    if (player == nil) then
        LOG_INFO("player is nil")
        player:send_failed_cs_res(seq, res_cmd, -1)
    end

    local old_player_idx = OBJ_MGR_MODULE:find_player_idx_by_uid(uid)
    if (old_player_idx > 0) then
        -- 踢下线
        local old_player = OBJ_MGR_MODULE:get_player(old_player_idx)
        old_player:send_server_kick_off_notify()
        UPDATE_PLAYER_DATA_FLAG = 1
        local update_count = 0
        while (UPDATE_PLAYER_DATA_FLAG ~= 0 and update_count < 5) do
            old_player:do_update_player_data(task_id)

            local ret = CO_YIELD(rand_id)
            if (ret ~= 0) then
                player:send_failed_cs_res(seq, res_cmd, -1)
                return
            end
        end

        OBJ_MGR_MODULE:del_player(old_player_idx)

        if (update_count >= 5) then
            player:send_failed_cs_res(seq, res_cmd, -1)
            return
        end
    end

    player:do_get_player_data(task_id, uid, password_hash);

    local ret = CO_YIELD(rand_id)
    if (ret ~= 0) then
        player:send_failed_cs_res(seq, res_cmd, -1)
        return
    end

    if (PLAYER_DATA_FLAG ~= 0) then
        player:send_failed_cs_res(seq, res_cmd, -1)
        return
    end

    player:set_uid(uid)
    player:set_password_hash(password_hash)

    player:send_ok_cs_login_res(seq)
    player:send_role_info_notify()

    return
end
