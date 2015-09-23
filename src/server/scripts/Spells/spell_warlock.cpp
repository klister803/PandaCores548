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
#include "CreatureAI.h"

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
    WARLOCK_DOOM                            = 603,
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
    WARLOCK_DARK_REGENERATION               = 108359,
    WARLOCK_DARK_BARGAIN_DOT                = 110914,
    WARLOCK_MOLTEN_CORE                     = 122355,
    WARLOCK_MOLTEN_CORE_AURA                = 122351,
    WARLOCK_WILD_IMP_SUMMON                 = 104317,
    WARLOCK_DEMONIC_CALL                    = 114925,
    WARLOCK_DECIMATE_AURA                   = 108869,
    WARLOCK_ARCHIMONDES_VENGEANCE_COOLDOWN  = 116405,
    WARLOCK_ARCHIMONDES_VENGEANCE_DAMAGE    = 124051,
    WARLOCK_ARCHIMONDES_VENGEANCE_PASSIVE   = 116403,
    WARLOCK_SOUL_LINK_DUMMY_AURA            = 108446,
    WARLOCK_GLYPH_OF_CONFLAGRATE            = 56235,
    WARLOCK_SHIELD_OF_SHADOW                = 115232,
    WARLOCK_THREATENING_PRESENCE            = 112042,
    WARLOCK_OVERRIDE_COMMAND_DEMON          = 119904,
};

// Voidwalker : Shield of Shadow - 103130
class spell_warl_shield_of_shadow : public SpellScriptLoader
{
    public:
        spell_warl_shield_of_shadow() : SpellScriptLoader("spell_warl_shield_of_shadow") { }

        class spell_warl_shield_of_shadow_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_shield_of_shadow_AuraScript);

            void OnUpdate(uint32 diff, AuraEffect* aurEff)
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Pet* pet = _player->GetPet())
                    {
                        if (pet->GetEntry() == 1860) // Voidwalker
                        {
                            if (!pet->HasSpell(WARLOCK_SHIELD_OF_SHADOW))
                                pet->addSpell(WARLOCK_SHIELD_OF_SHADOW);
                            if (!pet->HasSpell(WARLOCK_THREATENING_PRESENCE))
                                pet->addSpell(WARLOCK_THREATENING_PRESENCE);
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectUpdate += AuraEffectUpdateFn(spell_warl_shield_of_shadow_AuraScript::OnUpdate, EFFECT_0, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_shield_of_shadow_AuraScript();
        }
};

// Grimoire of Sacrifice - 108503
class spell_warl_grimoire_of_sacrifice : public SpellScriptLoader
{
    public:
        spell_warl_grimoire_of_sacrifice() : SpellScriptLoader("spell_warl_grimoire_of_sacrifice") { }

        class spell_warl_grimoire_of_sacrifice_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_grimoire_of_sacrifice_AuraScript);

            void CalculateAmount(AuraEffect const* aurEff, int32 & amount, bool & /*canBeRecalculated*/)
            {
                if(GetCaster())
                if (Player* player = GetCaster()->ToPlayer())
                {
                    // EFFECT_0 : Instakill
                    // EFFECT_1 : 2% health every 5s
                    // EFFECT_2 : +50% DOT damage for Malefic Grasp, Drain Life and Drain Soul
                    // EFFECT_3 : +30% damage for Shadow Bolt, Hand of Gul'Dan, Soul Fire, Wild Imps and Fel Flame
                    // EFFECT_4 : +25% damage for Incinerate, Conflagrate, Chaos Bolt, Shadowburn and Fel Flame
                    // EFFECT_5 : +50% damage for Fel Flame
                    // EFFECT_6 : +20% Health if Soul Link talent is also chosen
                    // EFFECT_7 : +50% on EFFECT_2 of Malefic Grasp
                    // EFFECT_8 : +50% on EFFECT_4 and EFFECT_5 of Drain Soul -> Always set to 0
                    // EFFECT_9 : Always set to 0
                    // EFFECT_10 : Always set to 0
                    // EFFECT_11 : Duration for HB
                    switch(aurEff->GetEffIndex())
                    {
                        case EFFECT_2:
                        case EFFECT_5:
                        case EFFECT_7:
                        case EFFECT_8:
                        {
                            amount = player->GetSpecializationId(player->GetActiveSpec()) == SPEC_WARLOCK_AFFLICTION ? amount : 0;
                            break;
                        }
                        case EFFECT_3:
                        {
                            amount = player->GetSpecializationId(player->GetActiveSpec()) == SPEC_WARLOCK_DEMONOLOGY ? amount : 0;
                            break;
                        }
                        case EFFECT_4:
                        {
                            amount = player->GetSpecializationId(player->GetActiveSpec()) == SPEC_WARLOCK_DESTRUCTION ? amount : 0;
                            break;
                        }
                        case EFFECT_9:
                        case EFFECT_10:
                        {
                            amount = 0;
                            break;
                        }
                        case EFFECT_6:
                        switch (player->GetSpecializationId(player->GetActiveSpec()))
                        {
                            case SPEC_NONE:
                                amount = 0;
                                break;
                        }
                        if (!player->HasSpell(108415))
                            amount = 0;
                        break;
                    }
                }
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_grimoire_of_sacrifice_AuraScript::CalculateAmount, EFFECT_2, SPELL_AURA_ADD_PCT_MODIFIER);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_grimoire_of_sacrifice_AuraScript::CalculateAmount, EFFECT_3, SPELL_AURA_ADD_PCT_MODIFIER);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_grimoire_of_sacrifice_AuraScript::CalculateAmount, EFFECT_4, SPELL_AURA_ADD_PCT_MODIFIER);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_grimoire_of_sacrifice_AuraScript::CalculateAmount, EFFECT_5, SPELL_AURA_ADD_PCT_MODIFIER);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_grimoire_of_sacrifice_AuraScript::CalculateAmount, EFFECT_6, SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_grimoire_of_sacrifice_AuraScript::CalculateAmount, EFFECT_7, SPELL_AURA_ADD_PCT_MODIFIER);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_grimoire_of_sacrifice_AuraScript::CalculateAmount, EFFECT_8, SPELL_AURA_ADD_PCT_MODIFIER);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_grimoire_of_sacrifice_AuraScript::CalculateAmount, EFFECT_9, SPELL_AURA_ADD_PCT_MODIFIER);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_grimoire_of_sacrifice_AuraScript::CalculateAmount, EFFECT_10, SPELL_AURA_ADD_PCT_MODIFIER);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_grimoire_of_sacrifice_AuraScript();
        }
};

// Flames of Xoroth - 120451
class spell_warl_flames_of_xoroth : public SpellScriptLoader
{
    public:
        spell_warl_flames_of_xoroth() : SpellScriptLoader("spell_warl_flames_of_xoroth") { }

