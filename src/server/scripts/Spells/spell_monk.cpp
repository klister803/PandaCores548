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
 * Scripts for spells with SPELLFAMILY_MONK and SPELLFAMILY_GENERIC spells used by monk players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_monk_".
 */

#include "ScriptMgr.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"
#include "GridNotifiers.h"

enum MonkSpells
{
    SPELL_MONK_LEGACY_OF_THE_EMPEROR            = 117667,
    SPELL_MONK_FORTIFYING_BREW                  = 120954,
    SPELL_MONK_PROVOKE                          = 118635,
    SPELL_MONK_BLACKOUT_KICK_DOT                = 128531,
    SPELL_MONK_BLACKOUT_KICK_HEAL               = 128591,
    SPELL_MONK_SHUFFLE                          = 115307,
    SPELL_MONK_ZEN_PILGRIMAGE                   = 126892,
    SPELL_MONK_ZEN_PILGRIMAGE_RETURN            = 126895,
    SPELL_MONK_DISABLE_ROOT                     = 116706,
    SPELL_MONK_DISABLE                          = 116095,
    SPELL_MONK_SOOTHING_MIST_VISUAL             = 125955,
    SPELL_MONK_SOOTHING_MIST_ENERGIZE           = 116335,
    SPELL_MONK_BREATH_OF_FIRE_DOT               = 123725,
    SPELL_MONK_BREATH_OF_FIRE_CONFUSED          = 123393,
    SPELL_MONK_ELUSIVE_BREW_STACKS              = 128939,
    SPELL_MONK_ELUSIVE_BREW                     = 115308,
    SPELL_MONK_KEG_SMASH_VISUAL                 = 123662,
    SPELL_MONK_KEG_SMASH_ENERGIZE               = 127796,
    SPELL_MONK_WEAKENED_BLOWS                   = 115798,
    SPELL_MONK_DIZZYING_HAZE                    = 116330,
    SPELL_MONK_CLASH_CHARGE                     = 126452,
    SPELL_MONK_LIGHT_STAGGER                    = 124275,
    SPELL_MONK_MODERATE_STAGGER                 = 124274,
    SPELL_MONK_HEAVY_STAGGER                    = 124273,
    SPELL_MONK_ROLL                             = 109132,
    SPELL_MONK_ROLL_TRIGGER                     = 107427,
    SPELL_MONK_CHI_TORPEDO_HEAL                 = 124040,
    SPELL_MONK_CHI_TORPEDO_DAMAGE               = 117993,
    SPELL_MONK_FLYING_SERPENT_KICK              = 101545,
    SPELL_MONK_FLYING_SERPENT_KICK_NEW          = 115057,
    SPELL_MONK_FLYING_SERPENT_KICK_AOE          = 123586,
    SPELL_MONK_TIGEREYE_BREW                    = 116740,
    SPELL_MONK_TIGEREYE_BREW_STACKS             = 125195,
    SPELL_MONK_SPEAR_HAND_STRIKE_SILENCE        = 116709,
    SPELL_MONK_CHI_BURST_DAMAGE                 = 130651,
    SPELL_MONK_CHI_BURST_HEAL                   = 130654,
    SPELL_MONK_ZEN_SPHERE_DAMAGE                = 124098,
    SPELL_MONK_ZEN_SPHERE_HEAL                  = 124081,
    SPELL_MONK_ZEN_SPHERE_DETONATE_HEAL         = 124101,
    SPELL_MONK_ZEN_SPHERE_DETONATE_DAMAGE       = 125033,
    SPELL_MONK_HEALING_ELIXIRS_AURA             = 122280,
    SPELL_MONK_HEALING_ELIXIRS_RESTORE_HEALTH   = 122281,
    SPELL_MONK_RENEWING_MIST_HOT                = 119611,
    SPELL_MONK_RENEWING_MIST_JUMP_AURA          = 119607,
    SPELL_MONK_GLYPH_OF_RENEWING_MIST           = 123334,
    SPELL_MONK_SURGING_MIST_HEAL                = 116995,
    SPELL_MONK_ENVELOPING_MIST_HEAL             = 132120,
    SPELL_MONK_PLUS_ONE_MANA_TEA                = 123760,
    SPELL_MONK_MANA_TEA_STACKS                  = 115867,
    SPELL_MONK_MANA_TEA_REGEN                   = 115294,
    SPELL_MONK_SPINNING_CRANE_KICK_HEAL         = 117640,
    MONK_NPC_JADE_SERPENT_STATUE                = 60849,
    SPELL_MONK_UPLIFT_ALLOWING_CAST             = 123757,
    SPELL_MONK_THUNDER_FOCUS_TEA                = 116680,
    SPELL_MONK_PATH_OF_BLOSSOM_AREATRIGGER      = 122035,
    SPELL_MONK_SPINNING_FIRE_BLOSSOM_DAMAGE     = 123408,
    SPELL_MONK_SPINNING_FIRE_BLOSSOM_MISSILE    = 118852,
    SPELL_MONK_SPINNING_FIRE_BLOSSOM_ROOT       = 123407,
    SPELL_MONK_TOUCH_OF_KARMA_REDIRECT_DAMAGE   = 124280,
    SPELL_MONK_JADE_LIGHTNING_ENERGIZE          = 123333,
    SPELL_MONK_CRACKLING_JADE_SHOCK_BUMP        = 117962,
    SPELL_MONK_POWER_STRIKES_TALENT             = 121817,
    SPELL_MONK_CREATE_CHI_SPHERE                = 121286,
    SPELL_MONK_GLYPH_OF_ZEN_FLIGHT              = 125893,
    SPELL_MONK_ZEN_FLIGHT                       = 125883,
    SPELL_MONK_BEAR_HUG                         = 127361,
    ITEM_MONK_T14_TANK_4P                       = 123159,
    MONK_NPC_BLACK_OX_STATUE                    = 61146,
    SPELL_MONK_GUARD                            = 115295,
};

class spell_monk_storm_earth_and_fire : public SpellScriptLoader
{
    public:
        spell_monk_storm_earth_and_fire() : SpellScriptLoader("spell_monk_storm_earth_and_fire") { }

