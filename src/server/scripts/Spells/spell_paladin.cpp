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
 * Scripts for spells with SPELLFAMILY_PALADIN and SPELLFAMILY_GENERIC spells used by paladin players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_pal_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"

enum PaladinSpells
{
    PALADIN_SPELL_DIVINE_PLEA                    = 54428,
    SPELL_BLESSING_OF_LOWER_CITY_DRUID           = 37878,
    SPELL_BLESSING_OF_LOWER_CITY_PALADIN         = 37879,
    SPELL_BLESSING_OF_LOWER_CITY_PRIEST          = 37880,
    SPELL_BLESSING_OF_LOWER_CITY_SHAMAN          = 37881,
    SPELL_FORBEARANCE                            = 25771,
    PALADIN_SPELL_WORD_OF_GLORY                  = 85673,
    PALADIN_SPELL_WORD_OF_GLORY_HEAL             = 130551,
    PALADIN_SPELL_GLYPH_OF_WORD_OF_GLORY         = 54936,
    PALADIN_SPELL_GLYPH_OF_WORD_OF_GLORY_DAMAGE  = 115522,
    PALADIN_SPELL_CONSECRATION_AREA_DUMMY        = 81298,
    PALADIN_SPELL_CONSECRATION_DAMAGE            = 81297,
    PALADIN_SPELL_HOLY_PRISM_ALLIES              = 114871,
    PALADIN_SPELL_HOLY_PRISM_ENNEMIES            = 114852,
    PALADIN_SPELL_HOLY_PRISM_DAMAGE_VISUAL       = 114862,
    PALADIN_SPELL_HOLY_PRISM_DAMAGE_VISUAL_2     = 114870,
    PALADIN_SPELL_HOLY_PRISM_HEAL_VISUAL         = 121551,
    PALADIN_SPELL_HOLY_PRISM_HEAL_VISUAL_2       = 121552,
    PALADIN_SPELL_ARCING_LIGHT_HEAL              = 119952,
    PALADIN_SPELL_ARCING_LIGHT_DAMAGE            = 114919,
    PALADIN_SPELL_EXECUTION_SENTENCE             = 114916,
    PALADIN_SPELL_STAY_OF_EXECUTION              = 114917,
    PALADIN_SPELL_GLYPH_OF_BLINDING_LIGHT        = 54934,
    PALADIN_SPELL_BLINDING_LIGHT_CONFUSE         = 105421,
    PALADIN_SPELL_BLINDING_LIGHT_STUN            = 115752,
    PALADIN_SPELL_EXORCISM                       = 879,
    PALADIN_SPELL_SACRED_SHIELD                  = 65148,
    PALADIN_SPELL_ARDENT_DEFENDER_HEAL           = 66235,
    PALADIN_SPELL_TOWER_OF_RADIANCE_ENERGIZE     = 88852,
    PALADIN_SPELL_BEACON_OF_LIGHT                = 53563,
    PALADIN_SPELL_SELFLESS_HEALER_STACK          = 114250,
    PALADIN_SPELL_SHIELD_OF_THE_RIGHTEOUS_PROC   = 132403,
    PALADIN_SPELL_BASTION_OF_GLORY               = 114637,
    PALADIN_SPELL_DIVINE_PURPOSE                 = 90174,
    PALADIN_SPELL_DIVINE_SHIELD                  = 642,
    PALADIN_SPELL_LAY_ON_HANDS                   = 633,
    PALADIN_SPELL_DIVINE_PROTECTION              = 498,
    PALADIN_SPELL_GLYPH_OF_AVENGING_WRATH        = 54927,
    PALADIN_SPELL_AVENGING_WRATH_REGEN_BY_GLYPH  = 115547,
    PALADIN_SPELL_SACRED_CLEANSING               = 53551,
};

// Called by Avenging Wrath - 31884
// Glyph of Avenging Wrath - 54927
class spell_pal_glyph_of_avenging_wrath : public SpellScriptLoader
{
    public:
        spell_pal_glyph_of_avenging_wrath() : SpellScriptLoader("spell_pal_glyph_of_avenging_wrath") { }

        class spell_pal_glyph_of_avenging_wrath_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_glyph_of_avenging_wrath_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (_player->HasAura(PALADIN_SPELL_GLYPH_OF_AVENGING_WRATH))
                        _player->CastSpell(_player, PALADIN_SPELL_AVENGING_WRATH_REGEN_BY_GLYPH, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pal_glyph_of_avenging_wrath_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_glyph_of_avenging_wrath_SpellScript();
        }
};

// Shield of the Righteous - 53600
class spell_pal_shield_of_the_righteous : public SpellScriptLoader
{
    public:
        spell_pal_shield_of_the_righteous() : SpellScriptLoader("spell_pal_shield_of_the_righteous") { }

        class spell_pal_shield_of_the_righteous_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_shield_of_the_righteous_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* unitTarget = GetHitUnit())
                    {
                        // -30% damage taken for 3s
                        if (Aura* spellShield = _player->GetAura(PALADIN_SPELL_SHIELD_OF_THE_RIGHTEOUS_PROC))
                            spellShield->SetDuration(spellShield->GetDuration() + 3000);
                        else
                            _player->CastSpell(_player, PALADIN_SPELL_SHIELD_OF_THE_RIGHTEOUS_PROC, true);
                        _player->CastSpell(_player, PALADIN_SPELL_BASTION_OF_GLORY, true);
                        if (Aura* aura = _player->GetAura(PALADIN_SPELL_BASTION_OF_GLORY)) //Item - Paladin T16 Protection 4P Bonus
                            if(aura->GetStackAmount() >= 3)
                                _player->CastSpell(_player, 144569, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pal_shield_of_the_righteous_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_shield_of_the_righteous_SpellScript();
        }
};

