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
 * Scripts for spells with SPELLFAMILY_MAGE and SPELLFAMILY_GENERIC spells used by mage players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_mage_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "ScriptedCreature.h"

#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"

enum MageSpells
{
    SPELL_MAGE_COLD_SNAP                         = 11958,
    SPELL_MAGE_SQUIRREL_FORM                     = 32813,
    SPELL_MAGE_GIRAFFE_FORM                      = 32816,
    SPELL_MAGE_SERPENT_FORM                      = 32817,
    SPELL_MAGE_DRAGONHAWK_FORM                   = 32818,
    SPELL_MAGE_WORGEN_FORM                       = 32819,
    SPELL_MAGE_SHEEP_FORM                        = 32820,
    SPELL_MAGE_GLYPH_OF_ETERNAL_WATER            = 70937,
    SPELL_MAGE_SUMMON_WATER_ELEMENTAL_PERMANENT  = 70908,
    SPELL_MAGE_SUMMON_WATER_ELEMENTAL_TEMPORARY  = 70907,
    SPELL_MAGE_GLYPH_OF_BLAST_WAVE               = 62126,
    SPELL_MAGE_ALTER_TIME_OVERRIDED              = 127140,
    SPELL_MAGE_ALTER_TIME                        = 110909,
    SPELL_MAGE_TEMPORAL_DISPLACEMENT             = 80354,
    HUNTER_SPELL_INSANITY                        = 95809,
    SPELL_SHAMAN_SATED                           = 57724,
    SPELL_SHAMAN_EXHAUSTED                       = 57723,
    SPELL_MAGE_CONJURE_REFRESHMENT_R1            = 92739,
    SPELL_MAGE_CONJURE_REFRESHMENT_R2            = 92799,
    SPELL_MAGE_CONJURE_REFRESHMENT_R3            = 92802,
    SPELL_MAGE_CONJURE_REFRESHMENT_R4            = 92805,
    SPELL_MAGE_CONJURE_REFRESHMENT_R5            = 74625,
    SPELL_MAGE_CONJURE_REFRESHMENT_R6            = 42956,
    SPELL_MAGE_CONJURE_REFRESHMENT_R7            = 92727,
    SPELL_MAGE_CONJURE_REFRESHMENT_R8            = 116130,
    SPELL_MAGE_MANA_GEM_ENERGIZE                 = 10052,
    SPELL_MAGE_ARCANE_BRILLIANCE                 = 1459,
    SPELL_MAGE_INFERNO_BLAST                     = 108853,
    SPELL_MAGE_INFERNO_BLAST_IMPACT              = 118280,
    SPELL_MAGE_IGNITE                            = 12654,
    SPELL_MAGE_PYROBLAST                         = 11366,
    SPELL_MAGE_COMBUSTION_DOT                    = 83853,
    SPELL_MAGE_COMBUSTION_IMPACT                 = 118271,
    SPELL_MAGE_FROSTJAW                          = 102051,
    SPELL_MAGE_NETHER_TEMPEST_DIRECT_DAMAGE      = 114954,
    SPELL_MAGE_NETHER_TEMPEST_VISUAL             = 114924,
    SPELL_MAGE_NETHER_TEMPEST_MISSILE            = 114956,
    SPELL_MAGE_LIVING_BOMB_TRIGGERED             = 44461,
    SPELL_MAGE_FROST_BOMB_TRIGGERED              = 113092,
    SPELL_MAGE_INVOKERS_ENERGY                   = 116257,
    SPELL_MAGE_INVOCATION                        = 114003,
    SPELL_MAGE_GLYPH_OF_EVOCATION                = 56380,
    SPELL_MAGE_BRAIN_FREEZE                      = 44549,
    SPELL_MAGE_BRAIN_FREEZE_TRIGGERED            = 57761,
    SPELL_MAGE_SLOW                              = 31589,
    SPELL_MAGE_ARCANE_CHARGE                     = 36032,
    SPELL_MAGE_ARCANE_BARRAGE_TRIGGERED          = 50273,
    SPELL_MAGE_PYROMANIAC_AURA                   = 132209,
    SPELL_MAGE_PYROMANIAC_DAMAGE_DONE            = 132210,
    SPELL_MAGE_MIRROR_IMAGE_SUMMON               = 58832,
    SPELL_MAGE_CAUTERIZE                         = 87023,
    SPELL_MAGE_ARCANE_MISSILES                   = 79683,
    SPELL_MAGE_INCANTERS_WARD_ENERGIZE           = 113842,
    SPELL_MAGE_INCANTERS_ABSORBTION              = 116267,
    SPELL_MAGE_INCANTERS_ABSORBTION_PASSIVE      = 118858,
    SPELL_DRUMS_OF_RAGE                          = 146555,
    SPELL_MAGE_FROSTBOLT                         = 116,
    SPELL_MAGE_ICE_LANCE                         = 30455, 
    SPELL_MAGE_FROSTFIRE_BOLT                    = 44614, 
    SPELL_MAGE_WATERBOLT                         = 31707,
    SPELL_MAGE_ICY_VEINS                         = 131078,
};

// Incanter's Ward (Cooldown marker) - 118859
class spell_mage_incanters_ward_cooldown : public SpellScriptLoader
{
    public:
        spell_mage_incanters_ward_cooldown() : SpellScriptLoader("spell_mage_incanters_ward_cooldown") { }

        class spell_mage_incanters_ward_cooldown_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_incanters_ward_cooldown_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    caster->RemoveAura(SPELL_MAGE_INCANTERS_ABSORBTION_PASSIVE);
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (!caster->HasAura(SPELL_MAGE_INCANTERS_ABSORBTION_PASSIVE))
                        caster->CastSpell(caster, SPELL_MAGE_INCANTERS_ABSORBTION_PASSIVE, true);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_mage_incanters_ward_cooldown_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_mage_incanters_ward_cooldown_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_incanters_ward_cooldown_AuraScript();
        }
};

// Incanter's Ward - 1463
class spell_mage_incanters_ward : public SpellScriptLoader
{
    public:
        spell_mage_incanters_ward() : SpellScriptLoader("spell_mage_incanters_ward") { }

        class spell_mage_incanters_ward_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_incanters_ward_AuraScript);

            float absorbTotal;
            float absorbtionAmount;

            bool Load()
            {
                absorbTotal = 0.0f;
                absorbtionAmount = 0.0f;
                return true;
            }

            void CalculateAmount(AuraEffect const* , int32 & amount, bool & )
            {
                if (Unit* caster = GetCaster())
                    amount += caster->GetSpellPowerDamage(SPELL_SCHOOL_MASK_ARCANE);

                absorbtionAmount = float(amount);
            }

            void OnAbsorb(AuraEffect* aurEff, DamageInfo& dmgInfo, uint32& absorbAmount)
            {
                if (Unit* caster = dmgInfo.GetVictim())
                {
                    if (Unit* attacker = dmgInfo.GetAttacker())
                    {
                        absorbTotal += float(dmgInfo.GetDamage());

                        int32 pct = aurEff->GetSpellInfo()->Effects[EFFECT_1].CalcValue(GetCaster());
                        int32 manaGain = CalculatePct(caster->GetMaxPower(POWER_MANA), CalculatePct(((float(dmgInfo.GetDamage()) / absorbtionAmount) * 100.0f), pct));

                        caster->EnergizeBySpell(caster, SPELL_MAGE_INCANTERS_WARD_ENERGIZE, manaGain, POWER_MANA);
                    }
                }
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if(absorbTotal > absorbtionAmount)
                        absorbTotal = absorbtionAmount;      
                        
                    int32 damageGain = CalculatePct(sSpellMgr->GetSpellInfo(SPELL_MAGE_INCANTERS_ABSORBTION)->Effects[0].BasePoints, ((absorbTotal / absorbtionAmount) * 100.0f));
                    if (!damageGain)
                        return;

                    caster->CastCustomSpell(caster, SPELL_MAGE_INCANTERS_ABSORBTION, &damageGain, NULL, NULL, true);
                }
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_incanters_ward_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_mage_incanters_ward_AuraScript::OnAbsorb, EFFECT_0);
                OnEffectRemove += AuraEffectRemoveFn(spell_mage_incanters_ward_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_incanters_ward_AuraScript();
        }
};

