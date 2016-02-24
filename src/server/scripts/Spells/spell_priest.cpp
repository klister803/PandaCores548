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
 * Scripts for spells with SPELLFAMILY_PRIEST and SPELLFAMILY_GENERIC spells used by priest players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_pri_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"

enum PriestSpells
{
    PRIEST_SPELL_GUARDIAN_SPIRIT_HEAL           = 48153,
    PRIEST_SPELL_PENANCE                        = 47540,
    PRIEST_SPELL_PENANCE_DAMAGE                 = 47758,
    PRIEST_SPELL_PENANCE_HEAL                   = 47757,
    PRIEST_SPELL_REFLECTIVE_SHIELD_TRIGGERED    = 33619,
    PRIEST_SHADOW_WORD_DEATH                    = 32409,
    PRIEST_ICON_ID_PAIN_AND_SUFFERING           = 2874,
    PRIEST_SHADOWFORM_VISUAL_WITHOUT_GLYPH      = 107903,
    PRIEST_SHADOWFORM_VISUAL_WITH_GLYPH         = 107904,
    PRIEST_GLYPH_OF_SHADOW                      = 107906,
    PRIEST_VOID_SHIFT                           = 108968,
    PRIEST_LEAP_OF_FAITH                        = 73325,
    PRIEST_LEAP_OF_FAITH_JUMP                   = 92832,
    PRIEST_INNER_WILL                           = 73413,
    PRIEST_INNER_FIRE                           = 588,
    PRIEST_NPC_SHADOWY_APPARITION               = 61966,
    PRIEST_SPELL_HALO_HEAL_SHADOW               = 120696,
    PRIEST_SPELL_HALO_HEAL_HOLY                 = 120692,

    // Cascade
    PRIEST_CASCADE_HOLY_DAMAGE                  = 120785,
    PRIEST_CASCADE_HOLY_TRIGGER                 = 120786,
    PRIEST_CASCADE_INVISIBLE_AURA               = 120840,
    PRIEST_CASCADE_HOLY_TALENT                  = 121135,
    PRIEST_CASCADE_HOLY_MISSILE                 = 121146,
    PRIEST_CASCADE_HOLY_HEAL                    = 121148,
    PRIEST_CASCADE_SHADOW_MISSILE               = 127627,
    PRIEST_CASCADE_SHADOW_DAMAGE                = 127628,
    PRIEST_CASCADE_SHADOW_HEAL                  = 127629,
    PRIEST_CASCADE_DAMAGE_TRIGGER               = 127630,
    PRIEST_CASCADE_INVISIBLE_AURA_2             = 127631,
    PRIEST_CASCADE_SHADOW_TALENT                = 127632,

    PRIEST_SHADOWFORM_STANCE                    = 15473,
    PRIEST_SHADOW_WORD_PAIN                     = 589,
    PRIEST_DEVOURING_PLAGUE                     = 2944,
    PRIEST_DEVOURING_PLAGUE_HEAL                = 127626,
    PRIEST_VAMPIRIC_TOUCH                       = 34914,
    PRIEST_SPIRIT_SHELL_AURA                    = 109964,
    PRIEST_SPIRIT_SHELL_ABSORPTION              = 114908,
    PRIEST_ATONEMENT_AURA                       = 81749,
    PRIEST_ATONEMENT_HEAL                       = 81751,
    PRIEST_RAPTURE_ENERGIZE                     = 47755,
    PRIEST_RAPTURE_AURA                         = 47536,
    PRIEST_TRAIN_OF_THOUGHT                     = 92297,
    PRIEST_INNER_FOCUS                          = 89485,
    PRIEST_GRACE_AURA                           = 47517,
    PRIEST_GRACE_PROC                           = 77613,
    PRIEST_STRENGTH_OF_SOUL_AURA                = 89488,
    PRIEST_STRENGTH_OF_SOUL_REDUCE_TIME         = 89490,
    PRIEST_WEAKENED_SOUL                        = 6788,
    PRIEST_STRENGTH_OF_SOUL                     = 89488,
    PRIEST_EVANGELISM_AURA                      = 81662,
    PRIEST_EVANGELISM_STACK                     = 81661,
    PRIEST_ARCHANGEL                            = 81700,
    LIGHTWELL_CHARGES                           = 59907,
    LIGHTWELL_CHARGES_MOP                       = 126150,
    LIGHTSPRING_RENEW                           = 7001,
    LIGHTSPRING_RENEW_MOP                       = 126154,
    LIGHTSPRING_RENEW_VISUAL                    = 126141,
    PRIEST_SMITE                                = 585,
    PRIEST_HOLY_WORD_CHASTISE                   = 88625,
    PRIEST_HOLY_WORD_SANCTUARY_AREA             = 88685,
    PRIEST_HOLY_WORD_SANCTUARY_HEAL             = 88686,
    PRIEST_RAPID_RENEWAL_AURA                   = 95649,
    PRIEST_SPELL_EMPOWERED_RENEW                = 63544,
    PRIEST_SPELL_DIVINE_INSIGHT_TALENT          = 109175,
    PRIEST_SPELL_DIVINE_INSIGHT_DISCIPLINE      = 123266,
    PRIEST_SPELL_DIVINE_INSIGHT_HOLY            = 123267,
    PRIEST_PRAYER_OF_MENDING                    = 33076,
    PRIEST_PRAYER_OF_MENDING_HEAL               = 33110,
    PRIEST_PRAYER_OF_MENDING_RADIUS             = 123262,
    PRIEST_BODY_AND_SOUL_AURA                   = 64129,
    PRIEST_BODY_AND_SOUL_INCREASE_SPEED         = 65081,
    PRIEST_FROM_DARKNESS_COMES_LIGHT_AURA       = 109186,
    PRIEST_SURGE_OF_LIGHT                       = 114255,
    PRIEST_SURGE_OF_DARKNESS                    = 87160,
    PRIEST_SHADOW_WORD_INSANITY_ALLOWING_CAST   = 130733,
    PRIEST_SHADOW_WORD_INSANITY_DAMAGE          = 129249,
    PRIEST_SPELL_MIND_BLAST                     = 8092,
    PRIEST_SPELL_2P_S12_SHADOW                  = 92711,
    PRIEST_SPELL_DISPERSION_SPRINT              = 129960,
    PRIEST_SPELL_4P_S12_SHADOW                  = 131556,
    PRIEST_SPELL_SIN_AND_PUNISHMENT             = 87204,
    PRIEST_SPELL_2P_S12_HEAL                    = 33333,
    PRIEST_SPELL_SOUL_OF_DIAMOND                = 96219,
    PRIEST_SPELL_4P_S12_HEAL                    = 131566,
    PRIEST_SPELL_HOLY_SPARK                     = 131567,
    PRIEST_SPELL_VOID_TENDRILS                  = 114404,
    PRIEST_SPELL_SHADOWFIEND_TRIGGERED          = 28305,
    PRIEST_SPELL_MINDBENDER_TRIGGERED           = 123050,
};

 class spell_pri_glyph_of_mass_dispel : public SpellScriptLoader
 {
 public:
     spell_pri_glyph_of_mass_dispel() : SpellScriptLoader("spell_pri_glyph_of_mass_dispel") { }

     class spell_pri_glyph_of_mass_dispel_SpellScript : public SpellScript
     {
         PrepareSpellScript(spell_pri_glyph_of_mass_dispel_SpellScript);

         void HandleDispel()
         {
             if (Unit* caster = GetCaster())
                if (caster->HasAura(55691)) // Glyph
                    if (WorldLocation const* dest = GetExplTargetDest())
                        caster->CastSpell(dest->GetPositionX(), dest->GetPositionY(), dest->GetPositionZ(), 39897, true);
         }

         void Register()
         {
             OnCast += SpellCastFn(spell_pri_glyph_of_mass_dispel_SpellScript::HandleDispel);
         }
     };

     SpellScript* GetSpellScript() const
     {
         return new spell_pri_glyph_of_mass_dispel_SpellScript;
     }
 };

// Called by Prayer of Mending - 33076
// Item : S12 4P bonus - Heal
class spell_pri_item_s12_4p_heal : public SpellScriptLoader
{
    public:
        spell_pri_item_s12_4p_heal() : SpellScriptLoader("spell_pri_item_s12_4p_heal") { }

        class spell_pri_item_s12_4p_heal_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_item_s12_4p_heal_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (_player->HasAura(PRIEST_SPELL_2P_S12_HEAL))
                            _player->CastSpell(target, PRIEST_SPELL_HOLY_SPARK, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_item_s12_4p_heal_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_item_s12_4p_heal_SpellScript();
        }
};

// Called by Dispersion - 47585
// Item : S12 2P bonus - Shadow
class spell_pri_item_s12_2p_shadow : public SpellScriptLoader
{
    public:
        spell_pri_item_s12_2p_shadow() : SpellScriptLoader("spell_pri_item_s12_2p_shadow") { }

        class spell_pri_item_s12_2p_shadow_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_item_s12_2p_shadow_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (_player->HasAura(PRIEST_SPELL_2P_S12_SHADOW))
                        _player->CastSpell(_player, PRIEST_SPELL_DISPERSION_SPRINT, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_item_s12_2p_shadow_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_item_s12_2p_shadow_SpellScript();
        }
};

// Power Word - Solace - 129250
class spell_pri_power_word_solace : public SpellScriptLoader
{
    public:
        spell_pri_power_word_solace() : SpellScriptLoader("spell_pri_power_word_solace") { }

        class spell_pri_power_word_solace_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_power_word_solace_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                    caster->CastSpell(caster, 129253, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_power_word_solace_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_power_word_solace_SpellScript();
        }
};

