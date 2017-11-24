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

#include "Unit.h"
#include "Player.h"
#include "Pet.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "SharedDefines.h"
#include "SpellAuras.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"

inline bool _ModifyUInt32(bool apply, uint32& baseValue, int32& amount)
{
    // If amount is negative, change sign and value of apply.
    if (amount < 0)
    {
        apply = !apply;
        amount = -amount;
    }
    if (apply)
        baseValue += amount;
    else
    {
        // Make sure we do not get uint32 overflow.
        if (amount > int32(baseValue))
            amount = baseValue;
        baseValue -= amount;
    }
    return apply;
}

/*#######################################
########                         ########
########   PLAYERS STAT SYSTEM   ########
########                         ########
#######################################*/

bool Player::UpdateStats(Stats stat)
{
    if (stat > STAT_SPIRIT)
        return false;

    // value = ((base_value * base_pct) + total_value) * total_pct
    float value  = GetTotalStatValue(stat);

    SetStat(stat, int32(value));

    switch (stat)
    {
        case STAT_STRENGTH:
            UpdateParryPercentage();
            break;
        case STAT_AGILITY:
            UpdateAllCritPercentages();
            UpdateDodgePercentage();
            break;
        case STAT_STAMINA:
            UpdateMaxHealth();
            break;
        case STAT_INTELLECT:
            UpdateMaxPower(POWER_MANA);
            UpdateSpellCritChance(SPELL_SCHOOL_MASK_ALL);
            UpdateArmor();                                  //SPELL_AURA_MOD_RESISTANCE_OF_INTELLECT_PERCENT, only armor currently
            break;
        case STAT_SPIRIT:
            break;
        default:
            break;
    }

    if (stat == STAT_STRENGTH)
        UpdateAttackPowerAndDamage(false);
    else if (stat == STAT_AGILITY)
    {
        UpdateAttackPowerAndDamage(false);
        UpdateAttackPowerAndDamage(true);
    }

    UpdateSpellDamageAndHealingBonus();
    UpdateManaRegen();

    // Update ratings in exist SPELL_AURA_MOD_RATING_FROM_STAT and only depends from stat
    uint32 mask = 0;
    AuraEffectList const& modRatingFromStat = GetAuraEffectsByType(SPELL_AURA_MOD_RATING_FROM_STAT);
    for (AuraEffectList::const_iterator i = modRatingFromStat.begin(); i != modRatingFromStat.end(); ++i)
        if (Stats((*i)->GetMiscValueB()) == stat)
            mask |= (*i)->GetMiscValue();
    if (mask)
    {
        for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
            if (mask & (1 << rating))
                ApplyRatingMod(CombatRating(rating), 0, true);
    }

    if (stat == STAT_INTELLECT)
    {
        if (Aura* aura = GetAura(108300))
        {
            aura->GetEffect(EFFECT_0)->SetCanBeRecalculated(true);
            aura->GetEffect(EFFECT_0)->RecalculateAmount();
        }
    }

    if (stat == STAT_STAMINA || stat == STAT_INTELLECT || stat == STAT_STRENGTH)
    {
        Pet* pet = GetPet();
        if (pet)
            pet->UpdateStats(stat);
    }

    return true;
}

void Player::ApplySpellPowerBonus(int32 amount, bool apply)
{
    _ModifyUInt32(apply, m_baseSpellPower, amount);

    UpdateSpellDamageAndHealingBonus();
}

void Player::UpdateSpellDamageAndHealingBonus()
{
    // Magic damage modifiers implemented in Unit::SpellDamageBonusDone
    // This information for client side use only
    // Get healing bonus for all schools

    int32 amount = 0;
    amount += m_baseSpellPower;

    AuraEffectList const& mOverrideSpellPowerAuras = GetAuraEffectsByType(SPELL_AURA_OVERRIDE_SPELL_POWER_BY_AP_PCT);
    if (!mOverrideSpellPowerAuras.empty())
    {
        for (AuraEffectList::const_iterator itr = mOverrideSpellPowerAuras.begin(); itr != mOverrideSpellPowerAuras.end(); ++itr)
            amount = int32(GetTotalAttackPowerValue(BASE_ATTACK) * (*itr)->GetAmount() / 100.0f);

        SetStatFloatValue(PLAYER_FIELD_OVERRIDE_SPELL_POWER_BY_AP_PCT, GetTotalAuraModifier(SPELL_AURA_OVERRIDE_SPELL_POWER_BY_AP_PCT));
        SetStatInt32Value(PLAYER_FIELD_MOD_HEALING_DONE_POS, amount);

        for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            SetStatInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS+i, amount);
    }
    else
    {
        if (GetPowerIndexByClass(POWER_MANA, getClass()) != MAX_POWERS)
            amount += std::max(0, int32(GetStat(STAT_INTELLECT)) - 10);

        int32 spellHeal = SpellBaseHealingBonusDone(SPELL_SCHOOL_MASK_ALL, amount);

        SetStatInt32Value(PLAYER_FIELD_MOD_HEALING_DONE_POS, spellHeal);
        // Get damage bonus for all schools
        for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            SetStatInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS+i, SpellBaseDamageBonusDone(SpellSchoolMask(1 << i), amount));
    }

    if (HasAuraType(SPELL_AURA_OVERRIDE_AP_BY_SPELL_POWER_PCT))
        UpdateAttackPowerAndDamage();

    if (GetPetGUID())
        SetNeedToUpdate(PET_AP_AND_DAMAGE);
}

bool Player::UpdateAllStats()
{
    for (int8 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        float value = GetTotalStatValue(Stats(i));
        SetStat(Stats(i), int32(value));
    }

    UpdateArmor();
    // calls UpdateAttackPowerAndDamage() in UpdateArmor for SPELL_AURA_MOD_ATTACK_POWER_OF_ARMOR
    UpdateAttackPowerAndDamage(true);
    UpdateMaxHealth();

    for (uint8 i = POWER_MANA; i < MAX_POWERS; ++i)
        UpdateMaxPower(Powers(i));

    // Custom MoP script
    // Jab Override Driver
    if (GetTypeId() == TYPEID_PLAYER && getClass() == CLASS_MONK)
    {
        Item* mainItem = GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);

        if (mainItem && mainItem->GetTemplate()->Class == ITEM_CLASS_WEAPON)
        {
            RemoveAura(108561); // 2H Staff Override
            RemoveAura(115697); // 2H Polearm Override
            RemoveAura(115689); // D/W Axes
            RemoveAura(115694); // D/W Maces
            RemoveAura(115696); // D/W Swords

            uint32 spellId = 0;
            switch (mainItem->GetTemplate()->SubClass)
            {
                case ITEM_SUBCLASS_WEAPON_STAFF:
                    spellId = 108561;
                    break;
                case ITEM_SUBCLASS_WEAPON_POLEARM:
                    spellId = 115697;
                    break;
                case ITEM_SUBCLASS_WEAPON_AXE:
                    spellId = 115689;
                    break;
                case ITEM_SUBCLASS_WEAPON_MACE:
                    spellId = 115694;
                    break;
                case ITEM_SUBCLASS_WEAPON_SWORD:
                    spellId = 115696;
                    break;
                default:
                    break;
            }
            if (HasAura(125660)) // Glyph of Jab
                spellId = 108561;
            if (spellId)
                CastSpell(this, spellId, true);
        }
    }

    UpdateAllRatings();
    UpdateAllCritPercentages();
    UpdateSpellCritChance(SPELL_SCHOOL_MASK_ALL);
    UpdateBlockPercentage();
    UpdateParryPercentage();
    UpdateDodgePercentage();
    UpdateSpellDamageAndHealingBonus();
    UpdateManaRegen();
    UpdateExpertise();
    for (int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        UpdateResistances(i);

    return true;
}

void Player::UpdateResistances(uint32 school)
{
    if (school > SPELL_SCHOOL_NORMAL)
    {
        float value  = GetTotalAuraModValue(UnitMods(UNIT_MOD_RESISTANCE_START + school));
        SetResistance(SpellSchools(school), int32(value));

        Pet* pet = GetPet();
        if (pet)
            pet->UpdateResistances(school);
    }
    else
        UpdateArmor();
}

void Player::UpdateArmor()
{
    UnitMods unitMod = UNIT_MOD_ARMOR;

    float value = GetModifierValue(unitMod, BASE_VALUE);                                        // base armor (from items)
    value *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_BASE_RESISTANCE_PCT, 1);     // armor percent from items
    value += GetModifierValue(unitMod, TOTAL_VALUE);

    //add dynamic flat mods
    AuraEffectList const& mResbyIntellect = GetAuraEffectsByType(SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT);
    for (AuraEffectList::const_iterator i = mResbyIntellect.begin(); i != mResbyIntellect.end(); ++i)
    {
        if ((*i)->GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL)
            value += CalculatePct(GetStat(Stats((*i)->GetMiscValueB())), (*i)->GetAmount());
    }

    value *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_RESISTANCE_PCT, 1);

    SetArmor(int32(value));

    Pet* pet = GetPet();
    if (pet)
        pet->UpdateArmor();

    UpdateAttackPowerAndDamage();                           // armor dependent auras update for SPELL_AURA_MOD_ATTACK_POWER_OF_ARMOR
}

float Player::GetHealthBonusFromStamina()
{
    // Taken from PaperDollFrame.lua - 4.3.4.15595
    float ratio = 14.0f;
    if (gtOCTHpPerStaminaEntry const* hpBase = sGtOCTHpPerStaminaStore.LookupEntry(getLevel() - 1))
        ratio = hpBase->ratio;

    float stamina = GetStat(STAT_STAMINA);
    float baseStam = std::min(20.0f, stamina);
    float moreStam = stamina - baseStam;

    return baseStam + moreStam * ratio;
}

void Player::UpdateMaxHealth()
{
    UnitMods unitMod = UNIT_MOD_HEALTH;

    float value = GetModifierValue(unitMod, BASE_VALUE) + GetCreateHealth();
    value *= GetModifierValue(unitMod, BASE_PCT);
    value += GetModifierValue(unitMod, TOTAL_VALUE) + GetHealthBonusFromStamina();
    value *= GetModifierValue(unitMod, TOTAL_PCT);

    SetMaxHealth((uint32)value);

    if(Pet* pet = GetPet())
        if(pet->IsWarlockPet() || pet->isHunterPet())
            pet->UpdateMaxHealth();
}

