/**
 * @file epoller.cpp
 * @brief 异步事件机制封装
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-01
 */
#include "epoller.h"

void Epoller::Init(int max_events_size)
{
    ClearEpoller();
    ev_loop_ = (EpollEventLoop*)malloc(sizeof(EpollEventLoop));
    CHECK(ev_loop_ != NULL)
        << "malloc EpollEventLoop error.";
    memset(ev_loop_, 0, sizeof(EpollEventLoop));

    ev_loop_->epoll_events_ = (struct epoll_event*)
        malloc(sizeof(struct epoll_event) * max_events_size);
    CHECK(ev_loop_->epoll_events_ != NULL)
        << "malloc ev_loop_->epoll_events_ error.";
    memset(ev_loop_->epoll_events_, 0, sizeof(struct epoll_event) * max_events_size);

    ev_loop_->file_events_ = (FileEvent*)malloc(sizeof(FileEvent) * max_events_size);
    CHECK(ev_loop_->file_events_ != NULL)
        << "malloc ev_loop_->file_events_ error.";
    memset(ev_loop_->file_events_, 0, sizeof(FileEvent) * max_events_size);

    ev_loop_->max_events_size_ = max_events_size;

    ev_loop_->epfd_ = epoll_create(max_events_size);
    CHECK(ev_loop_->epfd_ != -1)
        << "epoll_create error.";
}

int Epoller::AddEvent(int fd, int mask, FileProc* proc, void* client_data)
{
    if (fd >= ev_loop_->max_events_size_) {
        LOG(ERROR) 
            << "fd[" << fd
            << "] >= max_events_size_[" << ev_loop_->max_events_size_
            << "] overload.";
        return -1; 
    }

    FileEvent* fe = &(ev_loop_->file_events_[fd]);

    int op = fe->mask_ == EVENT_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

    struct epoll_event ee;
    ee.events = 0;
    if ((mask | fe->mask_) & EVENT_READ) ee.events |= EPOLLIN;
    if ((mask | fe->mask_) & EVENT_WRITE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0;
    ee.data.fd = fd;

    if (epoll_ctl(ev_loop_->epfd_, op, fd, &ee) == -1) {
        LOG(ERROR)
            << "epoll_ctl error.";
        return -1;
    } 

    fe->mask_ |= mask;
    if (mask & EVENT_READ) fe->read_proc_ = proc;
    if (mask & EVENT_WRITE) fe->write_proc_ = proc;
    fe->client_data_ = client_data;

    return 0;
}

void Epoller::DelEvent(int fd, int mask)
{
    if (fd >= ev_loop_->max_events_size_) return;

    FileEvent* fe = &(ev_loop_->file_events_[fd]);
    if (fe->mask_ == EVENT_NONE) return;

    fe->mask_ = fe->mask_ & (~mask);
    
    struct epoll_event ee;
    ee.events = 0;
    if (fe->mask_ & EVENT_READ) ee.events |= EPOLLIN;
    if (fe->mask_ & EVENT_WRITE) ee.events |= EPOLLOUT;
    ee.data.u64 = 0;
    ee.data.fd = fd;

    if (fe->mask_ != EVENT_NONE) {
        epoll_ctl(ev_loop_->epfd_, EPOLL_CTL_MOD, fd, &ee); 
    } else {
        epoll_ctl(ev_loop_->epfd_, EPOLL_CTL_DEL, fd, &ee); 
    }
}

void Epoller::Loop(int timeout)
{
    int ret = epoll_wait(ev_loop_->epfd_,
        ev_loop_->epoll_events_, ev_loop_->max_events_size_, timeout); 

    if (ret > 0) {
        for (int i = 0; i < ret; i++) {
            struct epoll_event* e = ev_loop_->epoll_events_ + i;
            int fd = e->data.fd;
            FileEvent* fe = &(ev_loop_->file_events_[fd]);
            if (e->events & EPOLLIN) 
                (*fe->read_proc_)(fd, fe->client_data_);
            if (e->events & EPOLLOUT) 
                (*fe->write_proc_)(fd, fe->client_data_);
            if (e->events & EPOLLERR) 
                (*fe->write_proc_)(fd, fe->client_data_);
            if (e->events & EPOLLHUP) 
                (*fe->write_proc_)(fd, fe->client_data_);
        } 
    }
}

void Epoller::SetNonBlock(int32_t fd)
{
    int32_t flags = 1;        
    int32_t ret = ioctl(fd, FIONBIO, &flags);
    CHECK(ret == 0)
        << "ioctl FIOBIO error. ret[" << ret
        << "] fd[" << fd
        << "]";

    flags = fcntl(fd, F_GETFL); 
    flags |= O_NONBLOCK;
    ret = fcntl(fd, F_SETFL, flags);
    CHECK(ret >= 0)
        << "fcntl F_SETFL error. ret[" << ret
        << "] fd[" << fd
        << "]";
}

void Epoller::SetSocketOpt(int32_t fd)
{
    // 设置套接字重用
    int32_t reuse_addr_ok = 1;
    setsockopt(fd,SOL_SOCKET, SO_REUSEADDR, &reuse_addr_ok, sizeof(reuse_addr_ok));

    // 心跳机制
    int32_t flags = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags));

    // 将数据继续发送至对端, 优雅关闭连接
    struct linger ling = {0, 0};
    setsockopt(fd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));
}

