/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

#ifndef SCRIPTEDCREATURE_H_
#define SCRIPTEDCREATURE_H_

#include "Creature.h"
#include "CreatureAI.h"
#include "CreatureAIImpl.h"
#include "InstanceScript.h"

#define CAST_PLR(a)     (TO_PLAYER(a))
#define CAST_CRE(a)     (TO_CREATURE(a))
#define CAST_AI(a, b)   (dynamic_cast<a*>(b))

class InstanceScript;

class SummonList : public std::list<uint64>
{
    public:
        explicit SummonList(CreaturePtr creature) : me(creature) {}
        void Summon(CreaturePtr summon) { push_back(summon->GetGUID()); }
        void Despawn(CreaturePtr summon) { remove(summon->GetGUID()); }
        void DespawnEntry(uint32 entry);
        void DespawnAll();

        template <class Predicate> void DoAction(int32 info, Predicate& predicate, uint16 max = 0)
        {
            // We need to use a copy of SummonList here, otherwise original SummonList would be modified
            std::list<uint64> listCopy = *this;
            Trinity::Containers::RandomResizeList<uint64, Predicate>(listCopy, predicate, max);
            for (iterator i = listCopy.begin(); i != listCopy.end(); )
            {
                CreaturePtr summon = Unit::GetCreature(TO_WORLDOBJECT(me), *i++);
                if (summon && summon->IsAIEnabled)
                    summon->AI()->DoAction(info);
            }
        }

        void DoZoneInCombat(uint32 entry = 0);
        void RemoveNotExisting();
        bool HasEntry(uint32 entry);
    private:
        CreaturePtr me;
};

class EntryCheckPredicate
{
    public:
        EntryCheckPredicate(uint32 entry) : _entry(entry) {}
        bool operator()(uint64 guid) { return GUID_ENPART(guid) == _entry; }

    private:
        uint32 _entry;
};

class DummyEntryCheckPredicate
{
    public:
        bool operator()(uint64) { return true; }
};

struct ScriptedAI : public CreatureAI
{
    explicit ScriptedAI(CreaturePtr creature);
    virtual ~ScriptedAI() {}

    // *************
    //CreatureAI Functions
    // *************

    void AttackStartNoMove(UnitPtr target);

    // Called at any Damage from any attacker (before damage apply)
    void DamageTaken(UnitPtr /*attacker*/, uint32& /*damage*/) {}

    //Called at World update tick
    virtual void UpdateAI(uint32 const diff);

    //Called at creature death
    void JustDied(UnitPtr /*killer*/) {}

    //Called at creature killing another unit
    void KilledUnit(UnitPtr /*victim*/) {}

    // Called when the creature summon successfully other creature
    void JustSummoned(CreaturePtr /*summon*/) {}

    // Called when a summoned creature is despawned
    void SummonedCreatureDespawn(CreaturePtr /*summon*/) {}

    // Called when hit by a spell
    void SpellHit(UnitPtr /*caster*/, SpellInfo const* /*spell*/) {}

    // Called when spell hits a target
    void SpellHitTarget(UnitPtr /*target*/, SpellInfo const* /*spell*/) {}

    //Called at waypoint reached or PointMovement end
    void MovementInform(uint32 /*type*/, uint32 /*id*/) {}

    // Called when AI is temporarily replaced or put back when possess is applied or removed
    void OnPossess(bool /*apply*/) {}

    // *************
    // Variables
    // *************

    //Pointer to creature we are manipulating
    CreaturePtr me;

    //For fleeing
    bool IsFleeing;

    // *************
    //Pure virtual functions
    // *************

    //Called at creature reset either by death or evade
    void Reset() {}

    //Called at creature aggro either by MoveInLOS or Attack Start
    void EnterCombat(UnitPtr /*victim*/) {}

    // *************
    //AI Helper Functions
    // *************

    //Start movement toward victim
    void DoStartMovement(UnitPtr target, float distance = 0.0f, float angle = 0.0f);