// Called by Flash of Light - 19750 and Divine Light - 82326
// Tower of Radiance - 85512
class spell_pal_tower_of_radiance : public SpellScriptLoader
{
    public:
        spell_pal_tower_of_radiance() : SpellScriptLoader("spell_pal_tower_of_radiance") { }

        class spell_pal_tower_of_radiance_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_tower_of_radiance_SpellScript);

            void HandleHeal(SpellEffIndex /*effIndex*/)
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (target->HasAura(PALADIN_SPELL_BEACON_OF_LIGHT, _player->GetGUID()))
                        {
                            if(_player->HasAura(105809) && _player->HasAura(85512))
                            {
                                _player->EnergizeBySpell(_player, PALADIN_SPELL_TOWER_OF_RADIANCE_ENERGIZE, 3, POWER_HOLY_POWER);
                                SetHitHeal(GetHitHeal() * 1.3f);
                            }
                            else
                                _player->EnergizeBySpell(_player, PALADIN_SPELL_TOWER_OF_RADIANCE_ENERGIZE, 1, POWER_HOLY_POWER);
                        }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_pal_tower_of_radiance_SpellScript::HandleHeal, EFFECT_0, SPELL_EFFECT_HEAL);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_tower_of_radiance_SpellScript();
        }
};

// Sacred shield - 20925, 148039
class spell_pal_sacred_shield : public SpellScriptLoader
{
    public:
        spell_pal_sacred_shield() : SpellScriptLoader("spell_pal_sacred_shield") { }

        class spell_pal_sacred_shield_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_sacred_shield_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Player* _player = caster->ToPlayer())
                        if (Unit* target = GetTarget())
                       {
                           float amount = 0;
                            switch(_player->GetSpecializationId(_player->GetActiveSpec()))
                            {
                                case SPEC_PALADIN_HOLY:
                                case SPEC_PALADIN_RETRIBUTION:
                                    amount = int32(343 + caster->GetSpellPowerDamage(SPELL_SCHOOL_MASK_HOLY) * 1.17f);
                                    break;
                                case SPEC_PALADIN_PROTECTION:
                                    amount = int32(240 + caster->GetSpellPowerDamage(SPELL_SCHOOL_MASK_HOLY) * 0.819f);
                                    break;
                            }
                            _player->CastCustomSpell(target, PALADIN_SPELL_SACRED_SHIELD, &amount, NULL, NULL, true, NULL, aurEff);
                        }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_sacred_shield_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pal_sacred_shield_AuraScript();
        }
};

// Emancipate - 121783
class spell_pal_emancipate : public SpellScriptLoader
{
    public:
        spell_pal_emancipate() : SpellScriptLoader("spell_pal_emancipate") { }

        class spell_pal_emancipate_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_emancipate_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    std::list<Aura*> auraList;

                    for (Unit::AuraApplicationMap::const_iterator itr = _player->GetAppliedAuras().begin(); itr!= _player->GetAppliedAuras().end(); ++itr)
                    {
                        Aura* aura = itr->second->GetBase();
                        if (aura && aura->GetSpellInfo()->GetAllEffectsMechanicMask() & ((1<<MECHANIC_SNARE)|(1<<MECHANIC_ROOT)))
                            auraList.push_back(aura);
                    }

                    if (!auraList.empty())
                    {
                        Trinity::Containers::RandomResizeList(auraList, 1);
                        _player->RemoveAura(*auraList.begin());
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pal_emancipate_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_emancipate_SpellScript();
        }
};

// Art of War - 59578
class spell_pal_art_of_war : public SpellScriptLoader
{
    public:
        spell_pal_art_of_war() : SpellScriptLoader("spell_pal_art_of_war") { }

        class spell_pal_art_of_war_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_art_of_war_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->HasSpellCooldown(PALADIN_SPELL_EXORCISM))
                        _player->RemoveSpellCooldown(PALADIN_SPELL_EXORCISM, true);
                    if (_player->HasSpellCooldown(122032))
                        _player->RemoveSpellCooldown(122032, true);
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pal_art_of_war_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_art_of_war_SpellScript();
        }
};

// Blinding Light - 115750
class spell_pal_blinding_light : public SpellScriptLoader
{
    public:
        spell_pal_blinding_light() : SpellScriptLoader("spell_pal_blinding_light") { }

        class spell_pal_blinding_light_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_blinding_light_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (_player->HasAura(PALADIN_SPELL_GLYPH_OF_BLINDING_LIGHT))
                            _player->CastSpell(target, PALADIN_SPELL_BLINDING_LIGHT_STUN, true);
                        else
                            _player->CastSpell(target, PALADIN_SPELL_BLINDING_LIGHT_CONFUSE, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pal_blinding_light_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_blinding_light_SpellScript();
        }
};

// Hand of Protection - 1022
class spell_pal_hand_of_protection : public SpellScriptLoader
{
    public:
        spell_pal_hand_of_protection() : SpellScriptLoader("spell_pal_hand_of_protection") { }

