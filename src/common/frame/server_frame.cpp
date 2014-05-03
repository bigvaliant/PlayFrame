/**
 * @file server_frame.cpp
 * @brief 服务器框架类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#include "server_frame.h"

AppBase*    ServerFrame::app_ = NULL;
char        ServerFrame::pid_file_[FILE_NAME_SIZE] = {0};
bool        ServerFrame::is_shutdown_ = false;
int32_t     ServerFrame::sigleton_ = 0;

void ServerFrame::Daemonize()
{
    // 设置忽略信号
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    // SID PGID PPID PID
    // 父进程的PPID和SID都跟bash相同，因为是bash创建的
    // 父进程的PGID和PID相同，因为父进程是GROUP组长
    // 父进程退出
    if (fork() != 0)
        exit(0);
    // fork后子进程创建新的PID 但是PGID继承自父进程

    // 创建新的session 赋值新的SID
    // 必须PGID!=PID 否则会失败，因为GROUP组长创建新的session后
    // 该GROUP下的其他进程都属于老的session，但是PGID不知道怎么去改变
    if (setsid() == -1)
        exit(0);

    signal(SIGHUP, SIG_IGN);
    umask(0);

    if (fork() != 0)
        exit(0);

    // 如果不需要关闭STDIN STDOUT STDERR, close fd from 3
    // getdtablesize 返回最大fd + 1
    int32_t fd = open( "/dev/null", O_RDWR );
    for (int32_t i = 0; i < getdtablesize(); i++) {
        close(i);
        dup2(fd, i);
    }
    close(fd);
}

void ServerFrame::RemovePidFile()
{
    int32_t result = -1;
    do {
        result = unlink(pid_file_);
    } while (errno == EINTR && result == -1);

    return;
}

void ServerFrame::SignalHandler(int32_t signo)
{
    fprintf(stderr, "\nrecv signal\n");
    if (SIGRTMAX-1== signo) {
        fprintf(stderr, "recv stop signal\n");
        is_shutdown_ = true;
    } else if(SIGRTMAX-2 == signo) {
        fprintf(stderr, "recv reload signal\n");
        app_->AppReload();
    }

    return;
}

int32_t ServerFrame::CreatePidFile()
{
    pid_t proc_id = getpid();
    fprintf(stderr, "pid = %d\n", proc_id);
    char pid_name[16] = {0};
    int32_t len = snprintf(pid_name, 16, "%d", proc_id);
    int32_t fd = ::open(pid_file_, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR);
    if(fd != -1 && ::write(fd, (const void*)pid_name, len)) {
        close(fd);
        return 0;
    }

    return -1;
}

pid_t ServerFrame::GetRunningPid()
{
    int32_t fd = ::open(pid_file_, O_RDONLY);
    if (fd < 0)
        return 0;

    pid_t proc_id = 0;
    char pid_name[16] = {0};
    size_t pidbuf_len = ::read(fd, (void*)pid_name, 16);
    if (pidbuf_len == 0) {
        ::close(fd);
        return 0;
    }

    pid_name[pidbuf_len] = 0;
    proc_id = strtol(pid_name, NULL, 10);
    ::close(fd);

    return proc_id;
}

int32_t ServerFrame::CheckForPidFile()
{
    pid_t proc_id = GetRunningPid();
    if(proc_id <= 0)
        return 0;

    char exe_path[EXE_FILE_PATH] = {0};
    size_t exe_pathlen = EXE_FILE_PATH;
    snprintf(exe_path, exe_pathlen, "/proc/%d/cwd/%s", proc_id, server_name_.c_str());
    fprintf(stderr, "path = %s\n", exe_path);
    if (access(exe_path, F_OK) < 0)
        return 0;

    return -1;
}

void ServerFrame::KillApp()
{
    if (access(pid_file_, F_OK) == 0) {
        pid_t proc_id = GetRunningPid();
        fprintf(stderr, "kill running svr pid:%d\n", proc_id);
        kill(proc_id, SIGRTMAX-1);
    } else {
        fprintf(stderr, "Pid File %s NOT exist!\n", pid_file_);
    }
}

void ServerFrame::ReloadApp()
{
    if (access(pid_file_, F_OK) == 0) {
        pid_t proc_id = GetRunningPid();;
        fprintf(stderr, "reload running svr pid:%d\n", proc_id);
        kill(proc_id, SIGRTMAX-2);
    } else {
        fprintf(stderr, "Pid File %s NOT exist!\n", pid_file_);
    }
}

void ServerFrame::SetSignal()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));

    // 忽略
    act.sa_handler = SIG_IGN;
    sigaction(SIGINT, &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGPIPE, &act, NULL);
    sigaction(SIGHUP,  &act, NULL);
    sigaction(SIGCHLD, &act, NULL);
    sigaction(SIGALRM, &act, NULL);
    sigaction(SIGTRAP, &act, NULL);

    act.sa_handler = SignalHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    // kill
    sigaction(SIGRTMAX-1, &act, NULL);
    // reload
    sigaction(SIGRTMAX-2, &act, NULL);

    return;
}

ServerFrame::ServerFrame(const std::string& svr_ver/*="1.0.0"*/)
{
    sigleton_++;
    if (sigleton_ >= 2)
        fprintf(stderr, "must only one ServerFrame instance\n");
    assert(sigleton_ < 2);

    server_version_ = svr_ver;

    is_daemon_ = false;
    // INFO 0
    // WARNING 1
    // ERROR 2
    // FATAL 3
    log_level_ = 0;
    memset(log_path_, 0, sizeof(log_path_));
    memset(log_name_, 0, sizeof(log_name_));
    memset(conf_name_, 0, sizeof(conf_name_));
    memset(script_name_, 0, sizeof(script_name_));
}

