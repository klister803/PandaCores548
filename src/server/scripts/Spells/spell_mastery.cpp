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
    MASTERY_SPELL_HAND_OF_LIGHT         = 96172
};

// Called by 35395 - Crusader Strike, 53595 - Hammer of the Righteous, 24275 - Hammer of Wrath, 85256 - Templar's Verdict and 53385 - Divine Storm
// 76672 - Mastery : Hand of Light
class spell_mastery_hand_of_light : public SpellScriptLoader
{
    public:
        spell_mastery_hand_of_light() : SpellScriptLoader("spell_mastery_hand_of_light") { }

        class spell_mastery_hand_of_light_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mastery_hand_of_light_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(35395) ||
                    !sSpellMgr->GetSpellInfo(53595) ||
                    !sSpellMgr->GetSpellInfo(51505) ||
                    !sSpellMgr->GetSpellInfo(85256) ||
                    !sSpellMgr->GetSpellInfo(53385))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (caster->HasAura(76672))
                        {
                            uint32 procSpellId = GetSpellInfo()->Id ? GetSpellInfo()->Id : 0;
                            if (procSpellId != MASTERY_SPELL_HAND_OF_LIGHT)
                            {
                                float value = caster->GetFloatValue(PLAYER_MASTERY);
                                value *= 2;

                                int32 bp = int32(GetHitDamage() * value / 100);

                                caster->CastCustomSpell(target, MASTERY_SPELL_HAND_OF_LIGHT, &bp, NULL, NULL, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mastery_hand_of_light_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mastery_hand_of_light_SpellScript();
        }
};

// Called by 403 - Lightning Bolt, 421 - Chain Lightning and 51505 - Lava Burst
// 77222 - Mastery : Elemental Overload
class spell_mastery_elemental_overload : public SpellScriptLoader
{
    public:
        spell_mastery_elemental_overload() : SpellScriptLoader("spell_mastery_elemental_overload") { }

        class spell_mastery_elemental_overload_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mastery_elemental_overload_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(403) || !sSpellMgr->GetSpellInfo(421) || !sSpellMgr->GetSpellInfo(51505))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                SpellInfo const* procSpell = GetSpellInfo();

                if (procSpell)
                {
                    if (Unit* caster = GetCaster())
                    {
                        if (Unit* unitTarget = GetHitUnit())
                        {
                            if (caster->HasAura(77222))
                            {
                                // Every Lightning Bolt, Chain Lightning and Lava Burst spells have duplicate vs 75% damage and no cost
                                switch (procSpell->Id)
                                {
                                    case 51505:
                                    {
                                        float value = caster->GetFloatValue(PLAYER_MASTERY);
                                        value *= 2;

                                        if (roll_chance_f(value))
                                            caster->CastSpell(unitTarget, MASTERY_SPELL_LAVA_BURST, true);

                                        break;
                                    }
                                    case 403:
                                    {
                                        float value = caster->GetFloatValue(PLAYER_MASTERY);
                                        value *= 2;

                                        if (roll_chance_f(value))
                                            caster->CastSpell(unitTarget, MASTERY_SPELL_LIGHTNING_BOLT, true);

                                        break;
                                    }
                                    case 421:
                                    {
                                        float value = caster->GetFloatValue(PLAYER_MASTERY);
                                        value *= 2;

                                        if (roll_chance_f(value))
                                            caster->CastSpell(unitTarget, MASTERY_SPELL_CHAIN_LIGHTNING, true);

                                        break;
                                    }
                                    default: break;
                                }
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mastery_elemental_overload_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mastery_elemental_overload_SpellScript();
        }
};

void AddSC_mastery_spell_scripts()
{
    new spell_mastery_hand_of_light();
    new spell_mastery_elemental_overload();
}