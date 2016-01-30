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
#include "siege_of_orgrimmar.h"

enum eSpells
{
    //Garrosh
    SPELL_DESECRETE                  = 144748,
    SPELL_EM_DESECRETE               = 144749,
    SPELL_DESECRATED                 = 144762,
    SPELL_EM_DESECRATED              = 144817,
    SPELL_DESECRATED_WEAPON_AT       = 144760,
    SPELL_EM_DESECRATED_WEAPON_AT    = 144818,
    SPELL_DESECRATED_WEAPON_AXE      = 145880,

    //Iron Star
    SPELL_IRON_STAR_IMPACT_AT        = 144645,
    SPELL_IRON_STAR_IMPACT_DMG       = 144650,
    SPELL_EXPLODING_IRON_STAR        = 144798,

    //Engeneer
    SPELL_POWER_IRON_STAR            = 144616, //engeneer entry 71984
};

enum sEvents
{
    //Garrosh
    EVENT_DESECRATED_WEAPON          = 1,
    //Desecrated weapon
    EVENT_REGENERATE                 = 2,
    //Summons
    //Iron Star
    EVENT_ACTIVE                     = 3,
    EVENT_START_MOVING               = 4,
};

enum Phase
{
    PHASE_NULL,
    PHASE_ONE,
    PHASE_TWO,
};

Position ironstarspawnpos[2] =
{
    {1087.05f, -5758.29f, -317.689f, 1.45992f}, //Right
    {1059.92f, -5520.2f, -317.689f, 4.64799f},  //Left
};

Position engeneerspawnpos[2] =
{                                               //From Boss face
    {1061.84f, -5746.28f, -304.4846f, 1.4820f}, //Right
    {1084.89f, -5530.73f, -304.4842f, 4.5215f}, //Left
};

Position ironstardestpos[2] =
{
    {1106.16f, -5555.63f, -317.5304f},
    {0.0f, 0.0f, 0.0f},
};

class boss_garrosh_hellscream : public CreatureScript
{
    public:
        boss_garrosh_hellscream() : CreatureScript("boss_garrosh_hellscream") {}

        struct boss_garrosh_hellscreamAI : public BossAI
        {
            boss_garrosh_hellscreamAI(Creature* creature) : BossAI(creature, DATA_GARROSH)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;
            Phase phase;

            void Reset()
            {
                _Reset();
                me->SetReactState(REACT_PASSIVE);//test only
                phase = PHASE_NULL;
                me->setPowerType(POWER_ENERGY);
                me->SetMaxPower(POWER_ENERGY, 100);
                me->SetPower(POWER_ENERGY, 0);
            }

            void SpawnIronStar()
            {
                me->SummonCreature(NPC_KORKRON_IRON_STAR, ironstarspawnpos[0]);
                /*for (uint8 n = 0; n < 2; n++)
                    me->SummonCreature(NPC_KORKRON_IRON_STAR, ironstarspawnpos[n]);*/
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                SpawnIronStar();
                phase = PHASE_ONE;
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (HealthBelowPct(10) && phase == PHASE_ONE)
                    phase = PHASE_TWO; //enter phase two
            }

            void DoAction(int32 const action)
            {
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }

            bool IfTargetHavePlayersInRange(Player* target, uint8 count)
            {
                count++;
                std::list<Player*>pllist;
                GetPlayerListInGrid(pllist, target, 15.0f);
                if (pllist.size() >= count)
                    return true;
                return false;
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_DESECRATED_WEAPON:
                        uint8 count = Is25ManRaid() ? 7 : 3;
                        bool havetarget = false;
                        std::list<Player*> pllist;
                        pllist.clear();
                        GetPlayerListInGrid(pllist, me, 150.0f);
                        if (!pllist.empty())
                        {
                            for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                            {
                                if ((*itr)->GetRoleForGroup((*itr)->GetSpecializationId((*itr)->GetActiveSpec())) != ROLES_TANK && me->GetExactDist(*itr) >= 15.0f)
                                {
                                    if (IfTargetHavePlayersInRange(*itr, count))
                                    {
                                        havetarget = true;
                                        DoCast(*itr, phase != PHASE_ONE ? SPELL_EM_DESECRETE : SPELL_DESECRETE);
                                        break;
                                    }
                                }
                            }
                            //If no target in range, take melee
                            if (!havetarget)
                            {
                                for (std::list<Player*>::const_iterator itr = pllist.begin(); itr != pllist.end(); ++itr)
                                {
                                    if ((*itr)->GetRoleForGroup((*itr)->GetSpecializationId((*itr)->GetActiveSpec())) != ROLES_TANK)
                                    {
                                        if (IfTargetHavePlayersInRange(*itr, count))
                                        {
                                            DoCast(*itr, phase != PHASE_ONE ? SPELL_EM_DESECRETE : SPELL_DESECRETE);
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        break;
                    }
                }
                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_garrosh_hellscreamAI(creature);
        }
};

class npc_garrosh_soldier : public CreatureScript
{
public:
    npc_garrosh_soldier() : CreatureScript("npc_garrosh_soldier") {}

    struct npc_garrosh_soldierAI : public ScriptedAI
    {
        npc_garrosh_soldierAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
        }

        InstanceScript* instance;

        void Reset()
        {
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_garrosh_soldierAI(creature);
    }
};

//71985
class npc_iron_star : public CreatureScript
{
public:
    npc_iron_star() : CreatureScript("npc_iron_star") {}

    struct npc_iron_starAI : public ScriptedAI
    {
        npc_iron_starAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
        }

        InstanceScript* instance;
        EventMap events;

        void Reset()
        {
            events.ScheduleEvent(EVENT_ACTIVE, 5000);
        }

        void MovementInform(uint32 type, uint32 pointId)
        {
            if (type == EFFECT_MOTION_TYPE && pointId == 1)
            {
                DoCast(me, SPELL_EXPLODING_IRON_STAR, true);
                me->Kill(me, true);
            }
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_ACTIVE)
                {
                    float x, y;
                    DoCast(me, SPELL_IRON_STAR_IMPACT_AT, true);
                    GetPositionWithDistInOrientation(me, 200.0f, me->GetOrientation(), x, y);
                    me->GetMotionMaster()->MoveJump(x, y, -317.4815f, 25.0f, 0.0f, 1);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_iron_starAI(creature);
    }
};

//72154
class npc_desecrated_weapon : public CreatureScript
{
public:
    npc_desecrated_weapon() : CreatureScript("npc_desecrated_weapon") {}

    struct npc_desecrated_weaponAI : public ScriptedAI
    {
        npc_desecrated_weaponAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            lastpct = 90;
        }

        InstanceScript* instance;
        uint8 lastpct;

        void Reset()
        {
            DoCast(me, SPELL_DESECRATED_WEAPON_AXE);
            DoCast(me, SPELL_DESECRATED_WEAPON_AT);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (lastpct && HealthBelowPct(lastpct))
            {
                float scale = float(lastpct)/100;
                if (AreaTrigger* at = me->GetAreaObject(SPELL_DESECRATED_WEAPON_AT))
                    at->SetFloatValue(AREATRIGGER_EXPLICIT_SCALE, scale);
                lastpct = lastpct - 10;
            }
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff){}
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_desecrated_weaponAI(creature);
    }
};

//72198
class npc_empowered_desecrated_weapon : public CreatureScript
{
public:
    npc_empowered_desecrated_weapon() : CreatureScript("npc_empowered_desecrated_weapon") {}

    struct npc_empowered_desecrated_weaponAI : public ScriptedAI
    {
        npc_empowered_desecrated_weaponAI(Creature* creature) : ScriptedAI(creature)
        {
            instance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_DISABLE_MOVE);
            lastpct = 90;
        }

        InstanceScript* instance;
        EventMap events;
        uint32 hppctmod;
        uint8 lastpct;

        void Reset()
        {
            events.Reset();
            hppctmod = me->CountPctFromMaxHealth(10);
            DoCast(me, SPELL_DESECRATED_WEAPON_AXE);
            DoCast(me, SPELL_EM_DESECRATED_WEAPON_AT);
            events.ScheduleEvent(EVENT_REGENERATE, 10000);
        }

        void DamageTaken(Unit* attacker, uint32 &damage)
        {
            if (damage >= me->GetHealth())
                damage = 0;

            if (lastpct && HealthBelowPct(lastpct))
            {
                float scale = float(lastpct) / 100;
                if (AreaTrigger* at = me->GetAreaObject(SPELL_EM_DESECRATED_WEAPON_AT))
                    at->SetFloatValue(AREATRIGGER_EXPLICIT_SCALE, scale);
                lastpct = lastpct - 10;
            }
        }

        void EnterCombat(Unit* who){}

        void EnterEvadeMode(){}

        void UpdateAI(uint32 diff)
        {
            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                if (eventId == EVENT_REGENERATE)
                {
                    if (me->GetHealthPct() < 100)
                    {
                        float scale = 0;
                        if (me->GetHealth() + hppctmod >= me->GetMaxHealth())
                        {
                            me->SetFullHealth();
                            lastpct = 100;
                            scale = 1;
                        }
                        else
                        {
                            me->SetHealth(me->GetHealth() + hppctmod);
                            lastpct = uint8(floor(me->GetHealthPct()));
                            scale = float(lastpct) / 100;
                        }
                        if (AreaTrigger* at = me->GetAreaObject(SPELL_EM_DESECRATED_WEAPON_AT))
                            at->SetFloatValue(AREATRIGGER_EXPLICIT_SCALE, scale);
                    }
                    events.ScheduleEvent(EVENT_REGENERATE, 10000);
                }
            }
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_empowered_desecrated_weaponAI(creature);
    }
};

//144798
class spell_exploding_iron_star : public SpellScriptLoader
{
public:
    spell_exploding_iron_star() : SpellScriptLoader("spell_exploding_iron_star") { }

    class spell_exploding_iron_star_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_exploding_iron_star_SpellScript);

        void DealDamage()
        {
            if (GetCaster() && GetHitUnit())
            {
                float distance = GetCaster()->GetExactDist2d(GetHitUnit());
                if (distance >= 0 && distance < 300)
                    SetHitDamage(GetHitDamage() * (1 - (distance / 300)));
            }
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_exploding_iron_star_SpellScript::DealDamage);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_exploding_iron_star_SpellScript();
    }
};


void AddSC_boss_garrosh_hellscream()
{
    new boss_garrosh_hellscream();
    new npc_garrosh_soldier();
    new npc_iron_star();
    new npc_desecrated_weapon();
    new npc_empowered_desecrated_weapon();
    new spell_exploding_iron_star();
}