void Player::UpdateMaxPower(Powers power)
{
    int32 cur_maxpower = GetMaxPower(power);

    float value = GetCreatePowers(power);
    value += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_INCREASE_ENERGY, power);
    value += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_MAX_POWER, power);
    value *= GetTotalAuraMultiplierByMiscValue(SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT, power);
    value *= GetTotalAuraMultiplierByMiscValue(SPELL_AURA_MOD_ADD_ENERGY_PERCENT, power);

    uint32 val = RoundingFloatValue(value);

    if (cur_maxpower != val)
        SetMaxPower(power, val);
}

void Player::UpdateHast(bool _melee, bool _range, bool _spell, bool _cast)
{
//     SPELL_AURA_MELEE_SLOW                  = 0;
//     SPELL_AURA_MOD_MELEE_HASTE             = 1;
//     SPELL_AURA_MOD_MELEE_HASTE_2           = 2;
//     SPELL_AURA_MOD_MELEE_HASTE_3           = 3;
//     SPELL_AURA_MOD_RANGED_HASTE            = 4;
//     SPELL_AURA_MOD_RANGED_HASTE_3          = 5;
//     SPELL_AURA_MOD_MELEE_RANGED_HASTE      = 6;
//     SPELL_AURA_MOD_MELEE_RANGED_HASTE_2    = 7;
//     SPELL_AURA_MOD_CASTING_SPEED           = 8;
//     SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK = 9;
//     SPELL_AURA_HASTE_SPELLS                = 10;
    if (!_melee && !_range && !_spell && !_cast)
        return;

    std::vector<float>hastAurasMod;
    hastAurasMod.assign(11, 1.0f);

    if (_melee)
    {
        hastAurasMod[0] = GetTotalAuraMultiplier(SPELL_AURA_MELEE_SLOW, true);
        hastAurasMod[1] = GetTotalAuraMultiplier(SPELL_AURA_MOD_MELEE_HASTE, true);
        hastAurasMod[2] = GetTotalAuraMultiplier(SPELL_AURA_MOD_MELEE_HASTE_2, true);
        hastAurasMod[3] = GetTotalAuraMultiplier(SPELL_AURA_MOD_MELEE_HASTE_3, true);
        hastAurasMod[6] = GetTotalAuraMultiplier(SPELL_AURA_MOD_MELEE_RANGED_HASTE, true);
        hastAurasMod[7] = GetTotalAuraMultiplier(SPELL_AURA_MOD_MELEE_RANGED_HASTE_2, true);

        UpdateMeleeHastMod(hastAurasMod[0] * hastAurasMod[1] * hastAurasMod[2] * hastAurasMod[3] * hastAurasMod[6] * hastAurasMod[7]);

        if (GetPower(POWER_ENERGY))
            UpdateEnergyRegen(hastAurasMod[0] * hastAurasMod[1] * hastAurasMod[6] * hastAurasMod[7]);

        m_needToUpdate[MELEE_HAST_MODS] = false;
    }

    if (_range)
    {
        hastAurasMod[4] = GetTotalAuraMultiplier(SPELL_AURA_MOD_RANGED_HASTE, true);
        hastAurasMod[5] = GetTotalAuraMultiplier(SPELL_AURA_MOD_RANGED_HASTE_3, true);

        if (!_melee)
        {
            hastAurasMod[6] = GetTotalAuraMultiplier(SPELL_AURA_MOD_MELEE_RANGED_HASTE, true);
            hastAurasMod[7] = GetTotalAuraMultiplier(SPELL_AURA_MOD_MELEE_RANGED_HASTE_2, true);
            hastAurasMod[0] = GetTotalAuraMultiplier(SPELL_AURA_MELEE_SLOW, true);
        }

        UpdateRangeHastMod(hastAurasMod[0] * hastAurasMod[4] * hastAurasMod[5] * hastAurasMod[6] * hastAurasMod[7]);

        if (GetPower(POWER_FOCUS))
            UpdateFocusRegen(hastAurasMod[4] * hastAurasMod[6] * hastAurasMod[0] * hastAurasMod[7]);

        m_needToUpdate[RANGE_HAST_MODS] = false;
    }

    if (_spell)
    {
        hastAurasMod[9] = GetTotalAuraMultiplier(SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, true);
        hastAurasMod[8] = GetTotalAuraMultiplier(SPELL_AURA_MOD_CASTING_SPEED, true);
        hastAurasMod[10] = GetTotalAuraMultiplier(SPELL_AURA_HASTE_SPELLS, true);

        if (!_range && !_melee)
            hastAurasMod[0] = GetTotalAuraMultiplier(SPELL_AURA_MELEE_SLOW, true);

        UpdateHastMod(hastAurasMod[9] * hastAurasMod[8] * hastAurasMod[10] * hastAurasMod[0]);

        m_needToUpdate[HAST_MODS] = false;
    }

    if (_cast)
    {
        if (!_spell)
            hastAurasMod[9] = GetTotalAuraMultiplier(SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK, true);

        if (!_melee && !_range)
            hastAurasMod[7] = GetTotalAuraMultiplier(SPELL_AURA_MOD_MELEE_RANGED_HASTE_2, true);

        if (!_melee && !_range && !_spell)
            hastAurasMod[0] = GetTotalAuraMultiplier(SPELL_AURA_MELEE_SLOW, true);

        UpdateCastHastMods(hastAurasMod[0] * hastAurasMod[7] * hastAurasMod[9]);

        m_needToUpdate[CAST_HAST_MODS] = false;
    }
}

void Player::UpdateCastHastMods(float auraMods)
{
    float amount = 100.0f;
    amount += GetRatingBonusValue(CR_HASTE_MELEE);

    m_baseMHastRatingPct = amount / 100.0f;
    amount *= auraMods;
    amount -= 100.0f;

    float value = 1.0f;

    if (amount > 0)
        ApplyPercentModFloatVar(value, amount, false);
    else
        ApplyPercentModFloatVar(value, -amount, true);

    SetFloatValue(UNIT_MOD_CAST_HASTE, value);

    if (getClass() == CLASS_MONK)
        SetNeedToUpdate(SPELL_HAST_DURATION_RECOVERY);
}

void Player::UpdateMeleeHastMod(float auraMods)
{
    float amount = 100.0f;
    amount += GetRatingBonusValue(CR_HASTE_MELEE);

    m_baseMHastRatingPct = amount / 100.0f;
    amount *= auraMods;
    amount -= 100.0f;

    //sLog->outError(LOG_FILTER_NETWORKIO, "UpdateMeleeHastMod mod %f", mod);

    float value = 1.0f;

    if (amount > 0)
        ApplyPercentModFloatVar(value, amount, false);
    else
        ApplyPercentModFloatVar(value, -amount, true);

    SetFloatValue(UNIT_MOD_HASTE, value);

    for (uint8 i = BASE_ATTACK; i < RANGED_ATTACK; ++i)
        CalcAttackTimePercentMod(WeaponAttackType(i), value);

    Unit::AuraEffectList const& CooldownByMeleeHaste = GetAuraEffectsByType(SPELL_AURA_MOD_SPELL_COOLDOWN_BY_MELEE_HASTE);
    for (Unit::AuraEffectList::const_iterator itr = CooldownByMeleeHaste.begin(); itr != CooldownByMeleeHaste.end(); ++itr)
    {
        (*itr)->SetCanBeRecalculated(true);
        (*itr)->RecalculateAmount(this);
    }
    Unit::AuraEffectList const& GcdByMeleeHaste = GetAuraEffectsByType(SPELL_AURA_MOD_SPELL_GDC_BY_MELEE_HASTE);
    for (Unit::AuraEffectList::const_iterator itr = GcdByMeleeHaste.begin(); itr != GcdByMeleeHaste.end(); ++itr)
    {
        (*itr)->SetCanBeRecalculated(true);
        (*itr)->RecalculateAmount(this);
    }

    if (getClass() == CLASS_DEATH_KNIGHT)
        SetNeedToUpdate(RUNES_REGEN);

    if (Pet* pet = GetPet())
        pet->UpdateMeleeHastMod(auraMods);
}

void Player::UpdateHastMod(float auraMods)
{
    float amount = 100.0f;
    amount += GetRatingBonusValue(CR_HASTE_SPELL);

    m_baseHastRatingPct = amount / 100.0f;

    amount *= auraMods;
    amount -= 100.0f;

    //sLog->outDebug(LOG_FILTER_NETWORKIO, "UpdateHastMod amount %f", amount);

    float value = 1.0f;

    if (amount > 0)
        ApplyPercentModFloatVar(value, amount, false);
    else
        ApplyPercentModFloatVar(value, -amount, true);

    SetFloatValue(UNIT_MOD_CAST_SPEED, value);

    if (getClass() == CLASS_DEATH_KNIGHT)
        SetNeedToUpdate(RUNES_REGEN);

    UpdateManaRegen();

    if (Pet* pet = GetPet())
        pet->UpdateHastMod(auraMods);
}

void Player::UpdateRangeHastMod(float auraMods)
{
    float amount = 100.0f;
    amount += GetRatingBonusValue(CR_HASTE_RANGED);
    m_baseRHastRatingPct = amount / 100.0f;
    amount *= auraMods;
    amount -= 100.0f;

    //sLog->outError(LOG_FILTER_NETWORKIO, "UpdateRangeHastMod mod %f", mod);

    float value = 1.0f;

    if (amount > 0)
        ApplyPercentModFloatVar(value, amount, false);
    else
        ApplyPercentModFloatVar(value, -amount, true);
    SetFloatValue(UNIT_FIELD_MOD_RANGED_HASTE, value);

    CalcAttackTimePercentMod(RANGED_ATTACK, value);

    if (getClass() == CLASS_DEATH_KNIGHT)
        SetNeedToUpdate(RUNES_REGEN);

    if (Pet* pet = GetPet())
        pet->UpdateRangeHastMod(auraMods);
}