// Arcane Missiles - 5143
class spell_mage_arcane_missile : public SpellScriptLoader
{
    public:
        spell_mage_arcane_missile() : SpellScriptLoader("spell_mage_arcane_missile") { }

        class spell_mage_arcane_missile_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_arcane_missile_AuraScript);

            bool casterCharge;

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                casterCharge = false;
                if (!GetCaster())
                    return;

                if (Player* _player = GetCaster()->ToPlayer())
                    if (Aura* arcaneMissiles = _player->GetAura(SPELL_MAGE_ARCANE_MISSILES))
                        arcaneMissiles->DropCharge();
            }

            void OnTick(AuraEffect const* aurEff)
            {
                Player* target = GetTarget()->ToPlayer();
                if (!target)
                    return;

                if (target->GetSpecializationId(target->GetActiveSpec()) != SPEC_MAGE_ARCANE)
                    return;

                if (!casterCharge)
                {
                    casterCharge = true;
                    target->CastSpell(target, SPELL_MAGE_ARCANE_CHARGE, true);
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_mage_arcane_missile_AuraScript::OnApply, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_mage_arcane_missile_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }

        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_arcane_missile_AuraScript();
        }
};

// Cauterize - 86949
class spell_mage_cauterize : public SpellScriptLoader
{
    public:
        spell_mage_cauterize() : SpellScriptLoader("spell_mage_cauterize") { }

        class spell_mage_cauterize_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_cauterize_AuraScript);

            uint32 healtPct;

            bool Load()
            {
                healtPct = GetSpellInfo()->Effects[EFFECT_1].CalcValue(GetCaster());
                return GetUnitOwner()->ToPlayer();
            }

            void CalculateAmount(AuraEffect const* /*AuraEffect**/, int32& amount, bool& /*canBeRecalculated*/)
            {
                amount = -1;
            }

            void Absorb(AuraEffect* /*AuraEffect**/, DamageInfo& dmgInfo, uint32& absorbAmount)
            {
                Unit* target = GetCaster();

                if (dmgInfo.GetDamage() < target->GetHealth())
                    return;

                if (target->ToPlayer()->HasSpellCooldown(SPELL_MAGE_CAUTERIZE))
                    return;

                int32 bp1 = target->CountPctFromMaxHealth(healtPct);
                target->CastCustomSpell(target, SPELL_MAGE_CAUTERIZE, NULL, &bp1, NULL, true);
                target->ToPlayer()->AddSpellCooldown(SPELL_MAGE_CAUTERIZE, 0, getPreciseTime() + 60);

                absorbAmount = dmgInfo.GetDamage();
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_cauterize_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_mage_cauterize_AuraScript::Absorb, EFFECT_0);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_cauterize_AuraScript();
        }
};

// Called by Nether Tempest - 114923, Frost Bomb - 112948 and Living Bomb - 44457
// Pyromaniac - 132209
class spell_mage_pyromaniac : public SpellScriptLoader
{
    public:
        spell_mage_pyromaniac() : SpellScriptLoader("spell_mage_pyromaniac") { }

        class spell_mage_pyromaniac_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_pyromaniac_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (_player->HasAura(SPELL_MAGE_PYROMANIAC_AURA))
                            _player->CastSpell(target, SPELL_MAGE_PYROMANIAC_DAMAGE_DONE, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_pyromaniac_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_pyromaniac_SpellScript();
        }
};

class CheckArcaneBarrageImpactPredicate
{
    public:
        CheckArcaneBarrageImpactPredicate(Unit* caster, Unit* mainTarget) : _caster(caster), _mainTarget(mainTarget) {}

        bool operator()(Unit* target)
        {
            if (!_caster || !_mainTarget)
                return true;

            if (!_caster->IsValidAttackTarget(target))
                return true;

            if (!target->IsWithinLOSInMap(_caster))
                return true;

            if (!_caster->isInFront(target))
                return true;

            if (target->GetGUID() == _caster->GetGUID())
                return true;

            if (target->GetGUID() == _mainTarget->GetGUID())
                return true;

            return false;
        }

    private:
        Unit* _caster;
        Unit* _mainTarget;
};

// Arcane Barrage - 44425
class spell_mage_arcane_barrage : public SpellScriptLoader
{
    public:
        spell_mage_arcane_barrage() : SpellScriptLoader("spell_mage_arcane_barrage") { }

        class spell_mage_arcane_barrage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_arcane_barrage_SpellScript);

            void HandleEffectHit(SpellEffIndex /*effIndex*/)
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        /*if (Aura* arcaneCharge = _player->GetAura(SPELL_MAGE_ARCANE_CHARGE))
                        {
                            chargeCount = arcaneCharge->GetStackAmount();
                            _player->RemoveAura(arcaneCharge);
                        }*/
                        if(_player->GetSelectedUnit() != target)
                        {
                            int32 _damage = int32((GetHitDamage() * GetSpellInfo()->Effects[EFFECT_1].BasePoints) / 100);
                            SetHitDamage(_damage);
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_mage_arcane_barrage_SpellScript::HandleEffectHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_arcane_barrage_SpellScript();
        }
};

// Arcane Explosion - 1449
class spell_mage_arcane_explosion : public SpellScriptLoader
{
    public:
        spell_mage_arcane_explosion() : SpellScriptLoader("spell_mage_arcane_explosion") { }

        class spell_mage_arcane_explosion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_arcane_explosion_SpellScript);

            bool casted;

        public:
            spell_mage_arcane_explosion_SpellScript() : SpellScript()
            {
                casted = false;
            }

            void HandleOnHit()
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                Player* player = caster->ToPlayer();
                if (!player)
                    return;

                if (player->GetSpecializationId(player->GetActiveSpec()) != SPEC_MAGE_ARCANE)
                    return;

                if (!casted)
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (roll_chance_i(30))
                        {
                            casted = true;
                            caster->CastSpell(caster, SPELL_MAGE_ARCANE_CHARGE, true);
                        }
                    }
                }

                if (!casted)
                {
                    if (Unit* target = GetHitUnit())
                        if (Aura* arcaneCharge = caster->GetAura(SPELL_MAGE_ARCANE_CHARGE))
                            arcaneCharge->RefreshDuration();
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_arcane_explosion_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_arcane_explosion_SpellScript();
        }
};

// Slow - 31589
class spell_mage_slow : public SpellScriptLoader
{
    public:
        spell_mage_slow() : SpellScriptLoader("spell_mage_slow") { }

        class spell_mage_slow_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_slow_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (target->GetTypeId() == TYPEID_PLAYER)
                        {
                            if (Aura* frostjaw = target->GetAura(SPELL_MAGE_SLOW, _player->GetGUID()))
                            {
                                // Only half time against players
                                frostjaw->SetDuration(frostjaw->GetMaxDuration() / 2);
                                frostjaw->SetMaxDuration(frostjaw->GetDuration());
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_slow_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_slow_SpellScript();
        }
};

// Frostbolt - 116
class spell_mage_frostbolt : public SpellScriptLoader
{
    public:
        spell_mage_frostbolt() : SpellScriptLoader("spell_mage_frostbolt") { }

        class spell_mage_frostbolt_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_frostbolt_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                return true;
            }