// Called by Shadow Word : Pain - 589
// Shadow Word : Insanity (allowing cast) - 130733
class spell_pri_shadow_word_insanity_allowing : public SpellScriptLoader
{
    public:
        spell_pri_shadow_word_insanity_allowing() : SpellScriptLoader("spell_pri_shadow_word_insanity_allowing") { }

        class spell_pri_shadow_word_insanity_allowing_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_shadow_word_insanity_allowing_AuraScript);

            std::list<Unit*> targetList;

            void OnUpdate(uint32 diff, AuraEffect* aurEff)
            {
                aurEff->GetTargetList(targetList);

                for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    if (Unit* caster = GetCaster())
                    {
                        if (Aura* shadowWordPain = (*itr)->GetAura(PRIEST_SHADOW_WORD_PAIN, caster->GetGUID()))
                        {
                            if (shadowWordPain->GetDuration() <= (shadowWordPain->GetEffect(0)->GetAmplitude() * 2))
                                caster->CastSpell(*itr, PRIEST_SHADOW_WORD_INSANITY_ALLOWING_CAST, true);
                            else
                                (*itr)->RemoveAura(PRIEST_SHADOW_WORD_INSANITY_ALLOWING_CAST);
                        }
                    }
                }

                targetList.clear();
            }

            void Register()
            {
                OnEffectUpdate += AuraEffectUpdateFn(spell_pri_shadow_word_insanity_allowing_AuraScript::OnUpdate, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_shadow_word_insanity_allowing_AuraScript();
        }
};

// Called by Flash Heal - 2061
// Surge of Light - 114255
class spell_pri_surge_of_light : public SpellScriptLoader
{
    public:
        spell_pri_surge_of_light() : SpellScriptLoader("spell_pri_surge_of_light") { }

        class spell_pri_surge_of_light_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_surge_of_light_SpellScript);

            void HandleOnCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Aura* surgeOfLight = _player->GetAura(PRIEST_SURGE_OF_LIGHT))
                        surgeOfLight->ModStackAmount(-1);
                }
            }

            void Register()
            {
                OnCast += SpellCastFn(spell_pri_surge_of_light_SpellScript::HandleOnCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_surge_of_light_SpellScript();
        }
};

// Called by Leap of Faith - 73325 and Power Word : Shield - 17
// Body and Soul - 64129
class spell_pri_body_and_soul : public SpellScriptLoader
{
    public:
        spell_pri_body_and_soul() : SpellScriptLoader("spell_pri_body_and_soul") { }

        class spell_pri_body_and_soul_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_body_and_soul_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (_player->HasAura(PRIEST_BODY_AND_SOUL_AURA))
                            _player->CastSpell(target, PRIEST_BODY_AND_SOUL_INCREASE_SPEED, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_body_and_soul_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_body_and_soul_SpellScript();
        }
};

// Prayer of Mending (Divine Insight) - 123259
class spell_pri_prayer_of_mending_divine_insight : public SpellScriptLoader
{
    public:
        spell_pri_prayer_of_mending_divine_insight() : SpellScriptLoader("spell_pri_prayer_of_mending_divine_insight") { }

        class spell_pri_prayer_of_mending_divine_insight_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_prayer_of_mending_divine_insight_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (Aura* prayerOfMending = target->GetAura(PRIEST_PRAYER_OF_MENDING_RADIUS, _player->GetGUID()))
                        {
                            int32 value = prayerOfMending->GetEffect(0)->GetAmount();

                            if (_player->HasAura(PRIEST_SPELL_DIVINE_INSIGHT_HOLY))
                                _player->RemoveAura(PRIEST_SPELL_DIVINE_INSIGHT_HOLY);

                            target->CastCustomSpell(target, PRIEST_PRAYER_OF_MENDING_HEAL, &value, NULL, NULL, true, NULL, NULL, _player->GetGUID());
                            if (target->HasAura(GetSpellInfo()->Id))
                                target->RemoveAura(GetSpellInfo()->Id);

                            float radius = sSpellMgr->GetSpellInfo(PRIEST_PRAYER_OF_MENDING_RADIUS)->Effects[0].CalcRadius(_player);

                            if (Unit* secondTarget = target->GetNextRandomRaidMemberOrPet(radius))
                            {
                                target->CastCustomSpell(secondTarget, PRIEST_PRAYER_OF_MENDING, &value, NULL, NULL, true, NULL, NULL, _player->GetGUID());
                                if (secondTarget->HasAura(PRIEST_PRAYER_OF_MENDING))
                                    secondTarget->RemoveAura(PRIEST_PRAYER_OF_MENDING);

                                secondTarget->CastCustomSpell(secondTarget, PRIEST_PRAYER_OF_MENDING_HEAL, &value, NULL, NULL, true, NULL, NULL, _player->GetGUID());

                                if (Unit* thirdTarget = target->GetNextRandomRaidMemberOrPet(radius))
                                {
                                    secondTarget->CastCustomSpell(thirdTarget, PRIEST_PRAYER_OF_MENDING, &value, NULL, NULL, true, NULL, NULL, _player->GetGUID());
                                    if (thirdTarget->HasAura(PRIEST_PRAYER_OF_MENDING))
                                        thirdTarget->RemoveAura(PRIEST_PRAYER_OF_MENDING);

                                    thirdTarget->CastCustomSpell(thirdTarget, PRIEST_PRAYER_OF_MENDING_HEAL, &value, NULL, NULL, true, NULL, NULL, _player->GetGUID());

                                    if (Unit* fourthTarget = target->GetNextRandomRaidMemberOrPet(radius))
                                    {
                                        thirdTarget->CastCustomSpell(fourthTarget, PRIEST_PRAYER_OF_MENDING, &value, NULL, NULL, true, NULL, NULL, _player->GetGUID());
                                        if (fourthTarget->HasAura(PRIEST_PRAYER_OF_MENDING))
                                            fourthTarget->RemoveAura(PRIEST_PRAYER_OF_MENDING);

                                        fourthTarget->CastCustomSpell(fourthTarget, PRIEST_PRAYER_OF_MENDING_HEAL, &value, NULL, NULL, true, NULL, NULL, _player->GetGUID());

                                        if (Unit* fifthTarget = target->GetNextRandomRaidMemberOrPet(radius))
                                        {
                                            fourthTarget->CastCustomSpell(fifthTarget, PRIEST_PRAYER_OF_MENDING, &value, NULL, NULL, true, NULL, NULL, _player->GetGUID());
                                            if (fifthTarget->HasAura(PRIEST_PRAYER_OF_MENDING))
                                                fifthTarget->RemoveAura(PRIEST_PRAYER_OF_MENDING);

                                            fifthTarget->CastCustomSpell(fifthTarget, PRIEST_PRAYER_OF_MENDING_HEAL, &value, NULL, NULL, true, NULL, NULL, _player->GetGUID());
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_prayer_of_mending_divine_insight_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_prayer_of_mending_divine_insight_SpellScript();
        }
};

// Called by Greater Heal - 2060 and Prayer of Healing - 596
// Divine Insight (Holy) - 109175
class spell_pri_divine_insight_holy : public SpellScriptLoader
{
    public:
        spell_pri_divine_insight_holy() : SpellScriptLoader("spell_pri_divine_insight_holy") { }

        class spell_pri_divine_insight_holy_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_divine_insight_holy_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (_player->HasAura(PRIEST_SPELL_DIVINE_INSIGHT_TALENT))
                        if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_PRIEST_HOLY)
                            if (roll_chance_i(40))
                                _player->CastSpell(_player, PRIEST_SPELL_DIVINE_INSIGHT_HOLY, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_divine_insight_holy_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_divine_insight_holy_SpellScript();
        }
};

// Called by Power Word : Shield (Divine Insight) - 123258
// Divine Insight (Discipline) - 123266
class spell_pri_divine_insight_discipline : public SpellScriptLoader
{
    public:
        spell_pri_divine_insight_discipline() : SpellScriptLoader("spell_pri_divine_insight_discipline") { }

        class spell_pri_divine_insight_discipline_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_divine_insight_discipline_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (_player->HasAura(PRIEST_SPELL_DIVINE_INSIGHT_DISCIPLINE))
                        _player->RemoveAura(PRIEST_SPELL_DIVINE_INSIGHT_DISCIPLINE);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_divine_insight_discipline_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_divine_insight_discipline_SpellScript();
        }
};

// Holy Word : Sanctuary - 88685
class spell_pri_holy_word_sanctuary : public SpellScriptLoader
{
    public:
        spell_pri_holy_word_sanctuary() : SpellScriptLoader("spell_pri_holy_word_sanctuary") { }

        class spell_pri_holy_word_sanctuary_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_holy_word_sanctuary_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* m_caster = GetCaster())
                    if (DynamicObject* dynObj = m_caster->GetDynObject(PRIEST_HOLY_WORD_SANCTUARY_AREA))
                        m_caster->CastSpell(dynObj->GetPositionX(), dynObj->GetPositionY(), dynObj->GetPositionZ(), PRIEST_HOLY_WORD_SANCTUARY_HEAL, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_pri_holy_word_sanctuary_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_holy_word_sanctuary_AuraScript();
        }
};

// Vampiric Embrace - 15286
class spell_pri_vampiric_embrace : public SpellScriptLoader
{
public:
    spell_pri_vampiric_embrace() : SpellScriptLoader("spell_pri_vampiric_embrace") { }

    class spell_pri_vampiric_embrace_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_pri_vampiric_embrace_AuraScript);

        void OnTick(AuraEffect const* aurEff)
        {
            if (Unit* caster = GetCaster())
                if (AuraEffect* eff = GetEffect(EFFECT_0))
                    if (int32 bp = eff->GetAmount())
                    {
                        caster->CastCustomSpell(caster, 15290, &bp, NULL, NULL, true, NULL, eff);
                        eff->SetAmount(0);
                    }
        }

