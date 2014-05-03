/**
 * @file net_mgr.h
 * @brief 网络管理类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _NET_MGR_H_
#define _NET_MGR_H_

#include "conn_cache.h"

#ifdef WIN32
    #include <winsock.h>
	typedef int				socklen_t;
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <fcntl.h>
	#include "errno.h"
	#include <unistd.h>
	#include <sys/stat.h>
	#include <sys/types.h>
	#include <arpa/inet.h>
    #include <sys/ioctl.h>
    #include <netinet/tcp.h>
	typedef int				SOCKET;

	//#pragma region define win32 const variable in linux
	#define INVALID_SOCKET	-1
	#define SOCKET_ERROR	-1
	//#pragma endregion
#endif

enum ConnMgrErrCode
{
    ERR_OK = 0,
    ERR_CLIENT_CLOSE= -1,
    ERR_UNCONNECT = -2,
};

class NetMgr
{
public:
    ~NetMgr() {};
    static NetMgr& GetInstance();

    static int GetErrno() { return errno_; }

    int Init();
    int Resume();
    void Run();
	int SendData(const char* buf, int len);

private:
    NetMgr(){}
    NetMgr(const NetMgr&);
    NetMgr& operator=(const NetMgr&);

private:
    int SetNonBlock();
    int SetSocketOpt();
    int DnsParse(const char* domain, char* ip);
    int ConnectServer(const char* ip, unsigned short port);
    int CloseSock();

    static int32_t GetMsgHead(const char* pkg_buf, int32_t pkg_len, char** msg_head, int32_t* msg_head_len);
    static int32_t GetMsgBody(const char* pkg_buf, int32_t pkg_len, char** msg_body, int32_t* msg_body_len);

private:
    static int  errno_;

private:
    SOCKET      sock_;
    ConnCache   conn_cache_;
};

#endif