            SpellCastResult CheckTarget()
            {
                Unit* target = GetExplTargetUnit();
                if (!target || target == GetCaster() || target->IsFriendlyTo(GetCaster()))
                    target = GetCaster()->GetGuardianPet();

                if (!target)
                    return SPELL_FAILED_NO_VALID_TARGETS;

                return SPELL_CAST_OK;
            }

            void OnTargetSelect(WorldObject* &obj)
            {
                if (!obj || !obj->isType(TYPEMASK_UNIT))
                    return;

                if (obj == GetCaster() || obj->ToUnit()->IsFriendlyTo(GetCaster()))
                {
                    if (Unit* pet = GetCaster()->GetGuardianPet())
                        obj = pet;
                }
            }

            void HandleEffect(SpellEffIndex effIndex)
            {
                Unit* target = GetHitUnit();
                if (!target)
                    return;

                Unit* caster = GetCaster();
                if (!caster)
                    return;

                if (target->GetOwnerGUID() != caster->GetGUID())
                    return;

                switch (effIndex)
                {
                    case EFFECT_0:
                    case EFFECT_3:
                        PreventHitAura();
                        break;
                    case EFFECT_1:
                        PreventHitDamage();
                        break;
                    case EFFECT_2:
                        caster->CastSpell(target, 126201, true);
                        break;
                }
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_mage_frostbolt_SpellScript::CheckTarget);
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_mage_frostbolt_SpellScript::OnTargetSelect, EFFECT_0, TARGET_UNIT_TARGET_ANY);
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_mage_frostbolt_SpellScript::OnTargetSelect, EFFECT_1, TARGET_UNIT_TARGET_ANY);
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_mage_frostbolt_SpellScript::OnTargetSelect, EFFECT_2, TARGET_UNIT_TARGET_ANY);
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_mage_frostbolt_SpellScript::OnTargetSelect, EFFECT_3, TARGET_UNIT_TARGET_ANY);
                OnEffectHitTarget += SpellEffectFn(spell_mage_frostbolt_SpellScript::HandleEffect, EFFECT_0, SPELL_EFFECT_APPLY_AURA);
                OnEffectHitTarget += SpellEffectFn(spell_mage_frostbolt_SpellScript::HandleEffect, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
                OnEffectHitTarget += SpellEffectFn(spell_mage_frostbolt_SpellScript::HandleEffect, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
                OnEffectHitTarget += SpellEffectFn(spell_mage_frostbolt_SpellScript::HandleEffect, EFFECT_3, SPELL_EFFECT_APPLY_AURA);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_frostbolt_SpellScript();
        }
};

// Called by Evocation - 12051
// Invocation - 114003
class spell_mage_invocation : public SpellScriptLoader
{
    public:
        spell_mage_invocation() : SpellScriptLoader("spell_mage_invocation") { }

        class spell_mage_invocation_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_invocation_AuraScript);

            void AfterRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                if (removeMode != AURA_REMOVE_BY_EXPIRE)
                    return;

                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAura(SPELL_MAGE_INVOCATION))
                    {
                        caster->CastSpell(caster, SPELL_MAGE_INVOKERS_ENERGY, true);

                        if (caster->HasAura(SPELL_MAGE_GLYPH_OF_EVOCATION))
                            caster->CastSpell(caster, 125440, true);
                    }
                }
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_mage_invocation_AuraScript::AfterRemove, EFFECT_0, SPELL_AURA_OBS_MOD_POWER, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_invocation_AuraScript();
        }
};

// Frost Bomb - 112948
class spell_mage_frost_bomb : public SpellScriptLoader
{
    public:
        spell_mage_frost_bomb() : SpellScriptLoader("spell_mage_frost_bomb") { }

        class spell_mage_frost_bomb_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_frost_bomb_AuraScript);

            void AfterRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                if (removeMode != AURA_REMOVE_BY_EXPIRE)
                    return;

                if (Unit* caster = GetCaster())
                {
                    caster->CastSpell(GetTarget(), SPELL_MAGE_FROST_BOMB_TRIGGERED, true);

                    if (caster->HasAura(SPELL_MAGE_BRAIN_FREEZE))
                        caster->CastSpell(caster, SPELL_MAGE_BRAIN_FREEZE_TRIGGERED, true);
                }
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_mage_frost_bomb_AuraScript::AfterRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_frost_bomb_AuraScript();
        }
};

class CheckNetherImpactPredicate
{
    public:
        CheckNetherImpactPredicate(Unit* caster, Unit* mainTarget) : _caster(caster), _mainTarget(mainTarget) {}

        bool operator()(Unit* target)
        {
            if (!_caster || !_mainTarget)
                return true;

            if (!_caster->IsValidAttackTarget(target))
                return true;

            if (!target->IsWithinLOSInMap(_caster))
                return true;

            if (!_caster->isInFront(target))
                return true;

            if (target->GetGUID() == _caster->GetGUID())
                return true;

            if (target->GetGUID() == _mainTarget->GetGUID())
                return true;

            return false;
        }

    private:
        Unit* _caster;
        Unit* _mainTarget;
};

// Nether Tempest - 114923
class spell_mage_nether_tempest : public SpellScriptLoader
{
    public:
        spell_mage_nether_tempest() : SpellScriptLoader("spell_mage_nether_tempest") { }

        class spell_mage_nether_tempest_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_nether_tempest_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* pCaster = GetCaster())
                {
                    std::list<Unit*> targetList;

                    GetTarget()->GetAttackableUnitListInRange(targetList, 10.0f);
                    targetList.remove_if(CheckNetherImpactPredicate(pCaster, GetTarget()));

                    Trinity::Containers::RandomResizeList(targetList, 1);

                    for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                    {
                        GetCaster()->CastSpell(*itr, SPELL_MAGE_NETHER_TEMPEST_DIRECT_DAMAGE, true);
                        GetCaster()->CastSpell(*itr, SPELL_MAGE_NETHER_TEMPEST_VISUAL, true);
                        GetTarget()->CastSpell(*itr, SPELL_MAGE_NETHER_TEMPEST_MISSILE, true);
                    }

                    if (GetCaster()->HasAura(SPELL_MAGE_BRAIN_FREEZE))
                        if (roll_chance_i(10))
                            GetCaster()->CastSpell(GetCaster(), SPELL_MAGE_BRAIN_FREEZE_TRIGGERED, true);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_mage_nether_tempest_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_nether_tempest_AuraScript();
        }
};

// Blazing Speed - 108843
class spell_mage_blazing_speed : public SpellScriptLoader
{
    public:
        spell_mage_blazing_speed() : SpellScriptLoader("spell_mage_blazing_speed") { }

        class spell_mage_blazing_speed_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_blazing_speed_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    _player->RemoveMovementImpairingAuras();
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_blazing_speed_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_blazing_speed_SpellScript();
        }
};

// Frostjaw - 102051
class spell_mage_frostjaw : public SpellScriptLoader
{
    public:
        spell_mage_frostjaw() : SpellScriptLoader("spell_mage_frostjaw") { }

        class spell_mage_frostjaw_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_frostjaw_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (target->GetTypeId() == TYPEID_PLAYER)
                        {
                            if (Aura* frostjaw = target->GetAura(SPELL_MAGE_FROSTJAW, _player->GetGUID()))
                            {
                                // Only half time against players
                                frostjaw->SetDuration(frostjaw->GetMaxDuration() / 2);
                                frostjaw->SetMaxDuration(frostjaw->GetDuration());
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_frostjaw_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_frostjaw_SpellScript();
        }
};

// Combustion - 11129
class spell_mage_combustion : public SpellScriptLoader
{
    public:
        spell_mage_combustion() : SpellScriptLoader("spell_mage_combustion") { }

