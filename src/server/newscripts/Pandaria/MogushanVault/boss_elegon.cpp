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

//Work only first phase (Sanmay)

#include "NewScriptPCH.h"
#include "mogu_shan_vault.h"

enum eSpells
{   
    //Elegon
    SPELL_TOUCH_OF_THE_TITANS       = 117870,
    SPELL_OVERCHARGED               = 117877,
    SPELL_CELESTIAL_BREATH          = 117960,
    SPELL_ENERGY_TENDROLS           = 127362,
    SPELL_RADIATING_ENERGIES        = 118310,
    SPELL_MATERIALIZE_PROTECTOR     = 117954,

    //Protector
    SPELL_TOTAL_ANNIHILATION        = 117914,
    SPELL_ARCING_ENERGY             = 117945,
    SPELL_STABILITY_FLUX            = 117911,
    SPELL_ECLIPSE                   = 117885,
};

enum eEvents
{
    //Elegon
    EVENT_CHECK_DISTANCE         = 1,
    EVENT_BREATH                 = 2,
    EVENT_PROTECTOR              = 3,

    //Buff controller
    EVENT_CHECK_DIST             = 4,

    //Protector
    EVENT_ARCING_ENERGY          = 5,
};

Position midpos = {4023.13f, 1907.75f, 358.083f, 0.0f};

class boss_elegon : public CreatureScript
{
    public:
        boss_elegon() : CreatureScript("boss_elegon") {}

        struct boss_elegonAI : public BossAI
        {
            boss_elegonAI(Creature* creature) : BossAI(creature, DATA_ELEGON)
            {
                pInstance = creature->GetInstanceScript();
                me->SetCanFly(true);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
                me->SetByteFlag(UNIT_FIELD_BYTES_1, 3, UNIT_BYTE1_FLAG_ALWAYS_STAND | UNIT_BYTE1_FLAG_HOVER);

            }

            InstanceScript* pInstance;

            void Reset()
            {
                _Reset();
                RemoveBuff();
                me->SetReactState(REACT_DEFENSIVE);
            }

            void RemoveBuff()
            {
                if (pInstance)
                {
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_OVERCHARGED);
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(SPELL_TOUCH_OF_THE_TITANS);
                    pInstance->DoRemoveAurasDueToSpellOnPlayers(117878);//Overcharged (trigger aura)
                }
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                me->SummonCreature(65297, me->GetPositionX(), me->GetPositionY(), 358.84118f);//Buff Controller
                events.ScheduleEvent(EVENT_CHECK_DISTANCE, 5000);
                events.ScheduleEvent(EVENT_BREATH, 10000);
                events.ScheduleEvent(EVENT_PROTECTOR, 20000);
            }

            void JustDied(Unit* attacker)
            {
                _JustDied();
                RemoveBuff();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                case EVENT_CHECK_DISTANCE:
                    if (me->getVictim())
                        if (!me->IsWithinMeleeRange(me->getVictim()))
                            DoCast(me->getVictim(), SPELL_ENERGY_TENDROLS);
                    events.ScheduleEvent(EVENT_CHECK_DISTANCE, 5000);
                    break;
                case EVENT_BREATH:
                    DoCast(me, SPELL_CELESTIAL_BREATH);
                    events.ScheduleEvent(EVENT_BREATH, 15000);
                    break;
                case EVENT_PROTECTOR:
                    DoCast(me, SPELL_MATERIALIZE_PROTECTOR);
                    events.ScheduleEvent(EVENT_PROTECTOR, 25000);
                    break;
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_elegonAI(creature);
        }
};

class npc_buff_controller : public CreatureScript
{
    public:
        npc_buff_controller() : CreatureScript("npc_buff_controller") {}

        struct npc_buff_controllerAI : public CreatureAI
        {
            npc_buff_controllerAI(Creature* creature) : CreatureAI(creature)
            {
                instance = creature->GetInstanceScript();
                me->SetReactState(REACT_PASSIVE);
                me->SetDisplayId(11686);
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_DISABLE_MOVE);
            }

            InstanceScript* instance;
            EventMap events;

            void Reset()
            {
                events.ScheduleEvent(EVENT_CHECK_DIST, 1000);
            }

            void CheckDistPlayersToMe()
            {
                if (Map* map = me->GetMap())
                    if (map->IsDungeon())
                    {
                        Map::PlayerList const &players = map->GetPlayers();
                        for (Map::PlayerList::const_iterator i = players.begin(); i != players.end(); ++i)
                        {
                            if (Player* pl = i->getSource())
                            {
                                if (pl->isAlive() && pl->GetDistance(me) <= 38.9f)
                                {
                                    if (!pl->HasAura(SPELL_TOUCH_OF_THE_TITANS))
                                        pl->AddAura(SPELL_TOUCH_OF_THE_TITANS, pl);

                                    if (!pl->HasAura(SPELL_OVERCHARGED))
                                        pl->AddAura(SPELL_OVERCHARGED, pl);
                                }
                                else if (pl->isAlive() && pl->GetDistance(me) >= 38.9f)
                                {
                                    pl->RemoveAurasDueToSpell(SPELL_TOUCH_OF_THE_TITANS);
                                    pl->RemoveAurasDueToSpell(SPELL_OVERCHARGED);
                                    pl->RemoveAurasDueToSpell(117878);//Overcharged (trigger aura)
                                }
                            }
                        }
                        events.ScheduleEvent(EVENT_CHECK_DIST, 1000);
                    }
            }
            
            void EnterEvadeMode(){}

            void EnterCombat(Unit* who){}

            void UpdateAI(const uint32 diff)
            {
                events.Update(diff);

                switch (events.ExecuteEvent())
                {
                case EVENT_CHECK_DIST:
                    CheckDistPlayersToMe();
                    break;
                }
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_buff_controllerAI(creature);
        }
};

class mob_celestial_protector : public CreatureScript
{
    public:
        mob_celestial_protector() : CreatureScript("mob_celestial_protector") { }

        struct mob_celestial_protectorAI : public ScriptedAI
        {
            mob_celestial_protectorAI(Creature* creature) : ScriptedAI(creature){}

            EventMap events;
            bool flux;
            bool annihilation;
            bool eclipse;

            void Reset()
            {
                DoZoneInCombat(me, 100.0f);
                events.Reset();
                events.ScheduleEvent(EVENT_ARCING_ENERGY, 30000);
                flux = false;
                eclipse = false;
                annihilation = false;
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                if (me->HealthBelowPctDamaged(25, damage) && !flux)
                {
                    flux = true;
                    DoCast(me, SPELL_STABILITY_FLUX);
                }

                if (me->GetHealthPct() > 25.0f && me->GetDistance(midpos) > 40 && !eclipse)
                {
                    eclipse = true;
                    DoCast(me, SPELL_ECLIPSE);
                }

                if (me->GetHealth() <= damage && !annihilation)
                {
                    damage = 0;
                    annihilation = true;
                    DoCast(me, SPELL_TOTAL_ANNIHILATION);
                    me->DespawnOrUnsummon(2000);
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;
                
                events.Update(diff);
                
                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ARCING_ENERGY:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 50.0f, true))
                                DoCast(target, SPELL_ARCING_ENERGY);
                            events.ScheduleEvent(EVENT_ARCING_ENERGY, 30000);
                            break;
                        default:
                            break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new mob_celestial_protectorAI(creature);
        }
};

void AddSC_boss_elegon()
{
    new boss_elegon();
    new npc_buff_controller();
    new mob_celestial_protector();
}
