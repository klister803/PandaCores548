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
 * Scripts for spells with SPELLFAMILY_WARLOCK and SPELLFAMILY_GENERIC spells used by warlock players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_warl_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum WarlockSpells
{
    WARLOCK_DEMONIC_EMPOWERMENT_SUCCUBUS    = 54435,
    WARLOCK_DEMONIC_EMPOWERMENT_VOIDWALKER  = 54443,
    WARLOCK_DEMONIC_EMPOWERMENT_FELGUARD    = 54508,
    WARLOCK_DEMONIC_EMPOWERMENT_FELHUNTER   = 54509,
    WARLOCK_DEMONIC_EMPOWERMENT_IMP         = 54444,
    WARLOCK_DEMONIC_CIRCLE_SUMMON           = 48018,
    WARLOCK_DEMONIC_CIRCLE_TELEPORT         = 48020,
    WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST       = 62388,
    WARLOCK_UNSTABLE_AFFLICTION_DISPEL      = 31117,
    WARLOCK_GLYPH_OF_FEAR                   = 56244,
    WARLOCK_FEAR_EFFECT                     = 118699,
    WARLOCK_GLYPH_OF_FEAR_EFFECT            = 130616,
    WARLOCK_CREATE_HEALTHSTONE              = 23517,
    WARLOCK_HARVEST_LIFE_HEAL               = 125314,
    WARLOCK_DRAIN_LIFE_HEAL                 = 89653,
    WARLOCK_SOULBURN_AURA                   = 74434,
    WARLOCK_CORRUPTION                      = 172,
    WARLOCK_AGONY                           = 980,
    WARLOCK_DOOM                            = 124913,
    WARLOCK_UNSTABLE_AFFLICTION             = 30108,
    WARLOCK_IMMOLATE                        = 348,
    WARLOCK_SHADOWBURN_ENERGIZE             = 125882,
    WARLOCK_CONFLAGRATE                     = 17962,
    WARLOCK_CONFLAGRATE_FIRE_AND_BRIMSTONE  = 108685,
    WARLOCK_IMMOLATE_FIRE_AND_BRIMSTONE     = 108686,
    WARLOCK_FIRE_AND_BRIMSTONE              = 108683,
    WARLOCK_BACKDRAFT                       = 117828,
    WARLOCK_PYROCLASM                       = 123686,
    WARLOCK_RAIN_OF_FIRE                    = 104232,
    WARLOCK_RAIN_OF_FIRE_TRIGGERED          = 42223,
    WARLOCK_SPAWN_PURPLE_DEMONIC_GATEWAY    = 113890,
    WARLOCK_DEMONIC_GATEWAY_TELEPORT_GREEN  = 113896,
    WARLOCK_DEMONIC_GATEWAY_TELEPORT_PURPLE = 120729,
    WARLOCK_DEMONIC_GATEWAY_PERIODIC_CHARGE = 113901,
    WARLOCK_NIGHTFALL                       = 108558,
    WARLOCK_SOUL_SWAP_AURA                  = 86211,
    WARLOCK_SOUL_SWAP_VISUAL                = 92795,
    WARLOCK_GRIMOIRE_OF_SACRIFICE           = 108503,
    WARLOCK_METAMORPHOSIS                   = 103958,
    WARLOCK_DEMONIC_LEAP_JUMP               = 54785,
    WARLOCK_ITEM_S12_TIER_4                 = 131632,
    WARLOCK_TWILIGHT_WARD_S12               = 131623,
    WARLOCK_TWILIGHT_WARD_METAMORPHOSIS_S12 = 131624,
    WARLOCK_SHADOWFLAME                     = 47960,
    WARLOCK_SOUL_LEECH_HEAL                 = 108366,
    WARLOCK_DARK_REGENERATION               = 108359,
    WARLOCK_DARK_BARGAIN_DOT                = 110914,
};

// Dark Bargain - 110013
class spell_warl_dark_bargain_on_absorb : public SpellScriptLoader
{
    public:
        spell_warl_dark_bargain_on_absorb() : SpellScriptLoader("spell_warl_dark_bargain_on_absorb") { }

        class spell_warl_dark_bargain_on_absorb_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_dark_bargain_on_absorb_AuraScript);

            uint32 totalAbsorbAmount;

            bool Load()
            {
                totalAbsorbAmount = 0;
                return true;
            }

            void CalculateAmount(constAuraEffectPtr /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                amount = int32(100000000);
            }

            void OnAbsorb(AuraEffectPtr aurEff, DamageInfo& dmgInfo, uint32& absorbAmount)
            {
                totalAbsorbAmount += dmgInfo.GetDamage();
            }

            void OnRemove(constAuraEffectPtr aurEff, AuraEffectHandleModes /*mode*/)
            {
                // (totalAbsorbAmount / 16) it's for totalAbsorbAmount 50% & totalAbsorbAmount / 8 (for each tick of custom spell)
                if (Unit* caster = GetCaster())
                    caster->CastCustomSpell(WARLOCK_DARK_BARGAIN_DOT, SPELLVALUE_BASE_POINT0, (totalAbsorbAmount / 16) , caster, true);
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_dark_bargain_on_absorb_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_warl_dark_bargain_on_absorb_AuraScript::OnAbsorb, EFFECT_0);
                AfterEffectRemove += AuraEffectRemoveFn(spell_warl_dark_bargain_on_absorb_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_dark_bargain_on_absorb_AuraScript();
        }
};

