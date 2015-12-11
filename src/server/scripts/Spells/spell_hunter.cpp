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
 * Scripts for spells with SPELLFAMILY_HUNTER, SPELLFAMILY_PET and SPELLFAMILY_GENERIC spells used by hunter players.
 * Ordered alphabetically using scriptname.
 * Scriptnames of files in this file should be prefixed with "spell_hun_".
 */

#include "ScriptMgr.h"
#include "Cell.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "SpellScript.h"
#include "SpellAuraEffects.h"

enum HunterSpells
{
    HUNTER_SPELL_READINESS                       = 23989,
    HUNTER_SPELL_BESTIAL_WRATH                   = 19574,
    HUNTER_PET_SPELL_LAST_STAND_TRIGGERED        = 53479,
    HUNTER_PET_HEART_OF_THE_PHOENIX              = 55709,
    HUNTER_PET_HEART_OF_THE_PHOENIX_TRIGGERED    = 54114,
    HUNTER_PET_HEART_OF_THE_PHOENIX_DEBUFF       = 55711,
    HUNTER_PET_SPELL_CARRION_FEEDER_TRIGGERED    = 54045,
    HUNTER_SPELL_POSTHASTE                       = 109215,
    HUNTER_SPELL_POSTHASTE_INCREASE_SPEED        = 118922,
    HUNTER_SPELL_NARROW_ESCAPE                   = 109298,
    HUNTER_SPELL_NARROW_ESCAPE_RETS              = 128405,
    HUNTER_SPELL_SERPENT_STING                   = 118253,
    HUNTER_SPELL_SERPENT_SPREAD                  = 87935,
    HUNTER_SPELL_CHIMERA_SHOT_HEAL               = 53353,
    HUNTER_SPELL_RAPID_INTENSITY                 = 131564,
    HUNTER_SPELL_RAPID_FIRE                      = 3045,
    HUNTER_SPELL_STEADY_SHOT_ENERGIZE            = 77443,
    HUNTER_SPELL_COBRA_SHOT_ENERGIZE             = 91954,
    HUNTER_SPELL_KILL_COMMAND                    = 34026,
    HUNTER_SPELL_KILL_COMMAND_TRIGGER            = 83381,
    HUNTER_SPELL_KILL_COMMAND_CHARGE             = 118171,
    SPELL_MAGE_TEMPORAL_DISPLACEMENT             = 80354,
    HUNTER_SPELL_INSANITY                        = 95809,
    SPELL_SHAMAN_SATED                           = 57724,
    SPELL_SHAMAN_EXHAUSTED                       = 57723,
    HUNTER_SPELL_CAMOUFLAGE_VISUAL               = 80326,
    HUNTER_SPELL_GLYPH_OF_CAMOUFLAGE_VISUAL      = 119450,
    HUNTER_SPELL_IMPROVED_SERPENT_STING_AURA     = 82834,
    HUNTER_SPELL_IMPROVED_SERPENT_STING          = 83077,
    HUNTER_SPELL_GLAIVE_TOSS_DAMAGES             = 121414,
    HUNTER_SPELL_GLAIVE_TOSS                     = 117050,
    HUNTER_SPELL_BINDING_SHOT_AREA               = 109248,
    HUNTER_SPELL_BINDING_SHOT_LINK               = 117405,
    HUNTER_SPELL_BINDING_SHOT_STUN               = 117526,
    HUNTER_SPELL_BINDING_SHOT_IMMUNE             = 117553,
    HUNTER_SPELL_PIERCIG_SHOTS                   = 53238,
    HUNTER_SPELL_PIERCIG_SHOTS_EFFECT            = 63468,
    HUNTER_SPELL_MASTERS_CALL_TRIGGERED          = 62305,
    HUNTER_SPELL_COBRA_STRIKES_AURA              = 53260,
    HUNTER_SPELL_COBRA_STRIKES_STACKS            = 53257,
    HUNTER_SPELL_BEAST_CLEAVE_AURA               = 115939,
    HUNTER_SPELL_BEAST_CLEAVE_PROC               = 118455,
    HUNTER_SPELL_BEAST_CLEAVE_DAMAGE             = 118459,
    HUNTER_SPELL_LYNX_RUSH_AURA                  = 120697,
    HUNTER_SPELL_LYNX_CRUSH_DAMAGE               = 120699,
    HUNTER_SPELL_KINDRED_SPIRIT_FOR_PET          = 88680,
    HUNTER_SPELL_FRENZY_STACKS                   = 19615,
    HUNTER_SPELL_FOCUS_FIRE_READY                = 88843,
    HUNTER_SPELL_FOCUS_FIRE_AURA                 = 82692,
    HUNTER_SPELL_A_MURDER_OF_CROWS_SUMMON        = 131900,
    HUNTER_NPC_MURDER_OF_CROWS                   = 61994,
    HUNTER_SPELL_DIRE_BEAST                      = 120679,
    DIRE_BEAST_JADE_FOREST                       = 121118,
    DIRE_BEAST_KALIMDOR                          = 122802,
    DIRE_BEAST_EASTERN_KINGDOMS                  = 122804,
    DIRE_BEAST_OUTLAND                           = 122806,
    DIRE_BEAST_NORTHREND                         = 122807,
    DIRE_BEAST_KRASARANG_WILDS                   = 122809,
    DIRE_BEAST_VALLEY_OF_THE_FOUR_WINDS          = 122811,
    DIRE_BEAST_VALE_OF_THE_ETERNAL_BLOSSOM       = 126213,
    DIRE_BEAST_KUN_LAI_SUMMIT                    = 126214,
    DIRE_BEAST_TOWNLONG_STEPPES                  = 126215,
    DIRE_BEAST_DREAD_WASTES                      = 126216,
    DIRE_BEAST_DUNGEONS                          = 132764,
    HUNTER_SPELL_STAMPEDE_DAMAGE_REDUCTION       = 130201,
    HUNTER_SPELL_GLYPH_OF_STAMPEDE               = 57902,
    HUNTER_SPELL_HUNTERS_MARK                    = 1130,
    HUNTER_SPELL_GLYPH_OF_MISDIRECTION           = 56829,
    HUNTER_SPELL_MISDIRECTION                    = 34477,
    HUNTER_SPELL_MISDIRECTION_PROC               = 35079,
    HUNTER_SPELL_BLINK_STRIKE                    = 130393,
    HUNTER_SPELL_GLYPH_OF_DIRECTION              = 126179,
    HUNTER_SPELL_GLYPH_OF_EXPLOSIVE_TRAP         = 119403,
    HUNTER_SPELL_HUN_THRILL_OF_THE_HUNT          = 34720,
    SPELL_DRUMS_OF_RAGE                          = 146555,
    HUNTER_SPELL_T16_2P_BONUS                    = 144637,
    HUNTER_SPELL_STEADY_FOCUS                    = 53220,
    HUNTER_SPELL_ICE_TRAP_TRIGGER                = 13810,
};

// Dash - 113073
class spell_hun_dash : public SpellScriptLoader
{
    public:
        spell_hun_dash() : SpellScriptLoader("spell_hun_dash") { }

        class spell_hun_dash_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_dash_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    _player->RemoveMovementImpairingAuras();
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_hun_dash_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_dash_SpellScript();
        }
};

// Stampede - 121818
#define STAMPED_COUNT 5
class spell_hun_stampede : public SpellScriptLoader
{
    public:
        spell_hun_stampede() : SpellScriptLoader("spell_hun_stampede") { }