void Player::UpdateEnergyRegen(float auraMods)
{
    float val = 1.0f / m_baseMHastRatingPct;
    float amount = 100.0f;
    amount *= auraMods;
    amount -= 100.0f;

    if (amount > 0)
        ApplyPercentModFloatVar(val, amount, false);
    else
        ApplyPercentModFloatVar(val, -amount, true);

    SetFloatValue(UNIT_MOD_HASTE_REGEN, val);

    UpdatePowerRegen(POWER_ENERGY);
}

void Player::UpdateFocusRegen(float auraMods)
{
    float val = 1.0f / m_baseRHastRatingPct;
    float amount = 100.0f;
    amount *= auraMods;
    amount -= 100.0f;

    if (amount > 0)
        ApplyPercentModFloatVar(val, amount, false);
    else
        ApplyPercentModFloatVar(val, -amount, true);

    SetFloatValue(UNIT_MOD_HASTE_REGEN, val);

    UpdatePowerRegen(POWER_FOCUS);
}

void Player::UpdateAttackPowerAndDamage(bool ranged)
{

    UnitMods unitMod = ranged ? UNIT_MOD_ATTACK_POWER_RANGED : UNIT_MOD_ATTACK_POWER;

    uint16 index = UNIT_FIELD_ATTACK_POWER;
    uint16 index_mod_pos = UNIT_FIELD_ATTACK_POWER_MOD_POS;
    uint16 index_mod_neg = UNIT_FIELD_ATTACK_POWER_MOD_NEG;
    uint16 index_mult = UNIT_FIELD_ATTACK_POWER_MULTIPLIER;

    if (ranged)
    {
        index = UNIT_FIELD_RANGED_ATTACK_POWER;
        index_mod_pos = UNIT_FIELD_RANGED_ATTACK_POWER_MOD_POS;
        index_mod_neg = UNIT_FIELD_RANGED_ATTACK_POWER_MOD_NEG;
        index_mult = UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER;
    }

    if (!HasAuraType(SPELL_AURA_OVERRIDE_AP_BY_SPELL_POWER_PCT))
    {
        float val2 = 0.0f;
        float level = float(getLevel());
        ChrClassesEntry const* entry = sChrClassesStore.LookupEntry(getClass());

        if (ranged)
            val2 = (level + std::max(GetStat(STAT_AGILITY) - 10.0f, 0.0f)) * entry->RAPPerAgility;
        else
        {
            float strengthValue = std::max((GetStat(STAT_STRENGTH) - 10.0f) * entry->APPerStrenth, 0.0f);
            float agilityValue = std::max((GetStat(STAT_AGILITY) - 10.0f) * entry->APPerAgility, 0.0f);

            SpellShapeshiftFormEntry const* form = sSpellShapeshiftFormStore.LookupEntry(GetShapeshiftForm());
            // Directly taken from client, SHAPESHIFT_FLAG_AP_FROM_STRENGTH ?
            if (form && form->flags1 & 0x20)
            {
                agilityValue += std::max((GetStat(STAT_AGILITY) - 10.0f) * entry->APPerStrenth, 0.0f);
                // Druid feral has AP per agility = 2
                if (form->ID == FORM_CAT || form->ID == FORM_BEAR)
                    agilityValue *= 2;
            }

            val2 = strengthValue + agilityValue;
        }

        SetModifierValue(unitMod, BASE_VALUE, val2);

        float base_attPower = GetModifierValue(unitMod, BASE_VALUE) * GetModifierValue(unitMod, BASE_PCT);
        float attPowerMod = GetModifierValue(unitMod, TOTAL_VALUE);

        base_attPower += ranged ? GetTotalAuraModifier(SPELL_AURA_MOD_RANGED_ATTACK_POWER) : GetTotalAuraModifier(SPELL_AURA_MOD_ATTACK_POWER);
        base_attPower *= GetTotalAuraMultiplier(ranged ? SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT: SPELL_AURA_MOD_ATTACK_POWER_PCT);

        //add dynamic flat mods
        if (!ranged && HasAuraType(SPELL_AURA_MOD_ATTACK_POWER_OF_ARMOR))
        {
            AuraEffectList const& mAPbyArmor = GetAuraEffectsByType(SPELL_AURA_MOD_ATTACK_POWER_OF_ARMOR);
            for (AuraEffectList::const_iterator iter = mAPbyArmor.begin(); iter != mAPbyArmor.end(); ++iter)
                // always: ((*i)->GetModifier()->m_miscvalue == 1 == SPELL_SCHOOL_MASK_NORMAL)
                attPowerMod += int32(GetArmor() / (*iter)->GetAmount());
        }

        float attPowerMultiplier = GetModifierValue(unitMod, TOTAL_PCT) - 1.0f;

        SetFloatValue(PLAYER_FIELD_OVERRIDE_AP_BY_SPELL_POWER_PCT, 0.0f);

        SetInt32Value(index, (uint32)base_attPower);        //UNIT_FIELD_(RANGED)_ATTACK_POWER field
        SetInt32Value(index_mod_pos, (uint32)attPowerMod);  //UNIT_FIELD_(RANGED)_ATTACK_POWER_MOD_POS field

        SetFloatValue(index_mult, attPowerMultiplier);      //UNIT_FIELD_(RANGED)_ATTACK_POWER_MULTIPLIER field
    }
    else
    {
        float ApBySpellPct = float(GetTotalAuraModifier(SPELL_AURA_OVERRIDE_AP_BY_SPELL_POWER_PCT));
        int32 spellPower = GetSpellPowerDamage();

        SetFloatValue(PLAYER_FIELD_OVERRIDE_AP_BY_SPELL_POWER_PCT, ApBySpellPct);
        SetModifierValue(unitMod, BASE_VALUE, ApBySpellPct / 100.0f * spellPower);
        SetInt32Value(index, ApBySpellPct / 100.0f * spellPower);

        SetInt32Value(index_mod_pos, 0);                    //UNIT_FIELD_(RANGED)_ATTACK_POWER_MOD_POS field
        float attPowerMultiplier = GetModifierValue(unitMod, TOTAL_PCT) - 1.0f;
        if (attPowerMultiplier < 0)
            SetFloatValue(index_mult, attPowerMultiplier);  //UNIT_FIELD_(RANGED)_ATTACK_POWER_MULTIPLIER field
    }
                            
    //automatically update weapon damage after attack power modification
    if (ranged)
    {
        UpdateDamagePhysical(RANGED_ATTACK);
    }
    else
    {
        UpdateDamagePhysical(BASE_ATTACK);
        if (CanDualWield() && haveOffhandWeapon())           //allow update offhand damage only if player knows DualWield Spec and has equipped offhand weapon
            UpdateDamagePhysical(OFF_ATTACK);
        if (getClass() == CLASS_SHAMAN || getClass() == CLASS_PALADIN)                      // mental quickness
            UpdateSpellDamageAndHealingBonus();
    }

    if (HasAuraType(SPELL_AURA_OVERRIDE_SPELL_POWER_BY_AP_PCT))
        UpdateSpellDamageAndHealingBonus();

    if (GetPetGUID()) //update pet's AP
        SetNeedToUpdate(PET_AP_AND_DAMAGE);
}

void Player::CalculateMinMaxDamage(WeaponAttackType attType, bool normalized, bool addTotalPct, float& min_damage, float& max_damage)
{
    UnitMods unitMod;

    switch (attType)
    {
        case BASE_ATTACK:
        default:
            unitMod = UNIT_MOD_DAMAGE_MAINHAND;
            break;
        case OFF_ATTACK:
            unitMod = UNIT_MOD_DAMAGE_OFFHAND;
            break;
        case RANGED_ATTACK:
            unitMod = UNIT_MOD_DAMAGE_RANGED;
            break;
    }

    float att_speed = GetAPMultiplier(attType, normalized);

    float base_value  = GetModifierValue(unitMod, BASE_VALUE) + GetTotalAttackPowerValue(attType) / 14.0f * att_speed;
    float base_pct    = GetModifierValue(unitMod, BASE_PCT);
    float total_value = GetModifierValue(unitMod, TOTAL_VALUE);
    float total_pct = 1.0f;

    if (addTotalPct)
    {
        total_pct *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, SPELL_SCHOOL_MASK_NORMAL);
        SetModifierValue(unitMod, TOTAL_PCT, total_pct);

        if (attType == OFF_ATTACK)
        {
            total_pct *= 0.5f;
            total_pct *= GetTotalAuraMultiplier(SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT);
        }
    }
    

    float weapon_mindamage = GetWeaponDamageRange(attType, MINDAMAGE);
    float weapon_maxdamage = GetWeaponDamageRange(attType, MAXDAMAGE);

    if (IsInFeralForm())
    {
        float weaponSpeed = BASE_ATTACK_TIME / 1000;

        if (Item* weapon = GetWeaponForAttack(BASE_ATTACK, false))
        {
            weaponSpeed = weapon->GetTemplate()->Delay / 1000.0f;
        }
        weapon_mindamage = weapon_mindamage / weaponSpeed * att_speed;
        weapon_maxdamage = weapon_maxdamage / weaponSpeed * att_speed;
    }

    if (!CanUseAttackType(attType))      //check if player not in form but still can't use (disarm case)
    {
        //cannot use ranged/off attack, set values to 0
        if (attType != BASE_ATTACK)
        {
            min_damage = 0;
            max_damage = 0;
            return;
        }
        weapon_mindamage = BASE_MINDAMAGE;
        weapon_maxdamage = BASE_MAXDAMAGE;
    }
    /*
    TODO: Is this still needed after ammo has been removed?
    else if (attType == RANGED_ATTACK)                       //add ammo DPS to ranged damage
    {
        weapon_mindamage += ammo * att_speed;
        weapon_maxdamage += ammo * att_speed;
    }*/

    min_damage = ((base_value + weapon_mindamage) * base_pct + total_value) * total_pct;
    max_damage = ((base_value + weapon_maxdamage) * base_pct + total_value) * total_pct;
}