        void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
        {
            amount = 0;
        }

        void Register()
        {
            OnEffectPeriodic += AuraEffectPeriodicFn(spell_pri_vampiric_embrace_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pri_vampiric_embrace_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_pri_vampiric_embrace_AuraScript();
    }
};


// Called by Smite - 585
// Chakra : Chastise - 81209
class spell_pri_chakra_chastise : public SpellScriptLoader
{
    public:
        spell_pri_chakra_chastise() : SpellScriptLoader("spell_pri_chakra_chastise") { }

        class spell_pri_chakra_chastise_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_chakra_chastise_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (roll_chance_i(10))
                            if (_player->HasSpellCooldown(PRIEST_HOLY_WORD_CHASTISE))
                                _player->RemoveSpellCooldown(PRIEST_HOLY_WORD_CHASTISE, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_chakra_chastise_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_chakra_chastise_SpellScript();
        }
};

// Lightwell Renew - 60123
class spell_pri_lightwell_renew : public SpellScriptLoader
{
    public:
        spell_pri_lightwell_renew() : SpellScriptLoader("spell_pri_lightwell_renew") { }

        class spell_pri_lightwell_renew_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_lightwell_renew_SpellScript);

            void HandleOnHit()
            {
                if (Unit* m_caster = GetCaster())
                {
                    if (Unit* unitTarget = GetHitUnit())
                    {
                        if (m_caster->GetTypeId() != TYPEID_UNIT || !m_caster->ToCreature()->isSummon())
                            return;

                        // proc a spellcast
                        if (Aura* chargesAura = m_caster->GetAura(LIGHTWELL_CHARGES))
                        {
                            m_caster->CastSpell(unitTarget, LIGHTSPRING_RENEW, true, NULL, NULL, m_caster->ToTempSummon()->GetSummonerGUID());
                            if (chargesAura->ModStackAmount(-1))
                                m_caster->ToTempSummon()->UnSummon();
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_lightwell_renew_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_lightwell_renew_SpellScript();
        }
};

// Called by Heal - 2050, Greater Heal - 2060 and Flash Heal - 2061
// Strength of Soul - 89488
class spell_pri_strength_of_soul : public SpellScriptLoader
{
    public:
        spell_pri_strength_of_soul() : SpellScriptLoader("spell_pri_strength_of_soul") { }

        class spell_pri_strength_of_soul_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_strength_of_soul_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (_player->HasAura(PRIEST_STRENGTH_OF_SOUL))
                        {
                            if (Aura* weakenedSoul = target->GetAura(PRIEST_WEAKENED_SOUL, _player->GetGUID()))
                            {
                                if (weakenedSoul->GetDuration() > 2000)
                                    weakenedSoul->SetDuration(weakenedSoul->GetDuration() - 2000);
                                else
                                    target->RemoveAura(PRIEST_WEAKENED_SOUL);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_strength_of_soul_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_strength_of_soul_SpellScript();
        }
};

// Called by Heal - 2050
// Grace - 47517
class spell_pri_grace : public SpellScriptLoader
{
    public:
        spell_pri_grace() : SpellScriptLoader("spell_pri_grace") { }

        class spell_pri_grace_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_grace_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (_player->HasAura(PRIEST_GRACE_AURA))
                            _player->CastSpell(target, PRIEST_GRACE_PROC, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_grace_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_grace_SpellScript();
        }
};

// Called by Power Word : Shield - 17
// Rapture - 47536
class spell_pri_rapture : public SpellScriptLoader
{
    public:
        spell_pri_rapture() : SpellScriptLoader("spell_pri_rapture") { }

        class spell_pri_rapture_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_rapture_AuraScript);

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();
                    if (removeMode == AURA_REMOVE_BY_ENEMY_SPELL)
                    {
                        int32 bp = GetSpellInfo()->CalcPowerCost(caster, GetSpellInfo()->GetSchoolMask());
                        if (caster->ToPlayer() && !caster->ToPlayer()->HasSpellCooldown(PRIEST_RAPTURE_ENERGIZE))
                        {
                            caster->CastCustomSpell(caster, PRIEST_RAPTURE_ENERGIZE, &bp, NULL, NULL, true);
                            caster->ToPlayer()->AddSpellCooldown(PRIEST_RAPTURE_ENERGIZE, 0, getPreciseTime() + 12.0);
                        }
                    }
                }
            }

            void CalculateAmount(AuraEffect const* , int32 & amount, bool & )
            {
                Unit* caster = GetCaster();
                if (!caster || !caster->ToPlayer())
                    return;

                if (Aura* aur = caster->GetAura(55672))// Glyph of Power Word: Shield
                {
                    int32 percent = aur->GetEffect(EFFECT_0)->GetAmount();
                    int32 bp = CalculatePct(amount, percent);
                    amount -= bp;
                    caster->CastCustomSpell(caster, 56160, &bp, NULL, NULL,  true);
                }
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pri_rapture_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectRemove += AuraEffectRemoveFn(spell_pri_rapture_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_rapture_AuraScript();
        }
};

// Atonement - 81751
class spell_pri_atonement : public SpellScriptLoader
{
    public:
        spell_pri_atonement() : SpellScriptLoader("spell_pri_atonement") { }

        class spell_pri_atonement_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_atonement_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if(caster == target)
                            SetHitHeal(int32(GetHitHeal() / 2));
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_atonement_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_atonement_SpellScript();
        }
};

// Called by Heal - 2050, Flash Heal - 2061, Greater Heal - 2060 and Prayer of Healing - 596
// Spirit Shell - 109964
class spell_pri_spirit_shell : public SpellScriptLoader
{
    public:
        spell_pri_spirit_shell() : SpellScriptLoader("spell_pri_spirit_shell") { }

        class spell_pri_spirit_shell_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_spirit_shell_SpellScript);

            void HandleOnHit()
            {
                Unit* caster = GetCaster();
                if (caster->HasAura(PRIEST_SPIRIT_SHELL_AURA))
                {
                    Unit* target = GetHitUnit();
                    int32 bp = GetHitHeal();
                    if (AuraEffect* aurEff = target->GetAuraEffect(PRIEST_SPIRIT_SHELL_ABSORPTION, 0))
                    {
                        bp += aurEff->GetAmount();
                    }
                    if (bp > int32(CalculatePct(target->GetMaxHealth(), 60)))
                    {
                        bp = int32(CalculatePct(target->GetMaxHealth(), 60));
                    }
                    caster->CastCustomSpell(target, PRIEST_SPIRIT_SHELL_ABSORPTION, &bp, NULL, NULL,  true);
                    SetHitHeal(0);
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_spirit_shell_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_spirit_shell_SpellScript();
        }
};

// Devouring Plague - 2944
class spell_pri_devouring_plague : public SpellScriptLoader
{
    public:
        spell_pri_devouring_plague() : SpellScriptLoader("spell_pri_devouring_plague") { }

        class spell_pri_devouring_plague_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_devouring_plague_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_PRIEST_SHADOW)
                        {
                            int32 currentPower = _player->GetPower(POWER_SHADOW_ORB) + 1; // Don't forget PowerCost

                            if(Aura* aura = _player->GetAura(145179)) // Item - Priest T16 Shadow 4P Bonus
                                aura->GetEffect(0)->SetAmount(20 * currentPower);

                            _player->ModifyPower(POWER_SHADOW_ORB, -currentPower, true);
                            // Shadow Orb visual
                            if (_player->HasAura(77487))
                                _player->RemoveAura(77487);
                            // Glyph of Shadow Ravens
                            else if (_player->HasAura(127850))
                                _player->RemoveAura(127850);

                            // Instant damage equal to amount of shadow orb
                            SetHitDamage(int32(GetHitDamage() * currentPower / 3));

                            if (Aura* aur = target->GetAura(GetSpellInfo()->Id, _player->GetGUID()))
                            {
                                if (AuraEffect* eff = aur->GetEffect(EFFECT_2))
                                    eff->SetAmount(100 * currentPower / 3);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_devouring_plague_SpellScript::HandleOnHit);
            }
        };

        class spell_pri_devouring_plague_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_devouring_plague_AuraScript);

            int32 orbCount;

            bool Load()
            {
                if(Unit* caster = GetCaster())
                    orbCount = caster->GetPower(POWER_SHADOW_ORB) + 1;
                GetAura()->SetCustomData(orbCount);
                return true;
            }

            void OnPereodic(AuraEffect const* /*aurEff*/)
            {
                if(Unit* caster = GetCaster())
                    caster->CastCustomSpell(caster, PRIEST_DEVOURING_PLAGUE_HEAL, &orbCount, 0, 0, true);
            }

            void HandleTick(AuraEffect const* /*aurEff*/, int32& amount, Unit* /*target*/, bool /*crit*/)
            {
                amount *= orbCount;
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_pri_devouring_plague_AuraScript::OnPereodic, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
                DoEffectChangeTickDamage += AuraEffectChangeTickDamageFn(spell_pri_devouring_plague_AuraScript::HandleTick, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_devouring_plague_AuraScript();
        }

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_devouring_plague_SpellScript;
        }
};

// Mind Spike - 73510
class spell_pri_mind_spike : public SpellScriptLoader
{
    public:
        spell_pri_mind_spike() : SpellScriptLoader("spell_pri_mind_spike") { }

