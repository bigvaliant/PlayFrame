/**
 * @file city.cpp
 * @brief City对象
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "city.h"

void City::set_city_by_db(const Database::CityInfo* city_info)
{
    m_population = city_info->population();
}