void Player::UpdateDamagePhysical(WeaponAttackType attType)
{
    float mindamage;
    float maxdamage;

    CalculateMinMaxDamage(attType, false, true, mindamage, maxdamage);

    switch (attType)
    {
        case BASE_ATTACK:
        default:
            SetStatFloatValue(UNIT_FIELD_MINDAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAXDAMAGE, maxdamage);
            break;
        case OFF_ATTACK:
            SetStatFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE, maxdamage);
            break;
        case RANGED_ATTACK:
            SetStatFloatValue(UNIT_FIELD_MINRANGEDDAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE, maxdamage);
            break;
    }
}

void Player::UpdateBlockPercentage()
{
    // No block
    float value = 0.0f;
    if (CanBlock())
    {
        // Base value
        value = 5.0f;
        // Increase from SPELL_AURA_MOD_BLOCK_PERCENT aura
        value += GetTotalAuraModifier(SPELL_AURA_MOD_BLOCK_PERCENT);

        // Increase from rating
        value += GetRatingBonusValue(CR_BLOCK);
        value = value < 0.0f ? 0.0f : value;
    }
    SetStatFloatValue(PLAYER_BLOCK_PERCENTAGE, value);
}

void Player::UpdateCritPercentage(uint32 attTypeMask)
{
    Pet* pet = GetPet();
    float value = GetMeleeCritFromAgility();
    float modCritPct = GetTotalAuraModifier(SPELL_AURA_MOD_CRIT_PCT, true);
    uint16 index;
    float Val = 0.0f;

    for (uint8 i = BASE_ATTACK; i < MAX_ATTACK; i++)
        if (attTypeMask & (1 << i))
        {
            switch (WeaponAttackType(i))
            {
                case OFF_ATTACK:
                {
                    index = PLAYER_OFFHAND_CRIT_PERCENTAGE;

                    Val = value + GetRatingBonusValue(CR_CRIT_MELEE);

                    if (pet)
                        pet->CalcExactCritPctForPets(MELEE_CRIT, Val);

                    Val += modCritPct;
                    SetExactMeleeCritPct(Val);
                    break;
                }
                case RANGED_ATTACK:
                {
                    index = PLAYER_RANGED_CRIT_PERCENTAGE;

                    Val = value + GetRatingBonusValue(CR_CRIT_RANGED);

                    if (pet)
                        pet->CalcExactCritPctForPets(RANGE_CRIT, Val);

                    Val += modCritPct;
                    SetExactRangeCritPct(Val);
                    break;
                }
                case BASE_ATTACK:
                default:
                {
                    index = PLAYER_CRIT_PERCENTAGE;

                    Val = value + GetRatingBonusValue(CR_CRIT_MELEE);

                    if (pet)
                        pet->CalcExactCritPctForPets(MELEE_CRIT, Val);

                    Val += modCritPct;
                    SetExactMeleeCritPct(Val);
                    break;
                }
            }

            Val = Val < 0.0f ? 0.0f : Val;
            SetStatFloatValue(index, Val);
        }
}

void Player::UpdateAllCritPercentages()
{
    float value = GetMeleeCritFromAgility();

    SetBaseModValue(CRIT_PERCENTAGE, PCT_MOD, value);
    SetBaseModValue(OFFHAND_CRIT_PERCENTAGE, PCT_MOD, value);
    SetBaseModValue(RANGED_CRIT_PERCENTAGE, PCT_MOD, value);

    UpdateCritPercentage((1 << BASE_ATTACK) | (1 << OFF_ATTACK) | (1 << RANGED_ATTACK));
}

const float m_diminishing_k[MAX_CLASSES] =
{
    0.9560f,  // Warrior
    0.8860f,  // Paladin
    0.9880f,  // Hunter
    0.9880f,  // Rogue
    0.9560f,  // Priest
    0.9560f,  // DK
    0.9880f,  // Shaman
    0.9830f,  // Mage
    0.9830f,  // Warlock
    1.4220f,  // Monk  @todo: find me !
    1.2220f   // Druid
};

void Player::UpdateParryPercentage()
{
    const float parry_cap[MAX_CLASSES] =
    {
        65.631440f,     // Warrior
        65.631440f,     // Paladin
        0.0f,           // Hunter
        145.560408f,    // Rogue
        0.0f,           // Priest
        65.631440f,     // DK
        145.560408f,    // Shaman
        0.0f,           // Mage
        0.0f,           // Warlock
        91.631440f,     // Monk  @todo: find me !
        0.0f            // Druid
    };

    const bool mainStr[MAX_CLASSES] =
    {
        true,  // Warrior
        true,  // Paladin
        false, // Hunter
        false, // Rogue
        false, // Priest
        true,  // DK
        false, // Shaman
        false, // Mage
        false, // Warlock
        false, // Monk  @todo: find me !
        false  // Druid
    };

    // No parry
    float value = 0.0f;
    uint32 pclass = getClass()-1;
    if (CanParry() && parry_cap[pclass] > 0.0f)
    {
        uint32 pclass = getClass() - 1;
        float baseParry = 3.0f + GetTotalAuraModifier(SPELL_AURA_MOD_PARRY_PERCENT) + GetTotalAuraModifier(SPELL_AURA_MOD_PARRY_PERCENT2);
        float scaling = 951.158596f;
        float base_strength = GetCreateStat(STAT_STRENGTH) * m_auraModifiersGroup[UNIT_MOD_STAT_START + STAT_STRENGTH][BASE_PCT];
        float ratingParry = GetRatingBonusValue(CR_PARRY);
        float total_strength = GetTotalStatValue(STAT_STRENGTH);

        float parryFromStrength = mainStr[pclass] ? ((total_strength - base_strength) / scaling) : 0.0f;
        float parryFromBaseStrength = mainStr[pclass] ? (base_strength / scaling) : 0.0f;

        value = pow((1 / parry_cap[pclass]) + (m_diminishing_k[pclass] / (parryFromStrength + ratingParry)), -1);
        value += baseParry + parryFromBaseStrength;
        value = value < 0.0f ? 0.0f : value;
    }
    SetStatFloatValue(PLAYER_PARRY_PERCENTAGE, value);
}

void Player::UpdateDodgePercentage()
{
    const float dodge_cap[MAX_CLASSES] =
    {
        90.6425f,       // Warrior
        66.5674f,       // Paladin
        145.560408f,    // Hunter
        145.560408f,    // Rogue
        66.5674f,       // Priest
        90.6425f,       // DK
        145.560408f,    // Shaman
        150.375940f,    // Mage
        150.375940f,    // Warlock
        501.25f,        // Monk  @todo: find me !
        150.375940f     // Druid
    };

    const bool mainAgi[MAX_CLASSES] =
    {
        false, // Warrior
        false, // Paladin
        true,  // Hunter
        true,  // Rogue
        false, // Priest
        false, // DK
        true,  // Shaman
        false, // Mage
        false, // Warlock
        true,  // Monk  @todo: find me !
        true   // Druid
    };

    uint32 pclass = getClass() - 1;
    float baseDodge = 3.0f + GetTotalAuraModifier(SPELL_AURA_MOD_DODGE_PERCENT);
    float scaling = 951.158596f;
    float base_agility = GetCreateStat(STAT_AGILITY) * m_auraModifiersGroup[UNIT_MOD_STAT_START + STAT_AGILITY][BASE_PCT];
    float ratingDodge = GetRatingBonusValue(CR_DODGE);
    float total_agility = GetTotalStatValue(STAT_AGILITY);

    float dodgeFromAgility     = mainAgi[pclass] ? ((total_agility - base_agility) / scaling) : 0.0f;
    float dodgeFromBaseAgility = mainAgi[pclass] ? (base_agility / scaling) : 0.0f;

    float val = pow((1 / dodge_cap[pclass]) + (m_diminishing_k[pclass] / (dodgeFromAgility + ratingDodge)), -1);
    val += baseDodge + dodgeFromBaseAgility;
    val = val < 0.0f ? 0.0f : val;

    SetStatFloatValue(PLAYER_DODGE_PERCENTAGE, val);
}

void Player::UpdateSpellCritChance(uint32 schoolMask)
{
    Pet* pet = GetPet();
    float crit = GetSpellCritFromIntellect();
    crit += GetRatingBonusValue(CR_CRIT_SPELL);

    if (pet)
        pet->CalcExactCritPctForPets(SPELL_CRIT, crit);

    float otherCritAuras = GetTotalAuraModifier(SPELL_AURA_MOD_CRIT_PCT, true);
    otherCritAuras += GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_CRIT_CHANCE);

    SetExactSpellCritPct(crit + otherCritAuras);

    for (uint8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
        if (schoolMask & (1 << i))
        {
            if (i == SPELL_SCHOOL_NORMAL)
            {
                SetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1, 0.0f);
                continue;
            }

            float Val = crit;

            Val += (otherCritAuras + GetTotalAuraModifierByMiscMask(SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL, 1 << i));
            SetFloatValue(PLAYER_SPELL_CRIT_PERCENTAGE1 + i, Val);
        }
}

void Player::UpdateMeleeHitChances()
{
    m_modMeleeHitChance = (float)GetTotalAuraModifier(SPELL_AURA_MOD_HIT_CHANCE);
    SetFloatValue(PLAYER_FIELD_UI_HIT_MODIFIER, m_modMeleeHitChance);

    m_modMeleeHitChance += GetRatingBonusValue(CR_HIT_MELEE);

    if (Pet* pet = GetPet())
        pet->CalcExpertiseAndHitChance(this);
}

void Player::UpdateRangedHitChances()
{
    m_modRangedHitChance = (float)GetTotalAuraModifier(SPELL_AURA_MOD_HIT_CHANCE);
    m_modRangedHitChance += GetRatingBonusValue(CR_HIT_RANGED);

    if (Pet* pet = GetPet())
        pet->CalcExpertiseAndHitChance(this);
}

void Player::UpdateSpellHitChances()
{
    m_modSpellHitChance = (float)GetTotalAuraModifier(SPELL_AURA_MOD_SPELL_HIT_CHANCE);
    SetFloatValue(PLAYER_FIELD_UI_SPELL_HIT_MODIFIER, m_modSpellHitChance);
    
    m_modSpellHitChance += GetRatingBonusValue(CR_HIT_SPELL);

    if (Pet* pet = GetPet())
        pet->CalcExpertiseAndHitChance(this);
}