// Dark Regeneration - 108359
class spell_warl_dark_regeneration : public SpellScriptLoader
{
    public:
        spell_warl_dark_regeneration() : SpellScriptLoader("spell_warl_dark_regeneration") { }

        class spell_warl_dark_regeneration_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_dark_regeneration_AuraScript);

            void HandleApply(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes mode)
            {
                if (GetTarget())
                    if (Guardian* pet = GetTarget()->GetGuardianPet())
                        pet->CastSpell(pet, WARLOCK_DARK_REGENERATION, true);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_warl_dark_regeneration_AuraScript::HandleApply, EFFECT_0, SPELL_AURA_OBS_MOD_HEALTH, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_dark_regeneration_AuraScript();
        }
};

// Called by Incinerate - 29722 and Chaos Bolt - 116858
// Soul Leech - 108370
class spell_warl_soul_leech : public SpellScriptLoader
{
    public:
        spell_warl_soul_leech() : SpellScriptLoader("spell_warl_soul_leech") { }

        class spell_warl_soul_leech_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_soul_leech_SpellScript);

            void HandleAfterHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        int32 bp = int32(GetHitDamage() / 10);

                        _player->CastCustomSpell(_player, WARLOCK_SOUL_LEECH_HEAL, &bp, NULL, NULL, true);

                        if (Guardian* pet = _player->GetGuardianPet())
                            _player->CastCustomSpell(pet, WARLOCK_SOUL_LEECH_HEAL, &bp, NULL, NULL, true);
                    }
                }
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_warl_soul_leech_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_soul_leech_SpellScript();
        }
};

// Sacrificial Pact - 108416
class spell_warl_sacrificial_pact : public SpellScriptLoader
{
    public:
        spell_warl_sacrificial_pact() : SpellScriptLoader("spell_warl_sacrificial_pact") { }

        class spell_warl_sacrificial_pact_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_sacrificial_pact_AuraScript);

            void CalculateAmount(constAuraEffectPtr , int32 & amount, bool & )
            {
                if (Unit* caster = GetCaster())
                {
                    if(!GetCaster()->GetGuardianPet())
                        amount = int32(GetCaster()->GetHealth() / 4);
                    else if(GetCaster()->GetGuardianPet())
                        amount = int32(GetCaster()->GetGuardianPet()->GetHealth() / 4);
                }
            }
            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_sacrificial_pact_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_sacrificial_pact_AuraScript();
        }
};

// Hand of Gul'Dan - 86040
class spell_warl_hand_of_guldan : public SpellScriptLoader
{
    public:
        spell_warl_hand_of_guldan() : SpellScriptLoader("spell_warl_hand_of_guldan") { }

        class spell_warl_hand_of_guldan_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_hand_of_guldan_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        _player->CastSpell(target, WARLOCK_SHADOWFLAME, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_hand_of_guldan_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_hand_of_guldan_SpellScript();
        }
};

// Twilight Ward - 6229 and Twilight Ward (Metamorphosis) - 104048
class spell_warl_twilight_ward_s12 : public SpellScriptLoader
{
    public:
        spell_warl_twilight_ward_s12() : SpellScriptLoader("spell_warl_twilight_ward_s12") { }

        class spell_warl_twilight_ward_s12_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_twilight_ward_s12_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->HasAura(WARLOCK_ITEM_S12_TIER_4))
                    {
                        if (GetSpellInfo()->Id == 6229)
                        {
                            if (_player->HasAura(GetSpellInfo()->Id))
                                _player->RemoveAura(GetSpellInfo()->Id);

                            _player->CastSpell(_player, WARLOCK_TWILIGHT_WARD_S12, true);
                        }
                        else if (GetSpellInfo()->Id == 104048)
                        {
                            if (_player->HasAura(GetSpellInfo()->Id))
                                _player->RemoveAura(GetSpellInfo()->Id);

                            _player->CastSpell(_player, WARLOCK_TWILIGHT_WARD_METAMORPHOSIS_S12, true);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_twilight_ward_s12_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_twilight_ward_s12_SpellScript();
        }
};

// Hellfire - 5857
class spell_warl_hellfire : public SpellScriptLoader
{
    public:
        spell_warl_hellfire() : SpellScriptLoader("spell_warl_hellfire") { }

        class spell_warl_hellfire_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_hellfire_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        _player->EnergizeBySpell(_player, GetSpellInfo()->Id, 3, POWER_DEMONIC_FURY);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_hellfire_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_hellfire_SpellScript();
        }
};

// Demonic Leap - 109151
class spell_warl_demonic_leap : public SpellScriptLoader
{
    public:
        spell_warl_demonic_leap() : SpellScriptLoader("spell_warl_demonic_leap") { }

        class spell_warl_demonic_leap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_demonic_leap_SpellScript);

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    _player->CastSpell(_player, WARLOCK_METAMORPHOSIS, true);
                    _player->CastSpell(_player, WARLOCK_DEMONIC_LEAP_JUMP, true);
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_warl_demonic_leap_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_demonic_leap_SpellScript();
        }
};

