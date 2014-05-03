/**
 * @file shm_memory.cpp
 * @brief 共享内存分配器
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#include "shm_memory.h"

ShmMemory::ShmMemory(bool verify)
    : verify_(verify),
      shm_head_(NULL),
      pool_size_(0),
      base_addr_(NULL)
{}

ShmMemory::~ShmMemory()
{
    if(shm_head_ != NULL)
    {
        shmdt((const void*)shm_head_);
		shm_head_ = NULL;
        base_addr_ = NULL;
    }
}

void* ShmMemory::Init(key_t shm_key, int32_t chk_id,
    int32_t alloc_size, int32_t& first_time)
{
    int32_t page_size = getpagesize();
    alloc_size += sizeof(SHM_HEAD);
    pool_size_ = ROUNDUP(alloc_size, page_size);

    int32_t shm_id = shmget(shm_key, pool_size_, 0666 | IPC_CREAT | IPC_EXCL);

    if(shm_id == -1) {
        if( errno != EEXIST ) {
            return NULL;
        }

        first_time = 0;

        shm_id = shmget(shm_key, 0, 0666);
        shm_head_ = (SHM_HEAD*)shmat(shm_id, 0, 0);

        if(verify_ && shm_head_->shm_key != shm_key
            && shm_head_->chk_id != chk_id) {
            shmdt((const void* )shm_head_);
            shm_head_ = NULL;
            return NULL;
        }
    } else {
        first_time = 1;
        shm_head_ = (SHM_HEAD*)shmat(shm_id, 0, 0);
        shm_head_->shm_key = shm_key;
        shm_head_->shm_id = shm_id;
        shm_head_->chk_id = chk_id;
    }

    base_addr_ = (void*)(((char*)shm_head_) + sizeof(SHM_HEAD));
    return base_addr_;
}

void* ShmMemory::BaseAddr() const
{
    return base_addr_;
}

int32_t ShmMemory::PoolSize() const
{
    if(shm_head_ != NULL)
        return pool_size_;
    return 0;
}