        class spell_pal_hand_of_protection_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_hand_of_protection_SpellScript);

            SpellCastResult CheckForbearance()
            {
                Unit* caster = GetCaster();
                if (Unit* target = GetExplTargetUnit())
                {
                    if (target->HasAura(SPELL_FORBEARANCE))
                        return SPELL_FAILED_TARGET_AURASTATE;
                    if (!caster || (caster->HasUnitState(UNIT_STATE_CONTROLLED) && caster != target))
                        return SPELL_FAILED_BAD_TARGETS;
                }

                return SPELL_CAST_OK;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        _player->CastSpell(target, SPELL_FORBEARANCE, true);
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_pal_hand_of_protection_SpellScript::CheckForbearance);
                OnHit += SpellHitFn(spell_pal_hand_of_protection_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_hand_of_protection_SpellScript();
        }
};

// Divine Shield - 642 and Divine Shield - 110700
class spell_pal_divine_shield : public SpellScriptLoader
{
    public:
        spell_pal_divine_shield() : SpellScriptLoader("spell_pal_divine_shield") { }

        class spell_pal_divine_shield_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_divine_shield_SpellScript);

            SpellCastResult CheckForbearance()
            {
                if (Unit* caster = GetCaster())
                    if (caster->HasAura(SPELL_FORBEARANCE))
                        return SPELL_FAILED_TARGET_AURASTATE;

                return SPELL_CAST_OK;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        _player->CastSpell(target, SPELL_FORBEARANCE, true);
            }

            void HandleHeal(SpellEffIndex /*effIndex*/)
            {
                if(Unit* caster = GetCaster())
                {
                    if(caster->HasAura(146956))
                    {
                        if(uint32 count = GetSpell()->GetCountDispel())
                        {
                            int32 _heal = caster->CountPctFromMaxHealth(10);
                            if(count > 5)
                                count = 5;
                            SetHitHeal(int32(_heal * count));
                        }
                    }
                }
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_pal_divine_shield_SpellScript::CheckForbearance);
                OnHit += SpellHitFn(spell_pal_divine_shield_SpellScript::HandleOnHit);
                OnEffectHitTarget += SpellEffectFn(spell_pal_divine_shield_SpellScript::HandleHeal, EFFECT_3, SPELL_EFFECT_HEAL_PCT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_divine_shield_SpellScript();
        }
};

// Execution Sentence - 114157
class spell_pal_execution_sentence : public SpellScriptLoader
{
    public:
        spell_pal_execution_sentence() : SpellScriptLoader("spell_pal_execution_sentence") { }

        class spell_pal_execution_sentence_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_execution_sentence_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (_player->IsValidAttackTarget(target))
                            _player->CastSpell(target, PALADIN_SPELL_EXECUTION_SENTENCE, true);
                        else if (_player->GetGUID() == target->GetGUID())
                            _player->CastSpell(_player, PALADIN_SPELL_STAY_OF_EXECUTION, true);
                        else
                            _player->CastSpell(target, PALADIN_SPELL_STAY_OF_EXECUTION, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pal_execution_sentence_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_execution_sentence_SpellScript();
        }
};

// Light's Hammer (periodic dummy for npc) - 114918
class spell_pal_lights_hammer : public SpellScriptLoader
{
    public:
        spell_pal_lights_hammer() : SpellScriptLoader("spell_pal_lights_hammer") { }

        class spell_pal_lights_hammer_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_lights_hammer_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (GetCaster())
                {
                    if (GetCaster()->GetOwner())
                    {
                        GetCaster()->CastSpell(GetCaster()->GetPositionX(), GetCaster()->GetPositionY(), GetCaster()->GetPositionZ(), PALADIN_SPELL_ARCING_LIGHT_HEAL, true, 0, NULL, GetCaster()->GetOwner()->GetGUID());
                        GetCaster()->CastSpell(GetCaster()->GetPositionX(), GetCaster()->GetPositionY(), GetCaster()->GetPositionZ(), PALADIN_SPELL_ARCING_LIGHT_DAMAGE, true, 0, NULL, GetCaster()->GetOwner()->GetGUID());
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_lights_hammer_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pal_lights_hammer_AuraScript();
        }
};

// called by Holy Prism (heal) - 114871
// Holy Prism visual for other targets
class spell_pal_holy_prism_heal : public SpellScriptLoader
{
    public:
        spell_pal_holy_prism_heal() : SpellScriptLoader("spell_pal_holy_prism_heal") { }

        class spell_pal_holy_prism_heal_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_holy_prism_heal_SpellScript);

            std::list<WorldObject*> targetList;
            Unit* unitTarget;

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetHitUnit())
                {
                    if (unitTarget)
                        unitTarget->CastSpell(target, PALADIN_SPELL_HOLY_PRISM_DAMAGE_VISUAL_2, true);
                }
            }

            void HandleHeal(SpellEffIndex /*effIndex*/)
            {
                unitTarget = GetHitUnit();
            }

            void FilterEnemy(std::list<WorldObject*>& unitList)
            {
                if (Unit* caster = GetCaster())
                {
                    for (std::list<WorldObject*>::iterator itr = unitList.begin(); itr != unitList.end();)
                    {
                        if (caster->IsFriendlyTo((*itr)->ToUnit()))
                            unitList.erase(itr++);
                        else
                            ++itr;
                    }
                }

                if(unitList.size() > 5)
                    Trinity::Containers::RandomResizeList(unitList, 5);
                targetList = unitList;
            }

            void FilterScript(std::list<WorldObject*>& unitList)
            {
                unitList.clear();
                unitList = targetList;
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_pal_holy_prism_heal_SpellScript::HandleHeal, EFFECT_0, SPELL_EFFECT_HEAL);
                OnEffectHitTarget += SpellEffectFn(spell_pal_holy_prism_heal_SpellScript::HandleDamage, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_holy_prism_heal_SpellScript::FilterEnemy, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_holy_prism_heal_SpellScript::FilterScript, EFFECT_2, TARGET_UNIT_DEST_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_holy_prism_heal_SpellScript();
        }
};

