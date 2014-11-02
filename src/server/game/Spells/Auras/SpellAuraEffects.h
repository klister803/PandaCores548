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

typedef void(AuraEffect::*pAuraEffectHandler)(AuraApplication const* aurApp, uint8 mode, bool apply) const;

class AuraEffect
{
    friend void Aura::_InitEffects(uint32 effMask, Unit* caster, int32 *baseAmount);
    friend Aura* Unit::_TryStackingOrRefreshingExistingAura(SpellInfo const* newAura, uint32 effMask, Unit* caster, int32* baseAmount, Item* castItem, uint64 casterGUID);
    friend Aura::~Aura();
    private:
        explicit AuraEffect(Aura* base, uint8 effIndex, int32 *baseAmount, Unit* caster);
    public:
        ~AuraEffect();
        Unit* GetCaster() const { return GetBase()->GetCaster(); }
        Unit* GetSaveTarget() const { return saveTarget; }
        uint64 GetCasterGUID() const { return GetBase()->GetCasterGUID(); }
        Aura* GetBase() const { return m_base; }
        void GetTargetList(std::list<Unit*> & targetList) const;
        void GetApplicationList(std::list<AuraApplication*> & applicationList) const;
        SpellModifier* GetSpellModifier() const { return m_spellmod; }

        SpellInfo const* GetSpellInfo() const { return m_spellInfo; }
        uint32 GetId() const { return m_spellInfo->Id; }
        uint32 GetEffIndex() const { return m_effIndex; }
        int32 GetBaseAmount() const { return m_baseAmount; }
        int32 GetBaseSendAmount() const { return m_send_baseAmount; }
        int32 GetOldBaseAmount() const { return m_oldbaseAmount; }
        int32 GetAmplitude() const { return m_amplitude; }
        void SetAmplitude(int32 amplitude) { m_amplitude = amplitude; }

        int32 GetMiscValueB() const { return m_spellInfo->GetEffect(m_effIndex, m_diffMode).MiscValueB; }
        int32 GetMiscValue() const { return m_spellInfo->GetEffect(m_effIndex, m_diffMode).MiscValue; }
        uint32 GetTriggerSpell() const { return m_spellInfo->GetEffect(m_effIndex, m_diffMode).TriggerSpell; }
        AuraType GetAuraType() const { return (AuraType)m_spellInfo->GetEffect(m_effIndex, m_diffMode).ApplyAuraName; }
        int32 GetAmount() const { return m_amount; }
        void SetAmount(int32 amount)
        {
            if (m_amount != amount)
            {
                m_amount = amount;
                GetBase()->SetNeedClientUpdateForTargets();
            }

            m_canBeRecalculated = false;
        }
        int32 GetCritAmount() const { return m_crit_amount; }
        void SetCritAmount(int32 amount) { m_crit_amount = amount;}
        float GetCritChance() const { return m_crit_chance; }
        void SetCritChance(float amount) { m_crit_chance = amount;}

        int32 GetPeriodicTimer() const { return m_periodicTimer; }
        void SetPeriodicTimer(int32 periodicTimer) { m_periodicTimer = periodicTimer; }

        int32 CalculateAmount(Unit* caster, int32 &m_aura_amount);
        void CalculateFromDummyAmount(Unit* caster, Unit* target, int32 &amount);
        void CalculatePeriodic(Unit* caster, bool resetPeriodicTimer = true, bool load = false);
        void CalculateSpellMod();
        void ChangeAmount(int32 newAmount, bool mark = true, bool onStackOrReapply = false);
        void RecalculateAmount() { if (!CanBeRecalculated()) return; ChangeAmount(CalculateAmount(GetCaster(), GetBase()->m_aura_amount), false); }
        void RecalculateAmount(Unit* caster) { if (!CanBeRecalculated()) return; ChangeAmount(CalculateAmount(caster, GetBase()->m_aura_amount), false); }
        bool CanBeRecalculated() const { return m_canBeRecalculated; }
        void SetCanBeRecalculated(bool val) { m_canBeRecalculated = val; }
        void HandleEffect(AuraApplication * aurApp, uint8 mode, bool apply);
        void HandleEffect(Unit* target, uint8 mode, bool apply);
        void ApplySpellMod(Unit* target, bool apply);

        void Update(uint32 diff, Unit* caster);
        void UpdatePeriodic(Unit* caster);

