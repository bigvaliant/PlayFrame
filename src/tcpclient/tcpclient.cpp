/**
 * @file tcpclient.cpp
 * @brief 用于测试服务器功能
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include <stdio.h>
#include "msg_mgr.h"

int main()
{
    int ret = 0;
    MsgMgr msg_mgr;
    ret = msg_mgr.Init();
    if (ret != 0) {
        printf("MsgMgr init error\n");
        exit(0);
    }

    msg_mgr.DispatchReqMsg(ProtoCs::Msg::kQuickRegReqFieldNumber);
    //MsgMgr::GetInstance().DispatchReqMsg(ProtoCs::Msg::kNormalRegReqFieldNumber);
    //MsgMgr::GetInstance().DispatchReqMsg(ProtoCs::Msg::kLoginReqFieldNumber);

    while(1) {
        msg_mgr.Run();
        sleep(3);
    }

    return 0;
}
