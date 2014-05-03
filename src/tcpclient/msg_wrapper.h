/**
 * @file msg_wrapper.h
 * @brief 消息包装
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _MSG_WRAPPER_H_
#define _MSG_WRAPPER_H_

#include <stdint.h>
#include <unistd.h>

class MsgWrapper
{
public:
    virtual ~MsgWrapper() {}
    virtual int32_t HandleReqMsg(void* msg, void* args) = 0;
    virtual int32_t HandleResMsg(void* msg, void* args) = 0;
};

template<class T, class MSG>
class MsgWrapperImpl : public MsgWrapper
{
public:
    typedef int32_t (T::*ReqFunc)(MSG* msg, void* args);
    typedef int32_t (T::*ResFunc)(MSG* msg, void* args);

    MsgWrapperImpl(T* obj, ReqFunc req_func, ResFunc res_func)
        : obj_(obj),
          req_func_(req_func),
          res_func_(res_func)
    {}

    virtual ~MsgWrapperImpl() {}

    virtual int32_t HandleReqMsg(void* msg, void* args)
    {
        MSG* msg_impl = static_cast<MSG*>(msg);
        if (msg_impl != NULL)
            return (obj_->*req_func_)(msg_impl, args);
        return -1;
    }

    virtual int32_t HandleResMsg(void* msg, void* args)
    {
        MSG* msg_impl = static_cast<MSG*>(msg);
        if (msg_impl != NULL)
            return (obj_->*res_func_)(msg_impl, args);
        return -1;
    }

private:
    T*              obj_;
    ReqFunc         req_func_;
    ResFunc         res_func_;
};

#endif
