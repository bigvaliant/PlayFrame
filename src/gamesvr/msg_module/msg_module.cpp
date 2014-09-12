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
#include "obj_mgr_module.h"
#include "lua_engine_module.h"

MsgModule::MsgModule(App* app)
	: AppModuleBase(app),
      zmq_ctx_(NULL),
      connsvr_zmq_sock_(NULL),
      datasvr_zmq_sock_(NULL)
     
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
    connsvr_zmq_sock_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
    PCHECK(connsvr_zmq_sock_ != NULL)
        << "zmq_socket error!";
    PCHECK(zmq_bind(connsvr_zmq_sock_, conf_module->GetConnsvrZmqAddr()) == 0)
        << "zmq_bind error!";

    datasvr_zmq_sock_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
    PCHECK(datasvr_zmq_sock_ != NULL)
        << "zmq_socket error!";
    PCHECK(zmq_connect(datasvr_zmq_sock_, conf_module->GetDatasvrZmqAddr()) == 0)
        << "zmq_connect error!";

    // 注册消息处理函数
    REGISTER_MSG_BEGIN(MsgModule, ProtoCs::Msg)
        REGISTER_MSG(this, ProtoCs::Msg::kQuickRegReqFieldNumber, &MsgModule::OnClientQuickRegReq)
        REGISTER_MSG(this, ProtoCs::Msg::kNormalRegReqFieldNumber, &MsgModule::OnClientNormalRegReq)
        REGISTER_MSG(this, ProtoCs::Msg::kLoginReqFieldNumber, &MsgModule::OnClientLoginReq)
    REGISTER_MSG_END;

    REGISTER_MSG_BEGIN(MsgModule, ProtoSs::Msg)
        REGISTER_MSG(this, ProtoSs::Msg::kAccountRegResFieldNumber, &MsgModule::OnDatasvrAccountRegRes)
        REGISTER_MSG(this, ProtoSs::Msg::kAccountVerifyResFieldNumber, &MsgModule::OnDatasvrAccountVerifyRes)
        REGISTER_MSG(this, ProtoSs::Msg::kGetPlayerDataResFieldNumber, &MsgModule::OnDatasvrGetPlayerDataRes)
        REGISTER_MSG(this, ProtoSs::Msg::kSetPlayerDataResFieldNumber, &MsgModule::OnDatasvrSetPlayerDataRes)
    REGISTER_MSG_END;

	LOG(INFO) << ModuleName() << " init ok!";
}