        class spell_pri_mind_spike_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_mind_spike_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        // Surge of Darkness - Your next Mind Spike will not consume your damage-over-time effects ...
                        if (!_player->HasAura(PRIEST_SURGE_OF_DARKNESS))
                        {
                            // Mind Spike remove all DoT on the target's
                            if (target->HasAura(PRIEST_SHADOW_WORD_PAIN, _player->GetGUID()))
                                target->RemoveAura(PRIEST_SHADOW_WORD_PAIN, _player->GetGUID());
                            if (target->HasAura(PRIEST_DEVOURING_PLAGUE, _player->GetGUID()))
                                target->RemoveAura(PRIEST_DEVOURING_PLAGUE, _player->GetGUID());
                            if (target->HasAura(PRIEST_VAMPIRIC_TOUCH, _player->GetGUID()))
                                target->RemoveAura(PRIEST_VAMPIRIC_TOUCH, _player->GetGUID());
                        }
                        // ... and deals 50% additional damage.
                        else if (Aura* surgeOfDarkness = _player->GetAura(PRIEST_SURGE_OF_DARKNESS))
                        {
                            SetHitDamage(int32(GetHitDamage() * 1.5f));

                            surgeOfDarkness->ModStackAmount(-1);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_mind_spike_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_mind_spike_SpellScript;
        }
};

// Archangel - 81700
class spell_pri_archangel : public SpellScriptLoader
{
    public:
        spell_pri_archangel() : SpellScriptLoader("spell_pri_archangel") { }

        class spell_pri_archangel_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_archangel_SpellScript);

			void HandleOnHit()
			{
				if (Player* _player = GetCaster()->ToPlayer())
				{
                    int stackNumber = _player->GetAura(PRIEST_EVANGELISM_STACK)->GetStackAmount();
					if (!(stackNumber > 0))
						return;

					if (Aura* archangel = _player->GetAura(GetSpellInfo()->Id))
					{
						if (archangel->GetEffect(0))
						{
                            archangel->GetEffect(0)->ChangeAmount(archangel->GetEffect(0)->GetAmount() * stackNumber);
                            _player->RemoveAura(PRIEST_EVANGELISM_STACK);
						}
					}
				}
			}

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_archangel_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_archangel_SpellScript;
        }
};

// Cascade - 127630 (damage trigger) or Cascade - 120786 (heal trigger)
class spell_pri_cascade_second : public SpellScriptLoader
{
    public:
        spell_pri_cascade_second() : SpellScriptLoader("spell_pri_cascade_second") { }

        class spell_pri_cascade_second_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_cascade_second_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        std::list<Unit*> checkAuras;
                        std::list<Unit*> targetList;
                        int32 affectedUnits = 0;

                        _player->GetAttackableUnitListInRange(targetList, 40.0f);

                        for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                        {
                            if ((*itr)->HasAura(PRIEST_CASCADE_INVISIBLE_AURA))
                                if (Unit* caster = (*itr)->GetAura(PRIEST_CASCADE_INVISIBLE_AURA)->GetCaster())
                                    if (caster->GetGUID() == _player->GetGUID())
                                        affectedUnits++;
                        }

                        // Stop the script if the max targets is reached ...
                        if (affectedUnits >= 15)
                            return;

                        if (Aura* boundNumber = _player->GetAura(PRIEST_CASCADE_INVISIBLE_AURA_2))
                            if (boundNumber->GetCharges() >= 3)
                                return;

                        for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                            checkAuras.push_back(*itr);

                        for (std::list<Unit*>::const_iterator itr = checkAuras.begin(); itr != checkAuras.end(); ++itr)
                        {
                            Unit* unit = *itr;
                            if (unit->HasAura(PRIEST_CASCADE_INVISIBLE_AURA))
                                if (Unit* caster = unit->GetAura(PRIEST_CASCADE_INVISIBLE_AURA)->GetCaster())
                                    if (caster->GetGUID() == _player->GetGUID())
                                        targetList.remove(unit);

                            if (!unit->IsWithinLOSInMap(_player))
                                targetList.remove(unit);

                            if (!unit->isInFront(_player))
                                targetList.remove(unit);

                            if (unit->GetGUID() == _player->GetGUID())
                                targetList.remove(unit);

                            // damage
                            if (GetSpellInfo()->Id == 127630)
                                if (!_player->IsValidAttackTarget(unit))
                                    targetList.remove(unit);

                            // heal
                            if (GetSpellInfo()->Id == 120786)
                                if (_player->IsValidAttackTarget(unit))
                                    targetList.remove(unit);
                        }

                        // ... or if there are no targets reachable
                        if (targetList.size() == 0)
                            return;

                        // Each bound hit twice more targets up to 8 for the same bound
                        Trinity::Containers::RandomResizeList(targetList, (affectedUnits * 2));

                        for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                        {
                            if (_player->HasAura(PRIEST_SHADOWFORM_STANCE))
                            {
                                switch (GetSpellInfo()->Id)
                                {
                                    // damage
                                    case 127630:
                                        target->CastSpell(*itr, PRIEST_CASCADE_SHADOW_DAMAGE, true, 0, NULL, _player->GetGUID());
                                        break;
                                    // heal
                                    case 120786:
                                        target->CastSpell(*itr, PRIEST_CASCADE_SHADOW_MISSILE, true, 0, NULL, _player->GetGUID());
                                        target->CastSpell(*itr, PRIEST_CASCADE_SHADOW_HEAL, true, 0, NULL, _player->GetGUID());
                                        break;
                                    default:
                                        break;
                                }
                            }
                            else
                            {
                                switch (GetSpellInfo()->Id)
                                {
                                    // damage
                                    case 127630:
                                        target->CastSpell(*itr, PRIEST_CASCADE_HOLY_DAMAGE, true, 0, NULL, _player->GetGUID());
                                        break;
                                    // heal
                                    case 120786:
                                        target->CastSpell(*itr, PRIEST_CASCADE_HOLY_MISSILE, true, 0, NULL, _player->GetGUID());
                                        target->CastSpell(*itr, PRIEST_CASCADE_HOLY_HEAL, true, 0, NULL, _player->GetGUID());
                                        break;
                                    default:
                                        break;
                                }
                            }

                            _player->CastSpell(*itr, PRIEST_CASCADE_INVISIBLE_AURA, true);
                        }

                        if (Aura* boundNumber = _player->GetAura(PRIEST_CASCADE_INVISIBLE_AURA_2))
                        {
                            boundNumber->RefreshDuration();
                            boundNumber->SetCharges(boundNumber->GetCharges() + 1);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_cascade_second_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_cascade_second_SpellScript;
        }
};

// Cascade - 120785 (holy damage) or Cascade - 127628 (shadow damage) or Cascade - 127627 (shadow missile) or Cascade - 121146 (holy missile)
class spell_pri_cascade_trigger : public SpellScriptLoader
{
    public:
        spell_pri_cascade_trigger() : SpellScriptLoader("spell_pri_cascade_trigger") { }

        class spell_pri_cascade_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_cascade_trigger_SpellScript);

            void HandleOnHit()
            {
                if (GetOriginalCaster())
                {
                    if (Player* _player = GetOriginalCaster()->ToPlayer())
                    {
                        if (Unit* target = GetHitUnit())
                        {
                            // Trigger for SpellScript
                            if (_player->IsValidAttackTarget(target))
                                _player->CastSpell(target, PRIEST_CASCADE_DAMAGE_TRIGGER, true); // Only damage
                            else
                                _player->CastSpell(target, PRIEST_CASCADE_HOLY_TRIGGER, true); // Only heal
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_cascade_trigger_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_cascade_trigger_SpellScript;
        }
};

// Cascade (shadow) - 127632 and Cascade - 121135
class spell_pri_cascade_first : public SpellScriptLoader
{
    public:
        spell_pri_cascade_first() : SpellScriptLoader("spell_pri_cascade_first") { }

        class spell_pri_cascade_first_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_cascade_first_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        switch (GetSpellInfo()->Id)
                        {
                            case 127632:
                            {
                                // First missile
                                if (_player->IsValidAttackTarget(target))
                                    _player->CastSpell(target, PRIEST_CASCADE_SHADOW_DAMAGE, true, 0, NULL, _player->GetGUID());
                                else
                                {
                                    _player->CastSpell(target, PRIEST_CASCADE_SHADOW_MISSILE, true, 0, NULL, _player->GetGUID());
                                    _player->CastSpell(target, PRIEST_CASCADE_SHADOW_HEAL, true, 0, NULL, _player->GetGUID());
                                }

                                break;
                            }
                            case 121135:
                            {
                                // First missile
                                if (_player->IsValidAttackTarget(target))
                                    _player->CastSpell(target, PRIEST_CASCADE_HOLY_DAMAGE, true, 0, NULL, _player->GetGUID());
                                else
                                {
                                    _player->CastSpell(target, PRIEST_CASCADE_HOLY_MISSILE, true, 0, NULL, _player->GetGUID());
                                    _player->CastSpell(target, PRIEST_CASCADE_HOLY_HEAL, true, 0, NULL, _player->GetGUID());
                                }

                                break;
                            }
                        }

                        // Invisible aura : Each target cannot be hit more than once time [...]
                        _player->CastSpell(target, PRIEST_CASCADE_INVISIBLE_AURA, true);
                        // Invisible aura 2 : [...] or Cascade can bound three times
                        _player->CastSpell(_player, PRIEST_CASCADE_INVISIBLE_AURA_2, true); // First bound

                        if (Aura* boundNumber = _player->GetAura(PRIEST_CASCADE_INVISIBLE_AURA_2))
                            boundNumber->SetCharges(1);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_cascade_first_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_cascade_first_SpellScript;
        }
};

// Halo
class spell_pri_halo : public SpellScriptLoader
{
    public:
        spell_pri_halo() : SpellScriptLoader("spell_pri_halo") { }

        class spell_pri_halo_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_halo_SpellScript);

            void HandleHeal(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        int32 _heal = GetHitHeal();
                        float Distance = caster->GetDistance(target);
                        float pct = (0.5f * pow((1.01f),(-1 * pow(((Distance - 25.0f) / 2), 4))) + 0.1f + 0.015f*Distance);
                        _heal = int32(_heal * pct);
                        SetHitHeal(_heal);
                    }
                }
            }

            void HandleDamage(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        int32 _damage = GetHitDamage();
                        float Distance = caster->GetDistance(target);
                        float pct = (0.5f * pow((1.01f),(-1 * pow(((Distance - 25.0f) / 2), 4))) + 0.1f + 0.015f*Distance);
                        _damage = int32(_damage * pct);
                        SetHitDamage(_damage);
                    }
                }
            }

            void FilterTargets(WorldObject*& target)
            {
                Unit* unit = target->ToUnit();
                if(!unit)
                    target = NULL;
                if(!GetCaster()->IsFriendlyTo(unit))
                    target = NULL;
            }

            void FilterTargets1(WorldObject*& target)
            {
                Unit* unit = target->ToUnit();
                if(!unit)
                    target = NULL;
                if(GetCaster()->IsFriendlyTo(unit))
                    target = NULL;
            }

            void Register()
            {
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_pri_halo_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_TARGET_ANY);
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_pri_halo_SpellScript::FilterTargets1, EFFECT_1, TARGET_UNIT_TARGET_ANY);
                OnEffectHitTarget += SpellEffectFn(spell_pri_halo_SpellScript::HandleHeal, EFFECT_0, SPELL_EFFECT_HEAL);
                OnEffectHitTarget += SpellEffectFn(spell_pri_halo_SpellScript::HandleDamage, EFFECT_1, SPELL_EFFECT_SCHOOL_DAMAGE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_halo_SpellScript;
        }
};

