/**
 * @file time_value.h
 * @brief 时间封装类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#ifndef _TIME_VALUE_H_
#define _TIME_VALUE_H_

#include "comm_def.h"

time_t const ONE_SECOND_IN_USECS = 1000000;

typedef struct timespec timespec_t;

class TimeValue
{
public:
    TimeValue();
    explicit TimeValue(time_t sec, time_t usec = 0);
    explicit TimeValue(const struct timeval& tv);
    void Set(time_t sec, time_t usec);
    void Set(const struct timeval& tv);
    time_t Sec() const;
    void Sec(time_t sec);
    time_t Usec() const;
    void Usec(time_t usec);
    TimeValue& operator += (const TimeValue& tv);
    TimeValue& operator += (time_t t);
    TimeValue& operator = (const TimeValue& tv);
    TimeValue& operator = (time_t t);
    TimeValue& operator -= (const TimeValue& tv);
    TimeValue& operator -= (time_t t);
    operator timespec_t() const;
    static TimeValue Time();
    static const char* TimeName(time_t t);
    static const char* CurTimeName();
    static uint64_t Rdtsc();
    static uint64_t NowUs();

    friend inline bool operator > (const TimeValue& left, const TimeValue& right)
    {
        if(left.tv_.tv_sec == right.tv_.tv_sec)
            return left.tv_.tv_usec > right.tv_.tv_usec;
        return left.tv_.tv_sec > right.tv_.tv_sec;
    }

    friend inline TimeValue operator + (const TimeValue& left, const TimeValue& right)
    {
        TimeValue temp = left;
        temp += right;
        return temp;
    }

    friend inline TimeValue operator - (const TimeValue& left, const TimeValue& right)
    {
        TimeValue temp = left;
        temp -= right;
        return temp;
    }

    friend inline bool operator == (const TimeValue& left, const TimeValue& right)
    {
        return (left.tv_.tv_sec == right.tv_.tv_sec &&
                 left.tv_.tv_usec == right.tv_.tv_usec);
    }

private:
    void Normalize();

    struct timeval tv_;
};

#endif // _TIME_VALUE_H_