        class spell_monk_storm_earth_and_fire_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_storm_earth_and_fire_SpellScript);

            bool castspell;
            bool addMainAura;

            void CheckTarget(bool fullstack, Unit* caster, Unit* target)
            {
                std::list<Unit*> ControllUnit;
                std::list<Unit*> removeAllClones;
                if (!caster->m_Controlled.empty())
                {
                    for (Unit::ControlList::iterator itr = caster->m_Controlled.begin(); itr != caster->m_Controlled.end(); ++itr)
                        ControllUnit.push_back(*itr);

                    for (std::list<Unit*>::const_iterator i = ControllUnit.begin(); i != ControllUnit.end(); ++i)
                    {
                        if (Unit* cloneUnit = (*i))
                        {
                            if (cloneUnit->GetEntry() != 69792 && cloneUnit->GetEntry() != 69680 && cloneUnit->GetEntry() != 69791)
                                continue;

                            if (cloneUnit->getVictim() != target)
                            {
                                if (!fullstack)
                                    return;

                                removeAllClones.push_back(cloneUnit);
                                continue;
                            }
                            else
                            {
                                cloneUnit->ToCreature()->AI()->ComonOnHome();
                                castspell = false;
                                return;
                            }
                        }
                    }

                    for (std::list<Unit*>::const_iterator i = removeAllClones.begin(); i != removeAllClones.end(); ++i)
                        if (Creature* crt = (*i)->ToCreature())
                            if (CreatureAI* ai = crt->AI())
                                ai->ComonOnHome();
                }
                addMainAura = false;
            }

            SpellCastResult HandleCheckCast()
            {
                if (Unit* caster = GetCaster())
                {
                    Unit* target = GetExplTargetUnit();

                    if (!target)
                        return SPELL_FAILED_DONT_REPORT;

                    std::vector<uint32> id;
                    uint32 visualId = 0;
                    uint32 thirdId = 0;
                    bool addVisual = true;
                    addMainAura = true;
                    castspell = true;
                    id.push_back(138121);
                    id.push_back(138122);
                    id.push_back(138123);
                    std::random_shuffle(id.begin(), id.end());

                    if (Aura* aura = caster->GetAura(137639))
                        CheckTarget(aura->GetStackAmount() > 1, caster, target);

                    if (!castspell)
                        return SPELL_FAILED_DONT_REPORT;

                    if (!addMainAura)
                        return SPELL_CAST_OK;

                    for (std::vector<uint32>::iterator iter = id.begin(); iter != id.end(); ++iter)
                    {
                        bool getVisual = false;

                        switch (*iter)
                        {
                            case 138121: visualId = 138083; break;
                            case 138122: visualId = 138080; break;
                            case 138123: visualId = 138081; break;
                        }

                        if (caster->HasAura(visualId))
                        {
                            getVisual = true;
                            addVisual = false;
                        }

                        if (!caster->m_Controlled.empty())
                        {
                            std::list<Unit*> ControllUnit;
                            for (Unit::ControlList::iterator itr = caster->m_Controlled.begin(); itr != caster->m_Controlled.end(); ++itr)
                                ControllUnit.push_back(*itr);

                            for (std::list<Unit*>::const_iterator i = ControllUnit.begin(); i != ControllUnit.end(); ++i)
                            {
                                if (Unit* cloneUnit = (*i))
                                {
                                    if (getVisual)
                                        break;

                                    if (cloneUnit->GetEntry() != 69792 && cloneUnit->GetEntry() != 69680 && cloneUnit->GetEntry() != 69791)
                                        continue;

                                    SpellInfo const* _spellinfo = sSpellMgr->GetSpellInfo(*iter);

                                    if (cloneUnit->GetEntry() == _spellinfo->Effects[EFFECT_0].MiscValue)
                                    {
                                        getVisual = true;
                                        addVisual = false;
                                        break;
                                    }
                                }
                            }
                        }

                        if (getVisual)
                            continue;
                        else if (thirdId && !getVisual)
                            break;

                        thirdId = *iter;
                    }

                    if (addVisual)
                        caster->AddAura(visualId, caster);

                    if (thirdId)
                        caster->CastSpell(target, thirdId, true);
                }
                return SPELL_CAST_OK;
            }

            void HandleTargetSelect(WorldObject*& target)
            {
                if (!addMainAura)
                    target = NULL;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_monk_storm_earth_and_fire_SpellScript::HandleCheckCast);
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_monk_storm_earth_and_fire_SpellScript::HandleTargetSelect, EFFECT_1, TARGET_UNIT_CASTER);
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_monk_storm_earth_and_fire_SpellScript::HandleTargetSelect, EFFECT_2, TARGET_UNIT_CASTER);
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_monk_storm_earth_and_fire_SpellScript::HandleTargetSelect, EFFECT_3, TARGET_UNIT_CASTER);
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_monk_storm_earth_and_fire_SpellScript::HandleTargetSelect, EFFECT_4, TARGET_UNIT_CASTER);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_storm_earth_and_fire_SpellScript();
        }

        class spell_monk_storm_earth_and_fire_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_monk_storm_earth_and_fire_AuraScript);

            void RemoveEff(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    caster->RemoveAura(138083);
                    caster->RemoveAura(138080);
                    caster->RemoveAura(138081);

                    if (!caster->m_Controlled.empty())
                    {
                        std::list<Unit*> ControllUnit;
                        for (Unit::ControlList::iterator itr = caster->m_Controlled.begin(); itr != caster->m_Controlled.end(); ++itr)
                            ControllUnit.push_back(*itr);

                        for (std::list<Unit*>::const_iterator i = ControllUnit.begin(); i != ControllUnit.end(); ++i)
                        {
                            if (Unit* cloneUnit = (*i))
                            {
                                if (cloneUnit->GetEntry() != 69792 && cloneUnit->GetEntry() != 69680 && cloneUnit->GetEntry() != 69791)
                                    continue;

                                cloneUnit->ToCreature()->AI()->ComonOnHome();
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_monk_storm_earth_and_fire_AuraScript::RemoveEff, EFFECT_1, SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_monk_storm_earth_and_fire_AuraScript();
        }
};

class spell_monk_storm_earth_and_fire_clone_visual : public SpellScriptLoader
{
public:
    spell_monk_storm_earth_and_fire_clone_visual() : SpellScriptLoader("spell_monk_storm_earth_and_fire_clone_visual") { }

    class spell_monk_storm_earth_and_fire_clone_visualAuraScript : public AuraScript
    {
        PrepareAuraScript(spell_monk_storm_earth_and_fire_clone_visualAuraScript);

        void ApplyEff(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
        {
            if (Unit* caster = GetCaster())
                if (caster->GetTypeId() == TYPEID_PLAYER)
                    if (caster->m_Controlled.empty())
                        caster->RemoveAura(137639);
        }

        void Register()
        {
            AfterEffectApply += AuraEffectApplyFn(spell_monk_storm_earth_and_fire_clone_visualAuraScript::ApplyEff, EFFECT_2, SPELL_AURA_MOD_SPELL_VISUAL, AURA_EFFECT_HANDLE_REAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_monk_storm_earth_and_fire_clone_visualAuraScript();
    }
};

class spell_monk_clone_cast : public SpellScriptLoader
{
public:
    spell_monk_clone_cast() : SpellScriptLoader("spell_monk_clone_cast") { }

    class spell_monk_clone_cast_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_monk_clone_cast_SpellScript);

        void HandleOnCast()
        {
            if (Unit* caster = GetCaster())
                if (Player* plr = caster->ToPlayer())
                {
                    uint32 spellid = GetSpellInfo()->Id;

                    std::list<Unit*> ControllUnit;
                    for (Unit::ControlList::iterator itr = plr->m_Controlled.begin(); itr != plr->m_Controlled.end(); ++itr)
                        ControllUnit.push_back(*itr);

                    for (std::list<Unit*>::const_iterator i = ControllUnit.begin(); i != ControllUnit.end(); ++i)
                    {
                        if (Unit* cloneUnit = (*i))
                        {
                            if (cloneUnit->GetEntry() != 69792 && cloneUnit->GetEntry() != 69680 && cloneUnit->GetEntry() != 69791)
                                continue;

                            if (cloneUnit->HasUnitState(UNIT_STATE_CASTING))
                                continue;

                            if (spellid != 113656 && spellid != 101546 && spellid != 116847)
                                if (Unit* cloneTarget = cloneUnit->getVictim())
                                    if (Unit* _target = GetExplTargetUnit())
                                        if (_target == cloneTarget)
                                            continue;
                                            
                            if (Unit* cloneTarget = cloneUnit->getVictim())
                                cloneUnit->CastSpell(cloneTarget, spellid, true);
                        }
                    }
                }
        }

        void Register()
        {
            OnCast += SpellCastFn(spell_monk_clone_cast_SpellScript::HandleOnCast);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_monk_clone_cast_SpellScript();
    }
};

class spell_monk_fists_of_fury_stun : public SpellScriptLoader
{
public:
    spell_monk_fists_of_fury_stun() : SpellScriptLoader("spell_monk_fists_of_fury_stun") { }

    class spell_monk_fists_of_fury_stun_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_monk_fists_of_fury_stun_SpellScript);

        void HandleObjectAreaTargetSelect(std::list<WorldObject*>& targets)
        {
            std::list<Unit*> unitTargets;
            for (std::list<WorldObject*>::const_iterator itr = targets.begin(); itr != targets.end(); ++itr)
                if (Unit * target = (*itr)->ToUnit())
                    unitTargets.push_back(target);

            for (std::list<Unit*>::const_iterator itr = unitTargets.begin(); itr != unitTargets.end(); ++itr)
                if (((*itr))->HasAura(120086))
                    targets.remove(*itr);
        }

        void Register()
        {
            OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_monk_fists_of_fury_stun_SpellScript::HandleObjectAreaTargetSelect, EFFECT_0, TARGET_UNIT_CONE_ENEMY_24);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_monk_fists_of_fury_stun_SpellScript();
    }
};

// spell 119611 Renewing Mist
class spell_monk_renewing_mist : public SpellScriptLoader
{
public:
    spell_monk_renewing_mist() : SpellScriptLoader("spell_monk_renewing_mist") { }

    class spell_monk_renewing_mistAuraScript : public AuraScript
    {
        PrepareAuraScript(spell_monk_renewing_mistAuraScript);

        void HandleEffectPeriodicUpdate(AuraEffect* aurEff)
        {
            std::list<Unit*> targets;
            std::list<Unit*> unitTargets;
            Unit* owner = GetUnitOwner();
            Unit* caster = GetCaster();
            Unit* target = NULL;
            Aura* aura = GetAura();

            if (!owner || !caster)
                return;

            if (Aura* Uplift = caster->GetAura(123757))
                Uplift->RefreshTimers();
                

            if (aura && aura->GetCharges() < 2)
                return;

            Trinity::AnyFriendlyUnitInObjectRangeCheck u_check(owner, owner, 20);
            Trinity::UnitListSearcher<Trinity::AnyFriendlyUnitInObjectRangeCheck> searcher(owner, unitTargets, u_check);
            owner->VisitNearbyObject(20.0f, searcher);

            for (std::list<Unit*>::const_iterator iter = unitTargets.begin(); iter != unitTargets.end(); ++iter)
            {
                if ((*iter)->HasAura(GetId(), caster->GetGUID()) || !(*iter)->IsInPartyWith(caster) || (*iter)->GetTypeId() != TYPEID_PLAYER)
                    continue;

                targets.push_back(*iter);
            }

            targets.sort(Trinity::HealthPctOrderPred());

            for (std::list<Unit*>::const_iterator iter = targets.begin(); iter != targets.end(); ++iter)
            {
                if (!(*iter)->IsWithinLOSInMap(owner))
                    continue;

                target = (*iter);
                break;
            }

            if (!target)
                return;

            if (AuraEffect* aurEff1 = aurEff->GetBase()->GetEffect(EFFECT_1))
            {
                int32 setstack = aurEff1->GetAmount() - 1;
                int32 amount = aurEff->GetAmount();
                caster->CastCustomSpell(target, GetId(), &amount, &setstack, NULL, true, NULL, aurEff, caster->GetGUID());
                aura->SetCharges(1);
            }
        }

        void Register()
        {
            OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_monk_renewing_mistAuraScript::HandleEffectPeriodicUpdate, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_monk_renewing_mistAuraScript();
    }
};

// Diffuse Magic - 122783
class spell_monk_diffuse_magic : public SpellScriptLoader
{
    public:
        spell_monk_diffuse_magic() : SpellScriptLoader("spell_monk_diffuse_magic") { }

        class spell_monk_diffuse_magic_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_diffuse_magic_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    Unit::AuraApplicationMap AuraList = _player->GetAppliedAuras();
                    for (Unit::AuraApplicationMap::iterator iter = AuraList.begin(); iter != AuraList.end(); ++iter)
                    {
                        Aura* aura = iter->second->GetBase();
                        if (!aura)
                            continue;

                        Unit* caster = aura->GetCaster();
                        if (!caster || caster->GetGUID() == _player->GetGUID())
                            continue;

                        if (!caster->IsWithinDist(_player, 40.0f))
                            continue;

                        if (aura->GetSpellInfo()->IsPositive())
                            continue;

                        if (!(aura->GetSpellInfo()->GetSchoolMask() & SPELL_SCHOOL_MASK_MAGIC))
                            continue;

                        _player->AddAura(aura->GetSpellInfo()->Id, caster);

                        if (Aura* targetAura = caster->GetAura(aura->GetSpellInfo()->Id, _player->GetGUID()))
                            for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
                                if (targetAura->GetEffect(i) && aura->GetEffect(i))
                                    targetAura->GetEffect(i)->SetAmount(aura->GetEffect(i)->GetAmount());

                        _player->RemoveAura(aura->GetSpellInfo()->Id, caster->GetGUID());
                        
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_diffuse_magic_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_diffuse_magic_SpellScript();
        }
};

// Guard - 115295
class spell_monk_guard : public SpellScriptLoader
{
    public:
        spell_monk_guard() : SpellScriptLoader("spell_monk_guard") { }

        class spell_monk_guard_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_monk_guard_AuraScript);

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                if (!GetCaster())
                    return;

                if (Player* _plr = GetCaster()->ToPlayer())
                {
                    amount += int32(_plr->GetTotalAttackPowerValue(BASE_ATTACK) * 1.971f);

                    if (_plr->HasAura(ITEM_MONK_T14_TANK_4P))
                        amount = int32(amount * 1.2f);
                }
                // For Black Ox Statue
                else if (GetCaster()->GetOwner())
                {
                    if (Player* _plr = GetCaster()->GetOwner()->ToPlayer())
                    {
                        amount += int32(_plr->GetTotalAttackPowerValue(BASE_ATTACK) * 1.971f);

                        if (_plr->HasAura(ITEM_MONK_T14_TANK_4P))
                            amount = int32(amount * 1.2f);
                    }
                }
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_monk_guard_AuraScript::CalculateAmount, EFFECT_0, SPELL_AURA_SCHOOL_ABSORB);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_monk_guard_AuraScript();
        }
};