void MsgModule::ModuleFini()
{
    // zmq 释放
    if (connsvr_zmq_sock_ != NULL)
        zmq_close(connsvr_zmq_sock_);
    if (datasvr_zmq_sock_ != NULL)
        zmq_close(datasvr_zmq_sock_);
    if (zmq_ctx_ != NULL)
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

int32_t MsgModule::SendToConnsvr(ProtoCs::Msg* msg, int32_t conn_cmd, int32_t conn_fd, int32_t player_idx)
{
    static ConnData conn_data;
    conn_data.conn_cmd = conn_cmd;
    conn_data.player_idx = player_idx;
    conn_data.conn_fd = conn_fd;

    static char send_buf[PKG_BUF_SIZE + CONN_DATA_SIZE];

    // --- 组包 start ---
    char* p = send_buf;
    memcpy(p, &conn_data, CONN_DATA_SIZE);
    p += CONN_DATA_SIZE;

    if (msg != NULL) {
        if (msg->SerializeToArray(p, PKG_BUF_SIZE) == false) {
            printf("msg.SerializeToArray error.\n");
            return -1;
        }

        p += msg->ByteSize();
    }

    int32_t send_len = p - send_buf;
    // --- 组包 end --- 

    int32_t ret = zmq_send(connsvr_zmq_sock_, send_buf, send_len, ZMQ_DONTWAIT);
    if (ret < 0) {
        LOG(ERROR)
           << "zmq_send error."; 
        return -1;
    }
    return 0; 
}


void MsgModule::Run()
{
    ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(app_);
    
    int32_t loop_times = 1000;
    static char buf[PKG_BUF_SIZE];
    int32_t len = 0;
    while (loop_times--) {
        // 处理connsvr消息
        len = zmq_recv(connsvr_zmq_sock_, buf, 1024, ZMQ_DONTWAIT);
        if (len > 0) {
            static ConnData conn_data;
            memcpy((void*)&conn_data, buf, CONN_DATA_SIZE);

            const char* msg_buf = Utils::GetMsgFromConn(buf);
            int32_t msg_len = Utils::GetMsgLenFromConn(len);

            if (conn_data.conn_cmd == CONN_START) {
                // 分配内存对象 
                LOG(INFO) << "CONN_START";
                int32_t player_idx = obj_mgr_module->add_player();
                Player* player =  obj_mgr_module->get_player(player_idx);
                if (player == NULL) {
                    // 内存池满了
                    LOG(ERROR) << "create_player error!"; 

                    static ProtoCs::Msg msg;        
                    msg.Clear();
                    if (msg.ParseFromArray(msg_buf, msg_len) == false) {
                        LOG(ERROR)
                            << "protobuf parse error!";
                        return;
                    } 

                    int32_t cmd = msg.head().cmd();
                    if (cmd == ProtoCs::Msg::kLoginReqFieldNumber) {
                        msg.mutable_head()->set_cmd(ProtoCs::Msg::kLoginResFieldNumber);
                        msg.mutable_head()->set_ret(ProtoCs::RET_LOGIN_GAMESVR_FULL);
                    } else if (cmd == ProtoCs::Msg::kQuickRegReqFieldNumber) {
                        msg.mutable_head()->set_cmd(ProtoCs::Msg::kQuickRegResFieldNumber);
                        msg.mutable_head()->set_ret(ProtoCs::RET_QUICK_REG_GAMESVR_FULL);
                    } else if (cmd == ProtoCs::Msg::kNormalRegReqFieldNumber) {
                        msg.mutable_head()->set_cmd(ProtoCs::Msg::kNormalRegResFieldNumber);
                        msg.mutable_head()->set_ret(ProtoCs::RET_NORMAL_REG_GAMESVR_FULL);
                    }

                    SendCloseToConnsvr(&msg, conn_data.conn_fd, 0);
                    return;
                }
                conn_data.player_idx = player_idx;
                player->set_conn_fd(conn_data.conn_fd);
            } else if (conn_data.conn_cmd == CONN_STOP) {
                LOG(INFO) << "CONN_STOP";
                LOG(INFO)
                    << "player_idx[" << conn_data.player_idx
                    << "] client disconnected";
                Player* player =  obj_mgr_module->get_player(conn_data.player_idx);
                if (player != NULL) {
                    LOG(INFO) << "update_player_data";
                    player->do_update_player_data();
                } else {
                    LOG(ERROR) << "update_player_data get_player error";
                }
                obj_mgr_module->del_player(conn_data.player_idx);
                return;
            } else if (conn_data.conn_cmd == CONN_PROC) {
                LOG(INFO) << "CONN_PROC";
            }

            HandleRequest<ProtoCs::Msg>(msg_buf, msg_len, &conn_data);
        }
        // 处理datasvr消息
        len = zmq_recv(datasvr_zmq_sock_, buf, 1024, ZMQ_DONTWAIT);
        if (len > 0) {
            HandleRequest<ProtoSs::Msg>(buf, len, NULL);
        }
    }
}

int32_t MsgModule::OnClientQuickRegReq(ProtoCs::Msg* msg, void* arg)
{
    ConnData* conn_data = (ConnData*)arg;
    LuaEngineModule* lua_engine_module = FindModule<LuaEngineModule>(app_);
    uint64_t task_id = lua_engine_module->CreateTask("quick_reg", 10);
    if (task_id == 0) {
        LOG(ERROR) << "task_id = " << task_id << "error"; 
        return -1;
    }

    lua_engine_module->Resume(task_id, task_id, msg->head().seq(),
            conn_data->player_idx, ProtoCs::Msg::kQuickRegResFieldNumber);

    return 0;
}

int32_t MsgModule::OnClientNormalRegReq(ProtoCs::Msg* msg, void* arg)
{
    ConnData* conn_data = (ConnData*)arg;
    LuaEngineModule* lua_engine_module = FindModule<LuaEngineModule>(app_);
    uint64_t task_id = lua_engine_module->CreateTask("normal_reg", 10);
    if (task_id == 0) {
        LOG(ERROR) << "task_id = " << task_id << "error"; 
        return -1;
    }

    const ProtoCs::NormalRegReq& normal_reg_req = msg->normal_reg_req();

    lua_engine_module->Resume(task_id, task_id, normal_reg_req.account().c_str(),
            normal_reg_req.password().c_str(), msg->head().seq(),
            conn_data->player_idx, ProtoCs::Msg::kNormalRegResFieldNumber);

    return 0;
}

int32_t MsgModule::OnClientLoginReq(ProtoCs::Msg* msg, void* arg)
{
    ConnData* conn_data = (ConnData*)arg;
    const ProtoCs::LoginReq& login_req = msg->login_req();
    std::string account = login_req.account();
    int32_t ret = Utils::CheckAccount(account.c_str(), account.size());
    uint64_t uid = 0;
    if (ret == 0) {
        uid = Utils::Fnv64aHash(account.c_str(), account.size());
    } else {
        uid = Utils::ToNumber<uint64_t>(account);
    }

    uint64_t password_hash = Utils::Fnv64aHash(login_req.password().c_str(), 
            login_req.password().size());
    LOG(INFO)
        << "account[" << account
        << "] uid[" << uid 
        << "] password_hash[" << password_hash
        << "]";

    LuaEngineModule* lua_engine_module = FindModule<LuaEngineModule>(app_);
    uint64_t task_id = lua_engine_module->CreateTask("login", 10);
    if (task_id == 0) {
        LOG(ERROR) << "task_id = " << task_id << "error"; 
        return -1;
    }
    lua_engine_module->Resume(task_id, task_id, uid, password_hash,
            msg->head().seq(), conn_data->player_idx, 
            ProtoCs::Msg::kLoginResFieldNumber);

    return 0;
}

int32_t MsgModule::OnDatasvrAccountRegRes(ProtoSs::Msg* msg, void* arg)
{
    UNUSE_ARG(arg);
    LuaEngineModule* lua_engine_module = FindModule<LuaEngineModule>(app_);
    uint64_t task_id = msg->head().seq();
    int32_t account_reg_flag = msg->head().ret();
    LOG(INFO) << "account reg result[" << account_reg_flag << "]";

    lua_engine_module->Resume(task_id, account_reg_flag);

    return 0;
}

int32_t MsgModule::OnDatasvrAccountVerifyRes(ProtoSs::Msg* msg, void* arg)
{
    UNUSE_ARG(arg);
    LuaEngineModule* lua_engine_module = FindModule<LuaEngineModule>(app_);
    uint64_t task_id = msg->head().seq();
    int32_t ret = msg->head().ret();
    LOG(INFO) << "account verify result[" << ret << "]";

    if (ret == 0) {
        lua_engine_module->SetGlobal("ACCOUNT_VERIFY_FLAG", 0);
    } else {
        lua_engine_module->SetGlobal("ACCOUNT_VERIFY_FLAG", 1);
    }

    lua_engine_module->Resume(task_id, CHECK_SIG_OK);

    return 0;
}

int32_t MsgModule::OnDatasvrGetPlayerDataRes(ProtoSs::Msg* msg, void* arg)
{
    UNUSE_ARG(arg);
    LuaEngineModule* lua_engine_module = FindModule<LuaEngineModule>(app_);
    uint64_t task_id = msg->head().seq();
    int32_t ret = msg->head().ret();
    int32_t player_idx = msg->head().player_idx();
    LOG(INFO)
        << "get player_data result[" << ret
        << "] player_idx[" << player_idx
        << "] seq[" << task_id 
        << "]";

    if (ret == 0) {
        const ProtoSs::GetPlayerDataRes& get_player_data_res = msg->get_player_data_res();
        Database::PlayerData player_data;
        player_data.ParseFromArray(get_player_data_res.player_data().c_str(),
            get_player_data_res.player_data().size());
        LOG(INFO) << "\nplayer_data:\n" << player_data.Utf8DebugString();

        // 从数据库初始化对象
        ObjMgrModule* obj_mgr_module = FindModule<ObjMgrModule>(app_);
        Player* player = obj_mgr_module->get_player(player_idx); 
        const Database::RoleInfo& role_info 
            = player_data.role_info();
        player->set_player_by_db(&role_info);

        for (int32_t i = 0; i < player_data.city_list_size(); i++) {
            const Database::CityInfo& city_info = player_data.city_list(i);
            int32_t city_idx = obj_mgr_module->add_city(player_idx);
            City* city = obj_mgr_module->get_city(city_idx);
            city->set_city_by_db(&city_info);
            if (city != NULL) {
                city->set_player_idx(player_idx);
            }
        }
        
        lua_engine_module->Resume(task_id, 0);
    } else {
        lua_engine_module->Resume(task_id, 1);
    }


    return 0;
}

int32_t MsgModule::OnDatasvrSetPlayerDataRes(ProtoSs::Msg* msg, void* arg)
{
    UNUSE_ARG(arg);
    LuaEngineModule* lua_engine_module = FindModule<LuaEngineModule>(app_);
    uint64_t task_id = msg->head().seq();
    int32_t update_player_data_flag = msg->head().ret();
    int32_t player_idx = msg->head().player_idx();
    LOG(INFO)
        << "get player_data result[" << update_player_data_flag
        << "] player_idx[" << player_idx
        << "] seq[" << task_id 
        << "]";

    lua_engine_module->Resume(task_id, update_player_data_flag);

    return 0;
}
