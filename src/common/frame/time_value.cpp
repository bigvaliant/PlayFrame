/**
 * @file time_value.cpp
 * @brief 时间封装类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#include "time_value.h"
#include <string.h>

TimeValue::TimeValue()
{
    Set(0, 0);
}

TimeValue::TimeValue(time_t sec, time_t usec)
{
    Set(sec, usec);
}

TimeValue::TimeValue(const struct timeval & tv)
{
    Set(tv);
}

void TimeValue::Set(time_t sec, time_t usec)
{
    tv_.tv_sec = sec;
    tv_.tv_usec = usec;
    Normalize();
}

void TimeValue::Set(const struct timeval & tv)
{
    tv_.tv_sec = tv.tv_sec;
    tv_.tv_usec = tv.tv_usec;
    Normalize();
}

time_t TimeValue::Sec() const
{
    return tv_.tv_sec;
}

void TimeValue::Sec(time_t sec)
{
    tv_.tv_sec = sec;
}

time_t TimeValue::Usec() const
{
    return tv_.tv_usec;
}

void TimeValue::Usec(time_t usec)
{
    tv_.tv_usec = usec;
}

TimeValue& TimeValue::operator += (const TimeValue& tv)
{
    tv_.tv_sec += tv.Sec();
    tv_.tv_usec += tv.Usec();
    Normalize();
    return *this;
}

TimeValue& TimeValue::operator += (time_t t)
{
    Sec(tv_.tv_sec + t);
    return *this;
}

TimeValue& TimeValue::operator = (const TimeValue& tv)
{
    tv_.tv_sec = tv.Sec();
    tv_.tv_usec = tv.Usec();
    return *this;
}

TimeValue& TimeValue::operator = (time_t t)
{
    tv_.tv_sec = t;
    tv_.tv_usec = 0;
    return *this;
}

TimeValue& TimeValue::operator -= (const TimeValue& tv)
{
    tv_.tv_sec -= tv.Sec();
    tv_.tv_usec -= tv.Usec();
    Normalize();
    return *this;
}

TimeValue& TimeValue::operator -= (time_t t)
{
    tv_.tv_sec -= t;
    return *this;
}

TimeValue::operator timespec_t() const
{
    timespec_t tv;
    tv.tv_sec = tv_.tv_sec;
    tv.tv_nsec = tv_.tv_usec * 1000;
    return tv;
}

TimeValue TimeValue::Time()
{
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if(gettimeofday(&tv, NULL) == -1)
    {
        return TimeValue(-1);
    }

    return TimeValue(tv);
}

const char* TimeValue::TimeName(time_t t)
{
    static char time_stamp[64] = {0};
    strftime(time_stamp, sizeof(time_stamp),
        "%m-%d-%Y %H:%M:%S", localtime(&t));
    return (const char*)time_stamp;
}

const char* TimeValue::CurTimeName()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return TimeValue::TimeName(tv.tv_sec);
}

// ReaD Time Stamp Count
uint64_t TimeValue::Rdtsc()
{
#if (defined _MSC_VER && (defined _M_IX86 || defined _M_X64))
    return __rdtsc ();
#elif (defined __GNUC__ && (defined __i386__ || defined __x86_64__))
    uint32_t low, high;
    __asm__ volatile ("rdtsc" : "=a" (low), "=d" (high));
    return (uint64_t) high << 32 | low;
#elif (defined __SUNPRO_CC && (__SUNPRO_CC >= 0x5100) && (defined __i386 || \
    defined __amd64 || defined __x86_64))
    union {
        uint64_t u64val;
        uint32_t u32val [2];
    } tsc;
    asm("rdtsc" : "=a" (tsc.u32val [0]), "=d" (tsc.u32val [1]));
    return tsc.u64val;
#elif defined(__s390__)
    uint64_t tsc;
    asm("\tstck\t%0\n" : "=Q" (tsc) : : "cc");
    tsc >>= 12;		/* convert to microseconds just to be consistent */
    return(tsc);
#else
    return 0;
#endif
}

uint64_t TimeValue::NowUs()
{
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if (gettimeofday(&tv, NULL) == -1) {
        return 0;
    }

    return (tv.tv_sec * (uint64_t)1000000 + tv.tv_usec);
}

void TimeValue::Normalize()
{
    CHECK((tv_.tv_usec) * ONE_SECOND_IN_USECS + tv_.tv_usec >= 0)
        << "TimeValue < 0";

    if (tv_.tv_usec >= ONE_SECOND_IN_USECS) {
        time_t now_usec = (tv_.tv_usec) % ONE_SECOND_IN_USECS;
        time_t add_sec = (tv_.tv_usec) / ONE_SECOND_IN_USECS;
        tv_.tv_sec += add_sec;
        tv_.tv_usec = now_usec;
    } else if (tv_.tv_usec <= -ONE_SECOND_IN_USECS) {
        time_t now_usec = (-tv_.tv_usec) % ONE_SECOND_IN_USECS;
        time_t sub_sec = (-tv_.tv_usec) / ONE_SECOND_IN_USECS;
        tv_.tv_sec -= sub_sec;
        tv_.tv_usec = -now_usec;
    }

    if(tv_.tv_usec < 0) {
        --( tv_.tv_sec );
        tv_.tv_usec += ONE_SECOND_IN_USECS;
    }
}



