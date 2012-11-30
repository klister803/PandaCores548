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
    SHAMAN_SPELL_GLYPH_OF_MANA_TIDE         = 55441,
    SHAMAN_SPELL_MANA_TIDE_TOTEM            = 39609,
    SHAMAN_SPELL_FIRE_NOVA_R1               = 1535,
    SHAMAN_SPELL_FIRE_NOVA_TRIGGERED_R1     = 8349,
    SHAMAN_SPELL_SATED                      = 57724,
    SHAMAN_SPELL_EXHAUSTION                 = 57723,
    HUNTER_SPELL_INSANITY                   = 95809,
    MAGE_SPELL_TEMPORAL_DISPLACEMENT        = 80354,
    SHAMAN_SPELL_STORM_EARTH_AND_FIRE       = 51483,
    EARTHBIND_TOTEM_SPELL_EARTHGRAB         = 64695,
    // For Earthen Power
    SHAMAN_TOTEM_SPELL_EARTHBIND_TOTEM      = 6474,
    SHAMAN_TOTEM_SPELL_EARTHEN_POWER        = 59566,
    SHAMAN_BIND_SIGHT                       = 6277,
    ICON_ID_SHAMAN_LAVA_FLOW                = 3087,
    SHAMAN_LAVA_FLOWS_R1                    = 51480,
    SHAMAN_LAVA_FLOWS_TRIGGERED_R1          = 64694,
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
    SPELL_SHA_LAVA_LASH                     = 60103,
    SPELL_SHA_FLAME_SHOCK                   = 8050,
    SPELL_SHA_STORMSTRIKE                   = 17364,
    SPELL_SHA_LIGHTNING_SHIELD_ORB_DAMAGE   = 26364,
    SPELL_SHA_HEALING_STREAM                = 52042,
    SPELL_SHA_GLYPH_OF_HEALING_STREAM       = 119423,
    SPELL_SHA_LAVA_SURGE_CAST_TIME          = 77762,
    SPELL_SHA_FULMINATION                   = 88766,
    SPELL_SHA_FULMINATION_TRIGGERED         = 88767,
    SPELL_SHA_FULMINATION_INFO              = 95774,
    SPELL_SHA_ROLLING_THUNDER_ENERGIZE      = 88765,
    SPELL_SHA_UNLEASH_ELEMENTS              = 73680
};

// Unleash Elements - 73680
class spell_sha_unleash_elements : public SpellScriptLoader
{
    public:
        spell_sha_unleash_elements() : SpellScriptLoader("spell_sha_unleash_elements") { }

        class spell_sha_unleash_elements_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_unleash_elements_SpellScript);

            bool Validate(SpellEntry const * spellEntry)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SHA_UNLEASH_ELEMENTS))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
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
                                return; // don't allow to attack non-hostile targets. TODO: check this before cast

                            if (!hostileSpell && hostileTarget)
                                target = _player;   // heal ourselves instead of the enemy

                            if (unleashSpell)
                                _player->CastSpell(target, unleashSpell, false);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_unleash_elements_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_unleash_elements_SpellScript();
        }
};

// Called by Lightning Bolt - 403 and Chain Lightning - 421
// Rolling Thunder - 88765
class spell_sha_rolling_thunder : public SpellScriptLoader
{
public:
    spell_sha_rolling_thunder() : SpellScriptLoader("spell_sha_rolling_thunder") { }

