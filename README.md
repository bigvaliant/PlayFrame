PlayFrame
=========

A game server develop framework for using lua coroutine as asynchronous task.

## PlayFrame游戏架构
![](https://github.com/zfengzhen/Blog/blob/master/img/PlayFrame游戏架构.png)

## C++类成员函数回调实现
C++类成员函数回调跟C函数回调不同的是必须绑定类的实例, 知道是具体某个实例的回调.   
   
1 实现一个虚基类, 使其拥有一个固定的回调执行函数.     
```c++
class CallbackBase
{
public:
    virtual ~CallbackBase() {};
    virtual int32_t Execute(int64_t code, void* data) = 0;
};
```  

2 为了使得类实例中的Execute函数类型的成员函数(返回值以及参数都跟Excute一样)能够被回调, 封装模板类继承上述虚基类, 其中模板形参为回调函数的类名, 该模板类的构造函数需要传入具体的类实例指针以及类成员函数指针.  
```c++
template<class T>
class Callback : public CallbackBase
{
public:
    typedef int32_t (T::*Func)(int64_t code, void* data);

    Callback(T* obj, Func func) : obj_(obj), func_(func) {}
    virtual ~Callback() {};

    int32_t Execute(int64_t code, void* data)
    {
        return (obj_->*func_)(code, data);
    }

private:
    T* obj_;
    Func func_;
};
```  

3 如果要申请一个回调类实例只能通过new Callback<T>(T* obj, Func func)实现, 以及某个回调类需要继承Callback, 也只能继承Callback<T>的模板类, 这样接口中使用模板并不是那么清晰. 我们需要统一回调对象的接口, 使得所有的回调对象有统一的接口, 这样在需要用到回调的场景中, 可以以统一接口去回调不同类的成员函数. 实现CallbackObject先对模板类进行封装, 使得类更加的清晰. 需要回调的类通过继承CallbackObject接口, 调用SetHandler接口, 可以清晰的将本类以CallbackObject的指针形式传递给其他函数, 如定时器的回调.   
```c++
class CallbackObject
{
public:
    CallbackObject() : callback_(NULL) {}
    virtual ~CallbackObject()
    {
        if(callback_)
            callback_->~CallbackBase();
    }

    template<class T>
    void SetHandler(T* obj, typename Callback<T>::Func func)
    {
        callback_ = new((void*)storage_)Callback<T>(obj, func);
    }

    int32_t Execute(int64_t code, void* data)
    {
        return callback_->Execute(code, data);
    }

private:
    char            storage_[sizeof(Callback<CallbackBase>)];
    CallbackBase*   callback_;
};
```   

## 线性同余数随机数生成器
参见[数据结构与算法分析10.4](http://book.douban.com/subject/1139426/)  

>> 产生随机数最简单的方法是线性同余数发生器, 它于1951年由Lehmer首先描述: 数x1, x2, .......的生成满足  
>>      X(i+i) = AX(i) mod M  
>> 为了开始这个序列, 必须给出x0的某个值, 这个值叫种子(seed). 如果x0=0, 那么这个序列远不是随机的, 但是如果A和M旋转得正确, 那么任何其他的 1 =< x0 < M都是同等有效的. 如果M是素数, 那么x(i)就绝不会是0.   
>> 如果M是素数, 那么总存在对A的一些旋转能够给出周期M-1, 对A的有些选择则得不到这样的周期(周期<=M-1).   
>> 如果M选择得很大, 比如31比特的素数, 那么对于大部分的应用来说周期应该是非常长的. Lehmer建议使用31个比特的素数M=2^31-1=2147483647. 对于这个素数, A=48271是给出整周期发生器的许多值中的一个. 它的用途已经被深入研究并被这个领域的专家推荐. 对于随机数发生器, 贸然修改通常意味着失败, 因此我们奉劝还是继续坚持使用这个公式到直到有新的成果发布.     

该算法不适合加密，理论上知道随机种子的值，从数学上可以猜解出整个序列.  
注意:  
1、计算机不收集外部信息，只能产生伪随机序列，实际上随机数都是随机序列上的一个个元素  
2、伪随机序列的一种生成方式是线性同余算法  
3、线性同余算法初始会有一个种子，种子一旦确定就不应该修改，虽然随机序列已经定下来，如果修改种子会破坏线性同余随机数分布的均匀性；确定一个种子以后，可以确定随机数的周期，有一个大致的评估，如果一直改动种子，虽然看上去，也是随机数，但是无法确认随机数的均匀分布，这样就会引入新的问题  

## 定时器
游戏中定时器非常常见, 比如给某个玩家增加一个增益, 每隔3秒回多少血.  
分析定时器需求:  
1 定时器需要分配一个唯一的timer_id, 这样我们可以用该id来标识其他对象需要timer的对象, 比如异步事件系统, 每个事件需要一个定时器去判断异步事件是否超时, timer_id用来表示事件id  
2 定时器的查找速度越快越好, 根据timer_id查找定时器, 比如增益被其他技能延长时间或者取消, 这个时候我们需要快速的查找到该定时器, 时间越快越好  
3 定时器的插入以及删除越快越好  
4 不用过多空间复杂度换取时间复杂度  

最终采用最小堆实现, 不过增加了几个辅助的数组.  
定义:  
TimerNodeArray: 用于最小堆的调整的数组, 存储TimerNode, TimerNodeArray[0]为最小值  
TimerNodeIndexArray: 用于存储TimerNode的数组下标, 因为TimerNodeArray中的元素是经常上旋下旋变换位置的, 如果为非负值, 表示存储着TimerNodeArray的数组下标位置, 如果为负值, 表示空闲节点, 通过-2指向2节点的这种方式链起来  
RandArray: 用于在定时器注册的时候, 分配一个随机值, TimerNodeIndexArray数组下标跟该随机值组成timer_id, 这样保证timer_id在大部分情况下的唯一性.  

![link](https://github.com/zfengzhen/Blog/blob/master/img/PlayFrame_heaptimer.png)  

时间复杂度:   
查找定时器: O(1)    
查找到定时器调整或者删除: O(logN)   
插入定时器: O(logN)  
  