        class spell_warl_flames_of_xoroth_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_flames_of_xoroth_SpellScript);

            bool Load()
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            SpellCastResult CheckPet()
            {
                if (!GetCaster())
                    return SPELL_FAILED_DONT_REPORT;

                Player* _plr = GetCaster()->ToPlayer();
                if (!_plr)
                    return SPELL_FAILED_DONT_REPORT;

                if (Pet* pet = _plr->GetPet())
                    return SPELL_FAILED_ALREADY_HAVE_PET;

                return SPELL_CAST_OK;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (!GetCaster())
                    return;

                Player* player = GetCaster()->ToPlayer();
                uint32 spellId = 0;
                
                switch(player->GetLastPetEntry())
                {
                    case   416: spellId = 95308; break;
                    case   417: spellId = 95310; break;
                    case  1860: spellId = 95307; break;
                    case  1863: spellId = 95309; break;
                    case 17252: spellId = 95312; break;
                    case 58959: spellId = player->HasAura(108499) ? 128036 : 95308; break;
                    case 58960: spellId = player->HasAura(108499) ? 128038 : 95307; break;
                    case 58963: spellId = player->HasAura(108499) ? 128039 : 95309; break;
                    case 58964: spellId = player->HasAura(108499) ? 128041 : 95310; break;
                    case 58965: spellId = player->HasAura(108499) ? 128042 : 95312; break;
                    default:
                        break;
                }

                if (spellId)
                    player->CastSpell(player, spellId, true);
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_warl_flames_of_xoroth_SpellScript::CheckPet);
                OnEffectHitTarget += SpellEffectFn(spell_warl_flames_of_xoroth_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_flames_of_xoroth_SpellScript();
        }
};

// Archimonde's Vengeance (Cooldown marker) - 108505
class spell_warl_archimondes_vengeance_cooldown : public SpellScriptLoader
{
    public:
        spell_warl_archimondes_vengeance_cooldown() : SpellScriptLoader("spell_warl_archimondes_vengeance_cooldown") { }

        class spell_warl_archimondes_vengeance_cooldown_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_archimondes_vengeance_cooldown_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (!_player->HasSpellCooldown(WARLOCK_ARCHIMONDES_VENGEANCE_PASSIVE))
                            _player->AddSpellCooldown(WARLOCK_ARCHIMONDES_VENGEANCE_PASSIVE, 0, getPreciseTime() + 120.0);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_archimondes_vengeance_cooldown_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_archimondes_vengeance_cooldown_SpellScript();
        }
};

// Archimonde's Vengeance - 108505
class spell_warl_archimondes_vengance : public SpellScriptLoader
{
    public:
        spell_warl_archimondes_vengance() : SpellScriptLoader("spell_warl_archimondes_vengance") { }

        class spell_warl_archimondes_vengance_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_archimondes_vengance_AuraScript);

            void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();

                if (!GetCaster())
                    return;

                if (!GetTarget())
                    return;

                std::list<Unit*> tempList;
                std::list<Unit*> targetList;
                Unit* target = NULL;

                GetCaster()->GetAttackableUnitListInRange(tempList, 100.0f);

                if (tempList.empty())
                    return;

                for (std::list<Unit*>::const_iterator itr = tempList.begin(); itr != tempList.end(); ++itr)
                {
                    if ((*itr)->GetGUID() == GetCaster()->GetGUID())
                        continue;

                    if ((*itr)->HasAura(aurEff->GetSpellInfo()->Id, GetCaster()->GetGUID()))
                        targetList.push_back(*itr);
                }

                if (targetList.empty())
                    return;

                if (targetList.size() > 1)
                    return;

                for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                    target = *itr;

                if (!target)
                    return;

                if (eventInfo.GetActor()->GetGUID() == GetTarget()->GetGUID())
                    return;

                if (!eventInfo.GetDamageInfo()->GetDamage())
                    return;

                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (target->HasAura(aurEff->GetSpellInfo()->Id, _player->GetGUID()))
                    {
                        int32 bp = int32(eventInfo.GetDamageInfo()->GetDamage() / 4);

                        if (!bp)
                            return;

                        _player->CastCustomSpell(target, WARLOCK_ARCHIMONDES_VENGEANCE_DAMAGE, &bp, NULL, NULL, true);
                    }
                }

                tempList.clear();
                targetList.clear();
            }

            void Register()
            {
                OnEffectProc += AuraEffectProcFn(spell_warl_archimondes_vengance_AuraScript::OnProc, EFFECT_1, SPELL_AURA_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_archimondes_vengance_AuraScript();
        }
};

// Called by Shadowflame - 47960
// Molten Core - 122351
class spell_warl_molten_core_dot : public SpellScriptLoader
{
    public:
        spell_warl_molten_core_dot() : SpellScriptLoader("spell_warl_molten_core_dot") { }

        class spell_warl_molten_core_dot_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_molten_core_dot_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (GetCaster())
                {
                    if (GetCaster()->HasAura(WARLOCK_MOLTEN_CORE_AURA))
                        if (roll_chance_i(8))
                            GetCaster()->CastSpell(GetCaster(), WARLOCK_MOLTEN_CORE, true);

                    GetCaster()->EnergizeBySpell(GetCaster(), aurEff->GetSpellInfo()->Id, 2, POWER_DEMONIC_FURY);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_molten_core_dot_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_molten_core_dot_AuraScript();
        }
};

// Void Ray - 115422 and Touch of Chaos - 103964
class spell_warl_void_ray : public SpellScriptLoader
{
    public:
        spell_warl_void_ray() : SpellScriptLoader("spell_warl_void_ray") { }

        class spell_warl_void_ray_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_void_ray_SpellScript);

            void HandleOnHit()
            {
                if (Unit* _caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (Aura* corruption = target->GetAura(146739))
                            corruption->SetDuration(corruption->GetSpellInfo()->GetMaxDuration(), true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_void_ray_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_void_ray_SpellScript();
        }
};

// Chaos Wave - 124916
class spell_warl_chaos_wave : public SpellScriptLoader
{
    public:
        spell_warl_chaos_wave() : SpellScriptLoader("spell_warl_chaos_wave") { }

        class spell_warl_chaos_wave_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_chaos_wave_SpellScript);

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    _player->CastSpell(_player, WARLOCK_MOLTEN_CORE, true);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_warl_chaos_wave_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_chaos_wave_SpellScript();
        }
};

// Metamorphosis - 103958
class spell_warl_metamorphosis_cost : public SpellScriptLoader
{
    public:
        spell_warl_metamorphosis_cost() : SpellScriptLoader("spell_warl_metamorphosis_cost") { }

