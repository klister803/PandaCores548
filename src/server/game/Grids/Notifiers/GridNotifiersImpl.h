/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TRINITY_GRIDNOTIFIERSIMPL_H
#define TRINITY_GRIDNOTIFIERSIMPL_H

#include "GridNotifiers.h"
#include "WorldPacket.h"
#include "Corpse.h"
#include "Player.h"
#include "UpdateData.h"
#include "CreatureAI.h"
#include "SpellAuras.h"

template<class T>
inline void Trinity::VisibleNotifier::Visit(std::shared_ptr<GridRefManager<T>> &m)
{
    for (typename GridRefManager<T>::iterator iter = m->begin(); iter != m->end(); ++iter)
    {
        vis_guids.erase(iter->getSource()->GetGUID());
        i_player->UpdateVisibilityOf(iter->getSource(), i_data, i_visibleNow);
    }
}

inline void Trinity::ObjectUpdater::Visit(std::shared_ptr<GridRefManager<Creature>> &m)
{
    for (GridRefManager<Creature>::iterator iter = m->begin(); iter != m->end(); ++iter)
        if (iter->getSource()->IsInWorld())
            iter->getSource()->Update(i_timeDiff);
}

// SEARCHERS & LIST SEARCHERS & WORKERS

// WorldObject searchers & workers

template<class Check>
void Trinity::WorldObjectSearcher<Check>::Visit(std::shared_ptr<GridRefManager<GameObject>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_GAMEOBJECT))
        return;

    // already found
    if (i_object)
        return;

    for (GridRefManager<GameObject>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void Trinity::WorldObjectSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Player>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_PLAYER))
        return;

    // already found
    if (i_object)
        return;

    for (GridRefManager<Player>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void Trinity::WorldObjectSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Creature>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CREATURE))
        return;

    // already found
    if (i_object)
        return;

    for (GridRefManager<Creature>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void Trinity::WorldObjectSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Corpse>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CORPSE))
        return;

    // already found
    if (i_object)
        return;

    for (GridRefManager<Corpse>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void Trinity::WorldObjectSearcher<Check>::Visit(std::shared_ptr<GridRefManager<DynamicObject>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_DYNAMICOBJECT))
        return;

    // already found
    if (i_object)
        return;

    for (GridRefManager<DynamicObject>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}


template<class Check>
void Trinity::WorldObjectLastSearcher<Check>::Visit(std::shared_ptr<GridRefManager<GameObject>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_GAMEOBJECT))
        return;

    for (GridRefManager<GameObject>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
            i_object = itr->getSource();
    }
}

template<class Check>
void Trinity::WorldObjectLastSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Player>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_PLAYER))
        return;

    for (GridRefManager<Player>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
            i_object = itr->getSource();
    }
}

template<class Check>
void Trinity::WorldObjectLastSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Creature>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CREATURE))
        return;

    for (GridRefManager<Creature>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
            i_object = itr->getSource();
    }
}

template<class Check>
void Trinity::WorldObjectLastSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Corpse>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CORPSE))
        return;

    for (GridRefManager<Corpse>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
            i_object = itr->getSource();
    }
}

template<class Check>
void Trinity::WorldObjectLastSearcher<Check>::Visit(std::shared_ptr<GridRefManager<DynamicObject>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_DYNAMICOBJECT))
        return;

    for (GridRefManager<DynamicObject>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
            i_object = itr->getSource();
    }
}

template<class Check>
void Trinity::WorldObjectListSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Player>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_PLAYER))
        return;

    for (GridRefManager<Player>::iterator itr=m->begin(); itr != m->end(); ++itr)
        if (i_check(itr->getSource()))
            i_objects.push_back(itr->getSource());
}

template<class Check>
void Trinity::WorldObjectListSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Creature>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CREATURE))
        return;

    for (GridRefManager<Creature>::iterator itr=m->begin(); itr != m->end(); ++itr)
        if (i_check(itr->getSource()))
            i_objects.push_back(itr->getSource());
}

template<class Check>
void Trinity::WorldObjectListSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Corpse>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CORPSE))
        return;

    for (GridRefManager<Corpse>::iterator itr=m->begin(); itr != m->end(); ++itr)
        if (i_check(itr->getSource()))
            i_objects.push_back(itr->getSource());
}

template<class Check>
void Trinity::WorldObjectListSearcher<Check>::Visit(std::shared_ptr<GridRefManager<GameObject>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_GAMEOBJECT))
        return;

    for (GridRefManager<GameObject>::iterator itr=m->begin(); itr != m->end(); ++itr)
        if (i_check(itr->getSource()))
            i_objects.push_back(itr->getSource());
}

template<class Check>
void Trinity::WorldObjectListSearcher<Check>::Visit(std::shared_ptr<GridRefManager<DynamicObject>> &m)
{
    if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_DYNAMICOBJECT))
        return;

    for (GridRefManager<DynamicObject>::iterator itr=m->begin(); itr != m->end(); ++itr)
        if (i_check(itr->getSource()))
            i_objects.push_back(itr->getSource());
}

// Gameobject searchers