// Bear Hug - 127361
class spell_monk_bear_hug : public SpellScriptLoader
{
    public:
        spell_monk_bear_hug() : SpellScriptLoader("spell_monk_bear_hug") { }

        class spell_monk_bear_hug_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_bear_hug_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (Aura* bearHug = target->GetAura(SPELL_MONK_BEAR_HUG, _player->GetGUID()))
                            if (bearHug->GetEffect(1))
                                bearHug->GetEffect(1)->SetAmount(_player->CountPctFromMaxHealth(2));
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_bear_hug_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_bear_hug_SpellScript();
        }
};

// Zen Flight - 125883
class spell_monk_zen_flight_check : public SpellScriptLoader
{
    public:
        spell_monk_zen_flight_check() : SpellScriptLoader("spell_monk_zen_flight_check") { }

        class spell_monk_zen_flight_check_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_zen_flight_check_SpellScript);

            SpellCastResult CheckTarget()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->GetMap()->IsBattlegroundOrArena())
                        return SPELL_FAILED_NOT_IN_BATTLEGROUND;

                    // In Kalimdor and Eastern Kingdom
                    if (!_player->HasSpell(90267) && (_player->GetMapId() == 1 || _player->GetMapId() == 0))
                        return SPELL_FAILED_NOT_HERE;
                }

                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_monk_zen_flight_check_SpellScript::CheckTarget);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_zen_flight_check_SpellScript();
        }
};

// Glyph of Zen Flight - 125893
class spell_monk_glyph_of_zen_flight : public SpellScriptLoader
{
    public:
        spell_monk_glyph_of_zen_flight() : SpellScriptLoader("spell_monk_glyph_of_zen_flight") { }

        class spell_monk_glyph_of_zen_flight_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_monk_glyph_of_zen_flight_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Player* _player = GetTarget()->ToPlayer())
                    _player->learnSpell(SPELL_MONK_ZEN_FLIGHT, false);
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Player* _player = GetTarget()->ToPlayer())
                    if (_player->HasSpell(SPELL_MONK_ZEN_FLIGHT))
                        _player->removeSpell(SPELL_MONK_ZEN_FLIGHT, false, false);
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_monk_glyph_of_zen_flight_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_monk_glyph_of_zen_flight_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }

        };

        AuraScript* GetAuraScript() const
        {
            return new spell_monk_glyph_of_zen_flight_AuraScript();
        }
};

// Called by Jab - 100780
// Power Strikes - 121817
class spell_monk_power_strikes : public SpellScriptLoader
{
    public:
        spell_monk_power_strikes() : SpellScriptLoader("spell_monk_power_strikes") { }

        class spell_monk_power_strikes_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_power_strikes_SpellScript)

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (target->GetGUID() != _player->GetGUID())
                        {
                            if (_player->HasAura(SPELL_MONK_POWER_STRIKES_TALENT))
                            {
                                if (!_player->HasSpellCooldown(SPELL_MONK_POWER_STRIKES_TALENT))
                                {
                                    if (_player->GetPower(POWER_CHI) < _player->GetMaxPower(POWER_CHI))
                                    {
                                        _player->EnergizeBySpell(_player, GetSpellInfo()->Id, 1, POWER_CHI);
                                        _player->AddSpellCooldown(SPELL_MONK_POWER_STRIKES_TALENT, 0, getPreciseTime() + 20.0);
                                    }
                                    else
                                        _player->CastSpell(_player, SPELL_MONK_CREATE_CHI_SPHERE, true);
                                }
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_power_strikes_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_power_strikes_SpellScript();
        }
};

// Crackling Jade Lightning - 117952
class spell_monk_crackling_jade_lightning : public SpellScriptLoader
{
    public:
        spell_monk_crackling_jade_lightning() : SpellScriptLoader("spell_monk_crackling_jade_lightning") { }

