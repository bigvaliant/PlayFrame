// lua_tinker.h
//
// LuaTinker - Simple and light C++ wrapper for Lua.
//
// Copyright (c) 2005-2007 Kwon-il Lee (zupet@hitel.net)
//
// please check Licence.txt file for licence and legal issues.

#if !defined(_LUA_TINKER_H_)
#define _LUA_TINKER_H_

#include <new>
#include <string.h>

// *****************************************************************************
// edit by fergus (zfengzhen@gmail.com)
// 1. 增加一些注释
// 2. 适应linux 64位, 增加宏定义如果是64位系统,
//    long 和 unsigned long会定义为64位
// 3. fix var_base, 增加虚析构函数
// 4. fix table构造函数, 增加table_obj引用
// 5. 修改meta_get和meta_set函数, 原代码不会从父类中查找子类中找不到的成员
// 6. call方法增加到最大8个参数
// 7. constructor仍然最多是8个参数
// 8. 全局函数参数增加到最大8个参数
// 9. 成员函数参数增加到最大8个参数
// *****************************************************************************
// 使用注意点:
// 1. class_def的时候不能添加静态成员函数,
//    如果时静态成员函数的时候需奥通过def去定义
// 2. class_con 构造函数只能注册一个, 并以最后一个注册的为准
//    只压入一个构造函数, 其他参数提供set方法设置
// 3. class type& 会退化为class type, 用到引用的地方最好都用指针代替(参数等)
// 4. 基本类型的数据在lua中修改, 不会同步到C++, 因为都是直接压入数据
// 5. const特性会被lua破坏
// *****************************************************************************

namespace lua_tinker
{
    // init LuaTinker
    void    init(lua_State *L);

    void    init_s64(lua_State *L);
    void    init_u64(lua_State *L);

    // string-buffer excution
    void    dofile(lua_State *L, const char *filename);
    void    dostring(lua_State *L, const char* buff);
    void    dobuffer(lua_State *L, const char* buff, size_t sz);

    // debug helpers
    void    enum_stack(lua_State *L);
    int     on_error(lua_State *L);
    void    print_error(lua_State *L, const char* fmt, ...);

    // dynamic type extention
    struct lua_value
    {
        virtual void to_lua(lua_State *L) = 0;
    };

    // type trait
    template<typename T> struct class_name;
    struct table;

    // 编译期间的if，如果C是true就是A类型，false就是B类型
    template<bool C, typename A, typename B> struct if_ {};
    template<typename A, typename B>        struct if_<true, A, B> { typedef A type; };
    template<typename A, typename B>        struct if_<false, A, B> { typedef B type; };

    // 判断是否是指针
    template<typename A>
    struct is_ptr { static const bool value = false; };
    template<typename A>
    struct is_ptr<A*> { static const bool value = true; };

    // 判断是否是引用
    template<typename A>
    struct is_ref { static const bool value = false; };
    template<typename A>
    struct is_ref<A&> { static const bool value = true; };

    // 移除const
    template<typename A>
    struct remove_const { typedef A type; };
    template<typename A>
    struct remove_const<const A> { typedef A type; };

    // 获取基本类型 指针以及引用
    template<typename A>
    struct base_type { typedef A type; };
    template<typename A>
    struct base_type<A*> { typedef A type; };
    template<typename A>
    struct base_type<A&> { typedef A type; };

    // 获取类的类型 class A;
    // class_type<A*>::type a_inst;
    // class_type<A&>::type a_inst;
    // class_type<const A*>::type a_inst;
    // class_type<const A&>::type a_inst;
    // 都可以用来声明
    template<typename A>
    struct class_type { typedef typename remove_const<typename base_type<A>::type>::type type; };

    // 判断是否是对象
    template<typename A>
    struct is_obj { static const bool value = true; };
    template<> struct is_obj<char>                  { static const bool value = false; };
    template<> struct is_obj<unsigned char>         { static const bool value = false; };
    template<> struct is_obj<short>                 { static const bool value = false; };
    template<> struct is_obj<unsigned short>        { static const bool value = false; };
    template<> struct is_obj<long>                  { static const bool value = false; };
    template<> struct is_obj<unsigned long>         { static const bool value = false; };
    template<> struct is_obj<int>                   { static const bool value = false; };
    template<> struct is_obj<unsigned int>          { static const bool value = false; };
    template<> struct is_obj<float>                 { static const bool value = false; };
    template<> struct is_obj<double>                { static const bool value = false; };
    template<> struct is_obj<char*>                 { static const bool value = false; };
    template<> struct is_obj<const char*>           { static const bool value = false; };
    template<> struct is_obj<bool>                  { static const bool value = false; };
    template<> struct is_obj<lua_value*>            { static const bool value = false; };
    template<> struct is_obj<long long>             { static const bool value = false; };
    template<> struct is_obj<unsigned long long>    { static const bool value = false; };
    template<> struct is_obj<table>                 { static const bool value = false; };

    /////////////////////////////////
    // 判断是否是引用 start
    // 方法: 通过是否可以隐式转化为int以及不是char* short* int* long* double* flaot* bool*的指针

    // 数组引用 sizeof(no_type) == 1  sizeof(yes_type) == 2
    // 通过判断两个sizeof的大小进行判断
    enum { no = 1, yes = 2 };
    typedef char (& no_type )[no];
    typedef char (& yes_type)[yes];

    // int_conv_type 结构体，里面是它的构造函数，需要传入一个int类型
    struct int_conv_type { int_conv_type(int); };

    // 枚举可以隐式转换为int
    // 如果是int类型就会隐式转化为int_conv_type，返回yes_type；否则是no_type
    no_type int_conv_tester (...);
    yes_type int_conv_tester (int_conv_type);

