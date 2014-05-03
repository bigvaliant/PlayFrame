/**
 * @file player.cpp
 * @brief Player对象
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "player.h"
#include "app.h"
#include "msg_module.h"
#include "obj_mgr_module.h"

void Player::set_uid(uint64_t uid)
{
    ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(App::GetInstance());
    int32_t player_idx = get_player_idx();
    obj_mgr_module->bind_uid_to_player_idx(uid, player_idx);
    m_uid = uid;
}

int32_t Player::get_player_idx()
{
    ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(App::GetInstance());
    int32_t player_idx = obj_mgr_module->get_player_idx(this);
    return player_idx;
}

void Player::set_player_by_db(const Database::RoleInfo* role_info)
{
    m_money = role_info->money();
    m_level = role_info->level();
}

void Player::do_update_player_data(uint64_t seq)
{
    LOG(INFO) << "do_update_player_data";
    ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(App::GetInstance());
    int32_t player_idx = get_player_idx();

    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    ProtoSs::Msg msg;
    ProtoSs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoSs::Msg::kSetPlayerDataReqFieldNumber);
    head->set_seq(seq);
    head->set_player_idx(player_idx);
    ProtoSs::SetPlayerDataReq* set_player_data_req = msg.mutable_set_player_data_req();
    set_player_data_req->set_uid(m_uid);

    // 玩家数据组装
    Database::PlayerData player_data;
    Database::RoleInfo* role_info = player_data.mutable_role_info();
    role_info->set_money(m_money);
    role_info->set_level(m_level);

    for (int32_t i = 1; i < MAX_CITY_NUM_PER_PLAYER; i++) {
        if (m_city_list[i] != 0) {
            Database::CityInfo* city_info = player_data.add_city_list();
            City* city = obj_mgr_module->get_city(m_city_list[i]);
            city_info->set_population(city->m_population);
        }
    }

    static char player_data_buf[PLAYER_DATA_MAX] = {0};
    if (player_data.SerializeToArray(player_data_buf, PLAYER_DATA_MAX) == false) {
        LOG(ERROR) 
            << "player_data SerializeToArray error!";
        return;
    };

    set_player_data_req->set_player_data(player_data_buf, player_data.ByteSize());

    msg_module->SendToDatasvr(&msg);
}

void Player::do_get_player_data(uint64_t seq, uint64_t uid, uint64_t password_hash)
{
    LOG(INFO) << "do_get_player_data";
    int32_t player_idx = get_player_idx();
    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    ProtoSs::Msg msg;
    ProtoSs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoSs::Msg::kGetPlayerDataReqFieldNumber);
    head->set_seq(seq);
    head->set_player_idx(player_idx);
    ProtoSs::GetPlayerDataReq* get_player_data_req
        = msg.mutable_get_player_data_req();
    get_player_data_req->set_uid(uid);
    get_player_data_req->set_password_hash(password_hash);

    msg_module->SendToDatasvr(&msg);
}

void Player::do_account_verify(uint64_t seq, uint64_t uid, uint64_t password_hash)
{
    LOG(INFO) << "do_account_verify";
    int32_t player_idx = get_player_idx();
    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    ProtoSs::Msg msg;
    ProtoSs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoSs::Msg::kAccountVerifyReqFieldNumber);
    head->set_seq(seq);
    head->set_player_idx(player_idx);
    ProtoSs::AccountVerifyReq* account_verify_req = msg.mutable_account_verify_req();
    account_verify_req->set_uid(uid);
    account_verify_req->set_password_hash(password_hash);

    msg_module->SendToDatasvr(&msg);
}

void Player::do_account_reg(uint64_t seq, uint64_t uid, uint64_t password_hash, const char* account)
{
    LOG(INFO) << "do_account_req";
    int32_t player_idx = get_player_idx();
    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    ProtoSs::Msg msg;
    ProtoSs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoSs::Msg::kAccountRegReqFieldNumber);
    head->set_seq(seq);
    head->set_player_idx(player_idx);
    ProtoSs::AccountRegReq* account_reg_req = msg.mutable_account_reg_req();
    account_reg_req->set_uid(uid);
    account_reg_req->set_password_hash(password_hash);
    if (account != NULL)
        account_reg_req->set_account(account);

    // 玩家初始化数据
    Database::PlayerData player_data;
    Database::RoleInfo* role_info = player_data.mutable_role_info();
    Database::CityInfo* city_info = player_data.add_city_list();
    role_info->set_money(1000);
    role_info->set_level(1);
    city_info->set_id(1121);

    static char player_data_buf[PLAYER_DATA_MAX] = {0};
    if (player_data.SerializeToArray(player_data_buf, PLAYER_DATA_MAX) == false) {
        LOG(ERROR) 
            << "player_data SerializeToArray error!";
        return;
    };

    account_reg_req->set_player_data(player_data_buf, player_data.ByteSize());

    msg_module->SendToDatasvr(&msg);
}

void Player::send_failed_cs_res(uint64_t seq, int32_t cmd, int32_t err_ret)
{
    LOG(INFO) << "send_failed_cs_res";
    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    ProtoCs::Msg msg;
    ProtoCs::Head* head = msg.mutable_head();
    head->set_cmd(cmd);
    head->set_ret(err_ret);
    head->set_seq(seq);

    if (cmd == ProtoCs::Msg::kLoginResFieldNumber) 
        msg_module->SendCloseToConnsvr(&msg, get_conn_fd(), get_player_idx());
    else 
        msg_module->SendStartToConnsvr(&msg, get_conn_fd(), get_player_idx());
}

void Player::send_ok_cs_quick_reg_res(uint64_t seq, uint64_t uid, const char* password)
{
    LOG(INFO) << "send_ok_cs_quick_reg_res";
    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    ProtoCs::Msg msg;
    ProtoCs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoCs::Msg::kQuickRegResFieldNumber);
    head->set_ret(0);
    head->set_seq(seq);
    ProtoCs::QuickRegRes* quick_reg_res = msg.mutable_quick_reg_res();

    std::string uid_str;
    Utils::ToString(uid_str, uid);
    quick_reg_res->set_account(uid_str);
    quick_reg_res->set_password(password);

    msg_module->SendStartToConnsvr(&msg, get_conn_fd(), get_player_idx());
}

void Player::send_ok_cs_normal_reg_res(uint64_t seq, const char* account, const char* password)
{
    LOG(INFO) << "send_ok_cs_normal_reg_res";
    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    ProtoCs::Msg msg;
    ProtoCs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoCs::Msg::kNormalRegResFieldNumber);
    head->set_ret(0);
    head->set_seq(seq);
    ProtoCs::NormalRegRes* normal_reg_res = msg.mutable_normal_reg_res();

    normal_reg_res->set_account(account);
    normal_reg_res->set_password(password);

    msg_module->SendStartToConnsvr(&msg, get_conn_fd(), get_player_idx());
}

void Player::send_ok_cs_login_res(uint64_t seq)
{
    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    ProtoCs::Msg msg;
    ProtoCs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoCs::Msg::kLoginResFieldNumber);
    head->set_ret(0);
    head->set_seq(seq);
    ProtoCs::LoginRes* login_res = msg.mutable_login_res();
    login_res->set_numb(1222);

    msg_module->SendStartToConnsvr(&msg, get_conn_fd(), get_player_idx());
}

void Player::send_role_info_notify()
{
    LOG(INFO) << "send_role_info_notify";
    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    ProtoCs::Msg msg;
    ProtoCs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoCs::Msg::kRoleInfoNtfFieldNumber);
    head->set_ret(0);
    head->set_seq(0);
    ProtoCs::RoleInfoNtf* role_info_ntf = msg.mutable_role_info_ntf();
    role_info_ntf->set_money(m_money);
    role_info_ntf->set_level(m_level);

    msg_module->SendProcToConnsvr(&msg, get_conn_fd(), get_player_idx());
}

void Player::send_server_kick_off_notify()
{
    LOG(INFO) << "send_server_kick_off_notify";
    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    ProtoCs::Msg msg;
    ProtoCs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoCs::Msg::kServerKickOffNtfFieldNumber);
    head->set_ret(0);
    head->set_seq(0);

    msg_module->SendCloseToConnsvr(&msg, get_conn_fd(), get_player_idx());
}
