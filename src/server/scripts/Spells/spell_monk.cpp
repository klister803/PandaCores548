/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
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

/*
 * Scripts for spells with SPELLFAMILY_MONK and SPELLFAMILY_GENERIC spells used by monk players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_monk_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum MonkSpells
{
    SPELL_MONK_TIGER_PALM               = 100787,
    SPELL_MONK_TIGER_POWER              = 125359,
    SPELL_MONK_LEGACY_OF_THE_EMPEROR    = 117667,
    SPELL_MONK_FORTIFYING_BREW          = 120954,
    SPELL_MONK_PARALYSIS                = 115078
};

// Paralysis - 115078
class spell_monk_paralysis : public SpellScriptLoader
{
    public:
        spell_monk_paralysis() : SpellScriptLoader("spell_monk_paralysis") { }

        class spell_monk_paralysis_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_paralysis_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (target->isInBack(caster))
                        {
                            AuraApplication* aura = target->GetAuraApplication(115078);
                            int32 duration = aura->GetBase()->GetDuration();
                            duration *= 2;
                            aura->GetBase()->SetDuration(duration, false);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_paralysis_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_paralysis_SpellScript();
        }
};

// Touch of Death - 115080
class spell_monk_touch_of_death : public SpellScriptLoader
{
    public:
        spell_monk_touch_of_death() : SpellScriptLoader("spell_monk_touch_of_death") { }

        class spell_monk_touch_of_death_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_touch_of_death_SpellScript);

            SpellCastResult CheckCast()
            {
                if (GetCaster() && GetCaster()->getVictim())
                {
                    if (GetCaster()->HasAura(124490))
                    {
                        if (GetCaster()->getVictim()->GetTypeId() == TYPEID_UNIT && GetCaster()->getVictim()->ToCreature()->IsDungeonBoss())
                            return SPELL_FAILED_BAD_TARGETS;
                        else if (GetCaster()->getVictim()->GetTypeId() == TYPEID_UNIT && (GetCaster()->getVictim()->GetHealth() > GetCaster()->GetHealth()))
                            return SPELL_FAILED_BAD_TARGETS;
                        else if (GetCaster()->getVictim()->GetTypeId() == TYPEID_PLAYER && (GetCaster()->getVictim()->GetHealthPct() > 10.0f))
                            return SPELL_FAILED_BAD_TARGETS;
                    }
                    else
                    {
                        if (GetCaster()->getVictim()->GetTypeId() == TYPEID_UNIT && GetCaster()->getVictim()->ToCreature()->IsDungeonBoss())
                            return SPELL_FAILED_BAD_TARGETS;
                        else if (GetCaster()->getVictim()->GetTypeId() == TYPEID_UNIT && (GetCaster()->getVictim()->GetHealth() > GetCaster()->GetHealth()))
                            return SPELL_FAILED_BAD_TARGETS;
                    }
                    return SPELL_CAST_OK;
                }
                return SPELL_FAILED_NO_VALID_TARGETS;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_monk_touch_of_death_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_touch_of_death_SpellScript();
        }
};

// Fortifying brew - 115203
class spell_monk_fortifying_brew : public SpellScriptLoader
{
    public:
        spell_monk_fortifying_brew() : SpellScriptLoader("spell_monk_fortifying_brew")
        {
            // Fortifying Brew - 120954
            SpellInfo* spellInfo = (SpellInfo*)sSpellMgr->GetSpellInfo(SPELL_MONK_FORTIFYING_BREW);
            if (spellInfo)
            {
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT;
                spellInfo->Effects[0].BasePoints = 20;
            }
        }

        class spell_monk_fortifying_brew_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_fortifying_brew_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (caster && caster->GetTypeId() == TYPEID_PLAYER)
                    caster->CastSpell(caster, SPELL_MONK_FORTIFYING_BREW, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_monk_fortifying_brew_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_fortifying_brew_SpellScript();
        }
};

// Legacy of the Emperor - 115921
class spell_monk_legacy_of_the_emperor : public SpellScriptLoader
{
    public:
        spell_monk_legacy_of_the_emperor() : SpellScriptLoader("spell_monk_legacy_of_the_emperor") { }

        class spell_monk_legacy_of_the_emperor_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_legacy_of_the_emperor_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (caster && caster->GetTypeId() == TYPEID_PLAYER)
                {
                    caster->CastSpell(caster, SPELL_MONK_LEGACY_OF_THE_EMPEROR, true);

                    std::list<Unit*> memberList;
                    Player* plr = caster->ToPlayer();
                    plr->GetPartyMembers(memberList);

                    for (auto itr : memberList)
                        caster->CastSpell((itr), SPELL_MONK_LEGACY_OF_THE_EMPEROR, true);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_monk_legacy_of_the_emperor_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_legacy_of_the_emperor_SpellScript();
        }
};

// Tiger Palm - 100787
class spell_monk_tiger_palm : public SpellScriptLoader
{
    public:
        spell_monk_tiger_palm() : SpellScriptLoader("spell_monk_tiger_palm") { }

        class spell_monk_tiger_palm_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_tiger_palm_SpellScript);

            void SchoolDmg(SpellEffIndex /*effIndex*/)
            {
                /*Unit* caster = GetCaster();
                if (caster)
                {
                    Unit* target = GetHitUnit();
                    int32 bp = 0;

                    float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);
                    float mwb = caster->GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE);
                    float MWB = caster->GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE);
                    float MWS = caster->GetAttackTime(BASE_ATTACK);
                    float min = 0;
                    float max = 0;

                    min = (3.0f * 1.0f * 1.0f * (0.898882f) * (mwb / MWS) + 1.0f * (mwb / 2.0f / MWS) + (ap / 14.0f) - 1.0f);
                    max = (3.0f * 1.0f * 1.0f * (0.898882f) * (MWB / (MWS + 1.0f * (MWB / 2.0f / MWS) + (ap / 14.0f) + 1.0f)));
                    bp = irand(int32(min), int32(max));

                    caster->CastCustomSpell(target, SPELL_MONK_TIGER_PALM, &bp, NULL, NULL, true);
                }*/
            }

            void Register()
            {
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_tiger_palm_SpellScript();
        }
};

void AddSC_monk_spell_scripts()
{
    new spell_monk_tiger_palm();
    new spell_monk_legacy_of_the_emperor();
    new spell_monk_fortifying_brew();
    new spell_monk_touch_of_death();
    new spell_monk_paralysis();
}