        class spell_warl_metamorphosis_cost_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_metamorphosis_cost_AuraScript);

            /*void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* caster = GetCaster())
                    caster->EnergizeBySpell(caster, WARLOCK_METAMORPHOSIS, -6, POWER_DEMONIC_FURY);
            }*/

            void OnUpdate(uint32 diff, AuraEffect* aurEff)
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->GetPower(POWER_DEMONIC_FURY) <= 40)
                         GetAura()->Remove();
                }
            }

            void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (AuraEffect* aurEff = caster->GetAuraEffect(109145, 0))
                        aurEff->ChangeAmount(0);
            }

            void Register()
            {
                //OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_metamorphosis_cost_AuraScript::OnTick, EFFECT_2, SPELL_AURA_PERIODIC_DUMMY);
                OnEffectUpdate += AuraEffectUpdateFn(spell_warl_metamorphosis_cost_AuraScript::OnUpdate, EFFECT_0, SPELL_AURA_MOD_SHAPESHIFT);
                OnEffectRemove += AuraEffectApplyFn(spell_warl_metamorphosis_cost_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_MOD_SHAPESHIFT, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_metamorphosis_cost_AuraScript();
        }
};

// Immolation Aura - 104025 and 140720
class spell_warl_immolation_aura : public SpellScriptLoader
{
    public:
        spell_warl_immolation_aura() : SpellScriptLoader("spell_warl_immolation_aura") { }

