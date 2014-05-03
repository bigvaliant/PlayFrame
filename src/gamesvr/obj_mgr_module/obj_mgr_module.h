/**
 * @file obj_mgr_module.h
 * @brief 对象管理模块
 * @author fergus <zfengzhen@gmail.com>
 * @version 
 * @date 2014-05-03
 */
#ifndef _OBJ_MGR_MODULE_H_
#define _OBJ_MGR_MODULE_H_

#include "app_def.h"
#include "player.h"
#include "city.h"

class ObjMgrModule : public AppModuleBase
{
public:
    DISALLOW_COPY_AND_ASSIGN(ObjMgrModule);

	ObjMgrModule(App* app);
	virtual ~ObjMgrModule();

	virtual void            ModuleInit();
	virtual void            ModuleFini();
	virtual const char*     ModuleName() const;
	static int32_t          ModuleId();
	static AppModuleBase*   CreateModule(App* app);

public:
    static void restore_uid_to_player_idx_map(Player* player, void* args);
    void restore_uid_to_player_idx_map(Player* player);

    int32_t find_player_idx_by_uid(uint64_t uid);
    void bind_uid_to_player_idx(uint64_t uid, int32_t player_idx);
    void erase_uid_to_player_idx(uint64_t uid, int32_t player_idx);

    int32_t add_player();
    Player* get_player(int32_t player_idx);
    int32_t get_player_idx(Player* player);    
    void del_player(int32_t player_idx);

    int32_t add_city(int32_t player_idx);
    City* get_city(int32_t city_idx);
    int32_t get_city_idx(City* city);    
    void del_city(int32_t city_idx);

private:
    typedef std::map<uint64_t, int32_t> UidToPlayerIdxMap_T;
    UidToPlayerIdxMap_T m_uid_to_player_idx_map;
    ShmPool<Player>     m_player_pool;
    ShmPool<City>       m_city_pool;
};

#endif

