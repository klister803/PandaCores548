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
 * Scripts for spells with SPELLFAMILY_SHAMAN and SPELLFAMILY_GENERIC spells used by shaman players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_sha_".
 */

#include "ScriptMgr.h"
#include "GridNotifiers.h"
#include "Unit.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum ShamanSpells
{
    SHAMAN_SPELL_SATED                      = 57724,
    SHAMAN_SPELL_EXHAUSTION                 = 57723,
    HUNTER_SPELL_INSANITY                   = 95809,
    MAGE_SPELL_TEMPORAL_DISPLACEMENT        = 80354,
    // For Earthen Power
    SHAMAN_TOTEM_SPELL_EARTHBIND_TOTEM      = 6474,
    SPELL_SHA_ASCENDANCE_ELEMENTAL	        = 114050,
    SPELL_SHA_ASCENDANCE_RESTORATION        = 114052,
    SPELL_SHA_ASCENDANCE_ENHANCED	        = 114051,
    SPELL_SHA_ASCENDANCE			        = 114049,
    SPELL_SHA_HEALING_RAIN                  = 73920,
    SPELL_SHA_HEALING_RAIN_TICK             = 73921,
    SPELL_SHA_EARTHQUAKE                    = 61882,
    SPELL_SHA_EARTHQUAKE_TICK               = 77478,
    SPELL_SHA_EARTHQUAKE_KNOCKING_DOWN      = 77505,
    SPELL_SHA_ELEMENTAL_BLAST               = 117014,
    SPELL_SHA_ELEMENTAL_BLAST_RATING_BONUS  = 118522,
    SPELL_SHA_ELEMENTAL_BLAST_NATURE_VISUAL = 118517,
    SPELL_SHA_ELEMENTAL_BLAST_FROST_VISUAL  = 118515,
    SPELL_SHA_LAVA_LASH                     = 60103,
    SPELL_SHA_FLAME_SHOCK                   = 8050,
    SPELL_SHA_STORMSTRIKE                   = 17364,
    SPELL_SHA_LIGHTNING_SHIELD_ORB_DAMAGE   = 26364,
    SPELL_SHA_LAVA_SURGE_CAST_TIME          = 77762,
    SPELL_SHA_FULMINATION                   = 88766,
    SPELL_SHA_FULMINATION_TRIGGERED         = 88767,
    SPELL_SHA_FULMINATION_INFO              = 95774,
    SPELL_SHA_ROLLING_THUNDER_ENERGIZE      = 88765,
    SPELL_SHA_UNLEASH_ELEMENTS              = 73680,
    SPELL_SHA_SEARING_FLAMES_DAMAGE_DONE    = 77661,
    SPELL_SHA_FIRE_NOVA                     = 1535,
    SPELL_SHA_FIRE_NOVA_TRIGGERED           = 8349,
    SPELL_SHA_MANA_TIDE                     = 16191,
    SPELL_SHA_FROST_SHOCK_FREEZE            = 63685,
    SPELL_SHA_FROZEN_POWER                  = 63374,
    SPELL_SHA_MAIL_SPECIALIZATION_AGI       = 86099,
    SPELL_SHA_MAIL_SPECIALISATION_INT       = 86100,
    SPELL_SHA_UNLEASHED_FURY_TALENT         = 117012,
    SPELL_SHA_UNLEASHED_FURY_FLAMETONGUE    = 118470,
    SPELL_SHA_UNLEASHED_FURY_WINDFURY       = 118472,
    SPELL_SHA_UNLEASHED_FURY_EARTHLIVING    = 118473,
    SPELL_SHA_UNLEASHED_FURY_FROSTBRAND     = 118474,
    SPELL_SHA_UNLEASHED_FURY_ROCKBITER      = 118475,
    SPELL_SHA_STONE_BULWARK_ABSORB          = 114893,
    SPELL_SHA_EARTHGRAB_IMMUNITY            = 116946,
    SPELL_SHA_EARTHBIND_FOR_EARTHGRAB_TOTEM = 116947,
    SPELL_SHA_ECHO_OF_THE_ELEMENTS          = 108283,
    SPELL_SHA_ANCESTRAL_GUIDANCE            = 114911,
    SPELL_SHA_CONDUCTIVITY_TALENT           = 108282,
    SPELL_SHA_CONDUCTIVITY_HEAL             = 118800,
    SPELL_SHA_GLYPH_OF_LAKESTRIDER          = 55448,
    SPELL_SHA_WATER_WALKING                 = 546,
    SPELL_SHA_GLYPH_OF_SHAMANISTIC_RAGE     = 63280,
    SPELL_SHA_SOLAR_BEAM                    = 113286,
    SPELL_SHA_SOLAR_BEAM_SILENCE            = 113288,
    SPELL_SHA_GHOST_WOLF                    = 2645,
    SPELL_SHA_ITEM_T14_4P                   = 123124,
    SPELL_DRUMS_OF_RAGE                     = 146555,
};

// Prowl - 113289
class spell_sha_prowl : public SpellScriptLoader
{
    public:
        spell_sha_prowl() : SpellScriptLoader("spell_sha_prowl") { }

        class spell_sha_prowl_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_prowl_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    _player->CastSpell(_player, SPELL_SHA_GHOST_WOLF, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_prowl_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_prowl_SpellScript();
        }
};

// Solar beam - 113286
class spell_sha_solar_beam : public SpellScriptLoader
{
    public:
        spell_sha_solar_beam() : SpellScriptLoader("spell_sha_solar_beam") { }

        class spell_sha_solar_beam_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_solar_beam_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (DynamicObject* dynObj = GetCaster()->GetDynObject(SPELL_SHA_SOLAR_BEAM))
                    GetCaster()->CastSpell(dynObj->GetPositionX(), dynObj->GetPositionY(), dynObj->GetPositionZ(), SPELL_SHA_SOLAR_BEAM_SILENCE, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_solar_beam_AuraScript::OnTick, EFFECT_2, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_solar_beam_AuraScript();
        }
};

// Called by Shamanistic Rage - 30823
// Glyph of Shamanistic Rage - 63280
class spell_sha_glyph_of_shamanistic_rage : public SpellScriptLoader
{
    public:
        spell_sha_glyph_of_shamanistic_rage() : SpellScriptLoader("spell_sha_glyph_of_shamanistic_rage") { }