        class spell_warl_immolation_aura_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_immolation_aura_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (GetCaster())
                {
                    GetCaster()->EnergizeBySpell(GetCaster(), GetSpellInfo()->Id, -25, POWER_DEMONIC_FURY);
                    if(aurEff->GetSpellInfo()->Id == 104025)
                        GetCaster()->CastSpell(GetCaster(), 129476, true);
                    else
                        GetCaster()->CastSpell(GetCaster(), 140721, true);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_immolation_aura_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_immolation_aura_AuraScript();
        }
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

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                amount = int32(100000000);
            }

            void OnAbsorb(AuraEffect* aurEff, DamageInfo& dmgInfo, uint32& absorbAmount)
            {
                totalAbsorbAmount += dmgInfo.GetDamage();
            }

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                // (totalAbsorbAmount / 16) it's for totalAbsorbAmount 50% & totalAbsorbAmount / 8 (for each tick of custom spell)
                if (Unit* caster = GetCaster())
                    caster->CastCustomSpell(WARLOCK_DARK_BARGAIN_DOT, SPELLVALUE_BASE_POINT0, (totalAbsorbAmount / 16) , caster, true);
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_dark_bargain_on_absorb_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_warl_dark_bargain_on_absorb_AuraScript::OnAbsorb, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
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

            void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes mode)
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

            void HandleTargetSelect(std::list<WorldObject*>& targets)
            {
                if (!targets.empty())
                    if (Unit* caster = GetCaster())
                        if (Player* _player = caster->ToPlayer())
                        {
                            SpellInfo const* _spellinfo = sSpellMgr->GetSpellInfo(1949);
                            int32 bp = 0;
                            bool firstTatget = true;

                            for (std::list<WorldObject*>::const_iterator itr = targets.begin(); itr != targets.end(); ++itr)
                            {
                                if (firstTatget)
                                {
                                    bp += _spellinfo->Effects[EFFECT_2].BasePoints;
                                    firstTatget = false;
                                }
                                else
                                {
                                    bp += _spellinfo->Effects[EFFECT_3].BasePoints;
                                }
                            }

                            _player->EnergizeBySpell(_player, GetSpellInfo()->Id, bp, POWER_DEMONIC_FURY);
                        }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_warl_hellfire_SpellScript::HandleTargetSelect, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
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
                if (Unit* caster = GetCaster())
                {
                    if (!caster->HasAura(56247) || !caster->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING_FAR))
                        caster->CastSpell(caster, WARLOCK_DEMONIC_LEAP_JUMP, true);
                    else
                    {
                        caster->CastSpell(caster, 124315, true);
                        caster->CastSpell(caster, 124342, true);
                    }
                }
            }

            SpellCastResult CheckHealth()
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING_FAR))
                    {
                        float _vmapHeight = caster->GetMap()->GetVmapHeight(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ());
                        float ground_Z = caster->GetPositionZ() - _vmapHeight;
                        if(ground_Z < 5.0 || ground_Z > 30.0f)
                            return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                    }
                    else
                        return SPELL_CAST_OK;
                }
                else
                    return SPELL_FAILED_DONT_REPORT;

                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_warl_demonic_leap_SpellScript::CheckHealth);
                AfterCast += SpellCastFn(spell_warl_demonic_leap_SpellScript::HandleAfterCast);
            }
        };

        class spell_warl_demonic_leap_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_demonic_leap_AuraScript);

            void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    //AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                    //if (removeMode == AURA_REMOVE_BY_DEATH)
                        caster->SendPlaySpellVisualKit(26177, 0);
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectApplyFn(spell_warl_demonic_leap_AuraScript::HandleRemove, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_demonic_leap_AuraScript();
        }

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_demonic_leap_SpellScript();
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

            void OnTick(AuraEffect const* aurEff)
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
                            target->CastSpell(caster, WARLOCK_SOUL_SWAP_VISUAL, true);
                            caster->m_SpecialTarget = target->GetGUID();
                        }
                        else if (GetSpellInfo()->Id == 86213)
                        {
                            caster->ApplySoulSwapDOT(target);
                            caster->RemoveAura(WARLOCK_SOUL_SWAP_AURA);
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

// Drain Soul - 1120
class spell_warl_drain_soul : public SpellScriptLoader
{
    public:
        spell_warl_drain_soul() : SpellScriptLoader("spell_warl_drain_soul") { }

        class spell_warl_drain_soul_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_drain_soul_AuraScript);

            void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes mode)
            {
                if (GetCaster())
                {
                    AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                    if (removeMode == AURA_REMOVE_BY_DEATH)
                        GetCaster()->ModifyPower(POWER_SOUL_SHARDS, 300);
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
enum GeteWay
{
    NPC_PURGE_GATE                          = 59271,
    NPC_GREEN_GATE                          = 59262,
};

uint32 gw[2][5]=
{
    { 113903, 113911, 113912, 113913, 113914 },
    { 113915, 113916, 113917, 113918, 113919}
};

class spell_warl_demonic_gateway_charges : public SpellScriptLoader
{
    public:
        spell_warl_demonic_gateway_charges() : SpellScriptLoader("spell_warl_demonic_gateway_charges") { }

        class spell_warl_demonic_gateway_charges_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_demonic_gateway_charges_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if(aurEff->GetAmount() < 5)
                    if(AuraEffect* aurEff0 = const_cast<AuraEffect*>(aurEff))
                    {
                        aurEff0->ChangeAmount(aurEff->GetAmount() + 1);
                        aurEff0->GetBase()->SetNeedClientUpdateForTargets();
                    }

                if (Player* _player = GetCaster()->ToPlayer())
                {
                    for (int32 i = 0; i < MAX_SUMMON_SLOT; ++i)
                    {
                        if (GUID_ENPART(_player->m_SummonSlot[i]) == NPC_PURGE_GATE ||
                            GUID_ENPART(_player->m_SummonSlot[i]) == NPC_GREEN_GATE)
                        {
                            if (Creature* gate = _player->GetMap()->GetCreature(_player->m_SummonSlot[i]))
                            {
                                uint8 g = GUID_ENPART(_player->m_SummonSlot[i]) == NPC_PURGE_GATE;
                                for(int32 j = 0; j < aurEff->GetAmount(); ++j)
                                    if (!gate->HasAura(gw[g][j]))
                                        gate->CastSpell(gate, gw[g][j], true);
                            }
                        }
                    }
                }
            }

            void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                ModCharges(-5);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_warl_demonic_gateway_charges_AuraScript::HandleApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_demonic_gateway_charges_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_demonic_gateway_charges_AuraScript();
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
                        if (!target->HasAura(WARLOCK_IMMOLATE) && !_player->HasAura(WARLOCK_GLYPH_OF_CONFLAGRATE))
                            if (Aura* conflagrate = target->GetAura(WARLOCK_CONFLAGRATE))
                                target->RemoveAura(WARLOCK_CONFLAGRATE);
                        if (!target->HasAura(WARLOCK_IMMOLATE_FIRE_AND_BRIMSTONE))
                            if (Aura* conflagrate = target->GetAura(WARLOCK_CONFLAGRATE_FIRE_AND_BRIMSTONE))
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

            void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes mode)
            {
                if (GetCaster())
                {
                    AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                    if (removeMode == AURA_REMOVE_BY_DEATH)
                        GetCaster()->ModifyPower(POWER_BURNING_EMBERS, 20); // Give 2 Burning Embers
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

// Called By : Incinerate - 29722
// Conflagrate - 17962
// Burning Embers generate
class spell_warl_burning_embers : public SpellScriptLoader
{
    public:
        spell_warl_burning_embers() : SpellScriptLoader("spell_warl_burning_embers") { }

        class spell_warl_burning_embers_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_burning_embers_SpellScript);

            void HandleAfterHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (GetSpell()->IsCritForTarget(target))
                            _player->ModifyPower(POWER_BURNING_EMBERS, 2);
                        else
                            _player->ModifyPower(POWER_BURNING_EMBERS, 1);
                    }
                }
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_warl_burning_embers_SpellScript::HandleAfterHit);
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
                        // Increases the duration of Immolate by 6s
                        if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_WARLOCK_DESTRUCTION)
                        {
                            if (GetSpell()->IsCritForTarget(target))
                                _player->ModifyPower(POWER_BURNING_EMBERS, 2);
                            else
                                _player->ModifyPower(POWER_BURNING_EMBERS, 1);
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

// Drain Life - 689, 89420, 103990
class spell_warl_drain_life : public SpellScriptLoader
{
    public:
        spell_warl_drain_life() : SpellScriptLoader("spell_warl_drain_life") { }

        class spell_warl_drain_life_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_drain_life_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* caster = GetCaster())
                {
                    Player* _player = GetCaster()->ToPlayer();
                    if (!_player)
                        return;

                    // In Demonology spec : Generates 10 Demonic Fury per second
                    if (GetSpellInfo()->Effects[2].IsAura(SPELL_AURA_DUMMY))
                        _player->EnergizeBySpell(_player, 689, GetSpellInfo()->Effects[2].BasePoints, POWER_DEMONIC_FURY);

                    int32 basepoints = _player->CountPctFromMaxHealth(GetSpellInfo()->Effects[1].BasePoints);
                    _player->CastCustomSpell(_player, WARLOCK_DRAIN_LIFE_HEAL, &basepoints, NULL, NULL, true);
                }
            }

            void Register()
            {
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

            void OnUpdate(uint32 diff, AuraEffect* aurEff)
            {
                update += diff;

                if (update >= 5000)
                {
                    if (Player* _player = GetCaster()->ToPlayer())
                    {
                        if (!_player->isInCombat() && !_player->InArena() && _player->isAlive())
                        {
                            _player->SetHealth(_player->GetHealth() + int32(_player->GetMaxHealth() * 0.02f));

                            if (Pet* pet = _player->GetPet())
                                pet->SetHealth(pet->GetHealth() + int32(pet->GetMaxHealth() * 0.02f));
                        }
                    }

                    update = 0;
                }
            }

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                amount = 0;
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_soul_harverst_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_MOD_HEALTH_REGEN_PERCENT);
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

        class spell_warl_life_tap_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_life_tap_AuraScript);

            void CalculateAmount(AuraEffect const* aurEff, int32& amount, bool& /*canBeRecalculated*/)
            {
                if(Unit* caster = GetCaster())
                {
                    amount = CalculatePct(caster->GetMaxHealth(), GetSpellInfo()->Effects[EFFECT_2].BasePoints);
                    amount += aurEff->GetOldBaseAmount();
                }
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_life_tap_AuraScript::CalculateAmount, EFFECT_2, SPELL_AURA_SCHOOL_HEAL_ABSORB);
            }
        };

        class spell_warl_life_tap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_life_tap_SpellScript);

            SpellCastResult CheckHealth()
            {
                if (Unit* caster = GetCaster())
                {
                    int32 percent = GetSpellInfo()->Effects[EFFECT_2].BasePoints;
                    if (caster->GetHealthPct() <= percent)
                    {
                        SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_NOT_ENOUGH_HEALTH);
                        return SPELL_FAILED_CUSTOM_ERROR;
                    }
                    else
                        return SPELL_CAST_OK;
                }
                else
                    return SPELL_FAILED_DONT_REPORT;

                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_warl_life_tap_SpellScript::CheckHealth);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_life_tap_SpellScript();
        }

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_life_tap_AuraScript();
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

            void OnTick(AuraEffect const* aurEff)
            {
                if (!GetCaster())
                    return;

                if (Player* _player = GetCaster()->ToPlayer())
                {
                    // Restoring 3-4.5% of the caster's total health every 1s - With 33% bonus
                    int32 basepoints = int32(frand(0.03f, 0.045f) * _player->GetMaxHealth());

                    if (!_player->HasSpellCooldown(WARLOCK_HARVEST_LIFE_HEAL))
                    {
                        _player->CastCustomSpell(_player, WARLOCK_HARVEST_LIFE_HEAL, &basepoints, NULL, NULL, true);
                        // prevent the heal to proc off for each targets
                        _player->AddSpellCooldown(WARLOCK_HARVEST_LIFE_HEAL, 0, getPreciseTime() + 1.0);
                    }

                    _player->EnergizeBySpell(_player, aurEff->GetSpellInfo()->Id, 4, POWER_DEMONIC_FURY);
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
                            _player->AddSpellCooldown(5782, 0, getPreciseTime() + 1.0);
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

            uint32 slot;
            bool slotExist;

            void HandleRemove(AuraEffect const* aurEff, AuraEffectHandleModes mode)
            {
                if(Unit* target = GetTarget())
                {
                    // If effect is removed by expire remove the summoned demonic circle too.
                    if (!(mode & AURA_EFFECT_HANDLE_REAPPLY))
                        target->RemoveGameObject(GetId(), true);

                    target->SendFakeAuraUpdate(WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST, (AFLAG_CASTER | AFLAG_POSITIVE | AFLAG_FAKEAURA), 0, slot, true);
                }
            }

            void HandleDummyTick(AuraEffect const* aurEff)
            {
                if(Unit* target = GetTarget())
                {
                    if (GameObject* circle = target->GetGameObject(GetId()))
                    {
                        // Here we check if player is in demonic circle teleport range, if so add
                        // WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST; allowing him to cast the WARLOCK_DEMONIC_CIRCLE_TELEPORT.

                        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(WARLOCK_DEMONIC_CIRCLE_TELEPORT);

                        if (target->IsWithinDist(circle, spellInfo->GetMaxRange(true)))
                            target->SendFakeAuraUpdate(WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST, (AFLAG_CASTER | AFLAG_POSITIVE | AFLAG_FAKEAURA), 0, slot, false);
                        else
                            target->SendFakeAuraUpdate(WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST, (AFLAG_CASTER | AFLAG_POSITIVE | AFLAG_FAKEAURA), 0, slot, true);
                    }
                }
            }

            void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* target = GetTarget())
                {
                    if(target->GetFreeAuraSlot(slot))
                        target->SendFakeAuraUpdate(WARLOCK_DEMONIC_CIRCLE_ALLOW_CAST, (AFLAG_CASTER | AFLAG_POSITIVE | AFLAG_DURATION), 1000, slot, false);
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectApplyFn(spell_warl_demonic_circle_summon_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_demonic_circle_summon_AuraScript::HandleDummyTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
                OnEffectApply += AuraEffectApplyFn(spell_warl_demonic_circle_summon_AuraScript::HandleApply, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
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

            void HandleTeleport(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Player* player = GetTarget()->ToPlayer())
                {
                    if (GameObject* circle = player->GetGameObjectbyId(191083))
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
                    if (AuraEffect const* aurEff = GetEffect(EFFECT_0))
                    {
                        int32 damage = aurEff->GetAmount() * 16;
                        Unit* dispeller = dispelInfo->GetDispeller();

                        if (dispeller->GetTypeId() == TYPEID_PLAYER)
                            caster->ApplyResilience(dispeller, &damage, true);
                        // backfire damage and silence
                        caster->CastCustomSpell(dispeller, WARLOCK_UNSTABLE_AFFLICTION_DISPEL, &damage, NULL, NULL, true, NULL, aurEff);
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

// Unbound Will - 108482
class spell_warl_unbound_will : public SpellScriptLoader
{
    public:
        spell_warl_unbound_will() : SpellScriptLoader("spell_warl_unbound_will") { }

        class spell_warl_unbound_will_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_unbound_will_SpellScript);

            SpellCastResult CheckHealth()
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->GetHealthPct() <= 20.0f)
                    {
                        SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_NOT_ENOUGH_HEALTH);
                        return SPELL_FAILED_CUSTOM_ERROR;
                    }
                    else
                        return SPELL_CAST_OK;
                }
                else
                    return SPELL_FAILED_DONT_REPORT;

                return SPELL_CAST_OK;
            }

            void HandleAfterHit()
            {
                if (Unit* player = GetCaster())
                {
                    player->ModifyHealth(-int32(player->CountPctFromMaxHealth(20)));
                    std::list<AuraType> auratypelist;
                    std::vector<uint32> removeAuraId;
                    auratypelist.push_back(SPELL_AURA_MOD_CONFUSE);
                    auratypelist.push_back(SPELL_AURA_MOD_FEAR);
                    auratypelist.push_back(SPELL_AURA_MOD_FEAR_2);
                    auratypelist.push_back(SPELL_AURA_MOD_STUN);
                    auratypelist.push_back(SPELL_AURA_MOD_ROOT);
                    auratypelist.push_back(SPELL_AURA_MOD_DECREASE_SPEED);
                    auratypelist.push_back(SPELL_AURA_TRANSFORM);

                    for (std::list<AuraType>::iterator auratype = auratypelist.begin(); auratype != auratypelist.end(); ++auratype)
                    {
                        std::list<AuraEffect*> const& effList = player->GetAuraEffectsByType(*auratype);
                        if (!effList.empty())
                            for (std::list<AuraEffect*>::const_iterator itr = effList.begin(); itr != effList.end(); ++itr)
                                if (AuraEffect* eff = (*itr))
                                    removeAuraId.push_back(eff->GetId());
                    }

                    if (!removeAuraId.empty())
                        for (std::vector<uint32>::iterator i = removeAuraId.begin(); i != removeAuraId.end(); ++i)
                            player->RemoveAura(*i);
                }
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_warl_unbound_will_SpellScript::CheckHealth);
                AfterHit += SpellHitFn(spell_warl_unbound_will_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_unbound_will_SpellScript();
        }
};

// Seed of Corruption - 27243
class spell_warl_seed_of_corruption_dota : public SpellScriptLoader
{
    public:
        spell_warl_seed_of_corruption_dota() : SpellScriptLoader("spell_warl_seed_of_corruption_dota") { }

        class spell_warl_seed_of_corruption_dota_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_seed_of_corruption_dota_AuraScript);

            int32 damage;

            bool Load()
            {
                damage = 0;
                return true;
            }

            void CalculateAmount(AuraEffect const* aurEff, int32 & amount, bool & /*canBeRecalculated*/)
            {
                damage = amount;
                if (Unit* caster = GetCaster())
                {
                    if(Player* _player = caster->ToPlayer())
                        if(Unit* target = _player->GetSelectedUnit())
                        {
                            damage = caster->SpellDamageBonusDone(target, GetSpellInfo(), damage, DOT, EFFECT_0, GetAura()->GetStackAmount());
                            damage = target->SpellDamageBonusTaken(caster, GetSpellInfo(), damage);
                        }
                }
                damage *= aurEff->GetTotalTicks();
            }

            void CalculateAmountDummy(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                amount = damage;
            }

            void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
                    return;

                if (Unit* caster = GetCaster())
                    if(Unit* target = GetTarget())
                    {
                        uint32 triggered_spell_id = GetSpellInfo()->Id == 27243 ? 27285 : 87385;
                        caster->CastSpell(target, triggered_spell_id, true);
                    }
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_seed_of_corruption_dota_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_seed_of_corruption_dota_AuraScript::CalculateAmountDummy, EFFECT_1, SPELL_AURA_DUMMY);
                OnEffectRemove += AuraEffectApplyFn(spell_warl_seed_of_corruption_dota_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_seed_of_corruption_dota_AuraScript();
        }
};