// Inner Fire - 588 or Inner Will - 73413
class spell_pri_inner_fire_or_will : public SpellScriptLoader
{
    public:
        spell_pri_inner_fire_or_will() : SpellScriptLoader("spell_pri_inner_fire_or_will") { }

        class spell_pri_inner_fire_or_will_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_inner_fire_or_will_SpellScript);

            bool Validate(SpellInfo const* /*SpellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(PRIEST_INNER_FIRE) || !sSpellMgr->GetSpellInfo(PRIEST_INNER_WILL))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (GetSpellInfo()->Id == PRIEST_INNER_FIRE)
                    {
                        if (_player->HasAura(PRIEST_INNER_WILL))
                            _player->RemoveAura(PRIEST_INNER_WILL);
                    }
                    else if (GetSpellInfo()->Id == PRIEST_INNER_WILL)
                    {
                        if (_player->HasAura(PRIEST_INNER_FIRE))
                            _player->RemoveAura(PRIEST_INNER_FIRE);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_inner_fire_or_will_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_inner_fire_or_will_SpellScript;
        }
};

// Leap of Faith - 73325 and Leap of Faith - 110718 (Symbiosis)
class spell_pri_leap_of_faith : public SpellScriptLoader
{
    public:
        spell_pri_leap_of_faith() : SpellScriptLoader("spell_pri_leap_of_faith") { }

        class spell_pri_leap_of_faith_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_leap_of_faith_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if(_player->HasAura(119850))
                            target->RemoveAurasWithMechanic((1<<MECHANIC_SNARE)|(1<<MECHANIC_ROOT));
                        target->CastSpell(_player, PRIEST_LEAP_OF_FAITH_JUMP, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_leap_of_faith_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_leap_of_faith_SpellScript;
        }
};

// Void Shift - 108968, 142723
class spell_pri_void_shift : public SpellScriptLoader
{
    public:
        spell_pri_void_shift() : SpellScriptLoader("spell_pri_void_shift") { }

        class spell_pri_void_shift_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_void_shift_SpellScript);

            SpellCastResult CheckTarget()
            {
                if (GetExplTargetUnit())
                    if (GetExplTargetUnit()->GetTypeId() != TYPEID_PLAYER)
                        return SPELL_FAILED_BAD_TARGETS;

                return SPELL_CAST_OK;
            }

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetExplTargetUnit())
                    {
                        float casterPct;
                        float targetPct;
                        float basePct = GetSpellInfo()->Effects[EFFECT_0].BasePoints;
                        int32 casterHeal = 0, targetHeal = 0;
                        int32 casterDamage = 0, targetDamage = 0;

                        casterPct = caster->GetHealthPct();
                        targetPct = target->GetHealthPct();

                        if (casterPct < basePct)
                        {
                            if(target->HasAura(47788))
                                targetHeal = target->CountPctFromMaxHealth(int32(basePct * 1.6f));
                            else
                                targetHeal = target->CountPctFromMaxHealth(int32(basePct));
                        }
                        else
                            targetHeal = target->CountPctFromMaxHealth(int32(casterPct));

                        if (targetPct < basePct)
                        {
                            if(caster->HasAura(47788))
                                casterHeal = int32(caster->CountPctFromMaxHealth(basePct * 1.6f));
                            else
                                casterHeal = int32(caster->CountPctFromMaxHealth(basePct));
                        }
                        else
                            casterHeal = caster->CountPctFromMaxHealth(int32(targetPct));

                        if(target->GetHealth() > uint32(targetHeal))
                        {
                            targetDamage = target->GetHealth() - targetHeal;
                            targetHeal = 0;
                        }
                        else
                            targetHeal -= target->GetHealth();

                        if(caster->GetHealth() > uint32(casterHeal))
                        {
                            casterDamage = caster->GetHealth() - casterHeal;
                            casterHeal = 0;
                        }
                        else
                            casterHeal -= caster->GetHealth();

                        caster->CastCustomSpell(caster, 118594, &casterDamage, &casterHeal, NULL, true);
                        caster->CastCustomSpell(target, 118594, &targetDamage, &targetHeal, NULL, true);
                        caster->CastSpell(target, 134977, true);
                        if(caster->HasAura(147779)) //Glyph of Shifted Appearances
                        {
                            caster->CastSpell(target, 147898, true);
                            target->CastSpell(caster, 147898, true);
                        }
                    }
                }
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_pri_void_shift_SpellScript::CheckTarget);
                OnHit += SpellHitFn(spell_pri_void_shift_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_void_shift_SpellScript;
        }
};

// 129176 - Shadow Word: Death (Glyph)
class spell_pri_shadow_word_death : public SpellScriptLoader
{
public:
    spell_pri_shadow_word_death() : SpellScriptLoader("spell_pri_shadow_word_death") { }

    class spell_pri_shadow_word_death_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pri_shadow_word_death_SpellScript);

        bool resetCooldown;
        bool isSP;

        void HandleBeforeHit()
        {
            isSP = false;
            resetCooldown = false;
            if (Unit* target = GetHitUnit())
                if (target->GetHealthPct() < 20)
                    if (Unit* caster = GetCaster())
                        if (Player* plr = caster->ToPlayer())
                        {
                            if (plr->GetSpecializationId(plr->GetActiveSpec()) == SPEC_PRIEST_SHADOW)
                            {
                                caster->CastSpell(caster, 125927, true);
                                isSP = true;
                            }

                            resetCooldown = true;
                        }
        }

        void HandleAfterHit()
        {
            if(Unit* caster = GetCaster())
                if (Unit* target = GetHitUnit())
                    if (Player* plr = caster->ToPlayer())
                    {
                        bool hasCD = caster->HasAura(95652);

                        if (target->isAlive())
                        {
                            caster->AddAura(32409, caster);

                            if (!hasCD)
                            {
                                if (resetCooldown)
                                {
                                    caster->AddAura(95652, caster);
                                    plr->RemoveSpellCooldown(GetSpellInfo()->Id, true);
                                }
                            }
                        }
                        else
                        {
                            if (isSP && !hasCD)
                                caster->CastSpell(caster, 125927, true);
                        }
                    }
        }

        void Register()
        {
            AfterHit += SpellHitFn(spell_pri_shadow_word_death_SpellScript::HandleAfterHit);
            BeforeHit += SpellHitFn(spell_pri_shadow_word_death_SpellScript::HandleBeforeHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_pri_shadow_word_death_SpellScript();
    }
};

// 32379 - Shadow Word : Death
class spell_pri_shadow_orb : public SpellScriptLoader
{
    public:
        spell_pri_shadow_orb() : SpellScriptLoader("spell_pri_shadow_orb") { }

        class spell_pri_shadow_orb_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_shadow_orb_SpellScript);

            void HandleDamage()
            {
                if(Unit* caster = GetCaster())
                    if (Unit* target = GetHitUnit())
                        if (Player* plr = caster->ToPlayer())
                            if (plr->GetSpecializationId(plr->GetActiveSpec()) == SPEC_PRIEST_SHADOW)
                            {
                                if (!caster->HasAura(95652))
                                {
                                    caster->CastSpell(caster, 125927, true);

                                    if (target->isAlive())
                                    {
                                        caster->AddAura(95652, caster);
                                        plr->RemoveSpellCooldown(GetSpellInfo()->Id, true);
                                    }
                                }
                            }
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_pri_shadow_orb_SpellScript::HandleDamage);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_shadow_orb_SpellScript();
        }
};

// Psychic Horror - 64044
class spell_pri_psychic_horror : public SpellScriptLoader
{
    public:
        spell_pri_psychic_horror() : SpellScriptLoader("spell_pri_psychic_horror") { }

        class spell_pri_psychic_horror_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_psychic_horror_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        Player* _player = caster->ToPlayer();
                        if (_player && _player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_PRIEST_SHADOW)
                        {
                            int32 currentPower = caster->GetPower(POWER_SHADOW_ORB) + 1;
                            caster->ModifyPower(POWER_SHADOW_ORB, -currentPower, true);

                            // +1s per Shadow Orb consumed
                            if (Aura* psychicHorror = target->GetAura(64044))
                            {
                                int32 maxDuration = psychicHorror->GetMaxDuration();
                                int32 newDuration = maxDuration + currentPower * IN_MILLISECONDS;
                                psychicHorror->SetDuration(newDuration);

                                if (newDuration > maxDuration)
                                    psychicHorror->SetMaxDuration(newDuration);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_psychic_horror_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_psychic_horror_SpellScript();
        }
};

// Guardian Spirit - 47788
class spell_pri_guardian_spirit : public SpellScriptLoader
{
    public:
        spell_pri_guardian_spirit() : SpellScriptLoader("spell_pri_guardian_spirit") { }

        class spell_pri_guardian_spirit_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_guardian_spirit_AuraScript);

            uint32 healPct;

            bool Validate(SpellInfo const* /*SpellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(PRIEST_SPELL_GUARDIAN_SPIRIT_HEAL))
                    return false;
                return true;
            }

            bool Load()
            {
                healPct = GetSpellInfo()->Effects[EFFECT_1].CalcValue();
                return true;
            }

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                // Set absorbtion amount to unlimited
                amount = -1;
            }

            void Absorb(AuraEffect* /*aurEff*/, DamageInfo & dmgInfo, uint32 & absorbAmount)
            {
                Unit* target = GetTarget();
                if (dmgInfo.GetDamage() < target->GetHealth())
                    return;

                int32 healAmount = int32(target->CountPctFromMaxHealth(healPct));
                // remove the aura now, we don't want 40% healing bonus
                Remove(AURA_REMOVE_BY_ENEMY_SPELL);
                target->CastCustomSpell(target, PRIEST_SPELL_GUARDIAN_SPIRIT_HEAL, &healAmount, NULL, NULL, true);
                absorbAmount = dmgInfo.GetDamage();
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pri_guardian_spirit_AuraScript::CalculateAmount, EFFECT_2, SPELL_AURA_SCHOOL_ABSORB);
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_pri_guardian_spirit_AuraScript::Absorb, EFFECT_2, SPELL_AURA_SCHOOL_ABSORB);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_guardian_spirit_AuraScript();
        }
};