        class spell_sha_glyph_of_shamanistic_rage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_glyph_of_shamanistic_rage_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->HasAura(SPELL_SHA_GLYPH_OF_SHAMANISTIC_RAGE))
                    {
                        DispelChargesList dispelList;
                        _player->GetDispellableAuraList(_player, DISPEL_ALL_MASK, dispelList);
                        if (!dispelList.empty())
                        {
                            for (DispelChargesList::const_iterator itr = dispelList.begin(); itr != dispelList.end(); ++itr)
                                if (_player->HasAura(itr->first->GetId()))
                                    _player->RemoveAura(itr->first);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_glyph_of_shamanistic_rage_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_glyph_of_shamanistic_rage_SpellScript();
        }
};

// Called by Ghost Wolf - 2645
// Glyph of Lakestrider - 55448
class spell_sha_glyph_of_lakestrider : public SpellScriptLoader
{
    public:
        spell_sha_glyph_of_lakestrider() : SpellScriptLoader("spell_sha_glyph_of_lakestrider") { }

        class spell_sha_glyph_of_lakestrider_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_glyph_of_lakestrider_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (_player->HasAura(SPELL_SHA_GLYPH_OF_LAKESTRIDER))
                        _player->CastSpell(_player, SPELL_SHA_WATER_WALKING, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_glyph_of_lakestrider_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_glyph_of_lakestrider_SpellScript();
        }
};

// Call of the Elements - 108285
class spell_sha_call_of_the_elements : public SpellScriptLoader
{
    public:
        spell_sha_call_of_the_elements() : SpellScriptLoader("spell_sha_call_of_the_elements") { }

        class spell_sha_call_of_the_elements_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_call_of_the_elements_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    // immediately finishes the cooldown on totems with less than 3min cooldown
                    const SpellCooldowns& cm = _player->GetSpellCooldownMap();
                    for (SpellCooldowns::const_iterator itr = cm.begin(); itr != cm.end();)
                    {
                        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);

                        if (spellInfo && (spellInfo->Id == 51485 || spellInfo->Id == 108273 || spellInfo->Id == 108270
                            || spellInfo->Id == 108269 || spellInfo->Id == 8143 || spellInfo->Id == 8177
                            || spellInfo->Id == 5394 || spellInfo->Id == 2484 || spellInfo->Id == 108273
                            && spellInfo->GetRecoveryTime() > 0))
                            _player->RemoveSpellCooldown((itr++)->first, true);
                        else
                            ++itr;
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_call_of_the_elements_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_call_of_the_elements_SpellScript();
        }
};

// Called by Healing Wave - 331, Greater Healing Wave - 77472 and Healing Surge - 8004
// Called by Lightning Bolt - 403, Chain Lightning - 421, Earth Shock - 8042 and Stormstrike - 17364
// Called by Lightning Bolt - 45284, Chain Lightning - 45297
// Conductivity - 108282
class spell_sha_conductivity : public SpellScriptLoader
{
    public:
        spell_sha_conductivity() : SpellScriptLoader("spell_sha_conductivity") { }

        class spell_sha_conductivity_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_conductivity_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAura(SPELL_SHA_CONDUCTIVITY_TALENT))
                    {
                        Aura* aura = caster->GetAura(73920);
                        if(caster->m_SummonSlot[17] && aura)
                        {
                            int32 duration = aura->GetDuration() + 4000;
                            if(duration > 40000)
                                duration = 40000;
                            if(Creature* summon = caster->GetMap()->GetCreature(caster->m_SummonSlot[17]))
                                if(summon->ToTempSummon())
                                    summon->ToTempSummon()->InitStats(duration);
                            aura->SetDuration(duration);
                        }
                    }
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_sha_conductivity_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_conductivity_SpellScript();
        }
};

// Mail Specialization - 86529
class spell_sha_mail_specialization : public SpellScriptLoader
{
    public:
        spell_sha_mail_specialization() : SpellScriptLoader("spell_sha_mail_specialization") { }

        class spell_sha_mail_specialization_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_mail_specialization_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (!GetCaster())
                    return;

                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_SHAMAN_ELEMENTAL
                            || _player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_SHAMAN_RESTORATION)
                        _player->CastSpell(_player, SPELL_SHA_MAIL_SPECIALISATION_INT, true);
                    else if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_SHAMAN_ENHANCEMENT)
                        _player->CastSpell(_player, SPELL_SHA_MAIL_SPECIALIZATION_AGI, true);
                }
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (!GetCaster())
                    return;

                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->HasAura(SPELL_SHA_MAIL_SPECIALISATION_INT))
                        _player->RemoveAura(SPELL_SHA_MAIL_SPECIALISATION_INT);
                    else if (_player->HasAura(SPELL_SHA_MAIL_SPECIALIZATION_AGI))
                        _player->RemoveAura(SPELL_SHA_MAIL_SPECIALIZATION_AGI);
                }
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_sha_mail_specialization_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_sha_mail_specialization_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_mail_specialization_AuraScript();
        }
};

// Frost Shock - 8056
class spell_sha_frozen_power : public SpellScriptLoader
{
    public:
        spell_sha_frozen_power() : SpellScriptLoader("spell_sha_frozen_power") { }

        class spell_sha_frozen_power_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_frozen_power_SpellScript);

            bool Validate(SpellInfo const * SpellInfo)
            {
                if (!sSpellMgr->GetSpellInfo(8056))
                    return false;
                return true;
            }

            void HandleAfterHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (_player->HasAura(SPELL_SHA_FROZEN_POWER))
                            _player->CastSpell(target, SPELL_SHA_FROST_SHOCK_FREEZE, true);
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_sha_frozen_power_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_frozen_power_SpellScript();
        }
};

// Spirit Link - 98020 : triggered by 98017
// Spirit Link Totem
class spell_sha_spirit_link : public SpellScriptLoader
{
    public:
        spell_sha_spirit_link() : SpellScriptLoader("spell_sha_spirit_link") { }

