/**
 * @file obj_mgr_module.cpp
 * @brief 对象管理模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#include "obj_mgr_module.h"
#include "app.h"
#include "config_module.h"

ObjMgrModule::ObjMgrModule(App* app)
	: AppModuleBase(app)
{}

ObjMgrModule::~ObjMgrModule()
{}

void ObjMgrModule::ModuleInit()
{
    ConfigModule* conf_module = FindModule<ConfigModule>(app_);
    int32_t player_pool_shm_key = conf_module->GetPlayerPoolShmKey();
    int32_t player_pool_size = conf_module->GetPlayerPoolSize();

    CHECK(m_player_pool.Init(player_pool_size, player_pool_shm_key) == 0)
        << "player_pool init error!";

    int32_t city_pool_shm_key = conf_module->GetCityPoolShmKey();
    int32_t city_pool_size = conf_module->GetCityPoolSize();

    CHECK(m_city_pool.Init(city_pool_size, city_pool_shm_key) == 0)
        << "city_pool init error!";

    // 恢复uid 和 player_idx关系
    m_player_pool.TravelPool(&ObjMgrModule::restore_uid_to_player_idx_map, this, 0);

	LOG(INFO) << ModuleName() << " init ok!";
}

int32_t ObjMgrModule::find_player_idx_by_uid(uint64_t uid)
{
    LOG(INFO) << "find_player_idx_by_uid[" << uid << "]";
    UidToPlayerIdxMap_T::iterator it = m_uid_to_player_idx_map.find(uid);
    if (it != m_uid_to_player_idx_map.end()) {
        int32_t player_idx = it->second;
        LOG(INFO)
            << "m_uid_to_player_idx_map find uid[" << uid
            << "] player_idx[" << player_idx
            << "].";
        return player_idx;
    }
    return 0;
}

void ObjMgrModule::bind_uid_to_player_idx(uint64_t uid, int32_t player_idx)
{
    m_uid_to_player_idx_map.insert(UidToPlayerIdxMap_T::value_type(uid, player_idx));
    LOG(INFO)
        << "m_uid_to_player_idx_map insert uid[" << uid
        << "] player_idx[" << player_idx
        << "].";
}

void ObjMgrModule::erase_uid_to_player_idx(uint64_t uid, int32_t player_idx)
{
    UidToPlayerIdxMap_T::iterator it = m_uid_to_player_idx_map.find(uid);
    if (it != m_uid_to_player_idx_map.end()) {
        if (it->second == player_idx) {
            m_uid_to_player_idx_map.erase(it);
            LOG(INFO)
                << "m_uid_to_player_idx_map erase uid[" << it->first
                << "] player_idx[" << it->second
                << "].";
        } else {
            LOG(ERROR)
                << "uid [" << uid
                << "] player_idx[" << player_idx
                << "], in m_uid_to_player_idx_map uid[" << it->first
                << "] player_idx[" << it->second
                << "].";
        }
    }
}

void ObjMgrModule::restore_uid_to_player_idx_map(Player* player, void* args)
{
    ObjMgrModule* obj_mgr_module = (ObjMgrModule*)(args);
    obj_mgr_module->restore_uid_to_player_idx_map(player);
}

void ObjMgrModule::restore_uid_to_player_idx_map(Player* player)
{
    bind_uid_to_player_idx(player->get_uid(), player->get_player_idx());
}

void ObjMgrModule::ModuleFini()
{
	LOG(INFO) << ModuleName() << " fini completed!";
}

const char* ObjMgrModule::ModuleName() const
{
	static const std::string ModuleName = "ObjMgrModule";
	return ModuleName.c_str();
}

int32_t ObjMgrModule::ModuleId()
{
	return MODULE_ID_PLAYER_MGR;
}

AppModuleBase* ObjMgrModule::CreateModule(App* app)
{
	ObjMgrModule* obj_mgr_module = new ObjMgrModule(app);
	if (obj_mgr_module != NULL) {
        obj_mgr_module->ModuleInit();
	}

	return static_cast<AppModuleBase*>(obj_mgr_module);
}

int32_t ObjMgrModule::add_player()
{
    Player* player = (Player*)m_player_pool.Alloc();
    player->init();
    if (player != NULL) {
        int32_t player_idx = m_player_pool.GetPos((void*)player);
        LOG(INFO)
            << "add_player mem_addr[" << (uint64_t)player
            << "] player_idx[" << player_idx
            << "]";
        return player_idx;
    }
    
    return -1;
}

Player* ObjMgrModule::get_player(int32_t player_idx)
{
    if (player_idx <= 0) {
        LOG(ERROR) 
            << "player_idx[" << player_idx
            << "] must be > 0";
        return NULL;
    }

    LOG(INFO) << "get_player[" << player_idx << "]";
    Player* player = (Player*)m_player_pool.Find(player_idx);
    if (player == NULL) {
        LOG(ERROR)
            << "player_idx[" << player_idx
            << "] can't find.";
    }
    return player;
}

int32_t ObjMgrModule::get_player_idx(Player* player)
{
    int32_t player_idx = m_player_pool.GetPos((void*)player);
    return player_idx;
}

void ObjMgrModule::del_player(int32_t player_idx)
{
    Player* player = get_player(player_idx);
    if (player != NULL) {
        uint64_t uid = player->get_uid();
        int32_t player_idx = get_player_idx(player);
        erase_uid_to_player_idx(uid, player_idx);
        // 注意: 每添加一种Player包含的内存对象, 则需要添加相应的释放
        // 有其他内存对象
        if (player->m_city_list[0] > 0) {
            for (int32_t i = 1; i < MAX_CITY_NUM_PER_PLAYER; i++) {
                if (player->m_city_list[i] != 0) {
                    del_city(player->m_city_list[i]);        
                }
            }
        }
        // 释放Player
        m_player_pool.Release((void*)player);
        LOG(INFO) << "del_player player_idx[" << player_idx << "]";
    }
}

int32_t ObjMgrModule::add_city(int32_t player_idx)
{
    Player* player = get_player(player_idx);
    if (player == NULL) {
        LOG(ERROR)
            << "player_idx[" << player_idx
            << "] is not existed!";
        return -1;
    }

    if (player->m_city_list[0] >= MAX_CITY_NUM_PER_PLAYER) {
        LOG(ERROR)
            << "player_idx[" << player_idx
            << "] city num beyond MAX_CITY_NUM_PER_PLAYER[" << MAX_CITY_NUM_PER_PLAYER
            << "]";
        return -2;
    }

    for (int32_t i = 1; i < MAX_CITY_NUM_PER_PLAYER; i++) {
        if (player->m_city_list[i] == 0) {
            City* city = (City*)m_city_pool.Alloc();
            city->init();
            if (city != NULL) {
                int32_t city_idx = m_city_pool.GetPos((void*)city);
                LOG(INFO)
                    << "add_city mem_addr[" << (uint64_t)city
                    << "] city_idx[" << city_idx
                    << "]";
                (player->m_city_list[0])++;
                player->m_city_list[i] = city_idx;
                LOG(INFO)
                    << "player->m_city_list[0] = " << player->m_city_list[0]
                    << ", player->m_city_list[" << i
                    << "] = " << city_idx;
                city->set_player_idx(player_idx);
                return city_idx;
            }
            return -3;
        }
    }

    return -4;
}

City* ObjMgrModule::get_city(int32_t city_idx)
{
    if (city_idx <= 0) {
        LOG(ERROR) 
            << "city_idx[" << city_idx
            << "] must be > 0";
        return NULL;
    }

    LOG(INFO) << "get_city[" << city_idx << "]";
    City* city = (City*)m_city_pool.Find(city_idx);
    if (city == NULL) {
        LOG(ERROR)
            << "city_idx[" << city_idx
            << "] can't find.";
    }
    return city;
}

int32_t ObjMgrModule::get_city_idx(City* city)
{
    int32_t city_idx = m_city_pool.GetPos((void*)city);
    return city_idx;
}

void ObjMgrModule::del_city(int32_t city_idx)
{
    City* city = get_city(city_idx);
    if (city != NULL) {
        m_city_pool.Release((void*)city);
        LOG(INFO) << "del_city city_idx[" << city_idx << "]";
    }
}

