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

#include "NewScriptPCH.h"
#include "throne_of_thunder.h"

uint32 const megaeraheads[6] =
{
    NPC_FLAMING_HEAD_MELEE,
    NPC_FROZEN_HEAD_MELEE,
    NPC_VENOMOUS_HEAD_MELEE,
    NPC_FLAMING_HEAD_RANGE,
    NPC_VENOMOUS_HEAD_RANGE,
    NPC_FROZEN_HEAD_RANGE,
};

enum eSpells
{
    //Flame Head
    SPELL_IGNITE_FLESH        = 137729, 
    SPELL_CINDERS_DOT         = 139822,
    SPELL_CINDERS_AURA_DMG    = 139835,
    //Frozen Head
    SPELL_ARCTIC_FREEZE       = 139841,
    SPELLTORRENT_OF_ICE_T     = 139857,
    //Venomous Head
    SPELL_ROT_ARMOR           = 139838, 
    //All
    SPELL_MEGAERA_RAGE        = 139758,
};

enum sEvents
{
    //All
    EVENT_BREATH              = 1,
    //Flame Head
    EVENT_CINDERS             = 2,
    EVENT_SPAWN_NEW_HEADS     = 3,
    EVENT_DESPAWN             = 4,
};

enum sActions
{
    //Flame Head
    ACTION_BREATH             = 1,
};

//68065
class npc_megaera : public CreatureScript
{
public:
    npc_megaera() : CreatureScript("npc_megaera") { }

    struct npc_megaeraAI : public ScriptedAI
    {
        npc_megaeraAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            instance = (InstanceScript*)pCreature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NON_ATTACKABLE);
            if (instance->GetBossState(DATA_MEGAERA != NOT_STARTED))
                instance->SetBossState(DATA_MEGAERA, NOT_STARTED);
        }
        InstanceScript* instance;

        void Reset()
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            me->setPowerType(POWER_ENERGY);
            me->SetPower(POWER_ENERGY, 0);
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
            case ACTION_MEGAERA_IN_PROGRESS:
                DoZoneInCombat(me, 150.0f);
                break;
            case ACTION_MEGAERA_RESET:
                if (me->isInCombat())
                    EnterEvadeMode();
                break;
            }
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth())
                damage = 0;
        }

        void EnterCombat(Unit* /*victim*/)
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, me);
        }

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_megaeraAI(pCreature);
    }
};

struct megaera_headAI : public ScriptedAI
{
    megaera_headAI(Creature* creature) : ScriptedAI(creature)
    {
        instance = creature->GetInstanceScript();
    }
    InstanceScript* instance;

    void MegaeraHeadEnterCombat()
    {
        for (uint8 i = 0; i < 6; i++)
            if (Creature* megaerahead = me->GetCreature(*me, instance->GetData64(megaeraheads[i])))
                if (me->GetEntry() != megaerahead->GetEntry())
                    if (megaerahead->isAlive() && !megaerahead->isInCombat())
                        DoZoneInCombat(megaerahead, 150.0f);

        if (instance->GetBossState(DATA_MEGAERA != IN_PROGRESS))
            instance->SetBossState(DATA_MEGAERA, IN_PROGRESS);
    }
};

class npc_megaera_head : public CreatureScript
{
public:
    npc_megaera_head() : CreatureScript("npc_megaera_head") {}

    struct npc_megaera_headAI : public megaera_headAI
    {
        npc_megaera_headAI(Creature* creature) : megaera_headAI(creature), summon(me)
        {
            instance = creature->GetInstanceScript();
            me->SetCanFly(true);
            me->SetDisableGravity(true);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetStandState(UNIT_STAND_STATE_SUBMERGED);
        }
        InstanceScript* instance;
        SummonList summon;
        EventMap events;
        uint32 checkvictim;
        uint32 spawntimer;
        uint32 nextheadentry;
        bool done;

        void Reset()
        {
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
            summon.DespawnAll();
            events.Reset();
            me->setFaction(16);
            me->SetReactState(REACT_DEFENSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            spawntimer = 2000;
            checkvictim = 0;
            nextheadentry = 0;
            done = false;
        }

        void EnterCombat(Unit* who)
        {
            MegaeraHeadEnterCombat();
            switch (me->GetEntry())
            {
            case NPC_FLAMING_HEAD_MELEE:
            case NPC_VENOMOUS_HEAD_MELEE:
            case NPC_FROZEN_HEAD_MELEE:
                events.ScheduleEvent(EVENT_BREATH, 16000);
                checkvictim = 4000;
                break;
            case NPC_FLAMING_HEAD_RANGE:
                events.ScheduleEvent(EVENT_CINDERS, 25000);
                break;
            default:
                break;
                //case NPC_VENOMOUS_HEAD_RANGE:
                //case NPC_FROZEN_HEAD_RANGE:
            }
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth() && !done)
            {
                damage = 0;
                done = true;
                me->RemoveAllAuras();
                me->SetAttackStop(true);
                events.Reset();
                instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
                me->SetStandState(UNIT_STAND_STATE_DEAD);
                instance->SetData(DATA_SEND_LAST_DIED_HEAD, me->GetEntry());
                if (instance->GetData(DATA_CHECK_PROGRESS_MEGAERA))
                    GetAndActiveNextHead();
            }

            if (damage >= me->GetHealth())
                damage = 0;
        }