        class spell_sha_spirit_link_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_spirit_link_SpellScript);

            void RemoveInvalidTargets(std::list<WorldObject*>& targets)
            {
                std::list<WorldObject*> removeList;
                for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                    if ((*itr) && (*itr)->GetTypeId() != TYPEID_PLAYER)
                        removeList.push_back(*itr);

                for (std::list<WorldObject*>::iterator iter = removeList.begin(); iter != removeList.end(); ++iter)
                    targets.remove(*iter);
            }
            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    std::list<TargetInfo>* memberList = GetSpell()->GetUniqueTargetInfo();
                    if(memberList->empty())
                        return;

                    float totalRaidHealthPct = 0;
                    for (std::list<TargetInfo>::iterator ihit = memberList->begin(); ihit != memberList->end(); ++ihit)
                    {
                        if(Unit* member = ObjectAccessor::GetUnit(*caster, ihit->targetGUID))
                            totalRaidHealthPct += member->GetHealthPct();
                    }
                    totalRaidHealthPct /= memberList->size() * 100.0f;
                    for (std::list<TargetInfo>::iterator ihit = memberList->begin(); ihit != memberList->end(); ++ihit)
                    {
                        if(Unit* member = ObjectAccessor::GetUnit(*caster, ihit->targetGUID))
                            member->SetHealth(uint32(totalRaidHealthPct * member->GetMaxHealth()));
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_spirit_link_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
                AfterCast += SpellCastFn(spell_sha_spirit_link_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_spirit_link_SpellScript();
        }
};

// Mana Tide - 16191
class spell_sha_mana_tide : public SpellScriptLoader
{
    public:
        spell_sha_mana_tide() : SpellScriptLoader("spell_sha_mana_tide") { }

        class spell_sha_mana_tide_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_mana_tide_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SHA_ELEMENTAL_BLAST))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                if(!GetCaster() || !GetCaster()->GetOwner())
                    return;

                if (Unit* target = GetHitUnit())
                {
                    if (Player* _player = GetCaster()->GetOwner()->ToPlayer())
                    {
                        AuraApplication* aura = target->GetAuraApplication(SPELL_SHA_MANA_TIDE, GetCaster()->GetGUID());

                        aura->GetBase()->GetEffect(0)->ChangeAmount(0);

                        int32 spirit = _player->GetStat(STAT_SPIRIT) * 2;

                        aura->GetBase()->GetEffect(0)->ChangeAmount(spirit);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_mana_tide_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_mana_tide_SpellScript();
        }
};

// Fire Nova - 1535
class spell_sha_fire_nova : public SpellScriptLoader
{
    public:
        spell_sha_fire_nova() : SpellScriptLoader("spell_sha_fire_nova") { }

        class spell_sha_fire_nova_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_fire_nova_SpellScript);

            bool Validate(SpellInfo const* SpellInfo)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SHA_FIRE_NOVA))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (target->HasAura(SPELL_SHA_FLAME_SHOCK))
                            _player->CastSpell(target, SPELL_SHA_FIRE_NOVA_TRIGGERED, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_fire_nova_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_fire_nova_SpellScript();
        }
};

// Unleash Elements - 73680
class spell_sha_unleash_elements : public SpellScriptLoader
{
    public:
        spell_sha_unleash_elements() : SpellScriptLoader("spell_sha_unleash_elements") { }

        class spell_sha_unleash_elements_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_unleash_elements_SpellScript);

            bool Validate(SpellInfo const * SpellInfo)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SHA_UNLEASH_ELEMENTS))
                    return false;
                return true;
            }

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetExplTargetUnit())
                    {
                        Item *weapons[2];
                        weapons[0] = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
                        weapons[1] = _player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND);

                        for (int i = 0; i < 2; i++)
                        {
                            if(!weapons[i])
                                continue;

                            uint32 unleashSpell = 0;
                            bool hostileTarget = _player->IsValidAttackTarget(target);
                            bool hostileSpell = true;
                            switch (weapons[i]->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
                            {
                                case 3345: // Earthliving Weapon
                                    unleashSpell = 73685; // Unleash Life
                                    hostileSpell = false;
                                    break;
                                case 5: // Flametongue Weapon
                                    unleashSpell = 73683; // Unleash Flame
                                    break;
                                case 2: // Frostbrand Weapon
                                    unleashSpell = 73682; // Unleash Frost
                                    break;
                                case 3021: // Rockbiter Weapon
                                    unleashSpell = 73684; // Unleash Earth
                                    break;
                                case 283: // Windfury Weapon
                                    unleashSpell = 73681; // Unleash Wind
                                    break;
                            }

                            if (hostileSpell && !hostileTarget)
                                continue; // don't allow to attack non-hostile targets. TODO: check this before cast

                            if (!hostileSpell && hostileTarget)
                                target = _player;   // heal ourselves instead of the enemy

                            if (unleashSpell)
                                _player->CastSpell(target, unleashSpell, true);

                            target = GetExplTargetUnit();

                            // If weapons are enchanted by same enchantment, only one should be unleashed
                            if (i == 0 && weapons[0] && weapons[1])
                                if (weapons[0]->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT) == weapons[1]->GetEnchantmentId(TEMP_ENCHANTMENT_SLOT))
                                    return;
                        }
                    }
                }
            }

            void Register()
            {
                //OnHit += SpellHitFn(spell_sha_unleash_elements_SpellScript::HandleOnHit);
                AfterCast += SpellCastFn(spell_sha_unleash_elements_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_unleash_elements_SpellScript();
        }
};

// Called by Lightning Bolt - 403 and Chain Lightning - 421
// Lightning Bolt (Mastery) - 45284 and Chain Lightning - 45297
// Rolling Thunder - 88764
class spell_sha_rolling_thunder : public SpellScriptLoader
{
    public:
        spell_sha_rolling_thunder() : SpellScriptLoader("spell_sha_rolling_thunder") { }