ServerFrame::~ServerFrame()
{
}

void ServerFrame::ShowHelp(const char* AppName)
{
    fprintf(stderr, "Usage:%s -d --daemon "
        "-c --conf-file "
        "-s --script-file "
        "-p --log-path "
        "-l --log-level\n",
        AppName);
}

int32_t ServerFrame::ParseParam(int32_t argc, char* argv[])
{

    static struct option long_options[] = {
        { "daemon",             0, NULL, 'd' },
        { "D",                  0, NULL, 'd' },
        { "conf-file",          1, NULL, 'c' },
        { "script-file",        1, NULL, 's' },
        { "log-path",           1, NULL, 'p' },
        { "log-level",          1, NULL, 'l' },
        { 0, 0, 0, 0 }
    };

    int32_t opt;
    while ((opt = getopt_long(argc, argv, "dc:s:p:l:D", long_options, NULL)) != -1) {
        switch (opt) {
            case 'D':
            case 'd': {
                is_daemon_ = true;
                break;
            }
            case 'c': {
                strcpy(conf_name_, optarg);
                break;
            }
            case 's': {
                strcpy(script_name_, optarg);
                break;
            }
            case 'p': {
                strcpy(log_path_, optarg);
                break;
            }
            case 'l': {
                if (strcmp(optarg, "info") == 0) {
                    log_level_ = 0;
                } else if (strcmp(optarg, "warning") == 0) {
                    log_level_ = 1;
                } else if (strcmp(optarg, "error") == 0) {
                    log_level_ = 2;
                } else if (strcmp(optarg, "fatal") == 0) {
                    log_level_ = 4;
                } else {
                    log_level_ = 0;
                }
                break;
            }
        case '?':
        default:
            {
                ShowHelp(argv[0]);
                continue;
            }
        }
    }

    snprintf(log_name_, FILE_NAME_SIZE-1, "%s/%s", log_path_, server_name_.c_str());

    return 0;
}