        class spell_mage_combustion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_combustion_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        _player->CastSpell(target, SPELL_MAGE_COMBUSTION_IMPACT, true);
                        _player->RemoveSpellCooldown(SPELL_MAGE_INFERNO_BLAST, true);
                        _player->RemoveSpellCooldown(SPELL_MAGE_INFERNO_BLAST_IMPACT, true);

                        int32 combustionBp = 0;
                        int32 percent = 20;
                        if(_player->HasAura(56368))
                            percent += 20;

                        if (AuraEffect const* aurEff = target->GetAuraEffect(SPELL_MAGE_IGNITE, EFFECT_0))
                            combustionBp += CalculatePct(aurEff->GetAmount(), percent);

                        if (combustionBp)
                            _player->CastCustomSpell(target, SPELL_MAGE_COMBUSTION_DOT, &combustionBp, NULL, NULL, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_combustion_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_combustion_SpellScript();
        }
};

class CheckInfernoBlastImpactPredicate
{
    public:
        CheckInfernoBlastImpactPredicate(Unit* caster, Unit* mainTarget) : _caster(caster), _mainTarget(mainTarget) {}

        bool operator()(Unit* target)
        {
            if (!_caster || !_mainTarget)
                return true;

            if (!_caster->IsValidAttackTarget(target))
                return true;

            if (!target->IsWithinLOSInMap(_caster))
                return true;

            if (!_caster->isInFront(target))
                return true;

            if (target->GetGUID() == _caster->GetGUID())
                return true;

            if (target->GetGUID() == _mainTarget->GetGUID())
                return true;

            return false;
        }

    private:
        Unit* _caster;
        Unit* _mainTarget;
};

// Inferno Blast - 108853
class spell_mage_inferno_blast : public SpellScriptLoader
{
    public:
        spell_mage_inferno_blast() : SpellScriptLoader("spell_mage_inferno_blast") { }

        class spell_mage_inferno_blast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_inferno_blast_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        std::list<Unit*> targetList;

                        if (target->HasAura(SPELL_MAGE_IGNITE, _player->GetGUID()) 
                        || target->HasAura(SPELL_MAGE_PYROBLAST, _player->GetGUID())
                        || target->HasAura(SPELL_MAGE_COMBUSTION_DOT, _player->GetGUID()))
                            _player->CastSpell(target, SPELL_MAGE_INFERNO_BLAST_IMPACT, true);

                        // Spreads any Pyroblast, Ignite, and Combustion effects to up to 2 nearby enemy targets within 10 yards

                        target->GetAttackableUnitListInRange(targetList, 10.0f);

                        targetList.remove_if(CheckInfernoBlastImpactPredicate(_player, target));

                        if (targetList.size() > 2)
                            Trinity::Containers::RandomResizeList(targetList, 2);

                        for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                        {
                            Unit* unit = *itr;
                            // 1 : Ignite
                            if (target->HasAura(SPELL_MAGE_IGNITE, _player->GetGUID()))
                            {
                                float value = _player->GetFloatValue(PLAYER_MASTERY) * 1.5f / 100.0f;

                                int32 igniteBp = 0;

                                if (unit->HasAura(SPELL_MAGE_IGNITE, _player->GetGUID()))
                                    igniteBp += unit->GetRemainingPeriodicAmount(_player->GetGUID(), SPELL_MAGE_IGNITE, SPELL_AURA_PERIODIC_DAMAGE);

                                igniteBp += int32(GetHitDamage() * value / 2);

                                _player->CastCustomSpell(unit, SPELL_MAGE_IGNITE, &igniteBp, NULL, NULL, true);
                            }

                            // 2 : Pyroblast
                            if (target->HasAura(SPELL_MAGE_PYROBLAST, _player->GetGUID()))
                                _player->AddAura(SPELL_MAGE_PYROBLAST, unit);

                            // 3 : Combustion
                            if (target->HasAura(SPELL_MAGE_COMBUSTION_DOT, _player->GetGUID()))
                            {
                                int32 combustionBp = 0;
                                if (unit->HasAura(SPELL_MAGE_IGNITE, _player->GetGUID()))
                                    combustionBp += unit->GetRemainingPeriodicAmount(_player->GetGUID(), SPELL_MAGE_IGNITE, SPELL_AURA_PERIODIC_DAMAGE);

                                if (combustionBp)
                                {
                                    //combustionBp = int32(combustionBp / targetList.size());
                                    _player->CastCustomSpell(unit, SPELL_MAGE_COMBUSTION_DOT, &combustionBp, NULL, NULL, true);
                                }
                            }
                            // 4 : Living Bomb
                            if (_player->HasAura(89926))
                            {
                                if (target->HasAura(44457, _player->GetGUID()))
                                {
                                    _player->AddAura(44457, unit);
                                }
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_inferno_blast_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_inferno_blast_SpellScript();
        }
};

// Arcane Brillance - 1459
class spell_mage_arcane_brilliance : public SpellScriptLoader
{
    public:
        spell_mage_arcane_brilliance() : SpellScriptLoader("spell_mage_arcane_brilliance") { }

        class spell_mage_arcane_brilliance_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_arcane_brilliance_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    _player->AddAura(SPELL_MAGE_ARCANE_BRILLIANCE, _player);

                    std::list<Unit*> memberList;
                    _player->GetPartyMembers(memberList);

                    for (std::list<Unit*>::const_iterator itr = memberList.begin(); itr != memberList.end(); ++itr)
                        _player->AddAura(SPELL_MAGE_ARCANE_BRILLIANCE, *itr);
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_arcane_brilliance_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_arcane_brilliance_SpellScript();
        }
};

// Conjure Refreshment - 42955
class spell_mage_conjure_refreshment : public SpellScriptLoader
{
    public:
        spell_mage_conjure_refreshment() : SpellScriptLoader("spell_mage_conjure_refreshment") { }

        class spell_mage_conjure_refreshment_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_conjure_refreshment_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->getLevel() < 44)
                        _player->CastSpell(_player, SPELL_MAGE_CONJURE_REFRESHMENT_R1, true);
                    else if (_player->getLevel() < 54)
                        _player->CastSpell(_player, SPELL_MAGE_CONJURE_REFRESHMENT_R2, true);
                    else if (_player->getLevel() < 64)
                        _player->CastSpell(_player, SPELL_MAGE_CONJURE_REFRESHMENT_R3, true);
                    else if (_player->getLevel() < 74)
                        _player->CastSpell(_player, SPELL_MAGE_CONJURE_REFRESHMENT_R4, true);
                    else if (_player->getLevel() < 80)
                        _player->CastSpell(_player, SPELL_MAGE_CONJURE_REFRESHMENT_R5, true);
                    else if (_player->getLevel() < 85)
                        _player->CastSpell(_player, SPELL_MAGE_CONJURE_REFRESHMENT_R6, true);
                    else if (_player->getLevel() < 90)
                        _player->CastSpell(_player, SPELL_MAGE_CONJURE_REFRESHMENT_R7, true);
                    else
                        _player->CastSpell(_player, SPELL_MAGE_CONJURE_REFRESHMENT_R8, true);
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_conjure_refreshment_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_conjure_refreshment_SpellScript();
        }
};

// Conjure Refreshment Table - 43987
class spell_mage_conjure_refreshment_table : public SpellScriptLoader
{
    public:
        spell_mage_conjure_refreshment_table() : SpellScriptLoader("spell_mage_conjure_refreshment_table") { }

        class spell_mage_conjure_refreshment_table_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_conjure_refreshment_table_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->getLevel() < 80)
                        _player->CastSpell(_player, 120056, true);  // Conjure Refreshment Table
                    else if (_player->getLevel() < 85)
                        _player->CastSpell(_player, 120055, true);  // Conjure Refreshment Table
                    else if (_player->getLevel() < 90)
                        _player->CastSpell(_player, 120054, true);  // Conjure Refreshment Table
                    else
                        _player->CastSpell(_player, 120053, true);  // Conjure Refreshment Table
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_conjure_refreshment_table_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_conjure_refreshment_table_SpellScript();
        }
};