// Called by Summon Felhunter - 691, Summon Succubus - 712, Summon Voidwalker - 697, Summon Imp - 688
// Summon Infernal - 1122, Summon Doomguard - 18540 and Summon Felguard - 30146
// Grimoire of Sacrifice - 108503
class spell_warl_grimoire_of_sacrifice : public SpellScriptLoader
{
    public:
        spell_warl_grimoire_of_sacrifice() : SpellScriptLoader("spell_warl_grimoire_of_sacrifice") { }

        class spell_warl_grimoire_of_sacrifice_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_grimoire_of_sacrifice_SpellScript);

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (_player->HasAura(WARLOCK_GRIMOIRE_OF_SACRIFICE))
                        _player->RemoveAura(WARLOCK_GRIMOIRE_OF_SACRIFICE);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_warl_grimoire_of_sacrifice_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_grimoire_of_sacrifice_SpellScript();
        }
};

// Burning Rush - 111400
class spell_warl_burning_rush : public SpellScriptLoader
{
    public:
        spell_warl_burning_rush() : SpellScriptLoader("spell_warl_burning_rush") { }

        class spell_warl_burning_rush_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_burning_rush_AuraScript);

            void OnTick(constAuraEffectPtr aurEff)
            {
                if (GetCaster())
                {
                    // Drain 4% of health every second
                    int32 basepoints = GetCaster()->CountPctFromMaxHealth(4);

                    GetCaster()->DealDamage(GetCaster(), basepoints, NULL, NODAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_burning_rush_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_burning_rush_AuraScript();
        }
};

// Soul Swap : Soulburn - 119678
class spell_warl_soul_swap_soulburn : public SpellScriptLoader
{
    public:
        spell_warl_soul_swap_soulburn() : SpellScriptLoader("spell_warl_soul_swap_soulburn") { }

        class spell_warl_soul_swap_soulburn_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_soul_swap_soulburn_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        // Apply instantly corruption, unstable affliction and agony on the target
                        _player->CastSpell(target, WARLOCK_CORRUPTION, true);
                        _player->CastSpell(target, WARLOCK_UNSTABLE_AFFLICTION, true);
                        _player->CastSpell(target, WARLOCK_AGONY, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_soul_swap_soulburn_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_soul_swap_soulburn_SpellScript();
        }
};

// Soul Swap - 86121 or Soul Swap : Exhale - 86213
class spell_warl_soul_swap : public SpellScriptLoader
{
    public:
        spell_warl_soul_swap() : SpellScriptLoader("spell_warl_soul_swap") { }

        class spell_warl_soul_swap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_soul_swap_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (GetSpellInfo()->Id == 86121)
                        {
                            // Soul Swap override spell
                            caster->CastSpell(caster, WARLOCK_SOUL_SWAP_AURA, true);
                            caster->RemoveSoulSwapDOT(target);
                        }
                        else if (GetSpellInfo()->Id == 86123)
                        {
                            caster->CastSpell(target, WARLOCK_SOUL_SWAP_VISUAL, true);
                            caster->ApplySoulSwapDOT(target);

                            if (caster->HasAura(56226) && caster->ToPlayer()) // Glyph of Soul Swap
                                caster->ToPlayer()->AddSpellCooldown(86121, 0, time(NULL) + 30);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_soul_swap_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_soul_swap_SpellScript();
        }
};

// Called by Corruption - 172
// Nightfall - 108558
class spell_warl_nightfall : public SpellScriptLoader
{
    public:
        spell_warl_nightfall() : SpellScriptLoader("spell_warl_nightfall") { }

        class spell_warl_nightfall_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_nightfall_AuraScript);

            void OnTick(constAuraEffectPtr aurEff)
            {
                if (GetCaster())
                    if (Player* _player = GetCaster()->ToPlayer())
                        if (_player->HasAura(WARLOCK_NIGHTFALL))
                            if (roll_chance_i(5))
                                _player->SetPower(POWER_SOUL_SHARDS, _player->GetPower(POWER_SOUL_SHARDS) + 100);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_nightfall_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_nightfall_AuraScript();
        }
};

// Drain Soul - 1120
class spell_warl_drain_soul : public SpellScriptLoader
{
    public:
        spell_warl_drain_soul() : SpellScriptLoader("spell_warl_drain_soul") { }

        class spell_warl_drain_soul_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_drain_soul_AuraScript);

            void HandleRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes mode)
            {
                if (GetCaster())
                {
                    AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                    if (removeMode == AURA_REMOVE_BY_DEATH)
                        GetCaster()->SetPower(POWER_SOUL_SHARDS, GetCaster()->GetPower(POWER_SOUL_SHARDS) + 300);
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectApplyFn(spell_warl_drain_soul_AuraScript::HandleRemove, EFFECT_4, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_drain_soul_AuraScript();
        }
};

// Demonic Gateway (periodic add charge) - 113901
class spell_warl_demonic_gateway_charges : public SpellScriptLoader
{
    public:
        spell_warl_demonic_gateway_charges() : SpellScriptLoader("spell_warl_demonic_gateway_charges") { }

        class spell_warl_demonic_gateway_charges_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_demonic_gateway_charges_AuraScript);

            void OnTick(constAuraEffectPtr aurEff)
            {
                if (Unit* target = GetTarget())
                    if (AuraPtr demonicGateway = target->GetAura(WARLOCK_DEMONIC_GATEWAY_PERIODIC_CHARGE))
                        demonicGateway->ModCharges(1);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_demonic_gateway_charges_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_demonic_gateway_charges_AuraScript();
        }
};