void Player::UpdateExpertise()
{
    bool first = true;
    float expertiseRating = GetRatingBonusValue(CR_EXPERTISE);

    for (uint8 i = BASE_ATTACK; i < MAX_ATTACK; ++i)
    {
        if (getClass() == CLASS_HUNTER)
        {
            if (i != RANGED_ATTACK)
                continue;
        }
        else
        {
            if (i == RANGED_ATTACK)
                continue;
        }
            
        float expertise = expertiseRating;
        Item* weapon = GetWeaponForAttack(WeaponAttackType(i), true);
        AuraEffectList const& expAuras = GetAuraEffectsByType(SPELL_AURA_MOD_EXPERTISE);
        for (AuraEffectList::const_iterator itr = expAuras.begin(); itr != expAuras.end(); ++itr)
        {
            // item neutral spell
            if ((*itr)->GetSpellInfo()->EquippedItemClass == -1)
                expertise += (*itr)->GetAmount();
            // item dependent spell
            else if (weapon && weapon->IsFitToSpellRequirements((*itr)->GetSpellInfo()))
                expertise += (*itr)->GetAmount();
        }

        if (expertise < 0)
            expertise = 0.0f;

        SetFloatValue(PLAYER_EXPERTISE + i, expertise);
    }

    if (Pet* pet = GetPet())
        pet->CalcExpertiseAndHitChance(this);
}

void Player::ApplyManaRegenBonus(int32 amount, bool apply)
{
    _ModifyUInt32(apply, m_baseManaRegen, amount);
    UpdateManaRegen();
}

void Player::ApplyHealthRegenBonus(int32 amount, bool apply)
{
    _ModifyUInt32(apply, m_baseHealthRegen, amount);
}

void Player::UpdateRuneRegen(RuneType rune)
{
    if (rune > NUM_RUNE_TYPES)
        return;

    uint32 cooldown = 0;

    for (uint32 i = 0; i < MAX_RUNES; ++i)
        if (GetBaseRune(i) == rune)
        {
            cooldown = RUNE_BASE_COOLDOWN;
            break;
        }

    if (cooldown <= 0)
        return;

    float regen = float(1 * IN_MILLISECONDS) / float(cooldown);
    SetFloatValue(PLAYER_RUNE_REGEN_1 + uint8(rune), regen);
}

void Player::UpdateAllRunesRegen()
{
    float val = m_baseMHastRatingPct;

    std::list<AuraType> auratypelist;
    auratypelist.push_back(SPELL_AURA_MOD_MELEE_HASTE);
    auratypelist.push_back(SPELL_AURA_MOD_MELEE_RANGED_HASTE);
    auratypelist.push_back(SPELL_AURA_MOD_MELEE_RANGED_HASTE_2);
    auratypelist.push_back(SPELL_AURA_MELEE_SLOW);

    val *= GetTotalForAurasMultiplier(&auratypelist);

    float coef[NUM_RUNE_TYPES];

    for (uint8 i = 0; i < NUM_RUNE_TYPES; i++)
        coef[i] = val;

    AuraEffectList const& mTotalAuraList = GetAuraEffectsByType(SPELL_AURA_MOD_POWER_REGEN_PERCENT);
    for (AuraEffectList::const_iterator i = mTotalAuraList.begin(); i != mTotalAuraList.end(); ++i)
        if ((*i)->GetMiscValue() == POWER_RUNES)
            AddPct(coef[(*i)->GetMiscValueB()], (*i)->GetAmount());

    for (uint8 i = 0; i < NUM_RUNE_TYPES; i++)
    {
        SetRuneCooldownCoef(RuneType(i), coef[i]);
        SetFloatValue(PLAYER_RUNE_REGEN_1 + i, coef[i] / 10.0f);
    }
}

void Player::_ApplyAllStatBonuses()
{
    SetCanModifyStats(false);

    _ApplyAllAuraStatMods();
    _ApplyAllItemMods();

    SetCanModifyStats(true);

    UpdateAllStats();
}

void Player::_RemoveAllStatBonuses()
{
    SetCanModifyStats(false);

    _RemoveAllItemMods();
    _RemoveAllAuraStatMods();

    SetCanModifyStats(true);

    UpdateAllStats();
}

/*#######################################
+ ########                         ########
+ ########   UNITS STAT SYSTEM     ########
+ ########                         ########
+ #######################################*/

void Unit::UpdateManaRegen()
{
    // Skip regeneration for power type we cannot have
    uint32 powerIndex = GetPowerIndexByClass(POWER_MANA, getClass());
    if (powerIndex == MAX_POWERS)
        return;

    // Mana regen from spirit
    float spirit_regen = OCTRegenMPPerSpirit();
    // percent of base mana per 5 sec
    float manaMod = (getClass() == CLASS_WARLOCK) ? 5.0f: (getClass() == CLASS_MAGE) ? 1.0f : 2.0f;

    // manaMod% of base mana every 5 seconds is base for all classes
    float baseRegen = CalculatePct(GetCreateMana(), manaMod) / 5;
    float auraMp5regen = 0.0f;
    AuraEffectList const& ModPowerRegenAuras = GetAuraEffectsByType(SPELL_AURA_MOD_POWER_REGEN);
    for (AuraEffectList::const_iterator i = ModPowerRegenAuras.begin(); i != ModPowerRegenAuras.end(); ++i)
    {
        if (Powers((*i)->GetMiscValue()) == POWER_MANA)
        {
            bool periodic = false;
            if (Aura* aur = (*i)->GetBase())
                if (AuraEffect const* aurEff = aur->GetEffect(1))
                    if(aurEff->GetAuraType() == SPELL_AURA_PERIODIC_DUMMY)
                    {
                        periodic = true;
                        auraMp5regen += aurEff->GetAmount() / 5.0f;
                    }
            if(!periodic)
                auraMp5regen += (*i)->GetAmount() / 5.0f;
        }
    }

    float interruptMod = std::max(float(std::min(GetTotalAuraModifier(SPELL_AURA_MOD_MANA_REGEN_INTERRUPT), 100)), 1.0f);
    float baseMod = std::max(GetTotalAuraMultiplier(SPELL_AURA_MOD_BASE_MANA_REGEN_PERCENT), 1.0f);
    float pctRegenMod = GetTotalAuraMultiplierByMiscValue(SPELL_AURA_MOD_POWER_REGEN_PERCENT, POWER_MANA);
    float regenFromHaste = 1.0f;

    // haste also increase your mana regeneration
    if (HasAuraType(SPELL_AURA_HASTE_AFFECTS_BASE_MANA_REGEN))
        regenFromHaste += (1.0f - GetFloatValue(UNIT_MOD_CAST_HASTE));

    float manaRegen = ((baseRegen * baseMod + auraMp5regen + spirit_regen) * pctRegenMod) * regenFromHaste;
    float manaRegenInterupted = ((baseRegen * baseMod + auraMp5regen + spirit_regen * interruptMod / 100.0f) * pctRegenMod) * regenFromHaste;

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Unit::UpdateManaRegen pctRegenMod %f, regenFromHaste %f, manaRegen %f", pctRegenMod, regenFromHaste, manaRegen);

    // out of combar
    SetStatFloatValue(UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER + powerIndex, manaRegen);
    // in combat
    SetStatFloatValue(UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER + powerIndex, manaRegenInterupted);
}

void Unit::UpdateCastHastMods()
{
    float amount = 100.0f;

    std::list<AuraType> auratypelist;
    auratypelist.push_back(SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK);
    auratypelist.push_back(SPELL_AURA_MOD_MELEE_RANGED_HASTE_2);
    auratypelist.push_back(SPELL_AURA_MELEE_SLOW);

    amount *= GetTotalForAurasMultiplier(&auratypelist);
    amount -= 100.0f;

    float value = 1.0f;

    if (amount > 0)
        ApplyPercentModFloatVar(value, amount, false);
    else
        ApplyPercentModFloatVar(value, -amount, true);

    SetFloatValue(UNIT_MOD_CAST_HASTE, value);
}

void Unit::UpdateMeleeHastMod(float ownerMods)
{
    float amount = 100.0f;
    Unit* owner = GetCharmerOrOwner();
    
    if (owner)
        m_baseMHastRatingPct = owner->m_baseMHastRatingPct;

    amount *= m_baseMHastRatingPct;

    std::list<AuraType> auratypelist;
    auratypelist.push_back(SPELL_AURA_MOD_MELEE_HASTE);
    auratypelist.push_back(SPELL_AURA_MOD_MELEE_HASTE_2);
    auratypelist.push_back(SPELL_AURA_MOD_MELEE_HASTE_3);
    auratypelist.push_back(SPELL_AURA_MOD_MELEE_RANGED_HASTE);
    auratypelist.push_back(SPELL_AURA_MOD_MELEE_RANGED_HASTE_2);
    auratypelist.push_back(SPELL_AURA_MELEE_SLOW);

    if (ownerMods)
        amount *= ownerMods;
    else if (owner)
        amount *= owner->GetTotalForAurasMultiplier(&auratypelist);

    amount *= GetTotalForAurasMultiplier(&auratypelist);

    amount -= 100.0f;

    //sLog->outError(LOG_FILTER_NETWORKIO, "UpdateMeleeHastMod mod %f", mod);

    float value = 1.0f;

    if(amount > 0)
        ApplyPercentModFloatVar(value, amount, false);
    else
        ApplyPercentModFloatVar(value, -amount, true);

    SetFloatValue(UNIT_MOD_HASTE, value);

    for (uint8 i = BASE_ATTACK; i < RANGED_ATTACK; ++i)
        CalcAttackTimePercentMod(WeaponAttackType(i), value);

    if (owner)
        if (owner->getClass() == CLASS_HUNTER && (amount != 0.0f))
            owner->SetFloatValue(PLAYER_FIELD_MOD_PET_HASTE, value);
}

