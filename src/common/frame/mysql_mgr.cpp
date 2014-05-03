/**
 * @file mysql_mgr.cpp
 * @brief MySQL封装类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#include "mysql_mgr.h"

template <typename TYPE, void (TYPE::*thread_routine)() >
void* _thread_t(void* args)
{
    TYPE* This = (TYPE*)args;
    This->ThreadRoutine();
    return NULL;
}

MysqlClient::MysqlClient() :
    client_id_(0),
    host_(""),
    user_(""),
    password_(""),
    dbname_(""),
    running_(0),
    sql_(""),
    res_(NULL),
    callback_(NULL),
    seq_id_(0),
    player_idx_(0),
    mgr_(NULL)
{
    pipefd_[0] = 0;
    pipefd_[1] = 0;
}

MysqlClient::~MysqlClient()
{
    running_ = 0;
    pthread_join(thread_, NULL);

    if (res_ != NULL)
        mysql_free_result(res_);
    mysql_close(&conn_);
    close(pipefd_[0]);
    close(pipefd_[1]);

    LOG(INFO) << "MysqlClient[" << client_id_ << "] fini ok";
}

void MysqlClient::Init(int32_t client_id,
                       MysqlMgr* mgr,
                       const std::string& host,
                       const std::string& user,
                       const std::string& password,
                       const std::string& dbname)
{
    client_id_ = client_id;
    mgr_ = mgr;
    host_ = host;
    user_ = user;
    password_ = password;
    dbname_ = dbname;
    running_ = 1;

    CHECK(Connect() == 0)
        << "client[" << client_id_
        << "] Connect error!";

    CHECK(pipe(pipefd_) == 0)
        << "client[" << client_id_
        << "] create pipe error!";
    pthread_create(&thread_, NULL, _thread_t<MysqlClient, &MysqlClient::ThreadRoutine>, this);

    LOG(INFO) << "init client[" << client_id_ << "] ok";
}

void MysqlClient::Process(const std::string& sql, MysqlResCallback callback, uint64_t seq_id, int32_t player_idx)
{
    LOG(INFO) << "process[" << sql << "]";

    sql_ = sql;
    callback_ = callback,
    seq_id_ = seq_id;
    player_idx_ = player_idx;

    write(pipefd_[1], "", 1);
}

void MysqlClient::ThreadLoop()
{
    fd_set r_set;
    FD_ZERO(&r_set);
    FD_SET(pipefd_[0], &r_set);

    struct timeval time_val;
    time_val.tv_sec = 0;
    time_val.tv_usec = 10000;

    int ret = 0;
    ret = select(pipefd_[0] + 1, &r_set, NULL, NULL, &time_val);
    if (ret > 0 && FD_ISSET(pipefd_[0], &r_set))
    {
        char c;
        if (read(pipefd_[0], &c, 1) == 1) {
            if (Ping() != 0) {
                LOG(ERROR)
                    << "client[" << client_id_
                    << "] Ping error!";
                return;
            }

            if(mysql_real_query(&conn_, sql_.c_str(), sql_.length()) != 0) {
                int32_t err = mysql_errno(&conn_);
                if (err == ER_DUP_ENTRY || err == ER_DUP_KEYNAME) {
                    (*callback_)(res_, MYSQL_RES_DUP, seq_id_, player_idx_);
                } else {
                    LOG(ERROR)
                        << "client[" << client_id_
                        << "] mysql_real_query[" << sql_
                        << "] errno[" << mysql_errno(&conn_)
                        << "] error[" << mysql_error(&conn_) << "]";
                    mysql_close(&conn_);

                    if (Connect() != 0) {
                        LOG(ERROR)
                            << "client[" << client_id_
                            << "] Connect error";
                    }

                    if (mysql_real_query(&conn_, sql_.c_str(), sql_.length()) != 0) {
                        LOG(ERROR)
                            << "client[" << client_id_
                            << "] mysql_real_query[" << sql_
                            << "] errno[" << mysql_errno(&conn_)
                            << "] error[" << mysql_error(&conn_) << "]";
                        mysql_close(&conn_);
                        Connect();
                    }
                }
            } else {
                res_ = mysql_store_result(&conn_);
                if (callback_ != NULL && res_ != NULL) {
                    (*callback_)(res_, MYSQL_RES_OK, seq_id_, player_idx_);
                } else if (res_ == NULL) {
                    if(mysql_field_count(&conn_) == 0) {
                        LOG(INFO)
                            << "client[" << client_id_
                            << "] affected_rows[" <<  mysql_affected_rows(&conn_) << "]";
                        (*callback_)(res_, MYSQL_RES_OK, seq_id_, player_idx_);
                    } else {
                        LOG(ERROR)
                            << "client[" << client_id_
                            << "] mysql_store_result[" << sql_
                            << "] error[" << mysql_error(&conn_) << "]";
                        (*callback_)(res_, MYSQL_RES_FAILED, seq_id_, player_idx_);
                    }
                }
                res_ = NULL;
            }

            mgr_->PushFreeClient(this);
            LOG(INFO)
                << "client[" << client_id_
                << "] mysql_real_query [" << sql_ << "] end";
        };
    }

}

void MysqlClient::ThreadRoutine()
{
    while(running_) {
        ThreadLoop();
    }
    running_ = 0;
}

int32_t MysqlClient::Connect()
{
    if(mysql_init(&conn_) == NULL) {
        LOG(ERROR)
            << "client[" << client_id_
            << "] mysql_init error!";
        return -1;
    }

    char value = 1;
    my_bool reconnect = 1;
    if (mysql_options(&conn_, MYSQL_OPT_RECONNECT, (const char*)&value) != 0) {
        LOG(ERROR)
            << "client[" << client_id_
            << "] mysql_options MYSQL_OPT_REConnect error["
            << mysql_error(&conn_) << "]";
        mysql_close(&conn_);
        return -2;
    }

    if (mysql_options(&conn_, MYSQL_OPT_CONNECT_TIMEOUT, &reconnect) != 0) {
        LOG(ERROR)
            << "client[" << client_id_
            << "] mysql_options MYSQL_OPT_Connect_TIMEOUT error["
            << mysql_error(&conn_) << "]";
        mysql_close(&conn_);
        return -3;
    }

    if (mysql_options(&conn_, MYSQL_SET_CHARSET_NAME, "utf8") != 0) {
        LOG(ERROR)
            << "client[" << client_id_
            << "] mysql_options MYSQL_SET_CHARSET_NAME error["
            << mysql_error(&conn_) << "]";
        mysql_close(&conn_);
        return -4;
    }

    if (mysql_real_connect(&conn_, host_.c_str(),
        user_.c_str(), password_.c_str(),
        dbname_.c_str(), 0, NULL, 0) == NULL) {
        LOG(ERROR)
            << "client[" << client_id_
            << "] mysql_real_Connect error["
            << mysql_error(&conn_) << "]";
        mysql_close(&conn_);
        return -5;
    }

    LOG(INFO)
        << "client[" << client_id_
        << "] Connect ok";
    return 0;
}

int32_t MysqlClient::Ping()
{
    if (mysql_ping(&conn_) != 0) {
        LOG(ERROR)
            << "client[" << client_id_
            << "] Ping error!";
        return Connect();
    } else {
        return 0;
    }

    return 0;
}

MysqlMgr::MysqlMgr() :
    host_(""),
    user_(""),
    password_(""),
    dbname_("")
{
}

MysqlMgr::~MysqlMgr()
{
    for (MysqlClientList::iterator it = total_client_list_.begin();
         it != total_client_list_.end(); ++it) {
        delete *it;
    }
    LOG(INFO) << "MysqlMgr fini ok";
}

void MysqlMgr::Init(int32_t max_client_count,
                    const std::string& host,
                    const std::string& user,
                    const std::string& password,
                    const std::string& dbname)
{
    host_ = host;
    user_ = user;
    password_ = password;
    dbname_ = dbname;

    pthread_mutex_init(&lock_, NULL);

    if (max_client_count > E_MAX_MYSQL_CLIENT_COUNT)
        max_client_count = E_MAX_MYSQL_CLIENT_COUNT;
    if (max_client_count <= 0)
        max_client_count = 1;

    for(int32_t i = 0; i < max_client_count; i++) {
        MysqlClient* tmp = new MysqlClient();
        tmp->Init(i, this, host_, user_, password_, dbname_);
        total_client_list_.push_back(tmp);
        PushFreeClient(tmp);
    }

    LOG(INFO) << "init MysqlMgr ok";
}