// Havoc - 80240
class spell_warl_havoc : public SpellScriptLoader
{
    public:
        spell_warl_havoc() : SpellScriptLoader("spell_warl_havoc") { }

        class spell_warl_havoc_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_havoc_AuraScript);

            void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                GetAura()->SetMaxStackAmount();
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_warl_havoc_AuraScript::HandleApply, EFFECT_1, SPELL_AURA_ADD_FLAT_MODIFIER, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_havoc_AuraScript();
        }
};

// 42223, 104233 - Rain of Fire spell for Immolate
class spell_warl_rain_of_fire_damage : public SpellScriptLoader
{
    public:
        spell_warl_rain_of_fire_damage() : SpellScriptLoader("spell_warl_rain_of_fire_damage") { }

        class spell_warl_rain_of_fire_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_rain_of_fire_damage_SpellScript);

            void Damage(SpellEffIndex /*effIndex*/)
            {
                if (Unit* unitTarget = GetHitUnit())
                {
                    if(unitTarget->HasAura(348) || unitTarget->HasAura(108686))
                        SetHitDamage(int32(GetHitDamage() * 1.5f));

                    GetSpell()->AddEffectTarget(unitTarget->GetGUID());
                }
            }

            void HandleAfterCast()
            {
                if(GetSpell()->GetEffectTargets().empty())
                    return;

                if (Unit* caster = GetCaster())
                    if (roll_chance_i(30))
                        caster->ModifyPower(POWER_BURNING_EMBERS, 1);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_rain_of_fire_damage_SpellScript::Damage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
                AfterCast += SpellCastFn(spell_warl_rain_of_fire_damage_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_rain_of_fire_damage_SpellScript();
        }
};