        void GetAndActiveNextHead()
        {
            if (Creature* nexthead = me->GetCreature(*me, instance->GetData64(DATA_GET_NEXT_HEAD)))
            {
                nexthead->AI()->DoAction(ACTION_UNSUMMON);
                nextheadentry = GetNextMeleeHeadEntry(nexthead->GetEntry());
                if (Creature* megaera = me->GetCreature(*me, instance->GetData64(NPC_MEGAERA)))
                {
                    uint32 modhp = megaera->CountPctFromMaxHealth(14.3f);
                    megaera->SetHealth(megaera->GetHealth() - modhp);
                    events.ScheduleEvent(EVENT_SPAWN_NEW_HEADS, 3000);
                }
            }
        }

        uint32 GetNextMeleeHeadEntry(uint32 megaeraheadentry)
        {
            switch (megaeraheadentry)
            {
            case NPC_FLAMING_HEAD_RANGE:
                return NPC_FLAMING_HEAD_MELEE;
            case NPC_FROZEN_HEAD_RANGE:
                return NPC_FROZEN_HEAD_MELEE;
            case NPC_VENOMOUS_HEAD_RANGE:
                return NPC_VENOMOUS_HEAD_MELEE;
            default:
                return 0;
            }
        }

        void EnterEvadeMode()
        {
            if (instance->GetBossState(DATA_MEGAERA != FAIL))
                instance->SetBossState(DATA_MEGAERA, FAIL);
        }

        void JustSummoned(Creature* summons)
        {
            summon.Summon(summons);
        }

        void DoAction(int32 const action)
        {
            switch (action)
            {
            case ACTION_BREATH:
                events.CancelEvent(EVENT_CINDERS);
                events.ScheduleEvent(EVENT_BREATH, 16000);
                checkvictim = 4000;
                break;
            case ACTION_UNSUMMON:
                events.Reset();
                me->SetStandState(UNIT_STAND_STATE_SUBMERGED);
                events.ScheduleEvent(EVENT_DESPAWN, 2000);
                break;
            case ACTION_MEGAERA_DONE:
                events.Reset();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                me->RemoveAllAuras();
                me->SetStandState(UNIT_STAND_STATE_DEAD);
                events.ScheduleEvent(EVENT_DESPAWN, 3000);
                break;
            }
        }

        void JustDied(Unit* /*killer*/)
        {
            me->RemoveFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);
            instance->SendEncounterUnit(ENCOUNTER_FRAME_DISENGAGE, me);
        }