        class spell_monk_crackling_jade_lightning_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_monk_crackling_jade_lightning_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* caster = GetCaster())
                    if (roll_chance_i(25))
                        caster->CastSpell(caster, SPELL_MONK_JADE_LIGHTNING_ENERGIZE, true);
            }

            void OnProc(AuraEffect const* aurEff, ProcEventInfo& eventInfo)
            {
                PreventDefaultAction();

                if (!GetCaster())
                    return;

                if (eventInfo.GetActor()->GetGUID() != GetTarget()->GetGUID())
                    return;

                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (GetTarget()->HasAura(aurEff->GetSpellInfo()->Id, _player->GetGUID()))
                    {
                        if (!_player->HasSpellCooldown(SPELL_MONK_CRACKLING_JADE_SHOCK_BUMP))
                        {
                            _player->CastSpell(GetTarget(), SPELL_MONK_CRACKLING_JADE_SHOCK_BUMP, true);
                            _player->AddSpellCooldown(SPELL_MONK_CRACKLING_JADE_SHOCK_BUMP, 0, getPreciseTime() + 8.0);
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_monk_crackling_jade_lightning_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
                OnEffectProc += AuraEffectProcFn(spell_monk_crackling_jade_lightning_AuraScript::OnProc, EFFECT_0, SPELL_AURA_PERIODIC_DAMAGE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_monk_crackling_jade_lightning_AuraScript();
        }
};

// Touch of Karma - 122470
class spell_monk_touch_of_karma : public SpellScriptLoader
{
    public:
        spell_monk_touch_of_karma() : SpellScriptLoader("spell_monk_touch_of_karma") { }

        class spell_monk_touch_of_karma_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_monk_touch_of_karma_AuraScript);

            uint64 eff1Target;

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    caster->m_SpecialTarget = GetUnitOwner();
            }

            void AfterRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                {
                    caster->RemoveAura(122470);
                    caster->m_SpecialTarget = NULL;
                }
            }

            void CalculateAmount(AuraEffect const* /*aurEff*/, int32 & amount, bool & /*canBeRecalculated*/)
            {
                if (GetCaster())
                    amount = GetCaster()->GetMaxHealth();
            }

            void AfterAbsorb(AuraEffect* /*aurEff*/, DamageInfo& dmgInfo, uint32& /*absorbAmount*/)
            {
                if (Unit* caster = dmgInfo.GetVictim())
                {
                    if (Unit* target = caster->m_SpecialTarget)
                    {
                        int32 bp = dmgInfo.GetAbsorb();

                        if (bp)
                            caster->CastCustomSpell(target, SPELL_MONK_TOUCH_OF_KARMA_REDIRECT_DAMAGE, &bp, NULL, NULL, true);
                    }
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_monk_touch_of_karma_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_monk_touch_of_karma_AuraScript::AfterRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_monk_touch_of_karma_AuraScript::CalculateAmount, EFFECT_1, SPELL_AURA_SCHOOL_ABSORB);
                AfterEffectAbsorb += AuraEffectAbsorbFn(spell_monk_touch_of_karma_AuraScript::AfterAbsorb, EFFECT_1);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_monk_touch_of_karma_AuraScript();
        }
};

// Spinning Fire Blossom - 123408
class spell_monk_spinning_fire_blossom_damage : public SpellScriptLoader
{
    public:
        spell_monk_spinning_fire_blossom_damage() : SpellScriptLoader("spell_monk_spinning_fire_blossom_damage") { }

        class spell_monk_spinning_fire_blossom_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_spinning_fire_blossom_damage_SpellScript);

            SpellCastResult CheckTarget()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetExplTargetUnit())
                        if (_player->IsFriendlyTo(target))
                            return SPELL_FAILED_BAD_TARGETS;

                return SPELL_CAST_OK;
            }

            void HandleAfterHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (target->GetDistance(_player) > 10.0f)
                        {
                            SetHitDamage(int32(GetHitDamage() * 1.5f));
                            _player->CastSpell(target, SPELL_MONK_SPINNING_FIRE_BLOSSOM_ROOT, true);
                        }
                    }
                }
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_monk_spinning_fire_blossom_damage_SpellScript::CheckTarget);
                AfterHit += SpellHitFn(spell_monk_spinning_fire_blossom_damage_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_spinning_fire_blossom_damage_SpellScript();
        }
};

// Spinning Fire Blossom - 115073
class spell_monk_spinning_fire_blossom : public SpellScriptLoader
{
    public:
        spell_monk_spinning_fire_blossom() : SpellScriptLoader("spell_monk_spinning_fire_blossom") { }

        class spell_monk_spinning_fire_blossom_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_spinning_fire_blossom_SpellScript)

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    std::list<Unit*> tempList;
                    std::list<Unit*> targetList;

                    _player->GetAttackableUnitListInRange(tempList, 50.0f);

                    for (std::list<Unit*>::const_iterator itr = tempList.begin(); itr != tempList.end(); ++itr)
                    {
                        if (!_player->IsValidAttackTarget(*itr))
                            continue;

                        if (!_player->isInFront(*itr))
                            continue;

                        if (!(*itr)->IsWithinLOSInMap(_player))
                            continue;

                        if ((*itr)->GetGUID() == _player->GetGUID())
                            continue;

                        targetList.push_back(*itr);
                    }

                    if (!targetList.empty())
                    {
                        Trinity::Containers::RandomResizeList(targetList, 1);

                        for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                            _player->CastSpell(*itr, SPELL_MONK_SPINNING_FIRE_BLOSSOM_DAMAGE, true);
                    }
                    else
                        _player->CastSpell(_player, SPELL_MONK_SPINNING_FIRE_BLOSSOM_MISSILE, true);
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_monk_spinning_fire_blossom_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_spinning_fire_blossom_SpellScript();
        }
};

// Path of Blossom - 124336
class spell_monk_path_of_blossom : public SpellScriptLoader
{
    public:
        spell_monk_path_of_blossom() : SpellScriptLoader("spell_monk_path_of_blossom") { }

        class spell_monk_path_of_blossom_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_monk_path_of_blossom_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (GetCaster())
                    GetCaster()->CastSpell(GetCaster(), SPELL_MONK_PATH_OF_BLOSSOM_AREATRIGGER, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_monk_path_of_blossom_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_monk_path_of_blossom_AuraScript();
        }
};

// Called by Uplift - 116670
// Thunder Focus Tea - 116680
class spell_monk_thunder_focus_tea : public SpellScriptLoader
{
    public:
        spell_monk_thunder_focus_tea() : SpellScriptLoader("spell_monk_thunder_focus_tea") { }

        class spell_monk_thunder_focus_tea_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_thunder_focus_tea_SpellScript)

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (_player->HasAura(SPELL_MONK_THUNDER_FOCUS_TEA))
                        {
                            std::list<Unit*> groupList;

                            _player->GetPartyMembers(groupList);

                            for (std::list<Unit*>::const_iterator itr = groupList.begin(); itr != groupList.end(); ++itr)
                                if (Aura* renewingMistGroup = (*itr)->GetAura(SPELL_MONK_RENEWING_MIST_HOT, _player->GetGUID()))
                                    renewingMistGroup->RefreshDuration();

                            _player->RemoveAura(SPELL_MONK_THUNDER_FOCUS_TEA);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_thunder_focus_tea_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_thunder_focus_tea_SpellScript();
        }
};

// Summon Jade Serpent Statue - 115313
class spell_monk_jade_serpent_statue : public SpellScriptLoader
{
    public:
        spell_monk_jade_serpent_statue() : SpellScriptLoader("spell_monk_jade_serpent_statue") { }

        class spell_monk_jade_serpent_statue_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_jade_serpent_statue_SpellScript)

            void HandleSummon(SpellEffIndex effIndex)
            {
                if (Player* player = GetCaster()->ToPlayer())
                {
                    PreventHitDefaultEffect(effIndex);

                    const SpellInfo* spell = GetSpellInfo();
                    std::list<Creature*> tempList;
                    std::list<Creature*> jadeSerpentlist;

                    player->GetCreatureListWithEntryInGrid(tempList, MONK_NPC_JADE_SERPENT_STATUE, 500.0f);
                    player->GetCreatureListWithEntryInGrid(jadeSerpentlist, MONK_NPC_JADE_SERPENT_STATUE, 500.0f);

                    // Remove other players jade statue
                    for (std::list<Creature*>::iterator i = tempList.begin(); i != tempList.end(); ++i)
                    {
                        Unit* owner = (*i)->GetOwner();
                        if (owner && owner == player && (*i)->isSummon())
                            continue;

                        jadeSerpentlist.remove((*i));
                    }

                    // 1 statue max
                    if ((int32)jadeSerpentlist.size() >= spell->Effects[effIndex].BasePoints)
                        jadeSerpentlist.back()->ToTempSummon()->UnSummon();

                    Position pos;
                    GetExplTargetDest()->GetPosition(&pos);
                    const SummonPropertiesEntry* properties = sSummonPropertiesStore.LookupEntry(spell->Effects[effIndex].MiscValueB);
                    TempSummon* summon = player->SummonCreature(spell->Effects[effIndex].MiscValue, pos, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, spell->GetDuration());
                    if (!summon)
                        return;

                    summon->SetUInt64Value(UNIT_FIELD_SUMMONEDBY, player->GetGUID());
                    summon->setFaction(player->getFaction());
                    summon->SetUInt32Value(UNIT_CREATED_BY_SPELL, GetSpellInfo()->Id);
                    summon->SetMaxHealth(player->CountPctFromMaxHealth(50));
                    summon->SetHealth(summon->GetMaxHealth());
                }
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_monk_jade_serpent_statue_SpellScript::HandleSummon, EFFECT_0, SPELL_EFFECT_SUMMON);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_jade_serpent_statue_SpellScript();
        }
};

