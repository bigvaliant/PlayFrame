/**
 * @file comm_def.h
 * @brief 基础定义
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */

#ifndef _COMM_DEF_H_
#define _COMM_DEF_H_

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <getopt.h>
#include <libgen.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <iostream>
#include <string>
#include <sstream>
#include <list>
#include <vector>
#include <map>
#include <queue>

// 开源组件
#include "logging.h"
#include "zmq.h"
extern "C"
{
	#include "lua.h"
	#include "lualib.h"
	#include "lauxlib.h"
};

// ConnData
enum ConnCmd {
    CONN_START = 1,
    CONN_PROC = 2,
    CONN_STOP = 3,
};

#pragma pack(push)
#pragma pack(4)
typedef struct tagConnData {
    int32_t conn_fd;
    int32_t player_idx;
    int32_t conn_cmd;
} ConnData;
#pragma pack(pop)

const int CONN_DATA_SIZE = sizeof(ConnData);
const int UINT16_SIZE = sizeof(uint16_t);

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
            TypeName(const TypeName&); \
            TypeName& operator=(const TypeName&)

#define ROUNDUP(n, width) (((n) + (width) - 1) & ~((width) - 1))

#define ROUNDDOWN(n, width) ((n) & ~(-width))

// UP(A/B) = int((A+B-1)/B)
#define DIV_ROUND_UP(n, d) (((n) + (d) - 1) / (d))

#define UNUSE_ARG(arg) ((void)arg)

#endif