        uint32 GetTickNumber() const { return m_tickNumber; }
        void SetTickNumber(uint32 tick) { m_tickNumber = tick; }
        int32 GetTotalTicks() const { return m_amplitude ? (GetBase()->GetMaxDuration() / m_amplitude) : 1;}
        void ResetPeriodic(bool resetPeriodicTimer = false) { if (resetPeriodicTimer) m_periodicTimer = m_amplitude; m_tickNumber = 0;}

        bool IsPeriodic() const { return m_isPeriodic; }
        void SetPeriodic(bool isPeriodic) { m_isPeriodic = isPeriodic; }
        bool IsAffectingSpell(SpellInfo const* spell) const;
        bool HasSpellClassMask() const { return m_spellInfo->GetEffect(m_effIndex, m_diffMode).SpellClassMask; }

        void SendTickImmune(Unit* target, Unit* caster) const;
        void PeriodicTick(AuraApplication * aurApp, Unit* caster, SpellEffIndex effIndex) const;

        void HandleProc(AuraApplication* aurApp, ProcEventInfo& eventInfo, SpellEffIndex effIndex);

        void CleanupTriggeredSpells(Unit* target);

        // add/remove SPELL_AURA_MOD_SHAPESHIFT (36) linked auras
        void HandleShapeshiftBoosts(Unit* target, bool apply) const;
    private:
        Aura* const m_base;

        SpellInfo const* const m_spellInfo;
        int32 const m_baseAmount;

        int32 m_amount;
        int32 m_crit_amount;
        float m_crit_chance;
        int32 m_oldbaseAmount;
        int32 m_send_baseAmount;
        Unit* saveTarget;

        SpellModifier* m_spellmod;

        int32 m_periodicTimer;
        int32 m_amplitude;
        uint32 m_tickNumber;

        uint8 const m_effIndex;
        bool m_canBeRecalculated;
        bool m_isPeriodic;
        uint8 m_diffMode;

