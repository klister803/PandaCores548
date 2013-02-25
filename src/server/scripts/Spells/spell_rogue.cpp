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
 * Scripts for spells with SPELLFAMILY_ROGUE and SPELLFAMILY_GENERIC spells used by rogue players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_rog_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum RogueSpells
{
    ROGUE_SPELL_SHIV_TRIGGERED                   = 5940,
    ROGUE_SPELL_PREY_ON_THE_WEAK                 = 58670,
    ROGUE_SPELL_RECUPERATE                       = 73651,
    ROGUE_SPELL_DEADLY_POISON                    = 2823,
    ROGUE_SPELL_WOUND_POISON                     = 8679,
    ROGUE_SPELL_MIND_NUMBLING_POISON             = 5761,
    ROGUE_SPELL_CRIPPLING_POISON                 = 3408,
    ROGUE_SPELL_LEECHING_POISON                  = 108211,
    ROGUE_SPELL_PARALYTIC_POISON                 = 108215,
    ROGUE_SPELL_PARALYTIC_POISON_DEBUFF          = 113952,
    ROGUE_SPELL_DEBILITATING_POISON              = 115196,
    ROGUE_SPELL_MIND_PARALYSIS                   = 115194,
    ROGUE_SPELL_LEECH_VITALITY                   = 116921,
    ROGUE_SPELL_PARTIAL_PARALYSIS                = 115197,
    ROGUE_SPELL_TOTAL_PARALYSIS                  = 3609,
    ROGUE_SPELL_DEADLY_POISON_DOT                = 2818,
    ROGUE_SPELL_DEADLY_POISON_INSTANT_DAMAGE     = 113780,
    ROGUE_SPELL_SLICE_AND_DICE                   = 5171,
    ROGUE_SPELL_SMOKE_BOMB_AREA_DUMMY            = 76577,
    ROGUE_SPELL_SMOKE_BOMB_AURA                  = 88611,
    ROGUE_SPELL_MASTER_POISONER_AURA             = 58410,
    ROGUE_SPELL_MASTER_POISONER_DEBUFF           = 93068,
    ROGUE_SPELL_CRIMSON_TEMPEST_DOT              = 122233,
    ROGUE_SPELL_SHROUD_OF_CONCEALMENT_AURA       = 115834,
    ROGUE_SPELL_VENOMOUS_VIM_ENERGIZE            = 51637,
    ROGUE_SPELL_VENOMOUS_WOUND_DAMAGE            = 79136,
    ROGUE_SPELL_GARROTE_DOT                      = 703,
    ROGUE_SPELL_RUPTURE_DOT                      = 1943,
    ROGUE_SPELL_CUT_TO_THE_CHASE_AURA            = 51667,
    ROGUE_SPELL_ADRENALINE_RUSH                  = 13750,
    ROGUE_SPELL_KILLING_SPREE                    = 51690,
    ROGUE_SPELL_REDIRECT                         = 73981,
    ROGUE_SPELL_SHADOW_BLADES                    = 121471,
    ROGUE_SPELL_SPRINT                           = 2983,
    ROGUE_SPELL_HEMORRHAGE_DOT                   = 89775,
    ROGUE_SPELL_SANGUINARY_VEIN_DEBUFF           = 124271,
    ROGUE_SPELL_NIGHTSTALKER_AURA                = 14062,
    ROGUE_SPELL_NIGHTSTALKER_DAMAGE_DONE         = 130493,
};

// Called by Stealth - 1784
// Nightstalker - 14062
class spell_rog_nightstalker : public SpellScriptLoader
{
    public:
        spell_rog_nightstalker() : SpellScriptLoader("spell_rog_nightstalker") { }

        class spell_rog_nightstalker_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_nightstalker_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (_player->HasAura(ROGUE_SPELL_NIGHTSTALKER_AURA))
                        _player->CastSpell(_player, ROGUE_SPELL_NIGHTSTALKER_DAMAGE_DONE, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_nightstalker_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_nightstalker_SpellScript();
        }

        class spell_rog_nightstalker_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_rog_nightstalker_AuraScript);

            void HandleRemove(constAuraEffectPtr /*aurEff*/, AuraEffectHandleModes mode)
            {
                if (GetCaster())
                    if (GetCaster()->HasAura(ROGUE_SPELL_NIGHTSTALKER_DAMAGE_DONE))
                        GetCaster()->RemoveAura(ROGUE_SPELL_NIGHTSTALKER_DAMAGE_DONE);
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_rog_nightstalker_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_MOD_SHAPESHIFT, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_rog_nightstalker_AuraScript();
        }
};