// Penance - 47540
class spell_pri_penance : public SpellScriptLoader
{
    public:
        spell_pri_penance() : SpellScriptLoader("spell_pri_penance") { }

        class spell_pri_penance_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_penance_SpellScript);

            bool Load()
            {
                return GetCaster()->GetTypeId() == TYPEID_PLAYER;
            }

            bool Validate(SpellInfo const* SpellInfo)
            {
                if (!sSpellMgr->GetSpellInfo(PRIEST_SPELL_PENANCE))
                    return false;
                // can't use other spell than this penance due to spell_ranks dependency
                if (sSpellMgr->GetFirstSpellInChain(PRIEST_SPELL_PENANCE) != sSpellMgr->GetFirstSpellInChain(SpellInfo->Id))
                    return false;

                uint8 rank = sSpellMgr->GetSpellRank(SpellInfo->Id);
                if (!sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_DAMAGE, rank, true))
                    return false;
                if (!sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_HEAL, rank, true))
                    return false;

                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* unitTarget = GetHitUnit())
                    {
                        if (!unitTarget->isAlive())
                            return;

                        uint8 rank = sSpellMgr->GetSpellRank(GetSpellInfo()->Id);

                        if (_player->IsFriendlyTo(unitTarget))
                            _player->CastSpell(unitTarget, sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_HEAL, rank), false, 0);
                        else
                            _player->CastSpell(unitTarget, sSpellMgr->GetSpellWithRank(PRIEST_SPELL_PENANCE_DAMAGE, rank), false, 0);

                        // Divine Insight (Discipline)
                        if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_PRIEST_DISCIPLINE)
                            if (_player->HasAura(PRIEST_SPELL_DIVINE_INSIGHT_TALENT))
                                _player->CastSpell(_player, PRIEST_SPELL_DIVINE_INSIGHT_DISCIPLINE, true);
                    }
                }
            }

            SpellCastResult CheckCast()
            {
                Player* caster = GetCaster()->ToPlayer();
                if (Unit* target = GetExplTargetUnit())
                    if (!caster->IsFriendlyTo(target) && !caster->IsValidAttackTarget(target))
                        return SPELL_FAILED_BAD_TARGETS;
                return SPELL_CAST_OK;
            }

            void Register()
            {
                // add dummy effect spell handler to Penance
                OnEffectHitTarget += SpellEffectFn(spell_pri_penance_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
                OnCheckCast += SpellCheckCastFn(spell_pri_penance_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_penance_SpellScript;
        }
};

enum PrayerOfMending
{
    SPELL_T9_HEALING_2_PIECE = 67201,
};

// Prayer of Mending Heal
class spell_pri_prayer_of_mending_heal : public SpellScriptLoader
{
public:
    spell_pri_prayer_of_mending_heal() : SpellScriptLoader("spell_pri_prayer_of_mending_heal") { }

    class spell_pri_prayer_of_mending_heal_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_pri_prayer_of_mending_heal_SpellScript);

        void HandleHeal(SpellEffIndex /*effIndex*/)
        {
            if (Unit* caster = GetOriginalCaster())
            {
                if (AuraEffect* aurEff = caster->GetAuraEffect(SPELL_T9_HEALING_2_PIECE, EFFECT_0))
                {
                    int32 heal = GetHitHeal();
                    AddPct(heal, aurEff->GetAmount());
                    SetHitHeal(heal);
                }
            }

        }

        void Register()
        {
            OnEffectHitTarget += SpellEffectFn(spell_pri_prayer_of_mending_heal_SpellScript::HandleHeal, EFFECT_0, SPELL_EFFECT_HEAL);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_pri_prayer_of_mending_heal_SpellScript();
    }
};

// Vampiric Touch - 34914
class spell_pri_vampiric_touch : public SpellScriptLoader
{
    public:
        spell_pri_vampiric_touch() : SpellScriptLoader("spell_pri_vampiric_touch") { }

        class spell_pri_vampiric_touch_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_vampiric_touch_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (GetCaster())
                    GetCaster()->EnergizeBySpell(GetCaster(), GetSpellInfo()->Id, GetCaster()->CountPctFromMaxMana(2), POWER_MANA);
            }

            void HandleDispel(DispelInfo* dispelInfo)
            {
                if (Unit* caster = GetCaster())
                    if (Unit* dispeller = dispelInfo->GetDispeller())
                        if (caster->HasAura(PRIEST_SPELL_4P_S12_SHADOW))
                            dispeller->CastSpell(dispeller, PRIEST_SPELL_SIN_AND_PUNISHMENT, true, 0, NULL, caster->GetGUID());
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_pri_vampiric_touch_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DAMAGE);
                AfterDispel += AuraDispelFn(spell_pri_vampiric_touch_AuraScript::HandleDispel);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_vampiric_touch_AuraScript();
        }
};

// Called by Renew - 139
// Rapid Renew - 95649
class spell_priest_renew : public SpellScriptLoader
{
    public:
        spell_priest_renew() : SpellScriptLoader("spell_priest_renew") { }

        class spell_priest_renew_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_priest_renew_AuraScript);

            void HandleApplyEffect(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    // Empowered Renew
                    if (Aura* empoweredRenew = caster->GetAura(PRIEST_RAPID_RENEWAL_AURA))
                    {
                        uint32 heal = aurEff->GetAmount();
                        int32 basepoints0 = empoweredRenew->GetEffect(EFFECT_2)->GetAmount() * aurEff->GetTotalTicks() * int32(heal) / 100;
                        caster->CastCustomSpell(GetTarget(), PRIEST_SPELL_EMPOWERED_RENEW, &basepoints0, NULL, NULL, true, NULL, aurEff);
                    }
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_priest_renew_AuraScript::HandleApplyEffect, EFFECT_0, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_priest_renew_AuraScript();
        }
};

// Called by Shadow Form - 15473
// Glyph of Shadow - 107906
class spell_pri_shadowform : public SpellScriptLoader
{
    public:
        spell_pri_shadowform() : SpellScriptLoader("spell_pri_shadowform") { }

        class spell_pri_shadowform_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_shadowform_AuraScript);

            bool Validate(SpellInfo const* /*entry*/)
            {
                if (!sSpellMgr->GetSpellInfo(PRIEST_SHADOWFORM_VISUAL_WITHOUT_GLYPH) ||
                    !sSpellMgr->GetSpellInfo(PRIEST_SHADOWFORM_VISUAL_WITH_GLYPH))
                    return false;
                return true;
            }

            void HandleEffectApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                GetTarget()->CastSpell(GetTarget(), GetTarget()->HasAura(PRIEST_GLYPH_OF_SHADOW) ? PRIEST_SHADOWFORM_VISUAL_WITH_GLYPH : PRIEST_SHADOWFORM_VISUAL_WITHOUT_GLYPH, true);
            }

            void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                GetTarget()->RemoveAurasDueToSpell(GetTarget()->HasAura(PRIEST_GLYPH_OF_SHADOW) ? PRIEST_SHADOWFORM_VISUAL_WITH_GLYPH : PRIEST_SHADOWFORM_VISUAL_WITHOUT_GLYPH);
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_pri_shadowform_AuraScript::HandleEffectApply, EFFECT_0, SPELL_AURA_MOD_SHAPESHIFT, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
                AfterEffectRemove += AuraEffectRemoveFn(spell_pri_shadowform_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_MOD_SHAPESHIFT, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_shadowform_AuraScript();
        }
};

enum PsyfiendSpells
{
    SPELL_PSYCHIC_HORROR    = 113792,
};

// Void Tendrils - 108920
class spell_pri_void_tendrils : public SpellScriptLoader
{
    public:
        spell_pri_void_tendrils() : SpellScriptLoader("spell_pri_void_tendrils") { }

        class spell_pri_void_tendrils_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_void_tendrils_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                        caster->CastSpell(target, 127665, true);
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_void_tendrils_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_void_tendrils_SpellScript();
        }
};