        class spell_hun_stampede_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_stampede_SpellScript);

            void HandleOnHit(SpellEffIndex effIndex)
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    Unit* target = GetExplTargetUnit();
                    WorldLocation* loc = GetHitDest();
                    Pet* curentPet = _player->GetPet();
                    PetSlot saveSlot = _player->m_currentSummonedSlot;

                    if (target)
                    {
                        float x, y, z, o;
                        target->GetPosition(x, y, z);
                        uint32 count = 0;
                        PetSlot currentSlot = _player->GetSlotForPetId(_player->m_currentPetNumber);
                        GetCaster()->SetInCombatWith(target);

                        if(curentPet)
                            count++;
                        else
                        {
                            for (uint32 i = uint32(PET_SLOT_HUNTER_FIRST); i <= uint32(PET_SLOT_HUNTER_LAST); ++i)
                            {
                                if (_player->getPetIdBySlot(i))
                                {
                                    currentSlot = PetSlot(i);
                                    break;
                                }
                            }
                        }

                        bool gliph = _player->HasAura(HUNTER_SPELL_GLYPH_OF_STAMPEDE);

                        for (uint32 i = uint32(PET_SLOT_HUNTER_FIRST); i <= uint32(PET_SLOT_HUNTER_LAST); ++i)
                        {
                            if (gliph)
                                _player->m_currentSummonedSlot = currentSlot;
                            else if (_player->getPetIdBySlot(i))
                                _player->m_currentSummonedSlot = PetSlot(i);
                            else if (count < STAMPED_COUNT)
                                _player->m_currentSummonedSlot = currentSlot;
                            else
                                continue;

                            if(curentPet && curentPet->GetSlot() == i && !gliph)
                                continue;

                            o = _player->GetOrientation() + PET_FOLLOW_ANGLE + (((PET_FOLLOW_ANGLE * 2) / 5) * count);

                            Position pos;
                            target->GetFirstCollisionPosition(pos, MIN_MELEE_REACH, o);

                            if(Pet* pet = _player->SummonPet(0, pos.m_positionX, pos.m_positionY, pos.m_positionZ, o, SUMMON_PET, _player->CalcSpellDuration(GetSpellInfo()), GetSpellInfo()->Id, true))
                            {
                                pet->SetReactState(REACT_AGGRESSIVE);
                                    
                                if (pet->GetMap()->IsBattlegroundOrArena())
                                    pet->CastSpell(pet, HUNTER_SPELL_STAMPEDE_DAMAGE_REDUCTION, true);

                                if (pet->IsAIEnabled)
                                    pet->AI()->AttackStart(target);

                                GetSpell()->ExecuteLogEffectGeneric(effIndex, pet->GetGUID());

                                ++count;
                                if (count >= STAMPED_COUNT)
                                    break;
                            }
                        }
                    }
                    _player->m_currentSummonedSlot = saveSlot;
                }
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_hun_stampede_SpellScript::HandleOnHit, EFFECT_0, SPELL_EFFECT_STAMPEDE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_stampede_SpellScript();
        }
};

// Dire Beast - 120679
class spell_hun_dire_beast : public SpellScriptLoader
{
    public:
        spell_hun_dire_beast() : SpellScriptLoader("spell_hun_dire_beast") { }

        class spell_hun_dire_beast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_dire_beast_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        // Summon's skin is different function of Map or Zone ID
                        switch (_player->GetZoneId())
                        {
                            case 5785: // The Jade Forest
                                _player->CastSpell(target, DIRE_BEAST_JADE_FOREST, true);
                                break;
                            case 5805: // Valley of the Four Winds
                                _player->CastSpell(target, DIRE_BEAST_VALLEY_OF_THE_FOUR_WINDS, true);
                                break;
                            case 5840: // Vale of Eternal Blossoms
                                _player->CastSpell(target, DIRE_BEAST_VALE_OF_THE_ETERNAL_BLOSSOM, true);
                                break;
                            case 5841: // Kun-Lai Summit
                                _player->CastSpell(target, DIRE_BEAST_KUN_LAI_SUMMIT, true);
                                break;
                            case 5842: // Townlong Steppes
                                _player->CastSpell(target, DIRE_BEAST_TOWNLONG_STEPPES, true);
                                break;
                            case 6134: // Krasarang Wilds
                                _player->CastSpell(target, DIRE_BEAST_KRASARANG_WILDS, true);
                                break;
                            case 6138: // Dread Wastes
                                _player->CastSpell(target, DIRE_BEAST_DREAD_WASTES, true);
                                break;
                            default:
                            {
                                switch (_player->GetMapId())
                                {
                                    case 0: // Eastern Kingdoms
                                        _player->CastSpell(target, DIRE_BEAST_EASTERN_KINGDOMS, true);
                                        break;
                                    case 1: // Kalimdor
                                        _player->CastSpell(target, DIRE_BEAST_KALIMDOR, true);
                                        break;
                                    case 8: // Outland
                                        _player->CastSpell(target, DIRE_BEAST_OUTLAND, true);
                                        break;
                                    case 10: // Northrend
                                        _player->CastSpell(target, DIRE_BEAST_NORTHREND, true);
                                        break;
                                    default:
                                        _player->CastSpell(target, DIRE_BEAST_DUNGEONS, true);
                                        break;
                                }
                                break;
                            }
                        }
                    }
                }
            }

            void Register()
            {
               OnHit += SpellHitFn(spell_hun_dire_beast_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_dire_beast_SpellScript();
        }
};

// A Murder of Crows - 131894
class spell_hun_a_murder_of_crows : public SpellScriptLoader
{
    public:
        spell_hun_a_murder_of_crows() : SpellScriptLoader("spell_hun_a_murder_of_crows") { }

        class spell_hun_a_murder_of_crows_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_a_murder_of_crows_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if (Unit* target = GetTarget())
                {
                    if (Unit* caster = GetCaster())
                    {
                        caster->CastSpell(target, HUNTER_SPELL_A_MURDER_OF_CROWS_SUMMON, true);
                        target->CastSpell(target, 131637, true);
                        target->CastSpell(target, 131951, true);
                        target->CastSpell(target, 131952, true);
                    }
                }
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Unit* target = GetTarget())
                    if(target->GetHealthPct() < 20.0f && GetCaster())
                        if (Player* caster = GetCaster()->ToPlayer())
                        {
                            double cd = 60000.0f - (GetAura()->GetMaxDuration() - GetAura()->GetDuration());
                            caster->ModifySpellCooldown(GetSpellInfo()->Id, -cd);
                        }
            }

            void Register()
            {
                OnEffectRemove += AuraEffectRemoveFn(spell_hun_a_murder_of_crows_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_hun_a_murder_of_crows_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hun_a_murder_of_crows_AuraScript();
        }
};

// Focus Fire - 82692
class spell_hun_focus_fire : public SpellScriptLoader
{
    public:
        spell_hun_focus_fire() : SpellScriptLoader("spell_hun_focus_fire") { }

        class spell_hun_focus_fire_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_focus_fire_SpellScript);

            SpellCastResult CheckFrenzy()
            {
                if (!GetCaster()->HasAura(HUNTER_SPELL_FRENZY_STACKS))
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;

                return SPELL_CAST_OK;
            }

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Aura* focusFire = _player->GetAura(HUNTER_SPELL_FOCUS_FIRE_AURA))
                    {
                        if (Aura* frenzy = _player->GetAura(HUNTER_SPELL_FRENZY_STACKS))
                        {
                            if (Pet* pet = _player->GetPet())
                            {
                                int32 stackAmount = frenzy->GetStackAmount();

                                focusFire->GetEffect(0)->ChangeAmount(focusFire->GetEffect(0)->GetAmount() * stackAmount);

                                if (pet->HasAura(HUNTER_SPELL_FRENZY_STACKS))
                                {
                                    pet->RemoveAura(HUNTER_SPELL_FRENZY_STACKS);
                                    pet->EnergizeBySpell(pet, GetSpellInfo()->Id, 6 * stackAmount, POWER_FOCUS);
                                }

                                _player->RemoveAura(HUNTER_SPELL_FRENZY_STACKS);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_hun_focus_fire_SpellScript::CheckFrenzy);
                OnHit += SpellHitFn(spell_hun_focus_fire_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_focus_fire_SpellScript();
        }
};

// Frenzy - 19615
class spell_hun_frenzy : public SpellScriptLoader
{
    public:
        spell_hun_frenzy() : SpellScriptLoader("spell_hun_frenzy") { }

        class spell_hun_frenzy_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_frenzy_SpellScript);

            void HandleOnHit()
            {
                if (Unit* caster = GetCaster())
                    if (caster->GetOwner())
                        if (Aura* frenzy = caster->GetAura(HUNTER_SPELL_FRENZY_STACKS))
                            if (frenzy->GetStackAmount() >= 5)
                                caster->GetOwner()->CastSpell(caster->GetOwner(), HUNTER_SPELL_FOCUS_FIRE_READY, true);
            }

            void Register()
            {
               OnHit += SpellHitFn(spell_hun_frenzy_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_frenzy_SpellScript();
        }

        class spell_hun_frenzy_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_frenzy_AuraScript);

            void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes mode)
            {
                if (GetTarget()->GetOwner())
                    if (GetTarget()->GetOwner()->HasAura(HUNTER_SPELL_FOCUS_FIRE_READY))
                        GetTarget()->GetOwner()->RemoveAura(HUNTER_SPELL_FOCUS_FIRE_READY);
            }

            void Register()
            {
                OnEffectRemove += AuraEffectApplyFn(spell_hun_frenzy_AuraScript::HandleRemove, EFFECT_0, SPELL_AURA_MOD_MELEE_HASTE_3, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hun_frenzy_AuraScript();
        }
};

