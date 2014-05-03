/**
 * @file msg_mgr.cpp
 * @brief 消息分发器
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include <stdio.h>
#include "msg_mgr.h"
#include "net_mgr.h"

MsgMgr& MsgMgr::GetInstance()
{
    static MsgMgr inst;
    return inst;
}

int32_t MsgMgr::Init()
{
    free_seq_ = 10000;
    
    // 注册消息
    REGISTER_MSG(ProtoCs::Msg::kLoginReqFieldNumber, &MsgMgr::DoLogin,
        ProtoCs::Msg::kLoginResFieldNumber, &MsgMgr::OnLogin);

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


        NetMgr::GetInstance().SendData(send_buf, send_len);
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

