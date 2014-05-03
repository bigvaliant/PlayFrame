/**
 * @file msg_module.h
 * @brief 消息模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _MSG_MODULE_H_
#define _MSG_MODULE_H_

#include "app_def.h"

class MsgModule : public AppModuleBase,
                  public MsgModuleBase
{
public:
    DISALLOW_COPY_AND_ASSIGN(MsgModule);

	MsgModule(App* app);
	virtual ~MsgModule();

	virtual void            ModuleInit();
	virtual void            ModuleFini();
	virtual const char*     ModuleName() const;
	static int32_t          ModuleId();
	static AppModuleBase*   CreateModule(App* app);

public:
    void Run();
    template<typename T>
    static int32_t SendMsg(void* zmq_sock, const T* msg)
    {
        static char send_buf[PKG_BUF_SIZE] = {0};

        if (msg->SerializeToArray(send_buf, PKG_BUF_SIZE) == false) {
            LOG(ERROR) << "serialize proto error!";
            return -1;
        }

        LOG(INFO)
            << "\nMsgModule Send msg:\n"
            << msg->Utf8DebugString();

        int ret = zmq_send(zmq_sock, send_buf, msg->ByteSize(), ZMQ_DONTWAIT);

        if (ret < 0) {
            LOG(ERROR)
                << "zmq_send errno[" << errno
                << "] error[" << strerror(errno)
                << "]";
            return -2;
        }
        return 0;
    }

    int32_t SendToConnsvr(ProtoCs::Msg* msg, int32_t conn_cmd, int32_t conn_fd, int32_t player_idx);

    int32_t SendStartToConnsvr(ProtoCs::Msg* msg, int32_t conn_fd, int32_t player_idx)
    {
        return SendToConnsvr(msg, CONN_START, conn_fd, player_idx); 
    }

    int32_t SendProcToConnsvr(ProtoCs::Msg* msg, int32_t conn_fd, int32_t player_idx)
    {
        return SendToConnsvr(msg, CONN_PROC, conn_fd, player_idx); 
    }

    int32_t SendCloseToConnsvr(ProtoCs::Msg* msg, int32_t conn_fd, int32_t player_idx)
    {
        return SendToConnsvr(msg, CONN_STOP, conn_fd, player_idx); 
    }

    int32_t SendToDatasvr(ProtoSs::Msg* msg)
    {
        return SendMsg(datasvr_zmq_sock_, msg);
    }

private:
    int32_t OnConnProcess(ProtoCs::Msg* msg, void* arg);

    // client 交互
    int32_t OnClientQuickRegReq(ProtoCs::Msg* msg, void* arg);
    int32_t OnClientNormalRegReq(ProtoCs::Msg* msg, void* arg);
    int32_t OnClientLoginReq(ProtoCs::Msg* msg, void* arg);

    // datasvr 交互
    int32_t OnDatasvrAccountRegRes(ProtoSs::Msg* msg, void* arg);
    int32_t OnDatasvrAccountVerifyRes(ProtoSs::Msg* msg, void* arg);
    int32_t OnDatasvrGetPlayerDataRes(ProtoSs::Msg* msg, void* arg);
    int32_t OnDatasvrSetPlayerDataRes(ProtoSs::Msg* msg, void* arg);

private:
    void*           zmq_ctx_;
    void*           connsvr_zmq_sock_;
    void*           datasvr_zmq_sock_;
};

#endif 

