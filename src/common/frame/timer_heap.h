/**
 * @file timer_heap.h
 * @brief 定时器堆
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#ifndef _TIMER_HEAP_H_
#define _TIMER_HEAP_H_

#include "comm_def.h"
#include "time_value.h"
#include "callback.h"

#define DEFAULT_HEAP_SIZE 1000

/**
* @brief 最小堆实现的定时器
* @note 返回的定时器ID > 0
* @note timer_id_数组作用:
* @note 1、保存空闲id，空闲队列下标是time_id，下标数据是下一个空闲元素的下标
* @note 2、分配的单元，用来存放定时器对象在其数组中的下标，这样通过定时器id就很快定位到那个定时器对象
* @note 在堆的旋转过程中，分配单元的数据是时时变化的
*/
class TimerHeap
{
    typedef struct tagTimeNode {
        int32_t             timer_id;
        TimeValue           expire_time;
        TimeValue           interval_time;
        void*               data;
        CallbackObject*     cb_obj;
    } TimerNode;

public:
    DISALLOW_COPY_AND_ASSIGN(TimerHeap);
    /**
     * @brief TimerHeap 构造函数
     *
     * @param heap_size 堆大小
     */
    TimerHeap(uint32_t heap_size = DEFAULT_HEAP_SIZE);

    /**
     * @brief ~TimerHeap 析构函数
     */
    ~TimerHeap();

    /**
     * @brief RegisterTimer 注册定时器
     *
     * @param interval 定时器执行时间间隔
     * @param delay 定时器当前时间延迟多长时间执行
     * @param cb_obj 回调对象
     * @param data 回调数据
     *
     * @return timer_id or <=0 失败
     */
    int32_t RegisterTimer(const TimeValue& interval,
        const TimeValue& delay,
        CallbackObject* cb_obj,
        void* data = NULL);

    /**
     * @brief UnregisterTimer 取消定时器
     *
     * @param timer_id 定时器ID
     */
    void UnregisterTimer(int32_t timer_id);

    /**
     * @brief TimerPoll 定时器轮询
     *
     * @param now 当前时间
     */
    void TimerPoll(const TimeValue& now);

    /**
     * @brief FirstTimeout 第一个到期时间
     *
     * @return 到期时间
     */
    TimeValue* FirstTimeout() const;

private:

    /**
     * @brief GrowHeap 堆扩容
     * @note 成倍扩容
     */
    void GrowHeap();

    /**
     * @brief PopFreeTimerId 弹出空闲timer_id
     * @note 并不设置该timer_id为索引元素的值，该值默认是负数
     * @return
     */
    inline int32_t PopFreeTimerId()
    {
		int32_t timer_id = free_timer_id_;
		free_timer_id_ = -timer_id_[free_timer_id_];
		return timer_id;
    }

    /**
     * @brief PushFreeTimerId 将该timer_id压入空闲timer_id队列
     *
     * @param old_timer_id
     */
    inline void PushFreeTimerId(int32_t old_timer_id)
    {
        timer_id_[old_timer_id] = -free_timer_id_;
        free_timer_id_ = old_timer_id;
    }

    /**
     * @brief ExpireTime 计算超时时间
     *
     * @param now 当前时间
     * @param interval 时间间隔
     *
     * @return
     */
    inline TimeValue ExpireTime(const TimeValue& now, const TimeValue& interval)
    {
        return TimeValue(now.Sec() + interval.Sec(), now.Usec() + interval.Usec());
    }

    /**
     * @brief GetParentPos 获取父节点的位置
     *
     * @param cur_pos 当前节点的位置
     *
     * @return 父节点的位置
     */
    inline uint32_t GetParentPos(uint32_t cur_pos)
    {
        return (cur_pos ==0 ? 0 : ((cur_pos - 1) / 2));
    }

    /**
     * @brief GetLeftChildPos 获取左儿子节点的位置
     *
     * @param cur_pos 但却节点的位置
     *
     * @return 左儿子节点位置
     */
	inline uint32_t GetLeftChildPos(uint32_t cur_pos)
	{
		return 2 * cur_pos + 1;
	}

    /**
     * @brief TimervalGte 比较定时器超时时间
     *
     * @param left 时间a
     * @param right 时间b
     *
     * @return  true:时间a>=时间b;否则false
     */
	inline bool TimervalGte(const TimeValue& left, const TimeValue& right)
	{
		return (left > right || left == right);
	}

    /**
     * @brief RotateUp 堆元素上旋
     *
     * @param move_node 当前要移动的节点
     * @param pos 当前要移动节点的位置
     * @param parent_pos 父节点位置
     */
    void RotateUp(TimerNode* move_node, uint32_t pos, uint32_t parent_pos);

    /**
     * @brief RotateDown 堆元素下旋
     *
     * @param move_node 当前要移动的节点
     * @param pos  当前要移动节点的位置
     * @param child_pos 左儿子节点位置
     */
    void RotateDown(TimerNode* move_node, uint32_t pos, uint32_t child_pos);

    /**
     * @brief RemoveNode 移除节点
     * @note 在timer_id_数组中会回收该timer_id
     *
     * @param del_node_pos 移除节点位置
     *
     * @return 要移除的节点
     */
    TimerNode* RemoveNode(int32_t del_node_pos);

private:
	uint32_t            heap_size_;
	uint32_t            cur_size_;
	TimerNode**         heap_;
	int32_t*            timer_id_;
	int32_t             free_timer_id_;
};


#endif // _TIMER_HEAP_H_

