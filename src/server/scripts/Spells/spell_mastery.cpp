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
 * Scripts for spells with MASTERY.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_mastery_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum MasterySpells
{
    MASTERY_SPELL_LIGHTNING_BOLT        = 45284,
    MASTERY_SPELL_CHAIN_LIGHTNING       = 45297,
    MASTERY_SPELL_LAVA_BURST            = 77451,
    MASTERY_SPELL_ELEMENTAL_BLAST       = 120588,
    MASTERY_SPELL_HAND_OF_LIGHT         = 96172,
    MASTERY_SPELL_BLOOD_SHIELD          = 77535,
    MASTERY_SPELL_COMBO_BREAKER_1       = 118864,
    MASTERY_SPELL_COMBO_BREAKER_2       = 116768,
    MASTERY_SPELL_DISCIPLINE_SHIELD     = 77484,
    SPELL_DK_SCENT_OF_BLOOD             = 50421,
};

// Called by Power Word : Shield - 17, Power Word : Shield (Divine Insight) - 123258, Spirit Shell - 114908, Angelic Bulwark - 114214 and Divine Aegis - 47753
// Mastery : Shield Discipline - 77484
class spell_mastery_shield_discipline : public SpellScriptLoader
{
    public:
        spell_mastery_shield_discipline() : SpellScriptLoader("spell_mastery_shield_discipline") { }

        class spell_mastery_shield_discipline_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mastery_shield_discipline_AuraScript);

            void CalculateAmount(AuraEffect const*, float & amount, bool &)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                AuraEffect const* aurEff = caster->GetAuraEffect(MASTERY_SPELL_DISCIPLINE_SHIELD, EFFECT_0);
                if (!aurEff)
                    return;

                if (caster->HasAura(47515)) // Divine Aegis (Passive)
                {
                    float critChance = caster->ToPlayer()->GetFloatValue(PLAYER_CRIT_PERCENTAGE);
                    if(roll_chance_f(critChance))
                        amount *= 2;
                }
                amount += int32(amount * aurEff->GetAmount() / 100.0f);
            }

            // Glyph of Reflective Shield
            void Absorb(AuraEffect* /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
            {
                Unit* caster = dmgInfo.GetVictim();
                Unit* target = dmgInfo.GetAttacker();

                if (!target || !caster || !caster->HasAura(33202, GetCasterGUID()) || (GetSpellInfo()->Id != 17 && GetSpellInfo()->Id != 123258))
                    return;

                float reflectiveDamage = (dmgInfo.GetDamage() > absorbAmount ? absorbAmount : dmgInfo.GetDamage()) * 0.7f;
                if(reflectiveDamage)
                    caster->CastCustomSpell(target, 33619, &reflectiveDamage, NULL, NULL, true);
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mastery_shield_discipline_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_mastery_shield_discipline_AuraScript::Absorb, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mastery_shield_discipline_AuraScript();
        }
};

void AddSC_mastery_spell_scripts()
{
    new spell_mastery_shield_discipline();
}