    // 传入枚举类型返回yes_type
    // enum XXX xxx;
    // sizeof(vfnd_ptr_tester(add_ptr(xxx))) == sizeof(yes_type)
    no_type vfnd_ptr_tester (const volatile char *);
    no_type vfnd_ptr_tester (const volatile short *);
    no_type vfnd_ptr_tester (const volatile int *);
    no_type vfnd_ptr_tester (const volatile long *);
    no_type vfnd_ptr_tester (const volatile double *);
    no_type vfnd_ptr_tester (const volatile float *);
    no_type vfnd_ptr_tester (const volatile bool *);
    yes_type vfnd_ptr_tester (const volatile void *);

    // 获取指针类型
    template <typename T> T* add_ptr(T&);

    // bool类型转化为yes_type 或者 no_type
    template <bool C> struct bool_to_yesno { typedef no_type type; };
    template <> struct bool_to_yesno<true> { typedef yes_type type; };

    // 判断是否是枚举变量
    template <typename T>
    struct is_enum
    {
        static T arg;
        static const bool value = ( (sizeof(int_conv_tester(arg)) == sizeof(yes_type)) && (sizeof(vfnd_ptr_tester(add_ptr(arg))) == sizeof(yes_type)) );
    };
    // 判断是否是引用 end
    /////////////////////////////////

    // from lua
    // 输入参数转成T类型
    template<typename T>
    struct void2val { static T invoke(void* input){ return *(T*)input; } };
    // 输入参数转成T类型指针
    template<typename T>
    struct void2ptr { static T* invoke(void* input){ return (T*)input; } };
    // 输入参数转成T类型引用
    template<typename T>
    struct void2ref { static T& invoke(void* input){ return *(T*)input; } };

    // 将输入参数ptr转换成T值 T*指针 或者T&引用
    template<typename T>
    struct void2type
    {
        static T invoke(void* ptr)
        {
            return  if_<is_ptr<T>::value
                       , void2ptr<typename base_type<T>::type>
                       , typename if_<is_ref<T>::value
                                     , void2ref<typename base_type<T>::type>
                                     , void2val<typename base_type<T>::type>
                                     >::type
                       >::type::invoke(ptr);
        }
    };

    // 存储指针的类
    struct user
    {
        user(void* p) : m_p(p) {}
        virtual ~user() {}
        void* m_p;
    };

    // 将lua栈上索引的userdata转换为T值 T*指针 T&引用
    template<typename T>
    struct user2type { static T invoke(lua_State *L, int index) { return void2type<T>::invoke(lua_touserdata(L, index)); } };

    // 将lua栈上索引的number转换为T值 T*指针 T&引用
    // T为枚举类型
    template<typename T>
    struct lua2enum { static T invoke(lua_State *L, int index) { return (T)(int)lua_tonumber(L, index); } };

    // 将lua栈上索引的userdata转换为T值 T*指针 T&引用
    // 非userdata报错
    template<typename T>
    struct lua2object
    {
        static T invoke(lua_State *L, int index)
        {
            if(!lua_isuserdata(L,index))
            {
                lua_pushstring(L, "no class at first argument. (forgot ':' expression ?)");
                lua_error(L);
            }
            //                         |------- struct user*类型--------|
            return void2type<T>::invoke(user2type<user*>::invoke(L,index)->m_p);
        }
    };

    // lua2type 函数
    // 将lua栈上索引的枚举值或者userdata转换为相对应的类型
    template<typename T>
    T lua2type(lua_State *L, int index)
    {
        return  if_<is_enum<T>::value
                    ,lua2enum<T>
                    ,lua2object<T>
                >::type::invoke(L, index);
    }

    // struct val2user类型
    // val转换到user
    // 将T类型转换为struct user类型
    // T的构造函数最多5个参数
    template<typename T>
    struct val2user : user
    {
        val2user() : user(new T) {}

        template<typename T1>
        val2user(T1 t1) : user(new T(t1)) {}

        template<typename T1, typename T2>
        val2user(T1 t1, T2 t2) : user(new T(t1, t2)) {}

        template<typename T1, typename T2, typename T3>
        val2user(T1 t1, T2 t2, T3 t3) : user(new T(t1, t2, t3)) {}

        template<typename T1, typename T2, typename T3, typename T4>
        val2user(T1 t1, T2 t2, T3 t3, T4 t4) : user(new T(t1, t2, t3,t4)) {}

        template<typename T1, typename T2, typename T3, typename T4, typename T5>
        val2user(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5) : user(new T(t1, t2, t3,t4,t5)) {}

        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
        val2user(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6) : user(new T(t1, t2, t3,t4,t5, t6)) {}

        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
        val2user(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7) : user(new T(t1, t2, t3,t4,t5, t6, t7)) {}

        template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
        val2user(T1 t1, T2 t2, T3 t3, T4 t4, T5 t5, T6 t6, T7 t7, T8 t8) : user(new T(t1, t2, t3,t4,t5, t6, t7, t8)) {}

        ~val2user() { delete ((T*)m_p); }
    };

    // struct ptr2user 类型
    // 将指针直接构造struct user类型
    template<typename T>
    struct ptr2user : user
    {
        ptr2user(T* t) : user((void*)t) {}
    };

    // struct ref2user 类型
    // 将引用直接构造struct user类型
    template<typename T>
    struct ref2user : user
    {
        ref2user(T& t) : user(&t) {}
    };

    // to lua

    // struct val2lua
    // 将T类型值input构造成val2user<T>结构体，并传入lua，val2user<T>分配在lua上，
    // 而val2user<T>中的指针指向的T类型值通过new分配在C++堆上, 注意是通过拷贝构造函数, 注意是浅拷贝
    template<typename T>
    struct val2lua { static void invoke(lua_State *L, T& input){ new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(input); } };

    // struct ptr2lua
    // 将T类型值input构造成ptr2user<T>结构体，并传入lua，ptr2user<T>分配在lua上，
    // 而ptr2user<T>中的指针指向C++中的T类型变量，如果input是NULL指针，lua_pushnil
    template<typename T>
    struct ptr2lua { static void invoke(lua_State *L, T* input){ if(input) new(lua_newuserdata(L, sizeof(ptr2user<T>))) ptr2user<T>(input); else lua_pushnil(L); } };

