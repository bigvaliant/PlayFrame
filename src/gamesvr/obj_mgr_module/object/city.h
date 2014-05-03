/**
 * @file city.h
 * @brief City对象
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _CITY_H_
#define _CITY_H_

#include "app_def.h"

class City
{
public:
    friend class ObjMgrModule;
    friend class Player;

    City() {}
    ~City() {}
    
    void init() { memset(this, 0, sizeof(*this)); }
    
    void set_player_idx(int32_t player_idx) { m_player_idx = player_idx; }
    int32_t get_player_idx() { return m_player_idx; }

    // 通过protobuf描述的blob数据初始化city
    void set_city_by_db(const Database::CityInfo* city_info);

    void set_population(int32_t population) { m_population = population; }
    int32_t get_population() { return m_population; }

private:
    int32_t         m_player_idx;

    int32_t         m_population;
};

#endif 
