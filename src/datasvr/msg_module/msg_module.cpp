/**
 * @file msg_module.cpp
 * @brief 消息模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "msg_module.h"
#include "app.h"
#include "config_module.h"
#include "mysql_module.h"

MsgModule::MsgModule(App* app)
	: AppModuleBase(app),
      gamesvr_zmq_sock_(NULL)
{}

MsgModule::~MsgModule()
{}

void MsgModule::ModuleInit()
{
    ConfigModule* conf_module = FindModule<ConfigModule>(app_);
    // ZMQ初始化
    zmq_ctx_  = zmq_init(1);
    PCHECK(zmq_ctx_ != NULL)
        << "zmq_init error!";
    gamesvr_zmq_sock_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
    PCHECK(gamesvr_zmq_sock_ != NULL)
        << "zmq_socket error!";
    PCHECK(zmq_bind(gamesvr_zmq_sock_, conf_module->gamesvr_zmq_addr()) == 0)
        << "zmq_bind error!";

    // 注册消息处理函数
    REGISTER_MSG_BEGIN(MsgModule, ProtoSs::Msg)
        REGISTER_MSG(this, ProtoSs::Msg::kAccountRegReqFieldNumber, &MsgModule::OnAccountRegReq)
        REGISTER_MSG(this, ProtoSs::Msg::kAccountVerifyReqFieldNumber, &MsgModule::OnAccountVerifyReq)
        REGISTER_MSG(this, ProtoSs::Msg::kGetPlayerDataReqFieldNumber, &MsgModule::OnGetPlayerDataReq)
        REGISTER_MSG(this, ProtoSs::Msg::kSetPlayerDataReqFieldNumber, &MsgModule::OnSetPlayerDataReq)
    REGISTER_MSG_END;

	LOG(INFO) << ModuleName() << " init ok!";
}

void MsgModule::ModuleFini()
{
    // zmq 释放
    zmq_close(gamesvr_zmq_sock_);
    zmq_term(zmq_ctx_);

	LOG(INFO) << ModuleName() << " fini completed!";
}

const char* MsgModule::ModuleName() const
{
	static const std::string ModuleName = "MsgModule";
	return ModuleName.c_str();
}

int32_t MsgModule::ModuleId()
{
	return MODULE_ID_MSG;
}

AppModuleBase* MsgModule::CreateModule(App* app)
{
	MsgModule* module = new MsgModule(app);
	if (module != NULL) {
        module->ModuleInit();
	}

	return static_cast<AppModuleBase*>(module);
}

void MsgModule::Run()
{
    int32_t loop_times = 1000;
    static char buf[PKG_BUF_SIZE];
    int32_t len = 0;
    MysqlModule* mysql_module = FindModule<MysqlModule>(app_);
    while (loop_times--) {
        // 处理gamesvr消息
        MysqlClient* mysql_client = NULL;
        mysql_client = mysql_module->PopFreeClient();
        if (mysql_client == NULL) {
            LOG(ERROR) << "mysql client used up!";
            usleep(10*1000);
            continue;
        }
        len = zmq_recv(gamesvr_zmq_sock_, buf, 1024, ZMQ_DONTWAIT);
        if (len > 0) {
            LOG(INFO) << "recv msg";
            HandleRequest<ProtoSs::Msg>(buf, len, mysql_client);
        } else {
            mysql_module->PushFreeClient(mysql_client);
        }
    }
}

int32_t MsgModule::OnAccountRegReq(ProtoSs::Msg* msg, void* args)
{
    UNUSE_ARG(args);
    MysqlClient* mysql_client = (MysqlClient*)args;
    LOG(INFO) << "OnAccountRegReq";

    const ProtoSs::AccountRegReq& account_reg_req = msg->account_reg_req();
    uint64_t seq = msg->head().seq();
    int32_t player_idx = msg->head().player_idx();
    uint64_t uid = account_reg_req.uid();
    uint64_t password_hash = account_reg_req.password_hash();
    LOG(INFO)
        << "seq[" << seq
        << "] uid[" << uid
        << "] passsword_hash[" << password_hash
        << "] account[" << account_reg_req.account()
        << "]";

    std::ostringstream str_sql;
    if (account_reg_req.has_account() == true && account_reg_req.account().size() != 0) {
        str_sql << "INSERT INTO sg_data (uid, password_hash, account, reg_time, player_data) VALUES("
                << uid << ","
                << password_hash << ", \""
                << MysqlModule::ConvertBinToString(
                        mysql_client->GetConn(),
                        account_reg_req.account().c_str(),
                        account_reg_req.account().size()) << "\","
                << "NOW(), \""
                << MysqlModule::ConvertBinToString(
                        mysql_client->GetConn(),
                        account_reg_req.player_data().c_str(),
                        account_reg_req.player_data().size()) << "\")";
    } else {
        str_sql << "INSERT INTO sg_data (uid, password_hash, reg_time, player_data) VALUES("
                << uid << ","
                << password_hash << ", "
                << "NOW(), \""
                << MysqlModule::ConvertBinToString(
                        mysql_client->GetConn(),
                        account_reg_req.player_data().c_str(),
                        account_reg_req.player_data().size()) << "\")";
    }

    mysql_client->Process(str_sql.str().c_str(), &MsgModule::AccountRegMysqlRes, seq, player_idx);

    return 0;
}

void MsgModule::AccountRegMysqlRes(MYSQL_RES* res,
    int32_t flags, uint64_t seq_id, int32_t player_idx)
{
    (void)res;
    ProtoSs::Msg msg;

    ProtoSs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoSs::Msg::kAccountRegResFieldNumber);
    head->set_seq(seq_id);
    head->set_player_idx(player_idx);
    ProtoSs::AccountRegRes* account_reg_res = msg.mutable_account_reg_res();
    account_reg_res->set_numb(1);

    if (flags == MYSQL_RES_OK) {
        LOG(INFO) << "AccountRegMysqlRes ok!";
        head->set_ret(ProtoSs::RET_ACCOUNT_REG_OK);
    } else if (flags == MYSQL_RES_DUP) {
        LOG(INFO) << "AccountRegMysqlRes dup!";
        head->set_ret(ProtoSs::RET_ACCOUNT_REG_DUP);
    } else {
        LOG(INFO) << "AccountRegMysqlRes failed!";
        head->set_ret(ProtoSs::RET_ACCOUNT_REG_FAILED);
    }

    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    msg_module->SendToGamesvr(&msg);
}

int32_t MsgModule::OnAccountVerifyReq(ProtoSs::Msg* msg, void* args)
{
    UNUSE_ARG(args);
    MysqlClient* mysql_client = (MysqlClient*)args;
    LOG(INFO) << "OnAccountVerifyReq";

    const ProtoSs::AccountVerifyReq& account_verify_req = msg->account_verify_req();
    uint64_t seq = msg->head().seq();
    int32_t player_idx = msg->head().player_idx();
    uint64_t uid = account_verify_req.uid();
    uint64_t password_hash = account_verify_req.password_hash();
    LOG(INFO) << "seq[" << seq << "] uid[" << uid  << "] passsword_hash[" << password_hash << "]";

    char sql_buf[1024] = {0};
    snprintf(sql_buf, 1024,
        "SELECT uid FROM sg_data WHERE uid = %lu and password_hash = %lu",
         uid, password_hash);

    mysql_client->Process(sql_buf, &MsgModule::AccountVerifyMysqlRes, seq, player_idx);

    return 0;
}


void MsgModule::AccountVerifyMysqlRes(MYSQL_RES* res,
    int32_t flags, uint64_t seq_id, int32_t player_idx)
{
    (void)res;
    ProtoSs::Msg msg;

    ProtoSs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoSs::Msg::kAccountVerifyResFieldNumber);
    head->set_seq(seq_id);
    head->set_seq(player_idx);
    ProtoSs::AccountVerifyRes* account_verify_res = msg.mutable_account_verify_res();
    account_verify_res->set_numb(1);

    if (flags == MYSQL_RES_OK) {
        LOG(INFO) << "AccountVerifyMysqlRes ok!";
        head->set_ret(ProtoSs::RET_ACCOUNT_VERIFY_OK);
    } else {
        LOG(INFO) << "AccountVerifyMysqlRes failed!";
        head->set_ret(ProtoSs::RET_ACCOUNT_VERIFY_FAILED);
    }
    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    msg_module->SendToGamesvr(&msg);
}

int32_t MsgModule::OnGetPlayerDataReq(ProtoSs::Msg* msg, void* args)
{
    UNUSE_ARG(args);
    MysqlClient* mysql_client = (MysqlClient*)args;
    LOG(INFO) << "OnGetPlayerDataReq";

    const ProtoSs::GetPlayerDataReq& get_player_data_req = msg->get_player_data_req();
    uint64_t seq = msg->head().seq();
    int32_t player_idx = msg->head().player_idx();
    uint64_t uid = get_player_data_req.uid();
    uint64_t password_hash = get_player_data_req.password_hash();
    LOG(INFO) << "seq[" << seq << "] uid[" << uid  << "] passsword_hash[" << password_hash << "]";

    std::ostringstream str_sql;
    str_sql << "SELECT player_data FROM sg_data WHERE uid = " << uid << " AND password_hash = " << password_hash;

    mysql_client->Process(str_sql.str().c_str(), &MsgModule::GetPlayerDataMysqlRes, seq, player_idx);

    return 0;
}


void MsgModule::GetPlayerDataMysqlRes(MYSQL_RES* res,
    int32_t flags, uint64_t seq_id, int32_t player_idx)
{
    (void)res;
    ProtoSs::Msg msg;

    ProtoSs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoSs::Msg::kGetPlayerDataResFieldNumber);
    head->set_seq(seq_id);
    head->set_player_idx(player_idx);

    if (flags == MYSQL_RES_OK) {
        LOG(INFO) << "GetPlayerDataMysqlRes ok!";
        ProtoSs::GetPlayerDataRes* get_player_data_res = msg.mutable_get_player_data_res();
        MYSQL_ROW row;
        row = mysql_fetch_row(res);
        if (row == NULL) {
            LOG(INFO) << "GetPlayerDataMysqlRes failed!";
            head->set_ret(ProtoSs::RET_GET_PLAYER_DATA_FAILED);
        } else {
            // int num_fields = mysql_num_fields(res);
            unsigned long *lengths = mysql_fetch_lengths(res);
            if (lengths == NULL) {
                LOG(INFO) << "GetPlayerDataMysqlRes failed!";
                head->set_ret(ProtoSs::RET_GET_PLAYER_DATA_FAILED);
            } else {
                get_player_data_res->set_player_data(row[0], lengths[0]);
                head->set_ret(ProtoSs::RET_GET_PLAYER_DATA_OK);
            }
        }
    } else {
        LOG(INFO) << "GetPlayerDataMysqlRes failed!";
        head->set_ret(ProtoSs::RET_GET_PLAYER_DATA_FAILED);
    }
    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    msg_module->SendToGamesvr(&msg);
}

int32_t MsgModule::OnSetPlayerDataReq(ProtoSs::Msg* msg, void* args)
{
    UNUSE_ARG(args);
    MysqlClient* mysql_client = (MysqlClient*)args;
    LOG(INFO) << "OnSetPlayerDataReq";

    const ProtoSs::SetPlayerDataReq& set_player_data_req = msg->set_player_data_req();
    uint64_t seq = msg->head().seq();
    int32_t player_idx = msg->head().player_idx();
    uint64_t uid = set_player_data_req.uid();
    LOG(INFO) << "seq[" << seq << "] uid[" << uid  << "]";

    std::ostringstream str_sql;
    str_sql
        << "UPDATE sg_data SET player_data=\""
        << MysqlModule::ConvertBinToString(
                mysql_client->GetConn(),
                set_player_data_req.player_data().c_str(),
                set_player_data_req.player_data().size()) << "\""
        << "WHERE uid = " << uid;

    mysql_client->Process(str_sql.str().c_str(), &MsgModule::SetPlayerDataMysqlRes, seq, player_idx);

    return 0;
}


void MsgModule::SetPlayerDataMysqlRes(MYSQL_RES* res,
    int32_t flags, uint64_t seq_id, int32_t player_idx)
{
    (void)res;
    ProtoSs::Msg msg;

    ProtoSs::Head* head = msg.mutable_head();
    head->set_cmd(ProtoSs::Msg::kSetPlayerDataResFieldNumber);
    head->set_seq(seq_id);
    head->set_player_idx(player_idx);

    if (flags == MYSQL_RES_OK) {
        LOG(INFO) << "SetPlayerDataMysqlRes ok!";
        head->set_ret(ProtoSs::RET_SET_PLAYER_DATA_OK);
    } else {
        LOG(INFO) << "SetPlayerDataMysqlRes failed!";
        head->set_ret(ProtoSs::RET_SET_PLAYER_DATA_FAILED);
    }
    MsgModule* msg_module = FindModule<MsgModule>(App::GetInstance());
    msg_module->SendToGamesvr(&msg);
}