// Called by Slice and Dice - 5171
// Energetic Recovery - 79152
class spell_rog_energetic_recovery : public SpellScriptLoader
{
    public:
        spell_rog_energetic_recovery() : SpellScriptLoader("spell_rog_energetic_recovery") { }

        class spell_rog_energetic_recovery_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_rog_energetic_recovery_AuraScript);

            void HandleEffectPeriodic(constAuraEffectPtr /*aurEff*/)
            {
                if (GetCaster())
                    GetCaster()->EnergizeBySpell(GetCaster(), 5171, 8, POWER_ENERGY);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_rog_energetic_recovery_AuraScript::HandleEffectPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_ENERGIZE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_rog_energetic_recovery_AuraScript();
        }
};

// Called by Rupture - 1943, Garrote - 703 and Crimson Tempest - 121411
// Sanguinary Vein - 79147
class spell_rog_sanguinary_vein : public SpellScriptLoader
{
    public:
        spell_rog_sanguinary_vein() : SpellScriptLoader("spell_rog_sanguinary_vein") { }

        class spell_rog_sanguinary_vein_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_sanguinary_vein_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        _player->CastSpell(target, ROGUE_SPELL_SANGUINARY_VEIN_DEBUFF, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_sanguinary_vein_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_sanguinary_vein_SpellScript();
        }
};

// Hemorrhage - 16511
class spell_rog_hemorrhage : public SpellScriptLoader
{
    public:
        spell_rog_hemorrhage() : SpellScriptLoader("spell_rog_hemorrhage") { }

        class spell_rog_hemorrhage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_hemorrhage_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        int32 bp = int32(GetHitDamage() / 2 / 8);

                        _player->CastCustomSpell(target, ROGUE_SPELL_HEMORRHAGE_DOT, &bp, NULL, NULL, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_hemorrhage_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_hemorrhage_SpellScript();
        }
};

// Called by Crimson Tempest - 121411, Rupture - 1943 and Eviscerate - 2098
// Restless Blades - 79096
class spell_rog_restless_blades : public SpellScriptLoader
{
    public:
        spell_rog_restless_blades() : SpellScriptLoader("spell_rog_restless_blades") { }

        class spell_rog_restless_blades_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_restless_blades_SpellScript);

            int32 comboPoints;

            bool Validate()
            {
                comboPoints = 0;

                if (!sSpellMgr->GetSpellInfo(121411) || !sSpellMgr->GetSpellInfo(1943) || !sSpellMgr->GetSpellInfo(2098))
                    return false;
                return true;
            }

            void HandleBeforeCast()
            {
                if (GetCaster()->ToPlayer())
                    comboPoints = GetCaster()->ToPlayer()->GetComboPoints();
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (comboPoints)
                        {
                            if (_player->HasSpellCooldown(ROGUE_SPELL_ADRENALINE_RUSH))
                            {
                                uint32 newCooldownDelay = _player->GetSpellCooldownDelay(ROGUE_SPELL_ADRENALINE_RUSH);
                                newCooldownDelay -= comboPoints * 2;

                                _player->AddSpellCooldown(ROGUE_SPELL_ADRENALINE_RUSH, 0, uint32(time(NULL) + newCooldownDelay));

                                WorldPacket data(SMSG_MODIFY_COOLDOWN, 4+8+4);
                                data << uint32(ROGUE_SPELL_ADRENALINE_RUSH);     // Spell ID
                                data << uint64(_player->GetGUID());              // Player GUID
                                data << int32(-1 * comboPoints * 2000);          // Cooldown mod in milliseconds
                                _player->GetSession()->SendPacket(&data);
                            }
                            if (_player->HasSpellCooldown(ROGUE_SPELL_KILLING_SPREE))
                            {
                                uint32 newCooldownDelay = _player->GetSpellCooldownDelay(ROGUE_SPELL_KILLING_SPREE);
                                newCooldownDelay -= comboPoints * 2;

                                _player->AddSpellCooldown(ROGUE_SPELL_KILLING_SPREE, 0, uint32(time(NULL) + newCooldownDelay));

                                WorldPacket data(SMSG_MODIFY_COOLDOWN, 4+8+4);
                                data << uint32(ROGUE_SPELL_KILLING_SPREE);     // Spell ID
                                data << uint64(_player->GetGUID());              // Player GUID
                                data << int32(-1 * comboPoints * 2000);          // Cooldown mod in milliseconds
                                _player->GetSession()->SendPacket(&data);
                            }
                            if (_player->HasSpellCooldown(ROGUE_SPELL_REDIRECT))
                            {
                                uint32 newCooldownDelay = _player->GetSpellCooldownDelay(ROGUE_SPELL_REDIRECT);
                                newCooldownDelay -= comboPoints * 2;

                                _player->AddSpellCooldown(ROGUE_SPELL_REDIRECT, 0, uint32(time(NULL) + newCooldownDelay));

                                WorldPacket data(SMSG_MODIFY_COOLDOWN, 4+8+4);
                                data << uint32(ROGUE_SPELL_REDIRECT);     // Spell ID
                                data << uint64(_player->GetGUID());              // Player GUID
                                data << int32(-1 * comboPoints * 2000);          // Cooldown mod in milliseconds
                                _player->GetSession()->SendPacket(&data);
                            }
                            if (_player->HasSpellCooldown(ROGUE_SPELL_SHADOW_BLADES))
                            {
                                uint32 newCooldownDelay = _player->GetSpellCooldownDelay(ROGUE_SPELL_SHADOW_BLADES);
                                newCooldownDelay -= comboPoints * 2;

                                _player->AddSpellCooldown(ROGUE_SPELL_SHADOW_BLADES, 0, uint32(time(NULL) + newCooldownDelay));

                                WorldPacket data(SMSG_MODIFY_COOLDOWN, 4+8+4);
                                data << uint32(ROGUE_SPELL_SHADOW_BLADES);     // Spell ID
                                data << uint64(_player->GetGUID());              // Player GUID
                                data << int32(-1 * comboPoints * 2000);          // Cooldown mod in milliseconds
                                _player->GetSession()->SendPacket(&data);
                            }
                            if (_player->HasSpellCooldown(ROGUE_SPELL_SPRINT))
                            {
                                uint32 newCooldownDelay = _player->GetSpellCooldownDelay(ROGUE_SPELL_SPRINT);
                                newCooldownDelay -= comboPoints * 2;

                                _player->AddSpellCooldown(ROGUE_SPELL_SPRINT, 0, uint32(time(NULL) + newCooldownDelay));

                                WorldPacket data(SMSG_MODIFY_COOLDOWN, 4+8+4);
                                data << uint32(ROGUE_SPELL_SPRINT);     // Spell ID
                                data << uint64(_player->GetGUID());              // Player GUID
                                data << int32(-1 * comboPoints * 2000);          // Cooldown mod in milliseconds
                                _player->GetSession()->SendPacket(&data);
                            }

                            comboPoints = 0;
                        }
                    }
                }
            }

            void Register()
            {
                BeforeCast += SpellCastFn(spell_rog_restless_blades_SpellScript::HandleBeforeCast);
                OnHit += SpellHitFn(spell_rog_restless_blades_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_restless_blades_SpellScript();
        }
};