        class spell_sha_rolling_thunder_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_rolling_thunder_SpellScript)

            bool Validate(SpellInfo const * /*SpellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(403) || !sSpellMgr->GetSpellInfo(421))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (roll_chance_i(60) && _player->HasAura(88764))
                        {
                            if (Aura* lightningShield = _player->GetAura(324))
                            {
                                _player->CastSpell(_player, SPELL_SHA_ROLLING_THUNDER_ENERGIZE, true);

                                uint8 lsCharges = lightningShield->GetCharges();

                                if (lsCharges < 6)
                                {
                                    uint8 chargesBonus = _player->HasAura(SPELL_SHA_ITEM_T14_4P) ? 2 : 1;
                                    lightningShield->SetCharges(lsCharges + chargesBonus);
                                    lightningShield->RefreshDuration();
                                }
                                else if (lsCharges < 7)
                                {
                                    lightningShield->SetCharges(lsCharges + 1);
                                    lightningShield->RefreshDuration();
                                }

                                // refresh to handle Fulmination visual
                                lsCharges = lightningShield->GetCharges();

                                if (lsCharges >= 7 && _player->HasAura(SPELL_SHA_FULMINATION))
                                    _player->CastSpell(_player, SPELL_SHA_FULMINATION_INFO, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_rolling_thunder_SpellScript::HandleOnHit);
            }
        };

        SpellScript *GetSpellScript() const
        {
            return new spell_sha_rolling_thunder_SpellScript();
        }
};

// 88766 Fulmination handled in 8042 Earth Shock
class spell_sha_fulmination : public SpellScriptLoader
{
    public:
        spell_sha_fulmination() : SpellScriptLoader("spell_sha_fulmination") { }

        class spell_sha_fulmination_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_fulmination_SpellScript)

            void HandleDamage(SpellEffIndex eff)
            {
                // make caster cast a spell on a unit target of effect
                Unit *target = GetHitUnit();
                Unit *caster = GetCaster();
                if (!target || !caster)
                    return;

                Aura* lightningShield = caster->GetAura(324);
                if (!lightningShield)
                    return;

                uint8 lsCharges = lightningShield->GetCharges();
                if (lsCharges <= 1)
                    return;

                uint8 usedCharges = lsCharges - 1;

                if(SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SPELL_SHA_LIGHTNING_SHIELD_ORB_DAMAGE))
                {
                    int32 basePoints = caster->CalculateSpellDamage(target, spellInfo, 0);
                    int32 damage = int32(usedCharges * caster->SpellDamageBonusDone(target, spellInfo, basePoints, SPELL_DIRECT_DAMAGE, EFFECT_0));
                    if(Aura* aura = caster->GetAura(144998)) // Item - Shaman T16 Elemental 2P Bonus
                        aura->GetEffect(0)->SetAmount(4 * usedCharges);

                    caster->CastCustomSpell(target, SPELL_SHA_FULMINATION_TRIGGERED, &damage, NULL, NULL, false);
                    lightningShield->SetCharges(lsCharges - usedCharges);

                    caster->RemoveAura(SPELL_SHA_FULMINATION_INFO);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_sha_fulmination_SpellScript::HandleDamage, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript *GetSpellScript() const
        {
            return new spell_sha_fulmination_SpellScript();
        }
};

// Triggered by Flame Shock - 8050
// Lava Surge - 77756
class spell_sha_lava_surge : public SpellScriptLoader
{
    public:
        spell_sha_lava_surge() : SpellScriptLoader("spell_sha_lava_surge") { }

        class spell_sha_lava_surge_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_lava_surge_AuraScript);

            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                // 20% chance to reset the cooldown of Lavaburst and make the next to be instantly casted
                if (GetCaster())
                {
                    if (Player* _player = GetCaster()->ToPlayer())
                    {
                        if (_player->HasAura(77756))
                        {
                            if (roll_chance_i(20))
                            {
                                _player->CastSpell(_player, SPELL_SHA_LAVA_SURGE_CAST_TIME, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_lava_surge_AuraScript::HandleEffectPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_lava_surge_AuraScript();
        }
};

// Called by Stormstrike - 17364 and Lava Lash - 60103
// Static Shock - 51527
class spell_sha_static_shock : public SpellScriptLoader
{
    public:
        spell_sha_static_shock() : SpellScriptLoader("spell_sha_static_shock") { }

        class spell_sha_static_shock_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_static_shock_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SHA_LAVA_LASH) || !sSpellMgr->GetSpellInfo(SPELL_SHA_STORMSTRIKE))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    Unit* target = GetHitUnit();

                    // While have Lightning Shield active
                    if (target && _player->HasAura(324) && _player->HasAura(51527))
                    {
                        // Item - Shaman T9 Enhancement 2P Bonus (Static Shock)
                        if (_player->HasAura(67220))
                        {
                            if (roll_chance_i(48))
                            {
                                _player->CastSpell(target, SPELL_SHA_LIGHTNING_SHIELD_ORB_DAMAGE, true);
                            }
                        }
                        else
                        {
                            if (roll_chance_i(45))
                            {
                                _player->CastSpell(target, SPELL_SHA_LIGHTNING_SHIELD_ORB_DAMAGE, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_static_shock_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_static_shock_SpellScript();
        }
};

// Elemental Blast - 117014
class spell_sha_elemental_blast : public SpellScriptLoader
{
    public:
        spell_sha_elemental_blast() : SpellScriptLoader("spell_sha_elemental_blast") { }

        class spell_sha_elemental_blast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_elemental_blast_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SHA_ELEMENTAL_BLAST))
                    return false;
                return true;
            }

            void HandleOnCast()
            {
                if (Unit* caster = GetCaster())
                    if (Player* _player = caster->ToPlayer())
                    {
                        _player->CastSpell(_player, SPELL_SHA_ELEMENTAL_BLAST_RATING_BONUS, true);

                        if (AuraApplication* aura = _player->GetAuraApplication(SPELL_SHA_ELEMENTAL_BLAST_RATING_BONUS, _player->GetGUID()))
                        {
                            uint8 maxeffect = _player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_SHAMAN_ENHANCEMENT ? 3: 2;
                            int32 randomEffect = irand(0, maxeffect);

                            for (uint8 i = 0; i < 4; ++i)
                                if (randomEffect != i)
                                    if (Aura* auraBase = aura->GetBase())
                                        if (AuraEffect* eff = auraBase->GetEffect(i))
                                            eff->ChangeAmount(0);
                        }
                    }
            }

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                    if (Player* _player = caster->ToPlayer())
                    {
                        if (Unit* target = GetExplTargetUnit())
                        {
                            _player->CastSpell(target, SPELL_SHA_ELEMENTAL_BLAST_FROST_VISUAL, true);
                            _player->CastSpell(target, SPELL_SHA_ELEMENTAL_BLAST_NATURE_VISUAL, true);
                        }
                    }
            }

            void Register()
            {
                OnCast += SpellCastFn(spell_sha_elemental_blast_SpellScript::HandleOnCast);
                AfterCast += SpellCastFn(spell_sha_elemental_blast_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_elemental_blast_SpellScript();
        }
};

// Earthquake : Ticks - 77478
class spell_sha_earthquake_tick : public SpellScriptLoader
{
    public:
        spell_sha_earthquake_tick() : SpellScriptLoader("spell_sha_earthquake_tick") { }

        class spell_sha_earthquake_tick_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_earthquake_tick_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SHA_EARTHQUAKE_TICK))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                // With a 10% chance of knocking down affected targets
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (roll_chance_i(10))
                            _player->CastSpell(target, SPELL_SHA_EARTHQUAKE_KNOCKING_DOWN, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_earthquake_tick_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_earthquake_tick_SpellScript();
        }
};

// Earthquake - 61882
class spell_sha_earthquake : public SpellScriptLoader
{
    public:
        spell_sha_earthquake() : SpellScriptLoader("spell_sha_earthquake") { }

        class spell_sha_earthquake_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_earthquake_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* caster = GetCaster())
                    if (DynamicObject* dynObj = caster->GetDynObject(SPELL_SHA_EARTHQUAKE))
                        caster->CastSpell(dynObj->GetPositionX(), dynObj->GetPositionY(), dynObj->GetPositionZ(), SPELL_SHA_EARTHQUAKE_TICK, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_earthquake_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_earthquake_AuraScript();
        }
};

// Healing Rain - 73920
class spell_sha_healing_rain : public SpellScriptLoader
{
    public:
        spell_sha_healing_rain() : SpellScriptLoader("spell_sha_healing_rain") { }

        class spell_sha_healing_rain_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_healing_rain_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* caster = GetCaster())
                {
                    if(caster->m_SummonSlot[17])
                        if(Creature* summon = caster->GetMap()->GetCreature(caster->m_SummonSlot[17]))
                            caster->CastSpell(summon->GetPositionX(), summon->GetPositionY(), summon->GetPositionZ(), SPELL_SHA_HEALING_RAIN_TICK, true);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_healing_rain_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        class spell_sha_healing_rain_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_healing_rain_SpellScript);

            void HandleOnCast()
            {
                if(Unit* caster = GetCaster())
                {
                    Position const* sumpos = GetExplTargetDest();
                    if(TempSummon* summon = caster->GetMap()->SummonCreature(73400, *sumpos, NULL, GetSpellInfo()->GetDuration(), caster))
                    {
                        if(caster->m_SummonSlot[17])
                        {
                            if(Creature* tempsummon = caster->GetMap()->GetCreature(caster->m_SummonSlot[17]))
                                tempsummon->DespawnOrUnsummon(500);
                        }
                        caster->m_SummonSlot[17] = summon->GetGUID();
                    }
                }
            }

            void Register()
            {
                OnCast += SpellCastFn(spell_sha_healing_rain_SpellScript::HandleOnCast);
            }

        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_healing_rain_SpellScript();
        }

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_healing_rain_AuraScript();
        }
};

// Ascendance - 114049
class spell_sha_ascendance : public SpellScriptLoader
{
    public:
        spell_sha_ascendance() : SpellScriptLoader("spell_sha_ascendance") { }

        class spell_sha_ascendance_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_ascendance_SpellScript);

            bool Validate(SpellInfo const* SpellInfo)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SHA_ASCENDANCE))
                    return false;
                return true;
            }

            SpellCastResult CheckCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_NONE)
                    {
                        SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_MUST_SELECT_TALENT_SPECIAL);
                        return SPELL_FAILED_CUSTOM_ERROR;
                    }

                    return SPELL_CAST_OK;
                }

                return SPELL_CAST_OK;
            }

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    switch(_player->GetSpecializationId(_player->GetActiveSpec()))
                    {
                        case SPEC_SHAMAN_ELEMENTAL:
                            _player->CastSpell(_player, SPELL_SHA_ASCENDANCE_ELEMENTAL, true);
                            break;
                        case SPEC_SHAMAN_ENHANCEMENT:
                            _player->CastSpell(_player, SPELL_SHA_ASCENDANCE_ENHANCED, true);

                            if (_player->HasSpellCooldown(SPELL_SHA_STORMSTRIKE))
                                _player->RemoveSpellCooldown(SPELL_SHA_STORMSTRIKE, true);
                            break;
                        case SPEC_SHAMAN_RESTORATION:
                            _player->CastSpell(_player, SPELL_SHA_ASCENDANCE_RESTORATION, true);
                            break;
                        default:
                            break;
                    }
                }
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_sha_ascendance_SpellScript::CheckCast);
                AfterCast += SpellCastFn(spell_sha_ascendance_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_ascendance_SpellScript();
        }
};

class EarthenPowerTargetSelector
{
    public:
        EarthenPowerTargetSelector() { }

        bool operator() (WorldObject* target)
        {
            if (!target->ToUnit())
                return true;

            if (!target->ToUnit()->HasAuraWithMechanic(1 << MECHANIC_SNARE))
                return true;

            return false;
        }
};

class spell_sha_bloodlust : public SpellScriptLoader
{
    public:
        spell_sha_bloodlust() : SpellScriptLoader("spell_sha_bloodlust") { }

        class spell_sha_bloodlust_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_bloodlust_SpellScript);

            bool Validate(SpellInfo const* /*SpellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SHAMAN_SPELL_SATED))
                    return false;
                return true;
            }

            void RemoveInvalidTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if(Trinity::UnitAuraCheck(true, SHAMAN_SPELL_SATED));
                targets.remove_if(Trinity::UnitAuraCheck(true, HUNTER_SPELL_INSANITY));
                targets.remove_if(Trinity::UnitAuraCheck(true, MAGE_SPELL_TEMPORAL_DISPLACEMENT));
                targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_DRUMS_OF_RAGE));
            }

            void ApplyDebuff()
            {
                if (Unit* target = GetHitUnit())
                    target->CastSpell(target, SHAMAN_SPELL_SATED, true);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_bloodlust_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_bloodlust_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_CASTER_AREA_RAID);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_bloodlust_SpellScript::RemoveInvalidTargets, EFFECT_2, TARGET_UNIT_CASTER_AREA_RAID);
                AfterHit += SpellHitFn(spell_sha_bloodlust_SpellScript::ApplyDebuff);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_bloodlust_SpellScript();
        }
};

class spell_sha_heroism : public SpellScriptLoader
{
    public:
        spell_sha_heroism() : SpellScriptLoader("spell_sha_heroism") { }

        class spell_sha_heroism_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_heroism_SpellScript);

            bool Validate(SpellInfo const* /*SpellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SHAMAN_SPELL_EXHAUSTION))
                    return false;
                return true;
            }

            void RemoveInvalidTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if(Trinity::UnitAuraCheck(true, SHAMAN_SPELL_EXHAUSTION));
                targets.remove_if(Trinity::UnitAuraCheck(true, HUNTER_SPELL_INSANITY));
                targets.remove_if(Trinity::UnitAuraCheck(true, MAGE_SPELL_TEMPORAL_DISPLACEMENT));
                targets.remove_if(Trinity::UnitAuraCheck(true, SPELL_DRUMS_OF_RAGE));
            }

            void ApplyDebuff()
            {
                if (Unit* target = GetHitUnit())
                    GetCaster()->CastSpell(target, SHAMAN_SPELL_EXHAUSTION, true);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_heroism_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_heroism_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_CASTER_AREA_RAID);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_heroism_SpellScript::RemoveInvalidTargets, EFFECT_2, TARGET_UNIT_CASTER_AREA_RAID);
                AfterHit += SpellHitFn(spell_sha_heroism_SpellScript::ApplyDebuff);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_heroism_SpellScript();
        }
};

enum AncestralAwakeningProc
{
    SPELL_ANCESTRAL_AWAKENING_PROC   = 52752,
};

class spell_sha_ancestral_awakening_proc : public SpellScriptLoader
{
    public:
        spell_sha_ancestral_awakening_proc() : SpellScriptLoader("spell_sha_ancestral_awakening_proc") { }

        class spell_sha_ancestral_awakening_proc_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_ancestral_awakening_proc_SpellScript);

            bool Validate(SpellInfo const* /*SpellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_ANCESTRAL_AWAKENING_PROC))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                int32 damage = GetEffectValue();
                if (GetCaster() && GetHitUnit())
                    GetCaster()->CastCustomSpell(GetHitUnit(), SPELL_ANCESTRAL_AWAKENING_PROC, &damage, NULL, NULL, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_sha_ancestral_awakening_proc_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_ancestral_awakening_proc_SpellScript();
        }
};

// Lava Lash - 60103
class spell_sha_lava_lash : public SpellScriptLoader
{
    public:
        spell_sha_lava_lash() : SpellScriptLoader("spell_sha_lava_lash") { }

        class spell_sha_lava_lash_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_lava_lash_SpellScript)

            int32 searingFlameAmount;

            bool Load()
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        int32 hitDamage = GetHitDamage();
                        if (_player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_OFFHAND))
                        {
                            // Damage is increased by 40% if your off-hand weapon is enchanted with Flametongue.
                            if (_player->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_SHAMAN, 0x200000, 0, 0))
                                AddPct(hitDamage, 40);

                            // Your Lava Lash ability will consume Searing Flame effect, dealing 20% increased damage for each application
                            if (AuraApplication* searingFlame = _player->GetAuraApplication(SPELL_SHA_SEARING_FLAMES_DAMAGE_DONE))
                            {
                                searingFlameAmount = searingFlame->GetBase()->GetStackAmount();
                                searingFlameAmount *= 8;
                                AddPct(hitDamage, searingFlameAmount);

                                _player->RemoveAura(SPELL_SHA_SEARING_FLAMES_DAMAGE_DONE);
                            }

                            SetHitDamage(hitDamage);

                            // Spreading your Flame shock from the target to up to four ennemies within 12 yards
                            // Effect desactivated if has Glyph of Lava Lash
                            if (!_player->HasAura(55444))
                            {
                                if (target->HasAura(SPELL_SHA_FLAME_SHOCK))
                                {
                                    std::list<Unit*> targetList;

                                    _player->GetAttackableUnitListInRange(targetList, 12.0f);

                                    int32 hitTargets = 0;

                                    for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                                    {
                                        Unit* unit = *itr;
                                        if (!_player->IsValidAttackTarget(unit))
                                            continue;

                                        if (unit->GetGUID() == target->GetGUID())
                                            continue;

                                        if (unit->GetGUID() == _player->GetGUID())
                                            continue;

                                        if (hitTargets >= 4)
                                            continue;

                                        double cooldownDelay = _player->GetSpellCooldownDelay(SPELL_SHA_FLAME_SHOCK);
                                        if (_player->HasSpellCooldown(SPELL_SHA_FLAME_SHOCK))
                                            _player->RemoveSpellCooldown(SPELL_SHA_FLAME_SHOCK, true);

                                        _player->CastSpell(unit, SPELL_SHA_FLAME_SHOCK, true);
                                        _player->AddSpellCooldown(SPELL_SHA_FLAME_SHOCK, 0, getPreciseTime() + (double)cooldownDelay);
                                        hitTargets++;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_lava_lash_SpellScript::HandleOnHit);
            }

        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_lava_lash_SpellScript();
        }
};

// Chain Heal - 1064
class spell_sha_chain_heal : public SpellScriptLoader
{
    public:
        spell_sha_chain_heal() : SpellScriptLoader("spell_sha_chain_heal") { }

        class spell_sha_chain_heal_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_chain_heal_SpellScript);

            bool Load()
            {
                firstHeal = true;
                riptide = false;
                return true;
            }

            void HandleHeal(SpellEffIndex /*effIndex*/)
            {
                if (firstHeal)
                {
                    // Check if the target has Riptide
                    if (AuraEffect* aurEff = GetHitUnit()->GetAuraEffect(SPELL_AURA_PERIODIC_HEAL, SPELLFAMILY_SHAMAN, 0, 0, 0x10, GetCaster()->GetGUID()))
                    {
                        riptide = true;
                    }
                    firstHeal = false;
                }
                // Riptide increases the Chain Heal effect by 25%
                if (riptide)
                    SetHitHeal(GetHitHeal() * 1.25f);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_sha_chain_heal_SpellScript::HandleHeal, EFFECT_0, SPELL_EFFECT_HEAL);
            }

            bool firstHeal;
            bool riptide;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_chain_heal_SpellScript();
        }
};