template<class Check>
void Trinity::GameObjectSearcher<Check>::Visit(std::shared_ptr<GridRefManager<GameObject>> &m)
{
    // already found
    if (i_object)
        return;

    for (GridRefManager<GameObject>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void Trinity::GameObjectLastSearcher<Check>::Visit(std::shared_ptr<GridRefManager<GameObject>> &m)
{
    for (GridRefManager<GameObject>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
            i_object = itr->getSource();
    }
}

template<class Check>
void Trinity::GameObjectListSearcher<Check>::Visit(std::shared_ptr<GridRefManager<GameObject>> &m)
{
    for (GridRefManager<GameObject>::iterator itr=m->begin(); itr != m->end(); ++itr)
        if (itr->getSource()->InSamePhase(i_phaseMask))
            if (i_check(itr->getSource()))
                i_objects.push_back(itr->getSource());
}

// Unit searchers

template<class Check>
void Trinity::UnitSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Creature>> &m)
{
    // already found
    if (i_object)
        return;

    for (GridRefManager<Creature>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void Trinity::UnitSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Player>> &m)
{
    // already found
    if (i_object)
        return;

    for (GridRefManager<Player>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void Trinity::UnitLastSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Creature>> &m)
{
    for (GridRefManager<Creature>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
            i_object = itr->getSource();
    }
}

template<class Check>
void Trinity::UnitLastSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Player>> &m)
{
    for (GridRefManager<Player>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
            i_object = itr->getSource();
    }
}

template<class Check>
void Trinity::UnitListSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Player>> &m)
{
    for (GridRefManager<Player>::iterator itr=m->begin(); itr != m->end(); ++itr)
        if (itr->getSource()->InSamePhase(i_phaseMask))
            if (i_check(itr->getSource()))
                i_objects.push_back(itr->getSource());
}

template<class Check>
void Trinity::UnitListSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Creature>> &m)
{
    for (GridRefManager<Creature>::iterator itr=m->begin(); itr != m->end(); ++itr)
        if (itr->getSource()->InSamePhase(i_phaseMask))
            if (i_check(itr->getSource()))
                i_objects.push_back(itr->getSource());
}

// Creature searchers

template<class Check>
void Trinity::CreatureSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Creature>> &m)
{
    // already found
    if (i_object)
        return;

    for (GridRefManager<Creature>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void Trinity::CreatureLastSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Creature>> &m)
{
    for (GridRefManager<Creature>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
            i_object = itr->getSource();
    }
}

template<class Check>
void Trinity::CreatureListSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Creature>> &m)
{
    for (GridRefManager<Creature>::iterator itr=m->begin(); itr != m->end(); ++itr)
        if (itr->getSource()->InSamePhase(i_phaseMask))
            if (i_check(itr->getSource()))
                i_objects.push_back(itr->getSource());
}

template<class Check>
void Trinity::PlayerListSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Player>> &m)
{
    for (GridRefManager<Player>::iterator itr=m->begin(); itr != m->end(); ++itr)
        if (itr->getSource()->InSamePhase(i_phaseMask))
            if (i_check(itr->getSource()))
                i_objects.push_back(itr->getSource());
}

template<class Check>
void Trinity::PlayerSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Player>> &m)
{
    // already found
    if (i_object)
        return;

    for (GridRefManager<Player>::iterator itr=m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
        {
            i_object = itr->getSource();
            return;
        }
    }
}

template<class Check>
void Trinity::PlayerLastSearcher<Check>::Visit(std::shared_ptr<GridRefManager<Player>>& m)
{
    for (GridRefManager<Player>::iterator itr = m->begin(); itr != m->end(); ++itr)
    {
        if (!itr->getSource()->InSamePhase(i_phaseMask))
            continue;

        if (i_check(itr->getSource()))
            i_object = itr->getSource();
    }
}

template<class Builder>
void Trinity::LocalizedPacketDo<Builder>::operator()(PlayerPtr p)
{
    LocaleConstant loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
    uint32 cache_idx = loc_idx+1;
    WorldPacket* data;

    // create if not cached yet
    if (i_data_cache.size() < cache_idx+1 || !i_data_cache[cache_idx])
    {
        if (i_data_cache.size() < cache_idx+1)
            i_data_cache.resize(cache_idx+1);

        data = new WorldPacket(SMSG_MESSAGECHAT, 200);

        i_builder(*data, loc_idx);

        i_data_cache[cache_idx] = data;
    }
    else
        data = i_data_cache[cache_idx];

    p->SendDirectMessage(data);
}

template<class Builder>
void Trinity::LocalizedPacketListDo<Builder>::operator()(PlayerPtr p)
{
    LocaleConstant loc_idx = p->GetSession()->GetSessionDbLocaleIndex();
    uint32 cache_idx = loc_idx+1;
    WorldPacketList* data_list;

    // create if not cached yet
    if (i_data_cache.size() < cache_idx+1 || i_data_cache[cache_idx].empty())
    {
        if (i_data_cache.size() < cache_idx+1)
            i_data_cache.resize(cache_idx+1);

        data_list = &i_data_cache[cache_idx];

        i_builder(*data_list, loc_idx);
    }
    else
        data_list = &i_data_cache[cache_idx];

    for (size_t i = 0; i < data_list->size(); ++i)
        p->SendDirectMessage((*data_list)[i]);
}

#endif                                                      // TRINITY_GRIDNOTIFIERSIMPL_H
