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
#include "terrace_of_endless_spring.h"

enum eSpells
{
    SPELL_AFRAID            = 123181,
    SPELL_SPRAY             = 123121,
    SPELL_GETAWAY           = 123461,
    SPELL_PROTECT           = 123250,
};

enum sSummon
{
    NPC_ANIMATED_PROTECTOR  = 62995,      
};

Position const sumprpos[3] = 
{
    {-990.73f,  -2927.51f, 19.1718f},
    {-1045.78f, -2925.12f, 19.1729f},
    {-1017.72f, -2885.31f, 19.6366f},
};

float const minpullpos = -2978.5349f;
float const maxpullpos = -2851.7839f;

bool CheckTsulong(InstanceScript* instance, Creature* caller)
{
    if (instance && caller)
    {
        if (Creature* ts = caller->GetCreature(*caller, instance->GetData64(NPC_TSULONG)))
        {
            if (ts->isAlive())
                return true;
        }
    }
    return false;
}

class boss_lei_shi : public CreatureScript
{
    public:
        boss_lei_shi() : CreatureScript("boss_lei_shi") {}

        struct boss_lei_shiAI : public BossAI
        {
            boss_lei_shiAI(Creature* creature) : BossAI(creature, DATA_LEI_SHI)
            {
                instance = creature->GetInstanceScript();
                if (instance)
                {
                    if (CheckTsulong(instance, me))
                    {
                        me->SetVisible(false);
                        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    }
                }
            }

            InstanceScript* instance;
            uint32 checkvictim;
            uint8 health;

            void Reset()
            {
                _Reset();
                health = 0;
                me->SetReactState(REACT_DEFENSIVE);
                checkvictim = 0;
            }

            void JustReachedHome()
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                me->SetReactState(REACT_DEFENSIVE);
            }
            
            void EnterCombat(Unit* who)
            {
                if (instance)
                {
                    if (!CheckPullPlayerPos(who))
                    {
                        EnterEvadeMode();
                        return;
                    }
                }
              _EnterCombat();
              checkvictim = 1500;
            }

            bool CheckPullPlayerPos(Unit* who)
            {
                if (!who->ToPlayer() || who->GetPositionY() < minpullpos || who->GetPositionY() > maxpullpos)
                    return false;

                return true;
            }

            void DamageTaken(Unit* attacker, uint32 &damage)
            {
                if (me->HasAura(SPELL_PROTECT))
                    damage = 0;

                if (HealthBelowPct(80) && !health ||
                    HealthBelowPct(60) && health == 1 ||
                    HealthBelowPct(40) && health == 2 ||
                    HealthBelowPct(20) && health == 3)
                {
                    health++;
                    me->AddAura(SPELL_PROTECT, me);
                    for (uint8 n = 0; n < 3 ; n++)
                    {
                        if (Creature* pr = me->SummonCreature(NPC_ANIMATED_PROTECTOR, sumprpos[n]))
                            pr->AI()->DoZoneInCombat(pr, 100.0f);
                    }

                }
            }

            void DoAction(int32 const action)
            {
                if (action == ACTION_REMOVE_PROTECT)
                {
                    if (me->HasAura(SPELL_PROTECT))
                    {
                        me->RemoveAurasDueToSpell(SPELL_PROTECT);
                        summons.DespawnAll();
                    }
                }
            }

            void JustDied(Unit* /*killer*/)
            {
                if (instance)
                {
                    _JustDied();
                    if (Creature* sha = me->GetCreature(*me, instance->GetData64(NPC_SHA_OF_FEAR)))
                    {
                        sha->SetVisible(true);
                        sha->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                    }
                }
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim())
                    return;

                if (checkvictim && instance)
                {
                    if (checkvictim <= diff)
                    {
                        if (me->getVictim())
                        {
                            if (!CheckPullPlayerPos(me->getVictim()))
                            {
                                me->AttackStop();
                                me->SetReactState(REACT_PASSIVE);
                                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_NON_ATTACKABLE);
                                EnterEvadeMode();
                                checkvictim = 0;
                            }
                            else
                                checkvictim = 1500;
                        }
                    }
                    else
                        checkvictim -= diff;
                }

                DoSpellAttackIfReady(SPELL_SPRAY);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_lei_shiAI(creature);
        }
};

void AddSC_boss_lei_shi()
{
    new boss_lei_shi();
}
