/**
 * @file net_mgr.cpp
 * @brief 网络管理类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "net_mgr.h"
#include "msg_mgr.h"
#include "stdio.h"

int NetMgr::errno_ = ERR_UNCONNECT;

// 接收和发送缓冲区大小
const size_t SOCK_RECV_BUFFER = 512*1024;
const size_t SOCK_SEND_BUFFER = 512*1024;

NetMgr& NetMgr::GetInstance()
{
    static NetMgr inst;
    return inst;
}

int NetMgr::Init()
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

    return 0;
}

int NetMgr::ConnectServer(const char* ip, unsigned short port)
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

int NetMgr::SetNonBlock()
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

int NetMgr::SetSocketOpt()
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

void NetMgr::Run()
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
            printf("NetMgr send part [%d]\n", ret);
            conn_cache_.SkipSendData(ret);
        } else if (ret == send_len) {
            // 发送完全部, 清空conn_cache
            printf("NetMgr send all [%d]\n", ret);
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
        MsgMgr::GetInstance().DispatchResMsg(&msg, NULL);
        conn_cache_.SkipRecvProto();
    }
}

int NetMgr::SendData(const char* buf, int len)
{
    conn_cache_.AddSendData(buf, len);
    return 0;
}

int NetMgr::CloseSock()
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

int NetMgr::DnsParse(const char* domain, char* ip)
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

int NetMgr::Resume()
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

int32_t NetMgr::GetMsgHead(const char* pkg_buf, int32_t pkg_len, char** msg_head, int32_t* msg_head_len)
{
    int32_t pkg_total_len = (int32_t)ntohs(*(uint16_t*)pkg_buf);
    if (pkg_total_len != pkg_len || pkg_total_len <=0) {
        printf("pkg_total_len parse error!\n");
        return -1; 
    }

    *msg_head_len = (int32_t)ntohs(*(uint16_t*)(pkg_buf + 1));
    if (*msg_head_len <= 0) {
        printf("msg_head_len parse error!\n");
        return -1; 
    }

    *msg_head = (char*)pkg_buf + 2*sizeof(uint16_t);
    return 0; 
}

int32_t NetMgr::GetMsgBody(const char* pkg_buf, int32_t pkg_len, char** msg_body, int32_t* msg_body_len)
{
    int32_t pkg_total_len = (int32_t)ntohs(*(uint16_t*)pkg_buf);
    if (pkg_total_len != pkg_len || pkg_total_len <=0) {
        printf("pkg_total_len parse error!\n");
        return -1; 
    }

    int32_t msg_head_len = (int32_t)ntohs(*(uint16_t*)(pkg_buf + 1));
    if (msg_head_len <= 0) {
        printf("msg_head_len parse error!\n");
        return -1; 
    }

    *msg_body_len = pkg_total_len - msg_head_len;
    if (*msg_body_len <= 0) {
        printf("msg_head_len parse error!\n");
        return -1; 
    }

    *msg_body = (char*)pkg_buf + 2*sizeof(uint16_t) + msg_head_len;
    return 0; 
}

