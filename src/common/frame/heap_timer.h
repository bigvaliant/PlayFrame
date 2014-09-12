#ifndef _HEAP_TIMER_H_
#define _HEAP_TIMER_H_

#include "comm_def.h"
#include "random.h"
#include "time_value.h"
#include "callback.h"

#define DEFAULT_HEAP_SIZE 1000

class HeapTimer
{
    typedef struct tagTimeNode {
        int64_t             timer_id;
        TimeValue           expire_time;
        TimeValue           interval_time;
        void*               data;
        CallbackObject*     cb_obj;
    } TimerNode;

public:
    DISALLOW_COPY_AND_ASSIGN(HeapTimer);
    HeapTimer(uint32_t heap_size = DEFAULT_HEAP_SIZE);
    ~HeapTimer();

    int64_t RegisterTimer(const TimeValue& interval,
        const TimeValue& delay,
        CallbackObject* cb_obj,
        void* data = NULL);

    void UnregisterTimer(int64_t timer_id);

    void TimerPoll(const TimeValue& now);

    TimeValue* FirstTimeout() const;

    int32_t GetTimerSize() const
    {
        return (int32_t)cur_size_; 
    }

private:

    void GrowHeap();

    inline int64_t PopFreeTimerId()
    {
		int32_t timer_node_index = free_timer_node_index_;
		free_timer_node_index_ = -timer_node_index_array_[free_timer_node_index_];

        int32_t rand_num = rand_maker_.GetRand();
        rand_array_[timer_node_index] = rand_num;

        int64_t timer_id = ((int64_t)timer_node_index << 32) | ((int64_t)rand_num);

		return timer_id;
    }

    inline void PushFreeTimerId(int64_t old_timer_id)
    {
        int32_t timer_node_index = (int32_t)(old_timer_id >> 32);
        int32_t rand_num = (int32_t)(old_timer_id & 0xffffffff);
        
        int32_t timer_rand = rand_array_[timer_node_index];
        if (timer_rand != rand_num) {
            LOG(ERROR) 
                << "PushFreeTimerId timer_rand[" << timer_rand
                << "] != rand_num[" << rand_num
                << "] error!!!!";
            return;
        }

        timer_node_index_array_[timer_node_index] = -free_timer_node_index_;
        free_timer_node_index_ = timer_node_index;

        rand_array_[timer_node_index] = 0;
    }

    inline TimeValue ExpireTime(const TimeValue& now, const TimeValue& interval)
    {
        return TimeValue(now.Sec() + interval.Sec(), now.Usec() + interval.Usec());
    }

    inline uint32_t GetParentPos(uint32_t cur_pos)
    {
        return (cur_pos == 0 ? 0 : ((cur_pos - 1) / 2));
    }

	inline uint32_t GetLeftChildPos(uint32_t cur_pos)
	{
		return 2 * cur_pos + 1;
	}

	inline bool TimervalGte(const TimeValue& left, const TimeValue& right)
	{
		return (left > right || left == right);
	}
    void RotateUp(TimerNode* move_node, uint32_t pos, uint32_t parent_pos);
    void RotateDown(TimerNode* move_node, uint32_t pos, uint32_t child_pos);
    TimerNode* RemoveNode(int32_t del_node_pos);

private:
	uint32_t            heap_size_;
	uint32_t            cur_size_;
	TimerNode**         heap_;
	int32_t*            timer_node_index_array_;
	int32_t*            rand_array_;
	int32_t             free_timer_node_index_;
    Random              rand_maker_;
};


#endif

