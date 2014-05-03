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
// 协议
#include "cs_msg.pb.h"

#define PKG_BUF_SIZE 10240

#define REGISTER_MSG(REQ_MSG_ID, REQ_MSG_FUNC, RES_MSG_ID, RES_MSG_FUNC) \
        MsgWrapper * msg_wrapper = static_cast<MsgWrapper*> \
        (new MsgWrapperImpl<MsgMgr, ProtoCs::Msg>(this, REQ_MSG_FUNC, RES_MSG_FUNC));   \
        assert(msg_wrapper != NULL); \
        MsgMgr::GetInstance().RegisterReqMsg(REQ_MSG_ID, msg_wrapper); \
        MsgMgr::GetInstance().RegisterResMsg(RES_MSG_ID, msg_wrapper); \
        msg_wrapper = NULL; \

class MsgMgr
{
public:
    ~MsgMgr() {}
    static MsgMgr& GetInstance();

    int32_t Init();

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

private:
    void FreeMap();

private:
    MsgMgr(){}
    MsgMgr(const MsgMgr&);
    MsgMgr& operator=(const MsgMgr&);

private:    
    // 消息映射表
    typedef std::map<int32_t, MsgWrapper*> MsgWrapperMap;
    MsgWrapperMap req_msg_wrapper_map_;
    MsgWrapperMap res_msg_wrapper_map_;
    uint64_t free_seq_;
};

#endif 