    public:
        // aura effect apply/remove handlers
        void HandleNULL(AuraApplication const* /*aurApp*/, uint8 /*mode*/, bool /*apply*/) const
        {
            // not implemented
        }
        void HandleUnused(AuraApplication const* /*aurApp*/, uint8 /*mode*/, bool /*apply*/) const
        {
            // useless
        }
        void HandleNoImmediateEffect(AuraApplication const* /*aurApp*/, uint8 /*mode*/, bool /*apply*/) const
        {
            // aura type not have immediate effect at add/remove and handled by ID in other code place
        }
        //  visibility & phases
        void HandleModInvisibilityDetect(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModInvisibility(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModCamouflage(AuraApplication const * aurApp, uint8 mode, bool apply) const;
        void HandleModStealth(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModStealthLevel(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModStealthDetect(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleSpiritOfRedemption(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraGhost(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandlePhase(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  unit model
        void HandleAuraModShapeshift(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraTransform(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModScale(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraCloneCaster(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraInitializeImages(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  fight
        void HandleFeignDeath(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModUnattackable(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModDisarm(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModSilence(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModPacify(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModPacifyAndSilence(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraAllowOnlyAbility(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  tracking
        void HandleAuraTrackResources(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraTrackCreatures(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraTrackStealthed(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModStalked(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraUntrackable(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  skills & talents
        void HandleAuraModPetTalentsPoints(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModSkill(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  movement
        void HandleAuraMounted(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraAllowFlight(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraWaterWalk(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraFeatherFall(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraGlide(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraHover(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleWaterBreathing(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleForceMoveForward(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  threat
        void HandleModThreat(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModTotalThreat(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModTaunt(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  control
        void HandleModConfuse(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModFear(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModStun(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModRoot(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandlePreventFleeing(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  charm
        void HandleModPossess(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModPossessPet(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModCharm(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleCharmConvert(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraControlVehicle(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  modify speed
        void HandleAuraModIncreaseSpeed(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseMountedSpeed(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseFlightSpeed(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseSwimSpeed(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModDecreaseSpeed(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModUseNormalSpeed(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  immunity
        void HandleModStateImmunityMask(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModMechanicImmunity(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModEffectImmunity(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModStateImmunity(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModSchoolImmunity(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModDmgImmunity(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModDispelImmunity(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  modify stats
        //   resistance
        void HandleAuraModResistanceExclusive(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModResistance(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModBaseResistancePCT(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModResistancePercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModBaseResistance(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModTargetResistance(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //    stat
        void HandleAuraModStat(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModPercentStat(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModSpellDamagePercentFromStat(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModSpellHealingPercentFromStat(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModSpellDamagePercentFromAttackPower(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModSpellHealingPercentFromAttackPower(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModSpellPowerPercent(AuraApplication const * aurApp, uint8 mode, bool apply) const;
        void HandleModHealingDone(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModTotalPercentStat(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModResistenceOfStatPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModExpertise(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleOverrideSpellPowerByAttackPower(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleIncreaseHasteFromItemsByPct(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //   heal and energize
        void HandleModPowerRegen(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModPowerRegenPCT(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModManaRegen(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseHealth(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseMaxHealth(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseEnergy(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseEnergyPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModMaxPower(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModAddEnergyPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModIncreaseHealthPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraIncreaseBaseHealthPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModPetStatsModifier(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //   fight
        void HandleAuraModParryPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModDodgePercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModBlockPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModRegenInterrupt(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModWeaponCritPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModHitChance(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModSpellHitChance(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModSpellCritChance(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModSpellCritChanceShool(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModCritPct(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModResiliencePct(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //   attack speed
        void HandleModCastingSpeed(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModMeleeRangedSpeedPct(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModCombatSpeedPct(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModAttackSpeed(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModMeleeSpeedPct(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModRangedHaste(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //   combat rating
        void HandleModRating(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModRatingFromStat(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //   attack power
        void HandleAuraModAttackPower(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModRangedAttackPower(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModAttackPowerPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModRangedAttackPowerPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModAttackPowerOfArmor(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleOverrideAttackPowerBySpellPower(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //   damage bonus
        void HandleModDamageDone(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModDamagePercentDone(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModOffhandDamagePercent(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleShieldBlockValue(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  power cost
        void HandleModPowerCostPCT(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleModPowerCost(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleArenaPreparation(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleNoReagentUseAura(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraRetainComboPoints(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        //  others
        void HandleAuraDummy(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleChannelDeathItem(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleBindSight(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleForceReaction(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraEmpathy(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModFaction(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleComprehendLanguage(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraConvertRune(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraLinked(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraOpenStable(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraStrangulate(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModFakeInebriation(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraOverrideSpells(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraSetVehicle(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandlePreventResurrection(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraForceWeather(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleProgressBar(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleOverrideActionbarSpells(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModCategoryCooldown(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraSeeWhileInvisible(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraMastery(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleAuraModCharges(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleBattlegroundFlag(AuraApplication const* aurApp, uint8 mode, bool apply) const;
        void HandleCreateAreaTrigger(AuraApplication const* aurApp, uint8 mode, bool apply) const;

        // aura effect periodic tick handlers
        void HandlePeriodicDummyAuraTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const;
        void HandlePeriodicTriggerSpellAuraTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const;
        void HandlePeriodicTriggerSpellWithValueAuraTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const;
        void HandlePeriodicDamageAurasTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const;
        void HandlePeriodicHealthLeechAuraTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const;
        void HandlePeriodicHealthFunnelAuraTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const;
        void HandlePeriodicHealAurasTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const;
        void HandlePeriodicManaLeechAuraTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const;
        void HandleObsModPowerAuraTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const;
        void HandlePeriodicEnergizeAuraTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const;
        void HandlePeriodicPowerBurnAuraTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const;

        // aura effect proc handlers
        void HandleProcTriggerSpellAuraProc(AuraApplication* aurApp, ProcEventInfo& eventInfo, SpellEffIndex effIndex);
        void HandleProcTriggerSpellWithValueAuraProc(AuraApplication* aurApp, ProcEventInfo& eventInfo, SpellEffIndex effIndex);
        void HandleProcTriggerDamageAuraProc(AuraApplication* aurApp, ProcEventInfo& eventInfo, SpellEffIndex effIndex);
        void HandleRaidProcFromChargeAuraProc(AuraApplication* aurApp, ProcEventInfo& eventInfo, SpellEffIndex effIndex);
        void HandleRaidProcFromChargeWithValueAuraProc(AuraApplication* aurApp, ProcEventInfo& eventInfo, SpellEffIndex effIndex);
};

namespace Trinity
{
    // Binary predicate for sorting the priority of absorption aura effects
    class AbsorbAuraOrderPred
    {
        public:
            AbsorbAuraOrderPred() { }
            bool operator() (AuraEffect* aurEffA, AuraEffect* aurEffB) const
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