// Lynx Rush - 120697
class spell_hun_lynx_rush : public SpellScriptLoader
{
    public:
        spell_hun_lynx_rush() : SpellScriptLoader("spell_hun_lynx_rush") { }

        class spell_hun_lynx_rush_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_lynx_rush_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                std::list<Unit*> tempList;
                std::list<Unit*> targetList;
                Unit* unitTarget = NULL;

                GetTarget()->GetAttackableUnitListInRange(tempList, 10.0f);

                for (std::list<Unit*>::const_iterator itr = tempList.begin(); itr != tempList.end(); ++itr)
                {
                    if ((*itr)->GetGUID() == GetTarget()->GetGUID())
                        continue;

                    if (GetTarget()->GetOwner() && GetTarget()->GetOwner()->GetGUID() == (*itr)->GetGUID())
                        continue;

                    if (!GetTarget()->IsValidAttackTarget(*itr))
                        continue;

                    targetList.push_back(*itr);
                }

                tempList.clear();

                if (targetList.empty())
                    return;

                Trinity::Containers::RandomResizeList(targetList, 1);

                for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                {
                    unitTarget = *itr;
                    break;
                }

                if (!unitTarget)
                    return;

                float angle = unitTarget->GetRelativeAngle(GetTarget());
                Position pos;

                unitTarget->GetContactPoint(GetTarget(), pos.m_positionX, pos.m_positionY, pos.m_positionZ);
                unitTarget->GetFirstCollisionPosition(pos, unitTarget->GetObjectSize(), angle);
                GetTarget()->GetMotionMaster()->MoveCharge(pos.m_positionX, pos.m_positionY, pos.m_positionZ + unitTarget->GetObjectSize());

                GetTarget()->CastSpell(unitTarget, HUNTER_SPELL_LYNX_CRUSH_DAMAGE, true);
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_hun_lynx_rush_AuraScript::OnTick, EFFECT_0, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hun_lynx_rush_AuraScript();
        }

        class spell_hun_lynx_rush_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_lynx_rush_SpellScript);

            SpellCastResult CheckCastMeet()
            {
                Unit* caster = GetCaster();
                Unit* pet = caster->GetGuardianPet();
                
                if (pet->HasUnitState(UNIT_STATE_CONTROLLED) || pet->HasUnitState(UNIT_STATE_ROOT))
                {
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                }
                
                return SPELL_CAST_OK;
            }
            
            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (GetHitUnit())
                    {
                        if (Pet* pet = _player->GetPet())
                        {
                            if (pet->GetGUID() == GetHitUnit()->GetGUID())
                            {
                                std::list<Unit*> tempList;
                                std::list<Unit*> targetList;
                                Unit* unitTarget = NULL;

                                pet->GetAttackableUnitListInRange(tempList, 10.0f);

                                for (std::list<Unit*>::const_iterator itr = tempList.begin(); itr != tempList.end(); ++itr)
                                {
                                    if ((*itr)->GetGUID() == pet->GetGUID())
                                        continue;

                                    if (_player->GetGUID() == (*itr)->GetGUID())
                                        continue;

                                    if (!pet->IsValidAttackTarget(*itr))
                                        continue;

                                    targetList.push_back(*itr);
                                }

                                tempList.clear();

                                if (targetList.empty())
                                    return;

                                Trinity::Containers::RandomResizeList(targetList, 1);

                                for (std::list<Unit*>::const_iterator itr = targetList.begin(); itr != targetList.end(); ++itr)
                                {
                                    unitTarget = *itr;
                                    break;
                                }

                                if (!unitTarget)
                                    return;

                                float angle = unitTarget->GetRelativeAngle(pet);
                                Position pos;

                                unitTarget->GetContactPoint(pet, pos.m_positionX, pos.m_positionY, pos.m_positionZ);
                                unitTarget->GetFirstCollisionPosition(pos, unitTarget->GetObjectSize(), angle);
                                pet->GetMotionMaster()->MoveCharge(pos.m_positionX, pos.m_positionY, pos.m_positionZ + unitTarget->GetObjectSize());

                                pet->CastSpell(unitTarget, HUNTER_SPELL_LYNX_CRUSH_DAMAGE, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_hun_lynx_rush_SpellScript::CheckCastMeet);
                OnHit += SpellHitFn(spell_hun_lynx_rush_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_lynx_rush_SpellScript();
        }
};

// Called by Multi Shot - 2643
// Beast Cleave - 115939
class spell_hun_beast_cleave : public SpellScriptLoader
{
    public:
        spell_hun_beast_cleave() : SpellScriptLoader("spell_hun_beast_cleave") { }

        class spell_hun_beast_cleave_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_beast_cleave_SpellScript);

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (_player->HasAura(HUNTER_SPELL_BEAST_CLEAVE_AURA))
                        if (Pet* pet = _player->GetPet())
                            _player->CastSpell(pet, HUNTER_SPELL_BEAST_CLEAVE_PROC, true);
            }

            void Register()
            {
               AfterCast += SpellCastFn(spell_hun_beast_cleave_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_beast_cleave_SpellScript();
        }
};

// Called by Arcane Shot - 3044
// Cobra Strikes - 53260
class spell_hun_cobra_strikes : public SpellScriptLoader
{
    public:
        spell_hun_cobra_strikes() : SpellScriptLoader("spell_hun_cobra_strikes") { }

        class spell_hun_cobra_strikes_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_cobra_strikes_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_HUNTER_BEASTMASTER)
                    {    
                        if (Unit* target = GetHitUnit())
                        {
                            if (roll_chance_i(15))
                            {
                                _player->CastSpell(_player, HUNTER_SPELL_COBRA_STRIKES_STACKS, true);
                                _player->CastSpell(_player, HUNTER_SPELL_COBRA_STRIKES_STACKS, true);
                            }
                        }
                    }
                }
            }

            void Register()
            {
               OnHit += SpellHitFn(spell_hun_cobra_strikes_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_cobra_strikes_SpellScript();
        }
};

// Barrage damage - 120361
class spell_hun_barrage : public SpellScriptLoader
{
    public:
        spell_hun_barrage() : SpellScriptLoader("spell_hun_barrage") { }

        class spell_hun_barrage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_barrage_SpellScript);

            void HandleOnHit()
            {
                if (Unit* target = GetHitUnit())
                    if (!target->HasAura(120360))
                        SetHitDamage(GetHitDamage() / 2);
            }

            void Register()
            {
               OnHit += SpellHitFn(spell_hun_barrage_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_barrage_SpellScript();
        }
};

// Binding Shot - 117405
class spell_hun_binding_shot : public SpellScriptLoader
{
    public:
        spell_hun_binding_shot() : SpellScriptLoader("spell_hun_binding_shot") { }

        class spell_hun_binding_shot_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_binding_shot_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                targets.remove_if(Trinity::UnitAuraCheck(true, HUNTER_SPELL_BINDING_SHOT_IMMUNE));
                targets.remove(GetCaster());

                if (targets.empty())
                    return;

                std::list<uint64> targetList;
                for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                    targetList.push_back((*itr)->GetGUID());