// 73899 Primal strike
class spell_shaman_primal_strike : public SpellScriptLoader
{
public:
    spell_shaman_primal_strike() : SpellScriptLoader("spell_shaman_primal_strike") { }

    class spell_shaman_primal_strike_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_shaman_primal_strike_SpellScript)

        void HandleDummy(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetCaster())
            {
                if (caster->GetTypeId() == TYPEID_PLAYER)
                    caster->ToPlayer()->KilledMonsterCredit(44548, 0);
            }
        }

        void Register()
        {
            OnEffectHit += SpellEffectFn(spell_shaman_primal_strike_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_WEAPON_DAMAGE);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_shaman_primal_strike_SpellScript();
    }
};

class spell_shaman_healing_tide: public SpellScriptLoader
{
    public:
        spell_shaman_healing_tide() : SpellScriptLoader("spell_shaman_healing_tide") { }

        class spell_shaman_healing_tide_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_shaman_healing_tide_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                uint32 count = 5;
                if (Unit* caster = GetCaster())
                    if(caster->GetMap() && (caster->GetMap()->GetSpawnMode() == MAN25_HEROIC_DIFFICULTY || caster->GetMap()->GetSpawnMode() == MAN25_DIFFICULTY))
                        count = 12;

                for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end();)
                {
                    if ((*itr)->ToUnit() && (*itr)->ToUnit()->GetHealthPct() != 100)
                        ++itr;
                    else
                        targets.erase(itr++);
                }
                if (targets.size() > count)
                    targets.resize(count);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_shaman_healing_tide_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_shaman_healing_tide_SpellScript();
        }
};