class spell_pri_psychic_terror : public SpellScriptLoader
{
    public:
        spell_pri_psychic_terror() : SpellScriptLoader("spell_pri_psychic_terror") { }

        class spell_pri_psychic_terror_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_psychic_terror_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                std::list<WorldObject*> unitList;
                if(Unit* caster = GetCaster())
                {
                    if (Unit* owner = caster->GetOwner())
                    {
                        Unit::AttackerSet attackers = owner->getAttackers();
                        for (Unit::AttackerSet::iterator itr = attackers.begin(); itr != attackers.end();)
                        {
                            if (Unit* m_target = (*itr))
                            {
                                if (m_target->GetDistance2d(caster) <= 20.0f && !m_target->HasAura(SPELL_PSYCHIC_HORROR) && !m_target->HasAura(122300) && !m_target->HasInvisibilityAura() && !m_target->HasStealthAura())
                                {
                                    targets.clear();
                                    targets.push_back(m_target);
                                    caster->AddAura(122300, m_target);
                                    return;
                                }
                            }
                            ++itr;
                        }
                    }

                    for (std::list<WorldObject*>::iterator itr = targets.begin() ; itr != targets.end(); ++itr)
                    {
                        if(Unit* targer = (*itr)->ToUnit())
                        if (targer->IsWithinDist(caster, 20) && !targer->HasAura(SPELL_PSYCHIC_HORROR) && !targer->HasAura(119032) && !targer->HasAura(122300) && !targer->HasInvisibilityAura() && !targer->HasStealthAura())
                            unitList.push_back((*itr));
                    }
                
                    targets.clear();
                    targets = unitList;

                    Trinity::Containers::RandomResizeList(targets, 1);
                    for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                    {
                        if (Unit* target = (*itr)->ToUnit())
                            caster->AddAura(122300, target);
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_psychic_terror_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_psychic_terror_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_SRC_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_psychic_terror_SpellScript::FilterTargets, EFFECT_2, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_psychic_terror_SpellScript();
        }
};

// Divine Star 110744 122121
class spell_pri_divine_star : public SpellScriptLoader
{
    public:
        spell_pri_divine_star() : SpellScriptLoader("spell_pri_divine_star") { }

        class spell_pri_divine_star_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_divine_star_AuraScript)

            Position pos, _ownPos;
            bool check;

            bool Load()
            {
                pos.Relocate(0, 0, 0, 0);
                _ownPos.Relocate(0, 0, 0, 0);
                check = false;
                return true;
            }

            //--
            void OnPereodic(AuraEffect const* aurEff)
            {
                PreventDefaultAction();
                Unit* caster = GetCaster();
                if (!caster || !check)
                    return;

                uint32 tick = aurEff->GetTickNumber() - 1;

                if(tick == 5)
                {
                    caster->SendMissileCancel(GetSpellInfo()->Effects[1].TriggerSpell);
                    GetAura()->ClearEffectTarget();
                }

                //uint32 _countTick = uint32(1000.0f / 250);
                //float _RealDistanceintick = (24.0f / _countTick) * tick;
                float distanceintick = 6.0f * tick;
                if(distanceintick > 24.0f)
                    distanceintick = (24.0f * 2) - distanceintick;

                if(distanceintick < 0.0f)
                    return;

                // expload at tick
                float x = _ownPos.GetPositionX() + (caster->GetObjectSize() + distanceintick) * std::cos(_ownPos.GetOrientation());
                float y = _ownPos.GetPositionY() + (caster->GetObjectSize() + distanceintick) * std::sin(_ownPos.GetOrientation());
                Trinity::NormalizeMapCoord(x);
                Trinity::NormalizeMapCoord(y);

                caster->CastSpell(x, y, _ownPos.GetPositionZ(), GetSpellInfo()->Effects[0].TriggerSpell, true, NULL, aurEff);
            }

            void HandleApplyEffect(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                check = true;
                float x, y;
                caster->GetNearPoint2D(x, y, 24.0f, caster->GetOrientation());
                pos.Relocate(x, y, caster->GetPositionZ(), caster->GetOrientation());

                GetAura()->SetDuration(uint32(1000.0f * 2));
                _ownPos.Relocate(caster->GetPositionX(), caster->GetPositionY(), caster->GetPositionZ(), caster->GetOrientation());
            }

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if(Unit* caster = GetCaster())
                    caster->SendMissileCancel(GetSpellInfo()->Effects[1].TriggerSpell, false);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_pri_divine_star_AuraScript::OnPereodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
                OnEffectApply += AuraEffectApplyFn(spell_pri_divine_star_AuraScript::HandleApplyEffect, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_pri_divine_star_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_divine_star_AuraScript();
        }
};

// Divine Star 110745 122128
class spell_pri_divine_star_filter : public SpellScriptLoader
{
    public:
        spell_pri_divine_star_filter() : SpellScriptLoader("spell_pri_divine_star_filter") { }

        class spell_pri_divine_star_filter_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_divine_star_filter_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                Unit* caster = GetCaster();
                if (!caster)
                    return;

                AuraEffect const* aurEff = GetSpell()->GetTriggeredAuraEff();
                if (!aurEff)
                {
                    targets.clear();
                    return;
                }

                uint32 tick = aurEff->GetTickNumber();
                Aura* auraTrigger = aurEff->GetBase();
                Position const* pos = auraTrigger->GetDstPos();

                float distanceintick = 6.0f * tick;
                if(distanceintick > 24.0f)
                    distanceintick = (24.0f * 2) - distanceintick;

                if(distanceintick < 0.0f)
                {
                    targets.clear();
                    return;
                }

                float angle = caster->GetAngle(pos);

                // expload at tick
                float x = caster->GetPositionX() + (caster->GetObjectSize() + distanceintick) * std::cos(angle);
                float y = caster->GetPositionY() + (caster->GetObjectSize() + distanceintick) * std::sin(angle);
                Trinity::NormalizeMapCoord(x);
                Trinity::NormalizeMapCoord(y);

                std::list<uint64> saveTargets = auraTrigger->GetEffectTargets();

                for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end();)
                {
                    uint64 guid = (*itr)->GetGUID();
                    bool find = false;
                    if(!saveTargets.empty())
                    {
                        for (std::list<uint64>::iterator itrGuid = saveTargets.begin(); itrGuid != saveTargets.end();)
                        {
                            if(guid == (*itrGuid))
                            {
                                find = true;
                                break;
                            }
                            ++itrGuid;
                        }
                    }
                    if(find || ((*itr)->GetDistance2d(x, y) > 4.0f))
                        targets.erase(itr++);
                    else
                    {
                        auraTrigger->AddEffectTarget(guid);
                        ++itr;
                    }
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_divine_star_filter_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_divine_star_filter_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_divine_star_filter_SpellScript();
        }
};

// Power Word: Solace - 140815
class spell_pri_power_word_solace_heal : public SpellScriptLoader
{
    public:
        spell_pri_power_word_solace_heal() : SpellScriptLoader("spell_pri_power_word_solace_heal") { }

        class spell_pri_power_word_solace_heal_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_power_word_solace_heal_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                {
                    targets.push_back(GetCaster());
                    return;
                }

                Unit* targerSave = NULL;
                for (std::list<WorldObject*>::iterator itr = targets.begin() ; itr != targets.end(); ++itr)
                {
                    if(Unit* target = (*itr)->ToUnit())
                        if(target->GetTypeId() == TYPEID_PLAYER)
                            if(!targerSave || target->GetHealth() < targerSave->GetHealth())
                                targerSave = target;
                }

                targets.clear();
                if(targerSave)
                    targets.push_back(targerSave);
                else
                    targets.push_back(GetCaster());
            }

            void FilterSelf(std::list<WorldObject*>& targets)
            {
                targets.clear();
            }

            void HandleDamageCalc(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();
                if(caster == target)
                    SetHitHeal(GetHitHeal() / 2);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_pri_power_word_solace_heal_SpellScript::HandleDamageCalc, EFFECT_0, SPELL_EFFECT_HEAL);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_power_word_solace_heal_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ALLY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_power_word_solace_heal_SpellScript::FilterSelf, EFFECT_0, TARGET_DEST_DEST);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_power_word_solace_heal_SpellScript();
        }
};

// Mind Flay - 15407 for Glyph of Mind Flay 120585
class spell_pri_mind_flay : public SpellScriptLoader
{
    public:
        spell_pri_mind_flay() : SpellScriptLoader("spell_pri_mind_flay") { }

        class spell_pri_mind_flay_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_mind_flay_AuraScript);

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                if (Unit* caster = GetCaster())
                    if (caster->HasAura(120585))
                        amount = 0;
            }

            void OnTick(AuraEffect const* /*aurEff*/)
            {
                if (Unit* m_caster = GetCaster())
                    if (m_caster->HasAura(120585))
                        m_caster->CastSpell(m_caster, 120587, true);
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pri_mind_flay_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_MOD_DECREASE_SPEED);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_pri_mind_flay_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_mind_flay_AuraScript();
        }
};

class spell_pri_holy_nova_damage : public SpellScriptLoader
{
    public:
        spell_pri_holy_nova_damage() : SpellScriptLoader("spell_pri_holy_nova_damage") { }

        class spell_pri_holy_nova_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_holy_nova_damage_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                Trinity::Containers::RandomResizeList(targets, 5);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_holy_nova_damage_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_holy_nova_damage_SpellScript();
        }
};

class spell_pri_holy_nova_heal : public SpellScriptLoader
{
    public:
        spell_pri_holy_nova_heal() : SpellScriptLoader("spell_pri_holy_nova_heal") { }

        class spell_pri_holy_nova_heal_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_holy_nova_heal_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                Trinity::Containers::RandomResizeList(targets, 5);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_holy_nova_heal_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_holy_nova_heal_SpellScript();
        }
};