                if(Unit* caster = GetCaster())
                    if (Aura* aura = caster->GetAura(109248))
                        aura->SetEffectTargets(targetList);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hun_binding_shot_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_binding_shot_SpellScript();
        }
};

// Binding Shot - 109248
class spell_hun_binding_shot_zone : public SpellScriptLoader
{
    public:
        spell_hun_binding_shot_zone() : SpellScriptLoader("spell_hun_binding_shot_zone") { }

        class spell_hun_binding_shot_zone_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_binding_shot_zone_AuraScript);

            void OnTick(AuraEffect const* aurEff)
            {
                if(Unit* caster = GetCaster())
                {
                    if (DynamicObject* dynObj = caster->GetDynObject(HUNTER_SPELL_BINDING_SHOT_AREA))
                    {
                        std::list<uint64> targets = GetAura()->GetEffectTargets();
                        if(!targets.empty())
                        {
                            for (std::list<uint64>::iterator itr = targets.begin(); itr != targets.end();)
                            {
                                Unit* unit = ObjectAccessor::GetUnit(*caster, (*itr));
                                if (unit && unit->IsInWorld() && unit->GetDistance(dynObj) > 5.0f)
                                {
                                    unit->CastSpell(unit, HUNTER_SPELL_BINDING_SHOT_STUN, true);
                                    unit->CastSpell(unit, HUNTER_SPELL_BINDING_SHOT_IMMUNE, true);
                                    targets.erase(itr++);
                                }
                                else
                                    ++itr;
                            }
                            GetAura()->SetEffectTargets(targets);
                        }

                        caster->CastSpell(dynObj->GetPositionX(), dynObj->GetPositionY(), dynObj->GetPositionZ(), HUNTER_SPELL_BINDING_SHOT_LINK, true);
                    }
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_hun_binding_shot_zone_AuraScript::OnTick, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        class spell_hun_binding_shot_zone_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_binding_shot_zone_SpellScript);

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                std::list<uint64> targetList;
                for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end(); ++itr)
                    targetList.push_back((*itr)->GetGUID());

                GetSpell()->SetEffectTargets(targetList);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hun_binding_shot_zone_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_binding_shot_zone_SpellScript();
        }

        AuraScript* GetAuraScript() const
        {
            return new spell_hun_binding_shot_zone_AuraScript();
        }
};

// Called by Serpent Sting - 118253
// Improved Serpent Sting - 82834
class spell_hun_improved_serpent_sting : public SpellScriptLoader
{
    public:
        spell_hun_improved_serpent_sting() : SpellScriptLoader("spell_hun_improved_serpent_sting") { }

        class spell_hun_improved_serpent_sting_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_improved_serpent_sting_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (Aura* aura = _player->GetAura(HUNTER_SPELL_IMPROVED_SERPENT_STING_AURA))
                        {
                            uint16 pct = 0;
                            if (AuraEffect* aeff = aura->GetEffect(EFFECT_0))
                                pct = aeff->GetAmount();

                            if (Aura* serpentSting = target->GetAura(HUNTER_SPELL_SERPENT_STING, _player->GetGUID()))
                                if (AuraEffect* aeff = serpentSting->GetEffect(EFFECT_0))
                                {
                                    int32 bp = aeff->GetAmount();
                                    bp *= aeff->GetTotalTicks();
                                    bp = CalculatePct(bp, pct);

                                    _player->CastCustomSpell(target, HUNTER_SPELL_IMPROVED_SERPENT_STING, &bp, NULL, NULL, true);
                                }
                        }
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_hun_improved_serpent_sting_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_improved_serpent_sting_SpellScript();
        }
};

// Camouflage - 51755
class spell_hun_camouflage_visual : public SpellScriptLoader
{
    public:
        spell_hun_camouflage_visual() : SpellScriptLoader("spell_hun_camouflage_visual") { }

        class spell_hun_camouflage_visual_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_camouflage_visual_AuraScript);

            void HandleEffectRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetCaster())
                {
                    GetCaster()->RemoveAura(HUNTER_SPELL_CAMOUFLAGE_VISUAL);
                    GetCaster()->RemoveAura(HUNTER_SPELL_GLYPH_OF_CAMOUFLAGE_VISUAL);
                }
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_hun_camouflage_visual_AuraScript::HandleEffectRemove, EFFECT_0, SPELL_AURA_INTERFERE_TARGETTING, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hun_camouflage_visual_AuraScript();
        }
};

// Called by Multi Shot - 2643
// Serpent Spread - 87935
class spell_hun_serpent_spread : public SpellScriptLoader
{
    public:
        spell_hun_serpent_spread() : SpellScriptLoader("spell_hun_serpent_spread") { }

        class spell_hun_serpent_spread_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_serpent_spread_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                    if (Unit* target = GetHitUnit())
                        if (_player->HasAura(HUNTER_SPELL_SERPENT_SPREAD))
                            _player->CastSpell(target, HUNTER_SPELL_SERPENT_STING, true);
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_hun_serpent_spread_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_serpent_spread_SpellScript();
        }
};

// Ancient Hysteria - 90355
class spell_hun_ancient_hysteria : public SpellScriptLoader
{
    public:
        spell_hun_ancient_hysteria() : SpellScriptLoader("spell_hun_ancient_hysteria") { }

        class spell_hun_ancient_hysteria_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_ancient_hysteria_SpellScript);

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
                    target->CastSpell(target, HUNTER_SPELL_INSANITY, true);
            }

            void Register()
            {
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hun_ancient_hysteria_SpellScript::RemoveInvalidTargets, EFFECT_0, TARGET_UNIT_CASTER_AREA_RAID);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hun_ancient_hysteria_SpellScript::RemoveInvalidTargets, EFFECT_1, TARGET_UNIT_CASTER_AREA_RAID);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hun_ancient_hysteria_SpellScript::RemoveInvalidTargets, EFFECT_2, TARGET_UNIT_CASTER_AREA_RAID);
                AfterHit += SpellHitFn(spell_hun_ancient_hysteria_SpellScript::ApplyDebuff);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_ancient_hysteria_SpellScript();
        }
};

// Kill Command - 34026
class spell_hun_kill_command : public SpellScriptLoader
{
    public:
        spell_hun_kill_command() : SpellScriptLoader("spell_hun_kill_command") { }

