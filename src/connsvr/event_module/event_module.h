/**
 * @file event_module.h
 * @brief 事件处理模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _EVENT_MODULE_H_
#define _EVENT_MODULE_H_

#include "app_def.h"

class EventModule : public AppModuleBase
{
public:
    DISALLOW_COPY_AND_ASSIGN(EventModule);

	EventModule(App* app);
	virtual ~EventModule();

	virtual void            ModuleInit();
	virtual void            ModuleFini();
	virtual const char*     ModuleName() const;
	static int32_t          ModuleId();
	static AppModuleBase*   CreateModule(App* app);

public:
    void Run();

    inline void* zmq_sock() const
    {
        return zmq_sock_;
    }

    inline int32_t zmq_pair_fd() const
    {
        return zmq_pair_fd_;
    }

private:
    static void DoTcpAccept(int fd, void* arg);
    static void DoTcpRead(int fd, void* arg);
    static void DoTcpWrite(int fd, void* arg);
    static void SendConnStop(void* zmq_sock, int32_t player_idx);
    void ZmqReadLoop();

private:
    void*               zmq_ctx_;
    void*               zmq_sock_;
    int32_t             zmq_pair_fd_;
    int32_t             listen_fd_;

    Epoller*            epoller_;
};

#endif // _EVENT_MODULE_H_