class spell_pri_binding_heal : public SpellScriptLoader
{
    public:
        spell_pri_binding_heal() : SpellScriptLoader("spell_pri_binding_heal") { }

        class spell_pri_binding_heal_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_binding_heal_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                targets.remove(GetCaster());
                if(Unit* caster = GetCaster())
                {
                    if (Unit* target = GetExplTargetUnit())
                    {
                        if (caster->HasAura(63248))
                        {
                            targets.remove(target);
                            Trinity::Containers::RandomResizeList(targets, 1);
                        }
                        else
                            targets.clear();

                        targets.push_back(target);
                    }
                    else
                        Trinity::Containers::RandomResizeList(targets, 1);
                }
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_binding_heal_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_binding_heal_SpellScript();
        }
};

// Dispel Magic - 528 for Glyph of Dispel Magic
class spell_pri_dispel_magic : public SpellScriptLoader
{
    public:
        spell_pri_dispel_magic() : SpellScriptLoader("spell_pri_dispel_magic") { }

        class spell_pri_dispel_magic_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_dispel_magic_SpellScript);

            void HandleDispel()
            {
                Unit* caster = GetCaster();
                Unit* target = GetHitUnit();
                if(!caster || !target)
                    return;

                if (caster->HasAura(119864) && GetSpell()->GetCountDispel())
                    caster->CastSpell(target, 119856, true);
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_pri_dispel_magic_SpellScript::HandleDispel);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_dispel_magic_SpellScript();
        }
};

class spell_pri_t15_healer_4p : public SpellScriptLoader
{
    public:
        spell_pri_t15_healer_4p() : SpellScriptLoader("spell_pri_t15_healer_4p") { }

        class spell_pri_t15_healer_4p_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_t15_healer_4p_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                std::list<WorldObject*> unitList;
                if(Unit* caster = GetCaster())
                {
                    for (std::list<WorldObject*>::iterator itr = targets.begin() ; itr != targets.end(); ++itr)
                    {
                        if(Unit* targer = (*itr)->ToUnit())
                        if (targer != caster)
                            unitList.push_back((*itr));
                    }
                }
                targets.clear();
                targets = unitList;
            }
            
            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_t15_healer_4p_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ALLY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_t15_healer_4p_SpellScript();
        }
};

// Lightwell Triger - 126137
class spell_pri_lightwell_trigger : public SpellScriptLoader
{
    public:
        spell_pri_lightwell_trigger() : SpellScriptLoader("spell_pri_lightwell_trigger") { }

        class spell_pri_lightwell_trigger_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_lightwell_trigger_SpellScript);

            void HandleOnHit()
            {
                if (Unit* m_caster = GetCaster())
                {
                    if (Unit* unitTarget = GetHitUnit())
                    {
                        if (m_caster->GetTypeId() != TYPEID_UNIT || !m_caster->ToCreature()->isSummon())
                            return;

                        // proc a spellcast
                        if (Aura* chargesAura = m_caster->GetAura(LIGHTWELL_CHARGES_MOP))
                        {
                            m_caster->CastSpell(unitTarget, LIGHTSPRING_RENEW_MOP, true, NULL, NULL, m_caster->ToTempSummon()->GetSummonerGUID());
                            m_caster->CastSpell(unitTarget, LIGHTSPRING_RENEW_VISUAL, true, NULL, NULL, m_caster->ToTempSummon()->GetSummonerGUID());
                            if (chargesAura->ModStackAmount(-1))
                                m_caster->ToTempSummon()->UnSummon();
                        }
                    }
                }
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                std::list<WorldObject*> unitList;
                for (std::list<WorldObject*>::iterator itr = targets.begin() ; itr != targets.end(); ++itr)
                {
                    if (Unit* targer = (*itr)->ToUnit())
                        if (Unit* caster = GetCaster())
                            if (targer->GetHealthPct() <= 50.0f && !targer->HasAura(LIGHTSPRING_RENEW_MOP) && targer != caster)
                                unitList.push_back((*itr));
                }
                targets.clear();
                targets = unitList;
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_pri_lightwell_trigger_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
                OnHit += SpellHitFn(spell_pri_lightwell_trigger_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_lightwell_trigger_SpellScript();
        }
};

// Hymn of Hope - 64904
class spell_pri_hymn_of_hope : public SpellScriptLoader
{
    public:
        spell_pri_hymn_of_hope() : SpellScriptLoader("spell_pri_hymn_of_hope") { }

        class spell_pri_hymn_of_hope_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_hymn_of_hope_AuraScript);

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                if (Unit* target = GetUnitOwner())
                    amount = target->CountPctFromMaxMana(amount);
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_pri_hymn_of_hope_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_MOD_INCREASE_ENERGY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_hymn_of_hope_AuraScript();
        }
};

// Shadowy Apparition - 148859
class spell_pri_shadowy_apparition : public SpellScriptLoader
{
    public:
        spell_pri_shadowy_apparition() : SpellScriptLoader("spell_pri_shadowy_apparition") { }

        class spell_pri_shadowy_apparition_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_shadowy_apparition_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (caster->HasAura(138156) && roll_chance_i(65))
                        {
                            if (Aura* aura = target->GetAura(34914, caster->GetGUID()))
                                if (AuraEffect* eff = aura->GetEffect(EFFECT_1))
                                    aura->SetDuration(aura->GetDuration() + eff->GetAmplitude());

                            if (Aura* aura = target->GetAura(589, caster->GetGUID()))
                                if (AuraEffect* eff = aura->GetEffect(EFFECT_0))
                                    aura->SetDuration(aura->GetDuration() + eff->GetAmplitude());
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_shadowy_apparition_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_shadowy_apparition_SpellScript;
        }
};

// 114404 - Void Tendril's Grasp
class spell_pri_void_tendrils_grasp : public SpellScriptLoader
{
    public:
        spell_pri_void_tendrils_grasp() : SpellScriptLoader("spell_pri_void_tendrils_grasp") { }

        class spell_pri_void_tendrils_grasp_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_pri_void_tendrils_grasp_AuraScript);

            void CalculateMaxDuration(int32 & duration)
            {
                if (WorldObject* owner = GetOwner())
                    if (owner->GetTypeId() == TYPEID_PLAYER)
                        duration = 8000;
            }

            void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (Creature* me = caster->ToCreature())
                        me->DespawnOrUnsummon(500);
            }

            void Register()
            {
                DoCalcMaxDuration += AuraCalcMaxDurationFn(spell_pri_void_tendrils_grasp_AuraScript::CalculateMaxDuration);
                AfterEffectRemove += AuraEffectRemoveFn(spell_pri_void_tendrils_grasp_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_MOD_ROOT, AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_pri_void_tendrils_grasp_AuraScript();
        }
};

// Devouring Plague - 124467
class spell_pri_devouring_plague_mastery : public SpellScriptLoader
{
    public:
        spell_pri_devouring_plague_mastery() : SpellScriptLoader("spell_pri_devouring_plague_mastery") { }

        class spell_pri_devouring_plague_mastery_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_pri_devouring_plague_mastery_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if(Aura* aura = target->GetAura(2944))
                        {
                            int32 count = aura->GetCustomData();
                            caster->CastCustomSpell(caster, PRIEST_DEVOURING_PLAGUE_HEAL, &count, 0, 0, true);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_pri_devouring_plague_mastery_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_pri_devouring_plague_mastery_SpellScript;
        }
};

void AddSC_priest_spell_scripts()
{
    new spell_pri_glyph_of_mass_dispel();
    new spell_pri_item_s12_4p_heal();
    new spell_pri_item_s12_2p_shadow();
    new spell_pri_power_word_solace();
    new spell_pri_shadow_word_insanity_allowing();
    new spell_pri_surge_of_light();
    new spell_pri_body_and_soul();
    new spell_pri_prayer_of_mending_divine_insight();
    new spell_pri_divine_insight_holy();
    new spell_pri_divine_insight_discipline();
    new spell_pri_holy_word_sanctuary();
    new spell_pri_chakra_chastise();
    new spell_pri_lightwell_renew();
    new spell_pri_strength_of_soul();
    new spell_pri_grace();
    new spell_pri_rapture();
    new spell_pri_atonement();
    new spell_pri_spirit_shell();
    new spell_pri_devouring_plague();
    new spell_pri_mind_spike();
    new spell_pri_cascade_second();
    new spell_pri_cascade_trigger();
    new spell_pri_cascade_first();
    new spell_pri_halo();
    new spell_pri_shadowy_apparition();
    new spell_pri_inner_fire_or_will();
    new spell_pri_leap_of_faith();
    new spell_pri_void_shift();
    new spell_pri_shadow_word_death();
    new spell_pri_shadow_orb();
    new spell_pri_psychic_horror();
    new spell_pri_guardian_spirit();
    new spell_pri_penance();
    new spell_pri_prayer_of_mending_heal();
    new spell_pri_vampiric_touch();
    new spell_priest_renew();
    new spell_pri_shadowform();
    new spell_pri_archangel();
    new spell_pri_void_tendrils();
    new spell_pri_psychic_terror();
    new spell_pri_divine_star();
    new spell_pri_power_word_solace_heal();
    new spell_pri_mind_flay();
    new spell_pri_holy_nova_damage();
    new spell_pri_holy_nova_heal();
    new spell_pri_binding_heal();
    new spell_pri_dispel_magic();
    new spell_pri_t15_healer_4p();
    new spell_pri_lightwell_trigger();
    new spell_pri_hymn_of_hope();
    new spell_pri_void_tendrils_grasp();
    new spell_pri_divine_star_filter();
    new spell_pri_devouring_plague_mastery();
    new spell_pri_vampiric_embrace();
}
