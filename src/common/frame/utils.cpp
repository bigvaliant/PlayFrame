/**
 * @file utils.cpp
 * @brief 工具类
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#include "utils.h"

uint64_t Utils::Fnv64aHash(const char* buf, size_t len)
{
    unsigned char* bp = (unsigned char*)buf; // start of buffer
    unsigned char* be = bp + len; // beyond end of buffer
    uint64_t hval = 0xcbf29ce484222325ULL;

    while (bp < be) {
        hval ^= (uint64_t) *bp++;
        hval += (hval << 1) + (hval << 4) + (hval << 5) +
                (hval << 7) + (hval << 8) + (hval << 40);
    }

    return hval;
}

uint64_t Utils::Rdtsc()
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

uint64_t Utils::GenUidNum()
{
    uint64_t rand_num = Rdtsc();
    BitSwap64(&rand_num, 2, 3);
    BitSwap64(&rand_num, 5, 7);
    BitSwap64(&rand_num, 11, 13);
    BitSwap64(&rand_num, 17, 19);
    BitSwap64(&rand_num, 23, 29);
    BitSwap64(&rand_num, 31, 37);
    BitSwap64(&rand_num, 41, 43);
    BitSwap64(&rand_num, 47, 53);
    BitSwap64(&rand_num, 59, 61);
    return rand_num;
}

void Utils::GenUidStr(std::string& uid_str)
{
    uint64_t uid = GenUidNum();
    uid_str = "";
    ToString(uid_str, uid);
}

void Utils::GenPassword8(std::string& password)
{
    std::stringstream ss;
    for (int i = 0; i < 8; i++) {
        int rand_num = rand() % 10 + '0';
        ss << (char)rand_num;
    }
    password = ss.str();
}

int32_t Utils::CheckAccount(const char* account, int32_t len)
{
    if (len < 6 || len > 32)
        return -1;

    int32_t i = 0;
    // 首字母必须为字母
    if (account[i] < 'A' ||
        (account[i] > 'Z' && account[i] < 'a') ||
        account[i] >'z')
    return -2;

    for (i = 1; i < len; i++) {
        if (account[i] < '0' ||
                (account[i] > '9' && account[i] < 'A') ||
                (account[i] > 'Z' && account[i] < 'a') ||
                account[i] >'z')
            return -3;
    }
    return 0;
}

int32_t Utils::CheckPassword(const char* password, int32_t len)
{
    if (len < 4 || len > 16)
        return -1;

    for (int32_t i = 0; i < len; i++) {
        if (password[i] < '0' ||
                (password[i] > '9' && password[i] < 'A') ||
                (password[i] > 'Z' && password[i] < 'a') ||
                password[i] >'z')
            return -2;
    }
    return 0;
}