    class spell_sha_rolling_thunder_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_sha_rolling_thunder_SpellScript)

        bool Validate(SpellEntry const * /*spellEntry*/)
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
                    if (roll_chance_i(60))
                    {
                        if (Aura * lightningShield = _player->GetAura(324))
                        {
                            _player->CastSpell(_player, SPELL_SHA_ROLLING_THUNDER_ENERGIZE, true);

                            uint8 lsCharges = lightningShield->GetCharges();
                            if (lsCharges < 7)
                                lightningShield->SetCharges(lsCharges + 1);
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

        bool Validate(SpellEntry const * /*spellEntry*/)
        {
            if (!sSpellMgr->GetSpellInfo(SPELL_SHA_FULMINATION) || !sSpellMgr->GetSpellInfo(SPELL_SHA_FULMINATION_TRIGGERED) || !sSpellMgr->GetSpellInfo(SPELL_SHA_FULMINATION_INFO))
                return false;
            return true;
        }

        void HandleFulmination(SpellEffIndex /*effIndex*/)
        {
            // make caster cast a spell on a unit target of effect
            Unit *target = GetHitUnit();
            Unit *caster = GetCaster();
            if(!target || !caster)
                return;

            AuraEffect *fulminationAura = caster->GetDummyAuraEffect(SPELLFAMILY_SHAMAN, 2010, 0);
            if (!fulminationAura)
                return;

            Aura * lightningShield = caster->GetAura(324);
            if(!lightningShield)
                return;

            uint8 lsCharges = lightningShield->GetCharges();
            if(lsCharges <= 1)
                return;

            uint8 usedCharges = lsCharges - 1;

            SpellEntry const* spellInfo = sSpellStore.LookupEntry(SPELL_SHA_LIGHTNING_SHIELD_ORB_DAMAGE);
            int32 basePoints = caster->CalculateSpellDamage(target, GetSpellInfo(), 0);
            uint32 damage = usedCharges * caster->SpellDamageBonusDone(target, GetSpellInfo(), basePoints, SPELL_DIRECT_DAMAGE);

            caster->CastCustomSpell(SPELL_SHA_FULMINATION_TRIGGERED, SPELLVALUE_BASE_POINT0, damage, target, true, NULL, fulminationAura);
            lightningShield->SetCharges(lsCharges - usedCharges);
        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_sha_fulmination_SpellScript::HandleFulmination, EFFECT_FIRST_FOUND, SPELL_EFFECT_ANY);
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
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->HasAura(77756))
                    {
                        if (roll_chance_i(20))
                        {
                            _player->CastSpell(_player, SPELL_SHA_LAVA_SURGE_CAST_TIME, true);
                            _player->RemoveSpellCooldown(51505, true);
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

// Healing Stream - 52042
class spell_sha_healing_stream : public SpellScriptLoader
{
    public:
        spell_sha_healing_stream() : SpellScriptLoader("spell_sha_healing_stream") { }

        class spell_sha_healing_stream_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_healing_stream_SpellScript);

            bool Validate()
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SHA_HEALING_STREAM))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->GetOwner()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        // Glyph of Healing Stream Totem
                        if (target->GetGUID() != _player->GetGUID() && _player->HasAura(55456))
                            _player->CastSpell(target, SPELL_SHA_GLYPH_OF_HEALING_STREAM, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_healing_stream_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_healing_stream_SpellScript();
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

            bool Validate()
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
                    if (target && _player->HasAura(324))
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

            bool Validate()
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SHA_ELEMENTAL_BLAST))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    int32 randomEffect = irand(0, 2);

                    _player->CastSpell(_player, SPELL_SHA_ELEMENTAL_BLAST_RATING_BONUS, true);

                    AuraApplication* aura = _player->GetAuraApplication(SPELL_SHA_ELEMENTAL_BLAST_RATING_BONUS, _player->GetGUID());

                    if (aura)
                    {
                        switch (randomEffect)
                        {
                            case 0:
                            {
                                aura->GetBase()->GetEffect(1)->ChangeAmount(0);
                                aura->GetBase()->GetEffect(2)->ChangeAmount(0);
                                break;
                            }
                            case 1:
                            {
                                aura->GetBase()->GetEffect(0)->ChangeAmount(0);
                                aura->GetBase()->GetEffect(2)->ChangeAmount(0);
                                break;
                            }
                            case 2:
                            {
                                aura->GetBase()->GetEffect(0)->ChangeAmount(0);
                                aura->GetBase()->GetEffect(1)->ChangeAmount(0);
                                break;
                            }
                            default:
                                break;
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_sha_elemental_blast_SpellScript::HandleOnHit);
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

            bool Validate()
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
                if (DynamicObject* dynObj = GetCaster()->GetDynObject(SPELL_SHA_EARTHQUAKE))
                    GetCaster()->CastSpell(dynObj->GetPositionX(), dynObj->GetPositionY(), dynObj->GetPositionZ(), SPELL_SHA_EARTHQUAKE_TICK, true);
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
                if (DynamicObject* dynObj = GetCaster()->GetDynObject(SPELL_SHA_HEALING_RAIN))
                    GetCaster()->CastSpell(dynObj->GetPositionX(), dynObj->GetPositionY(), dynObj->GetPositionZ(), SPELL_SHA_HEALING_RAIN_TICK, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_healing_rain_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

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

            bool Validate(SpellInfo const* spellEntry)
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
                        return SPELL_FAILED_CUSTOM_ERROR;

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

// 1535 Fire Nova
class spell_sha_fire_nova : public SpellScriptLoader
{
    public:
        spell_sha_fire_nova() : SpellScriptLoader("spell_sha_fire_nova") { }

        class spell_sha_fire_nova_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_fire_nova_SpellScript);

            bool Validate(SpellInfo const* spellEntry)
            {
                if (!sSpellMgr->GetSpellInfo(SHAMAN_SPELL_FIRE_NOVA_R1) || sSpellMgr->GetFirstSpellInChain(SHAMAN_SPELL_FIRE_NOVA_R1) != sSpellMgr->GetFirstSpellInChain(spellEntry->Id))
                    return false;

                uint8 rank = sSpellMgr->GetSpellRank(spellEntry->Id);
                if (!sSpellMgr->GetSpellWithRank(SHAMAN_SPELL_FIRE_NOVA_TRIGGERED_R1, rank, true))
                    return false;
                return true;
            }

            SpellCastResult CheckFireTotem()
            {
                // fire totem
                if (!GetCaster()->m_SummonSlot[1])
                {
                    SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_MUST_HAVE_FIRE_TOTEM);
                    return SPELL_FAILED_CUSTOM_ERROR;
                }

                return SPELL_CAST_OK;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    uint8 rank = sSpellMgr->GetSpellRank(GetSpellInfo()->Id);
                    if (uint32 spellId = sSpellMgr->GetSpellWithRank(SHAMAN_SPELL_FIRE_NOVA_TRIGGERED_R1, rank))
                    {
                        Creature* totem = caster->GetMap()->GetCreature(caster->m_SummonSlot[1]);
                        if (totem && totem->isTotem())
                            caster->CastSpell(totem, spellId, true);
                    }
                }
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_sha_fire_nova_SpellScript::CheckFireTotem);
                OnEffectHitTarget += SpellEffectFn(spell_sha_fire_nova_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_fire_nova_SpellScript();
        }
};