class spell_shaman_totemic_projection : public SpellScriptLoader
{
    public:
        spell_shaman_totemic_projection() : SpellScriptLoader("spell_shaman_totemic_projection") { }

        class spell_shaman_totemic_projection_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_shaman_totemic_projection_SpellScript);

            void SelectDest()
            {
                Unit* caster = GetCaster();
                if(!caster || !caster->GetMap())
                    return;

                Position const* sumpos = GetExplTargetDest();
                TempSummon* summon = caster->GetMap()->SummonCreature(47319, *sumpos, NULL, 0, caster, GetSpellInfo()->Id);
                if(!summon)
                    return;

                if(Creature* totem = caster->GetMap()->GetCreature(caster->m_SummonSlot[1]))
                {
                    Position pos;
                    summon->GetFirstCollisionPosition(pos, 2.5f, static_cast<float>(-M_PI/4));
                    //totem->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), totem->GetOrientation());
                    totem->UpdatePosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), totem->GetOrientation(), false);
                    totem->GetMotionMaster()->MovePoint(0, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), false, 60.0f);
                    //totem->SendMovementFlagUpdate();
                }
                if(Creature* totem = caster->GetMap()->GetCreature(caster->m_SummonSlot[2]))
                {
                    Position pos;
                    summon->GetFirstCollisionPosition(pos, 2.5f, static_cast<float>(-3*M_PI/4));
                    //totem->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), totem->GetOrientation());
                    totem->UpdatePosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), totem->GetOrientation(), false);
                    totem->GetMotionMaster()->MovePoint(0, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), false, 60.0f);
                    //totem->SendMovementFlagUpdate();
                }
                if(Creature* totem = caster->GetMap()->GetCreature(caster->m_SummonSlot[3]))
                {
                    Position pos;
                    summon->GetFirstCollisionPosition(pos, 2.5f, static_cast<float>(3*M_PI/4));
                    //totem->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), totem->GetOrientation());
                    totem->UpdatePosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), totem->GetOrientation(), false);
                    totem->GetMotionMaster()->MovePoint(0, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), false, 60.0f);
                    //totem->SendMovementFlagUpdate();
                }
                if(Creature* totem = caster->GetMap()->GetCreature(caster->m_SummonSlot[4]))
                {
                    Position pos;
                    summon->GetFirstCollisionPosition(pos, 2.5f, static_cast<float>(M_PI/4));
                    //totem->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), totem->GetOrientation());
                    totem->UpdatePosition(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), totem->GetOrientation(), false);
                    totem->GetMotionMaster()->MovePoint(0, pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), false, 60.0f);
                    //totem->SendMovementFlagUpdate();
                }
            }

            void Register()
            {
                BeforeCast += SpellCastFn(spell_shaman_totemic_projection_SpellScript::SelectDest);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_shaman_totemic_projection_SpellScript();
        }
};