// Metamorphosis - 103965 for Dark Apotheosis 114168
class spell_warl_metamorphosis : public SpellScriptLoader
{
    public:
        spell_warl_metamorphosis() : SpellScriptLoader("spell_warl_metamorphosis") { }

        class spell_warl_metamorphosis_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_metamorphosis_AuraScript);

            void CalculateAmount(AuraEffect const* aurEff, int32 & amount, bool & /*canBeRecalculated*/)
            {
                if (Unit* caster = GetCaster())
                {
                    switch(aurEff->GetEffIndex())
                    {
                        case EFFECT_0:
                            if(caster->HasAura(114168))
                                amount = 114175;
                            break;
                        case EFFECT_1:
                        case EFFECT_3:
                        case EFFECT_14:
                            if(caster->HasAura(114168))
                                amount = 0;
                            break;
                    }
                }
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_metamorphosis_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_metamorphosis_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_metamorphosis_AuraScript::CalculateAmount, EFFECT_3, SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_metamorphosis_AuraScript::CalculateAmount, EFFECT_14, SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_metamorphosis_AuraScript();
        }
};

// Corruption - 146739 for Malefic Grasp 103103
class spell_warl_corruption : public SpellScriptLoader
{
    public:
        spell_warl_corruption() : SpellScriptLoader("spell_warl_corruption") { }

        class spell_warl_corruption_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_corruption_AuraScript);

            void HandleTick(AuraEffect const* aurEff, int32& /*amount*/, Unit* /*target*/)
            {
                if (GetCaster())
                    GetCaster()->EnergizeBySpell(GetCaster(), aurEff->GetSpellInfo()->Id, 4, POWER_DEMONIC_FURY);
            }

            void Register()
            {
                DoEffectChangeTickDamage += AuraEffectChangeTickDamageFn(spell_warl_corruption_AuraScript::HandleTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_corruption_AuraScript();
        }
};

// Healthstone - 6262
class spell_warl_healthstone : public SpellScriptLoader
{
    public:
        spell_warl_healthstone() : SpellScriptLoader("spell_warl_healthstone") { }

        class spell_warl_healthstone_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_healthstone_SpellScript);

            void HandleHeal(SpellEffIndex effIndex)
            {
                int32 percent = GetSpellInfo()->Effects[effIndex].BasePoints;
                if (Unit* caster = GetCaster())
                    SetHitHeal(CalculatePct(caster->GetMaxHealth(), percent));
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_healthstone_SpellScript::HandleHeal, EFFECT_0, SPELL_EFFECT_HEAL);
            }
        };

        class spell_warl_healthstone_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_healthstone_AuraScript);

            void CalculateAmount(AuraEffect const* aurEff, int32 & amount, bool & /*canBeRecalculated*/)
            {
                int32 percent = int32(GetSpellInfo()->Effects[aurEff->GetEffIndex()].BasePoints / 10);
                if (Unit* caster = GetCaster())
                    amount = CalculatePct(caster->GetMaxHealth(), percent);
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_healthstone_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_PERIODIC_HEAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_healthstone_AuraScript();
        }

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_healthstone_SpellScript();
        }
};

//Imp Swarm - 104316
class spell_warl_imp_swarm : public SpellScriptLoader
{
    public:
        spell_warl_imp_swarm() : SpellScriptLoader("spell_warl_imp_swarm") { }

        class spell_warl_imp_swarm_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_imp_swarm_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                if (Unit* target = GetHitUnit())
                {
                    caster->CastSpell(target, WARLOCK_WILD_IMP_SUMMON, true);
                    caster->CastSpell(target, WARLOCK_WILD_IMP_SUMMON, true);
                    caster->CastSpell(target, WARLOCK_WILD_IMP_SUMMON, true);
                    caster->CastSpell(target, WARLOCK_WILD_IMP_SUMMON, true);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_imp_swarm_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_imp_swarm_SpellScript();
        }
};

// 113942 - Demonic Gateway Debuf
class spell_warl_demonic_gateway : public SpellScriptLoader
{
    public:
        spell_warl_demonic_gateway() : SpellScriptLoader("spell_warl_demonic_gateway") { }

        class spell_warl_demonic_gateway_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_demonic_gateway_AuraScript);

            void CalculateMaxDuration(int32 & duration)
            {
                if (Unit* caster = GetCaster())
                    if(Unit* owner = ObjectAccessor::GetUnit(*caster, GetCasterGUID()))
                        if (owner->HasAura(143395))
                            duration = 45000;
            }

            void Register()
            {
                DoCalcMaxDuration += AuraCalcMaxDurationFn(spell_warl_demonic_gateway_AuraScript::CalculateMaxDuration);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_demonic_gateway_AuraScript();
        }
};

// Demonic Gateway - 111771
class spell_warl_demonic_gateway_cast : public SpellScriptLoader
{
    public:
        spell_warl_demonic_gateway_cast() : SpellScriptLoader("spell_warl_demonic_gateway_cast") { }

        class spell_warl_demonic_gateway_cast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_demonic_gateway_cast_SpellScript);

            SpellCastResult CheckCast()
            {
                if(Unit* caster = GetCaster())
                {
                    Position const* pos = GetExplTargetDest();
                    float delta_z = fabs(pos->GetPositionZ()) - fabs(caster->GetPositionZ());
                    if(delta_z > 2.7f || delta_z < -2.7f)
                        return SPELL_FAILED_NOT_HERE;
                }

                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_warl_demonic_gateway_cast_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_demonic_gateway_cast_SpellScript();
        }
};

// Fire and Brimstone - 108683
class spell_warl_fire_and_brimstone : public SpellScriptLoader
{
    public:
        spell_warl_fire_and_brimstone() : SpellScriptLoader("spell_warl_fire_and_brimstone") { }