// Demonic Gateway - 111771
class spell_warl_demonic_gateway : public SpellScriptLoader
{
    public:
        spell_warl_demonic_gateway() : SpellScriptLoader("spell_warl_demonic_gateway") { }

        class spell_warl_demonic_gateway_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_demonic_gateway_SpellScript);

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    _player->CastSpell(_player, WARLOCK_SPAWN_PURPLE_DEMONIC_GATEWAY, true);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_warl_demonic_gateway_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_demonic_gateway_SpellScript();
        }
};

// Rain of Fire - 104232
class spell_warl_rain_of_fire : public SpellScriptLoader
{
    public:
        spell_warl_rain_of_fire() : SpellScriptLoader("spell_warl_rain_of_fire") { }

        class spell_warl_rain_of_fire_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_rain_of_fire_AuraScript);

            void OnTick(constAuraEffectPtr aurEff)
            {
                if (DynamicObject* dynObj = GetCaster()->GetDynObject(WARLOCK_RAIN_OF_FIRE))
                    GetCaster()->CastSpell(dynObj->GetPositionX(), dynObj->GetPositionY(), dynObj->GetPositionZ(), WARLOCK_RAIN_OF_FIRE_TRIGGERED, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_rain_of_fire_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_rain_of_fire_AuraScript();
        }
};

// Chaos Bolt - 116858
class spell_warl_chaos_bolt : public SpellScriptLoader
{
    public:
        spell_warl_chaos_bolt() : SpellScriptLoader("spell_warl_chaos_bolt") { }

        class spell_warl_chaos_bolt_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_chaos_bolt_SpellScript);

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (_player->HasAura(WARLOCK_PYROCLASM))
                        if(AuraPtr backdraft = _player->GetAura(WARLOCK_BACKDRAFT))
                            backdraft->SetCharges(backdraft->GetCharges() - 3);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_warl_chaos_bolt_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_chaos_bolt_SpellScript();
        }
};

// Ember Tap - 114635
class spell_warl_ember_tap : public SpellScriptLoader
{
    public:
        spell_warl_ember_tap() : SpellScriptLoader("spell_warl_ember_tap") { }

        class spell_warl_ember_tap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_ember_tap_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        int32 healAmount;
                        float pct;
                        float Mastery;

                        Mastery = 3.0f * _player->GetFloatValue(PLAYER_MASTERY) / 100.0f;

                        pct = 0.15f * (1 + Mastery);

                        healAmount = int32(_player->GetMaxHealth() * pct);

                        SetHitHeal(healAmount);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_ember_tap_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_ember_tap_SpellScript();
        }
};

// Called By : Incinerate (Fire and Brimstone) - 114654, Conflagrate (Fire and Brimstone) - 108685
// Curse of the Elements (Fire and Brimstone) - 104225, Curse of Enfeeblement (Fire and Brimstone) - 109468
// Immolate (Fire and Brimstone) - 108686
// Fire and Brimstone - 108683
class spell_warl_fire_and_brimstone : public SpellScriptLoader
{
    public:
        spell_warl_fire_and_brimstone() : SpellScriptLoader("spell_warl_fire_and_brimstone") { }

        class spell_warl_fire_and_brimstone_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_fire_and_brimstone_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (_player->HasAura(WARLOCK_FIRE_AND_BRIMSTONE))
                            _player->RemoveAura(WARLOCK_FIRE_AND_BRIMSTONE);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_fire_and_brimstone_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_fire_and_brimstone_SpellScript();
        }
};

// Conflagrate - 17962 and Conflagrate (Fire and Brimstone) - 108685
class spell_warl_conflagrate_aura : public SpellScriptLoader
{
    public:
        spell_warl_conflagrate_aura() : SpellScriptLoader("spell_warl_conflagrate_aura") { }

        class spell_warl_conflagrate_aura_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_conflagrate_aura_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (!target->HasAura(WARLOCK_IMMOLATE))
                            if (AuraPtr conflagrate = target->GetAura(WARLOCK_CONFLAGRATE))
                                target->RemoveAura(WARLOCK_CONFLAGRATE);
                        if (!target->HasAura(WARLOCK_IMMOLATE_FIRE_AND_BRIMSTONE))
                            if (AuraPtr conflagrate = target->GetAura(WARLOCK_CONFLAGRATE_FIRE_AND_BRIMSTONE))
                                target->RemoveAura(WARLOCK_CONFLAGRATE_FIRE_AND_BRIMSTONE);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_conflagrate_aura_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_conflagrate_aura_SpellScript();
        }
};

