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

enum eSpells
{
    SPELL_STATIC_BURST          = 137162,
    SPELL_STATIC_WOUND          = 138349,
    SPELL_BALL_LIGHTNING        = 136620,
    SPELL_LIGHTNING_STORM       = 137313, 
    SPELL_LIGHTNING_BALL_DMG    = 136620,

    //Visual
    SPELL_LIGTNING_BALL_VISUAL  = 136534, //Visual for npc
    SPELL_LIGHTNING_BALL_TARGET = 137422, 
    SPELL_WATER_POOL_VISUAL     = 137277, //Visual water pool
    SPELL_WATER_POOL_SCALE_AURA = 137676,
    SPELL_STATIC_WATER_VISUAL   = 137978, //Visual static water pool
    SPELL_WATER_FONT_VISUAL     = 137340, //Visual water from font's head
};

enum Events
{
    EVENT_LIGHTNING_BALL      = 1,
    EVENT_LIGHTNING_STORM     = 2,
    EVENT_STATIC_BURST        = 3,
};

class boss_jinrokh : public CreatureScript
{
    public:
        boss_jinrokh() : CreatureScript("boss_jinrokh") {}

        struct boss_jinrokhAI : public BossAI
        {
            boss_jinrokhAI(Creature* creature) : BossAI(creature, DATA_JINROKH)
            {
                instance = creature->GetInstanceScript();
            }

            InstanceScript* instance;

            void Reset()
            {
                _Reset();
            }

            void EnterCombat(Unit* who)
            {
                _EnterCombat();
                events.ScheduleEvent(EVENT_STATIC_BURST,    urand(20000, 25000));
                events.ScheduleEvent(EVENT_LIGHTNING_BALL,  urand(10000, 18000)); 
                events.ScheduleEvent(EVENT_LIGHTNING_STORM, urand(80000, 90000));
            }

            void UpdateAI(uint32 diff)
            {
                if (!UpdateVictim() || me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                    case EVENT_STATIC_BURST:
                        if (me->getVictim())
                            DoCast(me->getVictim(), SPELL_STATIC_BURST);
                        events.ScheduleEvent(EVENT_STATIC_BURST, urand(20000, 25000));
                        break;
                    case EVENT_LIGHTNING_BALL:
                        if (Unit* target = SelectTarget(SELECT_TARGET_FARTHEST, 1, 40.0f, true)) 
                        {
                            if (Creature* ball = me->SummonCreature(NPC_LIGHTNING_BALL, me->GetPositionX(), me->GetPositionY(), me->GetPositionZ()))
                                ball->AI()->SetGUID(target->GetGUID(), 0);
                        }
                        events.ScheduleEvent(EVENT_LIGHTNING_BALL,  urand(10000, 18000));
                        break;
                    case EVENT_LIGHTNING_STORM:
                        DoCast(me, SPELL_LIGHTNING_STORM);
                        events.ScheduleEvent(EVENT_LIGHTNING_STORM, urand(80000, 90000));
                        break;
                    }
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
            return new boss_jinrokhAI(creature);
        }
};

class npc_lightning_ball : public CreatureScript
{
public:
    npc_lightning_ball() : CreatureScript("npc_lightning_ball") {}

    struct npc_lightning_ballAI : public ScriptedAI
    {
        npc_lightning_ballAI(Creature* creature) : ScriptedAI(creature)
        {
            InstanceScript* pInstance = creature->GetInstanceScript();
            me->SetReactState(REACT_PASSIVE);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
            me->AddAura(SPELL_LIGTNING_BALL_VISUAL, me);
            explose = false;
        }

        InstanceScript* pInstance;
        uint64 pl_guid;
        uint32 CheckDist;
        bool explose;

        void Reset(){}

        void EnterEvadeMode(){}
        
        void SetGUID(uint64 plguid, int32 id)
        {
            if (Player* pl = me->GetPlayer(*me, plguid))
            {
                if (pl->isAlive())
                {
                    pl_guid = plguid;
                    me->AddAura(SPELL_LIGHTNING_BALL_TARGET, pl);
                    me->GetMotionMaster()->MoveFollow(pl, 0.0f, 0.0f);
                    CheckDist = 1000;
                    return;
                }
            }
            me->DespawnOrUnsummon();
        }
        
        void UpdateAI(uint32 diff)
        {
            if (CheckDist)
            {
                if (CheckDist <= diff)
                {
                    if (Player* pl = me->GetPlayer(*me, pl_guid))
                    {
                        if (pl->isAlive())
                        {
                            me->GetMotionMaster()->MoveFollow(pl, 0.0f, 0.0f);
                            if (me->GetDistance(pl) <= 1.0f && !explose)
                            {
                                explose = true;
                                CheckDist = 0;
                                pl->RemoveAurasDueToSpell(SPELL_LIGHTNING_BALL_TARGET);
                                DoCast(pl, SPELL_LIGHTNING_BALL_DMG); 
                                me->DespawnOrUnsummon();
                            }
                        }
                        else
                            me->DespawnOrUnsummon();
                    }
                }
                else
                    CheckDist -= diff;
            }
        }
    };
    
    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_lightning_ballAI(creature);
    }
};

class spell_static_burst : public SpellScriptLoader
{
    public:
        spell_static_burst() : SpellScriptLoader("spell_static_burst") { }

        class spell_static_burst_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_static_burst_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                {
                    if (GetTarget() && GetCaster())
                        GetCaster()->CastCustomSpell(SPELL_STATIC_WOUND, SPELLVALUE_AURA_STACK, 10, GetTarget());
                }
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_static_burst_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_static_burst_AuraScript();
        }
};

//138389
class spell_static_wound : public SpellScriptLoader
{
public:
    spell_static_wound() : SpellScriptLoader("spell_static_wound") { }

    class spell_static_wound_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_static_wound_SpellScript);

        void _HandleHit()
        {
            if (GetHitUnit())
                if (!GetHitUnit()->HasAura(SPELL_STATIC_WOUND))
                    SetHitDamage(GetHitDamage() / 3);
        }

        void Register()
        {
            OnHit += SpellHitFn(spell_static_wound_SpellScript::_HandleHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_static_wound_SpellScript();
    }
};

void AddSC_boss_jinrokh()
{
    new boss_jinrokh();
    new npc_lightning_ball();
    new spell_static_burst();
    new spell_static_wound();
}
