/**
 * @file random.h
 * @brief 线性同余随机数产生器
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#ifndef _RANDOM_H_
#define _RANDOM_H_

#include "comm_def.h"

/**
* @brief 线性同余数随机数生成器
* @note 数据结构与算法分析10.4
* @note X(i+i) = AX(i) mod M
* @note 该算法不适合加密，理论上知道随机种子的值，从数学上可以猜解出整个序列
* @note 注意:
* @note 1、计算机不收集外部信息，只能产生伪随机序列，实际上随机数都是随机序列
* @note 上一个个元素
* @note 2、伪随机序列的一种生成方式是线性同余算法
* @note 3、线性同余算法初始会有一个种子，种子一旦确定就不应该修改，虽然随机序列
* @note 已经定下来，如果修改种子会破坏线性同余随机数分布的均匀性；确定一个种子
* @note 以后，可以确定随机数的周期，有一个大致的评估，如果一直改动种子，虽然看上
* @note 去，也是随机数，但是无法确认随机数的均匀分布，这样就会引入新的问题，随机
*/
class Random
{
public:
    DISALLOW_COPY_AND_ASSIGN(Random);
    explicit Random(int32_t seed = 0);
    int32_t GetRand();

private:
    int32_t seed_;

    static const int32_t A;
    static const int32_t M;
    static const int32_t Q;
    static const int32_t R;
};

#endif // _RANDOM_H_