// Called by Spinning Crane Kick - 107270
// Teachings of the Monastery - 116645
class spell_monk_teachings_of_the_monastery : public SpellScriptLoader
{
    public:
        spell_monk_teachings_of_the_monastery() : SpellScriptLoader("spell_monk_teachings_of_the_monastery") { }

        class spell_monk_teachings_of_the_monastery_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_teachings_of_the_monastery_SpellScript);

            void HandleAfterCast()
            {
                if (GetCaster())
                    if (GetCaster()->HasAura(118672))
                        GetCaster()->CastSpell(GetCaster(), SPELL_MONK_SPINNING_CRANE_KICK_HEAL, true);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_monk_teachings_of_the_monastery_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_teachings_of_the_monastery_SpellScript();
        }
};

// Mana Tea - 115294
class spell_monk_mana_tea : public SpellScriptLoader
{
    public:
        spell_monk_mana_tea() : SpellScriptLoader("spell_monk_mana_tea") { }

        class spell_monk_mana_tea_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_mana_tea_SpellScript);

            SpellModifier* spellMod;

            void HandleBeforeCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    int32 stacks = 0;

                    if (Aura* manaTeaStacks = _player->GetAura(SPELL_MONK_MANA_TEA_STACKS))
                        stacks = manaTeaStacks->GetStackAmount();

                    int32 newDuration = stacks * IN_MILLISECONDS - GetSpellInfo()->GetDuration();

                    spellMod = new SpellModifier();
                    spellMod->op = SPELLMOD_DURATION;
                    spellMod->type = SPELLMOD_FLAT;
                    spellMod->spellId = SPELL_MONK_MANA_TEA_REGEN;
                    spellMod->value = newDuration;
                    spellMod->mask[1] = 0x200000;
                    spellMod->mask[2] = 0x1;

                    _player->AddSpellMod(spellMod, true);
                }
            }

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    _player->AddSpellMod(spellMod, false);
            }

            void Register()
            {
                BeforeCast += SpellCastFn(spell_monk_mana_tea_SpellScript::HandleBeforeCast);
                AfterCast += SpellCastFn(spell_monk_mana_tea_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_mana_tea_SpellScript();
        }

        class spell_monk_mana_tea_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_monk_mana_tea_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (GetCaster())
                {
                    // remove one charge per tick instead of remove aura on cast
                    // "Cancelling the channel will not waste stacks"
                    if (Aura* manaTea = GetCaster()->GetAura(SPELL_MONK_MANA_TEA_STACKS))
                    {
                        if (manaTea->GetStackAmount() > 1)
                            manaTea->SetStackAmount(manaTea->GetStackAmount() - 1);
                        else
                            GetCaster()->RemoveAura(SPELL_MONK_MANA_TEA_STACKS);
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_monk_mana_tea_AuraScript::OnTick, EFFECT_0, SPELL_AURA_OBS_MOD_POWER);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_monk_mana_tea_AuraScript();
        }
};

// Brewing : Mana Tea - 123766
class spell_monk_mana_tea_stacks : public SpellScriptLoader
{
    public:
        spell_monk_mana_tea_stacks() : SpellScriptLoader("spell_monk_mana_tea_stacks") { }

        class spell_monk_mana_tea_stacks_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_monk_mana_tea_stacks_AuraScript);

            uint32 chiConsumed;

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                chiConsumed = 0;
            }

            void SetData(uint32 type, uint32 data)
            {
                while ((chiConsumed += data) >= 4)
                {
                    chiConsumed = 0;
                    data = data > 4 ? data - 4: 0;

                    if (GetCaster())
                    {
                        GetCaster()->CastSpell(GetCaster(), SPELL_MONK_MANA_TEA_STACKS, true);
                        GetCaster()->CastSpell(GetCaster(), SPELL_MONK_PLUS_ONE_MANA_TEA, true);
                    }
                }
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_monk_mana_tea_stacks_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_monk_mana_tea_stacks_AuraScript();
        }
};

// Enveloping Mist - 124682
class spell_monk_enveloping_mist : public SpellScriptLoader
{
    public:
        spell_monk_enveloping_mist() : SpellScriptLoader("spell_monk_enveloping_mist") { }

        class spell_monk_enveloping_mist_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_enveloping_mist_SpellScript);

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetExplTargetUnit())
                        _player->CastSpell(target, SPELL_MONK_ENVELOPING_MIST_HEAL, true);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_monk_enveloping_mist_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_enveloping_mist_SpellScript();
        }
};

// Surging Mist - 116694
class spell_monk_surging_mist : public SpellScriptLoader
{
    public:
        spell_monk_surging_mist() : SpellScriptLoader("spell_monk_surging_mist") { }

        class spell_monk_surging_mist_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_surging_mist_SpellScript);

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetExplTargetUnit())
                        _player->CastSpell(target, SPELL_MONK_SURGING_MIST_HEAL, true);
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_monk_surging_mist_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_surging_mist_SpellScript();
        }
};

// Called by : Fortifying Brew - 115203, Chi Brew - 115399, Elusive Brew - 115308, Tigereye Brew - 116740
// Purifying Brew - 119582, Mana Tea - 115294, Thunder Focus Tea - 116680 and Energizing Brew - 115288
// Healing Elixirs - 122280
class spell_monk_healing_elixirs : public SpellScriptLoader
{
    public:
        spell_monk_healing_elixirs() : SpellScriptLoader("spell_monk_healing_elixirs") { }

        class spell_monk_healing_elixirs_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_healing_elixirs_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->HasAura(SPELL_MONK_HEALING_ELIXIRS_AURA))
                    {
                        int32 bp = 10;

                        if (!_player->HasSpellCooldown(SPELL_MONK_HEALING_ELIXIRS_RESTORE_HEALTH))
                        {
                            _player->CastCustomSpell(_player, SPELL_MONK_HEALING_ELIXIRS_RESTORE_HEALTH, &bp, NULL, NULL, true);
                            // This effect cannot occur more than once per 18s
                            _player->AddSpellCooldown(SPELL_MONK_HEALING_ELIXIRS_RESTORE_HEALTH, 0, getPreciseTime() + 18.0);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_healing_elixirs_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_healing_elixirs_SpellScript();
        }
};

// Zen Sphere - 124081
class spell_monk_zen_sphere : public SpellScriptLoader
{
    public:
        spell_monk_zen_sphere() : SpellScriptLoader("spell_monk_zen_sphere") { }

        class spell_monk_zen_sphere_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_zen_sphere_SpellScript);

            bool active;

            void HandleBeforeHit()
            {
                active = false;

                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (target->HasAura(SPELL_MONK_ZEN_SPHERE_HEAL))
                            active = true;
            }

            void HandleAfterHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (active)
                        {
                            _player->CastSpell(_player, SPELL_MONK_ZEN_SPHERE_DETONATE_HEAL, true);
                            _player->CastSpell(_player, SPELL_MONK_ZEN_SPHERE_DETONATE_DAMAGE, true);
                            _player->RemoveAura(SPELL_MONK_ZEN_SPHERE_HEAL);
                            active = false;
                        }
                    }
                }
            }

            void Register()
            {
                BeforeHit += SpellHitFn(spell_monk_zen_sphere_SpellScript::HandleBeforeHit);
                AfterHit += SpellHitFn(spell_monk_zen_sphere_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_zen_sphere_SpellScript();
        }
};

// Zen Sphere - 124081
class spell_monk_zen_sphere_hot : public SpellScriptLoader
{
    public:
        spell_monk_zen_sphere_hot() : SpellScriptLoader("spell_monk_zen_sphere_hot") { }