void Unit::UpdateHastMod(float ownerMods)
{
    float amount = 100.0f;
    Unit* owner = GetCharmerOrOwner();

    if (owner)
        m_baseHastRatingPct = owner->m_baseHastRatingPct;

    amount *= m_baseHastRatingPct;

    std::list<AuraType> auratypelist;
    auratypelist.push_back(SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK);
    auratypelist.push_back(SPELL_AURA_MOD_CASTING_SPEED);
    auratypelist.push_back(SPELL_AURA_HASTE_SPELLS);
    auratypelist.push_back(SPELL_AURA_MELEE_SLOW);

    if (ownerMods)
        amount *= ownerMods;
    else if (owner)
        amount *= owner->GetTotalForAurasMultiplier(&auratypelist);

    amount *= GetTotalForAurasMultiplier(&auratypelist);
    
    amount -= 100.0f;
    //sLog->outDebug(LOG_FILTER_NETWORKIO, "UpdateHastMod amount %f", amount);

    float value = 1.0f;

    if (amount > 0)
        ApplyPercentModFloatVar(value, amount, false);
    else
        ApplyPercentModFloatVar(value, -amount, true);

    SetFloatValue(UNIT_MOD_CAST_SPEED, value);

    UpdateManaRegen();
}

void Unit::UpdateRangeHastMod(float ownerMods)
{
    float amount = 100.0f;
    Unit* owner = GetCharmerOrOwner();

    if (owner)
        m_baseRHastRatingPct = owner->m_baseRHastRatingPct;

    amount *= m_baseRHastRatingPct;

    std::list<AuraType> auratypelist;
    auratypelist.push_back(SPELL_AURA_MOD_RANGED_HASTE);
    auratypelist.push_back(SPELL_AURA_MOD_RANGED_HASTE_3);
    auratypelist.push_back(SPELL_AURA_MOD_MELEE_RANGED_HASTE);
    auratypelist.push_back(SPELL_AURA_MOD_MELEE_RANGED_HASTE_2);
    auratypelist.push_back(SPELL_AURA_MELEE_SLOW);

    if (ownerMods)
        amount *= ownerMods;
    else if (owner)
        amount *= owner->GetTotalForAurasMultiplier(&auratypelist);

    amount *= GetTotalForAurasMultiplier(&auratypelist);

    amount -= 100.0f;
    //sLog->outError(LOG_FILTER_NETWORKIO, "UpdateRangeHastMod mod %f", mod);

    float value = 1.0f;

    if (amount > 0)
        ApplyPercentModFloatVar(value, amount, false);
    else
        ApplyPercentModFloatVar(value, -amount, true);
    SetFloatValue(UNIT_FIELD_MOD_RANGED_HASTE, value);

    CalcAttackTimePercentMod(RANGED_ATTACK, value);
}

void Unit::UpdatePowerRegen(uint32 power)
{
    uint32 powerIndex = GetPowerIndexByClass(power, getClass());
    if (powerIndex == MAX_POWERS)
        return;

    float meleeHaste = GetFloatValue(UNIT_MOD_HASTE);
    float addvalue = 0.0f;
    bool regenInCombat = false;
    bool regenNotCombat = false;

    //add value in 1s
    switch (power)
    {
        case POWER_RAGE: // Regenerate Rage
        {
            addvalue -= 25.0f / meleeHaste / 2.0f;
            regenInCombat = true;
            regenNotCombat = true;
            break;
        }
        case POWER_FOCUS: // Regenerate Focus
        {
            addvalue += 1.0f * 5.0f;
            regenInCombat = true;
            regenNotCombat = true;
            break;
        }
        case POWER_ENERGY: // Regenerate Energy
        {
            addvalue += 0.01f * 1000.0f;
            regenInCombat = true;
            regenNotCombat = true;
            break;
        }
        case POWER_RUNIC_POWER: // Regenerate Runic Power
        {
            addvalue -= 30.0f / 2.0f;
            regenNotCombat = true;
            break;
        }
        case POWER_HOLY_POWER:
        case POWER_CHI:
        {
            addvalue -= 1.0f / 10.0f;
            regenNotCombat = true;
            break;
        }
        // Regenerate Demonic Fury
        case POWER_DEMONIC_FURY:
        {
            addvalue -= 1.0f * 12.5f;
            regenNotCombat = true;
            break;
        }
        // Regenerate Burning Embers
        case POWER_BURNING_EMBERS:
        {
            addvalue -= 1.0f / 2.5f;
            regenNotCombat = true;
            break;
        }
        // Regenerate Soul Shards
        case POWER_SOUL_SHARDS:
        {
            addvalue += 100.0f / 20.0f;
            regenNotCombat = true;
            break;
        }
        // Regenerate Soul Shards
        case POWER_ECLIPSE:
        {
            addvalue += 1.0f;
            regenInCombat = true;
            break;
        }
        default:
            break;
    }

    float val = (addvalue * GetTotalAuraMultiplierByMiscValue(SPELL_AURA_MOD_POWER_REGEN_PERCENT, power) - addvalue);
    val += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_POWER_REGEN, power) / 5.0f;

    if(regenNotCombat)
        SetFloatValue(UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER + powerIndex, val);
    if(regenInCombat)
        SetFloatValue(UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER + powerIndex, val);

    //sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "Unit::UpdatePowerRegen val %f, powerIndex %i, power %i, addvalue %f", val, powerIndex, power, addvalue);
}

/*#######################################
########                         ########
########    MOBS STAT SYSTEM     ########
########                         ########
#######################################*/

bool Creature::UpdateStats(Stats /*stat*/)
{
    return true;
}

bool Creature::UpdateAllStats()
{
    UpdateMaxHealth();
    UpdateAttackPowerAndDamage();
    UpdateAttackPowerAndDamage(true);

    // not do iteration powers for creatures. Now it has 2 indexes 0 - main, 1 - alt
    //for (uint8 i = POWER_MANA; i < MAX_POWERS; ++i)
    //    UpdateMaxPower(Powers(i));
    UpdateMaxPower(getPowerType());
    UpdateMaxPower(POWER_ALTERNATE_POWER);

    for (int8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        UpdateResistances(i);

    return true;
}

void Creature::UpdateResistances(uint32 school)
{
    if (school > SPELL_SCHOOL_NORMAL)
    {
        float value  = GetTotalAuraModValue(UnitMods(UNIT_MOD_RESISTANCE_START + school));
        SetResistance(SpellSchools(school), int32(value));
    }
    else
        UpdateArmor();
}

void Creature::UpdateArmor()
{
    float value = GetTotalAuraModValue(UNIT_MOD_ARMOR);
    SetArmor(int32(value));
}

void Creature::UpdateMaxHealth()
{
    float value = GetTotalAuraModValue(UNIT_MOD_HEALTH);
    float mod = 1.0f;
    float percHealth = GetHealthPct();
    if (GetMap()->GetInstanceId() == 0)
    {
        uint32 count = GetSizeSaveThreat();
        if(IsPersonalLoot() && count)
        {
            mod += _GetHealthModPersonal(count) * count;
            value *= mod;
        }
        SetMaxHealth((uint32)value);

        if(IsPersonalLoot() && count)
        {
            int32 health = CalculatePct(GetMaxHealth(), percHealth);
            SetHealth(health);
        }
        return;
    }
    else if(GetMap()->IsNeedRecalc())
    {
        if(uint32 count = GetPlayerCount())
        {
            mod += _GetHealthModPersonal(count) * count;
            value *= mod;

            SetMaxHealth((uint32)value);

            int32 health = CalculatePct(GetMaxHealth(), percHealth);
            SetHealth(health);
        }
        return;
    }
    SetMaxHealth((uint32)value);
}

void Creature::UpdateMaxPower(Powers power)
{
    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + power);

    float value  = GetTotalAuraModValue(unitMod);
    SetMaxPower(power, uint32(value));
}

void Creature::UpdateAttackPowerAndDamage(bool ranged)
{
    UnitMods unitMod = ranged ? UNIT_MOD_ATTACK_POWER_RANGED : UNIT_MOD_ATTACK_POWER;

    uint16 index = UNIT_FIELD_ATTACK_POWER;
    uint16 index_mult = UNIT_FIELD_ATTACK_POWER_MULTIPLIER;

    if (ranged)
    {
        index = UNIT_FIELD_RANGED_ATTACK_POWER;
        index_mult = UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER;
    }

    float base_attPower  = GetModifierValue(unitMod, BASE_VALUE) * GetModifierValue(unitMod, BASE_PCT);
    float attPowerMultiplier = GetModifierValue(unitMod, TOTAL_PCT) - 1.0f;

    SetInt32Value(index, (uint32)base_attPower);            //UNIT_FIELD_(RANGED)_ATTACK_POWER field
    SetFloatValue(index_mult, attPowerMultiplier);          //UNIT_FIELD_(RANGED)_ATTACK_POWER_MULTIPLIER field

    //automatically update weapon damage after attack power modification
    if (ranged)
        UpdateDamagePhysical(RANGED_ATTACK);
    else
    {
        UpdateDamagePhysical(BASE_ATTACK);
        UpdateDamagePhysical(OFF_ATTACK);
    }
}

