/**
 * @file mysql_mgr.h
 * @brief MySQL封装类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#ifndef _MYSQL_MGR_H_
#define _MYSQL_MGR_H_

#include "comm_def.h"
#include <mysql.h>
#include <mysqld_error.h>
#include <pthread.h>

typedef void (*MysqlResCallback)(MYSQL_RES* res, int32_t flags, uint64_t seq_id, int32_t player_idx);

enum MYSQL_RES_FLAGS {
    MYSQL_RES_OK = 0,
    MYSQL_RES_FAILED = -1,
    MYSQL_RES_DUP = -2,
};

class MysqlClient;
class MysqlMgr;

/**
* @brief Mysql连接客户端
*/
class MysqlClient
{
public:
    DISALLOW_COPY_AND_ASSIGN(MysqlClient);

    MysqlClient();
    ~MysqlClient();

    void Init(int32_t client_id, MysqlMgr* mgr,
        const std::string& host, const std::string& user,
        const std::string& password, const std::string& dbname);

    MYSQL* GetConn() {
        return &conn_;
    }

    void Process(const std::string& sql, MysqlResCallback callback, uint64_t seq_id, int32_t player_idx = 0);
    void ThreadRoutine();
    void ThreadLoop();

private:
    int32_t Ping();
    int32_t Connect();

private:
    int32_t             client_id_;

    MYSQL               conn_;
    std::string         host_;
    std::string         user_;
    std::string         password_;
    std::string         dbname_;
    pthread_t           thread_;
    int                 pipefd_[2];
    int                 running_;

    std::string         sql_;

    MYSQL_RES*          res_;

    MysqlResCallback    callback_;
    uint64_t            seq_id_;
    int32_t             player_idx_;

    MysqlMgr*           mgr_;
};

/**
* @brief MysqlClient管理类
*/
class MysqlMgr
{
public:
    enum { E_MAX_MYSQL_CLIENT_COUNT = 8, };
    MysqlMgr();
    ~MysqlMgr();

    void Init(int32_t max_client_count, const std::string& host,
        const std::string& user, const std::string& password,
        const std::string& dbname);

    void PushFreeClient(MysqlClient* client) {
        LockFreeClientList();
        free_client_list_.push_back(client);
        UnlockFreeClientList();
    }

    MysqlClient* PopFreeClient() {
        MysqlClient* tmp = NULL;
        LockFreeClientList();
        if (free_client_list_.empty() != true) {
            tmp = free_client_list_.front();
            free_client_list_.pop_front();
        }
        UnlockFreeClientList();

        return tmp;
    }

private:
    void LockFreeClientList() {
        pthread_mutex_lock(&lock_);
    }

    void UnlockFreeClientList() {
        pthread_mutex_unlock(&lock_);
    }

private:
    std::string         host_;
    std::string         user_;
    std::string         password_;
    std::string         dbname_;

    pthread_mutex_t     lock_;

    typedef std::list<MysqlClient*> MysqlClientList;
    MysqlClientList free_client_list_;
    MysqlClientList total_client_list_;
};

#endif
