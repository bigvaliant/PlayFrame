/**
 * @file player.h
 * @brief Player对象
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _PLAYER_H_
#define _PLAYER_H_

#include "app_def.h"

#define PLAYER_DATA_MAX 20480
#define MAX_CITY_NUM_PER_PLAYER 100

class Player
{
public:
    friend class ObjMgrModule;

    Player() {}
    ~Player() {}
    
    void init() { memset(this, 0, sizeof(*this)); }

    void set_conn_fd(int32_t conn_fd) { m_conn_fd = conn_fd; }
    int32_t get_conn_fd() { return m_conn_fd; }

    int32_t get_player_idx();

    // 设置了uid 才算真正建立了Player
    void set_uid(uint64_t uid);
    uint64_t get_uid() { return m_uid; }

    void set_password_hash(uint64_t password_hash) { m_password_hash = password_hash; }
    uint64_t get_password_hash() { return m_password_hash; } 

    // 通过protobuf描述的blob数据初始化Player
    void set_player_by_db(const Database::RoleInfo* role_info);

    void do_update_player_data(uint64_t seq = 0);

    // 客户端错误回包
    void send_failed_cs_res(uint64_t seq, int32_t cmd, int32_t err_ret);

    // notify
    void send_role_info_notify();
    void send_server_kick_off_notify();

    // 客户端成功回包
    void send_ok_cs_quick_reg_res(uint64_t seq, uint64_t uid, const char* password);
    void send_ok_cs_normal_reg_res(uint64_t seq, const char* account, const char* password);
    void send_ok_cs_login_res(uint64_t seq);

    // datasvr 操作
    void do_account_reg(uint64_t seq, uint64_t uid, uint64_t password_hash, const char* account);
    void do_account_verify(uint64_t seq, uint64_t uid, uint64_t password);
    void do_get_player_data(uint64_t seq, uint64_t uid, uint64_t password);

private:
    int32_t         m_conn_fd;
    uint64_t        m_uid;
    uint64_t        m_password_hash;

    int32_t         m_city_list[MAX_CITY_NUM_PER_PLAYER + 1];

    // role_info 注意跟database.proto保持一致
    int32_t         m_money;
    int32_t         m_level;
    // ---------------------------------
};

#endif // _PLAYER_H_
