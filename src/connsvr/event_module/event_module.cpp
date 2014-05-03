/**
 * @file event_module.cpp
 * @brief 事件处理模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "event_module.h"
#include "app.h"
#include "config_module.h"
#include "conn_mgr_module.h"
#include "cs_msg.pb.h"

EventModule::EventModule(App* app)
	: AppModuleBase(app),
      zmq_ctx_(NULL),
	  zmq_sock_(NULL),
      zmq_pair_fd_(0),
      listen_fd_(0)
{}

EventModule::~EventModule()
{}

void EventModule::ModuleInit()
{
    // TCP 初始化
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    PCHECK(listen_fd_ > 0)
        << "socket error!";
    Epoller::SetNonBlock(listen_fd_);
    Epoller::SetSocketOpt(listen_fd_);

    ConfigModule* conf_module = FindModule<ConfigModule>(app_);
    int32_t listen_port = conf_module->listen_port();

    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 0;
    sin.sin_port = htons(listen_port);

    CHECK(bind(listen_fd_, (struct sockaddr*)&sin, sizeof(sin)) ==  0)
        << "bind error!";

    CHECK(listen(listen_fd_, 32) ==  0)
        << "listen error!";

    // ZMQ初始化
    zmq_ctx_ = zmq_init(1);
    PCHECK(zmq_ctx_ != NULL)
        << "zmq_init error!";
    zmq_sock_ = zmq_socket(zmq_ctx_, ZMQ_PAIR);
    PCHECK(zmq_sock_ != NULL)
        << "zmq_socket error!";
    PCHECK(zmq_connect(zmq_sock_, conf_module->gamesvr_zmq_addr()) == 0)
        << "zmq_connect error!";

    // 初始化 Epoller
    int32_t conn_pool_size = conf_module->conn_pool_size();
    epoller_ = new Epoller();
    epoller_->Init(conn_pool_size);
    
    // 增加tcp accept事件
    int ret = epoller_->AddEvent(listen_fd_, EVENT_READ, EventModule::DoTcpAccept, (void*)epoller_);
    CHECK(ret == 0)
        << "epoller_.AddEvent error.";

	LOG(INFO) << ModuleName() << " init ok!";
}

void EventModule::ModuleFini()
{
    // fd 释放
    close(listen_fd_);

    // zmq 释放
    zmq_close(zmq_sock_);
    zmq_term(zmq_ctx_);

    delete(epoller_);

	LOG(INFO) << ModuleName() << " fini completed!";
}

const char* EventModule::ModuleName() const
{
	static const std::string ModuleName = "EventModule";
	return ModuleName.c_str();
}

int32_t EventModule::ModuleId()
{
	return MODULE_ID_EVENT;
}

AppModuleBase* EventModule::CreateModule(App* app)
{
	EventModule* event_module = new EventModule(app);
	if (event_module != NULL) {
        event_module->ModuleInit();
	}

	return static_cast<AppModuleBase*>(event_module);
}

void EventModule::Run()
{
    int32_t loop_times = LOOP_TIMES;
    while (loop_times--) {
        epoller_->Loop();
        ZmqReadLoop();
    }
}

void EventModule::DoTcpAccept(int listen_fd, void* arg)
{
    Epoller* epoller_ = (Epoller*)arg;
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);

    ConfigModule* conf_module = FindModule<ConfigModule>(App::GetInstance());
    int32_t max_connection = conf_module->conn_pool_size();

    ConnMgrModule* conn_mgr_module = FindModule<ConnMgrModule>(App::GetInstance());
    int32_t cur_conn_map_size = conn_mgr_module->GetCurConnMapSize();

    if (cur_conn_map_size >= max_connection)
        return;

    int32_t conn_fd = accept(listen_fd, (struct sockaddr*)&sin, &len);
    if (conn_fd < 0) {
        if (errno != EAGAIN && errno != ECONNABORTED
            && errno != EPROTO && errno != EINTR) {
            PLOG(ERROR) << "accept";
        }
        return;
    }

    Epoller::SetNonBlock(conn_fd);
    Epoller::SetSocketOpt(conn_fd);

    Connection* conn = conn_mgr_module->CreateConn(conn_fd);
    conn->Init();
    if (conn == NULL) {
        LOG(ERROR) << "CreateConn error!";
        return;
    }

    LOG(INFO)
        << "accept ip[" << sin.sin_addr.s_addr
        << "] port[" << sin.sin_port
        << "] conn_fd[" << conn_fd
        << "]";

    int ret = epoller_->AddEvent(conn_fd, EVENT_READ, EventModule::DoTcpRead, (void*)epoller_);
    if (ret < 0)
        LOG(ERROR) << "epoller_->AddEvent error!";

    conn->set_conn_fd(conn_fd);
}

void EventModule::DoTcpRead(int conn_fd, void* arg)
{
    Epoller* epoller_ = (Epoller*)arg;
    static char recv_buf[PKG_BUF_SIZE] = {0};

    EventModule* event_module = FindModule<EventModule>(App::GetInstance());
    void* zmq_sock = event_module->zmq_sock();

    ConnMgrModule* conn_mgr_module = FindModule<ConnMgrModule>(App::GetInstance());
    Connection* conn = conn_mgr_module->GetConn(conn_fd);
    if (conn == NULL) {
        LOG(ERROR) << "GetConn error!";
        return;
    }

    int32_t recv_len = recv(conn_fd, recv_buf, PKG_BUF_SIZE, 0);
    if (recv_len < 0) {
        PLOG(ERROR)
            << "recv";
        conn_mgr_module->ReleaseConn(conn_fd);
        epoller_->ClearEvent(conn_fd);
        return;
    } else if(recv_len == 0) {
        LOG(INFO) << "client close connection!";
        SendConnStop(zmq_sock, conn->player_idx());
        conn_mgr_module->ReleaseConn(conn_fd);
        epoller_->ClearEvent(conn_fd);
        return;
    }

    LOG(INFO) << "-----> recv from client len[" << recv_len << "]";

    if (conn->AddRecvData(recv_buf, recv_len) != 0) {
        LOG(ERROR)
            << "conn_fd[" << conn_fd
            << "] over tcp buf";
        conn_mgr_module->ReleaseConn(conn_fd);
        epoller_->ClearEvent(conn_fd);
        return;
    }

    while (conn->IsRecvProtoReady()) {
        // 接收真实长度大于协议长度就进行处理
        int32_t ret = 0;
        const char* msg_buf = Utils::GetMsgFromClient(conn->recv_buf());
        int32_t msg_len = Utils::GetMsgLenFromClient(conn->recv_buf());

        static ProtoCs::Msg msg;        
        msg.Clear();

        // Protobuf解析
        if (msg.ParseFromArray(msg_buf, msg_len) == false) {
            LOG(ERROR)
                << "protobuf parse error! msg_len[" << msg_len << "]";
        } 

        if (msg_len > PKG_BUF_SIZE) {
            LOG(ERROR)
               << "msg_len > PKG_BUF_SIZE"; 
            return;
        }

        static ConnData conn_data;

        if (conn->player_idx() == 0)
            conn_data.conn_cmd = CONN_START;
        else
            conn_data.conn_cmd = CONN_PROC;

        conn_data.conn_fd = conn_fd;
        conn_data.player_idx = conn->player_idx();

        static char send_buf[PKG_BUF_SIZE + sizeof(conn_data)];

        // --- 组包 start ---
        char* p = send_buf;
        memcpy(p, &conn_data, sizeof(conn_data));
        p += sizeof(conn_data);

        memcpy(p, msg_buf, msg_len);
        p += msg_len;

        int32_t send_len = sizeof(conn_data) + msg_len;
        // --- 组包 end --- 

        ret = zmq_send(zmq_sock, send_buf, send_len, ZMQ_DONTWAIT);
        conn->RemoveRecvProto();

        LOG(INFO) << "zmq_send ret[" << ret << "]";
        if (ret < 0) {
            LOG(ERROR) << "zmq_send errno[" << errno << "] error[" << strerror(errno) << "]";
            conn_mgr_module->ReleaseConn(conn_fd);
            epoller_->ClearEvent(conn_fd);
            return;
        }
    }
}

void EventModule::SendConnStop(void* zmq_sock, int32_t player_idx)
{
    LOG(INFO) << "player_idx[" << player_idx << "] send CONN_STOP to gamesvr ";
    if (player_idx == 0)
        return;

    static ConnData conn_data;
    conn_data.conn_cmd = CONN_STOP;
    conn_data.conn_fd = 0;
    conn_data.player_idx = player_idx;

    zmq_send(zmq_sock, (void*)&conn_data, sizeof(conn_data), ZMQ_DONTWAIT);
}

void EventModule::ZmqReadLoop()
{
    int32_t ret = 0;
    static char recv_buf[PKG_BUF_SIZE];
    int32_t recv_len = zmq_recv(zmq_sock_, recv_buf, PKG_BUF_SIZE, ZMQ_DONTWAIT);

    if (recv_len >= (int32_t)sizeof(ConnData)) {
        const char* msg_buf = Utils::GetMsgFromConn(recv_buf);
        int32_t msg_len = Utils::GetMsgLenFromConn(recv_len);
        LOG(INFO) << "-----> recv from gamesvr len[" << recv_len << "]";
        static ConnData conn_data;
        memcpy((void*)&conn_data, recv_buf, sizeof(conn_data));

        int32_t conn_fd = conn_data.conn_fd;
        int32_t conn_cmd = conn_data.conn_cmd;
        int32_t player_idx = conn_data.player_idx;

        ConnMgrModule* conn_mgr_module = FindModule<ConnMgrModule>(App::GetInstance());
        Connection* conn = conn_mgr_module->GetConn(conn_fd);
        if (conn == NULL) {
            if (conn_cmd == CONN_STOP) {
                LOG(INFO) << "client has already been stoped!";
            } else {
                LOG(ERROR) << "GetConn error!";
            }
            return;
        }

        static char send_buf[PKG_BUF_SIZE + sizeof(uint16_t)];

        char* p = send_buf;
        *(uint16_t*)p = htons(sizeof(uint16_t) + msg_len);
        int32_t send_len = sizeof(uint16_t) + msg_len;
        if (msg_len > PKG_BUF_SIZE) {
            LOG(ERROR)
               << "msg_len > PKG_BUF_SIZE"; 
            return;
        }
        p += sizeof(uint16_t);
        memcpy(p, msg_buf, msg_len);
        p += msg_len;

        if (conn->send_buf_size() == 0) {
            // 1 send_buf 没有数据直接发送
            int ret = send(conn_fd, send_buf, send_len, 0);
            // 错误处理
            if (ret < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 1.1 处理EAGAIN 或 EWOULDBLOCK
                    // 添加写事件
                    if (epoller_->AddEvent(conn_fd, EVENT_WRITE, EventModule::DoTcpWrite, NULL) != 0) {
                        LOG(ERROR) 
                            << "epoller_->AddEvent error.";    
                    }
                } else {
                    // 其他错误关闭连接
                    LOG(ERROR)
                        << "send errno[" << errno
                        << "] error[" << strerror(errno)
                        << "] conn_fd[" << conn_fd
                        << "]";
                    conn_mgr_module->ReleaseConn(conn_fd);
                    epoller_->ClearEvent(conn_fd);
                    return;
                }
            } else if (ret >= 0 && ret < send_len) {
                // 1.2 发送部分情况
                // 1.1.1 增加写事件
                LOG(INFO)
                    << "create write event";

                if (epoller_->AddEvent(conn_fd, EVENT_WRITE, EventModule::DoTcpWrite, (void*)epoller_) != 0) {
                    LOG(ERROR) 
                        << "epoller_->AddEvent error.";    
                }

                LOG(INFO)
                    << "part send\n"
                    << "remain_send_buf_len[" << conn->GetRemainSendBufSize()
                    << "]";
                if (conn->AddSendData(send_buf + ret, send_len - ret) != 0 ) {
                    LOG(ERROR)
                        << "send_buf overload.";
                    conn_mgr_module->ReleaseConn(conn_fd);
                    epoller_->ClearEvent(conn_fd);
                    return;
                }
            }
        } else {
            // 2 send_buf 有数据的话, 将发送数据追加到send_buf
            if (conn->AddSendData(send_buf + ret, send_len - ret) != 0 ) {
                LOG(ERROR)
                    << "send_buf overload.";
                conn_mgr_module->ReleaseConn(conn_fd);
                epoller_->ClearEvent(conn_fd);
                return;
            }
        }

        if (conn_cmd == CONN_PROC && conn->player_idx() == 0) {
            conn->set_player_idx(player_idx);
        } else if (conn_cmd == CONN_START) {
            LOG(INFO) << "recv gamesvr CONN_START";
            conn->set_player_idx(player_idx);
        } else if (conn_cmd == CONN_STOP) {
            LOG(INFO) << "recv gamesvr CONN_STOP";
            conn_mgr_module->ReleaseConn(conn_fd);
            epoller_->ClearEvent(conn_fd);
            return;
        }

    } else if (recv_len == -1) {
        return;
    }
}

void EventModule::DoTcpWrite(int conn_fd, void* arg)
{
    LOG(INFO) << "do_tcp_write";
    Epoller* epoller_ = (Epoller*)arg;
    ConnMgrModule* conn_mgr_module = FindModule<ConnMgrModule>(App::GetInstance());
    Connection* conn = conn_mgr_module->GetConn(conn_fd);
    if (conn == NULL) {
        LOG(INFO)
            << "Connection fd[" << conn_fd
            << "] has been released.";
        return;
    }

    int ret = send(conn_fd, conn->send_buf(), conn->send_buf_size(), 0);
    if (ret < 0) {
        LOG(ERROR)
            << "send errno[" << errno
            << "] error[" << strerror(errno)
            << "] conn_fd[" << conn_fd
            << "]";
        conn_mgr_module->ReleaseConn(conn_fd);
        epoller_->ClearEvent(conn_fd);
        return;
    } else if (ret > 0 && ret < conn->send_buf_size()) {
        LOG(INFO) << "tcp send part";
        conn->RemoveSendData(ret);
    } else if (ret == conn->send_buf_size()) {
        LOG(INFO) << "tcp send all send_len[" << ret << "]";
        conn->ClearSendData();
        epoller_->DelEvent(conn_fd, EVENT_WRITE);
    }
}