void Creature::UpdateDamagePhysical(WeaponAttackType attType)
{
    UnitMods unitMod;
    switch (attType)
    {
        case BASE_ATTACK:
        default:
            unitMod = UNIT_MOD_DAMAGE_MAINHAND;
            break;
        case OFF_ATTACK:
            unitMod = UNIT_MOD_DAMAGE_OFFHAND;
            break;
        case RANGED_ATTACK:
            unitMod = UNIT_MOD_DAMAGE_RANGED;
            break;
    }

    //float att_speed = float(GetAttackTime(attType))/1000.0f;

    float weapon_mindamage = GetWeaponDamageRange(attType, MINDAMAGE);
    float weapon_maxdamage = GetWeaponDamageRange(attType, MAXDAMAGE);

    /* difference in AP between current attack power and base value from DB */
    float att_pwr_change = GetTotalAttackPowerValue(attType) - GetCreatureTemplate()->attackpower;
    float base_value  = GetModifierValue(unitMod, BASE_VALUE) + (att_pwr_change * GetAPMultiplier(attType, false) / 14.0f);
    float base_pct    = GetModifierValue(unitMod, BASE_PCT);
    float total_value = GetModifierValue(unitMod, TOTAL_VALUE);
    float total_pct   = GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, SPELL_SCHOOL_MASK_NORMAL);
    SetModifierValue(unitMod, TOTAL_PCT, total_pct);
    float dmg_multiplier = GetCreatureTemplate()->dmg_multiplier * _GetDamageModMulti(cInfo->rank, GetMapId());
    if (CreatureDifficultyStat const* _stats = GetCreatureDiffStat())
    {
        switch (GetMobDifficulty())
        {
            case 1:
            {
                dmg_multiplier *= _stats->dmg_multiplier;
                break;
            }
            case 2:
            {
                if (RoundingFloatValue(_stats->dmg_multiplier * 100.f) != 115)
                    dmg_multiplier *= _stats->dmg_multiplier;
                else
                    dmg_multiplier *= 1.3f;
                break;
            }
            case 3:
            {
                if (RoundingFloatValue(_stats->dmg_multiplier * 100.f) != 115)
                    dmg_multiplier *= _stats->dmg_multiplier;
                else
                    dmg_multiplier *= 1.45f;
                break;
            }
            default:
                break;
        }
    }

    if (!CanUseAttackType(attType))
    {
        weapon_mindamage = 0;
        weapon_maxdamage = 0;
    }

    float mindamage = ((base_value + weapon_mindamage) * dmg_multiplier * base_pct + total_value) * total_pct;
    float maxdamage = ((base_value + weapon_maxdamage) * dmg_multiplier * base_pct + total_value) * total_pct;

    switch (attType)
    {
        case BASE_ATTACK:
        default:
            SetStatFloatValue(UNIT_FIELD_MINDAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAXDAMAGE, maxdamage);
            break;
        case OFF_ATTACK:
            SetStatFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE, maxdamage);
            break;
        case RANGED_ATTACK:
            SetStatFloatValue(UNIT_FIELD_MINRANGEDDAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAXRANGEDDAMAGE, maxdamage);
            break;
    }
}

/*#######################################
########                         ########
########    PETS STAT SYSTEM     ########
########                         ########
#######################################*/

bool Guardian::UpdateStats(Stats stat)
{
    if (stat >= MAX_STATS)
        return false;

    float value  = GetTotalStatValue(stat);
    ApplyStatBuffMod(stat, m_statFromOwner[stat], false);
    float ownersBonus = 0.0f;

    Unit* owner = GetOwner();
    // Handle Death Knight Glyphs and Talents
    float mod = 0.75f;

    switch (stat)
    {
        case STAT_STAMINA:
        {
            mod = 0.3f;

            if (IsPetGhoul() || IsPetGargoyle())
                mod = 0.45f;
            else if (owner->getClass() == CLASS_WARLOCK && isPet())
                mod = 0.75f;
            else if (owner->getClass() == CLASS_MAGE && isPet())
                mod = 0.75f;
            else
            {
                mod = 0.45f;

                if (isPet())
                {
                    switch (ToPet()->GetSpecializationId())
                    {
                        case SPEC_PET_FEROCITY: mod = 0.67f; break;
                        case SPEC_PET_TENACITY: mod = 0.78f; break;
                        case SPEC_PET_CUNNING: mod = 0.725f; break;
                    }
                }
            }

            ownersBonus = float(owner->GetStat(stat)) * mod;
            ownersBonus *= GetModifierValue(UNIT_MOD_STAT_STAMINA, TOTAL_PCT);
            value += ownersBonus;
            break;
        }
        case STAT_STRENGTH:
        {
            mod = 0.7f;

            ownersBonus = owner->GetStat(stat) * mod;
            value += ownersBonus;
            break;
        }
        case STAT_INTELLECT:
        {
            mod = 0.3f;

            ownersBonus = owner->GetStat(stat) * mod;
            value += ownersBonus;
            break;
        }
        default:
            break;
    }

    SetStat(stat, int32(value));
    m_statFromOwner[stat] = ownersBonus;
    ApplyStatBuffMod(stat, m_statFromOwner[stat], true);

    switch (stat)
    {
        case STAT_STRENGTH:
            UpdateAttackPowerAndDamage();
            break;
        case STAT_AGILITY:
            UpdateArmor();
            break;
        case STAT_STAMINA:
            UpdateMaxHealth();
            break;
        case STAT_INTELLECT:
        {
            if (getPowerType() == POWER_MANA)
                UpdateMaxPower(POWER_MANA);
            if (owner->getClass() == CLASS_MAGE)
                UpdateAttackPowerAndDamage();
            break;
        }
        case STAT_SPIRIT:
        default:
            break;
    }

    return true;
}

bool Guardian::UpdateAllStats()
{
    for (uint8 i = STAT_STRENGTH; i < MAX_STATS; ++i)
        UpdateStats(Stats(i));

    // not do iteration powers for creatures. Now it has 2 indexes 0 - main, 1 - alt
    //for (uint8 i = POWER_MANA; i < MAX_POWERS; ++i)
    //    UpdateMaxPower(Powers(i));
    UpdateMaxPower(getPowerType());
    UpdateMaxPower(POWER_ALTERNATE_POWER);

    for (uint8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; ++i)
        UpdateResistances(i);

    return true;
}

void Guardian::UpdateResistances(uint32 school)
{
    if (school > SPELL_SCHOOL_NORMAL)
    {
        float value;
        if (!isHunterPet())
        {
            value = GetTotalAuraModValue(UnitMods(UNIT_MOD_RESISTANCE_START + school));

            // hunter and warlock pets gain 40% of owner's resistance
            if (isPet())
                value += float(CalculatePct(m_owner->GetResistance(SpellSchools(school)), 40));
        }
        else
            value = m_owner->GetResistance(SpellSchools(school));

        SetResistance(SpellSchools(school), int32(value));
    }
    else
        UpdateArmor();
}

void Guardian::UpdateArmor()
{
    float value = 0.0f;
    UnitMods unitMod = UNIT_MOD_ARMOR;
    uint32 creature_ID = isHunterPet() ? 1 : GetEntry();

    if (PetStats const* pStats = sObjectMgr->GetPetStats(creature_ID))
        value = m_owner->GetModifierValue(unitMod, BASE_VALUE) * pStats->armor;
    else
        value = m_owner->GetModifierValue(unitMod, BASE_VALUE);

    //sLog->outDebug(LOG_FILTER_PETS, "Guardian::UpdateArmor value %f creature_ID %i", value, creature_ID);

    value += GetModifierValue(unitMod, BASE_PCT);
    value *= GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_RESISTANCE_PCT, 1);
    value *= m_owner->GetTotalAuraMultiplierByMiscValueB(SPELL_AURA_MOD_PET_STATS_MODIFIER, int32(PETSPELLMOD_ARMOR), GetEntry());

    SetArmor(int32(value));
}

void Guardian::UpdateMaxHealth()
{
    float multiplicator = 0.3f;
    float value;

    if(Unit* owner = GetOwner())
    {
        uint32 creature_ID = isHunterPet() ? 1 : GetEntry();
        if (PetStats const* pStats = sObjectMgr->GetPetStats(creature_ID))
        {
            if(pStats->hp)
                multiplicator = pStats->hp;
        }
        else
        {
            SetMaxHealth(GetCreateHealth());
            return;
        }

        multiplicator *= owner->GetTotalAuraMultiplier(SPELL_AURA_MOD_PET_HEALTH_FROM_OWNER_PCT);
        multiplicator *= owner->GetTotalAuraMultiplierByMiscValueB(SPELL_AURA_MOD_PET_STATS_MODIFIER, int32(PETSPELLMOD_MAX_HP), GetEntry());
        value = owner->GetMaxHealth() * multiplicator;

        UnitMods unitMod = UNIT_MOD_HEALTH;
        //value += GetModifierValue(unitMod, BASE_VALUE);
        //value *= GetModifierValue(unitMod, BASE_PCT);
        //value += GetModifierValue(unitMod, TOTAL_VALUE);
        value *= GetModifierValue(unitMod, TOTAL_PCT);

        //sLog->outDebug(LOG_FILTER_PETS, "Guardian::UpdateMaxHealth multiplicator %f creature_ID %i hp %f", multiplicator, creature_ID, value);
    }
    else
    {
        UnitMods unitMod = UNIT_MOD_HEALTH;
        float stamina = GetStat(STAT_STAMINA) - GetCreateStat(STAT_STAMINA);
        multiplicator = 8.0f;
        value = GetModifierValue(unitMod, BASE_VALUE) + GetCreateHealth();
        value *= GetModifierValue(unitMod, BASE_PCT);
        value += GetModifierValue(unitMod, TOTAL_VALUE) + stamina * multiplicator;
        value *= GetModifierValue(unitMod, TOTAL_PCT);
    }

    SetMaxHealth((uint32)value);
}

void Guardian::UpdateMaxPower(Powers power)
{
    bool hasEnergy = true;
    uint32 val = 0;
    int32 cur_maxpower = GetMaxPower(power);
    uint32 creature_ID = isHunterPet() ? 1 : GetEntry();

    if (PetStats const* pStats = sObjectMgr->GetPetStats(creature_ID))
        if (!pStats->energy && pStats->energy_type == 1)
        {
            val = pStats->energy;
            hasEnergy = false;
        }

    if (hasEnergy)
    {
        float value = GetCreatePowers(power);
        value += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_INCREASE_ENERGY, power);
        value += GetTotalAuraModifierByMiscValue(SPELL_AURA_MOD_MAX_POWER, power);
        value *= GetTotalAuraMultiplierByMiscValue(SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT, power);
        value *= GetTotalAuraMultiplierByMiscValue(SPELL_AURA_MOD_ADD_ENERGY_PERCENT, power);

        val = RoundingFloatValue(value);
    }

    //sLog->outDebug(LOG_FILTER_PETS, "Guardian::UpdateMaxPower value %f creature_ID %i", value, creature_ID);

    if (cur_maxpower != val)
        SetMaxPower(power, val);
}