// Time Warp - 80353
class spell_mage_time_warp : public SpellScriptLoader
{
    public:
        spell_mage_time_warp() : SpellScriptLoader("spell_mage_time_warp") { }

        class spell_mage_time_warp_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_time_warp_SpellScript);

            void RemoveInvalidTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if(Trinity::UnitAuraCheck(true, HUNTER_SPELL_INSANITY));
                targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_SHAMAN_EXHAUSTED));
                targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_SHAMAN_SATED));
                targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_MAGE_TEMPORAL_DISPLACEMENT));
                targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_DRUMS_OF_RAGE));
            }

            void ApplyDebuff()
            {
                if (Unit* target = GetHitUnit())
                    target->CastSpell(target, SPELL_MAGE_TEMPORAL_DISPLACEMENT, true);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_time_warp_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_time_warp_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_CASTER_AREA_RAID);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_time_warp_SpellScript::RemoveInvalidTargets, EFFECT_2, TARGET_UNIT_CASTER_AREA_RAID);
                AfterHit += SpellHitFn(spell_mage_time_warp_SpellScript::ApplyDebuff);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_time_warp_SpellScript();
        }
};

// Alter Time - 127140 (overrided)
class spell_mage_alter_time_overrided : public SpellScriptLoader
{
    public:
        spell_mage_alter_time_overrided() : SpellScriptLoader("spell_mage_alter_time_overrided") { }

        class spell_mage_alter_time_overrided_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_alter_time_overrided_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_MAGE_ALTER_TIME_OVERRIDED))
                    return false;
                return true;
            }

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    _player->RemoveAurasDueToSpell(SPELL_MAGE_ALTER_TIME);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_mage_alter_time_overrided_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_alter_time_overrided_SpellScript();
        }
};

struct auraData
{
    auraData(uint32 id, int32 duration) : m_id(id), m_duration(duration) {}
    uint32 m_id;
    int32 m_duration;
};

// Alter Time - 110909
class spell_mage_alter_time : public SpellScriptLoader
{
    public:
        spell_mage_alter_time() : SpellScriptLoader("spell_mage_alter_time") { }

        class spell_mage_alter_time_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_alter_time_AuraScript);

            int32 mana;
            int32 health;
            float posX;
            float posY;
            float posZ;
            float orientation;
            uint32 map;
            std::set<auraData*> auras;

        public:
            spell_mage_alter_time_AuraScript() : AuraScript()
            {
                map = uint32(-1);
            }

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Player* _player = GetTarget()->ToPlayer())
                {
                    posX = _player->GetPositionX();
                    posY = _player->GetPositionY();
                    posZ = _player->GetPositionZ();
                    orientation = _player->GetOrientation();
                    map = _player->GetMapId();
                    Unit::AuraApplicationMap const& appliedAuras = _player->GetAppliedAuras();
                    for (Unit::AuraApplicationMap::const_iterator itr = appliedAuras.begin(); itr != appliedAuras.end(); ++itr)
                    {
                        if (Aura* aura = itr->second->GetBase())
                        {
                            SpellInfo const* auraInfo = aura->GetSpellInfo();

                            if (auraInfo->Attributes & SPELL_ATTR0_HIDDEN_CLIENTSIDE)
                                continue;

                            if (auraInfo->Attributes & SPELL_ATTR0_PASSIVE)
                                continue;

                            if (aura->IsArea() && aura->GetCasterGUID() != _player->GetGUID())
                                continue;

                            if (auraInfo->Id == SPELL_MAGE_ALTER_TIME)
                                continue;

                            auras.insert(new auraData(auraInfo->Id, aura->GetDuration()));
                        }
                    }
                    mana = _player->GetPower(POWER_MANA);
                    health = _player->GetHealth();
                }
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (map == uint32(-1))
                    return;

                if (Player* _player = GetTarget()->ToPlayer())
                {
                    if (_player->GetMapId() != map)
                        return;

                    for (std::set<auraData*>::const_iterator itr = auras.begin(); itr != auras.end(); ++itr)
                    {
                        Aura* aura = !_player->HasAura((*itr)->m_id) ? _player->AddAura((*itr)->m_id, _player) : _player->GetAura((*itr)->m_id);
                        if (aura)
                        {
                            aura->SetDuration((*itr)->m_duration);
                            aura->SetNeedClientUpdateForTargets();
                        }

                        delete *itr;
                    }

                    auras.clear();

                    _player->SetPower(POWER_MANA, mana);
                    _player->SetHealth(health);
                    _player->TeleportTo(map, posX, posY, posZ, orientation);
                }
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_mage_alter_time_AuraScript::OnApply, EFFECT_0, SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_mage_alter_time_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_alter_time_AuraScript();
        }
};

class spell_mage_cold_snap : public SpellScriptLoader
{
    public:
        spell_mage_cold_snap() : SpellScriptLoader("spell_mage_cold_snap") { }

        class spell_mage_cold_snap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_cold_snap_SpellScript);

            bool Load()
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                // immediately finishes the cooldown on Frost spells
                if(Player* caster = GetCaster()->ToPlayer())
                {
                    caster->RemoveSpellCooldown(120, true);
                    caster->RemoveSpellCooldown(122, true);
                    caster->RemoveSpellCooldown(45438, true);
                }
            }

            void Register()
            {
                // add dummy effect spell handler to Cold Snap
                OnEffectHit += SpellEffectFn(spell_mage_cold_snap_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_cold_snap_SpellScript();
        }
};

enum SilvermoonPolymorph
{
    NPC_AUROSALIA   = 18744,
};

// TODO: move out of here and rename - not a mage spell
class spell_mage_polymorph_cast_visual : public SpellScriptLoader
{
    public:
        spell_mage_polymorph_cast_visual() : SpellScriptLoader("spell_mage_polymorph_visual") { }

        class spell_mage_polymorph_cast_visual_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_polymorph_cast_visual_SpellScript);

            static const uint32 PolymorhForms[6];

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                // check if spell ids exist in dbc
                for (uint32 i = 0; i < 6; i++)
                    if (!sSpellMgr->GetSpellInfo(PolymorhForms[i]))
                        return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetCaster()->FindNearestCreature(NPC_AUROSALIA, 30.0f))
                    if (target->GetTypeId() == TYPEID_UNIT)
                        target->CastSpell(target, PolymorhForms[urand(0, 5)], true);
            }

            void Register()
            {
                // add dummy effect spell handler to Polymorph visual
                OnEffectHitTarget += SpellEffectFn(spell_mage_polymorph_cast_visual_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_polymorph_cast_visual_SpellScript();
        }
};

const uint32 spell_mage_polymorph_cast_visual::spell_mage_polymorph_cast_visual_SpellScript::PolymorhForms[6] =
{
    SPELL_MAGE_SQUIRREL_FORM,
    SPELL_MAGE_GIRAFFE_FORM,
    SPELL_MAGE_SERPENT_FORM,
    SPELL_MAGE_DRAGONHAWK_FORM,
    SPELL_MAGE_WORGEN_FORM,
    SPELL_MAGE_SHEEP_FORM
};

class spell_mage_incanters_absorbtion_base_AuraScript : public AuraScript
{
    public:
        enum Spells
        {
            SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED = 44413,
            SPELL_MAGE_INCANTERS_ABSORBTION_R1 = 44394,
        };

        bool Validate(SpellInfo const* /*spellEntry*/)
        {
            return sSpellMgr->GetSpellInfo(SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED)
                && sSpellMgr->GetSpellInfo(SPELL_MAGE_INCANTERS_ABSORBTION_R1);
        }

