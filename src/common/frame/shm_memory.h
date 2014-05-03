/**
 * @file shm_memory.h
 * @brief 共享内存分配器
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#ifndef _SHM_MEMORY_H_
#define _SHM_MEMORY_H_

#include "comm_def.h"

class ShmMemory
{
public:
    DISALLOW_COPY_AND_ASSIGN(ShmMemory);
    explicit ShmMemory(bool verify = true);
    ~ShmMemory();

    void* Init(key_t shm_key, int32_t chk_id, int32_t alloc_size, int32_t& first_time);
    void* BaseAddr() const;
    int32_t PoolSize() const;

private:
    struct SHM_HEAD {
        key_t       shm_key;
        int32_t     shm_id;
        int32_t     chk_id;
    };

    bool        verify_;
    SHM_HEAD*   shm_head_;
    int32_t     pool_size_;
    void*       base_addr_;
};

#endif // _SHM_MEMORY_H_