// Shadowburn - 29341
class spell_warl_shadowburn : public SpellScriptLoader
{
    public:
        spell_warl_shadowburn() : SpellScriptLoader("spell_warl_shadowburn") { }

        class spell_warl_shadowburn_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_shadowburn_AuraScript);

            void HandleRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes mode)
            {
                if (GetCaster())
                {
                    AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                    if (removeMode == AURA_REMOVE_BY_DEATH)
                        GetCaster()->SetPower(POWER_BURNING_EMBERS, GetCaster()->GetPower(POWER_BURNING_EMBERS) + 20); // Give 2 Burning Embers
                    else if (removeMode == AURA_REMOVE_BY_EXPIRE)
                        GetCaster()->CastSpell(GetCaster(), WARLOCK_SHADOWBURN_ENERGIZE, true);
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectApplyFn(spell_warl_shadowburn_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_shadowburn_AuraScript();
        }
};

// Called By : Incinerate - 29722 and Incinerate (Fire and Brimstone) - 114654
// Conflagrate - 17962 and Conflagrate (Fire and Brimstone) - 108685
// Burning Embers generate
class spell_warl_burning_embers : public SpellScriptLoader
{
    public:
        spell_warl_burning_embers() : SpellScriptLoader("spell_warl_burning_embers") { }

        class spell_warl_burning_embers_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_burning_embers_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (GetSpell()->IsCritForTarget(target))
                            _player->SetPower(POWER_BURNING_EMBERS, _player->GetPower(POWER_BURNING_EMBERS) + 2);
                        else
                            _player->SetPower(POWER_BURNING_EMBERS, _player->GetPower(POWER_BURNING_EMBERS) + 1);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_burning_embers_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_burning_embers_SpellScript();
        }
};

// Fel Flame - 77799
class spell_warl_fel_flame : public SpellScriptLoader
{
    public:
        spell_warl_fel_flame() : SpellScriptLoader("spell_warl_fel_flame") { }

        class spell_warl_fel_flame_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_fel_flame_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        // Increases the duration of Corruption and Unstable Affliction by 6s
                        if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_WARLOCK_AFFLICTION)
                        {
                            if (AuraPtr unstableAffliction = target->GetAura(WARLOCK_UNSTABLE_AFFLICTION))
                            {
                                unstableAffliction->SetDuration(unstableAffliction->GetDuration() + 6000);
                                unstableAffliction->SetNeedClientUpdateForTargets();
                            }
                            if (AuraPtr corruption = target->GetAura(WARLOCK_CORRUPTION))
                            {
                                corruption->SetDuration(corruption->GetDuration() + 6000);
                                corruption->SetNeedClientUpdateForTargets();
                            }
                        }
                        // Increases the duration of Corruption by 6s
                        else if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_WARLOCK_DEMONOLOGY)
                        {
                            if (AuraPtr corruption = target->GetAura(WARLOCK_CORRUPTION))
                            {
                                corruption->SetDuration(corruption->GetDuration() + 6000);
                                corruption->SetNeedClientUpdateForTargets();
                            }
                            else if (AuraPtr doom = target->GetAura(WARLOCK_DOOM))
                            {
                                doom->SetDuration(doom->GetDuration() + 6000);
                                doom->SetNeedClientUpdateForTargets();
                            }
                        }
                        // Increases the duration of Immolate by 6s
                        else if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_WARLOCK_DESTRUCTION)
                        {
                            if (AuraPtr corruption = target->GetAura(WARLOCK_IMMOLATE))
                            {
                                corruption->SetDuration(corruption->GetDuration() + 6000);
                                corruption->SetNeedClientUpdateForTargets();
                            }

                            if (GetSpell()->IsCritForTarget(target))
                                _player->SetPower(POWER_BURNING_EMBERS, _player->GetPower(POWER_BURNING_EMBERS) + 2);
                            else
                                _player->SetPower(POWER_BURNING_EMBERS, _player->GetPower(POWER_BURNING_EMBERS) + 1);
                        }
                        // Increases the duration of Corruption by 6s
                        else
                        {
                            if (AuraPtr corruption = target->GetAura(WARLOCK_CORRUPTION))
                            {
                                corruption->SetDuration(corruption->GetDuration() + 6000);
                                corruption->SetNeedClientUpdateForTargets();
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_fel_flame_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_fel_flame_SpellScript();
        }
};

// Drain Life - 689
class spell_warl_drain_life : public SpellScriptLoader
{
    public:
        spell_warl_drain_life() : SpellScriptLoader("spell_warl_drain_life") { }

        class spell_warl_drain_life_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_drain_life_AuraScript);

            void HandleRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes mode)
            {
                if (GetCaster())
                    if (GetCaster()->HasAura(WARLOCK_SOULBURN_AURA))
                        GetCaster()->RemoveAura(WARLOCK_SOULBURN_AURA);
            }

            void OnTick(constAuraEffectPtr aurEff)
            {
                if (Unit* caster = GetCaster())
                {
                    Player* _player = GetCaster()->ToPlayer();
                    if (!_player)
                        return;

                    // Restoring 2% of the caster's total health every 1s
                    int32 basepoints = _player->GetMaxHealth() / 50;

                    // In Demonology spec : Generates 10 Demonic Fury per second
                    if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_WARLOCK_DEMONOLOGY)
                        _player->EnergizeBySpell(_player, 689, 10, POWER_DEMONIC_FURY);
                    // Soulburn : Increase heal by 50%
                    if (_player->HasAura(WARLOCK_SOULBURN_AURA))
                        basepoints = int32(basepoints * 1.5f);

                    _player->CastCustomSpell(_player, WARLOCK_DRAIN_LIFE_HEAL, &basepoints, NULL, NULL, true);
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectApplyFn(spell_warl_drain_life_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_drain_life_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_drain_life_AuraScript();
        }
};