        class spell_warl_fire_and_brimstone_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_fire_and_brimstone_SpellScript);

            SpellCastResult CheckCast()
            {
                if(Unit* caster = GetCaster())
                {
                    if(caster->GetPower(POWER_BURNING_EMBERS) < 10)
                        return SPELL_FAILED_NOT_READY;
                }

                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_warl_fire_and_brimstone_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_fire_and_brimstone_SpellScript();
        }
};

// Felsteed - 5784
class spell_warl_felsteed : public SpellScriptLoader
{
    public:
        spell_warl_felsteed() : SpellScriptLoader("spell_warl_felsteed") { }

        class spell_warl_felsteed_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_felsteed_AuraScript);

            Position savePos;

            void OnTick(AuraEffect const* /*aurEff*/)
            {
                if(Unit* caster = GetCaster())
                {
                    ZLiquidStatus status = caster->GetBaseMap()->getLiquidStatus(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), MAP_ALL_LIQUIDS);
                    if(!status)
                        return;

                    float distance = caster->GetDistance(savePos);
                    uint32 count = uint32(distance / 2);
                    float angle = caster->GetAngle(&savePos);
                    if(count > 0)
                    {
                        for(uint32 j = 0; j < count + 2; ++j)
                        {
                            int32 distanceNext = j * 2;
                            float destx = caster->GetPositionX() + distanceNext * std::cos(angle);
                            float desty = caster->GetPositionY() + distanceNext * std::sin(angle);
                            Position tempPos = {destx, desty, caster->GetPositionZ(), 0.0f};
                            caster->SendSpellCreateVisual(GetSpellInfo(), &tempPos);
                        }

                        savePos.Relocate(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ()); //save last position
                        caster->SendPlaySpellVisualKit(23384, 0);
                    }
                }
            }

            void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* caster = GetCaster())
                    savePos.Relocate(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ());
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_warl_felsteed_AuraScript::HandleApply, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_warl_felsteed_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_felsteed_AuraScript();
        }
};

// 114736 - Disrupted Nether
class spell_warl_disrupted_nether : public SpellScriptLoader
{
    public:
        spell_warl_disrupted_nether() : SpellScriptLoader("spell_warl_disrupted_nether") { }

        class spell_warl_disrupted_nether_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_disrupted_nether_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* caster = GetCaster())
                    if (!caster->HasAura(114592))
                        caster->AddAura(114592, caster);
            }

            void CalculateMaxDuration(int32 & duration)
            {
                duration *= 2;
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_warl_disrupted_nether_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                DoCalcMaxDuration += AuraCalcMaxDurationFn(spell_warl_disrupted_nether_AuraScript::CalculateMaxDuration);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_disrupted_nether_AuraScript();
        }
};

// 113896, 120729 - Demonic Gateway
class spell_warl_demonic_gateway_duration : public SpellScriptLoader
{
    public:
        spell_warl_demonic_gateway_duration() : SpellScriptLoader("spell_warl_demonic_gateway_duration") { }

        class spell_warl_demonic_gateway_duration_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_demonic_gateway_duration_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* caster = GetCaster())
                {
                    if(Position const* pos = GetExplTargetDest())
                    {
                        float dist = caster->GetExactDist2d(pos->GetPositionX(), pos->GetPositionY());
                        int32 duration = 1000.0f + ((dist - 20.0f) * 10.0f);
                        SetDuration(duration);
                    }
                }
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_warl_demonic_gateway_duration_AuraScript::OnApply, EFFECT_0, SPELL_AURA_SCREEN_EFFECT, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_demonic_gateway_duration_AuraScript();
        }
};

// 114592 - Wild Imps
class spell_warl_wild_imps : public SpellScriptLoader
{
    public:
        spell_warl_wild_imps() : SpellScriptLoader("spell_warl_wild_imps") { }

        class spell_warl_wild_imps_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_wild_imps_AuraScript);

            void HandleEffectCalcPeriodic(AuraEffect const* /*aurEff*/, bool& /*isPeriodic*/, int32& amplitude)
            {
                if(Unit* caster = GetCaster())
                    if (caster->HasAura(56242)) // Glyph of Imp Swarm
                        amplitude += 4 * IN_MILLISECONDS;
            }

            void Register()
            {
                DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_warl_wild_imps_AuraScript::HandleEffectCalcPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_wild_imps_AuraScript();
        }
};

// Demonic Slash - 114175
class spell_warl_demonic_slash : public SpellScriptLoader
{
    public:
        spell_warl_demonic_slash() : SpellScriptLoader("spell_warl_demonic_slash") { }

        class spell_warl_demonic_slash_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_demonic_slash_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        _player->EnergizeBySpell(_player, GetSpellInfo()->Id, 60, POWER_DEMONIC_FURY);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_demonic_slash_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_demonic_slash_SpellScript();
        }
};

// Demonic Gateway - 113902
class spell_warl_demonic_gateway_at : public SpellScriptLoader
{
    public:
        spell_warl_demonic_gateway_at() : SpellScriptLoader("spell_warl_demonic_gateway_at") { }

        class spell_warl_demonic_gateway_at_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_demonic_gateway_at_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    float distanceG = 0.0f;
                    float distanceP = 0.0f;
                    uint64 uGateG = 0;
                    uint64 uGateP = 0;
                    for (int32 i = 0; i < MAX_SUMMON_SLOT; ++i)
                    {
                        if(!caster->m_SummonSlot[i])
                            continue;

                        Unit* uGate = ObjectAccessor::GetUnit(*caster, caster->m_SummonSlot[i]);
                        if(uGate && uGate->GetEntry() == 59271)
                        {
                            distanceG = caster->GetDistance(uGate);
                            uGateG = caster->m_SummonSlot[i];
                        }
                        if(uGate && uGate->GetEntry() == 59262)
                        {
                            distanceP = caster->GetDistance(uGate);
                            uGateP = caster->m_SummonSlot[i];
                        }
                    }

                    if(uGateG && uGateP && (distanceG != 0.0f || distanceP != 0.0f))
                    {
                        if(distanceG > distanceP)
                        {
                            if(Creature* creature = ObjectAccessor::GetCreatureOrPetOrVehicle(*caster, uGateP))
                                creature->AI()->OnSpellClick(caster);
                        }
                        else
                        {
                            if(Creature* creature = ObjectAccessor::GetCreatureOrPetOrVehicle(*caster, uGateG))
                                creature->AI()->OnSpellClick(caster);
                        }
                    }
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_warl_demonic_gateway_at_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_demonic_gateway_at_SpellScript();
        }
};

// Seduction (Special Ability) - 6358, Mesmerize (Special Ability) - 115268
class spell_warl_seduction : public SpellScriptLoader
{
    public:
        spell_warl_seduction() : SpellScriptLoader("spell_warl_seduction") { }

        class spell_warl_seduction_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_seduction_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                    if (Unit* owner = caster->GetAnyOwner())
                        if(owner->HasAura(56249)) // Glyph of Demon Training
                            if (Unit* target = GetHitUnit())
                                target->RemoveAurasByType(SPELL_AURA_PERIODIC_DAMAGE);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_seduction_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_seduction_SpellScript();
        }
};