        class spell_monk_zen_sphere_hot_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_monk_zen_sphere_hot_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* caster = GetCaster())
                    if (Player* _player = caster->ToPlayer())
                        _player->CastSpell(_player, SPELL_MONK_ZEN_SPHERE_DAMAGE, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_monk_zen_sphere_hot_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_monk_zen_sphere_hot_AuraScript();
        }
};

// Chi Burst - 123986
class spell_monk_chi_burst : public SpellScriptLoader
{
    public:
        spell_monk_chi_burst() : SpellScriptLoader("spell_monk_chi_burst") { }

        class spell_monk_chi_burst_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_chi_burst_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        std::list<Unit*> tempUnitMap;
                        _player->GetAttackableUnitListInRange(tempUnitMap, _player->GetDistance(target));

                        // Chi Burst will always heal the Monk
                        _player->CastSpell(_player, SPELL_MONK_CHI_BURST_HEAL, true);

                        if (_player->IsValidAttackTarget(target))
                            _player->CastSpell(target, SPELL_MONK_CHI_BURST_DAMAGE, true);
                        else
                            _player->CastSpell(target, SPELL_MONK_CHI_BURST_HEAL, true);

                        for (std::list<Unit*>::const_iterator itr = tempUnitMap.begin(); itr != tempUnitMap.end(); ++itr)
                        {
                            if ((*itr)->GetGUID() == _player->GetGUID())
                                continue;

                            if (!(*itr)->IsInBetween(_player, target, 1.0f))
                                continue;

                            uint32 spell = _player->IsValidAttackTarget(*itr) ? SPELL_MONK_CHI_BURST_DAMAGE : SPELL_MONK_CHI_BURST_HEAL;
                            _player->CastSpell(*itr, spell, true);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_chi_burst_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_chi_burst_SpellScript();
        }
};

// Energizing Brew - 115288
class spell_monk_energizing_brew : public SpellScriptLoader
{
    public:
        spell_monk_energizing_brew() : SpellScriptLoader("spell_monk_energizing_brew") { }

        class spell_monk_energizing_brew_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_energizing_brew_SpellScript);

            SpellCastResult CheckFight()
            {
                if (!GetCaster()->isInCombat())
                    return SPELL_FAILED_CASTER_AURASTATE;
                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_monk_energizing_brew_SpellScript::CheckFight);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_energizing_brew_SpellScript();
        }
};

// Spear Hand Strike - 116705
class spell_monk_spear_hand_strike : public SpellScriptLoader
{
    public:
        spell_monk_spear_hand_strike() : SpellScriptLoader("spell_monk_spear_hand_strike") { }

        class spell_monk_spear_hand_strike_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_spear_hand_strike_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (target->isInFront(_player))
                        {
                            _player->CastSpell(target, SPELL_MONK_SPEAR_HAND_STRIKE_SILENCE, true);
                            _player->AddSpellCooldown(116705, 0, getPreciseTime() + 18.0);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_spear_hand_strike_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_spear_hand_strike_SpellScript();
        }
};

// Tiger's Lust - 116841
class spell_monk_tigers_lust : public SpellScriptLoader
{
    public:
        spell_monk_tigers_lust() : SpellScriptLoader("spell_monk_tigers_lust") { }

        class spell_monk_tigers_lust_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_tigers_lust_SpellScript);

            bool Validate()
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_MONK_FLYING_SERPENT_KICK_NEW))
                    return false;
                return true;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        target->RemoveMovementImpairingAuras();
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_tigers_lust_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_tigers_lust_SpellScript();
        }
};

// Flying Serpent Kick - 115057
class spell_monk_flying_serpent_kick : public SpellScriptLoader
{
    public:
        spell_monk_flying_serpent_kick() : SpellScriptLoader("spell_monk_flying_serpent_kick") { }

        class spell_monk_flying_serpent_kick_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_flying_serpent_kick_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_MONK_FLYING_SERPENT_KICK_NEW))
                    return false;
                return true;
            }

            void HandleOnCast()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Player* _player = caster->ToPlayer())
                    {
                        if (_player->HasAura(SPELL_MONK_FLYING_SERPENT_KICK))
                            _player->RemoveAura(SPELL_MONK_FLYING_SERPENT_KICK);

                        _player->CastSpell(_player, SPELL_MONK_FLYING_SERPENT_KICK_AOE, true);
                    }
                }
            }

            void Register()
            {
                OnCast += SpellCastFn(spell_monk_flying_serpent_kick_SpellScript::HandleOnCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_flying_serpent_kick_SpellScript();
        }
};

// Chi Torpedo - 115008 or Chi Torpedo (3 charges) - 121828
class spell_monk_chi_torpedo : public SpellScriptLoader
{
    public:
        spell_monk_chi_torpedo() : SpellScriptLoader("spell_monk_chi_torpedo") { }

        class spell_monk_chi_torpedo_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_chi_torpedo_SpellScript);

            void HandleAfterCast()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Player* _player = caster->ToPlayer())
                    {
                        std::list<Unit*> tempUnitMap;
                        _player->GetAttackableUnitListInRange(tempUnitMap, 20.0f);

                        for (std::list<Unit*>::const_iterator itr = tempUnitMap.begin(); itr != tempUnitMap.end(); ++itr)
                        {
                            if (!(_player)->isInFront((*itr), M_PI / 3) && (*itr)->GetGUID() != _player->GetGUID())
                                continue;

                            uint32 spell = _player->IsValidAttackTarget(*itr) ? SPELL_MONK_CHI_TORPEDO_DAMAGE : SPELL_MONK_CHI_TORPEDO_HEAL;
                            _player->CastSpell(*itr, spell, true);
                        }
                    }
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_monk_chi_torpedo_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_chi_torpedo_SpellScript();
        }
};

// Purifying Brew - 119582
class spell_monk_purifying_brew : public SpellScriptLoader
{
    public:
        spell_monk_purifying_brew() : SpellScriptLoader("spell_monk_purifying_brew") { }

        class spell_monk_purifying_brew_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_purifying_brew_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Player* _player = caster->ToPlayer())
                    {
                        AuraApplication* staggerAmount = _player->GetAuraApplication(SPELL_MONK_LIGHT_STAGGER);

                        if (!staggerAmount)
                            staggerAmount = _player->GetAuraApplication(SPELL_MONK_MODERATE_STAGGER);
                        if (!staggerAmount)
                            staggerAmount = _player->GetAuraApplication(SPELL_MONK_HEAVY_STAGGER);

                        if (staggerAmount)
                            _player->RemoveAura(staggerAmount->GetBase()->GetId());
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_purifying_brew_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_purifying_brew_SpellScript();
        }
};

// Clash - 122057 and Clash - 126449
class spell_monk_clash : public SpellScriptLoader
{
    public:
        spell_monk_clash() : SpellScriptLoader("spell_monk_clash") { }

        class spell_monk_clash_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_clash_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Player* _player = caster->ToPlayer())
                    {
                        if (Unit* target = GetHitUnit())
                        {
                            int32 basePoint = 2;
                            _player->CastCustomSpell(target, SPELL_MONK_CLASH_CHARGE, &basePoint, NULL, NULL, true);
                            target->CastSpell(_player, SPELL_MONK_CLASH_CHARGE, true);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_clash_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_clash_SpellScript();
        }
};

// Keg Smash - 121253
class spell_monk_keg_smash : public SpellScriptLoader
{
    public:
        spell_monk_keg_smash() : SpellScriptLoader("spell_monk_keg_smash") { }

        class spell_monk_keg_smash_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_keg_smash_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Player* _player = caster->ToPlayer())
                    {
                        if (Unit* target = GetHitUnit())
                        {
                            _player->CastSpell(target, SPELL_MONK_KEG_SMASH_VISUAL, true);
                            _player->CastSpell(target, SPELL_MONK_WEAKENED_BLOWS, true);
                            _player->CastSpell(_player, SPELL_MONK_KEG_SMASH_ENERGIZE, true);
                            // Prevent to receive 2 CHI more than once time per cast
                            _player->AddSpellCooldown(SPELL_MONK_KEG_SMASH_ENERGIZE, 0, getPreciseTime() + 1.0);
                            _player->CastSpell(target, SPELL_MONK_DIZZYING_HAZE, true);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_keg_smash_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_keg_smash_SpellScript();
        }
};

// Elusive Brew - 115308
class spell_monk_elusive_brew : public SpellScriptLoader
{
    public:
        spell_monk_elusive_brew() : SpellScriptLoader("spell_monk_elusive_brew") { }