// called by Holy Prism (damage) - 114852
// Holy Prism visual for other targets
class spell_pal_holy_prism_damage : public SpellScriptLoader
{
    public:
        spell_pal_holy_prism_damage() : SpellScriptLoader("spell_pal_holy_prism_damage") { }

        class spell_pal_holy_prism_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_holy_prism_damage_SpellScript);

            std::list<WorldObject*> targetList;
            Unit* unitTarget;

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                unitTarget = GetHitUnit();
            }

            void HandleHeal(SpellEffIndex /*effIndex*/)
            {
                if (Unit* target = GetHitUnit())
                    if (unitTarget)
                        unitTarget->CastSpell(target, PALADIN_SPELL_HOLY_PRISM_HEAL_VISUAL_2, true);
            }

            void FilterEnemy(std::list<WorldObject*>& unitList)
            {
                if (Unit* caster = GetCaster())
                {
                    for (std::list<WorldObject*>::iterator itr = unitList.begin(); itr != unitList.end();)
                    {
                        if (caster->IsFriendlyTo((*itr)->ToUnit()))
                            ++itr;
                        else
                            unitList.erase(itr++);
                    }
                }

                if(unitList.size() > 5)
                    Trinity::Containers::RandomResizeList(unitList, 5);
                targetList = unitList;
            }

            void FilterScript(std::list<WorldObject*>& unitList)
            {
                unitList.clear();
                unitList = targetList;
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_pal_holy_prism_damage_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
                OnEffectHitTarget += SpellEffectFn(spell_pal_holy_prism_damage_SpellScript::HandleHeal, EFFECT_1, SPELL_EFFECT_HEAL);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_holy_prism_damage_SpellScript::FilterEnemy, EFFECT_1, TARGET_UNIT_DEST_AREA_ALLY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_holy_prism_damage_SpellScript::FilterScript, EFFECT_2, TARGET_UNIT_DEST_AREA_ENTRY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_holy_prism_damage_SpellScript();
        }
};

// called by Holy Prism (visual damage) - 114862 or Holy Prism (visual heal) - 121551
// Holy Prism (damage) - 114852 or Holy Prism (heal) - 114871
class spell_pal_holy_prism_effect : public SpellScriptLoader
{
    public:
        spell_pal_holy_prism_effect() : SpellScriptLoader("spell_pal_holy_prism_effect") { }

        class spell_pal_holy_prism_effect_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_holy_prism_effect_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        // damage
                        if (GetSpellInfo()->Id == 114862)
                            _player->CastSpell(target, PALADIN_SPELL_HOLY_PRISM_ENNEMIES, true);
                        // heal
                        else if (GetSpellInfo()->Id == 121551)
                            _player->CastSpell(target, PALADIN_SPELL_HOLY_PRISM_ALLIES, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pal_holy_prism_effect_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_holy_prism_effect_SpellScript();
        }
};

// Holy Prism - 114165
class spell_pal_holy_prism : public SpellScriptLoader
{
    public:
        spell_pal_holy_prism() : SpellScriptLoader("spell_pal_holy_prism") { }

        class spell_pal_holy_prism_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_holy_prism_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (_player->IsFriendlyTo(target))
                        {
                            _player->CastSpell(target, PALADIN_SPELL_HOLY_PRISM_HEAL_VISUAL, true);
                            _player->CastSpell(target, PALADIN_SPELL_HOLY_PRISM_HEAL_VISUAL_2, true);
                        }
                        else
                        {
                            _player->CastSpell(target, PALADIN_SPELL_HOLY_PRISM_DAMAGE_VISUAL, true);
                            _player->CastSpell(target, PALADIN_SPELL_HOLY_PRISM_DAMAGE_VISUAL_2, true);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pal_holy_prism_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_holy_prism_SpellScript();
        }
};

// Consecration - 26573 (periodic dummy)
class spell_pal_consecration : public SpellScriptLoader
{
    public:
        spell_pal_consecration() : SpellScriptLoader("spell_pal_consecration") { }

        class spell_pal_consecration_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_consecration_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if(Unit* caster = GetCaster())
                {
                    if (DynamicObject* dynObj = caster->GetDynObject(PALADIN_SPELL_CONSECRATION_AREA_DUMMY))
                        caster->CastSpell(dynObj->GetPositionX(), dynObj->GetPositionY(), dynObj->GetPositionZ(), PALADIN_SPELL_CONSECRATION_DAMAGE, true);
                    else if(Creature* summon = caster->GetMap()->GetCreature(caster->m_SummonSlot[1]))
                        caster->CastSpell(summon->GetPositionX(), summon->GetPositionY(), summon->GetPositionZ(), PALADIN_SPELL_CONSECRATION_DAMAGE, true);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_consecration_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pal_consecration_AuraScript();
        }
};

// Consecration (override, by glyph 54928) - 116467 (periodic dummy)
class spell_pal_consecration_override : public SpellScriptLoader
{
    public:
        spell_pal_consecration_override() : SpellScriptLoader("spell_pal_consecration_override") { }

        class spell_pal_consecration_override_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_consecration_override_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* caster = GetCaster())
                {
                    if(caster->m_SummonSlot[1])
                        if(Creature* summon = caster->GetMap()->GetCreature(caster->m_SummonSlot[1]))
                            caster->CastSpell(summon->GetPositionX(), summon->GetPositionY(), summon->GetPositionZ(), PALADIN_SPELL_CONSECRATION_DAMAGE, true);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_pal_consecration_override_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pal_consecration_override_AuraScript();
        }
};