    // struct ref2lua
    // 将T类型值input构造成ref2user<T>结构体，并传入lua，ref2user<T>分配在lua上，
    // 而ref2user<T>中的指针指向C++中的T类型变量
    template<typename T>
    struct ref2lua { static void invoke(lua_State *L, T& input){ new(lua_newuserdata(L, sizeof(ref2user<T>))) ref2user<T>(input); } };

    // struct enum2lua
    // 将枚举T类型以lua_number的类型压入lua
    template<typename T>
    struct enum2lua { static void invoke(lua_State *L, T val) { lua_pushnumber(L, (int)val); } };

    // struct object2lua
    // 自动判断是值，指针或者引用，查找对应的user结构体进行赋值，
    // 并保存在lua栈上，并绑定对象相应的metatable
    template<typename T>
    struct object2lua
    {
        static void invoke(lua_State *L, T val)
        {
            if_<is_ptr<T>::value
               ,ptr2lua<typename base_type<T>::type>
               ,typename if_<is_ref<T>::value
                            ,ref2lua<typename base_type<T>::type>
                            ,val2lua<typename base_type<T>::type>
                            >::type
               >::type::invoke(L, val);

            push_meta(L, class_name<typename class_type<T>::type>::name());
            lua_setmetatable(L, -2);
        }
    };

    // type2lua 函数
    // 自动判断是object还是枚举, 直接转换成相应的对象传入lua stack
    template<typename T>
    void type2lua(lua_State *L, T val)
    {
        if_<is_enum<T>::value
           ,enum2lua<T>
           ,object2lua<T>
           >::type::invoke(L, val);
    }

    // get value from cclosure
    template<typename T>
    T upvalue_(lua_State *L)
    {
        return user2type<T>::invoke(L, lua_upvalueindex(1));
    }

    // read a value from lua stack
    // 在lua2type(object, enum)基础上增加基本类型的读取
    // 并不会清除lua stack上的元素
    template<typename T>
    T read(lua_State *L, int index)             { return lua2type<T>(L, index); }

    template<>  char*               read(lua_State *L, int index);
    template<>  const char*         read(lua_State *L, int index);
    template<>  char                read(lua_State *L, int index);
    template<>  unsigned char       read(lua_State *L, int index);
    template<>  short               read(lua_State *L, int index);
    template<>  unsigned short      read(lua_State *L, int index);
    template<>  long                read(lua_State *L, int index);
    template<>  unsigned long       read(lua_State *L, int index);
    template<>  int                 read(lua_State *L, int index);
    template<>  unsigned int        read(lua_State *L, int index);
    template<>  float               read(lua_State *L, int index);
    template<>  double              read(lua_State *L, int index);
    template<>  bool                read(lua_State *L, int index);
    template<>  void                read(lua_State *L, int index);
    template<>  long long           read(lua_State *L, int index);
    template<>  unsigned long long  read(lua_State *L, int index);
    template<>  table               read(lua_State *L, int index);

    // push a value to lua stack
    // 在type2lua(object, enum)基础上加入基本类型的压入
    template<typename T>
    void push(lua_State *L, T ret)                  { type2lua<T>(L, ret); }

    template<>  void push(lua_State *L, char ret);
    template<>  void push(lua_State *L, unsigned char ret);
    template<>  void push(lua_State *L, short ret);
    template<>  void push(lua_State *L, unsigned short ret);
    template<>  void push(lua_State *L, long ret);
    template<>  void push(lua_State *L, unsigned long ret);
    template<>  void push(lua_State *L, int ret);
    template<>  void push(lua_State *L, unsigned int ret);
    template<>  void push(lua_State *L, float ret);
    template<>  void push(lua_State *L, double ret);
    template<>  void push(lua_State *L, char* ret);
    template<>  void push(lua_State *L, const char* ret);
    template<>  void push(lua_State *L, bool ret);
    template<>  void push(lua_State *L, lua_value* ret);
    template<>  void push(lua_State *L, long long ret);
    template<>  void push(lua_State *L, unsigned long long ret);
    template<>  void push(lua_State *L, table ret);

    // pop a value from lua stack
    // 在read函数的基础上, 增加弹出lua stack栈上的值
    template<typename T>
    T pop(lua_State *L) { T t = read<T>(L, -1); lua_pop(L, 1); return t; }

    template<>  void    pop(lua_State *L);
    template<>  table   pop(lua_State *L);

