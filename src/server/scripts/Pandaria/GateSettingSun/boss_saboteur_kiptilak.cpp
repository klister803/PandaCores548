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

/* ScriptData
SDName: Boss_Broggok
SD%Complete: 70
SDComment: pre-event not made
SDCategory: Hellfire Citadel, Blood Furnace
EndScriptData */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "gate_setting_sun.h"

enum eSpells
{
    SPELL_PLANT_EXPLOSIVE               = 107187,

    SPELL_SABOTAGE                      = 107268,
    SPELL_SABOTAGE_EXPLOSION            = 113645,
    
    SPELL_MUNITION_STABLE               = 109987,
    SPELL_MUNITION_EXPLOSION            = 107153,
    SPELL_MUNITION_EXPLOSION_AURA       = 120551,
};

enum eEvents
{
    EVENT_EXPLOSIVES        = 1,
    EVENT_SABOTAGE          = 2
};

enum eWorldInFlames
{
    WIF_NONE    = 0,
    WIF_70      = 1,
    WIF_30      = 2
};

class boss_saboteur_kiptilak : public CreatureScript
{
    public:
        boss_saboteur_kiptilak() : CreatureScript("boss_saboteur_kiptilak") {}

        struct boss_saboteur_kiptilakAI : public BossAI
        {
            boss_saboteur_kiptilakAI(Creature* creature) : BossAI(creature, DATA_KIPTILAK)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            uint8 WorldInFlamesEvents;

            void Reset()
            {
                _Reset();
                
                events.ScheduleEvent(EVENT_EXPLOSIVES, urand(7500,  10000));
                events.ScheduleEvent(EVENT_SABOTAGE,   urand(22500, 30000));

                WorldInFlamesEvents = 0;
            }

            void EnterCombat(Unit* /*who*/)
            {
                _EnterCombat();
            }

            void JustReachedHome()
            {
                instance->SetBossState(DATA_KIPTILAK, FAIL);
                summons.DespawnAll();
            }

            void DamageTaken(Unit* attacker, uint32& damage)
            {
                switch (attacker->GetEntry())
                {
                    case NPC_EXPLOSION_BUNNY_N:
                    case NPC_EXPLOSION_BUNNY_S:
                    case NPC_EXPLOSION_BUNNY_E:
                    case NPC_EXPLOSION_BUNNY_W:
                        damage = 0;
                        return;
                }

                float nextHealthPct = ((float(me->GetHealth()) - damage)  / float(me->GetMaxHealth())) * 100;

                if (WorldInFlamesEvents < WIF_70 && nextHealthPct <= 70.0f)
                {
                    DoWorldInFlamesEvent();
                    ++WorldInFlamesEvents;
                }
                else if (WorldInFlamesEvents < WIF_30 && nextHealthPct <= 30.0f)
                {
                    DoWorldInFlamesEvent();
                    ++WorldInFlamesEvents;
                }
            }

            void DoWorldInFlamesEvent()
            {
                std::list<Creature*> munitionList;
                GetCreatureListWithEntryInGrid(munitionList, me, NPC_STABLE_MUNITION, 100.0f);

                for (auto itr: munitionList)
                {
                    itr->RemoveAurasDueToSpell(SPELL_MUNITION_STABLE);
                    itr->CastSpell(itr, SPELL_MUNITION_EXPLOSION, true);
                    itr->DespawnOrUnsummon(2000);
                }
            }

            void JustSummoned(Creature* summoned)
            {
                if (summoned->GetEntry() == NPC_STABLE_MUNITION)
                    summoned->AddAura(SPELL_MUNITION_STABLE, summoned);

                summons.Summon(summoned);
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                switch(events.ExecuteEvent())
                {
                    case EVENT_EXPLOSIVES:
                        for (uint8 i = 0; i < 3; ++i)
                            me->CastSpell(frand(702, 740), frand(2292, 2320), 388.5f, SPELL_PLANT_EXPLOSIVE, true);

                        events.ScheduleEvent(EVENT_EXPLOSIVES, urand(7500,  10000));
                        break;
                    case EVENT_SABOTAGE:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                            me->CastSpell(me, SPELL_SABOTAGE, true);

                        events.ScheduleEvent(EVENT_SABOTAGE,   urand(22500, 30000));
                        break;
                    default:
                        break;
                }

                DoMeleeAttackIfReady();
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_saboteur_kiptilakAI(creature);
        }
};

class npc_munition_explosion_bunny : public CreatureScript
{
public:
    npc_munition_explosion_bunny() : CreatureScript("npc_munition_explosion_bunny") { }

    struct npc_munition_explosion_bunnyAI : public ScriptedAI
    {
        npc_munition_explosion_bunnyAI(Creature* creature) : ScriptedAI(creature) {}

        float orientation;
        uint32 checkTimer;

        void Reset()
        {
            me->SetReactState(REACT_PASSIVE);
            orientation = 0.0f;
            checkTimer = 500;

            switch (me->GetEntry())
            {
                case NPC_EXPLOSION_BUNNY_N:
                    orientation = 0.0f;
                    break;
                case NPC_EXPLOSION_BUNNY_S:
                    orientation = M_PI;
                    break;
                case NPC_EXPLOSION_BUNNY_E:
                    orientation = 4.71f;
                    break;
                case NPC_EXPLOSION_BUNNY_W:
                    orientation = 1.57f;
                    break;
            }

            float x = 0.0f;
            float y = 0.0f;
            GetPositionWithDistInOrientation(me, 40.0f, orientation, x, y);
            me->GetMotionMaster()->MovePoint(1, x, y, me->GetPositionZ());

            me->AddAura(SPELL_MUNITION_EXPLOSION_AURA, me);
        }

        void DamageTaken(Unit* attacker, uint32& damage)
        {
            damage = 0;
        }

        void MovementInform(uint32 type, uint32 id)
        {
            if (id == 1)
                me->DespawnOrUnsummon();
        }

        void EnterCombat(Unit* /*who*/)
        {
            return;
        }
        
        void UpdateAI(const uint32 diff)
        {
            if (checkTimer <= diff)
            {
                checkTimer = 500;
                if (Creature* munition = GetClosestCreatureWithEntry(me, NPC_STABLE_MUNITION, 2.0f, true))
                {
                    if (munition->HasAura(SPELL_MUNITION_STABLE))
                    {
                        munition->RemoveAurasDueToSpell(SPELL_MUNITION_STABLE);
                        munition->CastSpell(munition, SPELL_MUNITION_EXPLOSION, true);
                        munition->DespawnOrUnsummon(2000);
                    }
                }
            }
            else checkTimer -= diff;
        }
    };

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_munition_explosion_bunnyAI (creature);
    }
};

class spell_kiptilak_sabotage : public SpellScriptLoader
{
    public:
        spell_kiptilak_sabotage() :  SpellScriptLoader("spell_kiptilak_sabotage") { }

        class spell_kiptilak_sabotage_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_kiptilak_sabotage_AuraScript);

            void OnRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();

                if (!target)
                    return;

                target->CastSpell(target, SPELL_MUNITION_EXPLOSION, true);
                target->CastSpell(target, SPELL_SABOTAGE_EXPLOSION, true);
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_kiptilak_sabotage_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_kiptilak_sabotage_AuraScript();
        }
};

void AddSC_boss_saboteur_kiptilak()
{
    new boss_saboteur_kiptilak();
    new npc_munition_explosion_bunny();
    new spell_kiptilak_sabotage();
}