// Ardent Defender - 31850
class spell_pal_ardent_defender : public SpellScriptLoader
{
    public:
        spell_pal_ardent_defender() : SpellScriptLoader("spell_pal_ardent_defender") { }

        class spell_pal_ardent_defender_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_ardent_defender_AuraScript);

            uint32 absorbPct, healPct;

            bool Load()
            {
                healPct = GetSpellInfo()->Effects[EFFECT_1]->CalcValue();
                absorbPct = GetSpellInfo()->Effects[EFFECT_0]->CalcValue();
                return GetUnitOwner()->GetTypeId() == TYPEID_PLAYER;
            }

            void CalculateAmount(AuraEffect const* aurEff, float & amount, bool & canBeRecalculated)
            {
                // Set absorbtion amount to unlimited
                amount = -1;
            }

            void Absorb(AuraEffect* aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
            {
                Unit* victim = GetTarget();
                int32 remainingHealth = victim->GetHealth() - dmgInfo.GetDamage();
                // If damage kills us
                if (remainingHealth <= 0 && !victim->ToPlayer()->HasSpellCooldown(PALADIN_SPELL_ARDENT_DEFENDER_HEAL))
                {
                    // Cast healing spell, completely avoid damage
                    absorbAmount = dmgInfo.GetDamage();

                    float healAmount = victim->CountPctFromMaxHealth(healPct);
                    victim->CastCustomSpell(victim, PALADIN_SPELL_ARDENT_DEFENDER_HEAL, &healAmount, NULL, NULL, true, NULL, aurEff);
                    victim->ToPlayer()->AddSpellCooldown(PALADIN_SPELL_ARDENT_DEFENDER_HEAL, 0, getPreciseTime() + 120.0);
                }
                else
                    absorbAmount = CalculatePct(dmgInfo.GetDamage(), absorbPct);
            }

            void Register()
            {
                 DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pal_ardent_defender_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                 OnEffectAbsorb += AuraEffectAbsorbFn(spell_pal_ardent_defender_AuraScript::Absorb, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pal_ardent_defender_AuraScript();
        }
};

class spell_pal_blessing_of_faith : public SpellScriptLoader
{
    public:
        spell_pal_blessing_of_faith() : SpellScriptLoader("spell_pal_blessing_of_faith") { }

        class spell_pal_blessing_of_faith_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_blessing_of_faith_SpellScript);

            bool Validate(SpellInfo const* /*SpellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_BLESSING_OF_LOWER_CITY_DRUID) || !sSpellMgr->GetSpellInfo(SPELL_BLESSING_OF_LOWER_CITY_PALADIN) || !sSpellMgr->GetSpellInfo(SPELL_BLESSING_OF_LOWER_CITY_PRIEST) || !sSpellMgr->GetSpellInfo(SPELL_BLESSING_OF_LOWER_CITY_SHAMAN))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* unitTarget = GetHitUnit())
                {
                    uint32 spell_id = 0;
                    switch (unitTarget->getClass())
                    {
                        case CLASS_DRUID:   spell_id = SPELL_BLESSING_OF_LOWER_CITY_DRUID; break;
                        case CLASS_PALADIN: spell_id = SPELL_BLESSING_OF_LOWER_CITY_PALADIN; break;
                        case CLASS_PRIEST:  spell_id = SPELL_BLESSING_OF_LOWER_CITY_PRIEST; break;
                        case CLASS_SHAMAN:  spell_id = SPELL_BLESSING_OF_LOWER_CITY_SHAMAN; break;
                        default: return;                    // ignore for non-healing classes
                    }
                    Unit* caster = GetCaster();
                    caster->CastSpell(caster, spell_id, true);
                }
            }

            void Register()
            {
                // add dummy effect spell handler to Blessing of Faith
                OnEffectHitTarget += SpellEffectFn(spell_pal_blessing_of_faith_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_blessing_of_faith_SpellScript();
        }
};

// Lay on Hands - 633
class spell_pal_lay_on_hands : public SpellScriptLoader
{
    public:
        spell_pal_lay_on_hands() : SpellScriptLoader("spell_pal_lay_on_hands") { }

        class spell_pal_lay_on_hands_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_lay_on_hands_SpellScript);

            SpellCastResult CheckForbearance()
            {
                Unit* caster = GetCaster();
                if (Unit* target = GetExplTargetUnit())
                    if (target->HasAura(SPELL_FORBEARANCE))
                        return SPELL_FAILED_TARGET_AURASTATE;

                return SPELL_CAST_OK;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        _player->CastSpell(target, SPELL_FORBEARANCE, true);
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_pal_lay_on_hands_SpellScript::CheckForbearance);
                OnHit += SpellHitFn(spell_pal_lay_on_hands_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_lay_on_hands_SpellScript();
        }
};

class spell_pal_righteous_defense : public SpellScriptLoader
{
    public:
        spell_pal_righteous_defense() : SpellScriptLoader("spell_pal_righteous_defense") { }

        class spell_pal_righteous_defense_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_righteous_defense_SpellScript);

            SpellCastResult CheckCast()
            {
                Unit* caster = GetCaster();
                if (caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_DONT_REPORT;

                if (Unit* target = GetExplTargetUnit())
                {
                    if (!target->IsFriendlyTo(caster) || target->getAttackers().empty())
                        return SPELL_FAILED_BAD_TARGETS;
                }
                else
                    return SPELL_FAILED_BAD_TARGETS;

                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_pal_righteous_defense_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_righteous_defense_SpellScript();
        }
};

// spell 114250 - Selfless Healer
class spell_pal_selfless_healer : public SpellScriptLoader
{
    public:
        spell_pal_selfless_healer() : SpellScriptLoader("spell_pal_selfless_healer") { }

        class spell_pal_selfless_healerAuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_selfless_healerAuraScript);

            void OnStackChange(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                    if(GetStackAmount() >= 3)
                        target->CastSpell(target, 128863, false); // visaul effect
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_pal_selfless_healerAuraScript::OnStackChange, EFFECT_1, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        // function which creates AuraScript
        AuraScript* GetAuraScript() const
        {
            return new spell_pal_selfless_healerAuraScript();
        }
};

// Hand of Purity - 114039
class spell_pal_hand_of_purity : public SpellScriptLoader
{
    public:
        spell_pal_hand_of_purity() : SpellScriptLoader("spell_pal_hand_of_purity") { }

        class spell_pal_hand_of_purity_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_hand_of_purity_AuraScript);

            void CalculateAmount(AuraEffect const* /*aurEff*/, float & amount, bool & /*canBeRecalculated*/)
            {
                // Set absorbtion amount to unlimited
                amount = GetSpellInfo()->Effects[EFFECT_0]->BasePoints;
            }

            void Absorb(AuraEffect* aurEff, DamageInfo & dmgInfo, uint32 & absorbAmount)
            {
                PreventDefaultAction();
                if(dmgInfo.GetDamageType() == DOT)
                    absorbAmount = CalculatePct(dmgInfo.GetDamage(), GetSpellInfo()->Effects[EFFECT_0]->BasePoints);
                else
                    absorbAmount = 0;
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pal_hand_of_purity_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_pal_hand_of_purity_AuraScript::Absorb, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pal_hand_of_purity_AuraScript();
        }
};