// Called by Envenom - 1329 and Eviscerate - 2098
// Cut to the Chase - 51667
class spell_rog_cut_to_the_chase : public SpellScriptLoader
{
    public:
        spell_rog_cut_to_the_chase() : SpellScriptLoader("spell_rog_cut_to_the_chase") { }

        class spell_rog_cut_to_the_chase_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_cut_to_the_chase_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (_player->HasAura(ROGUE_SPELL_CUT_TO_THE_CHASE_AURA))
                            if (AuraPtr sliceAndDice = _player->GetAura(ROGUE_SPELL_SLICE_AND_DICE, _player->GetGUID()))
                                sliceAndDice->SetDuration(sliceAndDice->GetMaxDuration());
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_cut_to_the_chase_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_cut_to_the_chase_SpellScript();
        }
};

// Called by Garrote - 703 and Rupture - 1943
// Venomous Wounds - 79134
class spell_rog_venomous_wounds : public SpellScriptLoader
{
    public:
        spell_rog_venomous_wounds() : SpellScriptLoader("spell_rog_venomous_wounds") { }

        class spell_rog_venomous_wounds_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_rog_venomous_wounds_AuraScript);

            void HandleEffectPeriodic(constAuraEffectPtr /*aurEff*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetTarget())
                    {
                        if (caster->HasAura(79134))
                        {
                            // Each time your Rupture or Garrote deals damage to an enemy that you have poisoned ...
                            if (target->HasAura(8680, caster->GetGUID())
                                || target->HasAura(2818, caster->GetGUID())
                                || target->HasAura(5760, caster->GetGUID())
                                || target->HasAura(3409, caster->GetGUID())
                                || target->HasAura(113952, caster->GetGUID())
                                || target->HasAura(112961, caster->GetGUID()))
                            {
                                if (AuraPtr rupture = target->GetAura(ROGUE_SPELL_RUPTURE_DOT, caster->GetGUID()))
                                {
                                    // ... you have a 75% chance ...
                                    if (roll_chance_i(75))
                                    {
                                        // ... to deal [ X + 16% of AP ] additional Nature damage and to regain 10 Energy
                                        caster->CastSpell(target, ROGUE_SPELL_VENOMOUS_WOUND_DAMAGE, true);
                                        int32 bp = 10;
                                        caster->CastCustomSpell(caster, ROGUE_SPELL_VENOMOUS_VIM_ENERGIZE, &bp, NULL, NULL, true);
                                    }
                                }
                                // Garrote will not trigger this effect if the enemy is also afflicted by your Rupture
                                else if (AuraPtr garrote = target->GetAura(ROGUE_SPELL_GARROTE_DOT, caster->GetGUID()))
                                {
                                    // ... you have a 75% chance ...
                                    if (roll_chance_i(75))
                                    {
                                        // ... to deal [ X + 16% of AP ] additional Nature damage and to regain 10 Energy
                                        caster->CastSpell(target, ROGUE_SPELL_VENOMOUS_WOUND_DAMAGE, true);
                                        int32 bp = 10;
                                        caster->CastCustomSpell(caster, ROGUE_SPELL_VENOMOUS_VIM_ENERGIZE, &bp, NULL, NULL, true);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            void OnRemove(constAuraEffectPtr aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetTarget())
                    {
                        AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                        if (removeMode == AURA_REMOVE_BY_DEATH)
                        {
                            if (AuraPtr rupture = aurEff->GetBase())
                            {
                                // If an enemy dies while afflicted by your Rupture, you regain energy proportional to the remaining Rupture duration
                                int32 duration = int32(rupture->GetDuration() / 1000);
                                caster->CastCustomSpell(caster, ROGUE_SPELL_VENOMOUS_VIM_ENERGIZE, &duration, NULL, NULL, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_rog_venomous_wounds_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                OnEffectRemove += AuraEffectRemoveFn(spell_rog_venomous_wounds_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_rog_venomous_wounds_AuraScript();
        }
};

// Redirect - 73981
class spell_rog_redirect : public SpellScriptLoader
{
    public:
        spell_rog_redirect() : SpellScriptLoader("spell_rog_redirect") { }

        class spell_rog_redirect_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_redirect_SpellScript);

            SpellCastResult CheckCast()
            {
                if (GetCaster())
                {
                    if (GetCaster()->GetTypeId() != TYPEID_PLAYER)
                        return SPELL_FAILED_DONT_REPORT;

                    if (!GetCaster()->ToPlayer()->GetComboPoints())
                        return SPELL_FAILED_NO_COMBO_POINTS;
                }
                else
                    return SPELL_FAILED_DONT_REPORT;

                return SPELL_CAST_OK;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        uint8 cp = _player->GetComboPoints();

                        if (cp > 5)
                            cp = 5;

                        _player->ClearComboPoints();
                        _player->AddComboPoints(target, cp, GetSpell());
                    }
                }
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_rog_redirect_SpellScript::CheckCast);
                OnHit += SpellHitFn(spell_rog_redirect_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_redirect_SpellScript();
        }
};

// Shroud of Concealment - 115834
class spell_rog_shroud_of_concealment : public SpellScriptLoader
{
    public:
        spell_rog_shroud_of_concealment() : SpellScriptLoader("spell_rog_shroud_of_concealment") { }

        class spell_rog_shroud_of_concealment_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_shroud_of_concealment_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (AuraPtr shroudOfConcealment = target->GetAura(ROGUE_SPELL_SHROUD_OF_CONCEALMENT_AURA, _player->GetGUID()))
                            if (!target->IsInRaidWith(_player) && !target->IsInPartyWith(_player))
                                target->RemoveAura(ROGUE_SPELL_SHROUD_OF_CONCEALMENT_AURA, _player->GetGUID());
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_shroud_of_concealment_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_shroud_of_concealment_SpellScript();
        }
};

// Crimson Tempest - 121411
class spell_rog_crimson_tempest : public SpellScriptLoader
{
    public:
        spell_rog_crimson_tempest() : SpellScriptLoader("spell_rog_crimson_tempest") { }

        class spell_rog_crimson_tempest_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_crimson_tempest_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        int32 damage = int32(GetHitDamage() * 0.30f / 6); // 30% / number_of_ticks
                        _player->CastCustomSpell(target, ROGUE_SPELL_CRIMSON_TEMPEST_DOT, &damage, NULL, NULL, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_crimson_tempest_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_crimson_tempest_SpellScript();
        }
};

// Called by Wound Poison - 8680, Deadly Poison - 2818, Mind-Numbing Poison - 5760, Crippling Poison - 3409
// Paralytic Poison - 113952, Leeching Poison - 112961 and Deadly Poison : Instant damage - 113780
// Master Poisoner - 58410
class spell_rog_master_poisoner : public SpellScriptLoader
{
    public:
        spell_rog_master_poisoner() : SpellScriptLoader("spell_rog_master_poisoner") { }

        class spell_rog_master_poisoner_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_master_poisoner_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (_player->HasAura(ROGUE_SPELL_MASTER_POISONER_AURA))
                            _player->CastSpell(target, ROGUE_SPELL_MASTER_POISONER_DEBUFF, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_master_poisoner_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_master_poisoner_SpellScript();
        }
};

// Smoke Bomb - 88611
class spell_rog_smoke_bomb_aura : public SpellScriptLoader
{
    public:
        spell_rog_smoke_bomb_aura() : SpellScriptLoader("spell_rog_smoke_bomb_aura") { }

        class spell_rog_smoke_bomb_aura_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_smoke_bomb_aura_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (target->GetGUID() != _player->GetGUID() && !target->IsFriendlyTo(_player))
                        {
                            target->InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
                            target->AttackStop();
                            if (target->ToPlayer())
                                target->ToPlayer()->SendAttackSwingCancelAttack();
                        }
                        else if (target->GetGUID() == _player->GetGUID())
                            _player->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_smoke_bomb_aura_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_smoke_bomb_aura_SpellScript();
        }

        class spell_rog_smoke_bomb_aura_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_rog_smoke_bomb_aura_AuraScript);

            void OnRemove(constAuraEffectPtr aurEff, AuraEffectHandleModes /*mode*/)
            {
               if (GetCaster())
                   if (GetTarget()->GetGUID() == GetCaster()->GetGUID())
                       if (GetTarget()->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE))
                           GetTarget()->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_rog_smoke_bomb_aura_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_INTERFERE_TARGETTING, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_rog_smoke_bomb_aura_AuraScript();
        }
};