        class spell_hun_kill_command_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_kill_command_SpellScript);

            bool Validate(SpellInfo const* /*SpellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(HUNTER_SPELL_KILL_COMMAND))
                    return false;
                return true;
            }

            void FilterTargets(WorldObject*& target)
            {
                if (Unit* caster = GetCaster())
                    if (Unit* pet = caster->GetGuardianPet())
                        if (pet->getVictim())
                            target = pet->getVictim();
            }

            SpellCastResult CheckCastMeet()
            {
                Unit* caster = GetCaster();
                if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_NO_PET;

                Player* player = caster->ToPlayer();
                Unit* pet = caster->GetGuardianPet();
                Unit* target = player->GetSelectedUnit();

                if (!pet || pet->isDead())
                    return SPELL_FAILED_NO_PET;

                // pet has a target and target is within 5 yards
                if (!target || !pet->IsWithinDist(target, 25.0f, true))
                {
                    SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_TARGET_TOO_FAR);
                    return SPELL_FAILED_CUSTOM_ERROR;
                }
                
                if (pet->HasUnitState(UNIT_STATE_CONTROLLED))
                {
                    return SPELL_FAILED_CANT_DO_THAT_RIGHT_NOW;
                }
                
                return SPELL_CAST_OK;
            }
            
            SpellCastResult CheckIfPetInLOS()
            {
                Unit* caster = GetCaster();
                if (Player* player = GetCaster()->ToPlayer())
                {
                    if (Unit* pet = GetCaster()->GetGuardianPet())
                    {
                        float x, y, z;
                        pet->GetPosition(x, y, z);
                        
                        if(Unit* target = player->GetSelectedUnit())
                           if (target->IsWithinLOS(x, y, z))
                               return SPELL_CAST_OK;
                    }
                }
                return SPELL_FAILED_LINE_OF_SIGHT;
            }
            
            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                    if (Unit* pet = caster->GetGuardianPet())
                        if (Unit* target = GetHitUnit())
                        {
                            pet->CastSpell(target, HUNTER_SPELL_KILL_COMMAND_TRIGGER, false);

                            if (!pet->IsWithinMeleeRange(target))
                                pet->CastSpell(target, HUNTER_SPELL_KILL_COMMAND_CHARGE, false);

                            caster->AddAura(HUNTER_SPELL_HUNTERS_MARK, target);
                        }
            }

            void Register()
            {
                OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_hun_kill_command_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_TARGET_ENEMY);
                OnCheckCast += SpellCheckCastFn(spell_hun_kill_command_SpellScript::CheckCastMeet);
                OnCheckCast += SpellCheckCastFn(spell_hun_kill_command_SpellScript::CheckIfPetInLOS);
                OnEffectHitTarget += SpellEffectFn(spell_hun_kill_command_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_kill_command_SpellScript();
        }
};

// Cobra Shot - 77767
class spell_hun_cobra_shot : public SpellScriptLoader
{
    public:
        spell_hun_cobra_shot() : SpellScriptLoader("spell_hun_cobra_shot") { }

        class spell_hun_cobra_shot_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_cobra_shot_SpellScript);

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        if (AuraEffect* aurEff = target->GetAuraEffect(SPELL_AURA_PERIODIC_DAMAGE, SPELLFAMILY_HUNTER, 16384, 0, 0, GetCaster()->GetGUID()))
                        {
                            Aura* serpentSting = aurEff->GetBase();
                            serpentSting->SetDuration(serpentSting->GetDuration() + (GetSpellInfo()->Effects[EFFECT_2].BasePoints * 1000));

                            if (serpentSting->GetMaxDuration() < serpentSting->GetDuration())
                                serpentSting->RefreshTimers();
                        }
                    }
                }
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_hun_cobra_shot_SpellScript::HandleScriptEffect, EFFECT_2, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_cobra_shot_SpellScript();
        }
};

// Steady Shot - 56641
class spell_hun_steady_shot : public SpellScriptLoader
{
    public:
        spell_hun_steady_shot() : SpellScriptLoader("spell_hun_steady_shot") { }

        class spell_hun_steady_shot_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_steady_shot_SpellScript);

            void HandleOnCast()
            {
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(HUNTER_SPELL_STEADY_SHOT_ENERGIZE);
                int32 basepoints0 = spellInfo->Effects[EFFECT_0].BasePoints;
                
                if (Unit* caster = GetCaster())
                {
                    // Steady Focus
                    if (AuraEffect * steadyfocus = caster->GetAuraEffect(HUNTER_SPELL_STEADY_FOCUS, 1))
                        basepoints0 += steadyfocus->GetAmount();

                    caster->CastCustomSpell(caster, spellInfo->Id, &basepoints0, NULL, NULL, true);
                }
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit *caster = GetCaster())
                    caster->ToPlayer()->KilledMonsterCredit(44175, 0);
            }

            void Register()
            {
                OnCast += SpellCastFn(spell_hun_steady_shot_SpellScript::HandleOnCast);
                OnEffectHit += SpellEffectFn(spell_hun_steady_shot_SpellScript::HandleDummy, EFFECT_2, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_steady_shot_SpellScript();
        }
};

// Chimera Shot - 53209
class spell_hun_chimera_shot : public SpellScriptLoader
{
    public:
        spell_hun_chimera_shot() : SpellScriptLoader("spell_hun_chimera_shot") { }

        class spell_hun_chimera_shot_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_chimera_shot_SpellScript);

            void HandleOnHit()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Unit* target = GetHitUnit())
                    {
                        AuraEffect const* serpentSting = target->GetAuraEffect(HUNTER_SPELL_SERPENT_STING, EFFECT_0, _player->GetGUID());

                        if (serpentSting)
                            serpentSting->GetBase()->RefreshDuration();

                        _player->CastSpell(_player, HUNTER_SPELL_CHIMERA_SHOT_HEAL, true);
                    }
                }
            }

            void Register()
            {
                OnHit += SpellHitFn(spell_hun_chimera_shot_SpellScript::HandleOnHit);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_chimera_shot_SpellScript();
        }
};

class spell_hun_last_stand_pet : public SpellScriptLoader
{
    public:
        spell_hun_last_stand_pet() : SpellScriptLoader("spell_hun_last_stand_pet") { }

        class spell_hun_last_stand_pet_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_last_stand_pet_SpellScript);

            bool Validate(SpellInfo const* /*SpellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(HUNTER_PET_SPELL_LAST_STAND_TRIGGERED))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                int32 healthModSpellBasePoints0 = int32(caster->CountPctFromMaxHealth(30));
                caster->CastCustomSpell(caster, HUNTER_PET_SPELL_LAST_STAND_TRIGGERED, &healthModSpellBasePoints0, NULL, NULL, true, NULL);
            }

            void Register()
            {
                // add dummy effect spell handler to pet's Last Stand
                OnEffectHitTarget += SpellEffectFn(spell_hun_last_stand_pet_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_last_stand_pet_SpellScript();
        }
};

// Master's Call - 53271
class spell_hun_masters_call : public SpellScriptLoader
{
    public:
        spell_hun_masters_call() : SpellScriptLoader("spell_hun_masters_call") { }

        class spell_hun_masters_call_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_masters_call_SpellScript);

            bool Validate(SpellInfo const* SpellInfo)
            {
                if (!sSpellMgr->GetSpellInfo(HUNTER_SPELL_MASTERS_CALL_TRIGGERED))
                    return false;
                return true;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                if (Unit* ally = GetHitUnit())
                {
                    if (Player* caster = GetCaster()->ToPlayer())
                        if (Pet* target = caster->GetPet())
                        {
                            TriggerCastFlags castMask = TriggerCastFlags(TRIGGERED_FULL_MASK & ~TRIGGERED_IGNORE_CASTER_AURASTATE);
                            target->CastSpell(target, HUNTER_SPELL_MASTERS_CALL_TRIGGERED, castMask);
                            target->CastSpell(ally, GetEffectValue(), castMask);
                        }
                }
            }
            
            SpellCastResult CheckIfPetInLOS()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Pet* pet = _player->GetPet())
                    {
                        if (pet->isDead())
                            return SPELL_FAILED_NO_PET;

                        float x, y, z;
                        pet->GetPosition(x, y, z);

                        if (_player->IsWithinLOS(x, y, z))
                            return SPELL_CAST_OK;
                    }
                }
                return SPELL_FAILED_LINE_OF_SIGHT;
            }
            
            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_hun_masters_call_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
                OnCheckCast += SpellCheckCastFn(spell_hun_masters_call_SpellScript::CheckIfPetInLOS);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_masters_call_SpellScript();
        }
};

// 53302, 53303, 53304 Sniper Training
enum eSniperTrainingSpells
{
    SPELL_SNIPER_TRAINING_R1        = 53302,
    SPELL_SNIPER_TRAINING_BUFF_R1   = 64418,
};

class spell_hun_sniper_training : public SpellScriptLoader
{
    public:
        spell_hun_sniper_training() : SpellScriptLoader("spell_hun_sniper_training") { }

        class spell_hun_sniper_training_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_sniper_training_AuraScript);

            bool Validate(SpellInfo const* /*entry*/)
            {
                if (!sSpellMgr->GetSpellInfo(SPELL_SNIPER_TRAINING_R1) || !sSpellMgr->GetSpellInfo(SPELL_SNIPER_TRAINING_BUFF_R1))
                    return false;
                return true;
            }

            void HandlePeriodic(AuraEffect const* aurEff)
            {
                PreventDefaultAction();
                if (aurEff->GetAmount() <= 0)
                {
                    Unit* caster = GetCaster();
                    uint32 spellId = SPELL_SNIPER_TRAINING_BUFF_R1 + GetId() - SPELL_SNIPER_TRAINING_R1;
                    if (Unit* target = GetTarget())
                        if (!target->HasAura(spellId))
                        {
                            SpellInfo const* triggeredSpellInfo = sSpellMgr->GetSpellInfo(spellId);
                            Unit* triggerCaster = triggeredSpellInfo->NeedsToBeTriggeredByCaster(aurEff->GetSpellInfo(), target->GetMap()->GetDifficulty()) ? caster : target;
                            triggerCaster->CastSpell(target, triggeredSpellInfo, true, 0, aurEff);
                        }
                }
            }

            void HandleUpdatePeriodic(AuraEffect* aurEff)
            {
                if (Player* playerTarget = GetUnitOwner()->ToPlayer())
                {
                    int32 baseAmount = aurEff->GetBaseAmount();
                    int32 amount = playerTarget->isMoving() ?
                    playerTarget->CalculateSpellDamage(playerTarget, GetSpellInfo(), aurEff->GetEffIndex(), &baseAmount) :
                    aurEff->GetAmount() - 1;
                    aurEff->SetAmount(amount);
                }
            }

            void Register()
            {
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_hun_sniper_training_AuraScript::HandlePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
                OnEffectUpdatePeriodic += AuraEffectUpdatePeriodicFn(spell_hun_sniper_training_AuraScript::HandleUpdatePeriodic, EFFECT_0, SPELL_AURA_PERIODIC_TRIGGER_SPELL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hun_sniper_training_AuraScript();
        }
};

