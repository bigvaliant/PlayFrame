/**
 * @file msg_wrapper.h
 * @brief 消息分发封装
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _MSG_WRAPPER_H_
#define _MSG_WRAPPER_H_

#include "comm_def.h"

class MsgWrapper
{
public:
    virtual ~MsgWrapper() {}
    virtual int32_t Process(void* msg, void* args) = 0;
};

template<class T, class MSG>
class MsgWrapperImpl : public MsgWrapper
{
public:
    typedef int32_t (T::*Func)(MSG* msg, void* args);

    MsgWrapperImpl(T* obj, Func func)
        : obj_(obj),
          func_(func)
    {}

    virtual ~MsgWrapperImpl() {}

    virtual int32_t Process(void* msg, void* args)
    {
        MSG* msg_impl = static_cast<MSG*>(msg);
        if (msg_impl != NULL)
            return (obj_->*func_)(msg_impl, args);
        LOG(ERROR)
            << "msg is NULL!";
        return -1;
    }

private:
    T*      obj_;
    Func    func_;
};

#define REGISTER_MSG_BEGIN(HANDLER_CLASS, PKG_CLASS) do {   \
        MsgWrapper * msg_wrapper = NULL;            \
        typedef HANDLER_CLASS handler_class_type;           \
        typedef PKG_CLASS pkg_class_type;                   \

#define REGISTER_MSG(MSG_MODULE, MSG_ID, MSG_FUNC) \
        msg_wrapper = static_cast<MsgWrapper*> \
        (new MsgWrapperImpl<handler_class_type, pkg_class_type>(this, MSG_FUNC));   \
        CHECK(msg_wrapper != NULL) << "msg_wrapper == NULL"; \
        MSG_MODULE->Register(MSG_ID, msg_wrapper); \
        msg_wrapper = NULL; \

#define REGISTER_MSG_END } while(0)

class MsgModuleBase
{
public:
    MsgModuleBase() {}
    virtual ~MsgModuleBase() { FreeMsgWrapperMap(); }

    template<typename T>
    int32_t HandleRequest(const char* pkg_buf, const size_t pkg_len, void* data)
    {
        static T msg;        
    
        msg.Clear();

        // Protobuf解析
        if (msg.ParseFromArray(pkg_buf, pkg_len) == false) {
            LOG(ERROR)
                << "protobuf parse error!";
            return -1;
        } 

        // 输出包结果
        int32_t ret = 0;
        bool need_print = NeedPrintPkg(msg.head().cmd());
        if (need_print) {
            LOG(INFO)
                << "\n"
                << msg.Utf8DebugString();
        }

        // 根据cmd进行处理
        MsgWrapperMap::iterator it = msg_wrapper_map_.find(msg.head().cmd());
        if (it != msg_wrapper_map_.end()) {
            // 处理消息
            ret = it->second->Process((void*)&msg, data);
            if (need_print) {
                LOG(INFO)
                    << "msg[" << msg.head().cmd() 
                    << "] ret[" << ret 
                    << "]";
            }
        } else {
            LOG(ERROR)
                << "msg[" << msg.head().cmd() 
                << "] is not register!";
            return -2;
        }

        return 0;
    }
protected:
    void Register(int32_t msg_id, MsgWrapper* wrapper)
    {
        msg_wrapper_map_[msg_id] = wrapper;
    }

    void FreeMsgWrapperMap()
    {
        if (msg_wrapper_map_.size() > 0) {
            for (MsgWrapperMap::iterator it = msg_wrapper_map_.begin(); it != msg_wrapper_map_.end(); ++it)
                if (it->second != NULL)
                    delete it->second;
            msg_wrapper_map_.clear();
        }
    }

    virtual bool NeedPrintPkg(uint32_t cmd)
    {
        UNUSE_ARG(cmd);
        return true;
    }
private:    
    // 消息映射表
    typedef std::map<int32_t, MsgWrapper*> MsgWrapperMap;
    MsgWrapperMap msg_wrapper_map_;
};

#endif