// Smoke Bomb - 76577
class spell_rog_smoke_bomb : public SpellScriptLoader
{
    public:
        spell_rog_smoke_bomb() : SpellScriptLoader("spell_rog_smoke_bomb") { }

        class spell_rog_smoke_bomb_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_rog_smoke_bomb_AuraScript);

            void OnTick(constAuraEffectPtr aurEff)
            {
                if (DynamicObject* dynObj = GetCaster()->GetDynObject(ROGUE_SPELL_SMOKE_BOMB_AREA_DUMMY))
                    GetCaster()->CastSpell(dynObj->GetPositionX(), dynObj->GetPositionY(), dynObj->GetPositionZ(), ROGUE_SPELL_SMOKE_BOMB_AURA, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_rog_smoke_bomb_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_rog_smoke_bomb_AuraScript();
        }
};

// Slice and Dice - 5171
class spell_rog_slice_and_dice : public SpellScriptLoader
{
    public:
        spell_rog_slice_and_dice() : SpellScriptLoader("spell_rog_slice_and_dice") { }

        class spell_rog_slice_and_dice_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_slice_and_dice_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (AuraPtr sliceAndDice = _player->GetAura(ROGUE_SPELL_SLICE_AND_DICE))
                    {
                        int32 duration = sliceAndDice->GetDuration();
                        int32 maxDuration = sliceAndDice->GetMaxDuration();

