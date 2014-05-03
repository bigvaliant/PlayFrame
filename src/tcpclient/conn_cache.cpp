/**
 * @file conn_cache.cpp
 * @brief 连接缓存
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "conn_cache.h"


ConnCache::ConnCache()
{
    send_buf_ = NULL;
    send_buf_size_ = 0;
    send_buf_capacity_ = 0;

    recv_buf_ = NULL;
    recv_buf_size_ = 0;
    recv_buf_capacity_ = 0;
}
    
void ConnCache::Init() {
    send_buf_ = (char*)malloc(CONN_CACHE_BUF_LEN);
    assert(send_buf_ != NULL);
    send_buf_capacity_ = CONN_CACHE_BUF_LEN;

    recv_buf_ = (char*)malloc(CONN_CACHE_BUF_LEN);
    assert(recv_buf_ != NULL);
    recv_buf_capacity_ = CONN_CACHE_BUF_LEN;
}

void ConnCache::expand()
{
    // 成倍扩展send_buf
    if (send_buf_capacity_ == 0) {
        send_buf_ = (char*)malloc(CONN_CACHE_BUF_LEN);
        assert(send_buf_ != NULL);
        send_buf_capacity_ = CONN_CACHE_BUF_LEN;
    } else {
        char* temp = (char*)malloc(send_buf_capacity_*2);
        assert(temp != NULL);
        memcpy(temp, send_buf_, send_buf_size_);
        free(send_buf_);
        send_buf_ = temp;
        temp = NULL;
        send_buf_capacity_ *= 2;
    }

    // 成倍扩展recv_buf
    if (recv_buf_capacity_ == 0) {
        recv_buf_ = (char*)malloc(CONN_CACHE_BUF_LEN);
        assert(recv_buf_ != NULL);
        recv_buf_capacity_ = CONN_CACHE_BUF_LEN;
    } else {
        char* temp = (char*)malloc(recv_buf_capacity_*2);
        assert(temp != NULL);
        memcpy(temp, recv_buf_, recv_buf_size_);
        free(recv_buf_);
        recv_buf_ = temp;
        temp = NULL;
        recv_buf_capacity_ *= 2;
    }
}

void ConnCache::AddSendData(const char* buf, int buf_len)
{
    while (send_buf_size_ + buf_len > send_buf_capacity_) {
        expand();
    }

    memcpy(send_buf_ + send_buf_size_, buf, buf_len);
    send_buf_size_ += buf_len;
}

void ConnCache::AddRecvData(const char* buf, int buf_len)
{
    while (recv_buf_size_ + buf_len > recv_buf_capacity_) {
        expand();
    }

    memcpy(recv_buf_ + recv_buf_size_, buf, buf_len);
    recv_buf_size_ += buf_len;
}

void ConnCache::SkipSendData(int len)
{
    send_buf_size_ -= len;
    memmove(send_buf_, send_buf_ + len, send_buf_size_);
}

void ConnCache::SkipRecvData(int len)
{
    recv_buf_size_ -= len;
    memmove(recv_buf_, recv_buf_ + len, recv_buf_size_);
}

void ConnCache::SkipRecvProto()
{
    int len = (int)ntohs(*(uint16_t*)recv_buf_);
    recv_buf_size_ -= len;
    memmove(recv_buf_, recv_buf_ + len, recv_buf_size_);
}

bool ConnCache::IsRecvProtoReady()
{
    int proto_len = (int)ntohs(*(uint16_t*)recv_buf_);
    if (recv_buf_size_ > (int)sizeof(uint16_t) &&
        proto_len <= recv_buf_size_)
        return true;
    else
        return false;
}

int ConnCache::GetRecvProtoLen()
{
    int len = (int)ntohs(*(uint16_t*)recv_buf_);
    return len;
}

