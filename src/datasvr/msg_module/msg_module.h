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
            << "\nmsg:\n"
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

    template<typename T>
    int32_t SendToGamesvr(const T* msg) 
    {
        return SendMsg(gamesvr_zmq_sock_, msg);
    }

private:
    int32_t OnAccountRegReq(ProtoSs::Msg* msg, void* args);
    static void AccountRegMysqlRes(MYSQL_RES* res, int32_t flags, uint64_t seq_id, int32_t player_idx);

    int32_t OnAccountVerifyReq(ProtoSs::Msg* msg, void* args);
    static void AccountVerifyMysqlRes(MYSQL_RES* res, int32_t flags, uint64_t seq_id, int32_t player_idx);

    int32_t OnGetPlayerDataReq(ProtoSs::Msg* msg, void* args);
    static void GetPlayerDataMysqlRes(MYSQL_RES* res, int32_t flags, uint64_t seq_id, int32_t player_idx);

    int32_t OnSetPlayerDataReq(ProtoSs::Msg* msg, void* args);
    static void SetPlayerDataMysqlRes(MYSQL_RES* res, int32_t flags, uint64_t seq_id, int32_t player_idx);

private:
    void*           zmq_ctx_;
    void*           gamesvr_zmq_sock_;
};

#endif // _MSG_MODULE_H_