        class spell_monk_elusive_brew_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_elusive_brew_SpellScript);

            void HandleOnHit()
            {
                int32 stackAmount = 0;

                if (Unit* caster = GetCaster())
                {
                    if (Player* _player = caster->ToPlayer())
                    {
                        if (AuraApplication* brewStacks = _player->GetAuraApplication(SPELL_MONK_ELUSIVE_BREW_STACKS))
                            stackAmount = brewStacks->GetBase()->GetStackAmount();

                        _player->AddAura(SPELL_MONK_ELUSIVE_BREW, _player);

                        if (AuraApplication* aura = _player->GetAuraApplication(SPELL_MONK_ELUSIVE_BREW))
                        {
                            Aura* elusiveBrew = aura->GetBase();
                            int32 maxDuration = elusiveBrew->GetMaxDuration();
                            int32 newDuration = stackAmount * 1000;
                            elusiveBrew->SetDuration(newDuration);

                            if (newDuration > maxDuration)
                                elusiveBrew->SetMaxDuration(newDuration);

                            _player->RemoveAura(SPELL_MONK_ELUSIVE_BREW_STACKS);
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_monk_elusive_brew_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_elusive_brew_SpellScript();
        }
};

// Breath of Fire - 115181
class spell_monk_breath_of_fire : public SpellScriptLoader
{
    public:
        spell_monk_breath_of_fire() : SpellScriptLoader("spell_monk_breath_of_fire") { }

        class spell_monk_breath_of_fire_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_breath_of_fire_SpellScript);

            void HandleAfterHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Player* _player = caster->ToPlayer())
                    {
                        if (Unit* target = GetHitUnit())
                        {
                            // if Dizzying Haze is on the target, they will burn for an additionnal damage over 8s
                            if (target->HasAura(SPELL_MONK_DIZZYING_HAZE))
                            {
                                _player->CastSpell(target, SPELL_MONK_BREATH_OF_FIRE_DOT, true);

                                // Glyph of Breath of Fire
                                if (_player->HasAura(123394))
                                    _player->CastSpell(target, SPELL_MONK_BREATH_OF_FIRE_CONFUSED, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_monk_breath_of_fire_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_breath_of_fire_SpellScript();
        }
};

// Soothing Mist - 115175
class spell_monk_soothing_mist : public SpellScriptLoader
{
    public:
        spell_monk_soothing_mist() : SpellScriptLoader("spell_monk_soothing_mist") { }

        class spell_monk_soothing_mist_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_monk_soothing_mist_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (!GetCaster())
                    return;

                if (Unit* target = GetTarget())
                    target->CastSpell(target, SPELL_MONK_SOOTHING_MIST_VISUAL, true);

                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetTarget())
                    {
                        std::list<Unit*> playerList;
                        std::list<Creature*> tempList;
                        std::list<Creature*> statueList;
                        Creature* statue;

                        _player->GetPartyMembers(playerList);

                        if (playerList.size() > 1)
                        {
                            playerList.remove(target);
                            playerList.sort(Trinity::HealthPctOrderPred());
                            playerList.resize(1);
                        }

                        _player->GetCreatureListWithEntryInGrid(tempList, 60849, 100.0f);
                        _player->GetCreatureListWithEntryInGrid(statueList, 60849, 100.0f);

                        // Remove other players jade statue
                        for (std::list<Creature*>::iterator i = tempList.begin(); i != tempList.end(); ++i)
                        {
                            Unit* owner = (*i)->GetOwner();
                            if (owner && owner == _player && (*i)->isSummon())
                                continue;

                            statueList.remove((*i));
                        }

                        for (std::list<Unit*>::const_iterator itr = playerList.begin(); itr != playerList.end(); ++itr)
                        {
                            if (statueList.size() == 1)
                            {
                                for (std::list<Creature*>::const_iterator itrBis = statueList.begin(); itrBis != statueList.end(); ++itrBis)
                                    statue = *itrBis;

                                if (statue && (statue->isPet() || statue->isGuardian()))
                                    if (statue->GetOwner() && statue->GetOwner()->GetGUID() == _player->GetGUID())
                                        statue->CastSpell(*itr, GetSpellInfo()->Id, true);
                            }
                        }
                    }
                }
            }

            void HandleEffectPeriodic(AuraEffect const* /*aurEff*/)
            {
                if (Unit* caster = GetCaster())
                    if (Unit* target = GetTarget())
                        // 25% to give 1 chi per tick
                        if (roll_chance_i(25))
                            caster->CastSpell(caster, SPELL_MONK_SOOTHING_MIST_ENERGIZE, true);
            }

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* caster = GetCaster())
                    if (Unit* target = GetTarget())
                        if (target->HasAura(SPELL_MONK_SOOTHING_MIST_VISUAL))
                            target->RemoveAura(SPELL_MONK_SOOTHING_MIST_VISUAL);
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_monk_soothing_mist_AuraScript::OnApply, EFFECT_0, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_monk_soothing_mist_AuraScript::HandleEffectPeriodic, EFFECT_0, SPELL_AURA_PERIODIC_HEAL);
                AfterEffectRemove += AuraEffectRemoveFn(spell_monk_soothing_mist_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_HEAL, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_monk_soothing_mist_AuraScript();
        }
};

// Disable - 116095
class spell_monk_disable : public SpellScriptLoader
{
    public:
        spell_monk_disable() : SpellScriptLoader("spell_monk_disable") { }

        class spell_monk_disable_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_disable_SpellScript);

            bool snaredOnHit;

            SpellCastResult CheckCast()
            {
                snaredOnHit = false;

                if (GetCaster())
                    if (Unit* target = GetCaster()->getVictim())
                        if (target->HasAuraType(SPELL_AURA_MOD_DECREASE_SPEED))
                            snaredOnHit = true;

                return SPELL_CAST_OK;
            }

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                    if (Player* _player = caster->ToPlayer())
                        if (Unit* target = GetHitUnit())
                            if (snaredOnHit)
                                _player->CastSpell(target, SPELL_MONK_DISABLE_ROOT, true);
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_monk_disable_SpellScript::CheckCast);
                OnHit += SpellHitFn(spell_monk_disable_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_disable_SpellScript();
        }
};

// Zen Pilgrimage - 126892 and Zen Pilgrimage : Return - 126895
class spell_monk_zen_pilgrimage : public SpellScriptLoader
{
    public:
        spell_monk_zen_pilgrimage() : SpellScriptLoader("spell_monk_zen_pilgrimage") { }

        class spell_monk_zen_pilgrimage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_zen_pilgrimage_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_MONK_ZEN_PILGRIMAGE) || !sSpellMgr->GetSpellInfo(SPELL_MONK_ZEN_PILGRIMAGE_RETURN))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    if (Player* _player = caster->ToPlayer())
                    {
                        if (GetSpellInfo()->Id == SPELL_MONK_ZEN_PILGRIMAGE)
                        {
                            _player->SaveRecallPosition();
                            _player->TeleportTo(870, 3818.55f, 1793.18f, 950.35f, _player->GetOrientation());
                        }
                        else if (GetSpellInfo()->Id == SPELL_MONK_ZEN_PILGRIMAGE_RETURN)
                        {
                            _player->TeleportTo(_player->m_recallMap, _player->m_recallX, _player->m_recallY, _player->m_recallZ, _player->m_recallO);
                            _player->RemoveAura(126896);
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_monk_zen_pilgrimage_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_zen_pilgrimage_SpellScript();
        }
};

// Blackout Kick - 100784
class spell_monk_blackout_kick : public SpellScriptLoader
{
    public:
        spell_monk_blackout_kick() : SpellScriptLoader("spell_monk_blackout_kick") { }

        class spell_monk_blackout_kick_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_blackout_kick_SpellScript);

            void HandleAfterHit()
            {
                if (Unit* caster = GetCaster())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (caster->HasAura(128595))
                        {
                            uint32 triggered_spell_id = 128531;
                            Unit* originalCaster = caster->GetOwner() ? caster->GetOwner(): caster;
                            int32 basepoints0 = CalculatePct(GetHitDamage(), GetSpellInfo()->Effects[EFFECT_1].BasePoints);
                            if (!originalCaster->HasAura(132005) && !target->isInBack(caster))
                                triggered_spell_id = 128591;

                            if (triggered_spell_id == 128531)
                                basepoints0 /= 4;

                            caster->CastCustomSpell(target, triggered_spell_id, &basepoints0, NULL, NULL, true);
                        }
                        // Brewmaster : Training - you gain Shuffle, increasing parry chance and stagger amount by 20%
                        if (caster->GetTypeId() == TYPEID_PLAYER && caster->ToPlayer()->GetSpecializationId(caster->ToPlayer()->GetActiveSpec()) == SPEC_MONK_BREWMASTER)
                            caster->CastSpell(caster, SPELL_MONK_SHUFFLE, true);
                    }
                }
            }

            void Register()
            {
                AfterHit += SpellHitFn(spell_monk_blackout_kick_SpellScript::HandleAfterHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_blackout_kick_SpellScript();
        }
};