// Stay of Execution - 114917
class spell_pal_stay_of_execution : public SpellScriptLoader
{
    public:
        spell_pal_stay_of_execution() : SpellScriptLoader("spell_pal_stay_of_execution") { }

        class spell_pal_stay_of_execution_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_stay_of_execution_AuraScript);

            bool Load()
            {
                saveTick = 0;
                return true;
            }

            void CalculateAmount(AuraEffect const* /*aurEff*/, float& amount, bool& /*canBeRecalculated*/)
            {
                if(Unit* caster = GetCaster())
                    amount = int32((caster->GetSpellPowerDamage(SPELL_SCHOOL_MASK_HOLY) * GetSpellInfo()->Effects[EFFECT_1]->BasePoints / 1000 + 26.72716306 * amount) / 20);
            }

            void HandleTick(AuraEffect const* aurEff, float& amount, Unit* /*target*/, bool /*crit*/)
            {
                if(aurEff->GetTotalTicks() == aurEff->GetTickNumber())
                    amount *= 9;
                else if(aurEff->GetTickNumber() == 20)
                {
                    amount += int32((saveTick + 1) * 0.025f * amount);
                    amount *= 5; // 500% for next tick
                }
                else
                    amount += int32(aurEff->GetTickNumber() * 0.025f * amount);
                saveTick = aurEff->GetTickNumber();
            }

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                    if (removeMode == AURA_REMOVE_BY_ENEMY_SPELL)
                    {
                        const_cast<AuraEffect*>(aurEff)->SetTickNumber(20);
                        aurEff->HandlePeriodicHealAurasTick(caster, caster, (SpellEffIndex) aurEff->GetEffIndex());
                    }
                }
            }

            int32 saveTick;

            void Register()
            {
                DoEffectBeforeCalcAmount += AuraEffectCalcAmountFn(spell_pal_stay_of_execution_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
                DoEffectChangeTickDamage += AuraEffectChangeTickDamageFn(spell_pal_stay_of_execution_AuraScript::HandleTick, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_pal_stay_of_execution_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pal_stay_of_execution_AuraScript();
        }
};

// Execution Sentence - 114916
class spell_pal_execution_sentence_damage : public SpellScriptLoader
{
    public:
        spell_pal_execution_sentence_damage() : SpellScriptLoader("spell_pal_execution_sentence_damage") { }

        class spell_pal_execution_sentence_damage_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_execution_sentence_damage_AuraScript);

            bool Load()
            {
                saveTick = 0;
                return true;
            }

            void CalculateAmount(AuraEffect const* /*aurEff*/, float& amount, bool& /*canBeRecalculated*/)
            {
                if(Unit* caster = GetCaster())
                    amount = int32((caster->GetSpellPowerDamage(SPELL_SCHOOL_MASK_HOLY) * GetSpellInfo()->Effects[EFFECT_1]->BasePoints / 1000 + 26.72716306 * amount) / 20);
            }

            void HandleTick(AuraEffect const* aurEff, float& amount, Unit* /*target*/, bool /*crit*/)
            {
                if(aurEff->GetTotalTicks() == aurEff->GetTickNumber())
                    amount *= 9;
                else if(aurEff->GetTickNumber() == 20)
                {
                    amount += int32((saveTick + 1) * 0.025f * amount);
                    amount *= 5; // 500% for next tick
                }
                else
                    amount += int32(aurEff->GetTickNumber() * 0.025f * amount);
                saveTick = aurEff->GetTickNumber();

            }

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                if (Unit* target = GetTarget())
                {
                    AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                    if (removeMode == AURA_REMOVE_BY_ENEMY_SPELL)
                    {
                        const_cast<AuraEffect*>(aurEff)->SetTickNumber(20);
                        aurEff->HandlePeriodicDamageAurasTick(target, caster, (SpellEffIndex) aurEff->GetEffIndex());
                    }
                }
            }

            int32 saveTick;

            void Register()
            {
                DoEffectBeforeCalcAmount += AuraEffectCalcAmountFn(spell_pal_execution_sentence_damage_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                DoEffectChangeTickDamage += AuraEffectChangeTickDamageFn(spell_pal_execution_sentence_damage_AuraScript::HandleTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                OnEffectRemove += AuraEffectRemoveFn(spell_pal_execution_sentence_damage_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pal_execution_sentence_damage_AuraScript();
        }
};

// for glyhp 146955 - 64364 Devotion Aura
class spell_pal_devotion_aura : public SpellScriptLoader
{
    public:
        spell_pal_devotion_aura() : SpellScriptLoader("spell_pal_devotion_aura") { }

        class spell_pal_devotion_aura_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_devotion_aura_SpellScript);

            void FilterScript(std::list<WorldObject*>& unitList)
            {
                if (Unit* caster = GetCaster())
                {
                    if (caster->HasAura(146955))
                    {
                        unitList.clear();
                        unitList.push_back(caster);
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_devotion_aura_SpellScript::FilterScript, EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_devotion_aura_SpellScript::FilterScript, EFFECT_1, TARGET_UNIT_CASTER_AREA_RAID);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_devotion_aura_SpellScript::FilterScript, EFFECT_2, TARGET_UNIT_CASTER_AREA_RAID);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_devotion_aura_SpellScript();
        }
};

// for glyhp 115738 and 54935 - 119072 Holy Wrath
class spell_pal_holy_wrath : public SpellScriptLoader
{
    public:
        spell_pal_holy_wrath() : SpellScriptLoader("spell_pal_holy_wrath") { }

        class spell_pal_holy_wrath_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_holy_wrath_SpellScript);

            bool Load()
            {
                targetcount = 0;
                return true;
            }

            void FilterScript(std::list<WorldObject*>& unitList)
            {
                if (GetCaster())
                {
                    if (GetCaster()->HasAura(115738))
                    {
                        if (Player* _player = GetCaster()->ToPlayer())
                        {
                            if (Unit* target = _player->GetSelectedUnit())
                            {
                                unitList.clear();
                                unitList.push_back(target);
                            }
                        }
                        else
                            if (unitList.size() > 1)
                                Trinity::Containers::RandomResizeList(unitList, 1);
                    }
                    else
                        targetcount = unitList.size();
                }
            }

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                int32 dmg = !targetcount ? GetHitDamage() : GetHitDamage() / targetcount;
                if (GetCaster() && GetHitUnit())
                {
                    if (GetCaster()->HasAura(54935))
                    {
                        if (GetHitUnit()->HealthBelowPct(20))
                            SetHitDamage(dmg + (dmg/2));
                    }
                    else
                        SetHitDamage(dmg);
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pal_holy_wrath_SpellScript::FilterScript, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                OnEffectHitTarget += SpellEffectFn(spell_pal_holy_wrath_SpellScript::HandleDamage, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        private:
            int32 targetcount;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_holy_wrath_SpellScript();
        }
};

// Divine Protection - 498
class spell_pal_divine_protection : public SpellScriptLoader
{
    public:
        spell_pal_divine_protection() : SpellScriptLoader("spell_pal_divine_protection") { }

        class spell_pal_divine_protection_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_divine_protection_AuraScript);

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if(Aura* aura = caster->GetAura(144580))
                    {
                        float _heal = (aura->GetEffect(0)->GetAmount() * 0.75f) / 10.0f;
                        if(_heal)
                            caster->CastCustomSpell(caster, 144581, &_heal, NULL, NULL, true, NULL, aurEff);
                    }
                    if (Aura* aura = caster->GetAura(144580))
                        aura->GetEffect(0)->SetAmount(0);
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_pal_divine_protection_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pal_divine_protection_AuraScript();
        }
};

