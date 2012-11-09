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
    MASTERY_SPELL_HAND_OF_LIGHT         = 96172,
    MASTERY_SPELL_IGNITE                = 12654,
    MASTERY_SPELL_BLOOD_SHIELD          = 77535
};

// Called by 45470 - Death Strike (Heal)
// 77513 - Mastery : Blood Shield
class spell_mastery_blood_shield : public SpellScriptLoader
{
    public:
        spell_mastery_blood_shield() : SpellScriptLoader("spell_mastery_blood_shield") { }

        class spell_mastery_blood_shield_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mastery_blood_shield_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(45470))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (caster->HasAura(77513))
                        {
                            // Check the Mastery aura while in Blood presence
                            if (caster->HasAura(77513) && caster->HasAura(48263))
                            {
                                float Mastery = caster->GetFloatValue(PLAYER_MASTERY) * 6.25f / 100.0f;

                                int32 bp = int32(GetHitHeal() * Mastery);

                                caster->CastCustomSpell(target, MASTERY_SPELL_BLOOD_SHIELD, &bp, NULL, NULL, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mastery_blood_shield_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mastery_blood_shield_SpellScript();
        }
};

// Called by 133 - Fireball, 44614 - Frostfire Bolt, 108853 - Inferno Blast, 2948 - Scorch and 11366 - Pyroblast
// 12846 - Mastery : Ignite
class spell_mastery_ignite : public SpellScriptLoader
{
    public:
        spell_mastery_ignite() : SpellScriptLoader("spell_mastery_ignite") { }

        class spell_mastery_ignite_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mastery_ignite_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(133) ||
                    !sSpellMgr->GetSpellInfo(44614) ||
                    !sSpellMgr->GetSpellInfo(108853) ||
                    !sSpellMgr->GetSpellInfo(2948) ||
                    !sSpellMgr->GetSpellInfo(11366))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (caster->HasAura(12846))
                        {
                            uint32 procSpellId = GetSpellInfo()->Id ? GetSpellInfo()->Id : 0;
                            if (procSpellId != MASTERY_SPELL_IGNITE)
                            {
                                float value = caster->GetFloatValue(PLAYER_MASTERY);
                                value *= 1.5f;

                                int32 bp = int32(GetHitDamage() * value / 100 / 2);
                                bp += target->GetRemainingPeriodicAmount(caster->GetGUID(), MASTERY_SPELL_IGNITE, SPELL_AURA_PERIODIC_DAMAGE);

                                caster->CastCustomSpell(target, MASTERY_SPELL_IGNITE, &bp, NULL, NULL, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mastery_ignite_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mastery_ignite_SpellScript();
        }
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
                                value *= 1.85f;

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
                                        float Mastery = caster->GetFloatValue(PLAYER_MASTERY) * 2.0f;
                                        int32 bp = int32(GetHitDamage() * 0.75f);

                                        if (roll_chance_f(Mastery))
                                            caster->CastCustomSpell(unitTarget, MASTERY_SPELL_LAVA_BURST, &bp, NULL, NULL, true);

                                        break;
                                    }
                                    case 403:
                                    {
                                        float Mastery = caster->GetFloatValue(PLAYER_MASTERY) * 2.0f;
                                        int32 bp = int32(GetHitDamage() * 0.75f);

                                        if (roll_chance_f(Mastery))
                                            caster->CastCustomSpell(unitTarget, MASTERY_SPELL_LIGHTNING_BOLT, &bp, NULL, NULL, true);

                                        break;
                                    }
                                    case 421:
                                    {
                                        float Mastery = caster->GetFloatValue(PLAYER_MASTERY) * 2.0f;
                                        int32 bp = int32(GetHitDamage() * 0.75f);

                                        if (roll_chance_f(Mastery))
                                            caster->CastCustomSpell(unitTarget, MASTERY_SPELL_CHAIN_LIGHTNING, &bp, NULL, NULL, true);

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
    new spell_mastery_blood_shield();
    new spell_mastery_ignite();
    new spell_mastery_hand_of_light();
    new spell_mastery_elemental_overload();
}