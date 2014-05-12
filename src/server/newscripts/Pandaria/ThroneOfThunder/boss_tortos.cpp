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
    SPELL_FURIOS_STONE_BREATH  = 133939,
    SPELL_SNAPPING_BITE        = 135251,
    SPELL_QUAKE_STOMP          = 134920,
    SPELL_CALL_OF_TORTOS       = 136294,
    SPELL_ROCKFALL_P_DMG       = 134539,
};

enum eEvents
{
};

class boss_tortos : public CreatureScript
{
    public:
        boss_tortos() : CreatureScript("boss_tortos") {}

        struct boss_tortosAI : public BossAI
        {
            boss_tortosAI(Creature* creature) : BossAI(creature, DATA_TORTOS)
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
            }

            void JustDied(Unit* /*killer*/)
            {
                _JustDied();
            }

            void UpdateAI(const uint32 diff)
            {
                if (!UpdateVictim() && me->HasUnitState(UNIT_STATE_CASTING))
                    return;

                events.Update(diff);

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new boss_tortosAI(creature);
        }
};

class spell_quake_stomp : public SpellScriptLoader
{
    public:
        spell_quake_stomp() : SpellScriptLoader("spell_quake_stomp") { }

        class spell_quake_stomp_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_quake_stomp_SpellScript);

            void DealDamage()
            {
                if (GetCaster() && GetHitUnit())
                {
                    int32 curdmg = ((GetHitUnit()->GetMaxHealth()/2) + (GetHitUnit()->GetMaxHealth()/10));
                    
                    if (curdmg)
                        SetHitDamage(curdmg);
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_quake_stomp_SpellScript::DealDamage);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_quake_stomp_SpellScript();
        }
};

void AddSC_boss_tortos()
{
    new boss_tortos();
    new spell_quake_stomp();
}