// Provoke - 115546
class spell_monk_provoke : public SpellScriptLoader
{
    public:
        spell_monk_provoke() : SpellScriptLoader("spell_monk_provoke") { }

        class spell_monk_provoke_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_provoke_SpellScript);

            SpellCastResult CheckCast()
            {
                Unit* target = GetExplTargetUnit();
                if (!target)
                    return SPELL_FAILED_NO_VALID_TARGETS;
                else if (target->GetTypeId() == TYPEID_PLAYER)
                    return SPELL_FAILED_BAD_TARGETS;
                else if (!target->IsWithinLOSInMap(GetCaster()))
                    return SPELL_FAILED_LINE_OF_SIGHT;
                return SPELL_CAST_OK;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                    if (caster->getClass() == CLASS_MONK && caster->GetTypeId() == TYPEID_PLAYER)
                        if (Unit* target = GetHitUnit())
                            caster->CastSpell(target, SPELL_MONK_PROVOKE, true);
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_monk_provoke_SpellScript::CheckCast);
                OnEffectHitTarget += SpellEffectFn(spell_monk_provoke_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_provoke_SpellScript();
        }
};

// Fortifying brew - 115203
class spell_monk_fortifying_brew : public SpellScriptLoader
{
    public:
        spell_monk_fortifying_brew() : SpellScriptLoader("spell_monk_fortifying_brew")
        {
            // Fortifying Brew - 120954
            SpellInfo* spellInfo = (SpellInfo*)sSpellMgr->GetSpellInfo(SPELL_MONK_FORTIFYING_BREW);
            if (spellInfo)
            {
                spellInfo->Effects[0].ApplyAuraName = SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT;
                spellInfo->Effects[0].BasePoints = 20;
            }
        }

        class spell_monk_fortifying_brew_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_fortifying_brew_SpellScript);

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (caster && caster->GetTypeId() == TYPEID_PLAYER)
                    caster->CastSpell(caster, SPELL_MONK_FORTIFYING_BREW, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_monk_fortifying_brew_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_fortifying_brew_SpellScript();
        }
};

// Roll - 109132 or Roll (3 charges) - 121827
class spell_monk_roll : public SpellScriptLoader
{
    public:
        spell_monk_roll() : SpellScriptLoader("spell_monk_roll") { }

        class spell_monk_roll_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_monk_roll_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_MONK_ROLL))
                    return false;
                return true;
            }

            void HandleBeforeCast()
            {
                Aura* aur = GetCaster()->AddAura(SPELL_MONK_ROLL_TRIGGER, GetCaster());
                if (!aur)
                    return;

                AuraApplication* app =  aur->GetApplicationOfTarget(GetCaster()->GetGUID());
                if (!app)
                    return;

                app->ClientUpdate();
            }

            void HandleAfterCast()
            {
                Unit* caster = GetCaster();
                if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                    return;

                caster->CastSpell(caster, SPELL_MONK_ROLL_TRIGGER, true);
            }

            void Register()
            {
                BeforeCast += SpellCastFn(spell_monk_roll_SpellScript::HandleBeforeCast);
                AfterCast += SpellCastFn(spell_monk_roll_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_monk_roll_SpellScript();
        }
};

// Brewing : Tigereye Brew - 123980
class spell_monk_tigereye_brew_stacks : public SpellScriptLoader
{
    public:
        spell_monk_tigereye_brew_stacks() : SpellScriptLoader("spell_monk_tigereye_brew_stacks") { }

        class spell_monk_tigereye_brew_stacks_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_monk_tigereye_brew_stacks_AuraScript);

            uint32 chiConsumed;

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                chiConsumed = 0;
            }

            void SetData(uint32 type, uint32 data)
            {
                while ((chiConsumed += data) >= 4)
                {
                    chiConsumed = 0;
                    data = data > 4 ? data - 4: 0;

                    if (GetCaster())
                        GetCaster()->CastSpell(GetCaster(), SPELL_MONK_TIGEREYE_BREW_STACKS, true);
                }
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_monk_tigereye_brew_stacks_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_monk_tigereye_brew_stacks_AuraScript();
        }
};

// 115636 - Mastery : Bottled Fury
class spell_mastery_bottled_fury : public SpellScriptLoader
{
public:
    spell_mastery_bottled_fury() : SpellScriptLoader("spell_mastery_bottled_fury") { }

    class spell_mastery_bottled_fury_SpellScript : public SpellScript
    {
        PrepareSpellScript(spell_mastery_bottled_fury_SpellScript);

        void HandleAfterHit()
        {
            if (Unit* caster = GetCaster())
            {
                if (Aura* WW_mastery = caster->GetAura(115636))
                {
                    int32 amount = WW_mastery->GetEffect(EFFECT_0)->GetAmount();

                    if (roll_chance_i(amount))
                        caster->AddAura(125195, caster);
                }
            }
        }

        void Register()
        {
            AfterHit += SpellHitFn(spell_mastery_bottled_fury_SpellScript::HandleAfterHit);
        }
    };

    SpellScript* GetSpellScript() const
    {
        return new spell_mastery_bottled_fury_SpellScript();
    }
};

class spell_monk_remove_zen_flight : public SpellScriptLoader
{
    public:
        spell_monk_remove_zen_flight() : SpellScriptLoader("spell_monk_remove_zen_flight") { }

        class spell_monk_remove_zen_flight_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_monk_remove_zen_flight_AuraScript);

            void OnRemove(AuraEffect const* aurEff, AuraEffectHandleModes /*mode*/)
            {
                Unit* target = GetTarget();
                if (!target)
                    return;

                Player* player = target->ToPlayer();
                if (!player)
                    return;

                AuraRemoveMode removeMode = GetTargetApplication()->GetRemoveMode();

                if (removeMode != AURA_REMOVE_BY_CANCEL)
                {
                    player->CastSpell(player, 54649, true);
                }
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_monk_remove_zen_flight_AuraScript::OnRemove, EFFECT_1, SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_monk_remove_zen_flight_AuraScript();
        }
};

void AddSC_monk_spell_scripts()
{
    new spell_monk_clone_cast();
    new spell_monk_storm_earth_and_fire_clone_visual();
    new spell_monk_storm_earth_and_fire();
    new spell_monk_fists_of_fury_stun();
    new spell_monk_renewing_mist();
    new spell_monk_diffuse_magic();
    new spell_monk_guard();
    new spell_monk_bear_hug();
    new spell_monk_zen_flight_check();
    new spell_monk_glyph_of_zen_flight();
    new spell_monk_power_strikes();
    new spell_monk_crackling_jade_lightning();
    new spell_monk_touch_of_karma();
    new spell_monk_spinning_fire_blossom_damage();
    new spell_monk_spinning_fire_blossom();
    new spell_monk_path_of_blossom();
    new spell_monk_thunder_focus_tea();
    new spell_monk_jade_serpent_statue();
    new spell_monk_teachings_of_the_monastery();
    new spell_monk_mana_tea();
    new spell_monk_mana_tea_stacks();
    new spell_monk_enveloping_mist();
    new spell_monk_surging_mist();
    new spell_monk_healing_elixirs();
    new spell_monk_zen_sphere();
    new spell_monk_zen_sphere_hot();
    new spell_monk_chi_burst();
    new spell_monk_energizing_brew();
    new spell_monk_spear_hand_strike();
    new spell_monk_tigers_lust();
    new spell_monk_flying_serpent_kick();
    new spell_monk_chi_torpedo();
    new spell_monk_purifying_brew();
    new spell_monk_clash();
    new spell_monk_keg_smash();
    new spell_monk_elusive_brew();
    new spell_monk_breath_of_fire();
    new spell_monk_soothing_mist();
    new spell_monk_disable();
    new spell_monk_zen_pilgrimage();
    new spell_monk_blackout_kick();
    new spell_monk_fortifying_brew();
    new spell_monk_provoke();
    new spell_monk_roll();
    new spell_monk_tigereye_brew_stacks();
    new spell_mastery_bottled_fury();
    new spell_monk_remove_zen_flight();
}