        void Trigger(AuraEffect* aurEff, DamageInfo & /*dmgInfo*/, uint32 & absorbAmount)
        {
            Unit* target = GetTarget();

            if (AuraEffect* talentAurEff = target->GetAuraEffectOfRankedSpell(SPELL_MAGE_INCANTERS_ABSORBTION_R1, EFFECT_0))
            {
                int32 bp = CalculatePct(absorbAmount, talentAurEff->GetAmount());
                target->CastCustomSpell(target, SPELL_MAGE_INCANTERS_ABSORBTION_TRIGGERED, &bp, NULL, NULL, true, NULL, aurEff);
            }
        }
};

// Incanter's Absorption
class spell_mage_incanters_absorbtion_absorb : public SpellScriptLoader
{
public:
    spell_mage_incanters_absorbtion_absorb() : SpellScriptLoader("spell_mage_incanters_absorbtion_absorb") { }

    class spell_mage_incanters_absorbtion_absorb_AuraScript : public spell_mage_incanters_absorbtion_base_AuraScript
    {
        PrepareAuraScript(spell_mage_incanters_absorbtion_absorb_AuraScript);

        void Register()
        {
             AfterEffectAbsorb += AuraEffectAbsorbFn(spell_mage_incanters_absorbtion_absorb_AuraScript::Trigger, EFFECT_0);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_mage_incanters_absorbtion_absorb_AuraScript();
    }
};

// Incanter's Absorption
class spell_mage_incanters_absorbtion_manashield : public SpellScriptLoader
{
public:
    spell_mage_incanters_absorbtion_manashield() : SpellScriptLoader("spell_mage_incanters_absorbtion_manashield") { }

    class spell_mage_incanters_absorbtion_manashield_AuraScript : public spell_mage_incanters_absorbtion_base_AuraScript
    {
        PrepareAuraScript(spell_mage_incanters_absorbtion_manashield_AuraScript);

        void Register()
        {
             AfterEffectManaShield += AuraEffectManaShieldFn(spell_mage_incanters_absorbtion_manashield_AuraScript::Trigger, EFFECT_0);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_mage_incanters_absorbtion_manashield_AuraScript();
    }
};

// Living Bomb - 44457
class spell_mage_living_bomb : public SpellScriptLoader
{
    public:
        spell_mage_living_bomb() : SpellScriptLoader("spell_mage_living_bomb") { }

        class spell_mage_living_bomb_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_living_bomb_AuraScript);

            void AfterApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                std::deque<uint64>::iterator curr = std::find(caster->m_livingBombTargets.begin(), caster->m_livingBombTargets.end(), target->GetGUID());
                if (curr != caster->m_livingBombTargets.end())
                    caster->m_livingBombTargets.erase(curr);

                // Living Bomb can only be at 3 targets at once
                while (caster->m_livingBombTargets.size() >= 3)
                {
                    if (Unit* toRemove = Unit::GetUnit(*caster, caster->m_livingBombTargets[0]))
                        toRemove->RemoveAurasDueToSpell(aurEff->GetId(), caster->GetGUID());

                    caster->m_livingBombTargets.pop_front();
                }

                caster->m_livingBombTargets.push_back(target->GetGUID());
            }
            
            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAura(SPELL_MAGE_BRAIN_FREEZE))
                        if (roll_chance_i(10))
                            caster->CastSpell(caster, SPELL_MAGE_BRAIN_FREEZE_TRIGGERED, true);
                }
            }

            void AfterRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                if (removeMode != AURA_REMOVE_BY_DEATH && removeMode != AURA_REMOVE_BY_EXPIRE)
                    return;

                if (Unit* caster = GetCaster())
                {
                    caster->CastSpell(GetTarget(), SPELL_MAGE_LIVING_BOMB_TRIGGERED, true);
                }
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_mage_living_bomb_AuraScript::AfterApply, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_mage_living_bomb_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                AfterEffectRemove += AuraEffectRemoveFn(spell_mage_living_bomb_AuraScript::AfterRemove, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_living_bomb_AuraScript();
        }
};

class spell_mage_frost_nova : public SpellScriptLoader
{
    public:
        spell_mage_frost_nova() : SpellScriptLoader("spell_mage_frost_nova") { }

        class spell_mage_frost_nova_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_frost_nova_SpellScript)

            bool Validate(SpellEntry const * /*spellEntry*/)
            {
                return true;
            }

            void HandleOnHit()
            {
                if (Unit * caster = GetCaster())
                {
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                    {
                        caster->ToPlayer()->KilledMonsterCredit(44175, 0);
                        caster->ToPlayer()->KilledMonsterCredit(44548, 0);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_frost_nova_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_frost_nova_SpellScript();
        }
};

// Greater Invisibility - 110960
class spell_mage_greater_invisibility : public SpellScriptLoader
{
    public:
        spell_mage_greater_invisibility() : SpellScriptLoader("spell_mage_greater_invisibility") { }

        class spell_mage_greater_invisibility_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_greater_invisibility_AuraScript);

            void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    int32 count = 0;
                    Unit::AuraEffectList const mPeriodic = caster->GetAuraEffectsByType(SPELL_AURA_PERIODIC_DAMAGE);
                    for (Unit::AuraEffectList::const_iterator iter = mPeriodic.begin(); iter != mPeriodic.end(); ++iter)
                    {
                        if (!(*iter)) // prevent crash
                            continue;
                        Aura* aura = (*iter)->GetBase();
                        aura->Remove();
                        count++;
                        if(count > 1)
                            return;
                    }
                    caster->CastSpell(GetTarget(), 113862, true, NULL, aurEff);
                }
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (Aura* aura = GetTarget()->GetAura(113862, caster->GetGUID()))
                    {
                        aura->SetDuration(3000);
                        aura->SetMaxDuration(3000);
                    }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_mage_greater_invisibility_AuraScript::OnApply, EFFECT_1, SPELL_AURA_MOD_INVISIBILITY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_mage_greater_invisibility_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_MOD_INVISIBILITY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_greater_invisibility_AuraScript();
        }
};

// Ice Block - 45438
class spell_mage_ice_block : public SpellScriptLoader
{
    public:
        spell_mage_ice_block() : SpellScriptLoader("spell_mage_ice_block") { }

        class spell_mage_ice_block_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_ice_block_AuraScript);

            void AfterRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                if (removeMode == AURA_REMOVE_BY_DEATH)
                    return;

                Unit* target = GetTarget();
                // Glyph of Ice Block
                if (target->HasAura(115723))
                {
                    // Glyph of Ice Block
                    target->CastSpell(target, 115760, true);
                    // Frost Nova
                    target->CastSpell(target, 115757, true);
                }
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_mage_ice_block_AuraScript::AfterRemove, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_ice_block_AuraScript();
        }
};

// Invisibility - 32612
// Greater Invisibility - 110960
class spell_elem_invisibility : public SpellScriptLoader
{
    public:
        spell_elem_invisibility() : SpellScriptLoader("spell_elem_invisibility") { }

        class spell_elem_invisibility_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_elem_invisibility_AuraScript);

        public:
 
            void OnApply(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                if (target->GetOwnerGUID())
                    return;

                if (Guardian* pet = target->GetGuardianPet())
                    pet->CastSpell(pet, 96243, true);
            }

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                if (target->GetOwnerGUID())
                    return;

                if (Guardian* pet = target->GetGuardianPet())
                    pet->RemoveAurasDueToSpell(96243);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_elem_invisibility_AuraScript::OnApply, EFFECT_1, SPELL_AURA_MOD_INVISIBILITY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_elem_invisibility_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_MOD_INVISIBILITY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_elem_invisibility_AuraScript();
        }
};

// Arcane Blast - 30451
class spell_mage_arcane_blast : public SpellScriptLoader
{
    public:
        spell_mage_arcane_blast() : SpellScriptLoader("spell_mage_arcane_blast") { }