                        // Replace old duration of Slice and Dice by the new duration ...
                        // ... five combo points : 36s instead of 30s
                        if (maxDuration >= 30000)
                        {
                            sliceAndDice->SetDuration(duration + 6000);
                            sliceAndDice->SetMaxDuration(maxDuration + 6000);
                        }
                        // ... four combo points : 30s instead of 25s
                        else if (maxDuration >= 25000)
                        {
                            sliceAndDice->SetDuration(duration + 5000);
                            sliceAndDice->SetMaxDuration(maxDuration + 5000);
                        }
                        // ... three combo points : 24s instead of 20s
                        else if (maxDuration >= 20000)
                        {
                            sliceAndDice->SetDuration(duration + 4000);
                            sliceAndDice->SetMaxDuration(maxDuration + 4000);
                        }
                        // ... two combo points : 18s instead of 15s
                        else if (maxDuration >= 15000)
                        {
                            sliceAndDice->SetDuration(duration + 3000);
                            sliceAndDice->SetMaxDuration(maxDuration + 3000);
                        }
                        // ... one combo point : 12s instead of 10s
                        else
                        {
                            sliceAndDice->SetDuration(duration + 2000);
                            sliceAndDice->SetMaxDuration(maxDuration + 2000);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_slice_and_dice_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_slice_and_dice_SpellScript();
        }
};

// Called by Deadly Poison - 2818
// Deadly Poison : Instant damage - 113780
class spell_rog_deadly_poison_instant_damage : public SpellScriptLoader
{
    public:
        spell_rog_deadly_poison_instant_damage() : SpellScriptLoader("spell_rog_deadly_poison_instant_damage") { }