// Soul Harvest - 101976
class spell_warl_soul_harverst : public SpellScriptLoader
{
    public:
        spell_warl_soul_harverst() : SpellScriptLoader("spell_warl_soul_harverst") { }

        class spell_warl_soul_harverst_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_soul_harverst_AuraScript);

            uint32 update;

            bool Validate(SpellInfo const* /*spell*/)
            {
                update = 0;

                if (!sSpellMgr->GetSpellInfo(101976))
                    return false;
                return true;
            }

            void OnUpdate(uint32 diff, AuraEffectPtr aurEff)
            {
                update += diff;

                if (update >= 1000)
                {
                    if (Player* _player = GetCaster()->ToPlayer())
                    {
                        if (!_player->isInCombat() && !_player->InArena() && _player->isAlive())
                        {
                            _player->SetHealth(_player->GetHealth() + int32(_player->GetMaxHealth() / 50));

                            if (Pet* pet = _player->GetPet())
                                pet->SetHealth(pet->GetHealth() + int32(pet->GetMaxHealth() / 50));
                        }
                    }

                    update = 0;
                }
            }

            void Register()
            {
                OnEffectUpdate += AuraEffectUpdateFn(spell_warl_soul_harverst_AuraScript::OnUpdate, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_soul_harverst_AuraScript();
        }
};

// Life Tap - 1454
class spell_warl_life_tap : public SpellScriptLoader
{
    public:
        spell_warl_life_tap() : SpellScriptLoader("spell_warl_life_tap") { }

        class spell_warl_life_tap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_life_tap_SpellScript);

            SpellCastResult CheckLife()
            {
                if (GetCaster()->GetHealthPct() > 15.0f)
                    return SPELL_CAST_OK;
                return SPELL_FAILED_FIZZLE;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    int32 healthCost = int32(_player->GetMaxHealth() * 0.15f);

                    _player->SetHealth(_player->GetHealth() - healthCost);
                    _player->EnergizeBySpell(_player, 1454, healthCost, POWER_MANA);
                }
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_warl_life_tap_SpellScript::CheckLife);
                OnHit += SpellHitFn(spell_warl_life_tap_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_life_tap_SpellScript();
        }
};

// Harvest Life - 115707
class spell_warl_harvest_life : public SpellScriptLoader
{
    public:
        spell_warl_harvest_life() : SpellScriptLoader("spell_warl_harvest_life") { }

        class spell_warl_harvest_life_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_harvest_life_AuraScript);

            void OnTick(constAuraEffectPtr aurEff)
            {
                if (!GetCaster())
                    return;

                if (Player* _player = GetCaster()->ToPlayer())
                {
                    // Restoring 3-4.5% of the caster's total health every 1s
                    int32 basepoints = int32(frand(0.03f, 0.045f) * _player->GetMaxHealth());

                    if (!_player->HasSpellCooldown(WARLOCK_HARVEST_LIFE_HEAL))
                    {
                        _player->CastCustomSpell(_player, WARLOCK_HARVEST_LIFE_HEAL, &basepoints, NULL, NULL, true);
                        // prevent the heal to proc off for each targets
                        _player->AddSpellCooldown(WARLOCK_HARVEST_LIFE_HEAL, 0, time(NULL) + 1);
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_harvest_life_AuraScript::OnTick, EFFECT_2, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_harvest_life_AuraScript();
        }
};

// Fear - 5782
class spell_warl_fear : public SpellScriptLoader
{
    public:
        spell_warl_fear() : SpellScriptLoader("spell_warl_fear") { }

        class spell_warl_fear_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_fear_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (_player->HasAura(WARLOCK_GLYPH_OF_FEAR))
                        {
                            _player->CastSpell(target, WARLOCK_GLYPH_OF_FEAR_EFFECT, true);
                            _player->AddSpellCooldown(5782, 0, time(NULL) + 5);
                        }
                        else
                            _player->CastSpell(target, WARLOCK_FEAR_EFFECT, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_fear_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_fear_SpellScript();
        }
};

// Updated 4.3.4
class spell_warl_banish : public SpellScriptLoader
{
public:
    spell_warl_banish() : SpellScriptLoader("spell_warl_banish") { }

    class spell_warl_banish_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_warl_banish_SpellScript);

        bool Load()
        {
            _removed = false;
            return true;
        }

        void HandleBanish()
        {
            // Casting Banish on a banished target will cancel the effect
            // Check if the target already has Banish, if so, do nothing.

            if (Unit* target = GetHitUnit())
            {
                if (target->GetAuraEffect(SPELL_AURA_SCHOOL_IMMUNITY, SPELLFAMILY_WARLOCK, 0, 0x08000000, 0))
                {
                    // No need to remove old aura since its removed due to not stack by current Banish aura
                    PreventHitDefaultEffect(EFFECT_0);
                    PreventHitDefaultEffect(EFFECT_1);
                    PreventHitDefaultEffect(EFFECT_2);
                    _removed = true;
                }
            }
        }

        void RemoveAura()
        {
            if (_removed)
                PreventHitAura();
        }

        void Register()
        {
            BeforeHit += SpellHitFn(spell_warl_banish_SpellScript::HandleBanish);
            AfterHit += SpellHitFn(spell_warl_banish_SpellScript::RemoveAura);
        }

        bool _removed;
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_warl_banish_SpellScript();
    }
};

