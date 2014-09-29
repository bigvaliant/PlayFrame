/**
 * @file msg_mgr.h
 * @brief 消息分发器
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _MSG_MGR_H_
#define _MSG_MGR_H_

#include "msg_wrapper.h"
#include "conn_cache.h"
// 协议
#include "cs_msg.pb.h"
// 网络
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

#define PKG_BUF_SIZE 10240
#define REGISTER_MSG(MSG_MGR, REQ_MSG_ID, REQ_MSG_FUNC, RES_MSG_ID, RES_MSG_FUNC) \
        do { \
        MsgWrapper * msg_wrapper = static_cast<MsgWrapper*> \
        (new MsgWrapperImpl<MsgMgr, ProtoCs::Msg>(this, REQ_MSG_FUNC, RES_MSG_FUNC));   \
        assert(msg_wrapper != NULL); \
        MSG_MGR->RegisterReqMsg(REQ_MSG_ID, msg_wrapper); \
        MSG_MGR->RegisterResMsg(RES_MSG_ID, msg_wrapper); \
        msg_wrapper = NULL; \
        } while(0) \


enum ConnMgrErrCode
{
    ERR_OK = 0,
    ERR_CLIENT_CLOSE= -1,
    ERR_UNCONNECT = -2,
};

class MsgMgr
{
public:
    MsgMgr() {}
    ~MsgMgr() {}

    int32_t Init();
    int32_t Resume();
    void Run();
	int32_t SendData(const char* buf, int len);

    int32_t DispatchReqMsg(int32_t cmd, void* data = NULL);
    int32_t DispatchResMsg(const ProtoCs::Msg* msg, void* data = NULL);

    void RegisterReqMsg(int32_t msg_id, MsgWrapper* wrapper)
    {
        req_msg_wrapper_map_[msg_id] = wrapper;
    }

    void RegisterResMsg(int32_t msg_id, MsgWrapper* wrapper)
    {
        res_msg_wrapper_map_[msg_id] = wrapper;
    }

//逻辑处理
public:
    int32_t DoLogin(ProtoCs::Msg* msg, void* args);
    int32_t OnLogin(ProtoCs::Msg* msg, void* args);

    int32_t DoNormalReg(ProtoCs::Msg* msg, void* args);
    int32_t OnNormalReg(ProtoCs::Msg* msg, void* args);

    int32_t DoQuickReg(ProtoCs::Msg* msg, void* args);
    int32_t OnQuickReg(ProtoCs::Msg* msg, void* args);

private:
    void FreeMap();

private:
    int SetNonBlock();
    int SetSocketOpt();
    int DnsParse(const char* domain, char* ip);
    int ConnectServer(const char* ip, unsigned short port);
    int CloseSock();


private:
    MsgMgr(const MsgMgr&);
    MsgMgr& operator=(const MsgMgr&);

private:    
    // 消息映射表
    typedef std::map<int32_t, MsgWrapper*> MsgWrapperMap;
    MsgWrapperMap req_msg_wrapper_map_;
    MsgWrapperMap res_msg_wrapper_map_;
    uint64_t free_seq_;
    SOCKET      sock_;
    ConnCache   conn_cache_;
    static int32_t errno_;
};

#endif 

