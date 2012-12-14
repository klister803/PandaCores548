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

#ifndef TRINITY_GRIDNOTIFIERS_H
#define TRINITY_GRIDNOTIFIERS_H

#include "ObjectGridLoader.h"
#include "UpdateData.h"
#include <iostream>

#include "Corpse.h"
#include "Object.h"
#include "DynamicObject.h"
#include "GameObject.h"
#include "Player.h"
#include "Unit.h"
#include "CreatureAI.h"
#include "Spell.h"

class Player;
//class Map;

namespace Trinity
{
    struct VisibleNotifier
    {
        PlayerPtr& i_player;
        UpdateData i_data;
        std::set<UnitPtr> i_visibleNow;
        Player::ClientGUIDs vis_guids;

        VisibleNotifier(PlayerPtr& player) : i_player(player), i_data(player->GetMapId()), vis_guids(player->m_clientGUIDs) {}
        template<class T> void Visit(std::shared_ptr<GridRefManager<T>> &m);
        void SendToSelf(void);
    };

    struct VisibleChangesNotifier
    {
        WorldObjectPtr& i_object;

        explicit VisibleChangesNotifier(WorldObjectPtr& object) : i_object(object) {}
        template<class T> void Visit(std::shared_ptr<GridRefManager<T>> &) {}
        void Visit(std::shared_ptr<GridRefManager<Player>> &);
        void Visit(std::shared_ptr<GridRefManager<Creature>> &);
        void Visit(std::shared_ptr<GridRefManager<DynamicObject>> &);
    };

    struct PlayerRelocationNotifier : public VisibleNotifier
    {
        PlayerRelocationNotifier(PlayerPtr& player) : VisibleNotifier(player) {}

        template<class T> void Visit(std::shared_ptr<GridRefManager<T>> &m) { VisibleNotifier::Visit(m); }
        void Visit(std::shared_ptr<GridRefManager<Creature>> &);
        void Visit(std::shared_ptr<GridRefManager<Player>> &);
    };

    struct CreatureRelocationNotifier
    {
        CreaturePtr& i_creature;
        CreatureRelocationNotifier(CreaturePtr& c) : i_creature(c) {}
        template<class T> void Visit(std::shared_ptr<GridRefManager<T>> &) {}
        void Visit(std::shared_ptr<GridRefManager<Creature>> &);
        void Visit(std::shared_ptr<GridRefManager<Player>> &);
    };

    struct DelayedUnitRelocation
    {
        Map &i_map;
        Cell &cell;
        CellCoord &p;
        const float i_radius;
        DelayedUnitRelocation(Cell &c, CellCoord &pair, Map &map, float radius) :
            i_map(map), cell(c), p(pair), i_radius(radius) {}
        template<class T> void Visit(std::shared_ptr<GridRefManager<T>> &) {}
        void Visit(std::shared_ptr<GridRefManager<Creature>> &);
        void Visit(std::shared_ptr<GridRefManager<Player>>   &);
    };

    struct AIRelocationNotifier
    {
        UnitPtr& i_unit;
        bool isCreature;
        explicit AIRelocationNotifier(UnitPtr& unit) : i_unit(unit), isCreature(unit->GetTypeId() == TYPEID_UNIT)  {}
        template<class T> void Visit(std::shared_ptr<GridRefManager<T>> &) {}
        void Visit(std::shared_ptr<GridRefManager<Creature>> &);
    };

    struct GridUpdater
    {
        GridType &i_grid;
        uint32 i_timeDiff;
        GridUpdater(GridType &grid, uint32 diff) : i_grid(grid), i_timeDiff(diff) {}

        template<class T> void updateObjects(std::shared_ptr<GridRefManager<T>> &m)
        {
            for (typename GridRefManager<T>::iterator iter = m->begin(); iter != m->end(); ++iter)
                iter->getSource()->Update(i_timeDiff);
        }

        void Visit(std::shared_ptr<GridRefManager<Player>> &m) { updateObjects<Player>(m); }
        void Visit(std::shared_ptr<GridRefManager<Creature>> &m){ updateObjects<Creature>(m); }
        void Visit(std::shared_ptr<GridRefManager<GameObject>> &m) { updateObjects<GameObject>(m); }
        void Visit(std::shared_ptr<GridRefManager<DynamicObject>> &m) { updateObjects<DynamicObject>(m); }
        void Visit(std::shared_ptr<GridRefManager<Corpse>> &m) { updateObjects<Corpse>(m); }
    };

    struct MessageDistDeliverer
    {
        WorldObjectPtr i_source;
        WorldPacket* i_message;
        uint32 i_phaseMask;
        float i_distSq;
        uint32 team;
        constPlayerPtr skipped_receiver;
        MessageDistDeliverer(WorldObjectPtr src, WorldPacket* msg, float dist, bool own_team_only = false, constPlayerPtr skipped = NULL)
            : i_source(src), i_message(msg), i_phaseMask(src->GetPhaseMask()), i_distSq(dist * dist)
            , team((own_team_only && src->GetTypeId() == TYPEID_PLAYER) ? (TO_PLAYER(src))->GetTeam() : 0)
            , skipped_receiver(skipped)
        {
        }
        void Visit(std::shared_ptr<GridRefManager<Player>> &m);
        void Visit(std::shared_ptr<GridRefManager<Creature>> &m);
        void Visit(std::shared_ptr<GridRefManager<DynamicObject>> &m);
        template<class SKIP> void Visit(std::shared_ptr<GridRefManager<SKIP>> &) {}

        void SendPacket(PlayerPtr player)
        {
            // never send packet to self
            if (player == i_source || (team && player->GetTeam() != team) || skipped_receiver == player)
                return;

            if (!player->HaveAtClient(i_source))
                return;

            if (WorldSession* session = player->GetSession())
                session->SendPacket(i_message);
        }
    };