// Pursuit of Justice - 114694
class spell_pal_pursuit_of_justice : public SpellScriptLoader
{
    public:
        spell_pal_pursuit_of_justice() : SpellScriptLoader("spell_pal_pursuit_of_justice") { }

        class spell_pal_pursuit_of_justice_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_pursuit_of_justice_SpellScript);

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    caster->RemoveAurasDueToSpell(114695);
                    int32 increment = 5;
                    float amount = 15;
                    if(SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(26023))
                        increment = spellInfo->Effects[EFFECT_7]->BasePoints;

                    if(AuraEffect const* triggerBy = GetSpell()->GetTriggeredAuraEff())
                    {
                        amount += increment * triggerBy->GetAmount();

                        if(amount > 15)
                            caster->CastCustomSpell(caster, 114695, &amount, NULL, NULL, true);
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_pal_pursuit_of_justice_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_pursuit_of_justice_SpellScript();
        }
};

// Seal of Insight - 20167
class spell_pal_seal_of_insight : public SpellScriptLoader
{
    public:
        spell_pal_seal_of_insight() : SpellScriptLoader("spell_pal_seal_of_insight") { }

        class spell_pal_seal_of_insight_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_seal_of_insight_SpellScript);

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                if (GetHitUnit() != GetCaster())
                    SetHitHeal(int32(GetHitHeal() * 0.3f));
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_pal_seal_of_insight_SpellScript::HandleDamage, EFFECT_1, SPELL_EFFECT_HEAL);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_seal_of_insight_SpellScript();
        }
};

// Holy Shock - 20473
class spell_pal_holy_shock : public SpellScriptLoader
{
    public:
        spell_pal_holy_shock() : SpellScriptLoader("spell_pal_holy_shock") { }