    //Start no movement on victim
    void DoStartNoMovement(UnitPtr target);

    //Stop attack of current victim
    void DoStopAttack();

    //Cast spell by spell info
    void DoCastSpell(UnitPtr target, SpellInfo const* spellInfo, bool triggered = false);

    //Plays a sound to all nearby players
    void DoPlaySoundToSet(WorldObjectPtr source, uint32 soundId);

    //Drops all threat to 0%. Does not remove players from the threat list
    void DoResetThreat();

    float DoGetThreat(UnitPtr unit);
    void DoModifyThreatPercent(UnitPtr unit, int32 pct);

    void DoTeleportTo(float x, float y, float z, uint32 time = 0);
    void DoTeleportTo(float const pos[4]);

    //Teleports a player without dropping threat (only teleports to same map)
    void DoTeleportPlayer(UnitPtr unit, float x, float y, float z, float o);
    void DoTeleportAll(float x, float y, float z, float o);

    //Returns friendly unit with the most amount of hp missing from max hp
    UnitPtr DoSelectLowestHpFriendly(float range, uint32 minHPDiff = 1);

    //Returns a list of friendly CC'd units within range
    std::list<CreaturePtr> DoFindFriendlyCC(float range);

    //Returns a list of all friendly units missing a specific buff within range
    std::list<CreaturePtr> DoFindFriendlyMissingBuff(float range, uint32 spellId);

    //Return a player with at least minimumRange from me
    PlayerPtr GetPlayerAtMinimumRange(float minRange);

    //Spawns a creature relative to me
    CreaturePtr DoSpawnCreature(uint32 entry, float offsetX, float offsetY, float offsetZ, float angle, uint32 type, uint32 despawntime);

    bool HealthBelowPct(uint32 pct) const { return me->HealthBelowPct(pct); }
    bool HealthAbovePct(uint32 pct) const { return me->HealthAbovePct(pct); }

    //Returns spells that meet the specified criteria from the creatures spell list
    SpellInfo const* SelectSpell(UnitPtr target, uint32 school, uint32 mechanic, SelectTargetType targets, uint32 powerCostMin, uint32 powerCostMax, float rangeMin, float rangeMax, SelectEffect effect);

    void SetEquipmentSlots(bool loadDefault, int32 mainHand = EQUIP_NO_CHANGE, int32 offHand = EQUIP_NO_CHANGE, int32 ranged = EQUIP_NO_CHANGE);

    //Generally used to control if MoveChase() is to be used or not in AttackStart(). Some creatures does not chase victims
    void SetCombatMovement(bool allowMovement);
    bool IsCombatMovementAllowed() const { return _isCombatMovementAllowed; }

    bool EnterEvadeIfOutOfCombatArea(uint32 const diff);

    // return true for heroic mode. i.e.
    //   - for dungeon in mode 10-heroic,
    //   - for raid in mode 10-Heroic
    //   - for raid in mode 25-heroic
    // DO NOT USE to check raid in mode 25-normal.
    bool IsHeroic() const { return _isHeroic; }

    // return the dungeon or raid difficulty
    Difficulty GetDifficulty() const { return _difficulty; }

    // return true for 25 man or 25 man heroic mode
    bool Is25ManRaid() const { return _difficulty & RAID_DIFFICULTY_MASK_25MAN; }

    template<class T> inline
    const T& DUNGEON_MODE(const T& normal5, const T& heroic10) const
    {
        switch (_difficulty)
        {
            case REGULAR_DIFFICULTY:
                return normal5;
            case HEROIC_DIFFICULTY:
                return heroic10;
            default:
                break;
        }

        return heroic10;
    }

    template<class T> inline
    const T& RAID_MODE(const T& normal10, const T& normal25) const
    {
        switch (_difficulty)
        {
            case MAN10_DIFFICULTY:
                return normal10;
            case MAN25_DIFFICULTY:
                return normal25;
            default:
                break;
        }

        return normal25;
    }