// 108686 - Immolate
class spell_warl_immolate : public SpellScriptLoader
{
    public:
        spell_warl_immolate() : SpellScriptLoader("spell_warl_immolate") { }

        class spell_warl_immolate_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_immolate_SpellScript);

            void Damage(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                   if(Aura* aura = caster->GetAura(77220))
                   {
                       int32 mastery = aura->GetEffect(EFFECT_0)->GetAmount();
                       float perc = (35 * (100 + mastery)) / 100;
                       int32 damage = CalculatePct(GetHitDamage(), perc);
                       SetHitDamage(damage);
                   }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_immolate_SpellScript::Damage, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        class spell_warl_immolate_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_immolate_AuraScript);

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                if (Unit* caster = GetCaster())
                {
                   if(Aura* aura = caster->GetAura(77220))
                   {
                       int32 mastery = aura->GetEffect(EFFECT_0)->GetAmount();
                       float perc = (35 * (100 + mastery)) / 100;
                       amount = CalculatePct(amount, perc);
                   }
                }
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_warl_immolate_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_immolate_AuraScript();
        }

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_immolate_SpellScript();
        }
};

// 114654, 108685 - Incinerate and Conflagrate
class spell_warl_incinerate : public SpellScriptLoader
{
    public:
        spell_warl_incinerate() : SpellScriptLoader("spell_warl_incinerate") { }

        class spell_warl_incinerate_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_incinerate_SpellScript);

            void Damage(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                   if(Aura* aura = caster->GetAura(77220))
                   {
                       int32 mastery = aura->GetEffect(EFFECT_0)->GetAmount();
                       float perc = (35 * (100 + mastery)) / 100;
                       int32 damage = CalculatePct(GetHitDamage(), perc);
                       SetHitDamage(damage);
                   }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_incinerate_SpellScript::Damage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_incinerate_SpellScript();
        }
};

// Conflagrate (Fire and Brimstone) - 108685 and Incinerate (Fire and Brimstone) - 114654
class spell_warl_burning_embers_aoe : public SpellScriptLoader
{
    public:
        spell_warl_burning_embers_aoe() : SpellScriptLoader("spell_warl_burning_embers_aoe") { }

        class spell_warl_burning_embers_aoe_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_burning_embers_aoe_SpellScript);

            void Damage(SpellEffIndex /*effIndex*/)
            {
                if (Unit* unitTarget = GetHitUnit())
                    GetSpell()->AddEffectTarget(unitTarget->GetGUID());
            }

            void HandleAfterCast()
            {
                if(GetSpell()->GetEffectTargets().empty())
                    return;

                if (Unit* caster = GetCaster())
                    caster->ModifyPower(POWER_BURNING_EMBERS, 1);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_warl_burning_embers_aoe_SpellScript::Damage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
                AfterCast += SpellCastFn(spell_warl_burning_embers_aoe_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_burning_embers_aoe_SpellScript();
        }
};

// 115236 - Void Shield (Special Ability)
class spell_warl_void_shield : public SpellScriptLoader
{
    public:
        spell_warl_void_shield() : SpellScriptLoader("spell_warl_void_shield") { }

        class spell_warl_void_shield_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_warl_void_shield_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* caster = GetCaster())
                {
                    caster->CastSpell(caster, 115241, true); // Void Shield Charge 1
                    caster->CastSpell(caster, 115242, true); // Void Shield Charge 2
                    caster->CastSpell(caster, 115243, true); // Void Shield Charge 3
                }
                SetStackAmount(3);
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* caster = GetCaster())
                {
                    caster->RemoveAurasDueToSpell(115241); // Void Shield Charge 1
                    caster->RemoveAurasDueToSpell(115242); // Void Shield Charge 2
                    caster->RemoveAurasDueToSpell(115243); // Void Shield Charge 3
                }
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_warl_void_shield_AuraScript::OnApply, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_warl_void_shield_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_warl_void_shield_AuraScript();
        }
};

// 115240 - Void Shield
class spell_warl_void_shield_damage : public SpellScriptLoader
{
    public:
        spell_warl_void_shield_damage() : SpellScriptLoader("spell_warl_void_shield_damage") { }

        class spell_warl_void_shield_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_warl_void_shield_damage_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Aura* shield = caster->GetAura(115236))
                    {
                        uint8 stack = shield->GetStackAmount();
                        switch(stack)
                        {
                            case 1:
                            shield->ModStackAmount(-1);
                            return;
                            case 2:
                            caster->RemoveAurasDueToSpell(115242); // Void Shield Charge 2
                            break;
                            case 3:
                            caster->RemoveAurasDueToSpell(115243); // Void Shield Charge 3
                            break;
                        }
                        shield->SetStackAmount(stack - 1);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_warl_void_shield_damage_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_warl_void_shield_damage_SpellScript();
        }
};

void AddSC_warlock_spell_scripts()
{
    new spell_warl_shield_of_shadow();
    new spell_warl_grimoire_of_sacrifice();
    new spell_warl_flames_of_xoroth();
    new spell_warl_archimondes_vengeance_cooldown();
    new spell_warl_archimondes_vengance();
    new spell_warl_molten_core_dot();
    new spell_warl_void_ray();
    new spell_warl_chaos_wave();
    new spell_warl_metamorphosis_cost();
    new spell_warl_immolation_aura();
    new spell_warl_dark_bargain_on_absorb();
    new spell_warl_dark_regeneration();
    new spell_warl_twilight_ward_s12();
    new spell_warl_hellfire();
    new spell_warl_demonic_leap();
    new spell_warl_burning_rush();
    new spell_warl_soul_swap_soulburn();
    new spell_warl_soul_swap();
    new spell_warl_drain_soul();
    new spell_warl_demonic_gateway_charges();
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
    new spell_warl_create_healthstone();
    new spell_warl_seed_of_corruption();
    new spell_warl_soulshatter();
    new spell_warl_demonic_circle_summon();
    new spell_warl_demonic_circle_teleport();
    new spell_warl_unstable_affliction();
    new spell_warl_unbound_will();
    new spell_warl_seed_of_corruption_dota();
    new spell_warl_havoc();
    new spell_warl_rain_of_fire_damage();
    new spell_warl_metamorphosis();
    new spell_warl_corruption();
    new spell_warl_healthstone();
    new spell_warl_imp_swarm();
    new spell_warl_demonic_gateway();
    new spell_warl_demonic_gateway_cast();
    new spell_warl_fire_and_brimstone();
    new spell_warl_felsteed();
    new spell_warl_disrupted_nether();
    new spell_warl_demonic_gateway_duration();
    new spell_warl_wild_imps();
    new spell_warl_demonic_slash();
    new spell_warl_demonic_gateway_at();
    new spell_warl_seduction();
    new spell_warl_immolate();
    new spell_warl_incinerate();
    new spell_warl_burning_embers_aoe();
    new spell_warl_void_shield();
    new spell_warl_void_shield_damage();
}