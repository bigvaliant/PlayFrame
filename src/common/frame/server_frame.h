/**
 * @file server_frame.h
 * @brief 服务器框架类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#ifndef _SERVER_FRAME_H_
#define _SERVER_FRAME_H_

#include "comm_def.h"
#include "app_base.h"

#define FILE_NAME_SIZE   128
#define EXE_FILE_PATH    128

/**
* @brief 服务器基础框架
*/
class ServerFrame
{
public:
    DISALLOW_COPY_AND_ASSIGN(ServerFrame);

    ServerFrame(const std::string& svr_ver="1.0.0");
    virtual ~ServerFrame();

    virtual int32_t Run(int32_t argc, char* argv[]);
    virtual int32_t InitFrame(int32_t argc, char* argv[]);
    virtual void RunFrame();
    virtual void FiniFrame();
    static const AppBase* GetApp() { return app_; }

protected:
    virtual void ShowHelp(const char* app_name);
    virtual int32_t ParseParam(int32_t argc, char* argv[]);
    static pid_t GetRunningPid();
    int32_t CheckForPidFile();
    void InitLog(const char* argv0);

private:
    static int32_t CreatePidFile();
    static void KillApp();
    static void ReloadApp();
    static void SetSignal();
    static void RemovePidFile();
    static void SignalHandler(int32_t signo);
    static void Daemonize();

    static AppBase*     app_;
    static char         pid_file_[FILE_NAME_SIZE];
    static bool         is_shutdown_;
    static int32_t      sigleton_;

public:
    uid_t               uid_;
    gid_t               gid_;

    std::string         server_name_;
    std::string         server_version_;
    bool                is_daemon_;

    char                log_path_[FILE_NAME_SIZE];
    char                log_name_[FILE_NAME_SIZE];
    int32_t             log_level_;
    char                conf_name_[FILE_NAME_SIZE];
    char                script_name_[FILE_NAME_SIZE];
};

#endif 