void ServerFrame::InitLog(const char* argv0)
{
    (void)argv0;
    google::InitGoogleLogging("");
    // 按日志级别输出到不同文件
    FLAGS_servitysinglelog = false;
    // 最小输出日志级别
    FLAGS_minloglevel = log_level_;
    // 输出FATAL基本到stderr
    FLAGS_stderrthreshold = 3;

    char tmp_log_name[FILE_NAME_SIZE];
    // 设置 google::FATAL 级别的日志存储路径和文件名前缀
    snprintf(tmp_log_name, FILE_NAME_SIZE-1, "%s_fatal_", log_name_);
    google::SetLogDestination(google::GLOG_FATAL, tmp_log_name);
    //设置 google::ERROR 级别的日志存储路径和文件名前缀
    snprintf(tmp_log_name, FILE_NAME_SIZE-1, "%s_error_", log_name_);
    google::SetLogDestination(google::GLOG_ERROR, tmp_log_name);
    //设置 google::WARNING 级别的日志存储路径和文件名前缀
    snprintf(tmp_log_name, FILE_NAME_SIZE-1, "%s_warning_", log_name_);
    google::SetLogDestination(google::GLOG_WARNING, tmp_log_name);
    //设置 google::INFO 级别的日志存储路径和文件名前缀
    snprintf(tmp_log_name, FILE_NAME_SIZE-1, "%s_info_", log_name_);
    google::SetLogDestination(google::GLOG_INFO, tmp_log_name);

    //缓冲日志输出，默认为30秒，此处改为立即输出
    FLAGS_logbufsecs = 0;
    //最大日志大小为 100MB
    FLAGS_max_log_size = 100;
    //当磁盘被写满时，停止日志输出
    FLAGS_stop_logging_if_full_disk = true;
    //捕捉 core dumped (linux)
    //google::InstallFailureSignalHandler();
    //默认捕捉 SIGSEGV 信号信息输出会输出到 stderr (linux)
    //google::InstallFailureWriter(NULL);
}

int32_t ServerFrame::InitFrame(int32_t argc, char* argv[])
{
    server_name_ = argv[0];
    snprintf(pid_file_, FILE_NAME_SIZE, "%s.pid", argv[0]);

    ParseParam(argc, argv);

    if (strcmp(argv[argc-1], "stop") == 0) {
        KillApp();
        exit(0);
    } else if (strcmp(argv[argc-1], "reload") == 0) {
        ReloadApp();
        exit(0);
    } else if (strcmp( argv[argc-1], "clean") == 0) {
        app_->AppClean();
        if (app_ != NULL)
            delete app_;
        app_ = NULL;
        exit(0);
    } else if (strcmp(argv[argc-1], "start") == 0) {
        if (is_daemon_) {
            Daemonize();
        }

        uid_ = getuid();
        gid_ = getgid();

        if (CheckForPidFile() < 0) {
            fprintf(stderr, "%s is running! please stop it!\n", server_name_.c_str());
            return -1;
        }

        if (CreatePidFile() < 0) {
            fprintf(stderr, "create %s file failed!\n", pid_file_);
            return -1;
        }

        // 需要root权限 修改服务连接数
        struct rlimit rlim;
        if (getrlimit(RLIMIT_NOFILE, &rlim) == 0) {
            rlim.rlim_cur = rlim.rlim_max;
            setrlimit(RLIMIT_NOFILE, &rlim);
            rlim.rlim_cur = 200000;
            rlim.rlim_max = 200000;
            setrlimit(RLIMIT_NOFILE, &rlim);
        }

        // 注册程序正常终止时要被调用的函数
        atexit(RemovePidFile);

        SetSignal();

        // 启动日志
        InitLog(argv[0]);

        // 加载app
        CHECK((app_ = AppBase::GetInstance()) != NULL)
            << "create app instance failed";

        LOG(INFO) << "\n\n----------------------------------------------";
        LOG(INFO) << server_name_ << " now starting...";

        // 初始化随机数
        srand(time(NULL));

        if (app_->AppInit() < 0) {
            LOG(ERROR)
                << "app [" << app_->AppName()
                << "] init failed";
            return -1;
        }
        return 0;
    }

    fprintf(stderr, "cmd: [start] [stop] [reload] [clean]\n");
    return -1;
}

void ServerFrame::RunFrame()
{
    CHECK(app_ != NULL)
        << "app [" << app_->AppName()
        <<  "] is not init";

    LOG(INFO) << "Svr[" << app_->AppName() << "] is running";

    while (is_shutdown_ == false) {
        app_->AppRun();
        usleep(100000);
    }

    LOG(INFO) << "Svr[" << server_name_ <<  "] is about to exit";
}

void ServerFrame::FiniFrame()
{
    if(app_) {
        app_->AppStop();
        if (app_ != NULL)
            delete app_;
        app_ = NULL;
        google::ShutdownGoogleLogging();
    }
}

int32_t ServerFrame::Run(int32_t argc, char* argv[])
{
    if (InitFrame(argc, argv) < 0) {
        fprintf( stderr, "%s frame init failed\n", server_name_.c_str());
        FiniFrame();
        return -1;
    }

    RunFrame();

    FiniFrame();

    fprintf(stderr, "%s exit!\n", server_name_.c_str());
    return 0;
}
