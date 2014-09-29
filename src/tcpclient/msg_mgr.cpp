/**
 * @file msg_mgr.cpp
 * @brief 消息分发器
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include <stdio.h>
#include "msg_mgr.h"

int32_t MsgMgr::errno_ = ERR_UNCONNECT;

// 接收和发送缓冲区大小
const size_t SOCK_RECV_BUFFER = 512*1024;
const size_t SOCK_SEND_BUFFER = 512*1024;

int32_t MsgMgr::Init()
{
    if (sock_ > 0)
        CloseSock();

    sock_ = 0;
    errno_ = ERR_UNCONNECT;

    conn_cache_.Init();

	sock_ = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_ == INVALID_SOCKET) {
		return -1;
	}

    int ret = ConnectServer("10.211.55.22", 8810);
    if (ret != 0) {
        return -2;
    }

    free_seq_ = 10000;
    
    // 注册消息
    REGISTER_MSG(this, ProtoCs::Msg::kLoginReqFieldNumber, &MsgMgr::DoLogin,
        ProtoCs::Msg::kLoginResFieldNumber, &MsgMgr::OnLogin);
    REGISTER_MSG(this, ProtoCs::Msg::kNormalRegReqFieldNumber, &MsgMgr::DoNormalReg,
        ProtoCs::Msg::kNormalRegResFieldNumber, &MsgMgr::OnNormalReg);
    REGISTER_MSG(this, ProtoCs::Msg::kQuickRegReqFieldNumber, &MsgMgr::DoQuickReg,
        ProtoCs::Msg::kQuickRegResFieldNumber, &MsgMgr::OnQuickReg);

    return 0;
}

int32_t MsgMgr::DispatchReqMsg(int32_t cmd, void* data)
{
    static ProtoCs::Msg msg;
    static char send_buf[PKG_BUF_SIZE];
    msg.Clear();

    // 统一设置包头
    msg.mutable_head()->set_cmd(cmd); 
    msg.mutable_head()->set_seq(free_seq_++);

    MsgWrapperMap::iterator it = req_msg_wrapper_map_.find(cmd);
    if (it != req_msg_wrapper_map_.end()) {
        // 设置包体
        int ret = it->second->HandleReqMsg((void*)&msg, data);
        if (ret <  0)
            return -1;

        *(uint16_t*)send_buf = htons(sizeof(uint16_t) + msg.ByteSize());
        int32_t send_len = sizeof(uint16_t) + msg.ByteSize();
        if (msg.SerializeToArray(send_buf + sizeof(uint16_t), PKG_BUF_SIZE - sizeof(uint16_t)) == false) {
            printf("msg.SerializeToArray error.\n");
            return -1;
        }

        printf("DispatchReqMsg cmd[%d] send_len[%d]\n", cmd, send_len);
        printf("\nmsg[%d]\n%s\n", msg.ByteSize(), msg.Utf8DebugString().c_str());


        SendData(send_buf, send_len);
    } else {
        return -2;
    }
    return 0;
}

int32_t MsgMgr::DispatchResMsg(const ProtoCs::Msg* msg, void* data)
{
    printf("MsgMgr::DispatchResMsg\n");
    (void)data;
    // 分发处理服务器响应包
    MsgWrapperMap::iterator it = res_msg_wrapper_map_.find(msg->head().cmd());
    if (it != res_msg_wrapper_map_.end()) {
        // 处理消息
        int ret = it->second->HandleResMsg((void*)msg, NULL);
        if (ret != 0) {
            printf("DispatchResMsg error!\n");
            return -1;
        }
    } else {
        printf("DispatchResMsg cmd[%d] can't find!\n", msg->head().cmd());
        return -2;
    }
    return 0;
}

void MsgMgr::FreeMap()
{
    if (req_msg_wrapper_map_.size() > 0) {
        for (MsgWrapperMap::iterator it = req_msg_wrapper_map_.begin();
                it != req_msg_wrapper_map_.end(); ++it)
            if (it->second != NULL) {
                delete it->second;
                it->second = NULL;
            }
        req_msg_wrapper_map_.clear();
    }

    if (res_msg_wrapper_map_.size() > 0) {
        for (MsgWrapperMap::iterator it = res_msg_wrapper_map_.begin();
                it != res_msg_wrapper_map_.end(); ++it)
            if (it->second != NULL) {
                delete it->second;
                it->second = NULL;
            }
        res_msg_wrapper_map_.clear();
    }
}

int MsgMgr::ConnectServer(const char* ip, unsigned short port)
{
	struct sockaddr_in svraddr;
	svraddr.sin_family = AF_INET;
	svraddr.sin_addr.s_addr = inet_addr(ip);
	svraddr.sin_port = htons(port);

	int ret = connect(sock_, (struct sockaddr*)&svraddr, sizeof(svraddr));
	if(ret == SOCKET_ERROR) {
        errno_ = ERR_UNCONNECT;
        return -1;        
	}
    
#ifdef WIN32
	u_long arg = 1;
	if (ioctlsocket(sock_, FIONBIO, &arg) != NO_ERROR) {
        CloseSock();
        errno_ = ERR_UNCONNECT;
		return -1; 
	}
#else
    // 非windows下设置非阻塞处理模式
    SetNonBlock();
    SetSocketOpt();
#endif

    errno_ = ERR_OK;
	return 0;
}

int MsgMgr::SetNonBlock()
{
#ifndef WIN32
    int flags = 1;
    int ret = ioctl(sock_, FIONBIO, &flags);
    if(ret != 0) {
        CloseSock();
        errno_ = ERR_UNCONNECT;
        return -1;
    }

    flags = fcntl(sock_, F_GETFL); 
    flags |= O_NONBLOCK;
    ret = fcntl(sock_, F_SETFL, flags);
    if(ret < 0) {
        CloseSock();
        errno_ = ERR_UNCONNECT;
        return -1;
    }
#endif
    return 0;
}

int MsgMgr::SetSocketOpt()
{
#ifndef WIN32
    // 设置套接字重用
    int reuse_addr_ok = 1;
    setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &reuse_addr_ok, sizeof(reuse_addr_ok));

    // 设置接收&发送buffer
    int recv_buf = SOCK_RECV_BUFFER;
    setsockopt(sock_, SOL_SOCKET, SO_RCVBUF, (const char*)&recv_buf, sizeof(recv_buf));
    int send_data = SOCK_SEND_BUFFER;
    setsockopt(sock_, SOL_SOCKET, SO_SNDBUF, (const char*)&send_data, sizeof(send_data));

    int keep_alive = 1; // 开启keepalive属性
    int keep_idle = 30; // 如该连接在30秒内没有任何数据往来,则进行探测 
    int keep_interval = 5; // 探测时发包的时间间隔为5 秒
    int keep_count = 3; // 探测尝试的次数.如果第1次探测包就收到响应了,则后2次的不再发.

    setsockopt(sock_, SOL_SOCKET, SO_KEEPALIVE, (void *)&keep_alive, sizeof(keep_alive));
    setsockopt(sock_, SOL_TCP, TCP_KEEPIDLE, (void*)&keep_idle, sizeof(keep_idle));
    setsockopt(sock_, SOL_TCP, TCP_KEEPINTVL, (void *)&keep_interval, sizeof(keep_interval));
    setsockopt(sock_, SOL_TCP, TCP_KEEPCNT, (void *)&keep_count, sizeof(keep_count));
#endif

    return 0;
}

void MsgMgr::Run()
{
    // 发送数据处理
    int ret = 0;
    char* send_buf = conn_cache_.send_buf();
    int send_len = conn_cache_.send_buf_size();

    if (send_len > 0) {
        ret = ::send(sock_, send_buf, send_len, 0);

        // 错误处理
        if (ret < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                // 忽略EINTR EAGAIN EWOULDBLOCK错误处理
                return;
            } else {
                Resume();
                return;
            }
        } else if (ret >= 0 && ret < send_len) {
            // 发送了部分, conn_cache跳过已发送部分
            printf("MsgMgr send part [%d]\n", ret);
            conn_cache_.SkipSendData(ret);
        } else if (ret == send_len) {
            // 发送完全部, 清空conn_cache
            printf("MsgMgr send all [%d]\n", ret);
            conn_cache_.clear_send_buf();
        }
    }

    // 接收数据处理
    char* recv_buf = conn_cache_.GetRemainRecvBuf();
    int recv_len = conn_cache_.GetRemainRecvBufLen();
    ret = ::recv(sock_, recv_buf, recv_len, 0);
    printf("recv ret = %d\n", ret);
    // 错误处理
    if (ret < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
            // 忽略EINTR EAGAIN EWOULDBLOCK错误处理
            return;
        } else {
            Resume();
            return;
        }
    } else if (ret == 0) {
        // 服务器关闭连接
        Resume(); 
        return;
    } else if (ret > 0) {
        conn_cache_.AddRecvBufLen(ret);
    }

    while (conn_cache_.IsRecvProtoReady()) {
        // 接收真实长度大于协议长度就进行处理
        static ProtoCs::Msg msg;
        msg.Clear();

        char* msg_buf = conn_cache_.recv_buf() + sizeof(uint16_t);
        int32_t msg_len = conn_cache_.GetRecvProtoLen() - sizeof(uint16_t);

        if (msg.ParseFromArray(msg_buf, msg_len) == false) {
            printf("msg protobuf parse error!\n");
            return;
        } 

        printf("\nRecv msg[%d]\n%s\n", msg.ByteSize(), msg.Utf8DebugString().c_str());
        // 消息分发处理
        DispatchResMsg(&msg, NULL);
        conn_cache_.SkipRecvProto();
    }
}

int MsgMgr::SendData(const char* buf, int len)
{
    conn_cache_.AddSendData(buf, len);
    return 0;
}

int MsgMgr::CloseSock()
{
    int ret = 0;
#ifdef WIN32
	ret = (closesocket(sock_));
#else
	ret = (close(sock_));
#endif
    errno_ = ERR_UNCONNECT;
    sock_ = 0;
    return ret;
}

int MsgMgr::DnsParse(const char* domain, char* ip)
{
	struct hostent* p;
	if ((p = gethostbyname(domain)) == NULL) {
		return -1;
    }
		
	sprintf(ip, 
		"%u.%u.%u.%u",
		(unsigned char)p->h_addr_list[0][0], 
		(unsigned char)p->h_addr_list[0][1], 
		(unsigned char)p->h_addr_list[0][2], 
		(unsigned char)p->h_addr_list[0][3]);
	
	return 0;
}

int MsgMgr::Resume()
{
    if(Init() == 0) {
        // 设置为网络模式
        // TODO: 重新登录
        return 0;
    }
    else
    {
        // 登录失败
        // MsgMgr设置为单机模式
    }
    return -1;
}
////////////////////////////逻辑处理部分////////////////////////////
int32_t MsgMgr::DoLogin(ProtoCs::Msg* msg, void* args)
{
    printf("DoLogin\n");
    (void)args;
    ProtoCs::LoginReq* login_req = msg->mutable_login_req();

    login_req->set_account("zhangfengzhen");
    login_req->set_password("qwe123456");

    return 0;
}

int32_t MsgMgr::OnLogin(ProtoCs::Msg* msg, void* args)
{
    printf("OnLogin\n");
    (void)args;
    int32_t msg_ret = msg->mutable_head()->ret();

    if (msg_ret == 0) {
        printf("Login ok!\n");
    } else {
        printf("Login failed!\n");
    }

    return 0;
}

int32_t MsgMgr::DoNormalReg(ProtoCs::Msg* msg, void* args)
{
    printf("DoNormalReg\n");
    (void)args;
    ProtoCs::NormalRegReq* normal_reg_req = msg->mutable_normal_reg_req();

    normal_reg_req->set_account("zhangfengzhen");
    normal_reg_req->set_password("qwe123456");

    return 0;
}

int32_t MsgMgr::OnNormalReg(ProtoCs::Msg* msg, void* args)
{
    printf("OnNormalReg\n");
    (void)args;
    int32_t msg_ret = msg->mutable_head()->ret();

    if (msg_ret == 0) {
        printf("NormalReg ok!\n");
    } else {
        printf("NormalReg failed!\n");
    }

    return 0;
}

int32_t MsgMgr::DoQuickReg(ProtoCs::Msg* msg, void* args)
{
    printf("DoQuickReg\n");
    (void)msg;
    (void)args;
    // ProtoCs::QuickRegReq* quick_reg_req = msg->mutable_quick_reg_req();

    return 0;
}

int32_t MsgMgr::OnQuickReg(ProtoCs::Msg* msg, void* args)
{
    printf("OnQuickReg\n");
    (void)args;
    int32_t msg_ret = msg->mutable_head()->ret();

    if (msg_ret == 0) {
        printf("QuickReg ok!\n");
        printf("account [%s] password[%s]\n", 
            msg->quick_reg_res().account().c_str(),
            msg->quick_reg_res().password().c_str());
    } else {
        printf("QuickReg failed!\n");
    }

    return 0;
}
