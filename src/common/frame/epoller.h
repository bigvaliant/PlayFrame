/**
 * @file epoller.h
 * @brief 异步事件机制封装
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-01
 */
#ifndef _EPOLLER_H_
#define _EPOLLER_H_

#include "comm_def.h"

#define EVENT_NONE 0
#define EVENT_READ 1
#define EVENT_WRITE 2

typedef void FileProc(int fd, void* client_data); 

typedef struct tagFileEvent {
    int mask_;
    FileProc* read_proc_;
    FileProc* write_proc_;
    void* client_data_;
} FileEvent;

typedef struct tagEpollEventLoop {
    int epfd_;
    struct epoll_event *epoll_events_;
    int max_events_size_;
    FileEvent* file_events_;
} EpollEventLoop;

class Epoller {
public:
    Epoller() { ev_loop_ = NULL; };
    virtual ~Epoller () { ClearEpoller(); }

    void Init(int max_events_size);
    int AddEvent(int fd, int mask, FileProc* proc, void* client_data);
    void DelEvent(int fd, int mask);
    void ClearEvent(int fd) { DelEvent(fd, EVENT_READ | EVENT_WRITE); }
    void Loop(int timeout = 1);

    static void SetNonBlock(int32_t fd);
    static void SetSocketOpt(int32_t fd);

private:
    void ClearEpoller() 
    {
        if (ev_loop_ != NULL) {
            close(ev_loop_->epfd_);
            ev_loop_->epfd_ = 0;

            free(ev_loop_->epoll_events_);
            ev_loop_->epoll_events_ = NULL;

            free(ev_loop_->file_events_);
            ev_loop_->file_events_ = NULL;

            free(ev_loop_);
            ev_loop_ = NULL;
        }
    }

private:
    EpollEventLoop* ev_loop_;
};

#endif