        void UpdateAI(uint32 diff)
        {
            if (spawntimer)
            {
                if (spawntimer <= diff)
                {
                    spawntimer = 0;
                    me->SetStandState(UNIT_STAND_STATE_STAND);
                    switch (me->GetEntry())
                    {
                    case NPC_FLAMING_HEAD_MELEE:
                    case NPC_VENOMOUS_HEAD_MELEE:
                    case NPC_FROZEN_HEAD_MELEE:
                        me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
                        break;
                    default:
                        break;
                    }
                }
                else
                    spawntimer -= diff;
            }

            if (!UpdateVictim())
                return;

            if (checkvictim)
            {
                if (checkvictim <= diff)
                {
                    if (me->getVictim() && !me->IsWithinMeleeRange(me->getVictim()))
                        DoCast(me->getVictim(), SPELL_MEGAERA_RAGE);
                    checkvictim = 4000;
                }
                else checkvictim -= diff;
            }

            events.Update(diff);

            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch (eventId)
                {
                case EVENT_BREATH:
                {
                    switch (me->GetEntry())
                    {
                    case NPC_FLAMING_HEAD_MELEE:
                        DoCast(me, SPELL_IGNITE_FLESH);
                        break;
                    case NPC_FROZEN_HEAD_MELEE:
                        DoCast(me, SPELL_ARCTIC_FREEZE);
                        break;
                    case NPC_VENOMOUS_HEAD_MELEE:
                        DoCast(me, SPELL_ROT_ARMOR);
                        break;
                    }
                    events.ScheduleEvent(EVENT_BREATH, 16000);
                    break;
                }
                case EVENT_CINDERS:
                    if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 80.0f, true))
                        DoCast(target, SPELL_CINDERS_DOT);
                    events.ScheduleEvent(EVENT_CINDERS, 25000);
                    break;
                case EVENT_DESPAWN:
                    me->DespawnOrUnsummon();
                    break;
                case EVENT_SPAWN_NEW_HEADS:
                    if (Creature* megaera = me->GetCreature(*me, instance->GetData64(NPC_MEGAERA)))
                    {
                        uint8 posmod = 6;
                        if (uint32(me->GetPositionX()) == 6438)      //left
                            posmod = 4;
                        else if (uint32(me->GetPositionX()) == 6419) //right
                            posmod = 5;
                        else if (uint32(me->GetPositionX()) == 6437) //left++
                            posmod = 0;
                        else if (uint32(me->GetPositionX()) == 6394) //right++
                            posmod = 1;

                        if (Creature* mh = megaera->SummonCreature(nextheadentry, megaeraspawnpos[posmod]))
                        {
                            mh->AI()->DoZoneInCombat(mh, 150.0f);
                            instance->SendEncounterUnit(ENCOUNTER_FRAME_ENGAGE, mh);
                        }

                        uint32 newheadsentry = 0;
                        switch (me->GetEntry())
                        {
                        case NPC_FLAMING_HEAD_MELEE:
                            newheadsentry = NPC_FLAMING_HEAD_RANGE;
                            break;
                        case NPC_VENOMOUS_HEAD_MELEE:
                            newheadsentry = NPC_VENOMOUS_HEAD_RANGE;
                            break;
                        case NPC_FROZEN_HEAD_MELEE:
                            newheadsentry = NPC_FROZEN_HEAD_RANGE;
                            break;
                        default:
                            break;
                        }

                        if (!instance->GetData(DATA_GET_COUNT_RANGE_HEADS))
                        {
                            for (uint8 n = 2; n < 4; n++)
                                if (Creature* mh2 = megaera->SummonCreature(newheadsentry, megaeraspawnpos[n]))
                                    mh2->AI()->DoZoneInCombat(mh2, 150.0f);
                        }
                        else
                        {
                            std::list<Creature*>rangemhlist;
                            rangemhlist.clear();
                            GetCreatureListWithEntryInGrid(rangemhlist, me, NPC_FLAMING_HEAD_RANGE, 150.0f);
                            GetCreatureListWithEntryInGrid(rangemhlist, me, NPC_FROZEN_HEAD_RANGE, 150.0f);
                            GetCreatureListWithEntryInGrid(rangemhlist, me, NPC_VENOMOUS_HEAD_RANGE, 150.0f);
                            if (!rangemhlist.empty())
                            {
                                bool blockpos = false;
                                uint8 count = 0;
                                for (uint8 n = 0; n < 8; n++, blockpos = false)
                                {
                                    for (std::list<Creature*>::const_iterator itr = rangemhlist.begin(); itr != rangemhlist.end(); itr++)
                                    {
                                        if (uint32((*itr)->GetPositionX()) == uint32(megaerarangespawnpos[n].GetPositionX()))
                                            blockpos = true;
                                    }

                                    if (!blockpos && count < 2)
                                    {
                                        count++;
                                        if (Creature* mh3 = megaera->SummonCreature(newheadsentry, megaerarangespawnpos[n]))
                                            mh3->AI()->DoZoneInCombat(mh3, 150.0f);
                                    }

                                    if (count == 2)
                                        break;
                                }
                            }
                        }
                    }
                    me->DespawnOrUnsummon();
                    break;
                }
            }
            if (!me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                DoMeleeAttackIfReady();
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_megaera_headAI(creature);
    }
};

//70432
class npc_cinders : public CreatureScript
{
public:
    npc_cinders() : CreatureScript("npc_cinders") { }

    struct npc_cindersAI : public ScriptedAI
    {
        npc_cindersAI(Creature* pCreature) : ScriptedAI(pCreature)
        {
            m_pInstance = (InstanceScript*)pCreature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
        }
        InstanceScript* m_pInstance;
        uint32 despawn;

        void Reset()
        {
            me->SetFloatValue(OBJECT_FIELD_SCALE_X, 0.4f);
            me->AddAura(SPELL_CINDERS_AURA_DMG, me);
            despawn = 60000;
        }

        void EnterCombat(Unit* /*victim*/){}

        void EnterEvadeMode() {}

        void UpdateAI(uint32 diff)
        {
            if (despawn <= diff)
                me->DespawnOrUnsummon();
            else
                despawn -= diff;
        }
    };

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new npc_cindersAI(pCreature);
    }
};

//139822
class spell_cinders : public SpellScriptLoader
{
public:
    spell_cinders() : SpellScriptLoader("spell_cinders") { }

    class spell_cinders_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_cinders_AuraScript);

        void HandleEffectRemove(AuraEffect const * /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (GetCaster() && GetTarget())
                GetCaster()->SummonCreature(NPC_CINDERS, GetTarget()->GetPositionX(), GetTarget()->GetPositionY(), GetTarget()->GetPositionZ());
        }

        void Register()
        {
            OnEffectRemove += AuraEffectRemoveFn(spell_cinders_AuraScript::HandleEffectRemove, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_cinders_AuraScript();
    }
};

void AddSC_boss_megaera()
{
    new npc_megaera();
    new npc_megaera_head();
    new npc_cinders();
    new spell_cinders();
}