        class spell_mage_arcane_blast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_arcane_blast_SpellScript);

            void HandleAfterHit()
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                caster->CastSpell(caster, GetSpellInfo()->Effects[1].TriggerSpell, true);
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_mage_arcane_blast_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_arcane_blast_SpellScript();
        }
};

// Ring of Frost - 82691
class spell_mage_ring_of_frost : public SpellScriptLoader
{
    public:
        spell_mage_ring_of_frost() : SpellScriptLoader("spell_mage_ring_of_frost") { }

        class spell_mage_ring_of_frost_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_ring_of_frost_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (Unit* caster = GetCaster())
                for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end();)
                {
                    if (Unit* target = (*itr)->ToUnit())
                    {
                        if(target->IsImmunedToSpell(GetSpellInfo()))
                        {
                            targets.erase(itr++);
                            continue;
                        }
                        DiminishingLevels m_diminishLevel = target->GetDiminishing(DIMINISHING_DISORIENT);
                        Position const* pos = GetExplTargetDest();
                        if(m_diminishLevel == DIMINISHING_LEVEL_IMMUNE)
                            target->AddAura(91264, target);
                        else if (target->GetDistance2d(pos->GetPositionX(), pos->GetPositionY()) < 3.5f)
                        {
                            targets.erase(itr++);
                            continue;
                        }
                        else if (!target->HasAura(82691) && !target->HasAura(91264))
                        {
                            ++itr;
                            continue;
                        }
                    }
                    targets.erase(itr++);
                }
                if(targets.size() > 10)
                    Trinity::Containers::RandomResizeList(targets, 10);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_ring_of_frost_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_ring_of_frost_SpellScript();
        }

        class spell_mage_ring_of_frost_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_ring_of_frost_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                    target->AddAura(91264, target);
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_mage_ring_of_frost_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_MOD_STUN, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_ring_of_frost_AuraScript();
        }
};

// Ring of Frost - 136511
class spell_mage_ring_of_frost_tick : public SpellScriptLoader
{
    public:
        spell_mage_ring_of_frost_tick() : SpellScriptLoader("spell_mage_ring_of_frost_tick") { }

        class spell_mage_ring_of_frost_tick_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_ring_of_frost_tick_AuraScript);

            bool Load()
            {
                find = false;
                return true;
            }

            void OnTick(AuraEffect const* aurEff)
            {
                PreventDefaultAction();
                if (Unit* caster = GetCaster())
                {
                    if (!find)
                    {
                        if (Creature* mob = caster->FindNearestCreature(44199, 100.0f))
                        {
                            mob->GetPosition(x, y, z);
                            find = true;
                        }
                    }
                    if (find)
                    {
                        caster->CastSpell(x, y, z, 82691, true);
                    }
                }
            }

            float x, y, z;
            bool find;
            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_mage_ring_of_frost_tick_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_ring_of_frost_tick_AuraScript();
        }
};

// Icicle - 148023 (periodic dummy)
class spell_mage_icicle : public SpellScriptLoader
{
    public:
        spell_mage_icicle() : SpellScriptLoader("spell_mage_icicle") { }

        class spell_mage_icicle_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_icicle_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* caster = GetCaster())
                if (Player* _player = caster->ToPlayer())
                if (Unit* target = _player->GetSelectedUnit())
                {
                    switch(aurEff->GetTickNumber())
                    {
                        case 1:
                        {
                            if (Aura* icicle = caster->GetAura(148012))
                            {
                                int32 tickamount = icicle->GetEffect(EFFECT_0)->GetAmount();
                                caster->CastSpell(target, 148017, true);
                                caster->CastCustomSpell(target, 148022, &tickamount, NULL, NULL, true);
                                icicle->Remove();
                            }
                            break;
                        }
                        case 2:
                        {
                            if (Aura* icicle = caster->GetAura(148013))
                            {
                                int32 tickamount = icicle->GetEffect(EFFECT_0)->GetAmount();
                                caster->CastSpell(target, 148018, true);
                                caster->CastCustomSpell(target, 148022, &tickamount, NULL, NULL, true);
                                icicle->Remove();
                            }
                            break;
                        }
                        case 3:
                        {
                            if (Aura* icicle = caster->GetAura(148014))
                            {
                                int32 tickamount = icicle->GetEffect(EFFECT_0)->GetAmount();
                                caster->CastSpell(target, 148019, true);
                                caster->CastCustomSpell(target, 148022, &tickamount, NULL, NULL, true);
                                icicle->Remove();
                            }
                            break;
                        }
                        case 4:
                        {
                            if (Aura* icicle = caster->GetAura(148015))
                            {
                                int32 tickamount = icicle->GetEffect(EFFECT_0)->GetAmount();
                                caster->CastSpell(target, 148020, true);
                                caster->CastCustomSpell(target, 148022, &tickamount, NULL, NULL, true);
                                icicle->Remove();
                            }
                            break;
                        }
                        case 5:
                        {
                            if (Aura* icicle = caster->GetAura(148016))
                            {
                                int32 tickamount = icicle->GetEffect(EFFECT_0)->GetAmount();
                                caster->CastSpell(target, 148021, true);
                                caster->CastCustomSpell(target, 148022, &tickamount, NULL, NULL, true);
                                icicle->Remove();
                            }
                            break;
                        }
                        default:
                            GetAura()->Remove();
                            break;
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_mage_icicle_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_icicle_AuraScript();
        }
};

//Illusion glyph - 131784
class spell_mage_illusion : public SpellScriptLoader
{
    public:
        spell_mage_illusion() : SpellScriptLoader("spell_mage_illusion") { }

        class spell_mage_illusion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_illusion_SpellScript)

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    /*if(Unit* target = _player->GetSelectedUnit())
                    {
                        if (target->GetTypeId() == TYPEID_PLAYER && target != GetCaster())
                        {
                            target->CastSpell(_player, 80396, true);
                            return;
                        }
                    }*/
                    _player->CastSpell(_player, 94632, true);
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_illusion_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_illusion_SpellScript();
        }
};

// Flameglow - 140468
class spell_mage_flameglow : public SpellScriptLoader
{
    public:
        spell_mage_flameglow() : SpellScriptLoader("spell_mage_flameglow") { }

        class spell_mage_flameglow_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_mage_flameglow_AuraScript);

            uint32 absorb;
            uint32 LimitAbsorb;
            uint32 TakenDamage;

            bool Load()
            {
                absorb = 0;
                LimitAbsorb = 0;
                TakenDamage = 0;
                return true;
            }

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                amount = -1;                
            }

            void Absorb(AuraEffect* /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
            {
                absorb = GetCaster()->GetSpellPowerDamage() * 15 / 100;
                LimitAbsorb = GetSpellInfo()->Effects[EFFECT_2].BasePoints;
                
                absorbAmount = CalculatePct(dmgInfo.GetDamage(), LimitAbsorb);
                if (absorbAmount > absorb)
                    absorbAmount = absorb;
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_mage_flameglow_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_mage_flameglow_AuraScript::Absorb, EFFECT_0);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_mage_flameglow_AuraScript();
        }
};

// Glyph of Icy Veins - 56364
class spell_mage_glyph_of_icy_veins : public SpellScriptLoader
{
    public:
        spell_mage_glyph_of_icy_veins() : SpellScriptLoader("spell_mage_glyph_of_icy_veins") { }

