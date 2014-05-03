/**
 * @file connection.cpp
 * @brief 连接缓存
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "connection.h"

int32_t Connection::AddRecvData(char* buf, int buf_len)
{
    if (recv_buf_size_ + buf_len > PKG_BUF_SIZE) {
        LOG(ERROR)
            << "player_idx[" << player_idx_
            << "] conn_fd[" << conn_fd_
            << "] recv_buf_size_ [" << recv_buf_size_
            << "] buf_len[" << buf_len
            << "] recv buf overflow!";
        return -1;
    }

    memcpy(recv_buf_ + recv_buf_size_, buf, buf_len);
    recv_buf_size_ += buf_len;

    return 0;
}


bool Connection::IsRecvProtoReady()
{
    if (recv_buf_size_ > (int32_t)(sizeof(uint16_t)*2) &&
        GetRecvProtoLen() <= recv_buf_size_)
        return true;
    else
        return false;
        
}

int32_t Connection::GetRecvProtoLen()
{
    return ntohs(*(uint16_t*)recv_buf_);
}

void Connection::RemoveRecvProto()
{
    recv_buf_size_ -= GetRecvProtoLen();
    memmove(recv_buf_, recv_buf_ + GetRecvProtoLen(), recv_buf_size_);
}

int32_t Connection::AddSendData(char* buf, int buf_len)
{
    if (send_buf_size_ + buf_len > PKG_BUF_SIZE) {
        LOG(ERROR)
            << "player_idx[" << player_idx_
            << "] conn_fd[" << conn_fd_
            << "] send_buf_size [" << send_buf_size_
            << "] buf_len[" << buf_len
            << "] send buf overflow!";
        return -1;
    }

    memcpy(send_buf_ + send_buf_size_, buf, buf_len);
    send_buf_size_ += buf_len;

    return 0;
}

void Connection::RemoveSendData(int32_t len)
{
    send_buf_size_ -= len;
    memmove(send_buf_, send_buf_ + len, send_buf_size_);
}