// Updated 4.3.4
// 47193 Demonic Empowerment
class spell_warl_demonic_empowerment : public SpellScriptLoader
{
    public:
        spell_warl_demonic_empowerment() : SpellScriptLoader("spell_warl_demonic_empowerment") { }

        class spell_warl_demonic_empowerment_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_demonic_empowerment_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_EMPOWERMENT_SUCCUBUS) || !sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_EMPOWERMENT_VOIDWALKER) || !sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_EMPOWERMENT_FELGUARD) || !sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_EMPOWERMENT_FELHUNTER) || !sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_EMPOWERMENT_IMP))
                    return false;
                return true;
            }

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                if (Creature* targetCreature = GetHitCreature())
                {
                    if (targetCreature->isPet())
                    {
                        CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(targetCreature->GetEntry());
                        switch (ci->family)
                        {
                        case CREATURE_FAMILY_SUCCUBUS:
                            targetCreature->CastSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_SUCCUBUS, true);
                            break;
                        case CREATURE_FAMILY_VOIDWALKER:
                        {
                            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_EMPOWERMENT_VOIDWALKER);
                            int32 hp = int32(targetCreature->CountPctFromMaxHealth(GetCaster()->CalculateSpellDamage(targetCreature, spellInfo, 0)));
                            targetCreature->CastCustomSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_VOIDWALKER, &hp, NULL, NULL, true);
                            //unitTarget->CastSpell(unitTarget, 54441, true);
                            break;
                        }
                        case CREATURE_FAMILY_FELGUARD:
                            targetCreature->CastSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_FELGUARD, true);
                            break;
                        case CREATURE_FAMILY_FELHUNTER:
                            targetCreature->CastSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_FELHUNTER, true);
                            break;
                        case CREATURE_FAMILY_IMP:
                            targetCreature->CastSpell(targetCreature, WARLOCK_DEMONIC_EMPOWERMENT_IMP, true);
                            break;
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_demonic_empowerment_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_demonic_empowerment_SpellScript();
        }
};

// Create Healthstone - 6201
class spell_warl_create_healthstone : public SpellScriptLoader
{
    public:
        spell_warl_create_healthstone() : SpellScriptLoader("spell_warl_create_healthstone") { }

        class spell_warl_create_healthstone_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_create_healthstone_SpellScript);

            void HandleAfterCast()
            {
                GetCaster()->CastSpell(GetCaster(), WARLOCK_CREATE_HEALTHSTONE, true);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_warl_create_healthstone_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_create_healthstone_SpellScript();
        }
};

// 47422 Everlasting Affliction
class spell_warl_everlasting_affliction : public SpellScriptLoader
{
    public:
        spell_warl_everlasting_affliction() : SpellScriptLoader("spell_warl_everlasting_affliction") { }

        class spell_warl_everlasting_affliction_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_everlasting_affliction_SpellScript);

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                if (Unit* unitTarget = GetHitUnit())
                    // Refresh corruption on target
                    if (AuraEffectPtr aur = unitTarget->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_WARLOCK, 0x2, 0, 0, GetCaster()->GetGUID()))
                        aur->GetBase()->RefreshDuration();
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_everlasting_affliction_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_everlasting_affliction_SpellScript();
        }
};

class spell_warl_seed_of_corruption : public SpellScriptLoader
{
    public:
        spell_warl_seed_of_corruption() : SpellScriptLoader("spell_warl_seed_of_corruption") { }

        class spell_warl_seed_of_corruption_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_seed_of_corruption_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (GetExplTargetUnit())
                    targets.remove(GetExplTargetUnit());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warl_seed_of_corruption_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_seed_of_corruption_SpellScript();
        }
};

enum Soulshatter
{
    SPELL_SOULSHATTER   = 32835,
};

class spell_warl_soulshatter : public SpellScriptLoader
{
    public:
        spell_warl_soulshatter() : SpellScriptLoader("spell_warl_soulshatter") { }

        class spell_warl_soulshatter_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_soulshatter_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SOULSHATTER))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (Unit* target = GetHitUnit())
                {
                    if (target->CanHaveThreatList() && target->getThreatManager().getThreat(caster) > 0.0f)
                        caster->CastSpell(target, SPELL_SOULSHATTER, true);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_soulshatter_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_soulshatter_SpellScript();
        }
};