    // functor (with return value)
    // 函数构造结构体
    // C函数
    // upvalue_<>(L)获取函数指针
    // 执行该函数，并压入栈
    template<typename RVal, typename T1=void, typename T2=void, typename T3=void, typename T4=void, typename T5=void, typename T6=void, typename T7=void, typename T8=void>
    struct functor
    {
        static int invoke(lua_State *L) { push(L,upvalue_<RVal(*)(T1,T2,T3,T4,T5,T6,T7,T8)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),read<T5>(L,5),read<T6>(L,6),read<T7>(L,7),read<T8>(L,8))); return 1; }
    };

    template<typename RVal, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    struct functor<RVal,T1,T2,T3,T4,T5,T6,T7>
    {
        static int invoke(lua_State *L) { push(L,upvalue_<RVal(*)(T1,T2,T3,T4,T5,T6,T7)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),read<T5>(L,5),read<T6>(L,6),read<T7>(L,7))); return 1; }
    };

    template<typename RVal, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    struct functor<RVal,T1,T2,T3,T4,T5,T6>
    {
        static int invoke(lua_State *L) { push(L,upvalue_<RVal(*)(T1,T2,T3,T4,T5,T6)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),read<T5>(L,5),read<T6>(L,6))); return 1; }
    };

    template<typename RVal, typename T1, typename T2, typename T3, typename T4, typename T5>
    struct functor<RVal,T1,T2,T3,T4,T5>
    {
        static int invoke(lua_State *L) { push(L,upvalue_<RVal(*)(T1,T2,T3,T4,T5)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),read<T5>(L,5))); return 1; }
    };

    template<typename RVal, typename T1, typename T2, typename T3, typename T4>
    struct functor<RVal,T1,T2,T3,T4>
    {
        static int invoke(lua_State *L) { push(L,upvalue_<RVal(*)(T1,T2,T3,T4)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4))); return 1; }
    };

    template<typename RVal, typename T1, typename T2, typename T3>
    struct functor<RVal,T1,T2,T3>
    {
        static int invoke(lua_State *L) { push(L,upvalue_<RVal(*)(T1,T2,T3)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3))); return 1; }
    };

    template<typename RVal, typename T1, typename T2>
    struct functor<RVal,T1,T2>
    {
        static int invoke(lua_State *L) { push(L,upvalue_<RVal(*)(T1,T2)>(L)(read<T1>(L,1),read<T2>(L,2))); return 1; }
    };

    template<typename RVal, typename T1>
    struct functor<RVal,T1>
    {
        static int invoke(lua_State *L) { push(L,upvalue_<RVal(*)(T1)>(L)(read<T1>(L,1))); return 1; }
    };

    template<typename RVal>
    struct functor<RVal>
    {
        static int invoke(lua_State *L) { push(L,upvalue_<RVal(*)()>(L)()); return 1; }
    };

    // functor (without return value)
    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    struct functor<void, T1, T2, T3, T4, T5, T6, T7, T8>
    {
        static int invoke(lua_State *L) { upvalue_<void(*)(T1,T2,T3,T4,T5,T6,T7,T8)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),read<T5>(L,5),read<T6>(L,6),read<T7>(L,7),read<T8>(L,8)); return 0; }
    };

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    struct functor<void, T1, T2, T3, T4, T5, T6, T7>
    {
        static int invoke(lua_State *L) { upvalue_<void(*)(T1,T2,T3,T4,T5,T6,T7)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),read<T5>(L,5),read<T6>(L,6),read<T7>(L,7)); return 0; }
    };

    template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    struct functor<void, T1, T2, T3, T4, T5, T6>
    {
        static int invoke(lua_State *L) { upvalue_<void(*)(T1,T2,T3,T4,T5,T6)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),read<T5>(L,5),read<T6>(L,6)); return 0; }
    };

    template<typename T1, typename T2, typename T3, typename T4, typename T5>
    struct functor<void, T1, T2, T3, T4, T5>
    {
        static int invoke(lua_State *L) { upvalue_<void(*)(T1,T2,T3,T4,T5)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4),read<T5>(L,5)); return 0; }
    };

    template<typename T1, typename T2, typename T3, typename T4>
    struct functor<void, T1, T2, T3, T4>
    {
        static int invoke(lua_State *L) { upvalue_<void(*)(T1,T2,T3,T4)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3),read<T4>(L,4)); return 0; }
    };

    template<typename T1, typename T2, typename T3>
    struct functor<void, T1, T2, T3>
    {
        static int invoke(lua_State *L) { upvalue_<void(*)(T1,T2,T3)>(L)(read<T1>(L,1),read<T2>(L,2),read<T3>(L,3)); return 0; }
    };

    template<typename T1, typename T2>
    struct functor<void, T1, T2>
    {
        static int invoke(lua_State *L) { upvalue_<void(*)(T1,T2)>(L)(read<T1>(L,1),read<T2>(L,2)); return 0; }
    };

    template<typename T1>
    struct functor<void, T1>
    {
        static int invoke(lua_State *L) { upvalue_<void(*)(T1)>(L)(read<T1>(L,1)); return 0; }
    };

    template<>
    struct functor<void>
    {
        static int invoke(lua_State *L) { upvalue_<void(*)()>(L)(); return 0; }
    };

    // functor (non-managed)
    template<typename T1>
    struct functor<int, lua_State*, T1>
    {
        static int invoke(lua_State *L) { return upvalue_<int(*)(lua_State*,T1)>(L)(L,read<T1>(L,1)); }
    };

    template<>
    struct functor<int,lua_State*>
    {
        static int invoke(lua_State *L) { return upvalue_<int(*)(lua_State*)>(L)(L); }
    };

    // push_functor
    // 调用前, 栈最后一个元素一定是函数指针
    // 压入lua_tinker构造的functor::invoke或者mem_functor::invoke的C函数
    // 该C函数闭包一个真正的函数指针
    // 相同参数和返回值的函数, 压入对应类型的functior::invoke或者mem_functor::invoke
    // 但是对应不同的闭包, 闭包为函数调用的真正的函数指针
    template<typename RVal>
    void push_functor(lua_State *L, RVal (*func)())
    {
        (void)func;
        lua_pushcclosure(L, functor<RVal>::invoke, 1);
    }

    template<typename RVal, typename T1>
    void push_functor(lua_State *L, RVal (*func)(T1))
    {
        (void)func;
        lua_pushcclosure(L, functor<RVal,T1>::invoke, 1);
    }

    template<typename RVal, typename T1, typename T2>
    void push_functor(lua_State *L, RVal (*func)(T1,T2))
    {
        (void)func;
        lua_pushcclosure(L, functor<RVal,T1,T2>::invoke, 1);
    }

    template<typename RVal, typename T1, typename T2, typename T3>
    void push_functor(lua_State *L, RVal (*func)(T1,T2,T3))
    {
        (void)func;
        lua_pushcclosure(L, functor<RVal,T1,T2,T3>::invoke, 1);
    }

    template<typename RVal, typename T1, typename T2, typename T3, typename T4>
    void push_functor(lua_State *L, RVal (*func)(T1,T2,T3,T4))
    {
        (void)func;
        lua_pushcclosure(L, functor<RVal,T1,T2,T3,T4>::invoke, 1);
    }

    template<typename RVal, typename T1, typename T2, typename T3, typename T4, typename T5>
    void push_functor(lua_State *L, RVal (*func)(T1,T2,T3,T4,T5))
    {
        (void)func;
        lua_pushcclosure(L, functor<RVal,T1,T2,T3,T4,T5>::invoke, 1);
    }

    template<typename RVal, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    void push_functor(lua_State *L, RVal (*func)(T1,T2,T3,T4,T5,T6))
    {
        (void)func;
        lua_pushcclosure(L, functor<RVal,T1,T2,T3,T4,T5,T6>::invoke, 1);
    }

    template<typename RVal, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    void push_functor(lua_State *L, RVal (*func)(T1,T2,T3,T4,T5,T6,T7))
    {
        (void)func;
        lua_pushcclosure(L, functor<RVal,T1,T2,T3,T4,T5,T6,T7>::invoke, 1);
    }

    template<typename RVal, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    void push_functor(lua_State *L, RVal (*func)(T1,T2,T3,T4,T5,T6,T7,T8))
    {
        (void)func;
        lua_pushcclosure(L, functor<RVal,T1,T2,T3,T4,T5,T6,T7,T8>::invoke, 1);
    }

    // member variable
    // lua_tinker构造的成员变量基类
    struct var_base
    {
        // add virtual destructor fix by fergus
        virtual ~var_base() {};
        virtual void get(lua_State *L) = 0;
        virtual void set(lua_State *L) = 0;
    };

    template<typename T, typename V>
    struct mem_var : var_base
    {
        // 保存成员变量指针
        V T::*_var;
        mem_var(V T::*val) : _var(val) {}
        // get方法
        void get(lua_State *L)  { push<typename if_<is_obj<V>::value,V&,V>::type>(L, read<T*>(L,1)->*(_var));   }
        // set方法
        void set(lua_State *L)  { read<T*>(L,1)->*(_var) = read<V>(L, 3);   }
    };

    // class member functor (with return value)
    template<typename RVal, typename T, typename T1=void, typename T2=void, typename T3=void, typename T4=void, typename T5=void, typename T6=void, typename T7=void, typename T8=void>
    struct mem_functor
    {
        static int invoke(lua_State *L) { push(L,(read<T*>(L,1)->*upvalue_<RVal(T::*)(T1,T2,T3,T4,T5,T6,T7,T8)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6),read<T6>(L,7),read<T7>(L,8),read<T8>(L,9)));; return 1; }
    };

    template<typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    struct mem_functor<RVal,T,T1,T2,T3,T4,T5,T6,T7>
    {
        static int invoke(lua_State *L) { push(L,(read<T*>(L,1)->*upvalue_<RVal(T::*)(T1,T2,T3,T4,T5,T6,T7)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6),read<T6>(L,7),read<T7>(L,8)));; return 1; }
    };

    template<typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    struct mem_functor<RVal,T,T1,T2,T3,T4,T5,T6>
    {
        static int invoke(lua_State *L) { push(L,(read<T*>(L,1)->*upvalue_<RVal(T::*)(T1,T2,T3,T4,T5,T6)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6),read<T6>(L,7)));; return 1; }
    };

    template<typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
    struct mem_functor<RVal,T,T1,T2,T3,T4,T5>
    {
        static int invoke(lua_State *L) { push(L,(read<T*>(L,1)->*upvalue_<RVal(T::*)(T1,T2,T3,T4,T5)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6)));; return 1; }
    };

    template<typename RVal, typename T, typename T1, typename T2, typename T3, typename T4>
    struct mem_functor<RVal,T,T1,T2,T3,T4>
    {
        static int invoke(lua_State *L) { push(L,(read<T*>(L,1)->*upvalue_<RVal(T::*)(T1,T2,T3,T4)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5))); return 1; }
    };

    template<typename RVal, typename T, typename T1, typename T2, typename T3>
    struct mem_functor<RVal,T,T1,T2,T3>
    {
        static int invoke(lua_State *L) { push(L,(read<T*>(L,1)->*upvalue_<RVal(T::*)(T1,T2,T3)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4))); return 1; }
    };

    template<typename RVal, typename T, typename T1, typename T2>
    struct mem_functor<RVal,T,T1,T2>
    {
        static int invoke(lua_State *L) { push(L,(read<T*>(L,1)->*upvalue_<RVal(T::*)(T1,T2)>(L))(read<T1>(L,2),read<T2>(L,3))); return 1; }
    };

    template<typename RVal, typename T, typename T1>
    struct mem_functor<RVal,T,T1>
    {
        static int invoke(lua_State *L) { push(L,(read<T*>(L,1)->*upvalue_<RVal(T::*)(T1)>(L))(read<T1>(L,2))); return 1; }
    };

    template<typename RVal, typename T>
    struct mem_functor<RVal,T>
    {
        static int invoke(lua_State *L) { push(L,(read<T*>(L,1)->*upvalue_<RVal(T::*)()>(L))()); return 1; }
    };

    // class member functor (without return value)
    template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    struct mem_functor<void,T,T1,T2,T3,T4,T5,T6,T7,T8>
    {
        static int invoke(lua_State *L)  { (read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3,T4,T5)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6),read<T6>(L,7),read<T7>(L,8),read<T8>(L,9)); return 0; }
    };

    template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    struct mem_functor<void,T,T1,T2,T3,T4,T5,T6,T7>
    {
        static int invoke(lua_State *L)  { (read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3,T4,T5)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6),read<T6>(L,7),read<T7>(L,8)); return 0; }
    };

    template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    struct mem_functor<void,T,T1,T2,T3,T4,T5,T6>
    {
        static int invoke(lua_State *L)  { (read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3,T4,T5)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6),read<T6>(L,7)); return 0; }
    };

    template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
    struct mem_functor<void,T,T1,T2,T3,T4,T5>
    {
        static int invoke(lua_State *L)  { (read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3,T4,T5)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6)); return 0; }
    };

    template<typename T, typename T1, typename T2, typename T3, typename T4>
    struct mem_functor<void,T,T1,T2,T3,T4>
    {
        static int invoke(lua_State *L)  { (read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3,T4)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5)); return 0; }
    };

    template<typename T, typename T1, typename T2, typename T3>
    struct mem_functor<void,T,T1,T2,T3>
    {
        static int invoke(lua_State *L)  { (read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2,T3)>(L))(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4)); return 0; }
    };

    template<typename T, typename T1, typename T2>
    struct mem_functor<void,T,T1,T2>
    {
        static int invoke(lua_State *L)  { (read<T*>(L,1)->*upvalue_<void(T::*)(T1,T2)>(L))(read<T1>(L,2),read<T2>(L,3)); return 0; }
    };

    template<typename T, typename T1>
    struct mem_functor<void,T,T1>
    {
        static int invoke(lua_State *L)  { (read<T*>(L,1)->*upvalue_<void(T::*)(T1)>(L))(read<T1>(L,2)); return 0; }
    };

    template<typename T>
    struct mem_functor<void,T>
    {
        static int invoke(lua_State *L)  { (read<T*>(L,1)->*upvalue_<void(T::*)()>(L))(); return 0; }
    };

    // class member functor (non-managed)
    template<typename T, typename T1>
    struct mem_functor<int,T,lua_State*,T1>
    {
        static int invoke(lua_State *L) { return (read<T*>(L,1)->*upvalue_<int(T::*)(lua_State*,T1)>(L))(L, read<T1>(L,2)); }
    };

    template<typename T>
    struct mem_functor<int,T,lua_State*>
    {
        static int invoke(lua_State *L) { return (read<T*>(L,1)->*upvalue_<int(T::*)(lua_State*)>(L))(L); }
    };

    // push_functor
    template<typename RVal, typename T>
    void push_functor(lua_State *L, RVal (T::*func)())
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T>::invoke, 1);
    }

    template<typename RVal, typename T>
    void push_functor(lua_State *L, RVal (T::*func)() const)
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1>
    void push_functor(lua_State *L, RVal (T::*func)(T1))
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1>
    void push_functor(lua_State *L, RVal (T::*func)(T1) const)
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1, typename T2>
    void push_functor(lua_State *L, RVal (T::*func)(T1,T2))
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1,T2>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1, typename T2>
    void push_functor(lua_State *L, RVal (T::*func)(T1,T2) const)
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1,T2>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1, typename T2, typename T3>
    void push_functor(lua_State *L, RVal (T::*func)(T1,T2,T3))
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1,T2,T3>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1, typename T2, typename T3>
    void push_functor(lua_State *L, RVal (T::*func)(T1,T2,T3) const)
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1,T2,T3>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1, typename T2, typename T3, typename T4>
    void push_functor(lua_State *L, RVal (T::*func)(T1,T2,T3,T4))
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1,T2,T3,T4>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1, typename T2, typename T3, typename T4>
    void push_functor(lua_State *L, RVal (T::*func)(T1,T2,T3,T4) const)
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1,T2,T3,T4>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
    void push_functor(lua_State *L, RVal (T::*func)(T1,T2,T3,T4,T5))
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1,T2,T3,T4,T5>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
    void push_functor(lua_State *L, RVal (T::*func)(T1,T2,T3,T4,T5) const)
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1,T2,T3,T4,T5>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    void push_functor(lua_State *L, RVal (T::*func)(T1,T2,T3,T4,T5,T6) const)
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1,T2,T3,T4,T5,T6>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    void push_functor(lua_State *L, RVal (T::*func)(T1,T2,T3,T4,T5,T6,T7) const)
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1,T2,T3,T4,T5,T6,T7>::invoke, 1);
    }

    template<typename RVal, typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    void push_functor(lua_State *L, RVal (T::*func)(T1,T2,T3,T4,T5,T6,T7,T8) const)
    {
        (void)func;
        lua_pushcclosure(L, mem_functor<RVal,T,T1,T2,T3,T4,T5,T6,T7,T8>::invoke, 1);
    }

    // constructor
    // lua_tinker产生的相应的构造函数
    // 创建val2user<T>结构体在lua上
    // 设置该fulluserdata的metatable为相应的metatable
    template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    int constructor(lua_State *L)
    {
        new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6),read<T6>(L,7),read<T7>(L,8),read<T8>(L,9));
        push_meta(L, class_name<typename class_type<T>::type>::name());
        lua_setmetatable(L, -2);

        return 1;
    }

    template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    int constructor(lua_State *L)
    {
        new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6),read<T6>(L,7),read<T7>(L,8));
        push_meta(L, class_name<typename class_type<T>::type>::name());
        lua_setmetatable(L, -2);

        return 1;
    }

    template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    int constructor(lua_State *L)
    {
        new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6),read<T6>(L,7));
        push_meta(L, class_name<typename class_type<T>::type>::name());
        lua_setmetatable(L, -2);

        return 1;
    }

    template<typename T, typename T1, typename T2, typename T3, typename T4, typename T5>
    int constructor(lua_State *L)
    {
        new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5),read<T5>(L,6));
        push_meta(L, class_name<typename class_type<T>::type>::name());
        lua_setmetatable(L, -2);

        return 1;
    }

    template<typename T, typename T1, typename T2, typename T3, typename T4>
    int constructor(lua_State *L)
    {
        new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4),read<T4>(L,5));
        push_meta(L, class_name<typename class_type<T>::type>::name());
        lua_setmetatable(L, -2);

        return 1;
    }

    template<typename T, typename T1, typename T2, typename T3>
    int constructor(lua_State *L)
    {
        new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(read<T1>(L,2),read<T2>(L,3),read<T3>(L,4));
        push_meta(L, class_name<typename class_type<T>::type>::name());
        lua_setmetatable(L, -2);

        return 1;
    }

    template<typename T, typename T1, typename T2>
    int constructor(lua_State *L)
    {
        new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(read<T1>(L,2),read<T2>(L,3));
        push_meta(L, class_name<typename class_type<T>::type>::name());
        lua_setmetatable(L, -2);

        return 1;
    }

    template<typename T, typename T1>
    int constructor(lua_State *L)
    {
        new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>(read<T1>(L,2));
        push_meta(L, class_name<typename class_type<T>::type>::name());
        lua_setmetatable(L, -2);

        return 1;
    }

    template<typename T>
    int constructor(lua_State *L)
    {
        new(lua_newuserdata(L, sizeof(val2user<T>))) val2user<T>();
        push_meta(L, class_name<typename class_type<T>::type>::name());
        lua_setmetatable(L, -2);

        return 1;
    }

    // destroyer
    // 设置__gc
    template<typename T>
    int destroyer(lua_State *L)
    {
        ((user*)lua_touserdata(L, 1))->~user();
        return 0;
    }

    // global function
    // 全局函数
    // 1 压入该函数在lua中的函数名
    // 2 压入函数指针
    // 3 压入相应的functor::invoke, 并闭包上该函数指针
    // 4 设置到全局表中
    template<typename F>
    void def(lua_State* L, const char* name, F func)
    {
        lua_pushstring(L, name);
        lua_pushlightuserdata(L, (void*)func);
        push_functor(L, func);
        lua_settable(L, LUA_GLOBALSINDEX);
    }

    // global variable
    // 设置全局变量
    // 调用type2lua, 调用相应的类型
    // object根据T T* T&建立相应的val2lua ptr2lua ref2lua结构体
    // enum 压入相应的lua_number
    // 基本类型压入基本类型
    // 设置到全局表中
    template<typename T>
    void set(lua_State* L, const char* name, T object)
    {
        lua_pushstring(L, name);
        push(L, object);
        lua_settable(L, LUA_GLOBALSINDEX);
    }

    // 获取全局变量
    // 调用lua2type, 调用相应的类型
    // object根据void2type建立相应T T* T&类型的返回值
    // enum返回lua_number类型
    // 基本类型返回基本类型
    template<typename T>
    T get(lua_State* L, const char* name)
    {
        lua_pushstring(L, name);
        lua_gettable(L, LUA_GLOBALSINDEX);
        return pop<T>(L);
    }

    // 设置全局变量, 调用set, 两种作用一样
    template<typename T>
    void decl(lua_State* L, const char* name, T object)
    {
        set(L, name, object);
    }

    // call
    // C++调用lua函数
    // 传入参数不能用引用!!!!!!!!
    template<typename RVal>
    RVal call(lua_State* L, const char* name)
    {
        lua_pushcclosure(L, on_error, 0);
        int errfunc = lua_gettop(L);

        lua_pushstring(L, name);
        lua_gettable(L, LUA_GLOBALSINDEX);

        if(lua_isfunction(L,-1))
        {
            lua_pcall(L, 0, 1, errfunc);
        }
        else
        {
            print_error(L, "lua_tinker::call() attempt to call global `%s' (not a function)", name);
        }

        lua_remove(L, errfunc);
        return pop<RVal>(L);
    }

    template<typename RVal, typename T1>
    RVal call(lua_State* L, const char* name, T1 arg)
    {
        lua_pushcclosure(L, on_error, 0);
        int errfunc = lua_gettop(L);

        lua_pushstring(L, name);
        lua_gettable(L, LUA_GLOBALSINDEX);
        if(lua_isfunction(L,-1))
        {
            push(L, arg);
            lua_pcall(L, 1, 1, errfunc);
        }
        else
        {
            print_error(L, "lua_tinker::call() attempt to call global `%s' (not a function)", name);
        }

        lua_remove(L, errfunc);
        return pop<RVal>(L);
    }

    template<typename RVal, typename T1, typename T2>
    RVal call(lua_State* L, const char* name, T1 arg1, T2 arg2)
    {
        lua_pushcclosure(L, on_error, 0);
        int errfunc = lua_gettop(L);

        lua_pushstring(L, name);
        lua_gettable(L, LUA_GLOBALSINDEX);
        if(lua_isfunction(L,-1))
        {
            push(L, arg1);
            push(L, arg2);
            lua_pcall(L, 2, 1, errfunc);
        }
        else
        {
            print_error(L, "lua_tinker::call() attempt to call global `%s' (not a function)", name);
        }

        lua_remove(L, errfunc);
        return pop<RVal>(L);
    }

    template<typename RVal, typename T1, typename T2, typename T3>
    RVal call(lua_State* L, const char* name, T1 arg1, T2 arg2, T3 arg3)
    {
        lua_pushcclosure(L, on_error, 0);
        int errfunc = lua_gettop(L);

        lua_pushstring(L, name);
        lua_gettable(L, LUA_GLOBALSINDEX);
        if(lua_isfunction(L,-1))
        {
            push(L, arg1);
            push(L, arg2);
            push(L, arg3);
            lua_pcall(L, 3, 1, errfunc);
        }
        else
        {
            print_error(L, "lua_tinker::call() attempt to call global `%s' (not a function)", name);
        }

        lua_remove(L, errfunc);
        return pop<RVal>(L);
    }

    template<typename RVal, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
    RVal call(lua_State* L, const char* name, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6)
    {
        lua_pushcclosure(L, on_error, 0);
        int errfunc = lua_gettop(L);

        lua_getglobal(L,name);
        if(lua_isfunction(L,-1))
        {
            push(L, arg1);
            push(L, arg2);
            push(L, arg3);
            push(L, arg4);
            push(L, arg5);
            push(L, arg6);
            if(lua_pcall(L, 6, 1, errfunc) != 0)
            {
                lua_pop(L, 1);
            }
        }
        else
        {
            print_error(L, "lua_tinker::call() attempt to call global `%s' (not a function)", name);
        }

        lua_remove(L, -2);
        return pop<RVal>(L);
    }

    template<typename RVal, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7>
    RVal call(lua_State* L, const char* name, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7)
    {
        lua_pushcclosure(L, on_error, 0);
        int errfunc = lua_gettop(L);

        lua_getglobal(L,name);
        if(lua_isfunction(L,-1))
        {
            push(L, arg1);
            push(L, arg2);
            push(L, arg3);
            push(L, arg4);
            push(L, arg5);
            push(L, arg6);
            push(L, arg7);
            if(lua_pcall(L, 7, 1, errfunc) != 0)
            {
                lua_pop(L, 1);
            }
        }
        else
        {
            print_error(L, "lua_tinker::call() attempt to call global `%s' (not a function)", name);
        }

        lua_remove(L, -2);
        return pop<RVal>(L);
    }

    template<typename RVal, typename T1, typename T2, typename T3, typename T4, typename T5, typename T6, typename T7, typename T8>
    RVal call(lua_State* L, const char* name, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6, T7 arg7, T8 arg8)
    {
        lua_pushcclosure(L, on_error, 0);
        int errfunc = lua_gettop(L);

        lua_getglobal(L,name);
        if(lua_isfunction(L,-1))
        {
            push(L, arg1);
            push(L, arg2);
            push(L, arg3);
            push(L, arg4);
            push(L, arg5);
            push(L, arg6);
            push(L, arg7);
            push(L, arg8);
            if(lua_pcall(L, 8, 1, errfunc) != 0)
            {
                lua_pop(L, 1);
            }
        }
        else
        {
            print_error(L, "lua_tinker::call() attempt to call global `%s' (not a function)", name);
        }

        lua_remove(L, -2);
        return pop<RVal>(L);
    }

    // class helper
    int meta_get(lua_State *L);
    int meta_set(lua_State *L);
    void push_meta(lua_State *L, const char* name);

    // class init
    // 注册一个类
    // 1 通过class_name<T>结构体设置类名
    // 2 以该类名为名字, 建立一个新的table
    // 3 注册__name __index __newindex __gc方法
    // 4 将该table保存到全局表中
    template<typename T>
    void class_add(lua_State* L, const char* name)
    {
        class_name<T>::name(name);

        lua_pushstring(L, name);
        lua_newtable(L);

        lua_pushstring(L, "__name");
        lua_pushstring(L, name);
        lua_rawset(L, -3);

        lua_pushstring(L, "__index");
        lua_pushcclosure(L, meta_get, 0);
        lua_rawset(L, -3);

        lua_pushstring(L, "__newindex");
        lua_pushcclosure(L, meta_set, 0);
        lua_rawset(L, -3);

        lua_pushstring(L, "__gc");
        lua_pushcclosure(L, destroyer<T>, 0);
        lua_rawset(L, -3);

        lua_settable(L, LUA_GLOBALSINDEX);
    }

    // Tinker Class Inheritence
    // 继承类
    // 1 获取子类的metatable
    // 2 设置__parent方法, 压入父类的metatable
    template<typename T, typename P>
    void class_inh(lua_State* L)
    {
        push_meta(L, class_name<T>::name());
        if(lua_istable(L, -1))
        {
            lua_pushstring(L, "__parent");
            push_meta(L, class_name<P>::name());
            lua_rawset(L, -3);
        }
        lua_pop(L, 1);
    }

    // Tinker Class Constructor
    template<typename T, typename F>
    void class_con(lua_State* L,F func)
    {
        // 获取类table
        push_meta(L, class_name<T>::name());
        if(lua_istable(L, -1))
        {
            // 创建新的table __call
            lua_newtable(L);
            lua_pushstring(L, "__call");
            // 压入构造函数
            lua_pushcclosure(L, func, 0);
            lua_rawset(L, -3);
            // 设置__call为类table的metatable
            lua_setmetatable(L, -2);
        }
        lua_pop(L, 1);
    }

    // Tinker Class Functions
    // 定义成员函数
    // 1 获取类的metatable
    // 2 压入该函数在lua中的成员函数名
    // 3 压入成员函数指针
    // 3 压入相应的mem_functor::invoke, 并闭包上该函数指针
    // 4 设置到类的metatable中
    template<typename T, typename F>
    void class_def(lua_State* L, const char* name, F func)
    {
        push_meta(L, class_name<T>::name());
        if(lua_istable(L, -1))
        {
            lua_pushstring(L, name);
            new(lua_newuserdata(L,sizeof(F))) F(func);
            push_functor(L, func);
            lua_rawset(L, -3);
        }
        lua_pop(L, 1);
    }

    // Tinker Class Variables
    // 设置成员变量
    // 1 获取类的metatable
    // 2 压入成员变量名
    // 3 构造mem_var结构, 压入lua
    // 4 设置键值对
    template<typename T, typename BASE, typename VAR>
    void class_mem(lua_State* L, const char* name, VAR BASE::*val)
    {
        push_meta(L, class_name<T>::name());
        if(lua_istable(L, -1))
        {
            lua_pushstring(L, name);
            new(lua_newuserdata(L,sizeof(mem_var<BASE,VAR>))) mem_var<BASE,VAR>(val);
            lua_rawset(L, -3);
        }
        lua_pop(L, 1);
    }

    // 用来获取类名
    template<typename T>
    struct class_name
    {
        // global name
        static const char* name(const char* name = NULL)
        {
            static char temp[256] = "";
            if(name) strncpy(temp, name, sizeof(temp)-1);
            return temp;
        }
    };

    // Table Object on Stack
    struct table_obj
    {
        table_obj(lua_State* L, int index);
        ~table_obj();

        void inc_ref();
        void dec_ref();

        bool validate();

        template<typename T>
        void set(const char* name, T object)
        {
            if(validate())
            {
                lua_pushstring(m_L, name);
                push(m_L, object);
                lua_settable(m_L, m_index);
            }
        }

        template<typename T>
        T get(const char* name)
        {
            if(validate())
            {
                lua_pushstring(m_L, name);
                lua_gettable(m_L, m_index);
            }
            else
            {
                lua_pushnil(m_L);
            }

            return pop<T>(m_L);
        }

        lua_State*      m_L;
        int             m_index;    // 指向栈上table的位置
        const void*     m_pointer;
        int             m_ref;
    };

    // Table Object Holder
    struct table
    {
        table(lua_State* L);
        table(lua_State* L, int index);
        table(lua_State* L, const char* name);
        table(const table& input);
        ~table();

        template<typename T>
        void set(const char* name, T object)
        {
            m_obj->set(name, object);
        }

        template<typename T>
        T get(const char* name)
        {
            return m_obj->get<T>(name);
        }

        table_obj*      m_obj;
    };

} // namespace lua_tinker

#endif //_LUA_TINKER_H_