        class spell_pal_holy_shock_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_holy_shock_SpellScript);

            SpellCastResult CheckCast()
            {
                if(Unit* caster = GetCaster())
                {
                    Unit* target = GetExplTargetUnit();
                    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(25912);
                    if(target && spellInfo && !target->IsFriendlyTo(caster))
                        if((spellInfo->FacingCasterFlags & SPELL_FACING_FLAG_INFRONT) && !caster->HasInArc(static_cast<float>(M_PI), target))
                            return SPELL_FAILED_UNIT_NOT_INFRONT;
                }

                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_pal_holy_shock_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_holy_shock_SpellScript();
        }
};

// Glyph of Double Jeopardy - 121027
class spell_pal_glyph_of_double_jeopardy : public SpellScriptLoader
{
    public:
        spell_pal_glyph_of_double_jeopardy() : SpellScriptLoader("spell_pal_glyph_of_double_jeopardy") { }

        class spell_pal_glyph_of_double_jeopardyAuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_glyph_of_double_jeopardyAuraScript);

            void SaveTarget(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = _player->GetSelectedUnit())
                    {
                        GetAura()->ClearEffectTarget();
                        GetAura()->AddEffectTarget(target->GetGUID());
                    }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_pal_glyph_of_double_jeopardyAuraScript::SaveTarget, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        // function which creates AuraScript
        AuraScript* GetAuraScript() const
        {
            return new spell_pal_glyph_of_double_jeopardyAuraScript();
        }
};

// Judgment - 20271
class spell_pal_judgment : public SpellScriptLoader
{
    public:
        spell_pal_judgment() : SpellScriptLoader("spell_pal_judgment") { }

        class spell_pal_judgment_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_judgment_SpellScript);

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Aura* glyph = caster->GetAura(121027))
                    {
                        uint64 savetarget = glyph->GetRndEffectTarget();
                        if (Unit* unitTarget = GetHitUnit())
                        {
                            if(savetarget != unitTarget->GetGUID())
                            {
                                int32 _amount = GetHitDamage();
                                _amount += CalculatePct(GetHitDamage(), glyph->GetEffect(0)->GetAmount());

                                SetHitDamage(_amount);
                                glyph->ClearEffectTarget();
                                glyph->AddEffectTarget(unitTarget->GetGUID());
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_pal_judgment_SpellScript::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_judgment_SpellScript();
        }
};

// Exorcism - 122032
class spell_pal_exorcism : public SpellScriptLoader
{
    public:
        spell_pal_exorcism() : SpellScriptLoader("spell_pal_exorcism") { }

        class spell_pal_exorcism_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pal_exorcism_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* unitTarget = GetHitUnit())
                    GetSpell()->AddEffectTarget(unitTarget->GetGUID());
            }

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                if (Unit* unitTarget = GetHitUnit())
                {
                    uint64 savetarget = GetSpell()->GetRndEffectTarget();
                    if(savetarget != unitTarget->GetGUID())
                        SetHitDamage(GetHitDamage() / 4);
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_pal_exorcism_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
                OnEffectHitTarget += SpellEffectFn(spell_pal_exorcism_SpellScript::HandleDamage, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pal_exorcism_SpellScript();
        }
};

// Shield of Glory - 138242
class spell_pal_shield_of_glory : public SpellScriptLoader
{
    public:
        spell_pal_shield_of_glory() : SpellScriptLoader("spell_pal_shield_of_glory") { }

        class spell_pal_shield_of_glory_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pal_shield_of_glory_AuraScript);

            void CalculateMaxDuration(int32& duration)
            {
                if (Unit* caster = GetCaster())
                {
                    if(caster->HasAura(PALADIN_SPELL_DIVINE_PURPOSE))
                        duration *= 3;
                    else
                        duration *= caster->GetPower(POWER_HOLY_POWER);
                }
            }

            void Register()
            {
                DoCalcMaxDuration += AuraCalcMaxDurationFn(spell_pal_shield_of_glory_AuraScript::CalculateMaxDuration);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pal_shield_of_glory_AuraScript();
        }
};

void AddSC_paladin_spell_scripts()
{
    new spell_pal_glyph_of_avenging_wrath();
    new spell_pal_shield_of_the_righteous();
    new spell_pal_tower_of_radiance();
    new spell_pal_sacred_shield();
    new spell_pal_emancipate();
    new spell_pal_art_of_war();
    new spell_pal_blinding_light();
    new spell_pal_hand_of_protection();
    new spell_pal_divine_shield();
    new spell_pal_execution_sentence();
    new spell_pal_lights_hammer();
    new spell_pal_holy_prism_heal();
    new spell_pal_holy_prism_damage();
    new spell_pal_holy_prism_effect();
    new spell_pal_holy_prism();
    new spell_pal_consecration();
    new spell_pal_consecration_override();
    new spell_pal_ardent_defender();
    new spell_pal_blessing_of_faith();
    new spell_pal_lay_on_hands();
    new spell_pal_righteous_defense();
    new spell_pal_selfless_healer();
    new spell_pal_hand_of_purity();
    new spell_pal_stay_of_execution();
    new spell_pal_execution_sentence_damage();
    new spell_pal_devotion_aura();
    new spell_pal_holy_wrath();
    new spell_pal_divine_protection();
    new spell_pal_pursuit_of_justice();
    new spell_pal_seal_of_insight();
    new spell_pal_holy_shock();
    new spell_pal_glyph_of_double_jeopardy();
    new spell_pal_judgment();
    new spell_pal_exorcism();
    new spell_pal_shield_of_glory();
}
