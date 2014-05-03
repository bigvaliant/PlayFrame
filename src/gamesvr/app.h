/**
 * @file app.h
 * @brief App实现
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _APP_H_
#define _APP_H_

#include "app_def.h"

extern ServerFrame* g_svr_frame;

class App: public AppBase
{
public:
    App() {};
    virtual ~App() { instance_ = NULL; }

    virtual const char* AppName() const;
    virtual int32_t AppInit();
    virtual int32_t AppRun();
    virtual void AppClean();
    virtual void AppReload();

    static App* instance_;
};

#endif