class spell_sha_ancestral_vigor : public SpellScriptLoader
{
    public:
        spell_sha_ancestral_vigor() : SpellScriptLoader("spell_sha_ancestral_vigor") { }

        class spell_sha_ancestral_vigor_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_ancestral_vigor_AuraScript);

            void CalculateAmount(AuraEffect const* aurEff, int32& amount, bool& /*canBeRecalculated*/)
            {
                amount += aurEff->GetOldBaseAmount();
                if (Unit* target = aurEff->GetSaveTarget())
                {
                    int32 cap = int32(target->GetMaxHealth() * 0.1f);
                    if(amount > cap)
                        amount = cap;
                }
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_ancestral_vigor_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_MOD_INCREASE_HEALTH);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_ancestral_vigor_AuraScript();
        }
};

class spell_sha_maelstrom_weapon : public SpellScriptLoader
{
    public:
        spell_sha_maelstrom_weapon() : SpellScriptLoader("spell_sha_maelstrom_weapon") { }

        class spell_sha_maelstrom_weapon_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_maelstrom_weapon_AuraScript);

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32& amount, bool& /*canBeRecalculated*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAura(89646))
                        amount = 20;
                }
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_sha_maelstrom_weapon_AuraScript::CalculateAmount, EFFECT_2, SPELL_AURA_ADD_PCT_MODIFIER);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_maelstrom_weapon_AuraScript();
        }
};

// Earth Shield - 379
class spell_sha_earth_shield : public SpellScriptLoader
{
    public:
        spell_sha_earth_shield() : SpellScriptLoader("spell_sha_earth_shield") { }

        class spell_sha_earth_shield_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_earth_shield_SpellScript)

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        int32 bp = GetHitHeal() * 3;
                        if (caster->HasAura(145378)) //Item - Shaman T16 Restoration 2P Bonus
                            target->CastCustomSpell(target, 145379, &bp, NULL, NULL, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_earth_shield_SpellScript::HandleOnHit);
            }

        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_earth_shield_SpellScript();
        }
};

// 144999 - Elemental Discharge
class spell_sha_elemental_discharge : public SpellScriptLoader
{
    public:
        spell_sha_elemental_discharge() : SpellScriptLoader("spell_sha_elemental_discharge") { }

        class spell_sha_elemental_discharge_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_elemental_discharge_AuraScript);

            void CalculateMaxDuration(int32 & duration)
            {
                if (Unit* caster = GetCaster())
                    if (AuraEffect const* aurEff = caster->GetAuraEffect(144998, EFFECT_0))
                        duration = int32(aurEff->GetAmount() / 2) * 1000;
            }

            void Register()
            {
                DoCalcMaxDuration += AuraCalcMaxDurationFn(spell_sha_elemental_discharge_AuraScript::CalculateMaxDuration);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_elemental_discharge_AuraScript();
        }
};

// Purge - 370
class spell_sha_purge : public SpellScriptLoader
{
    public:
        spell_sha_purge() : SpellScriptLoader("spell_sha_purge") { }

        class spell_sha_purge_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_purge_SpellScript);

            void HandleAfterHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if(GetSpell()->GetCountDispel() && caster->HasAura(147762) && caster->HasAura(77223)) //Glyph of Purging
                        caster->CastSpell(caster, 53817, true);
                }
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_sha_purge_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_purge_SpellScript();
        }
};

// Glyph of Cleansing Waters - 55445, Call - 77130, 51886
class spell_sha_cleansing_waters : public SpellScriptLoader
{
    public:
        spell_sha_cleansing_waters() : SpellScriptLoader("spell_sha_cleansing_waters") { }

        class spell_sha_cleansing_waters_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_cleansing_waters_SpellScript);

            void HandleAfterCast()
            {
                Unit* caster = GetCaster();
                Unit* target = GetExplTargetUnit();
                if(!caster || !target)
                    return;

                if(GetSpell()->GetCountDispel())
                {
                    if (AuraEffect const* aurEff = caster->GetAuraEffect(55445, EFFECT_0))
                    {
                        int32 bp0 = CalculatePct(target->GetMaxHealth(), aurEff->GetAmount());
                        caster->CastCustomSpell(target, 86961, &bp0, NULL, NULL, true);
                    }
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_sha_cleansing_waters_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_cleansing_waters_SpellScript();
        }
};

// Astral Recall - 556
class spell_sha_astral_recall : public SpellScriptLoader
{
    public:
        spell_sha_astral_recall() : SpellScriptLoader("spell_sha_astral_recall") { }

        class spell_sha_astral_recall_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_astral_recall_SpellScript);

            void HandleAfterCast()
            {
                Unit* caster = GetCaster();
                if(!caster)
                    return;

                if (caster->HasAura(147787)) //Glyph of Astral Fixation
                {
                    if (Player::TeamForRace(caster->getRace()) == HORDE)
                        caster->CastSpell(caster, 147902, true);
                    else
                        caster->CastSpell(caster, 147901, true);
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_sha_astral_recall_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_astral_recall_SpellScript();
        }
};

// Glyph of Elemental Familiars - 148118
class spell_sha_elemental_familiars : public SpellScriptLoader
{
    public:
        spell_sha_elemental_familiars() : SpellScriptLoader("spell_sha_elemental_familiars") { }

        class spell_sha_elemental_familiars_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_elemental_familiars_SpellScript);

            void HandleAfterCast()
            {
                if(Unit* caster = GetCaster())
                {
                    Position const* sumpos = GetExplTargetDest();
                    uint32 entry[] = { 73556, 73559, 73560 };
                    SummonPropertiesEntry const* properties = sSummonPropertiesStore.LookupEntry(3221);
                    if(TempSummon* summon = caster->GetMap()->SummonCreature(entry[urand(0, 2)], *sumpos, properties, 600000, caster))
                    {
                        if(caster->m_SummonSlot[17])
                        {
                            if(Creature* tempsummon = caster->GetMap()->GetCreature(caster->m_SummonSlot[17]))
                                tempsummon->DespawnOrUnsummon(500);
                        }
                        caster->m_SummonSlot[17] = summon->GetGUID();
                    }
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_sha_elemental_familiars_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_elemental_familiars_SpellScript();
        }
};

// Grounding Totem - 89523
class spell_sha_grounding_totem : public SpellScriptLoader
{
    public:
        spell_sha_grounding_totem() : SpellScriptLoader("spell_sha_grounding_totem") { }

        class spell_sha_grounding_totem_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_grounding_totem_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                    if ((removeMode == AURA_REMOVE_BY_DROP_CHARGERS || removeMode == AURA_REMOVE_BY_DEFAULT) && caster->ToTotem())
                        caster->setDeathState(JUST_DIED);
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_sha_grounding_totem_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_REFLECT_SPELLS, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_grounding_totem_AuraScript();
        }
};

void AddSC_shaman_spell_scripts()
{
    new spell_sha_prowl();
    new spell_sha_solar_beam();
    new spell_sha_glyph_of_shamanistic_rage();
    new spell_sha_glyph_of_lakestrider();
    new spell_sha_call_of_the_elements();
    new spell_sha_conductivity();
    new spell_sha_mail_specialization();
    new spell_sha_frozen_power();
    new spell_sha_spirit_link();
    new spell_sha_mana_tide();
    new spell_sha_fire_nova();
    new spell_sha_unleash_elements();
    new spell_sha_rolling_thunder();
    new spell_sha_fulmination();
    new spell_sha_lava_surge();
    new spell_sha_static_shock();
    new spell_sha_elemental_blast();
    new spell_sha_earthquake_tick();
    new spell_sha_earthquake();
    new spell_sha_healing_rain();
    new spell_sha_ascendance();
    new spell_sha_bloodlust();
    new spell_sha_heroism();
    new spell_sha_ancestral_awakening_proc();
    new spell_sha_lava_lash();
    new spell_sha_chain_heal();
    new spell_shaman_primal_strike();
    new spell_shaman_healing_tide();
    new spell_sha_ancestral_vigor();
    new spell_sha_maelstrom_weapon();
    new spell_sha_earth_shield();
    new spell_sha_elemental_discharge();
    new spell_sha_purge();
    new spell_sha_cleansing_waters();
    new spell_sha_astral_recall();
    new spell_sha_elemental_familiars();
    new spell_sha_grounding_totem();
    new spell_shaman_totemic_projection();
}