        class spell_rog_deadly_poison_instant_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_deadly_poison_instant_damage_SpellScript);

            void HandleOnCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetExplTargetUnit())
                        if (target->HasAura(ROGUE_SPELL_DEADLY_POISON_DOT, _player->GetGUID()))
                            _player->CastSpell(target, ROGUE_SPELL_DEADLY_POISON_INSTANT_DAMAGE, true);
            }

            void Register()
            {
                OnCast += SpellCastFn(spell_rog_deadly_poison_instant_damage_SpellScript::HandleOnCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_deadly_poison_instant_damage_SpellScript();
        }
};

// Paralytic Poison - 113952
class spell_rog_paralytic_poison : public SpellScriptLoader
{
    public:
        spell_rog_paralytic_poison() : SpellScriptLoader("spell_rog_paralytic_poison") { }

        class spell_rog_paralytic_poison_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_paralytic_poison_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (AuraPtr paralyticPoison = target->GetAura(ROGUE_SPELL_PARALYTIC_POISON_DEBUFF))
                        {
                            if (paralyticPoison->GetStackAmount() == 5 && !target->HasAura(ROGUE_SPELL_TOTAL_PARALYSIS))
                            {
                                _player->CastSpell(target, ROGUE_SPELL_TOTAL_PARALYSIS, true);
                                target->RemoveAura(ROGUE_SPELL_PARALYTIC_POISON_DEBUFF);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_paralytic_poison_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_paralytic_poison_SpellScript();
        }
};

// Shiv - 5938
class spell_rog_shiv : public SpellScriptLoader
{
    public:
        spell_rog_shiv() : SpellScriptLoader("spell_rog_shiv") { }

        class spell_rog_shiv_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_shiv_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (_player->HasAura(ROGUE_SPELL_MIND_NUMBLING_POISON))
                            _player->CastSpell(target, ROGUE_SPELL_DEBILITATING_POISON, true);
                        else if (_player->HasAura(ROGUE_SPELL_CRIPPLING_POISON))
                            _player->CastSpell(target, ROGUE_SPELL_MIND_PARALYSIS, true);
                        else if (_player->HasAura(ROGUE_SPELL_LEECHING_POISON))
                            _player->CastSpell(_player, ROGUE_SPELL_LEECH_VITALITY, true);
                        else if (_player->HasAura(ROGUE_SPELL_PARALYTIC_POISON))
                            _player->CastSpell(target, ROGUE_SPELL_PARTIAL_PARALYSIS, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_shiv_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_shiv_SpellScript();
        }
};

// All Poisons
// Deadly Poison - 2823, Wound Poison - 8679, Mind-numbing Poison - 5761, Leeching Poison - 108211, Paralytic Poison - 108215 or Crippling Poison - 3408
class spell_rog_poisons : public SpellScriptLoader
{
    public:
        spell_rog_poisons() : SpellScriptLoader("spell_rog_poisons") { }

        class spell_rog_poisons_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_poisons_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    switch (GetSpellInfo()->Id)
                    {
                        case ROGUE_SPELL_WOUND_POISON:
                        {
                            if (_player->HasAura(ROGUE_SPELL_DEADLY_POISON))
                                _player->RemoveAura(ROGUE_SPELL_DEADLY_POISON);
                            break;
                        }
                        case ROGUE_SPELL_MIND_NUMBLING_POISON:
                        {
                            if (_player->HasAura(ROGUE_SPELL_CRIPPLING_POISON))
                                _player->RemoveAura(ROGUE_SPELL_CRIPPLING_POISON);
                            if (_player->HasAura(ROGUE_SPELL_LEECHING_POISON))
                                _player->RemoveAura(ROGUE_SPELL_LEECHING_POISON);
                            if (_player->HasAura(ROGUE_SPELL_PARALYTIC_POISON))
                                _player->RemoveAura(ROGUE_SPELL_PARALYTIC_POISON);
                            break;
                        }
                        case ROGUE_SPELL_CRIPPLING_POISON:
                        {
                            if (_player->HasAura(ROGUE_SPELL_MIND_NUMBLING_POISON))
                                _player->RemoveAura(ROGUE_SPELL_MIND_NUMBLING_POISON);
                            if (_player->HasAura(ROGUE_SPELL_LEECHING_POISON))
                                _player->RemoveAura(ROGUE_SPELL_LEECHING_POISON);
                            if (_player->HasAura(ROGUE_SPELL_PARALYTIC_POISON))
                                _player->RemoveAura(ROGUE_SPELL_PARALYTIC_POISON);
                            break;
                        }
                        case ROGUE_SPELL_LEECHING_POISON:
                        {
                            if (_player->HasAura(ROGUE_SPELL_MIND_NUMBLING_POISON))
                                _player->RemoveAura(ROGUE_SPELL_MIND_NUMBLING_POISON);
                            if (_player->HasAura(ROGUE_SPELL_CRIPPLING_POISON))
                                _player->RemoveAura(ROGUE_SPELL_CRIPPLING_POISON);
                            if (_player->HasAura(ROGUE_SPELL_PARALYTIC_POISON))
                                _player->RemoveAura(ROGUE_SPELL_PARALYTIC_POISON);
                            break;
                        }
                        case ROGUE_SPELL_PARALYTIC_POISON:
                        {
                            if (_player->HasAura(ROGUE_SPELL_MIND_NUMBLING_POISON))
                                _player->RemoveAura(ROGUE_SPELL_MIND_NUMBLING_POISON);
                            if (_player->HasAura(ROGUE_SPELL_CRIPPLING_POISON))
                                _player->RemoveAura(ROGUE_SPELL_CRIPPLING_POISON);
                            if (_player->HasAura(ROGUE_SPELL_LEECHING_POISON))
                                _player->RemoveAura(ROGUE_SPELL_LEECHING_POISON);
                            break;
                        }
                        case ROGUE_SPELL_DEADLY_POISON:
                        {
                            if (_player->HasAura(ROGUE_SPELL_WOUND_POISON))
                                _player->RemoveAura(ROGUE_SPELL_WOUND_POISON);
                            break;
                        }
                        default:
                            break;
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_poisons_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_poisons_SpellScript();
        }
};

// Recuperate - 73651
class spell_rog_recuperate : public SpellScriptLoader
{
    public:
        spell_rog_recuperate() : SpellScriptLoader("spell_rog_recuperate") { }

