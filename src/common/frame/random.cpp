/**
 * @file random.cpp
 * @brief 线性同余随机数产生器
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-04-30
 */
#include "random.h"

const int32_t Random::A = 48271;
const int32_t Random::M = 2147483647;
const int32_t Random::Q = Random::M / Random::A;
const int32_t Random::R = Random::M % Random::A;


Random::Random(int32_t seed)
    : seed_(seed)
{
    if(seed_ == 0)
        seed_ = time(NULL);
}

int32_t Random::GetRand()
{
    int32_t temp_seed = A * ( seed_ % Q ) - R * ( seed_ / Q );
    if(temp_seed >= 0)
        seed_ = temp_seed;
    else
        seed_ = temp_seed + M;
    return seed_;
}