    struct ObjectUpdater
    {
        uint32 i_timeDiff;
        explicit ObjectUpdater(const uint32 diff) : i_timeDiff(diff) {}
        template<class T> void Visit(std::shared_ptr<GridRefManager<T>> &m);
        void Visit(std::shared_ptr<GridRefManager<Player>> &) {}
        void Visit(std::shared_ptr<GridRefManager<Corpse>> &) {}
        void Visit(std::shared_ptr<GridRefManager<Creature>> &);
    };

    // SEARCHERS & LIST SEARCHERS & WORKERS

    // WorldObject searchers & workers

    template<class Check>
    struct WorldObjectSearcher
    {
        uint32 i_mapTypeMask;
        uint32 i_phaseMask;
        WorldObjectPtr &i_object;
        Check &i_check;

        WorldObjectSearcher(constWorldObjectPtr searcher, WorldObjectPtr & result, Check& check, uint32 mapTypeMask = GRID_MAP_TYPE_MASK_ALL)
            : i_mapTypeMask(mapTypeMask), i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<GameObject>> &m);
        void Visit(std::shared_ptr<GridRefManager<Player>> &m);
        void Visit(std::shared_ptr<GridRefManager<Creature>> &m);
        void Visit(std::shared_ptr<GridRefManager<Corpse>> &m);
        void Visit(std::shared_ptr<GridRefManager<DynamicObject>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    template<class Check>
    struct WorldObjectLastSearcher
    {
        uint32 i_mapTypeMask;
        uint32 i_phaseMask;
        WorldObjectPtr &i_object;
        Check &i_check;

        WorldObjectLastSearcher(constWorldObjectPtr searcher, WorldObjectPtr & result, Check& check, uint32 mapTypeMask = GRID_MAP_TYPE_MASK_ALL)
            :  i_mapTypeMask(mapTypeMask), i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<GameObject>> &m);
        void Visit(std::shared_ptr<GridRefManager<Player>> &m);
        void Visit(std::shared_ptr<GridRefManager<Creature>> &m);
        void Visit(std::shared_ptr<GridRefManager<Corpse>> &m);
        void Visit(std::shared_ptr<GridRefManager<DynamicObject>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    template<class Check>
    struct WorldObjectListSearcher
    {
        uint32 i_mapTypeMask;
        uint32 i_phaseMask;
        std::list<WorldObjectPtr> &i_objects;
        Check& i_check;

        WorldObjectListSearcher(constWorldObjectPtr searcher, std::list<WorldObjectPtr> &objects, Check & check, uint32 mapTypeMask = GRID_MAP_TYPE_MASK_ALL)
            : i_mapTypeMask(mapTypeMask), i_phaseMask(searcher->GetPhaseMask()), i_objects(objects), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<Player>> &m);
        void Visit(std::shared_ptr<GridRefManager<Creature>> &m);
        void Visit(std::shared_ptr<GridRefManager<Corpse>> &m);
        void Visit(std::shared_ptr<GridRefManager<GameObject>> &m);
        void Visit(std::shared_ptr<GridRefManager<DynamicObject>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    template<class Do>
    struct WorldObjectWorker
    {
        uint32 i_mapTypeMask;
        uint32 i_phaseMask;
        Do const& i_do;

        WorldObjectWorker(constWorldObjectPtr searcher, Do const& _do, uint32 mapTypeMask = GRID_MAP_TYPE_MASK_ALL)
            : i_mapTypeMask(mapTypeMask), i_phaseMask(searcher->GetPhaseMask()), i_do(_do) {}

        void Visit(std::shared_ptr<GridRefManager<GameObject>> &m)
        {
            if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_GAMEOBJECT))
                return;
            for (GridRefManager<GameObject>::iterator itr=m->begin(); itr != m->end(); ++itr)
                if (itr->getSource()->InSamePhase(i_phaseMask))
                    i_do(itr->getSource());
        }

        void Visit(std::shared_ptr<GridRefManager<Player>> &m)
        {
            if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_PLAYER))
                return;
            for (GridRefManager<Player>::iterator itr=m->begin(); itr != m->end(); ++itr)
                if (itr->getSource()->InSamePhase(i_phaseMask))
                    i_do(itr->getSource());
        }
        void Visit(std::shared_ptr<GridRefManager<Creature>> &m)
        {
            if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CREATURE))
                return;
            for (GridRefManager<Creature>::iterator itr=m->begin(); itr != m->end(); ++itr)
                if (itr->getSource()->InSamePhase(i_phaseMask))
                    i_do(itr->getSource());
        }

        void Visit(std::shared_ptr<GridRefManager<Corpse>> &m)
        {
            if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_CORPSE))
                return;
            for (GridRefManager<Corpse>::iterator itr=m->begin(); itr != m->end(); ++itr)
                if (itr->getSource()->InSamePhase(i_phaseMask))
                    i_do(itr->getSource());
        }

        void Visit(std::shared_ptr<GridRefManager<DynamicObject>> &m)
        {
            if (!(i_mapTypeMask & GRID_MAP_TYPE_MASK_DYNAMICOBJECT))
                return;
            for (GridRefManager<DynamicObject>::iterator itr=m->begin(); itr != m->end(); ++itr)
                if (itr->getSource()->InSamePhase(i_phaseMask))
                    i_do(itr->getSource());
        }

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    // Gameobject searchers

    template<class Check>
    struct GameObjectSearcher
    {
        uint32 i_phaseMask;
        GameObjectPtr &i_object;
        Check &i_check;

        GameObjectSearcher(constWorldObjectPtr searcher, GameObjectPtr & result, Check& check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<GameObject>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    // Last accepted by Check GO if any (Check can change requirements at each call)
    template<class Check>
    struct GameObjectLastSearcher
    {
        uint32 i_phaseMask;
        GameObjectPtr &i_object;
        Check& i_check;

        GameObjectLastSearcher(constWorldObjectPtr searcher, GameObjectPtr & result, Check& check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<GameObject>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    template<class Check>
    struct GameObjectListSearcher
    {
        uint32 i_phaseMask;
        std::list<GameObjectPtr> &i_objects;
        Check& i_check;

        GameObjectListSearcher(constWorldObjectPtr searcher, std::list<GameObjectPtr> &objects, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_objects(objects), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<GameObject>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    template<class Functor>
    struct GameObjectWorker
    {
        GameObjectWorker(constWorldObjectPtr searcher, Functor& func)
            : _func(func), _phaseMask(searcher->GetPhaseMask()) {}

        void Visit(std::shared_ptr<GridRefManager<GameObject>>& m)
        {
            for (GridRefManager<GameObject>::iterator itr = m->begin(); itr != m->end(); ++itr)
                if (itr->getSource()->InSamePhase(_phaseMask))
                    _func(itr->getSource());
        }

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}

    private:
        Functor& _func;
        uint32 _phaseMask;
    };

    // Unit searchers

    // First accepted by Check Unit if any
    template<class Check>
    struct UnitSearcher
    {
        uint32 i_phaseMask;
        UnitPtr &i_object;
        Check & i_check;

        UnitSearcher(constWorldObjectPtr searcher, UnitPtr & result, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<Creature>> &m);
        void Visit(std::shared_ptr<GridRefManager<Player>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    // Last accepted by Check Unit if any (Check can change requirements at each call)
    template<class Check>
    struct UnitLastSearcher
    {
        uint32 i_phaseMask;
        UnitPtr &i_object;
        Check & i_check;

        UnitLastSearcher(constWorldObjectPtr searcher, UnitPtr & result, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<Creature>> &m);
        void Visit(std::shared_ptr<GridRefManager<Player>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    // All accepted by Check units if any
    template<class Check>
    struct UnitListSearcher
    {
        uint32 i_phaseMask;
        std::list<UnitPtr> &i_objects;
        Check& i_check;

        UnitListSearcher(constWorldObjectPtr searcher, std::list<UnitPtr> &objects, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_objects(objects), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<Player>> &m);
        void Visit(std::shared_ptr<GridRefManager<Creature>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    // Creature searchers

    template<class Check>
    struct CreatureSearcher
    {
        uint32 i_phaseMask;
        CreaturePtr &i_object;
        Check & i_check;

        CreatureSearcher(constWorldObjectPtr searcher, CreaturePtr & result, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<Creature>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    // Last accepted by Check Creature if any (Check can change requirements at each call)
    template<class Check>
    struct CreatureLastSearcher
    {
        uint32 i_phaseMask;
        CreaturePtr &i_object;
        Check & i_check;

        CreatureLastSearcher(constWorldObjectPtr searcher, CreaturePtr & result, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<Creature>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    template<class Check>
    struct CreatureListSearcher
    {
        uint32 i_phaseMask;
        std::list<CreaturePtr> &i_objects;
        Check& i_check;

        CreatureListSearcher(constWorldObjectPtr searcher, std::list<CreaturePtr> &objects, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_objects(objects), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<Creature>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    template<class Do>
    struct CreatureWorker
    {
        uint32 i_phaseMask;
        Do& i_do;

        CreatureWorker(constWorldObjectPtr searcher, Do& _do)
            : i_phaseMask(searcher->GetPhaseMask()), i_do(_do) {}

        void Visit(std::shared_ptr<GridRefManager<Creature>> &m)
        {
            for (GridRefManager<Creature>::iterator itr=m->begin(); itr != m->end(); ++itr)
                if (itr->getSource()->InSamePhase(i_phaseMask))
                    i_do(itr->getSource());
        }

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    // Player searchers

    template<class Check>
    struct PlayerSearcher
    {
        uint32 i_phaseMask;
        PlayerPtr &i_object;
        Check & i_check;

        PlayerSearcher(constWorldObjectPtr searcher, PlayerPtr & result, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<Player>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    template<class Check>
    struct PlayerListSearcher
    {
        uint32 i_phaseMask;
        std::list<PlayerPtr> &i_objects;
        Check& i_check;

        PlayerListSearcher(constWorldObjectPtr searcher, std::list<PlayerPtr> &objects, Check & check)
            : i_phaseMask(searcher->GetPhaseMask()), i_objects(objects), i_check(check) {}

        void Visit(std::shared_ptr<GridRefManager<Player>> &m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    template<class Check>
    struct PlayerLastSearcher
    {
        uint32 i_phaseMask;
        PlayerPtr &i_object;
        Check& i_check;

        PlayerLastSearcher(constWorldObjectPtr searcher, PlayerPtr& result, Check& check) : i_phaseMask(searcher->GetPhaseMask()), i_object(result), i_check(check)
        {
        }

        void Visit(std::shared_ptr<GridRefManager<Player>>& m);

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    template<class Do>
    struct PlayerWorker
    {
        uint32 i_phaseMask;
        Do& i_do;

        PlayerWorker(constWorldObjectPtr searcher, Do& _do)
            : i_phaseMask(searcher->GetPhaseMask()), i_do(_do) {}

        void Visit(std::shared_ptr<GridRefManager<Player>> &m)
        {
            for (GridRefManager<Player>::iterator itr=m->begin(); itr != m->end(); ++itr)
                if (itr->getSource()->InSamePhase(i_phaseMask))
                    i_do(itr->getSource());
        }

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    template<class Do>
    struct PlayerDistWorker
    {
        constWorldObjectPtr i_searcher;
        float i_dist;
        Do& i_do;

        PlayerDistWorker(constWorldObjectPtr searcher, float _dist, Do& _do)
            : i_searcher(searcher), i_dist(_dist), i_do(_do) {}

        void Visit(std::shared_ptr<GridRefManager<Player>> &m)
        {
            for (GridRefManager<Player>::iterator itr=m->begin(); itr != m->end(); ++itr)
                if (itr->getSource()->InSamePhase(i_searcher) && itr->getSource()->IsWithinDist(i_searcher, i_dist))
                    i_do(itr->getSource());
        }

        template<class NOT_INTERESTED> void Visit(std::shared_ptr<GridRefManager<NOT_INTERESTED>> &) {}
    };

    // CHECKS && DO classes

    // WorldObject check classes

    class AnyDeadUnitObjectInRangeCheck
    {
        public:
            AnyDeadUnitObjectInRangeCheck(UnitPtr searchObj, float range) : i_searchObj(searchObj), i_range(range) {}
            bool operator()(PlayerPtr u);
            bool operator()(CorpsePtr u);
            bool operator()(CreaturePtr u);
            template<class NOT_INTERESTED> bool operator()(std::shared_ptr<NOT_INTERESTED>) { return false; }
        protected:
            constUnitPtr const i_searchObj;
            float i_range;
    };

    class AnyDeadUnitSpellTargetInRangeCheck : public AnyDeadUnitObjectInRangeCheck
    {
        public:
            AnyDeadUnitSpellTargetInRangeCheck(UnitPtr searchObj, float range, SpellInfo const* spellInfo, SpellTargetCheckTypes check)
                : AnyDeadUnitObjectInRangeCheck(searchObj, range), i_spellInfo(spellInfo), i_check(searchObj, searchObj, spellInfo, check, NULL)
            {}
            bool operator()(PlayerPtr u);
            bool operator()(CorpsePtr u);
            bool operator()(CreaturePtr u);
            template<class NOT_INTERESTED> bool operator()(std::shared_ptr<NOT_INTERESTED>) { return false; }
        protected:
            SpellInfo const* i_spellInfo;
            WorldObjectSpellTargetCheck i_check;
    };

    // WorldObject do classes

    class RespawnDo
    {
        public:
            RespawnDo() {}
            void operator()(CreaturePtr u) const { u->Respawn(); }
            void operator()(GameObjectPtr u) const { u->Respawn(); }
            void operator()(WorldObjectPtr) const {}
            void operator()(CorpsePtr) const {}
    };

    // GameObject checks

    class GameObjectFocusCheck
    {
        public:
            GameObjectFocusCheck(constUnitPtr unit, uint32 focusId) : i_unit(unit), i_focusId(focusId) {}
            bool operator()(GameObjectPtr go) const
            {
                if (go->GetGOInfo()->type != GAMEOBJECT_TYPE_SPELL_FOCUS)
                    return false;

                if (go->GetGOInfo()->spellFocus.focusId != i_focusId)
                    return false;

                float dist = (float)((go->GetGOInfo()->spellFocus.dist)/2);

                return go->IsWithinDistInMap(i_unit, dist);
            }
        private:
            constUnitPtr i_unit;
            uint32 i_focusId;
    };

    // Find the nearest Fishing hole and return true only if source object is in range of hole
    class NearestGameObjectFishingHole
    {
        public:
            NearestGameObjectFishingHole(constWorldObjectPtr& obj, float range) : i_obj(obj), i_range(range) {}
            bool operator()(GameObjectPtr go)
            {
                if (go->GetGOInfo()->type == GAMEOBJECT_TYPE_FISHINGHOLE && go->isSpawned() && i_obj->IsWithinDistInMap(go, i_range) && i_obj->IsWithinDistInMap(go, (float)go->GetGOInfo()->fishinghole.radius))
                {
                    i_range = i_obj->GetDistance(go);
                    return true;
                }
                return false;
            }
            float GetLastRange() const { return i_range; }
        private:
            constWorldObjectPtr& i_obj;
            float  i_range;

            // prevent clone
            NearestGameObjectFishingHole(NearestGameObjectFishingHole const&);
    };

    class NearestGameObjectCheck
    {
        public:
            NearestGameObjectCheck(constWorldObjectPtr& obj) : i_obj(obj), i_range(999) {}
            bool operator()(GameObjectPtr go)
            {
                if (i_obj->IsWithinDistInMap(go, i_range))
                {
                    i_range = i_obj->GetDistance(go);        // use found GO range as new range limit for next check
                    return true;
                }
                return false;
            }
            float GetLastRange() const { return i_range; }
        private:
            constWorldObjectPtr& i_obj;
            float i_range;

            // prevent clone this object
            NearestGameObjectCheck(NearestGameObjectCheck const&);
    };

    // Success at unit in range, range update for next check (this can be use with GameobjectLastSearcher to find nearest GO)
    class NearestGameObjectEntryInObjectRangeCheck
    {
        public:
            NearestGameObjectEntryInObjectRangeCheck(constWorldObjectPtr& obj, uint32 entry, float range) : i_obj(obj), i_entry(entry), i_range(range) {}
            bool operator()(GameObjectPtr go)
            {
                if (go->GetEntry() == i_entry && i_obj->IsWithinDistInMap(go, i_range))
                {
                    i_range = i_obj->GetDistance(go);        // use found GO range as new range limit for next check
                    return true;
                }
                return false;
            }
            float GetLastRange() const { return i_range; }
        private:
            constWorldObjectPtr& i_obj;
            uint32 i_entry;
            float  i_range;

            // prevent clone this object
            NearestGameObjectEntryInObjectRangeCheck(NearestGameObjectEntryInObjectRangeCheck const&);
    };

    // Success at unit in range, range update for next check (this can be use with GameobjectLastSearcher to find nearest GO with a certain type)
    class NearestGameObjectTypeInObjectRangeCheck
    {
    public:
        NearestGameObjectTypeInObjectRangeCheck(constWorldObjectPtr& obj, GameobjectTypes type, float range) : i_obj(obj), i_type(type), i_range(range) {}
        bool operator()(GameObjectPtr go)
        {
            if (go->GetGoType() == i_type && i_obj->IsWithinDistInMap(go, i_range))
            {
                i_range = i_obj->GetDistance(go);        // use found GO range as new range limit for next check
                return true;
            }
            return false;
        }
        float GetLastRange() const { return i_range; }
    private:
        constWorldObjectPtr& i_obj;
        GameobjectTypes i_type;
        float  i_range;

        // prevent clone this object
        NearestGameObjectTypeInObjectRangeCheck(NearestGameObjectTypeInObjectRangeCheck const&);

    };

    class GameObjectWithDbGUIDCheck
    {
        public:
            GameObjectWithDbGUIDCheck(constWorldObjectPtr& obj, uint32 db_guid) : i_obj(obj), i_db_guid(db_guid) {}
            bool operator()(constGameObjectPtr go) const
            {
                return go->GetDBTableGUIDLow() == i_db_guid;
            }
        private:
            constWorldObjectPtr& i_obj;
            uint32 i_db_guid;
    };

    // Unit checks

    class MostHPMissingInRange
    {
        public:
            MostHPMissingInRange(constUnitPtr obj, float range, uint32 hp) : i_obj(obj), i_range(range), i_hp(hp) {}
            bool operator()(UnitPtr u)
            {
                if (u->isAlive() && u->isInCombat() && !i_obj->IsHostileTo(u) && i_obj->IsWithinDistInMap(u, i_range) && u->GetMaxHealth() - u->GetHealth() > i_hp)
                {
                    i_hp = u->GetMaxHealth() - u->GetHealth();
                    return true;
                }
                return false;
            }
        private:
            constUnitPtr i_obj;
            float i_range;
            uint32 i_hp;
    };

    class FriendlyCCedInRange
    {
        public:
            FriendlyCCedInRange(constUnitPtr obj, float range) : i_obj(obj), i_range(range) {}
            bool operator()(UnitPtr u)
            {
                if (u->isAlive() && u->isInCombat() && !i_obj->IsHostileTo(u) && i_obj->IsWithinDistInMap(u, i_range) &&
                    (u->isFeared() || u->isCharmed() || u->isFrozen() || u->HasUnitState(UNIT_STATE_STUNNED) || u->HasUnitState(UNIT_STATE_CONFUSED)))
                {
                    return true;
                }
                return false;
            }
        private:
            constUnitPtr i_obj;
            float i_range;
    };

    class FriendlyMissingBuffInRange
    {
        public:
            FriendlyMissingBuffInRange(constUnitPtr obj, float range, uint32 spellid) : i_obj(obj), i_range(range), i_spell(spellid) {}
            bool operator()(UnitPtr u)
            {
                if (u->isAlive() && u->isInCombat() && !i_obj->IsHostileTo(u) && i_obj->IsWithinDistInMap(u, i_range) &&
                    !(u->HasAura(i_spell)))
                {
                    return true;
                }
                return false;
            }
        private:
            constUnitPtr i_obj;
            float i_range;
            uint32 i_spell;
    };

    class AnyUnfriendlyUnitInObjectRangeCheck
    {
        public:
            AnyUnfriendlyUnitInObjectRangeCheck(constWorldObjectPtr obj, constUnitPtr funit, float range) : i_obj(obj), i_funit(funit), i_range(range) {}
            bool operator()(UnitPtr u)
            {
                if (u->isAlive() && i_obj->IsWithinDistInMap(u, i_range) && !i_funit->IsFriendlyTo(u))
                    return true;
                else
                    return false;
            }
        private:
            constWorldObjectPtr i_obj;
            constUnitPtr i_funit;
            float i_range;
    };

    class AnyUnfriendlyNoTotemUnitInObjectRangeCheck
    {
        public:
            AnyUnfriendlyNoTotemUnitInObjectRangeCheck(constWorldObjectPtr obj, constUnitPtr funit, float range) : i_obj(obj), i_funit(funit), i_range(range) {}
            bool operator()(UnitPtr u)
            {
                if (!u->isAlive())
                    return false;

                if (u->GetCreatureType() == CREATURE_TYPE_NON_COMBAT_PET)
                    return false;

                if (u->GetTypeId() == TYPEID_UNIT && (TO_CREATURE(u))->isTotem())
                    return false;

                if (!u->isTargetableForAttack(false))
                    return false;

                return i_obj->IsWithinDistInMap(u, i_range) && !i_funit->IsFriendlyTo(u);
            }
        private:
            constWorldObjectPtr i_obj;
            constUnitPtr i_funit;
            float i_range;
    };

    class AnyUnfriendlyAttackableVisibleUnitInObjectRangeCheck
    {
        public:
            AnyUnfriendlyAttackableVisibleUnitInObjectRangeCheck(constUnitPtr funit, float range)
                : i_funit(funit), i_range(range) {}

            bool operator()(constUnitPtr u)
            {
                return u->isAlive()
                    && i_funit->IsWithinDistInMap(u, i_range)
                    && !i_funit->IsFriendlyTo(u)
                    && i_funit->IsValidAttackTarget(u)
                    && u->GetCreatureType() != CREATURE_TYPE_CRITTER
                    && i_funit->canSeeOrDetect(u);
            }
        private:
            constUnitPtr i_funit;
            float i_range;
    };

    class CreatureWithDbGUIDCheck
    {
        public:
            CreatureWithDbGUIDCheck(constWorldObjectPtr obj, uint32 lowguid) : i_obj(obj), i_lowguid(lowguid) {}
            bool operator()(CreaturePtr u)
            {
                return u->GetDBTableGUIDLow() == i_lowguid;
            }
        private:
            constWorldObjectPtr i_obj;
            uint32 i_lowguid;
    };

    class AnyFriendlyUnitInObjectRangeCheck
    {
        public:
            AnyFriendlyUnitInObjectRangeCheck(constWorldObjectPtr obj, constUnitPtr funit, float range) : i_obj(obj), i_funit(funit), i_range(range) {}
            bool operator()(UnitPtr u)
            {
                if (u->isAlive() && i_obj->IsWithinDistInMap(u, i_range) && i_funit->IsFriendlyTo(u))
                    return true;
                else
                    return false;
            }
        private:
            constWorldObjectPtr i_obj;
            constUnitPtr i_funit;
            float i_range;
    };

    class AnyGroupedUnitInObjectRangeCheck
    {
        public:
            AnyGroupedUnitInObjectRangeCheck(constWorldObjectPtr obj, constUnitPtr funit, float range, bool raid) : _source(obj), _refUnit(funit), _range(range), _raid(raid) {}
            bool operator()(UnitPtr u)
            {
                if (_raid)
                {
                    if (!_refUnit->IsInRaidWith(u))
                        return false;
                }
                else if (!_refUnit->IsInPartyWith(u))
                    return false;

                return !_refUnit->IsHostileTo(u) && u->isAlive() && _source->IsWithinDistInMap(u, _range);
            }

        private:
            constWorldObjectPtr _source;
            constUnitPtr _refUnit;
            float _range;
            bool _raid;
    };

    class AnyUnitInObjectRangeCheck
    {
        public:
            AnyUnitInObjectRangeCheck(constWorldObjectPtr obj, float range) : i_obj(obj), i_range(range) {}
            bool operator()(UnitPtr u)
            {
                if (u->isAlive() && i_obj->IsWithinDistInMap(u, i_range))
                    return true;

                return false;
            }
        private:
            constWorldObjectPtr i_obj;
            float i_range;
    };

    // Success at unit in range, range update for next check (this can be use with UnitLastSearcher to find nearest unit)
    class NearestAttackableUnitInObjectRangeCheck
    {
        public:
            NearestAttackableUnitInObjectRangeCheck(constWorldObjectPtr obj, constUnitPtr funit, float range) : i_obj(obj), i_funit(funit), i_range(range) {}
            bool operator()(UnitPtr u)
            {
                if (u->isTargetableForAttack() && i_obj->IsWithinDistInMap(u, i_range) &&
                    !i_funit->IsFriendlyTo(u) && i_funit->canSeeOrDetect(u))
                {
                    i_range = i_obj->GetDistance(u);        // use found unit range as new range limit for next check
                    return true;
                }

                return false;
            }
        private:
            constWorldObjectPtr i_obj;
            constUnitPtr i_funit;
            float i_range;

            // prevent clone this object
            NearestAttackableUnitInObjectRangeCheck(NearestAttackableUnitInObjectRangeCheck const&);
    };

    class AnyAoETargetUnitInObjectRangeCheck
    {
        public:
            AnyAoETargetUnitInObjectRangeCheck(constWorldObjectPtr obj, constUnitPtr funit, float range)
                : i_obj(obj), i_funit(funit), _spellInfo(NULL), i_range(range)
            {
                constUnitPtr check = i_funit;
                constUnitPtr owner = i_funit->GetOwner();
                if (owner)
                    check = owner;
                i_targetForPlayer = (check->GetTypeId() == TYPEID_PLAYER);
                if (i_obj->GetTypeId() == TYPEID_DYNAMICOBJECT)
                    _spellInfo = sSpellMgr->GetSpellInfo((TO_CONST_DYNAMICOBJECT(i_obj))->GetSpellId());
            }
            bool operator()(UnitPtr u)
            {
                // Check contains checks for: live, non-selectable, non-attackable flags, flight check and GM check, ignore totems
                if (u->GetTypeId() == TYPEID_UNIT && (TO_CREATURE(u))->isTotem())
                    return false;

                if (i_funit->_IsValidAttackTarget(u, _spellInfo,i_obj->GetTypeId() == TYPEID_DYNAMICOBJECT ? i_obj : NULL) && i_obj->IsWithinDistInMap(u, i_range))
                    return true;

                return false;
            }
        private:
            bool i_targetForPlayer;
            constWorldObjectPtr i_obj;
            constUnitPtr i_funit;
            SpellInfo const* _spellInfo;
            float i_range;
    };

    // do attack at call of help to friendly crearture
    class CallOfHelpCreatureInRangeDo
    {
        public:
            CallOfHelpCreatureInRangeDo(UnitPtr funit, UnitPtr enemy, float range)
                : i_funit(funit), i_enemy(enemy), i_range(range)
            {}
            void operator()(CreaturePtr u)
            {
                if (u == i_funit)
                    return;

                if (!u->CanAssistTo(i_funit, i_enemy, false))
                    return;

                // too far
                if (!u->IsWithinDistInMap(i_funit, i_range))
                    return;

                // only if see assisted creature's enemy
                if (!u->IsWithinLOSInMap(i_enemy))
                    return;

                if (u->AI())
                    u->AI()->AttackStart(i_enemy);
            }
        private:
            UnitPtr const i_funit;
            UnitPtr const i_enemy;
            float i_range;
    };

    struct AnyDeadUnitCheck
    {
        bool operator()(UnitPtr u) { return !u->isAlive(); }
    };

    /*
    struct AnyStealthedCheck
    {
        bool operator()(UnitPtr u) { return u->GetVisibility() == VISIBILITY_GROUP_STEALTH; }
    };
    */

    // Creature checks

    class NearestHostileUnitCheck
    {
        public:
            explicit NearestHostileUnitCheck(constCreaturePtr creature, float dist = 0) : me(creature)
            {
                m_range = (dist == 0 ? 9999 : dist);
            }
            bool operator()(UnitPtr u)
            {
                if (!me->IsWithinDistInMap(u, m_range))
                    return false;

                if (!me->IsValidAttackTarget(u))
                    return false;

                m_range = me->GetDistance(u);   // use found unit range as new range limit for next check
                return true;
            }

    private:
            constCreaturePtr me;
            float m_range;
            NearestHostileUnitCheck(NearestHostileUnitCheck const&);
    };

    class NearestHostileUnitInAttackDistanceCheck
    {
        public:
            explicit NearestHostileUnitInAttackDistanceCheck(constCreaturePtr creature, float dist = 0) : me(creature)
            {
                m_range = (dist == 0 ? 9999 : dist);
                m_force = (dist == 0 ? false : true);
            }
            bool operator()(UnitPtr u)
            {
                if (!me->IsWithinDistInMap(u, m_range))
                    return false;

                if (!me->canSeeOrDetect(u))
                    return false;

                if (m_force)
                {
                    if (!me->IsValidAttackTarget(u))
                        return false;
                }
                else if (!me->canStartAttack(u, false))
                    return false;

                m_range = me->GetDistance(u);   // use found unit range as new range limit for next check
                return true;
            }
            float GetLastRange() const { return m_range; }
        private:
            constCreaturePtr me;
            float m_range;
            bool m_force;
            NearestHostileUnitInAttackDistanceCheck(NearestHostileUnitInAttackDistanceCheck const&);
    };

    class AnyAssistCreatureInRangeCheck
    {
        public:
            AnyAssistCreatureInRangeCheck(UnitPtr funit, UnitPtr enemy, float range)
                : i_funit(funit), i_enemy(enemy), i_range(range)
            {
            }
            bool operator()(CreaturePtr u)
            {
                if (u == i_funit)
                    return false;

                if (!u->CanAssistTo(i_funit, i_enemy))
                    return false;

                // too far
                if (!i_funit->IsWithinDistInMap(u, i_range))
                    return false;

                // only if see assisted creature
                if (!i_funit->IsWithinLOSInMap(u))
                    return false;

                return true;
            }
        private:
            UnitPtr const i_funit;
            UnitPtr const i_enemy;
            float i_range;
    };

    class NearestAssistCreatureInCreatureRangeCheck
    {
        public:
            NearestAssistCreatureInCreatureRangeCheck(CreaturePtr obj, UnitPtr enemy, float range)
                : i_obj(obj), i_enemy(enemy), i_range(range) {}

            bool operator()(CreaturePtr u)
            {
                if (u == i_obj)
                    return false;
                if (!u->CanAssistTo(i_obj, i_enemy))
                    return false;

                if (!i_obj->IsWithinDistInMap(u, i_range))
                    return false;

                if (!i_obj->IsWithinLOSInMap(u))
                    return false;

                i_range = i_obj->GetDistance(u);            // use found unit range as new range limit for next check
                return true;
            }
            float GetLastRange() const { return i_range; }
        private:
            CreaturePtr const i_obj;
            UnitPtr const i_enemy;
            float  i_range;

            // prevent clone this object
            NearestAssistCreatureInCreatureRangeCheck(NearestAssistCreatureInCreatureRangeCheck const&);
    };

    // Success at unit in range, range update for next check (this can be use with CreatureLastSearcher to find nearest creature)
    class NearestCreatureEntryWithLiveStateInObjectRangeCheck
    {
        public:
            NearestCreatureEntryWithLiveStateInObjectRangeCheck(constWorldObjectPtr& obj, uint32 entry, bool alive, float range)
                : i_obj(obj), i_entry(entry), i_alive(alive), i_range(range) {}

            bool operator()(CreaturePtr u)
            {
                if (u->GetEntry() == i_entry && u->isAlive() == i_alive && i_obj->IsWithinDistInMap(u, i_range))
                {
                    i_range = i_obj->GetDistance(u);         // use found unit range as new range limit for next check
                    return true;
                }
                return false;
            }
            float GetLastRange() const { return i_range; }
        private:
            constWorldObjectPtr& i_obj;
            uint32 i_entry;
            bool   i_alive;
            float  i_range;

            // prevent clone this object
            NearestCreatureEntryWithLiveStateInObjectRangeCheck(NearestCreatureEntryWithLiveStateInObjectRangeCheck const&);
    };

    class AnyPlayerInObjectRangeCheck
    {
        public:
            AnyPlayerInObjectRangeCheck(constWorldObjectPtr obj, float range, bool reqAlive = true) : _obj(obj), _range(range), _reqAlive(reqAlive) {}
            bool operator()(PlayerPtr u)
            {
                if (_reqAlive && !u->isAlive())
                    return false;

                if (!_obj->IsWithinDistInMap(u, _range))
                    return false;

                return true;
            }

        private:
            constWorldObjectPtr _obj;
            float _range;
            bool _reqAlive;
    };

    class NearestPlayerInObjectRangeCheck
    {
        public:
            NearestPlayerInObjectRangeCheck(constWorldObjectPtr obj, float range) : i_obj(obj), i_range(range)
            {
            }

            bool operator()(PlayerPtr u)
            {
                if (u->isAlive() && i_obj->IsWithinDistInMap(u, i_range))
                {
                    i_range = i_obj->GetDistance(u);
                    return true;
                }

                return false;
            }
        private:
            constWorldObjectPtr i_obj;
            float i_range;

            NearestPlayerInObjectRangeCheck(NearestPlayerInObjectRangeCheck const&);
    };

    class AllFriendlyCreaturesInGrid
    {
    public:
        AllFriendlyCreaturesInGrid(constUnitPtr obj) : unit(obj) {}
        bool operator() (UnitPtr u)
        {
            if (u->isAlive() && u->IsVisible() && u->IsFriendlyTo(unit))
                return true;

            return false;
        }
    private:
        constUnitPtr unit;
    };

    class AllGameObjectsWithEntryInRange
    {
    public:
        AllGameObjectsWithEntryInRange(constWorldObjectPtr object, uint32 entry, float maxRange) : m_pObject(object), m_uiEntry(entry), m_fRange(maxRange) {}
        bool operator() (GameObjectPtr go)
        {
            if (go->GetEntry() == m_uiEntry && m_pObject->IsWithinDist(go, m_fRange, false))
                return true;

            return false;
        }
    private:
        constWorldObjectPtr m_pObject;
        uint32 m_uiEntry;
        float m_fRange;
    };

    class AllCreaturesOfEntryInRange
    {
        public:
            AllCreaturesOfEntryInRange(constWorldObjectPtr object, uint32 entry, float maxRange) : m_pObject(object), m_uiEntry(entry), m_fRange(maxRange) {}
            bool operator() (UnitPtr unit)
            {
                if (unit->GetEntry() == m_uiEntry && m_pObject->IsWithinDist(unit, m_fRange, false))
                    return true;

                return false;
            }

        private:
            constWorldObjectPtr m_pObject;
            uint32 m_uiEntry;
            float m_fRange;
    };

    class PlayerAtMinimumRangeAway
    {
    public:
        PlayerAtMinimumRangeAway(constUnitPtr unit, float fMinRange) : unit(unit), fRange(fMinRange) {}
        bool operator() (PlayerPtr player)
        {
            //No threat list check, must be done explicit if expected to be in combat with creature
            if (!player->isGameMaster() && player->isAlive() && !unit->IsWithinDist(player, fRange, false))
                return true;

            return false;
        }

    private:
        constUnitPtr unit;
        float fRange;
    };

    class GameObjectInRangeCheck
    {
    public:
        GameObjectInRangeCheck(float _x, float _y, float _z, float _range, uint32 _entry = 0) :
          x(_x), y(_y), z(_z), range(_range), entry(_entry) {}
        bool operator() (GameObjectPtr go)
        {
            if (!entry || (go->GetGOInfo() && go->GetGOInfo()->entry == entry))
                return go->IsInRange(x, y, z, range);
            else return false;
        }
    private:
        float x, y, z, range;
        uint32 entry;
    };

    class AllWorldObjectsInRange
    {
    public:
        AllWorldObjectsInRange(constWorldObjectPtr object, float maxRange) : m_pObject(object), m_fRange(maxRange) {}
        bool operator() (WorldObjectPtr go)
        {
            return m_pObject->IsWithinDist(go, m_fRange, false) && m_pObject->InSamePhase(go);
        }
    private:
        constWorldObjectPtr m_pObject;
        float m_fRange;
    };

    class ObjectTypeIdCheck
    {
        public:
            ObjectTypeIdCheck(TypeID typeId, bool equals) : _typeId(typeId), _equals(equals) {}
            bool operator()(WorldObjectPtr object)
            {
                return (object->GetTypeId() == _typeId) == _equals;
            }

        private:
            TypeID _typeId;
            bool _equals;
    };

    class ObjectGUIDCheck
    {
        public:
            ObjectGUIDCheck(uint64 GUID) : _GUID(GUID) {}
            bool operator()(WorldObjectPtr object)
            {
                return object->GetGUID() == _GUID;
            }

        private:
            uint64 _GUID;
    };

    class UnitAuraCheck
    {
        public:
            UnitAuraCheck(bool present, uint32 spellId, uint64 casterGUID = 0) : _present(present), _spellId(spellId), _casterGUID(casterGUID) {}

            bool operator()(WorldObjectPtr object) const
            {
                return object->ToUnit() && object->ToUnit()->HasAura(_spellId, _casterGUID) == _present;
            }

        private:
            bool _present;
            uint32 _spellId;
            uint64 _casterGUID;
    };

    // Player checks and do

    // Prepare using Builder localized packets with caching and send to player
    template<class Builder>
    class LocalizedPacketDo
    {
        public:
            explicit LocalizedPacketDo(Builder& builder) : i_builder(builder) {}

            ~LocalizedPacketDo()
            {
                for (size_t i = 0; i < i_data_cache.size(); ++i)
                    delete i_data_cache[i];
            }
            void operator()(PlayerPtr p);

        private:
            Builder& i_builder;
            std::vector<WorldPacket*> i_data_cache;         // 0 = default, i => i-1 locale index
    };

    // Prepare using Builder localized packets with caching and send to player
    template<class Builder>
    class LocalizedPacketListDo
    {
        public:
            typedef std::vector<WorldPacket*> WorldPacketList;
            explicit LocalizedPacketListDo(Builder& builder) : i_builder(builder) {}

            ~LocalizedPacketListDo()
            {
                for (size_t i = 0; i < i_data_cache.size(); ++i)
                    for (size_t j = 0; j < i_data_cache[i].size(); ++j)
                        delete i_data_cache[i][j];
            }
            void operator()(PlayerPtr p);

        private:
            Builder& i_builder;
            std::vector<WorldPacketList> i_data_cache;
                                                            // 0 = default, i => i-1 locale index
    };
}
#endif