        class spell_rog_recuperate_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_recuperate_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (AuraPtr recuperate = _player->GetAura(ROGUE_SPELL_RECUPERATE))
                    {
                        int32 bp = _player->CountPctFromMaxHealth(3);
                        recuperate->GetEffect(0)->ChangeAmount(bp);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_rog_recuperate_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_recuperate_SpellScript();
        }
};

// Preparation - 14185
class spell_rog_preparation : public SpellScriptLoader
{
    public:
        spell_rog_preparation() : SpellScriptLoader("spell_rog_preparation") { }

        class spell_rog_preparation_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_preparation_SpellScript);

            bool Load()
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Player* caster = GetCaster()->ToPlayer();

                //immediately finishes the cooldown on certain Rogue abilities
                const SpellCooldowns& cm = caster->GetSpellCooldownMap();
                for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end();)
                {
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);

                    if (spellInfo->SpellFamilyName == SPELLFAMILY_ROGUE)
                    {
                        if (spellInfo->SpellFamilyFlags[0] & SPELLFAMILYFLAG_ROGUE_VAN_EVAS_SPRINT ||   // Vanish, Evasion, Sprint
                            spellInfo->Id == 31224 ||
                            spellInfo->SpellFamilyFlags[1] & SPELLFAMILYFLAG1_ROGUE_DISMANTLE)          // Dismantle
                            caster->RemoveSpellCooldown((itr++)->first, true);
                        else
                            ++itr;
                    }
                    else
                        ++itr;
                }
            }

            void Register()
            {
                // add dummy effect spell handler to Preparation
                OnEffectHitTarget += SpellEffectFn(spell_rog_preparation_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_preparation_SpellScript();
        }
};

// 51685-51689 Prey on the Weak
class spell_rog_prey_on_the_weak : public SpellScriptLoader
{
public:
    spell_rog_prey_on_the_weak() : SpellScriptLoader("spell_rog_prey_on_the_weak") { }

    class spell_rog_prey_on_the_weak_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_rog_prey_on_the_weak_AuraScript);

        bool Validate(SpellInfo const* /*spellEntry*/)
        {
            if (!sSpellMgr->GetSpellInfo(ROGUE_SPELL_PREY_ON_THE_WEAK))
                return false;
            return true;
        }

        void HandleEffectPeriodic(constAuraEffectPtr /*aurEff*/)
        {
            Unit* target = GetTarget();
            Unit* victim = target->getVictim();
            if (victim && (target->GetHealthPct() > victim->GetHealthPct()))
            {
                if (!target->HasAura(ROGUE_SPELL_PREY_ON_THE_WEAK))
                {
                    int32 bp = GetSpellInfo()->Effects[EFFECT_0].CalcValue();
                    target->CastCustomSpell(target, ROGUE_SPELL_PREY_ON_THE_WEAK, &bp, 0, 0, true);
                }
            }
            else
                target->RemoveAurasDueToSpell(ROGUE_SPELL_PREY_ON_THE_WEAK);
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_rog_prey_on_the_weak_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_rog_prey_on_the_weak_AuraScript();
    }
};