    template<class T> inline
    const T& RAID_MODE(const T& normal10, const T& normal25, const T& heroic10, const T& heroic25) const
    {
        switch (_difficulty)
        {
            case MAN10_DIFFICULTY:
                return normal10;
            case MAN25_DIFFICULTY:
                return normal25;
            case MAN10_HEROIC_DIFFICULTY:
                return heroic10;
            case MAN25_HEROIC_DIFFICULTY:
                return heroic25;
            default:
                break;
        }

        return heroic25;
    }

    private:
        Difficulty _difficulty;
        uint32 _evadeCheckCooldown;
        bool _isCombatMovementAllowed;
        bool _isHeroic;
};

struct Scripted_NoMovementAI : public ScriptedAI
{
    Scripted_NoMovementAI(CreaturePtr creature) : ScriptedAI(creature) {}
    virtual ~Scripted_NoMovementAI() {}

    //Called at each attack of me by any victim
    void AttackStart(UnitPtr target);
};

class BossAI : public ScriptedAI
{
    public:
        BossAI(CreaturePtr creature, uint32 bossId);
        virtual ~BossAI() {}

        InstanceScript* const instance;
        BossBoundaryMap const* GetBoundary() const { return _boundary; }

        void JustSummoned(CreaturePtr summon);
        void SummonedCreatureDespawn(CreaturePtr summon);

        virtual void UpdateAI(uint32 const diff);

        // Hook used to execute events scheduled into EventMap without the need
        // to override UpdateAI
        // note: You must re-schedule the event within this method if the event
        // is supposed to run more than once
        virtual void ExecuteEvent(uint32 const /*eventId*/) { }

        void Reset() { _Reset(); }
        void EnterCombat(UnitPtr /*who*/) { _EnterCombat(); }
        void JustDied(UnitPtr /*killer*/) { _JustDied(); }
        void JustReachedHome() { _JustReachedHome(); }

    protected:
        void _Reset();
        void _EnterCombat();
        void _JustDied();
        void _JustReachedHome() { me->setActive(false); }

        bool CheckInRoom()
        {
            if (CheckBoundary(me))
                return true;

            EnterEvadeMode();
            return false;
        }

        bool CheckBoundary(UnitPtr who);
        void TeleportCheaters();

        EventMap events;
        SummonList summons;

    private:
        BossBoundaryMap const* const _boundary;
        uint32 const _bossId;
};

class WorldBossAI : public ScriptedAI
{
    public:
        WorldBossAI(CreaturePtr creature);
        virtual ~WorldBossAI() {}

        void JustSummoned(CreaturePtr summon);
        void SummonedCreatureDespawn(CreaturePtr summon);

        virtual void UpdateAI(uint32 const diff);

        // Hook used to execute events scheduled into EventMap without the need
        // to override UpdateAI
        // note: You must re-schedule the event within this method if the event
        // is supposed to run more than once
        virtual void ExecuteEvent(uint32 const /*eventId*/) { }

        void Reset() { _Reset(); }
        void EnterCombat(UnitPtr /*who*/) { _EnterCombat(); }
        void JustDied(UnitPtr /*killer*/) { _JustDied(); }

    protected:
        void _Reset();
        void _EnterCombat();
        void _JustDied();

        EventMap events;
        SummonList summons;
};

// SD2 grid searchers.
CreaturePtr GetClosestCreatureWithEntry(WorldObjectPtr source, uint32 entry, float maxSearchRange, bool alive = true);
GameObjectPtr GetClosestGameObjectWithEntry(WorldObjectPtr source, uint32 entry, float maxSearchRange);
void GetCreatureListWithEntryInGrid(std::list<CreaturePtr>& list, WorldObjectPtr source, uint32 entry, float maxSearchRange);
void GetGameObjectListWithEntryInGrid(std::list<GameObjectPtr>& list, WorldObjectPtr source, uint32 entry, float maxSearchRange);
void GetPositionWithDistInOrientation(UnitPtr pUnit, float dist, float& x, float& y);

#endif // SCRIPTEDCREATURE_H_
