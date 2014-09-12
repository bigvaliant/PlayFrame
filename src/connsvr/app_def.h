/**
 * @file app_def.h
 * @brief app基本定义 
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _APP_DEF_H_
#define _APP_DEF_H_
// frame
#include "comm_def.h"
#include "shm_pool.h"
#include "time_value.h"
#include "app_base.h"
#include "server_frame.h"
#include "lua_config.h"
#include "epoller.h"
#include "utils.h"

#define PKG_BUF_SIZE 10240
#define LOOP_TIMES 1000

// 前置声明
class App;

enum {
    MODULE_ID_CONFIG = 0x9fff,
    MODULE_ID_EVENT,
    MODULE_ID_CONN_MGR,
};

#endif