class spell_hun_pet_heart_of_the_phoenix : public SpellScriptLoader
{
    public:
        spell_hun_pet_heart_of_the_phoenix() : SpellScriptLoader("spell_hun_pet_heart_of_the_phoenix") { }

        class spell_hun_pet_heart_of_the_phoenix_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_pet_heart_of_the_phoenix_SpellScript);

            bool Load()
            {
                if (!GetCaster()->isPet())
                    return false;
                return true;
            }

            bool Validate(SpellInfo const* /*SpellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(HUNTER_PET_HEART_OF_THE_PHOENIX_TRIGGERED) || !sSpellMgr->GetSpellInfo(HUNTER_PET_HEART_OF_THE_PHOENIX_DEBUFF))
                    return false;
                return true;
            }

            void HandleScript(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (Unit* owner = caster->GetOwner())
                    if (!caster->HasAura(HUNTER_PET_HEART_OF_THE_PHOENIX_DEBUFF))
                    {
                        owner->CastCustomSpell(HUNTER_PET_HEART_OF_THE_PHOENIX_TRIGGERED, SPELLVALUE_BASE_POINT0, 100, caster, true);
                        caster->CastSpell(caster, HUNTER_PET_HEART_OF_THE_PHOENIX_DEBUFF, true);
                    }
            }

            void Register()
            {
                // add dummy effect spell handler to pet's Last Stand
                OnEffectHitTarget += SpellEffectFn(spell_hun_pet_heart_of_the_phoenix_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_pet_heart_of_the_phoenix_SpellScript();
        }
};

class spell_hun_pet_carrion_feeder : public SpellScriptLoader
{
    public:
        spell_hun_pet_carrion_feeder() : SpellScriptLoader("spell_hun_pet_carrion_feeder") { }

        class spell_hun_pet_carrion_feeder_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_pet_carrion_feeder_SpellScript);

            bool Load()
            {
                if (!GetCaster()->isPet())
                    return false;
                return true;
            }

            bool Validate(SpellInfo const* /*SpellInfo*/)
            {
                if (!sSpellMgr->GetSpellInfo(HUNTER_PET_SPELL_CARRION_FEEDER_TRIGGERED))
                    return false;
                return true;
            }

            SpellCastResult CheckIfCorpseNear()
            {
                Unit* caster = GetCaster();
                float max_range = GetSpellInfo()->GetMaxRange(false);
                WorldObject* result = NULL;
                // search for nearby enemy corpse in range
                Trinity::AnyDeadUnitSpellTargetInRangeCheck check(caster, max_range, GetSpellInfo(), TARGET_CHECK_ENEMY, MAX_SPELL_EFFECTS);
                Trinity::WorldObjectSearcher<Trinity::AnyDeadUnitSpellTargetInRangeCheck> searcher(caster, result, check);
                caster->GetMap()->VisitFirstFound(caster->m_positionX, caster->m_positionY, max_range, searcher);
                if (!result)
                    return SPELL_FAILED_NO_EDIBLE_CORPSES;
                return SPELL_CAST_OK;
            }

            void HandleDummy(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                caster->CastSpell(caster, HUNTER_PET_SPELL_CARRION_FEEDER_TRIGGERED, false);
            }

            void Register()
            {
                // add dummy effect spell handler to pet's Last Stand
                OnEffectHit += SpellEffectFn(spell_hun_pet_carrion_feeder_SpellScript::HandleDummy, EFFECT_0, SPELL_EFFECT_DUMMY);
                OnCheckCast += SpellCheckCastFn(spell_hun_pet_carrion_feeder_SpellScript::CheckIfCorpseNear);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_pet_carrion_feeder_SpellScript();
        }
};

// Misdirection (proc) - 35079
class spell_hun_misdirection_proc : public SpellScriptLoader
{
    public:
        spell_hun_misdirection_proc() : SpellScriptLoader("spell_hun_misdirection_proc") { }

        class spell_hun_misdirection_proc_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_misdirection_proc_AuraScript);

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (GetCaster())
                    GetCaster()->SetReducedThreatPercent(0, 0);
            }

            void Register()
            {
                AfterEffectRemove += AuraEffectRemoveFn(spell_hun_misdirection_proc_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hun_misdirection_proc_AuraScript();
        }
};

// Disengage - 781
class spell_hun_disengage : public SpellScriptLoader
{
    public:
        spell_hun_disengage() : SpellScriptLoader("spell_hun_disengage") { }

        class spell_hun_disengage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_disengage_SpellScript);

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (_player->HasAura(HUNTER_SPELL_POSTHASTE))
                    {
                        _player->RemoveMovementImpairingAuras();
                        _player->CastSpell(_player, HUNTER_SPELL_POSTHASTE_INCREASE_SPEED, true);
                    }
                    else if (_player->HasAura(HUNTER_SPELL_NARROW_ESCAPE))
                    {
                        std::list<Unit*> unitList;
                        std::list<Unit*> retsList;

                        _player->GetAttackableUnitListInRange(unitList, 8.0f);

                        for (std::list<Unit*>::const_iterator itr = unitList.begin(); itr != unitList.end(); ++itr)
                            if (_player->IsValidAttackTarget(*itr))
                                retsList.push_back(*itr);

                        for (std::list<Unit*>::const_iterator itr = retsList.begin(); itr != retsList.end(); ++itr)
                            _player->CastSpell(*itr, HUNTER_SPELL_NARROW_ESCAPE_RETS, true);
                    }
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_hun_disengage_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_disengage_SpellScript();
        }
};

class spell_hun_tame_beast : public SpellScriptLoader
{
    public:
        spell_hun_tame_beast() : SpellScriptLoader("spell_hun_tame_beast") { }

        class spell_hun_tame_beast_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_tame_beast_SpellScript);

            SpellCastResult CheckCast()
            {
                Unit* caster = GetCaster();
                if (caster->GetTypeId() != TYPEID_PLAYER)
                    return SPELL_FAILED_DONT_REPORT;

                if (!GetExplTargetUnit())
                    return SPELL_FAILED_BAD_IMPLICIT_TARGETS;

                if (Creature* target = GetExplTargetUnit()->ToCreature())
                {
                    if (target->getLevel() > caster->getLevel())
                        return SPELL_FAILED_HIGHLEVEL;

                    if (!target->GetCreatureTemplate()->isTameable(caster->ToPlayer()->CanTameExoticPets()))
                        return SPELL_FAILED_BAD_TARGETS;

                    if (caster->GetPetGUID())
                        return SPELL_FAILED_ALREADY_HAVE_SUMMON;

                    if (caster->GetCharmGUID())
                        return SPELL_FAILED_ALREADY_HAVE_CHARM;
                }
                else
                    return SPELL_FAILED_BAD_IMPLICIT_TARGETS;

                return SPELL_CAST_OK;
            }

            void Register()
            {
                OnCheckCast += SpellCheckCastFn(spell_hun_tame_beast_SpellScript::CheckCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_tame_beast_SpellScript();
        }
};