void Guardian::UpdateAttackPowerAndDamage(bool ranged)
{
    if (ranged || isMonkClones())
        return;

    float val = 0.0f;
    float AP = 0.0f; // Attack Power
    uint32 SPD = 0.0f; // Pet Spell Power
    UnitMods unitMod = UNIT_MOD_ATTACK_POWER;

    Unit* owner = GetOwner();
    if (owner && owner->GetTypeId() == TYPEID_PLAYER)
    {
        uint32 creature_ID = isHunterPet() ? 1 : GetEntry();

        if (PetStats const* pStats = sObjectMgr->GetPetStats(creature_ID))
        {
            int32 temp_ap = owner->GetTotalAttackPowerValue(WeaponAttackType(pStats->ap_type));
            int32 school_spd = 0;
            int32 school_temp_spd = 0;
            for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                if(pStats->school_mask & (1 << i))
                {
                    school_temp_spd = owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i) + owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i);
                    if(school_temp_spd > school_spd)
                        school_spd = school_temp_spd;
                }
            }

            if(pStats->ap > 0)
                AP = int32(temp_ap * pStats->ap);
            else if(pStats->ap < 0)
            {
                if(school_spd)
                    AP = -int32(school_spd * pStats->ap);
            }
            if(pStats->spd < 0)
                SPD = -int32(temp_ap * pStats->spd);
            else if(pStats->spd > 0)
            {
                if(school_spd)
                    SPD = int32(school_spd * pStats->spd);
            }
            if(pStats->maxspdorap > 0)
            {
                AP = temp_ap > school_spd ? int32(temp_ap * fabs(pStats->ap)) : int32(school_spd * fabs(pStats->spd));
                SPD = temp_ap > school_spd ? int32(temp_ap * fabs(pStats->ap)) : int32(school_spd * fabs(pStats->spd));
            }
            else if(pStats->maxspdorap < 0)
            {
                AP = int32(((temp_ap + school_spd) / 2) * fabs(pStats->ap));
                SPD = int32(((temp_ap + school_spd) / 2) * fabs(pStats->spd));
            }
        }
        else
        {
            int32 school_temp_spd = 0;
            int32 school_spd = 0;
            for (int i = SPELL_SCHOOL_HOLY; i < MAX_SPELL_SCHOOL; ++i)
            {
                school_temp_spd = owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS + i) + owner->GetUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG + i);
                if(school_temp_spd > school_spd)
                    school_spd = school_temp_spd;
            }
            if(school_spd)
                SPD = school_spd;
        }

        switch (GetEntry())
        {
            case ENTRY_RUNE_WEAPON:
            {
                if (AuraEffect const* aurEff = owner->GetAuraEffect(63330, 1))
                    AP += int32((AP * aurEff->GetAmount()) / 100);
                break;
            }
        }

        AddPct(AP, GetMaxPositiveAuraModifier(SPELL_AURA_MOD_ATTACK_POWER_PCT));

        //UNIT_FIELD_(RANGED)_ATTACK_POWER field
        SetInt32Value(UNIT_FIELD_ATTACK_POWER, AP);

        //PLAYER_PET_SPELL_POWER field
        SetBonusDamage(SPD);
    }
    else
    {
        SetModifierValue(UNIT_MOD_ATTACK_POWER, BASE_VALUE, 0.0f);

        //in BASE_VALUE of UNIT_MOD_ATTACK_POWER for creatures we store data of meleeattackpower field in DB
        float base_attPower = GetModifierValue(unitMod, BASE_VALUE) * GetModifierValue(unitMod, BASE_PCT);
        float attPowerMultiplier = GetModifierValue(unitMod, TOTAL_PCT) - 1.0f;

        //UNIT_FIELD_(RANGED)_ATTACK_POWER field
        SetInt32Value(UNIT_FIELD_ATTACK_POWER, (int32)base_attPower);
        //UNIT_FIELD_(RANGED)_ATTACK_POWER_MULTIPLIER field
        SetFloatValue(UNIT_FIELD_ATTACK_POWER_MULTIPLIER, attPowerMultiplier);
    }

    //automatically update weapon damage after attack power modification
    UpdateDamagePhysical(BASE_ATTACK);
    UpdateDamagePhysical(OFF_ATTACK);
}

void Guardian::UpdateDamagePhysical(WeaponAttackType attType)
{
    if (attType > OFF_ATTACK || isMonkClones())
        return;

    float APCoefficient = 11.f;
    UnitMods unitMod = UNIT_MOD_DAMAGE_MAINHAND;

    float att_speed = BASE_ATTACK_TIME / 1000.0f;

    //float base_value  = GetModifierValue(unitMod, BASE_VALUE) + GetTotalAttackPowerValue(attType) / APCoefficient * att_speed;
    float base_value  = GetModifierValue(unitMod, BASE_VALUE) + (GetTotalAttackPowerValue(attType) * att_speed / APCoefficient);
    float base_pct    = GetModifierValue(unitMod, BASE_PCT);
    float total_value = GetModifierValue(unitMod, TOTAL_VALUE);
    float total_pct   = GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE, SPELL_SCHOOL_MASK_NORMAL);

    SetModifierValue(unitMod, TOTAL_PCT, total_pct);

    float weapon_mindamage = GetWeaponDamageRange(BASE_ATTACK, MINDAMAGE);
    float weapon_maxdamage = GetWeaponDamageRange(BASE_ATTACK, MAXDAMAGE);

    float mindamage = ((base_value + weapon_mindamage) * base_pct + total_value) * total_pct;
    float maxdamage = ((base_value + weapon_maxdamage) * base_pct + total_value) * total_pct;

    switch (attType)
    {
        case BASE_ATTACK:
        default:
            SetStatFloatValue(UNIT_FIELD_MINDAMAGE, mindamage);
            SetStatFloatValue(UNIT_FIELD_MAXDAMAGE, maxdamage);
            break;
        case OFF_ATTACK:
            SetStatFloatValue(UNIT_FIELD_MINOFFHANDDAMAGE, mindamage / 2);
            SetStatFloatValue(UNIT_FIELD_MAXOFFHANDDAMAGE, maxdamage / 2);
            break;
    }
}

void Guardian::SetBonusDamage(int32 damage)
{
    m_bonusSpellDamage = damage;
    if (GetOwner()->GetTypeId() == TYPEID_PLAYER)
        GetOwner()->SetUInt32Value(PLAYER_PET_SPELL_POWER, damage);
}

void Player::UpdateMasteryAuras()
{
    if (!HasAuraType(SPELL_AURA_MASTERY))
    {
        SetFloatValue(PLAYER_MASTERY, 0.0f);
        return;
    }

    float masteryValue = GetTotalAuraModifier(SPELL_AURA_MASTERY) + GetRatingBonusValue(CR_MASTERY);
    SetFloatValue(PLAYER_MASTERY, masteryValue);

    // TODO: rewrite 115556 Master Demonologist

    std::set<uint32> const* masterySpells = GetSpecializationMasterySpells(GetSpecializationId(GetActiveSpec()));
    if (!masterySpells)
        return;

    for (std::set<uint32>::const_iterator itr = masterySpells->begin(); itr != masterySpells->end(); ++itr)
    {
        Aura* aura = GetAura(*itr);
        if (!aura)
            continue;

        // update aura modifiers
        for (uint32 j = 0; j < MAX_SPELL_EFFECTS; ++j)
        {
            AuraEffect* auraEff = aura->GetEffect(j);
            if (!auraEff)
                continue;

            auraEff->SetCanBeRecalculated(true);
            auraEff->RecalculateAmount(this);
        }
    }
}

void Player::UpdatePvPPower()
{
    float  PowerPct  = GetRatingBonusValue(CR_PVP_POWER);
    float  Damage    = 0;
    float  Heal      = 0;
    uint16 DamagePct = 0;
    uint16 HealPct   = 0;
    
    switch (GetSpecializationId(GetActiveSpec()))
    {
        case SPEC_MAGE_ARCANE:
        case SPEC_MAGE_FIRE:
        case SPEC_MAGE_FROST:
        case SPEC_WARRIOR_ARMS:
        case SPEC_WARRIOR_FURY:
        case SPEC_WARRIOR_PROTECTION:
        case SPEC_PALADIN_PROTECTION:
        case SPEC_DRUID_BEAR:
        case SPEC_DK_BLOOD:
        case SPEC_DK_FROST:
        case SPEC_DK_UNHOLY:
        case SPEC_HUNTER_BEASTMASTER:
        case SPEC_HUNTER_MARKSMAN:
        case SPEC_HUNTER_SURVIVAL:
        case SPEC_ROGUE_ASSASSINATION:
        case SPEC_ROGUE_COMBAT:
        case SPEC_ROGUE_SUBTLETY:
        case SPEC_WARLOCK_AFFLICTION:
        case SPEC_WARLOCK_DEMONOLOGY:
        case SPEC_WARLOCK_DESTRUCTION:
        case SPEC_MONK_BREWMASTER:
        {
            DamagePct = 100;
            HealPct   =  40;
            break;
        }
        case SPEC_SHAMAN_ELEMENTAL:
        case SPEC_SHAMAN_ENHANCEMENT:
        case SPEC_PALADIN_RETRIBUTION:
        case SPEC_DRUID_BALANCE:
        case SPEC_DRUID_CAT:
        case SPEC_PRIEST_SHADOW:
        case SPEC_MONK_WINDWALKER:
        {
            DamagePct = 100;
            HealPct   =  70;
            break;
        }
        case SPEC_PALADIN_HOLY:
        case SPEC_DRUID_RESTORATION:
        case SPEC_PRIEST_DISCIPLINE:
        case SPEC_PRIEST_HOLY:
        case SPEC_SHAMAN_RESTORATION:
        case SPEC_MONK_MISTWEAVER:
        {
            HealPct   = 100;
            break;
        }
        default:
            break;
    }

    Damage = CalculatePct(PowerPct, DamagePct);
    Heal   = CalculatePct(PowerPct,   HealPct);

    SetFloatValue(PLAYER_PVP_POWER_DAMAGE,  Damage);
    SetFloatValue(PLAYER_PVP_POWER_HEALING,   Heal);
}