// 39610 Mana Tide Totem
class spell_sha_mana_tide_totem : public SpellScriptLoader
{
    public:
        spell_sha_mana_tide_totem() : SpellScriptLoader("spell_sha_mana_tide_totem") { }

        class spell_sha_mana_tide_totem_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_mana_tide_totem_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(SHAMAN_SPELL_GLYPH_OF_MANA_TIDE) || !sSpellMgr->GetSpellInfo(SHAMAN_SPELL_MANA_TIDE_TOTEM))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                    if (Unit* unitTarget = GetHitUnit())
                    {
                        if (unitTarget->getPowerType() == POWER_MANA)
                        {
                            int32 effValue = GetEffectValue();
                            // Glyph of Mana Tide
                            if (Unit* owner = caster->GetOwner())
                                if (AuraEffect* dummy = owner->GetAuraEffect(SHAMAN_SPELL_GLYPH_OF_MANA_TIDE, 0))
                                    effValue += dummy->GetAmount();
                            // Regenerate 6% of Total Mana Every 3 secs
                            int32 effBasePoints0 = int32(CalculatePct(unitTarget->GetMaxPower(POWER_MANA), effValue));
                            caster->CastCustomSpell(unitTarget, SHAMAN_SPELL_MANA_TIDE_TOTEM, &effBasePoints0, NULL, NULL, true, NULL, NULL, GetOriginalCaster()->GetGUID());
                        }
                    }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_sha_mana_tide_totem_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_mana_tide_totem_SpellScript();
        }
};