// Glave Toss - 117050
class spell_hun_toss : public SpellScriptLoader
{
    public:
        spell_hun_toss() : SpellScriptLoader("spell_hun_toss") { }

        class spell_hun_toss_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_toss_AuraScript);

            void HandleEffectPeriodic(AuraEffect const* aurEff)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                uint64 targetGUID = GetAura()->GetRndEffectTarget();
                if (!targetGUID)
                    return;

                Unit* target = ObjectAccessor::GetUnit(*caster, targetGUID);
                if (!target)
                    return;

                caster->CastSpell(target, 120761, true, NULL, aurEff);
                caster->CastSpell(target, 121414, true, NULL, aurEff);
            }

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                Player* _player = caster->ToPlayer();
                if (!_player)
                     return;

                Unit* target = _player->GetSelectedUnit();
                if (!target)
                    return;

                GetAura()->ClearEffectTarget();
                GetAura()->AddEffectTarget(target->GetGUID());
            }

            void Register()
            {
                AfterEffectApply += AuraEffectApplyFn(spell_hun_toss_AuraScript::OnApply, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectPeriodic += AuraEffectPeriodicFn(spell_hun_toss_AuraScript::HandleEffectPeriodic, EFFECT_1, SPELL_AURA_PERIODIC_DUMMY);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hun_toss_AuraScript();
        }
};

// Glave Toss - 120761, 121414
class spell_hun_toss_damage : public SpellScriptLoader
{
    public:
        spell_hun_toss_damage() : SpellScriptLoader("spell_hun_toss_damage") { }

        class spell_hun_toss_damage_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_toss_damage_SpellScript);

            void HandleOnHit(SpellEffIndex effIndex)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;
                Unit* target = GetHitUnit();
                if (!target)
                    return;

                if (Aura* toss = caster->GetAura(117050))
                {
                    uint64 targetGUID = toss->GetRndEffectTarget();
                    if (targetGUID == target->GetGUID())
                        SetHitDamage(GetHitDamage() * 4);
                }
            }

            void FilterTargets(std::list<WorldObject*>& targets)
            {
                if (targets.empty())
                    return;

                Unit* caster = GetCaster();
                if (!caster)
                    return;

                AuraEffect const* aurEff = GetSpell()->GetTriggeredAuraEff();
                if (!aurEff || !aurEff->GetBase())
                    return;
                uint64 targetGUID = aurEff->GetBase()->GetRndEffectTarget();
                if (!targetGUID)
                {
                    targets.clear();
                    return;
                }
                Unit* expltarget = ObjectAccessor::GetUnit(*caster, targetGUID);
                if (!expltarget)
                {
                    targets.clear();
                    return;
                }

                uint32 tick = aurEff->GetTickNumber();
                float distanceintick = 3.6f * (tick - 1);
                float maxDist = caster->GetDistance(expltarget);
                if(distanceintick > maxDist)
                    distanceintick = (maxDist * 2) - distanceintick;

                if(distanceintick < 0.0f)
                {
                    targets.clear();
                    return;
                }

                float angle = caster->GetAngle(expltarget);
                float angleShift = angle;
                float shift = 1.5f;
                if(GetSpellInfo()->Id == 120761)
                {
                    targets.remove_if(Trinity::UnitCheckInBetweenShift(false, caster, expltarget, 5.0f, 2.5f, 1.57f));
                    angleShift += 1.57f;
                }
                else
                {
                    targets.remove_if(Trinity::UnitCheckInBetweenShift(false, caster, expltarget, 5.0f, 2.5f, -1.57f));
                    angleShift -= 1.57f;
                }

                // expload at tick
                float x = caster->GetPositionX() + (caster->GetObjectSize() + distanceintick) * std::cos(angle);
                float y = caster->GetPositionY() + (caster->GetObjectSize() + distanceintick) * std::sin(angle);
                float shift_x = x + 2.5f * std::cos(angleShift);
                float shift_y = y + 2.5f * std::sin(angleShift);
                Trinity::NormalizeMapCoord(x);
                Trinity::NormalizeMapCoord(y);

                for (std::list<WorldObject*>::iterator itr = targets.begin(); itr != targets.end();)
                {
                    if((*itr)->GetDistance2d(shift_x, shift_y) > shift)
                        targets.erase(itr++);
                    else
                        ++itr;
                }
                if(expltarget->GetDistance2d(x, y) <= shift)
                    targets.push_back(expltarget);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_hun_toss_damage_SpellScript::HandleOnHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hun_toss_damage_SpellScript::FilterTargets, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
                OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_hun_toss_damage_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_DEST_AREA_ENEMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_toss_damage_SpellScript();
        }
};

// Fetch - 125050
class spell_hun_fetch : public SpellScriptLoader
{
    public:
        spell_hun_fetch() : SpellScriptLoader("spell_hun_fetch") { }

        class spell_hun_fetch_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_fetch_SpellScript);

            void HandleScriptEffect(SpellEffIndex /*effIndex*/)
            {
                Unit* caster = GetCaster();
                if (!caster)
                    return;

                Player* _player = caster->ToPlayer();
                if (!_player)
                    return;

                Unit* target = _player->GetSelectedUnit();
                if (!target)
                    return;

                Creature* corps = target->ToCreature();
                if (!corps)
                    return;

                if (!_player->isAllowedToLoot(corps))
                {
                    if (const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(125050))
                        Spell::SendCastResult(_player, spellInfo, 1, SPELL_FAILED_BAD_TARGETS);
                    return;
                }

                if (!target->IsWithinDistInMap(_player, 40.0f))
                {
                    if (const SpellInfo* spellInfo = sSpellMgr->GetSpellInfo(125050))
                        Spell::SendCastResult(_player, spellInfo, 1, SPELL_FAILED_OUT_OF_RANGE);
                    return;
                }

                Pet* pet = _player->GetPet();
                if (!pet)
                    return;

                pet->StopMoving();
                pet->GetMotionMaster()->Clear(false);
                pet->GetMotionMaster()->MoveFetch(target, PET_FOLLOW_DIST, pet->GetFollowAngle());
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_hun_fetch_SpellScript::HandleScriptEffect, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_fetch_SpellScript();
        }
};

// Fireworks - 127933
class spell_hun_fireworks : public SpellScriptLoader
{
    public:
        spell_hun_fireworks() : SpellScriptLoader("spell_hun_fireworks") { }

        class spell_hun_fireworks_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_fireworks_SpellScript);

            void HandleEffect(SpellEffIndex /*effIndex*/)
            {
                if (Unit* caster = GetCaster())
                {
                    switch(urand(0, 3))
                    {
                        case 0:
                            caster->CastSpell(caster, 127936, true);
                            break;
                        case 1:
                            caster->CastSpell(caster, 127937, true);
                            break;
                        case 2:
                            caster->CastSpell(caster, 127951, true);
                            break;
                        case 3:
                            caster->CastSpell(caster, 127961, true);
                            break;
                    }
                }
            }

            void Register()
            {
                OnEffectHit += SpellEffectFn(spell_hun_fireworks_SpellScript::HandleEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_fireworks_SpellScript();
        }
};

// Glyph of Direction - 126179, spell - 34477
class spell_hun_glyph_of_direction : public SpellScriptLoader
{
    public:
        spell_hun_glyph_of_direction() : SpellScriptLoader("spell_hun_glyph_of_direction") { }

        class spell_hun_glyph_of_direction_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_glyph_of_direction_AuraScript);

            void CalculateAmount(AuraEffect const* /*AuraEffect**/, int32& amount, bool& /*canBeRecalculated*/)
            {   
                if (Unit* caster = GetCaster())
                {
                    if (!caster->HasAura(HUNTER_SPELL_GLYPH_OF_DIRECTION))
                        amount = 0;
                }
            }

            void Register()
            {
                DoEffectCalcAmount += AuraEffectCalcAmountFn(spell_hun_glyph_of_direction_AuraScript::CalculateAmount, EFFECT_2, SPELL_AURA_MOD_SCALE);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hun_glyph_of_direction_AuraScript();
        }
};