        class spell_mage_glyph_of_icy_veins_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_glyph_of_icy_veins_SpellScript)

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        int32 _damage = GetHitDamage();
                        if (GetSpellInfo()->Id == SPELL_MAGE_ICE_LANCE)
                        {
                            if (target != GetExplTargetUnit())
                            {
                                _damage = int32(_damage * 0.25f);
                                SetHitDamage(_damage);
                            }
                        }
                        if (caster->HasAura(SPELL_MAGE_ICY_VEINS))
                        {
                            if (GetSpellInfo()->Id == SPELL_MAGE_FROSTBOLT)
                            {    
                                caster->CastSpell(target, 131079, true);
                                caster->CastSpell(target, 131079, true);
                            }

                            if (GetSpellInfo()->Id == SPELL_MAGE_ICE_LANCE)
                            {
                                caster->CastSpell(target, 131080, true);
                                caster->CastSpell(target, 131080, true);
                            }

                            if (GetSpellInfo()->Id == SPELL_MAGE_FROSTFIRE_BOLT)
                            {
                                caster->CastSpell(target, 131081, true);
                                caster->CastSpell(target, 131081, true);
                            }
                        }
                        // Icicle
                        if (caster->HasAura(148016))
                        {
                            if (GetSpellInfo()->Id == SPELL_MAGE_FROSTBOLT || GetSpellInfo()->Id == SPELL_MAGE_FROSTFIRE_BOLT)
                            {
                                int32 visual = 148017;
                                Aura* icicle2 = caster->GetAura(148012);
                                Aura* icicle3 = caster->GetAura(148013);
                                Aura* icicle4 = caster->GetAura(148014);
                                Aura* icicle5 = caster->GetAura(148015);
                                Aura* icicle6 = caster->GetAura(148016);

                                if(icicle2 && icicle3 && icicle4 && icicle5 && icicle6)
                                {
                                    Aura* icicle = icicle2;
                                    if(icicle->GetDuration() > icicle3->GetDuration())
                                    {
                                        icicle = icicle3;
                                        visual = 148018;
                                    }
                                    if(icicle->GetDuration() > icicle4->GetDuration())
                                    {
                                        icicle = icicle4;
                                        visual = 148019;
                                    }
                                    if(icicle->GetDuration() > icicle5->GetDuration())
                                    {
                                        icicle = icicle5;
                                        visual = 148020;
                                    }
                                    if(icicle->GetDuration() > icicle6->GetDuration())
                                    {
                                        icicle = icicle6;
                                        visual = 148021;
                                    }

                                    int32 tickamount = icicle->GetEffect(EFFECT_0)->GetAmount();
                                    caster->CastSpell(target, visual, true);
                                    caster->CastCustomSpell(target, 148022, &tickamount, NULL, NULL, true);
                                    icicle->Remove();
                                }
                            }
                        }
                        if (Unit* owner = caster->GetOwner())
                        {
                            if (GetSpellInfo()->Id == SPELL_MAGE_WATERBOLT)
                            {
                                if (owner->HasAura(SPELL_MAGE_ICY_VEINS))
                                {
                                    caster->CastSpell(target, 131581, true);
                                    caster->CastSpell(target, 131581, true);
                                }
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_mage_glyph_of_icy_veins_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_glyph_of_icy_veins_SpellScript();
        }
};

// Glyph of Conjure Familiar - 126748
enum CreateFamiliarStone
{
    ITEM_ICY_FAMILIAR_STONE_1 = 87259,
    ITEM_FIERY_FAMILIAR_STONE_2 = 87258,
    ITEM_ARCANE_FAMILIAR_STONE_3 = 87257,
};

class spell_mage_glyph_of_conjure_familiar : public SpellScriptLoader
{
    public:
        spell_mage_glyph_of_conjure_familiar() : SpellScriptLoader("spell_mage_glyph_of_conjure_familiar") { }

        class spell_mage_glyph_of_conjure_familiar_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_glyph_of_conjure_familiar_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (Player* target = GetHitPlayer())
                {
                    static const uint32 items[] = {ITEM_ICY_FAMILIAR_STONE_1, ITEM_FIERY_FAMILIAR_STONE_2, ITEM_ARCANE_FAMILIAR_STONE_3};
                    target->AddItem(items[urand(0, 2)], 1);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_mage_glyph_of_conjure_familiar_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_CREATE_ITEM_2);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_glyph_of_conjure_familiar_SpellScript();
        }
};

// Inferno Blast - 118280
class spell_mage_inferno_blast_impact : public SpellScriptLoader
{
    public:
        spell_mage_inferno_blast_impact() : SpellScriptLoader("spell_mage_inferno_blast_impact") { }

        class spell_mage_inferno_blast_impact_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_inferno_blast_impact_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                uint32 count = 3;
                if (Unit* caster = GetCaster())
                    if (Aura* aura = caster->GetAura(89926))
                    {
                        if(aura->GetEffect(EFFECT_0))
                            count += aura->GetEffect(EFFECT_0)->GetAmount();
                    }
                if(targets.size() > count)
                    Trinity::Containers::RandomResizeList(targets, count);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_mage_inferno_blast_impact_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_inferno_blast_impact_SpellScript();
        }
};

// Icicle - 148022
class spell_mage_icicle_damage : public SpellScriptLoader
{
    public:
        spell_mage_icicle_damage() : SpellScriptLoader("spell_mage_icicle_damage") { }

        class spell_mage_icicle_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_icicle_damage_SpellScript)

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetHitUnit())
                {
                    if (target != GetExplTargetUnit())
                        SetHitDamage(int32(GetHitDamage() * 0.5f));
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_mage_icicle_damage_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_icicle_damage_SpellScript();
        }
};

// Glyph of Icy Veins - 131079, 131080, 131081
class spell_mage_glyph_of_icy_veins_damage : public SpellScriptLoader
{
    public:
        spell_mage_glyph_of_icy_veins_damage() : SpellScriptLoader("spell_mage_glyph_of_icy_veins_damage") { }

        class spell_mage_glyph_of_icy_veins_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_mage_glyph_of_icy_veins_damage_SpellScript)

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetHitUnit())
                {
                    SetHitDamage(int32(GetHitDamage() * 0.4f));
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_mage_glyph_of_icy_veins_damage_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_mage_glyph_of_icy_veins_damage_SpellScript();
        }
};

void AddSC_mage_spell_scripts()
{
    new spell_mage_incanters_ward_cooldown();
    new spell_mage_incanters_ward();
    new spell_mage_arcane_missile();
    new spell_mage_cauterize();
    new spell_mage_pyromaniac();
    new spell_mage_arcane_barrage();
    new spell_mage_arcane_explosion();
    new spell_mage_slow();
    new spell_mage_frostbolt();
    new spell_mage_invocation();
    new spell_mage_frost_bomb();
    new spell_mage_nether_tempest();
    new spell_mage_blazing_speed();
    new spell_mage_frostjaw();
    new spell_mage_combustion();
    new spell_mage_inferno_blast();
    new spell_mage_arcane_brilliance();
    new spell_mage_conjure_refreshment();
    new spell_mage_conjure_refreshment_table();
    new spell_mage_time_warp();
    new spell_mage_alter_time_overrided();
    new spell_mage_alter_time();
    new spell_mage_cold_snap();
    new spell_mage_incanters_absorbtion_absorb();
    new spell_mage_incanters_absorbtion_manashield();
    new spell_mage_polymorph_cast_visual();
    new spell_mage_living_bomb();
    new spell_mage_frost_nova();
    new spell_mage_greater_invisibility();
    new spell_mage_ice_block();
    new spell_elem_invisibility();
    new spell_mage_arcane_blast();
    new spell_mage_ring_of_frost();
    new spell_mage_ring_of_frost_tick();
    new spell_mage_icicle();
    new spell_mage_illusion();
    new spell_mage_flameglow();
    new spell_mage_glyph_of_icy_veins();
    new spell_mage_glyph_of_conjure_familiar();
    new spell_mage_inferno_blast_impact();
    new spell_mage_icicle_damage();
    new spell_mage_glyph_of_icy_veins_damage();
}
