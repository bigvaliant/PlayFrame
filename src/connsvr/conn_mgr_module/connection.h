/**
 * @file connection.h
 * @brief 连接缓存
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "app_def.h"

class Connection
{
public:
    Connection()
    {
        player_idx_ = 0;
        conn_fd_ = 0;
        recv_buf_[0] = '\0';
        recv_buf_size_ = 0;
        send_buf_[0] = '\0';
        send_buf_size_ = 0;
    }
    ~Connection() {}

    void Init() { memset(this, 0, sizeof(*this)); }

    inline void set_player_idx(int32_t player_idx) { player_idx_ = player_idx; }
    inline int32_t player_idx() const { return player_idx_; }

    inline void set_conn_fd(int32_t conn_fd) { conn_fd_ = conn_fd; }
    inline int32_t conn_fd() const { return conn_fd_; }

    inline const char* send_buf() const { return send_buf_; }
    inline const char* recv_buf() const { return recv_buf_; }

    inline int32_t send_buf_size() const { return send_buf_size_; }
    inline int32_t recv_buf_size() const { return recv_buf_size_; }

    int32_t AddRecvData(char* buf, int buf_len);
    bool IsRecvProtoReady();
    int32_t GetRecvProtoLen();
    void RemoveRecvProto();
    char*  GetRemainSendBuf() { return send_buf_ + send_buf_size_; }
    int32_t GetRemainSendBufSize() { return PKG_BUF_SIZE - send_buf_size_; }
    int32_t AddSendData(char* buf, int buf_len);
    void RemoveSendData(int len);
    void ClearSendData() { send_buf_size_ = 0; }

private:
    int32_t         player_idx_;
    int32_t         conn_fd_;
    char            recv_buf_[PKG_BUF_SIZE];
    int32_t         recv_buf_size_;
    char            send_buf_[PKG_BUF_SIZE];
    int32_t         send_buf_size_;
};

#endif // _CONNECTON_H_