class spell_warl_demonic_circle_summon : public SpellScriptLoader
{
    public:
        spell_warl_demonic_circle_summon() : SpellScriptLoader("spell_warl_demonic_circle_summon") { }

        class spell_warl_demonic_circle_summon_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_demonic_circle_summon_AuraScript);

            void HandleRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes mode)
            {
                // If effect is removed by expire remove the summoned demonic circle too.
                if (!(mode & AURA_EFFECT_HANDLE_REAPPLY))
                    GetTarget()->RemoveGameObject(GetId(), true);

                GetTarget()->RemoveAura(WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST);
            }

            void HandleDummyTick(constAuraEffectPtr /*aurEff*/)
            {
                if (GameObject* circle = GetTarget()->GetGameObject(GetId()))
                {
                    // Here we check if player is in demonic circle teleport range, if so add
                    // WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST; allowing him to cast the WARLOCK_DEMONIC_CIRCLE_TELEPORT.
                    // If not in range remove the WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST.

                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_CIRCLE_TELEPORT);

                    if (GetTarget()->IsWithinDist(circle, spellInfo->GetMaxRange(true)))
                    {
                        if (!GetTarget()->HasAura(WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST))
                            GetTarget()->CastSpell(GetTarget(), WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST, true);
                    }
                    else
                        GetTarget()->RemoveAura(WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST);
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectApplyFn(spell_warl_demonic_circle_summon_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_demonic_circle_summon_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_demonic_circle_summon_AuraScript();
        }
};

class spell_warl_demonic_circle_teleport : public SpellScriptLoader
{
    public:
        spell_warl_demonic_circle_teleport() : SpellScriptLoader("spell_warl_demonic_circle_teleport") { }

        class spell_warl_demonic_circle_teleport_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_demonic_circle_teleport_AuraScript);

            void HandleTeleport(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Player* player = GetTarget()->ToPlayer())
                {
                    if (GameObject* circle = player->GetGameObject(WARLOCK_DEMONIC_CIRCLE_SUMMON))
                    {
                        player->NearTeleportTo(circle->GetPositionX(), circle->GetPositionY(), circle->GetPositionZ(), circle->GetOrientation());
                        player->RemoveMovementImpairingAuras();
                    }
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_warl_demonic_circle_teleport_AuraScript::HandleTeleport, EFFECT_0, SPELL_AURA_MECHANIC_IMMUNITY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_demonic_circle_teleport_AuraScript();
        }
};

class spell_warl_unstable_affliction : public SpellScriptLoader
{
    public:
        spell_warl_unstable_affliction() : SpellScriptLoader("spell_warl_unstable_affliction") { }

        class spell_warl_unstable_affliction_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_unstable_affliction_AuraScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(WARLOCK_UNSTABLE_AFFLICTION_DISPEL))
                    return false;
                return true;
            }

            void HandleDispel(DispelInfo* dispelInfo)
            {
                if (Unit* caster = GetCaster())
                    if (constAuraEffectPtr aurEff = GetEffect(EFFECT_0))
                    {
                        int32 damage = aurEff->GetAmount() * 7;
                        // backfire damage and silence
                        caster->CastCustomSpell(dispelInfo->GetDispeller(), WARLOCK_UNSTABLE_AFFLICTION_DISPEL, &damage, NULL, NULL, true, NULL, aurEff);
                    }
            }

            void Register()
            {
                AfterDispel += AuraDispelFn(spell_warl_unstable_affliction_AuraScript::HandleDispel);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_unstable_affliction_AuraScript();
        }
};

void AddSC_warlock_spell_scripts()
{
    new spell_warl_dark_bargain_on_absorb();
    new spell_warl_dark_regeneration();
    new spell_warl_soul_leech();
    new spell_warl_sacrificial_pact();
    new spell_warl_hand_of_guldan();
    new spell_warl_twilight_ward_s12();
    new spell_warl_hellfire();
    new spell_warl_demonic_leap();
    new spell_warl_grimoire_of_sacrifice();
    new spell_warl_burning_rush();
    new spell_warl_soul_swap_soulburn();
    new spell_warl_soul_swap();
    new spell_warl_nightfall();
    new spell_warl_drain_soul();
    new spell_warl_demonic_gateway_charges();
    new spell_warl_demonic_gateway();
    new spell_warl_rain_of_fire();
    new spell_warl_chaos_bolt();
    new spell_warl_ember_tap();
    new spell_warl_fire_and_brimstone();
    new spell_warl_conflagrate_aura();
    new spell_warl_shadowburn();
    new spell_warl_burning_embers();
    new spell_warl_fel_flame();
    new spell_warl_drain_life();
    new spell_warl_soul_harverst();
    new spell_warl_life_tap();
    new spell_warl_harvest_life();
    new spell_warl_fear();
    new spell_warl_banish();
    new spell_warl_demonic_empowerment();
    new spell_warl_create_healthstone();
    new spell_warl_everlasting_affliction();
    new spell_warl_seed_of_corruption();
    new spell_warl_soulshatter();
    new spell_warl_demonic_circle_summon();
    new spell_warl_demonic_circle_teleport();
    new spell_warl_unstable_affliction();
}