// 6474 - Earthbind Totem - Fix Talent:Earthen Power
class spell_sha_earthbind_totem : public SpellScriptLoader
{
    public:
        spell_sha_earthbind_totem() : SpellScriptLoader("spell_sha_earthbind_totem") { }

        class spell_sha_earthbind_totem_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_earthbind_totem_AuraScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(SHAMAN_TOTEM_SPELL_EARTHBIND_TOTEM) || !sSpellMgr->GetSpellInfo(SHAMAN_TOTEM_SPELL_EARTHEN_POWER))
                    return false;
                return true;
            }

            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                if (!GetCaster())
                    return;
                if (Player* owner = GetCaster()->GetCharmerOrOwnerPlayerOrPlayerItself())
                    if (AuraEffect* aur = owner->GetDummyAuraEffect(SPELLFAMILY_SHAMAN, 2289, 0))
                        if (roll_chance_i(aur->GetBaseAmount()))
                            GetTarget()->CastSpell((Unit*)NULL, SHAMAN_TOTEM_SPELL_EARTHEN_POWER, true);
            }

            void Apply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (!GetCaster())
                    return;
                Player* owner = GetCaster()->GetCharmerOrOwnerPlayerOrPlayerItself();
                if (!owner)
                    return;
                // Storm, Earth and Fire
                if (AuraEffect* aurEff = owner->GetAuraEffectOfRankedSpell(SHAMAN_SPELL_STORM_EARTH_AND_FIRE, EFFECT_1))
                {
                    if (roll_chance_i(aurEff->GetAmount()))
                        GetCaster()->CastSpell(GetCaster(), EARTHBIND_TOTEM_SPELL_EARTHGRAB, false);
                }
            }

            void Register()
            {
                 OnEffectPeriodic += AuraEffectPeriodicFn(spell_sha_earthbind_totem_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
                 OnEffectApply += AuraEffectApplyFn(spell_sha_earthbind_totem_AuraScript::Apply, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_earthbind_totem_AuraScript();
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

class spell_sha_earthen_power : public SpellScriptLoader
{
    public:
        spell_sha_earthen_power() : SpellScriptLoader("spell_sha_earthen_power") { }

        class spell_sha_earthen_power_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_earthen_power_SpellScript);

            void FilterTargets(std::list<WorldObject*>& unitList)
            {
                unitList.remove_if(EarthenPowerTargetSelector());
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_sha_earthen_power_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_earthen_power_SpellScript();
        }
};

class spell_sha_bloodlust : public SpellScriptLoader
{
    public:
        spell_sha_bloodlust() : SpellScriptLoader("spell_sha_bloodlust") { }

        class spell_sha_bloodlust_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_bloodlust_SpellScript);

            bool Validate(SpellInfo const* /*spellEntry*/)
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

            bool Validate(SpellInfo const* /*spellEntry*/)
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
            }

            void ApplyDebuff()
            {
                if (Unit* target = GetHitUnit())
                    target->CastSpell(target, SHAMAN_SPELL_EXHAUSTION, true);
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

            bool Validate(SpellInfo const* /*SpellEntry*/)
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

enum CleansingTotemPulse
{
    SPELL_CLEANSING_TOTEM_EFFECT   = 52025,
};

class spell_sha_cleansing_totem_pulse : public SpellScriptLoader
{
    public:
        spell_sha_cleansing_totem_pulse() : SpellScriptLoader("spell_sha_cleansing_totem_pulse") { }

        class spell_sha_cleansing_totem_pulse_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_cleansing_totem_pulse_SpellScript);

            bool Validate(SpellInfo const* /*SpellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_CLEANSING_TOTEM_EFFECT))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                int32 bp = 1;
                if (GetCaster() && GetHitUnit() && GetOriginalCaster())
                    GetCaster()->CastCustomSpell(GetHitUnit(), SPELL_CLEANSING_TOTEM_EFFECT, NULL, &bp, NULL, true, NULL, NULL, GetOriginalCaster()->GetGUID());
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_sha_cleansing_totem_pulse_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_cleansing_totem_pulse_SpellScript();
        }
};

enum ManaSpringTotem
{
    SPELL_MANA_SPRING_TOTEM_ENERGIZE     = 52032,
};

class spell_sha_mana_spring_totem : public SpellScriptLoader
{
    public:
        spell_sha_mana_spring_totem() : SpellScriptLoader("spell_sha_mana_spring_totem") { }

        class spell_sha_mana_spring_totem_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_sha_mana_spring_totem_SpellScript);

            bool Validate(SpellInfo const* /*SpellEntry*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_MANA_SPRING_TOTEM_ENERGIZE))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /* effIndex */)
            {
                int32 damage = GetEffectValue();
                if (Unit* target = GetHitUnit())
                    if (Unit* caster = GetCaster())
                        if (target->getPowerType() == POWER_MANA)
                            caster->CastCustomSpell(target, SPELL_MANA_SPRING_TOTEM_ENERGIZE, &damage, 0, 0, true, 0, 0, GetOriginalCaster()->GetGUID());
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_sha_mana_spring_totem_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }

        };

        SpellScript* GetSpellScript() const
        {
            return new spell_sha_mana_spring_totem_SpellScript();
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

                            SetHitDamage(hitDamage);

                            // Spreading your Flame shock from the target to up to four ennemies within 12 yards
                            // Effect desactivated if has Glyph of Lava Lash
                            if (!_player->HasAura(55444))
                            {
                                if (target->HasAura(SPELL_SHA_FLAME_SHOCK))
                                {
                                    std::list<Unit*> targetList;

                                    _player->GetAttackableUnitListInRange(_player, targetList, 12.0f);

                                    int32 hitTargets = 0;

                                    for (auto itr : targetList)
                                    {
                                        if (!_player->IsValidAttackTarget(itr) || itr->GetGUID() == target->GetGUID())
                                            continue;

                                        if (hitTargets >= 4)
                                            continue;

                                        _player->CastSpell(itr, SPELL_SHA_FLAME_SHOCK, true);
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

// 1064 Chain Heal
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
                        // Consume it
                        GetHitUnit()->RemoveAura(aurEff->GetBase());
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

class spell_sha_sentry_totem : public SpellScriptLoader
{
    public:
        spell_sha_sentry_totem() : SpellScriptLoader("spell_sha_sentry_totem") { }

        class spell_sha_sentry_totem_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_sha_sentry_totem_AuraScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SHAMAN_BIND_SIGHT))
                    return false;
                return true;
            }

            void AfterApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (Creature* totem = caster->GetMap()->GetCreature(caster->m_SummonSlot[4]))
                        if (totem->isTotem())
                            caster->CastSpell(totem, SHAMAN_BIND_SIGHT, true);
            }

            void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (caster->GetTypeId() == TYPEID_PLAYER)
                        caster->ToPlayer()->StopCastingBindSight();
            }

            void Register()
            {
                 AfterEffectApply += AuraEffectApplyFn(spell_sha_sentry_totem_AuraScript::AfterApply, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                 AfterEffectRemove += AuraEffectRemoveFn(spell_sha_sentry_totem_AuraScript::AfterRemove, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_sha_sentry_totem_AuraScript();
        }
};

void AddSC_shaman_spell_scripts()
{
    new spell_sha_unleash_elements();
    new spell_sha_rolling_thunder();
    new spell_sha_fulmination();
    new spell_sha_lava_surge();
    new spell_sha_healing_stream();
    new spell_sha_static_shock();
    new spell_sha_elemental_blast();
    new spell_sha_earthquake_tick();
    new spell_sha_earthquake();
    new spell_sha_healing_rain();
    new spell_sha_ascendance();
    new spell_sha_fire_nova();
    new spell_sha_mana_tide_totem();
    new spell_sha_earthbind_totem();
    new spell_sha_earthen_power();
    new spell_sha_bloodlust();
    new spell_sha_heroism();
    new spell_sha_ancestral_awakening_proc();
    new spell_sha_cleansing_totem_pulse();
    new spell_sha_mana_spring_totem();
    new spell_sha_lava_lash();
    new spell_sha_chain_heal();
    new spell_sha_sentry_totem();
}