class spell_rog_deadly_poison : public SpellScriptLoader
{
    public:
        spell_rog_deadly_poison() : SpellScriptLoader("spell_rog_deadly_poison") { }

        class spell_rog_deadly_poison_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_deadly_poison_SpellScript);

            bool Load()
            {
                _stackAmount = 0;
                // at this point CastItem must already be initialized
                return GetCaster()->GetTypeId() == TYPEID_PLAYER && GetCastItem();
            }

            void HandleBeforeHit()
            {
                if (Unit* target = GetHitUnit())
                    // Deadly Poison
                    if (constAuraEffectPtr aurEff = target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_ROGUE, 0x10000, 0x80000, 0, GetCaster()->GetGUID()))
                        _stackAmount = aurEff->GetBase()->GetStackAmount();
            }

            void HandleAfterHit()
            {
                if (_stackAmount < 5)
                    return;

                Player* player = GetCaster()->ToPlayer();

                if (Unit* target = GetHitUnit())
                {

                    Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);

                    if (item == GetCastItem())
                        item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

                    if (!item)
                        return;

                    // item combat enchantments
                    for (uint8 slot = 0; slot < MAX_ENCHANTMENT_SLOT; ++slot)
                    {
                        if (slot > PRISMATIC_ENCHANTMENT_SLOT || slot < PROP_ENCHANTMENT_SLOT_0)    // not holding enchantment id
                            continue;

                        SpellItemEnchantmentEntry const* enchant = sSpellItemEnchantmentStore.LookupEntry(item->GetEnchantmentId(EnchantmentSlot(slot)));
                        if (!enchant)
                            continue;

                        for (uint8 s = 0; s < 3; ++s)
                        {
                            if (enchant->type[s] != ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL)
                                continue;

                            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(enchant->spellid[s]);
                            if (!spellInfo)
                            {
                                sLog->outError(LOG_FILTER_SPELLS_AURAS, "Player::CastItemCombatSpell Enchant %i, player (Name: %s, GUID: %u) cast unknown spell %i", enchant->ID, player->GetName(), player->GetGUIDLow(), enchant->spellid[s]);
                                continue;
                            }

                            // Proc only rogue poisons
                            if (spellInfo->SpellFamilyName != SPELLFAMILY_ROGUE || spellInfo->Dispel != DISPEL_POISON)
                                continue;

                            // Do not reproc deadly
                            if (spellInfo->SpellFamilyFlags.IsEqual(0x10000, 0x80000, 0))
                                continue;

                            if (spellInfo->IsPositive())
                                player->CastSpell(player, enchant->spellid[s], true, item);
                            else
                                player->CastSpell(target, enchant->spellid[s], true, item);
                        }
                    }
                }
            }

            void Register()
            {
                BeforeHit += SpellHitFn(spell_rog_deadly_poison_SpellScript::HandleBeforeHit);
                AfterHit += SpellHitFn(spell_rog_deadly_poison_SpellScript::HandleAfterHit);
            }

            uint8 _stackAmount;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_deadly_poison_SpellScript();
        }
};

class spell_rog_shadowstep : public SpellScriptLoader
{
    public:
        spell_rog_shadowstep() : SpellScriptLoader("spell_rog_shadowstep") { }

        class spell_rog_shadowstep_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_rog_shadowstep_SpellScript);

            SpellCastResult CheckCast()
            {
                if (GetCaster()->HasUnitState(UNIT_STATE_ROOT))
                    return SPELL_FAILED_ROOTED;
                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_rog_shadowstep_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_rog_shadowstep_SpellScript();
        }
};

void AddSC_rogue_spell_scripts()
{
    new spell_rog_nightstalker();
    new spell_rog_energetic_recovery();
    new spell_rog_sanguinary_vein();
    new spell_rog_hemorrhage();
    new spell_rog_restless_blades();
    new spell_rog_cut_to_the_chase();
    new spell_rog_venomous_wounds();
    new spell_rog_redirect();
    new spell_rog_shroud_of_concealment();
    new spell_rog_crimson_tempest();
    new spell_rog_master_poisoner();
    new spell_rog_smoke_bomb_aura();
    new spell_rog_smoke_bomb();
    new spell_rog_slice_and_dice();
    new spell_rog_deadly_poison_instant_damage();
    new spell_rog_paralytic_poison();
    new spell_rog_shiv();
    new spell_rog_poisons();
    new spell_rog_recuperate();
    new spell_rog_preparation();
    new spell_rog_prey_on_the_weak();
    new spell_rog_deadly_poison();
    new spell_rog_shadowstep();
}
