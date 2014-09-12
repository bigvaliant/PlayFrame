/**
 * @file heap_timer.cpp
 * @brief 定时器堆
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#include "heap_timer.h"

// 用这种方式进行堆位置运算，更快，不用除和乘
#define HEAP_ENTRY_TO_INDEX(level, nth)  ((1 << (level)) + (nth) - 1)
#define HEAP_PARENT(index)               ((((index) + 1) >> 1) - 1)
#define HEAP_LEFT_CHILD(index)           ((((index) + 1) << 1) - 1 + 0)
#define HEAP_RIGHT_CHILD(index)          ((((index) + 1) << 1) - 1 + 1)

HeapTimer::HeapTimer(uint32_t heap_size)
    : heap_size_(heap_size + 1),
      cur_size_(0),
      heap_(NULL),
      timer_node_index_array_(NULL),
      free_timer_node_index_(1),
      rand_maker_(0)
{
    // TimerNode堆数组
    heap_ = (TimerNode**)new TimerNode*[heap_size_];

    // TimerNode索引数组
    timer_node_index_array_ = (int32_t*)new int32_t[heap_size_];
    for (uint32_t i = 0; i < heap_size_; i++) {
        timer_node_index_array_[i] = -((int32_t)(i + 1));
    }

    // Timer随机数数组
    rand_array_ = (int32_t*)new int32_t[heap_size_];
    for (uint32_t i = 0; i < heap_size_; i++) {
        rand_array_[i] = 0;
    }
}

HeapTimer::~HeapTimer()
{
    for (uint32_t i = 0; i < heap_size_; i++)
        if (timer_node_index_array_[i] >= 0)
            delete heap_[timer_node_index_array_[i]];

    delete[] timer_node_index_array_;
    delete[] rand_array_;
    delete[] heap_;
}

int64_t HeapTimer::RegisterTimer(const TimeValue& interval,
    const TimeValue& delay,
	CallbackObject* cb_obj,
    void* data)
{
	if (interval.Sec() == 0 && interval.Usec() == 0)
		return -1;

	if (cur_size_ >= heap_size_)
		return -1;

	TimerNode* new_node = new TimerNode;
    if (new_node == NULL)
        return -1;

	new_node->cb_obj            = cb_obj;
	new_node->data              = data;
	new_node->timer_id          = PopFreeTimerId();
    new_node->interval_time     = interval;
	new_node->expire_time       = ExpireTime(TimeValue(time(NULL)), delay);

	if (cur_size_ + 2 >= heap_size_)
		GrowHeap();

	RotateUp(new_node, cur_size_, GetParentPos(cur_size_));
	cur_size_++;

    LOG(INFO) << "RegisterTimer TimerSize[" << GetTimerSize() << "]";

	return new_node->timer_id;
}

void HeapTimer::UnregisterTimer(int64_t timer_id)
{
    int32_t timer_node_index = (int32_t)(timer_id >> 32);
    int32_t rand_num = (int32_t)(timer_id & 0xffffffff);

	if (timer_node_index < 0 || timer_node_index > (int32_t)heap_size_)
		return;

	int32_t node_pos = timer_node_index_array_[timer_node_index];
	if (node_pos < 0)
		return;

    int32_t timer_rand = rand_array_[timer_node_index];
    if (timer_rand != rand_num)
        return;

	TimerNode* del_node = heap_[node_pos];
	if (del_node == NULL)
		return;

	RemoveNode(node_pos);
	delete del_node;

    LOG(INFO) << "UnregisterTimer TimerSize[" << GetTimerSize() << "]";
	return;
}

void HeapTimer::TimerPoll(const TimeValue& now)
{
	if (cur_size_ == 0)
		return;

	while (cur_size_ > 0 && TimervalGte(now, heap_[0]->expire_time)) {
		int32_t ret = 0;
		TimerNode* expire_node = heap_[0];
        int32_t timer_id = expire_node->timer_id;

		if (expire_node && expire_node->cb_obj)
			ret = expire_node->cb_obj->Execute(timer_id, expire_node->data);

        // 如果调用了UnregisterTimer则节点被删除
        if (expire_node == NULL)
            return;

        if (ret == 0 && expire_node != NULL &&
            (0 != expire_node->interval_time.Sec() ||
             0 != expire_node->interval_time.Usec())) {
            expire_node->expire_time = ExpireTime(now, expire_node->interval_time);
            RotateDown(expire_node, 0, GetLeftChildPos(0));
        } else {
            RemoveNode(0);
            delete expire_node;
            LOG(INFO) << "Callback return < 0 TimerSize[" << GetTimerSize() << "]";
        }
	}

	return;
}

TimeValue* HeapTimer::FirstTimeout() const
{
    if(cur_size_ > 0)
        return &(heap_[0]->expire_time);
    return NULL;
}

void HeapTimer::GrowHeap()
{
	// 成倍扩展
	uint32_t new_size = 2 * heap_size_;
	TimerNode ** new_heap = NULL;

	new_heap = (TimerNode **)new TimerNode*[new_size];
	memcpy(new_heap, heap_, heap_size_ * sizeof(TimerNode*));
    delete heap_;
	heap_ = new_heap;

	int32_t* new_timer_node_index_array = (int32_t *)new int32_t[new_size];
	memcpy(new_timer_node_index_array, timer_node_index_array_, heap_size_ * sizeof(int32_t));
    delete timer_node_index_array_;
	timer_node_index_array_ = new_timer_node_index_array;
	for(uint32_t i = heap_size_; i < new_size; i++)
		timer_node_index_array_[i] = -((int32_t)(i + 1));

	int32_t* new_rand_array = (int32_t *)new int32_t[new_size];
	memcpy(new_rand_array, rand_array_, heap_size_ * sizeof(int32_t));
    delete rand_array_;
	rand_array_ = new_rand_array;
	for(uint32_t i = heap_size_; i < new_size; i++)
		rand_array_[i] = 0;

	// 重新设置长度
	heap_size_ = new_size;
}

void HeapTimer::RotateUp(TimerNode* move_node, uint32_t pos, uint32_t parent_pos)
{
    // 上旋和下旋的过程中节点的timer_id不变
	while (pos > 0) {
        // 当前节点跟父节点对比，如果小于，如果小于
        // 则当前节点是目前三元组中最小的一个
        // 将父节点下移
		if (!TimervalGte(move_node->expire_time,
            heap_[parent_pos]->expire_time)) {
			heap_[pos] = heap_[parent_pos];
            int32_t parent_timer_node_index = (int32_t)(heap_[parent_pos]->timer_id >> 32);
			timer_node_index_array_[parent_timer_node_index] = pos;
			pos = parent_pos;
			parent_pos = GetParentPos(parent_pos);
		} else
			break;
	}
    // 把对象放到最终的位置
	heap_[pos] = move_node;
    int32_t timer_node_index = (int32_t)(move_node->timer_id >> 32);
	timer_node_index_array_[timer_node_index] = pos;
}

void HeapTimer::RotateDown(TimerNode* move_node, uint32_t pos, uint32_t child_pos)
{
    // 上旋和下旋的过程中节点的timer_id不变
	while (child_pos < cur_size_) {
        // 选择最小的child
        if (child_pos + 1 < cur_size_ &&
			TimervalGte(heap_[child_pos]->expire_time,
            heap_[child_pos + 1]->expire_time))
			child_pos++;

        // 当前节点跟最小的儿子比较，如果大于，子节点上移
		if (TimervalGte(move_node->expire_time, heap_[child_pos]->expire_time)) {
			heap_[pos] = heap_[child_pos];
            int32_t child_timer_node_index = (int32_t)(heap_[child_pos]->timer_id >> 32);
			timer_node_index_array_[child_timer_node_index] = pos;
			pos = child_pos;
			child_pos = GetLeftChildPos(pos);
		}
		else
			break;
	}
    // 把对象放到最终的位置
	heap_[pos] = move_node;
    int32_t timer_node_index = (int32_t)(move_node->timer_id >> 32);
	timer_node_index_array_[timer_node_index] = pos;
}

HeapTimer::TimerNode* HeapTimer::RemoveNode(int32_t del_node_pos)
{
    if (cur_size_ == 0)
        return NULL;

	TimerNode* del_node = heap_[del_node_pos];

	PushFreeTimerId(del_node->timer_id);
	del_node->timer_id = -1;

	cur_size_--;

	if ((uint32_t)del_node_pos < cur_size_) {
		TimerNode* move_node = heap_[cur_size_];
		heap_[del_node_pos] = move_node;
        int32_t timer_node_index = (int32_t)(move_node->timer_id >> 32);
        timer_node_index_array_[timer_node_index] = del_node_pos;

		uint32_t parent_pos = GetParentPos(del_node_pos);

		if (TimervalGte(move_node->expire_time, heap_[parent_pos]->expire_time)) {
			RotateDown(move_node, del_node_pos, GetLeftChildPos(del_node_pos));
		} else {
			RotateUp(move_node, del_node_pos, parent_pos);
		}
	}

	return del_node;
}

