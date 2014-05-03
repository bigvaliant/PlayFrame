/**
 * @file conn_cache.h
 * @brief 连接缓存
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _CONN_CACHE_H_
#define _CONN_CACHE_H_

#include <stdint.h>
#include <arpa/inet.h>

#define CONN_CACHE_BUF_LEN (100*1024)

class ConnCache
{
public:
    ConnCache();
    ~ConnCache() {};

    void Init();

    inline char* send_buf() const { return send_buf_; }
    inline char* recv_buf() const { return recv_buf_; }

    inline int send_buf_size() const { return send_buf_size_; }
    inline int recv_buf_size() const { return recv_buf_size_; }

    inline int send_buf_capacity() const { return send_buf_capacity_; }
    inline int recv_buf_capacity() const { return recv_buf_capacity_; }

    inline void clear_send_buf() { send_buf_size_ = 0; }
    inline void clear_recv_buf() { recv_buf_size_ = 0; }

    void AddSendData(const char* buf, int buf_len);
    void AddRecvData(const char* buf, int buf_len);

    void SkipSendData(int len);
    void SkipRecvData(int len);
    void SkipRecvProto();

    bool IsRecvProtoReady();
    int GetRecvProtoLen();

    char* GetRemainSendBuf() { return send_buf_ + send_buf_size_; }
    int GetRemainSendBufLen() { return send_buf_capacity_ - send_buf_size_; } 
    void AddSendBufLen(int len) { send_buf_size_ += len; }

    char* GetRemainRecvBuf() { return recv_buf_ + recv_buf_size_; }
    int GetRemainRecvBufLen() { return recv_buf_capacity_ - recv_buf_size_; } 
    void AddRecvBufLen(int len) { recv_buf_size_ += len; }

private:
    void expand();

private:
    char* send_buf_;
    int send_buf_size_;
    int send_buf_capacity_;

    char* recv_buf_;
    int recv_buf_size_;
    int recv_buf_capacity_;
};

#endif
