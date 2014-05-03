/**
 * @file utils.h
 * @brief 工具类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#ifndef _UTILS_H
#define _UTILS_H

#include "comm_def.h"

class Utils {
public:
    static uint64_t Fnv64aHash(const char* buf, size_t len);
    static inline void BitSwap64(uint64_t* num, int m, int n) {
        // 如果两个比特位相同，自然就是返回原始的data，
        // 如果两个比特位不同，那么就是将原来的0变为1，1变为0
        if (m < 0 || m >63 || n < 0 || n > 63)
            return;
        if (((*num) & ((uint64_t)1 << m)) == ((*num) & ((uint64_t)1 << n)))
            return;
        else
            (*num) = (*num) ^ (((uint64_t)1 << m) | ((uint64_t)1 << n));
    }
    static uint64_t Rdtsc();
    static uint64_t GenUidNum();
    static void GenUidStr(std::string& uid_str);
    static void GenPassword8(std::string& password);

    template<class T>
    static void ToString(std::string& output, const T& t) {
        std::stringstream ss;
        ss << t;
        output = ss.str();
    }

    template<class T>
    static T ToNumber(std::string& input) {
        std::stringstream ss;
        ss << input;
        T number;
        ss >> number;
        return number;
    }

    static int32_t CheckAccount(const char* account, int32_t len);
    static int32_t CheckPassword(const char* password, int32_t len);

    static inline const char* GetMsgFromClient(const char* pkg_buf)
    {
        return pkg_buf + sizeof(uint16_t);
    }

    static inline int32_t GetMsgLenFromClient(const char* pkg_buf)
    {
        return (int32_t)ntohs(*(uint16_t*)pkg_buf) - sizeof(uint16_t);
    }

    static inline const char* GetMsgFromConn(const char* pkg_buf)
    {
        return pkg_buf + sizeof(ConnData);
    }

    static inline int32_t GetMsgLenFromConn(int32_t pkg_len)
    {
        return pkg_len - sizeof(ConnData);
    }

    static inline ConnData GetConnDataFromConn(const char* pkg_buf)
    {
        return *(ConnData*)pkg_buf;
    }
};

#endif
