PlayFrame
=========

A game server develop framework for using lua coroutine as asynchronous task.

## PlayFrame游戏架构
![](https://github.com/zfengzhen/Blog/blob/master/img/PlayFrame游戏架构.png)

## C++类成员函数回调实现
C++类成员函数回调跟C函数回调不同的是C++类成员函数必须绑定类的实例, 知道是具体某个实例的回调.    
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
