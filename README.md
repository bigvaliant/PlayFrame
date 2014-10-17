PlayFrame
=========

A game server develop framework for using lua coroutine as asynchronous task.

## PlayFrame游戏架构
![](https://github.com/zfengzhen/Blog/blob/master/img/PlayFrame游戏架构.png)

## 第一节 C++类成员函数回调实现
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

## 第二节 线性同余数随机数生成器
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

## 第三节 定时器
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
  
## 第四节 异步任务之lua协程模型    
### 异步任务   
在单进程异步IO模型中常见, 一个任务需要多步完成, 其中多步大部分情况是指跟多个其他后台服务交互获取数据, 如果同步的话会影响使得系统性能大幅下降, 引入异步任务方式.  

### 异步任务处理   
1. 请求--->保存上下文--->异步逻辑(其他后台服务)    
2. 异步逻辑回包--->恢复上下文--->本地逻辑--->保存上下文--->异步逻辑(其他后台服务)    
3. 异步逻辑回包--->恢复上下文--->本地逻辑--->响应请求   

### 上下文保存的两种方式  
1. 上下文保存在网络请求协议中, 回包时原样回传(依赖于后端服务提供echo_buf功能)    
2. 创建任务唯一表示id, 本地保存id对应的上下文, 回包时后端服务原样回传id, 通过id获取上下文   

### 异步任务常用实现方式---Task异步模型     
![](https://github.com/zfengzhen/Blog/blob/master/img/lua_async_task_task_model.png)    
Task为一个基类, 里面主要包含了一个Step类实例的链表.   
需要提供如下接口:  
1. OnTaskFailed 超时处理接口  
2. OnTaskSuccessed 任务成功完成时处理接口  

Step也为一个基类, 表示具体的异步步骤.  
需要提供如下接口:  
1. OnStepPerform 异步任务执行(发包), 执行完后回到游戏主逻辑循环  
2. OnStepNotify 异步任务回调通知, 表示该步骤回调完成, 如果成功跳转到Task的下一个Step, 否则执行OnTaskFailed  

异步任务需要继承Task, 并实现相应的具体Step, 每个Task和Step都得继承上述的接口, 阅读代码时, 具体的Step分散在各个地方, 可读性相对较差.  

### lua协程模型
一个异步任务即为一个lua协程, 协程保存异步任务的上下文信息, 一个lua协程中只包含一个具体的lua函数, 通过调用该lua函数采用lua的协程机制去粘合具体的异步任务, 需要将C++中的逻辑API解耦抽象出来, 传入lua中, 通过lua的协程去粘合具体需要调用的逻辑API, 使其具有顺序的代码可读性.   

超时处理:  
创建任务时, 分配一个定时器, 该定时器ID唯一, 也用该ID表示任务唯一ID, 也即是异步网络协议中必须传输的唯一标识上下文的ID. 超时后, 关闭该定时器, 以及关闭该协程(超时响应交给客户端自己去识别, 服务器不下发后端超时协议).   

#### PlayFrame中lua协程模型的实现  
1. GameSvr包含5个模块, ConfigModule(读取配置文件模块), MsgModule(消息处理模块), LuaEngineModule(lua异步任务处理模块), TimerMgrModule(定时器管理模块), ObjMgrModule(内存池对象管理模块)  
2. 游戏主循环: MsgModule一直非阻塞循环接收消息以及LuaEngineModule非阻塞循环触发定时器看是否有任务超时.   
3. MsgModule接收请求后, 通过调用LuaEngineModule创建具体异步任务, 分配任务ID, 绑定ID跟该任务协程的映射, 将具体的lua函数压入协程, 通过lua_resume传入参数, 任务开始执行  
4. MsgModule接收到后端的响应回包后, 根据回包中的唯一ID获取到具体协程, 解析协议中相应字段, 通过lua_resume压入协程中恢复协程执行, 异步任务继续执行.  

#### 程序挂掉如何保证重启后lua协程信息不会丢失?  
1 协程信息保存到共享内存中, 进程拉起后重读共享内存, 恢复协程状态   
2 类似于线程池的处理办法, 进程关闭时, 不接受新的请求, 等待所有之前的协程结束后才退出.  
PlayFrame采用第二种方法    

![](https://github.com/zfengzhen/Blog/blob/master/img/PlayeFrame_lua_coroutine_async.png)  

## 第五节 高效的epoll事件回调封装  
主要参考redis ae.h ae.c ae_epoll.c文件  

```c++
#define EVENT_NONE 0
#define EVENT_READ 1
#define EVENT_WRITE 2

typedef void FileProc(int fd, void* client_data); 

typedef struct tagFileEvent {
    int mask_;
    FileProc* read_proc_;
    FileProc* write_proc_;
    void* client_data_;
} FileEvent;

typedef struct tagEpollEventLoop {
    int epfd_;
    struct epoll_event *epoll_events_;
    int max_events_size_;
    FileEvent* file_events_;
} EpollEventLoop;
```    

通过Init申请要监听fd个数的大小max_events_size, 对于FileEvent和struct epoll_event数组都是以fd为数组索引进行取值, 所以最终能够监听的fd数小于max_events_size.   

**添加事件**   
传入的mask为EVENT_READ, 查看fd所在的FileEvent的mask_是否为EVENT_NONE, 如果为EVNET_NONE, 则epoll_ctl为EPOLL_CTL_ADD, 否则为EPOLL_CTL_MOD.  
EVENT_READ传入epoll_ctl的struct epoll_evnet的事件为EPOLLIN, 并将回调函数传入到FileEvent的read_proc.    
EVENT_WRITE传入epoll_ctl的struct epoll_evnet的事件为EPOLLOUT, 并将回调函数传入到FileEvent的write
_proc.  

**删除事件**  
fe->mask_ = fe->mask_ & (~mask)  
去除传入的要删除的事件(读事件或者写事件 或者全部)  
如果fe->mask_ 为EVENT_NONE, 则调用EPOLL_CTL_DEL, 否则调用EPOLL_CTL_MOD.  

**主循环**  
```c++
int ret = epoll_wait(ev_loop_->epfd_, ev_loop_->epoll_events_, ev_loop_->max_events_size_, timeout);
```  
遍历struct epoll_event数组, 如果是EPOLLIN事件调用read_proc, EPOLLOUT调用write_proc, 其他错误处理也调用write_proc.    

## 第六节 GLOG功能修改  
### 1、按天增加日志   
utilities.cc  
```c
static int32 g_main_day = 0;
bool DayHasChanged()
{
    time_t raw_time;
    struct tm* tm_info;
    time(&raw_time);
    tm_info = localtime(&raw_time);
    if (g_main_day == 0)
        return false;
    if (tm_info->tm_mday != g_main_day) {
        g_main_day = tm_info->tm_mday;
        return true;
    }
    return false;
} 
```

logging.cc LogFileObject::Write  
```c
  if (DayHasChanged()) {
    if (file_ != NULL) fclose(file_);
    file_ = NULL;
    file_length_ = bytes_since_flush_ = 0;
    rollover_attempt_ = kRolloverAttemptFrequency-1;
  }
```

### 2、只以年月日为结尾文件名，超过一定大小在文件名后增加.1 .2标识符    
比如： gamesvr_info_20140416.log gamesvr_info_20140416.log.1  
logging.cc LogFileObject::Write  

```c
  static int32_t changed_time = 0;
  if (static_cast<int>(file_length_ >> 20) >= MaxLogSize()) {
    changed_time++;
    if (file_ != NULL) fclose(file_);
    file_ = NULL;
    struct ::tm tm_time;
    localtime_r(&timestamp, &tm_time);
                                     
    ostringstream time_pid_stream;
    time_pid_stream.fill('0');
    time_pid_stream << 1900+tm_time.tm_year
          << setw(2) << 1+tm_time.tm_mon
          << setw(2) << tm_time.tm_mday
          << ".log";
                   
    ostringstream new_time_pid_stream;
    new_time_pid_stream.fill('0');
    new_time_pid_stream << 1900+tm_time.tm_year
          << setw(2) << 1+tm_time.tm_mon
          << setw(2) << tm_time.tm_mday
          << ".log"
          << "." << changed_time;
                                
    const string& time_pid_string = time_pid_stream.str();
    const string& new_time_pid_string = new_time_pid_stream.str();
                                                               
    string string_filename = base_filename_+filename_extension_+
          time_pid_string;
    string new_string_filename = base_filename_+filename_extension_+
          new_time_pid_string;
                             
    const char* filename = string_filename.c_str();
    const char* new_filename = new_string_filename.c_str();
                             
    rename(filename, new_filename);
                             
    file_length_ = bytes_since_flush_ = 0;
    rollover_attempt_ = kRolloverAttemptFrequency-1;
  }
```

### 3、错误日志分级，只写入指定级别文件  
glog/logging.h  
```c
DECLARE_bool(servitysinglelog);  
```

logging.cc  
```c
GLOG_DEFINE_bool(servitysinglelog, true,
                 "Prepend the log prefix to the start of each log line");
```

logging.cc LogDestination::LogToAllLogfiles  
```c
  if ( FLAGS_logtostderr ) {           // global flag: never log to file
    ColoredWriteToStderr(severity, message, len);
  } else {
    // edit by zfengzhen : add FLAGS_servitysinglelog
    if (FLAGS_servitysinglelog) {
        LogDestination::MaybeLogToLogfile(severity, timestamp, message, len);
    } else {
        for (int i = severity; i >= 0; --i)
            LogDestination::MaybeLogToLogfile(i, timestamp, message, len);
    }
  }
}
```
