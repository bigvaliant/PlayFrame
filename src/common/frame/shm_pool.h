/**
 * @file shm_pool.h
 * @brief 共享内存池
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#ifndef _SHM_POOL_H_
#define _SHM_POOL_H_

#include "shm_memory.h"

template< class T >
class ShmPool {
public:
    DISALLOW_COPY_AND_ASSIGN(ShmPool);
    typedef struct tagPoolUnit {
        T       mem;
        int32_t pos;
    } PoolUnit;

    typedef struct tagPoolCtrl {
        int32_t         free_mem_pos;
        int32_t         valid_unit_count;
        int32_t         total_unit_count;
        int32_t         unit_size;
        PoolUnit*       pool;
    } PoolCtrl;

    typedef void (*FuncRestore)(T* obj, void* arg);

    explicit ShmPool()
        : pool_ctrl_(NULL),
          shm_(false)
    {}

    ~ShmPool()
    {
        pool_ctrl_ = NULL;
    }

    /**
     * @brief Init 初始化
     * @note 申请的基本单元根据pos串连在一起
     * @note 如果pos为负数表示指向下一个基本单元
     * @note 为正数表示已经被使用
     * @param unit_count 基本单元个数
     * @param shm_key 共享内存KEY
     *
     * @return 0成功；其他失败
     */
    int32_t Init(int32_t unit_count, key_t shm_key)
    {
        int32_t first_time = 0;
		int32_t total_size = sizeof(PoolCtrl) + sizeof(PoolUnit) * (unit_count + 1);
		pool_ctrl_ = (PoolCtrl*)shm_.Init(shm_key, 0x1121, total_size, first_time);

        if (pool_ctrl_ == NULL)
            return -1;

        if (first_time == 0) {
            if (pool_ctrl_->total_unit_count != unit_count ||
                pool_ctrl_->unit_size != sizeof(PoolUnit)) {
                return -2;
            }

            pool_ctrl_->pool = (PoolUnit*)(pool_ctrl_ + 1);
        } else {
            pool_ctrl_->free_mem_pos = 1;
            pool_ctrl_->pool = (PoolUnit*)(pool_ctrl_ + 1);
            pool_ctrl_->valid_unit_count = unit_count;
            pool_ctrl_->total_unit_count = unit_count;
            pool_ctrl_->unit_size = sizeof(PoolUnit);
    		for (int32_t i = 0; i < unit_count + 1; i++)
    			(pool_ctrl_->pool)[i].pos = -((int32_t)(i + 1));
        }

        return 0;
    }

	int32_t PopFreePos()
	{
		int32_t free_pos = pool_ctrl_->free_mem_pos;
		pool_ctrl_->free_mem_pos = -pool_ctrl_->pool[free_pos].pos;
		pool_ctrl_->pool[free_pos].pos = free_pos;
		return free_pos;
	}

	void PushFreePos(int32_t free_pos)
	{
		pool_ctrl_->pool[free_pos].pos = -pool_ctrl_->free_mem_pos;
		pool_ctrl_->free_mem_pos = free_pos;
	}

	void* Alloc()
	{
		if (pool_ctrl_->valid_unit_count > 0) {
			int32_t free_pos = PopFreePos();
			PoolUnit* mem_unit = &pool_ctrl_->pool[free_pos];
			pool_ctrl_->valid_unit_count--;
			return &mem_unit->mem;
		}
		return NULL;
	}

    void* Find(int32_t index)
    {
        if (index <= 0 || index > pool_ctrl_->total_unit_count)
            return NULL;
        PoolUnit* mem_unit = &pool_ctrl_->pool[index];
        if (mem_unit->pos != index)
            return NULL;
        else
            return &mem_unit->mem;
    }

	void Release(void* mem)
	{
		CHECK(pool_ctrl_->total_unit_count > pool_ctrl_->valid_unit_count)
            << "total_unit_count must > valid_unit_count";
        PoolUnit* mem_unit = (PoolUnit*)mem;
        CHECK(mem_unit->pos > 0)
            << "mem_unit->pos must > 0";
        PushFreePos(mem_unit->pos);
        pool_ctrl_->valid_unit_count++;
	}

	int32_t GetPos(void* mem)
	{
        PoolUnit* mem_unit = (PoolUnit*)mem;
        CHECK(mem_unit->pos > 0)
            << "mem_unit->pos must > 0";
        return mem_unit->pos;
	}

	void TravelPool(FuncRestore func, void* arg = NULL, int32_t output = 0)
	{
        if (output) {
		    LOG(INFO) << "-------------------------------------------------";
		    LOG(INFO) << "free_mem_pos = " << pool_ctrl_->free_mem_pos;
            LOG(INFO) << "valid_unit_count = " << pool_ctrl_->valid_unit_count;
            LOG(INFO) << "total_unit_count = " << pool_ctrl_->total_unit_count;
		    LOG(INFO) << "-------------------------------------------------";
        }

		for (int32_t i = 1; i <= pool_ctrl_->total_unit_count; i++) {
            if(output)
			    LOG(INFO) << "index[" << i << "] pos[" << pool_ctrl_->pool[i].pos << "]";
            if (func != NULL) {
                if(pool_ctrl_->pool[i].pos > 0 )
                    (*func)(&pool_ctrl_->pool[i].mem, arg);
            }
		}
        if (output)
		    LOG(INFO) << "-------------------------------------------------";
	}

private:
    PoolCtrl*       pool_ctrl_;
    ShmMemory       shm_;
};

#endif // _SHM_POOL_H_

