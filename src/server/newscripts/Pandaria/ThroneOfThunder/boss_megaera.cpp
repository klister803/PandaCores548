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

uint32 const megaera_entry[3] = 
{
    NPC_FLAMING_HEAD, 
    NPC_FROZEN_HEAD,
    NPC_VENOMOUS_HEAD,
};

enum eSpells
{
    //Flame Head
    SPELL_IGNITE_FLESH        = 137729, 
    SPELL_CINDERS_DOT         = 139822,
    SPELL_CINDERS_AURA_DMG    = 139835,
    //Frozen Head
    SPELL_ARCTIC_FREEZE       = 139841, 
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
};

enum sActions
{
    //Flame Head
    ACTION_BREATH             = 1,
};

void ResetMegaeraHeads(InstanceScript* instance, Creature* caller, uint32 callerEntry)
{
    if (caller && instance)
    {
        for (uint8 n = 0; n < 3; n++)
        {
            if (Creature* mh = caller->GetCreature(*caller, instance->GetData64(megaera_entry[n])))
            {
                if (mh->GetEntry() != callerEntry)
                {
                    if (!mh->isAlive())
                    {
                        mh->Respawn();
                        mh->GetMotionMaster()->MoveTargetedHome();
                    }
                    else if (mh->isAlive() && mh->isInCombat())
                        mh->AI()->EnterEvadeMode();
                }
            }
        }
    }
}

void CallMegaeraHeads(InstanceScript* instance, Creature* caller, uint32 callerEntry)
{
    if (caller && instance)
    {
        for (uint8 n = 0; n < 3; n++)
        {
            if (Creature* mh = caller->GetCreature(*caller, instance->GetData64(megaera_entry[n])))
            {
                if (mh->GetEntry() != callerEntry)
                {
                    if (mh->isAlive() && !mh->isInCombat())
                        mh->AI()->DoZoneInCombat(mh, 150.0f);
                }
            }
        }
    }
}

void CheckMegaeraHeads(InstanceScript* instance, Creature* caller, uint32 callerEntry)
{
    if (caller && instance)
    {
        uint8 donecount = 0;
        for (uint8 n = 0; n < 3; n++)
        {
            if (Creature* mh = caller->GetCreature(*caller, instance->GetData64(megaera_entry[n])))
            {
                if (!mh->isAlive())
                    donecount++;
                else
                    mh->SetFullHealth();
            }
        }
        caller->RemoveFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE);

        if (donecount == 3) 
        {
            caller->setFaction(35);
            if(!caller->GetMap()->IsLfr())
                caller->SummonGameObject(218805, 6415.06f, 4527.67f, -209.1780f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 604800);
            instance->SetBossState(DATA_MEGAERA, DONE);
            return;
        }
        
        if (callerEntry == NPC_FROZEN_HEAD || callerEntry == NPC_VENOMOUS_HEAD)
        {
            if (Creature* fl_h = caller->GetCreature(*caller, instance->GetData64(NPC_FLAMING_HEAD)))
            {
                if (fl_h->isAlive() && fl_h->isInCombat())
                {
                    if (fl_h->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE))
                    {
                        fl_h->GetMotionMaster()->MoveCharge(caller->GetPositionX(), caller->GetPositionY(), caller->GetPositionZ());
                        fl_h->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                        fl_h->AI()->DoAction(ACTION_BREATH);
                    }
                }
            }
        }
    }
}

class npc_megaera_head : public CreatureScript
{
    public:
        npc_megaera_head() : CreatureScript("npc_megaera_head") {}

        struct npc_megaera_headAI : public ScriptedAI
        {
            npc_megaera_headAI(Creature* creature) : ScriptedAI(creature), summon(me)
            {
                instance = creature->GetInstanceScript();
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            }

            InstanceScript* instance;
            SummonList summon;
            EventMap events;
            uint32 checkvictim; //check distance to victim

            void Reset()
            {
                if (instance)
                {
                    ResetMegaeraHeads(instance, me, me->GetEntry());
                    instance->SetBossState(DATA_MEGAERA, NOT_STARTED);
                }
                summon.DespawnAll();
                events.Reset();
                me->setFaction(16);
                me->SetReactState(REACT_AGGRESSIVE);
                if (me->GetEntry() == NPC_FLAMING_HEAD)
                    me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE);
                checkvictim = 0;
            }

            void EnterCombat(Unit* who)
            {
                if (instance)
                    CallMegaeraHeads(instance, me, me->GetEntry());
                if (me->GetEntry() != NPC_FLAMING_HEAD)
                {
                    events.ScheduleEvent(EVENT_BREATH, 16000);
                    checkvictim = 4000;
                }
                else
                    events.ScheduleEvent(EVENT_CINDERS, 25000);
            }

            void JustSummoned(Creature* summons)
            {
                summon.Summon(summons);
            }

            void DoAction(int32 const action)
            {
                if (action == ACTION_BREATH)
                {
                    events.CancelEvent(EVENT_CINDERS);
                    events.ScheduleEvent(EVENT_BREATH, 16000);
                    checkvictim = 4000;
                }
            }

            void JustDied(Unit* /*killer*/)
            {
                CheckMegaeraHeads(instance, me, me->GetEntry());
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
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

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_BREATH:
                        {
                            switch (me->GetEntry())
                            {
                            case NPC_FLAMING_HEAD:
                                DoCast(me, SPELL_IGNITE_FLESH);
                                break;
                            case NPC_FROZEN_HEAD:
                                DoCast(me, SPELL_ARCTIC_FREEZE);
                                break;
                            case NPC_VENOMOUS_HEAD:
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
                    }
                }
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
    new npc_megaera_head();
    new npc_cinders();
    new spell_cinders();
}
