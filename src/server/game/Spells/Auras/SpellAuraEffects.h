/*
 * Copyright (C) 2008-2012 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
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

#ifndef TRINITY_SPELLAURAEFFECTS_H
#define TRINITY_SPELLAURAEFFECTS_H

class Unit;
class AuraEffect;
class Aura;

#include "SpellAuras.h"

typedef void(AuraEffect::*pAuraEffectHandler)(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;

class AuraEffect : public std::enable_shared_from_this<AuraEffect>
{
    friend void Aura::_InitEffects(uint32 effMask, UnitPtr caster, int32 *baseAmount);
    friend AuraPtr Unit::_TryStackingOrRefreshingExistingAura(SpellInfo const* newAura, uint32 effMask, UnitPtr caster, int32* baseAmount, ItemPtr castItem, uint64 casterGUID);
    friend Aura::~Aura();
    private:
        explicit AuraEffect(AuraPtr base, uint8 effIndex, int32 *baseAmount, UnitPtr caster);
    public:
        ~AuraEffect();
        UnitPtr GetCaster() const { return GetBase()->GetCaster(); }
        uint64 GetCasterGUID() const { return GetBase()->GetCasterGUID(); }
        AuraPtr GetBase() const { return std::const_pointer_cast<Aura>(m_base); }
        void GetTargetList(std::list<UnitPtr> & targetList) const;
        void GetApplicationList(std::list<AuraApplicationPtr> & applicationList) const;
        SpellModifier* GetSpellModifier() const { return m_spellmod; }

        SpellInfo const* GetSpellInfo() const { return m_spellInfo; }
        uint32 GetId() const { return m_spellInfo->Id; }
        uint32 GetEffIndex() const { return m_effIndex; }
        int32 GetBaseAmount() const { return m_baseAmount; }
        int32 GetAmplitude() const { return m_amplitude; }

        int32 GetMiscValueB() const { return m_spellInfo->Effects[m_effIndex].MiscValueB; }
        int32 GetMiscValue() const { return m_spellInfo->Effects[m_effIndex].MiscValue; }
        AuraType GetAuraType() const { return (AuraType)m_spellInfo->Effects[m_effIndex].ApplyAuraName; }
        int32 GetAmount() const { return m_amount; }
        void SetAmount(int32 amount) { m_amount = amount; m_canBeRecalculated = false;}

        int32 GetPeriodicTimer() const { return m_periodicTimer; }
        void SetPeriodicTimer(int32 periodicTimer) { m_periodicTimer = periodicTimer; }

        int32 CalculateAmount(UnitPtr caster);
        void CalculatePeriodic(UnitPtr caster, bool resetPeriodicTimer = true, bool load = false);
        void CalculateSpellMod();
        void ChangeAmount(int32 newAmount, bool mark = true, bool onStackOrReapply = false);
        void RecalculateAmount() { if (!CanBeRecalculated()) return; ChangeAmount(CalculateAmount(GetCaster()), false); }
        void RecalculateAmount(UnitPtr caster) { if (!CanBeRecalculated()) return; ChangeAmount(CalculateAmount(caster), false); }
        bool CanBeRecalculated() const { return m_canBeRecalculated; }
        void SetCanBeRecalculated(bool val) { m_canBeRecalculated = val; }
        void HandleEffect(AuraApplicationPtr aurApp, uint8 mode, bool apply);
        void HandleEffect(UnitPtr target, uint8 mode, bool apply);
        void ApplySpellMod(UnitPtr target, bool apply);

        void Update(uint32 diff, UnitPtr caster);
        void UpdatePeriodic(UnitPtr caster);

        uint32 GetTickNumber() const { return m_tickNumber; }
        int32 GetTotalTicks() const { return m_amplitude ? (GetBase()->GetMaxDuration() / m_amplitude) : 1;}
        void ResetPeriodic(bool resetPeriodicTimer = false) { if (resetPeriodicTimer) m_periodicTimer = m_amplitude; m_tickNumber = 0;}

        bool IsPeriodic() const { return m_isPeriodic; }
        void SetPeriodic(bool isPeriodic) { m_isPeriodic = isPeriodic; }
        bool IsAffectingSpell(SpellInfo const* spell) const;
        bool HasSpellClassMask() const { return m_spellInfo->Effects[m_effIndex].SpellClassMask; }

        void SendTickImmune(UnitPtr target, UnitPtr caster) const;
        void PeriodicTick(AuraApplicationPtr aurApp, UnitPtr caster) const;

        void HandleProc(AuraApplicationPtr aurApp, ProcEventInfo& eventInfo);

        void CleanupTriggeredSpells(UnitPtr target);

        // add/remove SPELL_AURA_MOD_SHAPESHIFT (36) linked auras
        void HandleShapeshiftBoosts(UnitPtr target, bool apply) const;
    private:
        constAuraPtr m_base;

        SpellInfo const* const m_spellInfo;
        int32 const m_baseAmount;

        int32 m_amount;

        SpellModifier* m_spellmod;

        int32 m_periodicTimer;
        int32 m_amplitude;
        uint32 m_tickNumber;

        uint8 const m_effIndex;
        bool m_canBeRecalculated;
        bool m_isPeriodic;
    private:
        bool IsPeriodicTickCrit(UnitPtr target, constUnitPtr caster) const;

    public:
        // aura effect apply/remove handlers
        void HandleNULL(constAuraApplicationPtr /*aurApp*/, uint8 /*mode*/, bool /*apply*/) const
        {
            // not implemented
        }
        void HandleUnused(constAuraApplicationPtr /*aurApp*/, uint8 /*mode*/, bool /*apply*/) const
        {
            // useless
        }
        void HandleNoImmediateEffect(constAuraApplicationPtr /*aurApp*/, uint8 /*mode*/, bool /*apply*/) const
        {
            // aura type not have immediate effect at add/remove and handled by ID in other code place
        }
        //  visibility & phases
        void HandleModInvisibilityDetect(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModInvisibility(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModStealth(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModStealthLevel(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModStealthDetect(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleSpiritOfRedemption(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraGhost(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandlePhase(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  unit model
        void HandleAuraModShapeshift(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraTransform(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModScale(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraCloneCaster(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  fight
        void HandleFeignDeath(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModUnattackable(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModDisarm(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModSilence(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModPacify(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModPacifyAndSilence(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraAllowOnlyAbility(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  tracking
        void HandleAuraTrackResources(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraTrackCreatures(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraTrackStealthed(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModStalked(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraUntrackable(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  skills & talents
        void HandleAuraModPetTalentsPoints(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModSkill(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  movement
        void HandleAuraMounted(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraAllowFlight(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraWaterWalk(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraFeatherFall(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraHover(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleWaterBreathing(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleForceMoveForward(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  threat
        void HandleModThreat(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModTotalThreat(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModTaunt(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  control
        void HandleModConfuse(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModFear(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModStun(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModRoot(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandlePreventFleeing(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  charm
        void HandleModPossess(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModPossessPet(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModCharm(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleCharmConvert(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraControlVehicle(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  modify speed
        void HandleAuraModIncreaseSpeed(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseMountedSpeed(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseFlightSpeed(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseSwimSpeed(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModDecreaseSpeed(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModUseNormalSpeed(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  immunity
        void HandleModStateImmunityMask(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModMechanicImmunity(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModEffectImmunity(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModStateImmunity(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModSchoolImmunity(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModDmgImmunity(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModDispelImmunity(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  modify stats
        //   resistance
        void HandleAuraModResistanceExclusive(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModResistance(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModBaseResistancePCT(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModResistancePercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModBaseResistance(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModTargetResistance(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //    stat
        void HandleAuraModStat(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModPercentStat(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModSpellDamagePercentFromStat(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModSpellHealingPercentFromStat(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModSpellDamagePercentFromAttackPower(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModSpellHealingPercentFromAttackPower(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModSpellPowerPercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModHealingDone(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModTotalPercentStat(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModResistenceOfStatPercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModExpertise(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleOverrideSpellPowerByAttackPower(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleIncreaseHasteFromItemsByPct(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //   heal and energize
        void HandleModPowerRegen(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModPowerRegenPCT(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModManaRegen(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseHealth(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseMaxHealth(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseEnergy(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseEnergyPercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseHealthPercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraIncreaseBaseHealthPercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //   fight
        void HandleAuraModParryPercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModDodgePercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModBlockPercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModRegenInterrupt(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModWeaponCritPercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModHitChance(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModSpellHitChance(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModSpellCritChance(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModSpellCritChanceShool(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModCritPct(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //   attack speed
        void HandleModCastingSpeed(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModMeleeRangedSpeedPct(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModCombatSpeedPct(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModAttackSpeed(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModMeleeSpeedPct(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModRangedHaste(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //   combat rating
        void HandleModRating(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModRatingFromStat(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //   attack power
        void HandleAuraModAttackPower(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModRangedAttackPower(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModAttackPowerPercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModRangedAttackPowerPercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModAttackPowerOfArmor(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleOverrideAttackPowerBySpellPower(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //   damage bonus
        void HandleModDamageDone(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModDamagePercentDone(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModOffhandDamagePercent(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleShieldBlockValue(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  power cost
        void HandleModPowerCostPCT(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleModPowerCost(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleArenaPreparation(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleNoReagentUseAura(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraRetainComboPoints(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        //  others
        void HandleAuraDummy(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleChannelDeathItem(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleBindSight(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleForceReaction(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraEmpathy(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModFaction(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleComprehendLanguage(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraConvertRune(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraLinked(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraOpenStable(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraModFakeInebriation(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraOverrideSpells(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraSetVehicle(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandlePreventResurrection(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;
        void HandleAuraForceWeather(constAuraApplicationPtr aurApp, uint8 mode, bool apply) const;

        // aura effect periodic tick handlers
        void HandlePeriodicDummyAuraTick(UnitPtr target, UnitPtr caster) const;
        void HandlePeriodicTriggerSpellAuraTick(UnitPtr target, UnitPtr caster) const;
        void HandlePeriodicTriggerSpellWithValueAuraTick(UnitPtr target, UnitPtr caster) const;
        void HandlePeriodicDamageAurasTick(UnitPtr target, UnitPtr caster) const;
        void HandlePeriodicHealthLeechAuraTick(UnitPtr target, UnitPtr caster) const;
        void HandlePeriodicHealthFunnelAuraTick(UnitPtr target, UnitPtr caster) const;
        void HandlePeriodicHealAurasTick(UnitPtr target, UnitPtr caster) const;
        void HandlePeriodicManaLeechAuraTick(UnitPtr target, UnitPtr caster) const;
        void HandleObsModPowerAuraTick(UnitPtr target, UnitPtr caster) const;
        void HandlePeriodicEnergizeAuraTick(UnitPtr target, UnitPtr caster) const;
        void HandlePeriodicPowerBurnAuraTick(UnitPtr target, UnitPtr caster) const;

        // aura effect proc handlers
        void HandleProcTriggerSpellAuraProc(AuraApplicationPtr aurApp, ProcEventInfo& eventInfo);
        void HandleProcTriggerSpellWithValueAuraProc(AuraApplicationPtr aurApp, ProcEventInfo& eventInfo);
        void HandleProcTriggerDamageAuraProc(AuraApplicationPtr aurApp, ProcEventInfo& eventInfo);
        void HandleRaidProcFromChargeAuraProc(AuraApplicationPtr aurApp, ProcEventInfo& eventInfo);
        void HandleRaidProcFromChargeWithValueAuraProc(AuraApplicationPtr aurApp, ProcEventInfo& eventInfo);
};

namespace Trinity
{
    // Binary predicate for sorting the priority of absorption aura effects
    class AbsorbAuraOrderPred
    {
        public:
            AbsorbAuraOrderPred() { }
            bool operator() (AuraEffectPtr aurEffA, AuraEffectPtr aurEffB) const
            {
                SpellInfo const* spellProtoA = aurEffA->GetSpellInfo();
                SpellInfo const* spellProtoB = aurEffB->GetSpellInfo();

                // Wards
                if ((spellProtoA->SpellFamilyName == SPELLFAMILY_MAGE) ||
                    (spellProtoA->SpellFamilyName == SPELLFAMILY_WARLOCK))
                    if (spellProtoA->Category == 56)
                        return true;
                if ((spellProtoB->SpellFamilyName == SPELLFAMILY_MAGE) ||
                    (spellProtoB->SpellFamilyName == SPELLFAMILY_WARLOCK))
                    if (spellProtoB->Category == 56)
                        return false;

                // Sacred Shield
                if (spellProtoA->Id == 58597)
                    return true;
                if (spellProtoB->Id == 58597)
                    return false;

                // Fel Blossom
                if (spellProtoA->Id == 28527)
                    return true;
                if (spellProtoB->Id == 28527)
                    return false;

                // Divine Aegis
                if (spellProtoA->Id == 47753)
                    return true;
                if (spellProtoB->Id == 47753)
                    return false;

                // Ice Barrier
                if (spellProtoA->Category == 471)
                    return true;
                if (spellProtoB->Category == 471)
                    return false;

                // Sacrifice
                if ((spellProtoA->SpellFamilyName == SPELLFAMILY_WARLOCK) &&
                    (spellProtoA->SpellIconID == 693))
                    return true;
                if ((spellProtoB->SpellFamilyName == SPELLFAMILY_WARLOCK) &&
                    (spellProtoB->SpellIconID == 693))
                    return false;

                return false;
            }
    };
}
#endif