// Thrill of the Hunt - 109306, spell: 
//! Arcane Shot - 3044, Multi-Shot - 2643
class spell_hun_thrill_of_the_hunt : public SpellScriptLoader
{
    public:
        spell_hun_thrill_of_the_hunt() : SpellScriptLoader("spell_hun_thrill_of_the_hunt") { }

        class spell_hun_thrill_of_the_hunt_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_thrill_of_the_hunt_SpellScript);

            void HandleAfterCast()
            {
                if (Player* _player = GetCaster()->ToPlayer())
                {
                    if (Aura* ThrillCharge = _player->GetAura(HUNTER_SPELL_HUN_THRILL_OF_THE_HUNT))
                        ThrillCharge->DropCharge();
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_hun_thrill_of_the_hunt_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_thrill_of_the_hunt_SpellScript();
        }
};

// Item - Hunter_T16_2P_Bonus - 144637
class spell_hun_t16_2p_bonus : public SpellScriptLoader
{
    public:
        spell_hun_t16_2p_bonus() : SpellScriptLoader("spell_hun_t16_2p_bonus") { }

        class spell_hun_t16_2p_bonus_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_t16_2p_bonus_SpellScript);

            void HandleOnCast()
            {
                if (Player* caster = GetCaster()->ToPlayer())
                {
                    if (caster->HasAura(HUNTER_SPELL_T16_2P_BONUS))
                    {
                        if (caster->GetSpecializationId(caster->GetActiveSpec()) == SPEC_HUNTER_SURVIVAL)
                            caster->ModifySpellCooldown(HUNTER_SPELL_RAPID_FIRE, - 8 * IN_MILLISECONDS);
                        else
                            caster->ModifySpellCooldown(HUNTER_SPELL_RAPID_FIRE, - 4 * IN_MILLISECONDS);
                    }
                }
            }

            void Register()
            {
                OnCast += SpellCastFn(spell_hun_t16_2p_bonus_SpellScript::HandleOnCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_t16_2p_bonus_SpellScript();
        }
};

class spell_hun_ice_trap : public SpellScriptLoader
{
    public:
        spell_hun_ice_trap() : SpellScriptLoader("spell_hun_ice_trap") {}

        class spell_hun_ice_trap_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_ice_trap_SpellScript);

            bool Validate(SpellInfo const* /*spell*/)
            {
                if (!sSpellMgr->GetSpellInfo(HUNTER_SPELL_ICE_TRAP_TRIGGER))
                    return false;
                return true;
            }

            void onEffect(SpellEffIndex /*effIndex*/)
            {
                //Owner of trap should cast
                if (Unit* originalCaster = GetOriginalCaster())
                    if (Unit* target = GetHitUnit())
                            originalCaster->CastSpell(target, HUNTER_SPELL_ICE_TRAP_TRIGGER, true);
            }

            void Register()
            {
                OnEffectHitTarget += SpellEffectFn(spell_hun_ice_trap_SpellScript::onEffect, EFFECT_0, SPELL_EFFECT_DUMMY);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_ice_trap_SpellScript();
        }
};

// Scatter Shot - 19503
class spell_hun_scatter_shot : public SpellScriptLoader
{
    public:
        spell_hun_scatter_shot() : SpellScriptLoader("spell_hun_scatter_shot") { }

        class spell_hun_scatter_shot_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_hun_scatter_shot_SpellScript);

            void HandleAfterCast()
            {
                if (Player* caster = GetCaster()->ToPlayer())
                {
					caster->SendAttackSwingResult(ATTACK_SWING_ERROR_DEAD_TARGET);
                }
            }

            void Register()
            {
                AfterCast += SpellCastFn(spell_hun_scatter_shot_SpellScript::HandleAfterCast);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_hun_scatter_shot_SpellScript();
        }
};

// Bestial Wrath - 19574
class spell_hun_bestial_wrath  : public SpellScriptLoader
{
    public:
        spell_hun_bestial_wrath() : SpellScriptLoader("spell_hun_bestial_wrath") { }

        class spell_hun_bestial_wrath_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_bestial_wrath_AuraScript);

            void Absorb(AuraEffect* aurEff, DamageInfo& dmgInfo, uint32& absorbAmount)
            {
                if (Unit* target = GetTarget())
                {
                    if (dmgInfo.GetDamage() >= target->GetHealth())
                    {
                        if(target->GetHealth() > 1)
                            absorbAmount = dmgInfo.GetDamage() - target->GetHealth() + 1;
                        else
                            absorbAmount = dmgInfo.GetDamage();
                        return;
                    }
                }
                absorbAmount = 0;
            }

            void Register()
            {
                OnEffectAbsorb += AuraEffectAbsorbFn(spell_hun_bestial_wrath_AuraScript::Absorb, EFFECT_3, SPELL_AURA_SCHOOL_ABSORB);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hun_bestial_wrath_AuraScript();
        }
};

// Widow Venom - 82654
class spell_hun_widow_venom : public SpellScriptLoader
{
    public:
        spell_hun_widow_venom() : SpellScriptLoader("spell_hun_widow_venom") { }

        class spell_hun_widow_venom_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_hun_widow_venom_AuraScript);

            void CalculateMaxDuration(int32& duration)
            {
                Unit* target = GetUnitOwner();
                if(target && target->ToPlayer())
                    duration =  10 * IN_MILLISECONDS;
            }

            void Register()
            {
                DoCalcMaxDuration += AuraCalcMaxDurationFn(spell_hun_widow_venom_AuraScript::CalculateMaxDuration);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_hun_widow_venom_AuraScript();
        }
};

// Hunter's Mark - 1130
class spell_hun_of_marked_for_die : public SpellScriptLoader
{
public:
    spell_hun_of_marked_for_die() : SpellScriptLoader("spell_hun_of_marked_for_die") { }

    class spell_hun_of_marked_for_die_AuraScript : public AuraScript
    {
        PrepareAuraScript(spell_hun_of_marked_for_die_AuraScript);

        void CalculateMaxDuration(int32& duration)
        {
            if (Unit* target = GetUnitOwner())
                if (target->GetTypeId() == TYPEID_PLAYER)
                    duration = 20 * IN_MILLISECONDS;
        }

        void Register()
        {
            DoCalcMaxDuration += AuraCalcMaxDurationFn(spell_hun_of_marked_for_die_AuraScript::CalculateMaxDuration);
        }
    };

    AuraScript* GetAuraScript() const
    {
        return new spell_hun_of_marked_for_die_AuraScript();
    }
};

void AddSC_hunter_spell_scripts()
{
    new spell_hun_dash();
    new spell_hun_of_marked_for_die();
    new spell_hun_stampede();
    new spell_hun_dire_beast();
    new spell_hun_a_murder_of_crows();
    new spell_hun_focus_fire();
    new spell_hun_frenzy();
    new spell_hun_lynx_rush();
    new spell_hun_beast_cleave();
    new spell_hun_cobra_strikes();
    new spell_hun_barrage();
    new spell_hun_binding_shot();
    new spell_hun_binding_shot_zone();
    new spell_hun_improved_serpent_sting();
    new spell_hun_camouflage_visual();
    new spell_hun_serpent_spread();
    new spell_hun_ancient_hysteria();
    new spell_hun_kill_command();
    new spell_hun_cobra_shot();
    new spell_hun_steady_shot();
    new spell_hun_chimera_shot();
    new spell_hun_last_stand_pet();
    new spell_hun_masters_call();
    new spell_hun_sniper_training();
    new spell_hun_pet_heart_of_the_phoenix();
    new spell_hun_pet_carrion_feeder();
    new spell_hun_misdirection_proc();
    new spell_hun_disengage();
    new spell_hun_tame_beast();
    new spell_hun_toss();
    new spell_hun_fetch();
    new spell_hun_fireworks();
    new spell_hun_glyph_of_direction();
    new spell_hun_thrill_of_the_hunt();
    new spell_hun_t16_2p_bonus();
    new spell_hun_ice_trap();
    new spell_hun_scatter_shot();
    new spell_hun_bestial_wrath();
    new spell_hun_toss_damage();
    new spell_hun_widow_venom();
}
