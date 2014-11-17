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

#include "Common.h"
#include "WorldPacket.h"
#include "Opcodes.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Player.h"
#include "Unit.h"
#include "ObjectAccessor.h"
#include "Util.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "Battleground.h"
#include "OutdoorPvPMgr.h"
#include "Formulas.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CellImpl.h"
#include "ScriptMgr.h"
#include "Vehicle.h"
#include "Battlefield.h"
#include "BattlefieldMgr.h"
#include "WeatherMgr.h"
#include "AreaTrigger.h"

class Aura;
//
// EFFECT HANDLER NOTES
//
// in aura handler there should be check for modes:
// AURA_EFFECT_HANDLE_REAL set
// AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK set
// AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK set - aura is recalculated or is just applied/removed - need to redo all things related to m_amount
// AURA_EFFECT_HANDLE_CHANGE_AMOUNT_SEND_FOR_CLIENT_MASK - logical or of above conditions
// AURA_EFFECT_HANDLE_STAT - set when stats are reapplied
// such checks will speedup trinity change amount/send for client operations
// because for change amount operation packets will not be send
// aura effect handlers shouldn't contain any AuraEffect or Aura object modifications

pAuraEffectHandler AuraEffectHandler[TOTAL_AURAS]=
{
    &AuraEffect::HandleNULL,                                      //  0 SPELL_AURA_NONE
    &AuraEffect::HandleBindSight,                                 //  1 SPELL_AURA_BIND_SIGHT
    &AuraEffect::HandleModPossess,                                //  2 SPELL_AURA_MOD_POSSESS
    &AuraEffect::HandleNoImmediateEffect,                         //  3 SPELL_AURA_PERIODIC_DAMAGE implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleAuraDummy,                                 //  4 SPELL_AURA_DUMMY
    &AuraEffect::HandleModConfuse,                                //  5 SPELL_AURA_MOD_CONFUSE
    &AuraEffect::HandleModCharm,                                  //  6 SPELL_AURA_MOD_CHARM
    &AuraEffect::HandleModFear,                                   //  7 SPELL_AURA_MOD_FEAR
    &AuraEffect::HandleNoImmediateEffect,                         //  8 SPELL_AURA_PERIODIC_HEAL implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleModAttackSpeed,                            //  9 SPELL_AURA_MOD_ATTACKSPEED
    &AuraEffect::HandleModThreat,                                 // 10 SPELL_AURA_MOD_THREAT
    &AuraEffect::HandleModTaunt,                                  // 11 SPELL_AURA_MOD_TAUNT
    &AuraEffect::HandleAuraModStun,                               // 12 SPELL_AURA_MOD_STUN
    &AuraEffect::HandleModDamageDone,                             // 13 SPELL_AURA_MOD_DAMAGE_DONE
    &AuraEffect::HandleNoImmediateEffect,                         // 14 SPELL_AURA_MOD_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonus and Unit::SpellDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         // 15 SPELL_AURA_DAMAGE_SHIELD    implemented in Unit::DoAttackDamage
    &AuraEffect::HandleModStealth,                                // 16 SPELL_AURA_MOD_STEALTH
    &AuraEffect::HandleModStealthDetect,                          // 17 SPELL_AURA_MOD_DETECT
    &AuraEffect::HandleModInvisibility,                           // 18 SPELL_AURA_MOD_INVISIBILITY
    &AuraEffect::HandleModInvisibilityDetect,                     // 19 SPELL_AURA_MOD_INVISIBILITY_DETECTION
    &AuraEffect::HandleNoImmediateEffect,                         // 20 SPELL_AURA_OBS_MOD_HEALTH implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNoImmediateEffect,                         // 21 SPELL_AURA_OBS_MOD_POWER implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleAuraModResistance,                         // 22 SPELL_AURA_MOD_RESISTANCE
    &AuraEffect::HandleNoImmediateEffect,                         // 23 SPELL_AURA_PERIODIC_TRIGGER_SPELL implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNoImmediateEffect,                         // 24 SPELL_AURA_PERIODIC_ENERGIZE implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleAuraModPacify,                             // 25 SPELL_AURA_MOD_PACIFY
    &AuraEffect::HandleAuraModRoot,                               // 26 SPELL_AURA_MOD_ROOT
    &AuraEffect::HandleAuraModSilence,                            // 27 SPELL_AURA_MOD_SILENCE
    &AuraEffect::HandleNoImmediateEffect,                         // 28 SPELL_AURA_REFLECT_SPELLS        implement in Unit::SpellHitResult
    &AuraEffect::HandleAuraModStat,                               // 29 SPELL_AURA_MOD_STAT
    &AuraEffect::HandleAuraModSkill,                              // 30 SPELL_AURA_MOD_SKILL
    &AuraEffect::HandleAuraModIncreaseSpeed,                      // 31 SPELL_AURA_MOD_INCREASE_SPEED
    &AuraEffect::HandleAuraModIncreaseMountedSpeed,               // 32 SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED
    &AuraEffect::HandleAuraModDecreaseSpeed,                      // 33 SPELL_AURA_MOD_DECREASE_SPEED
    &AuraEffect::HandleAuraModIncreaseHealth,                     // 34 SPELL_AURA_MOD_INCREASE_HEALTH
    &AuraEffect::HandleAuraModIncreaseEnergy,                     // 35 SPELL_AURA_MOD_INCREASE_ENERGY
    &AuraEffect::HandleAuraModShapeshift,                         // 36 SPELL_AURA_MOD_SHAPESHIFT
    &AuraEffect::HandleAuraModEffectImmunity,                     // 37 SPELL_AURA_EFFECT_IMMUNITY
    &AuraEffect::HandleAuraModStateImmunity,                      // 38 SPELL_AURA_STATE_IMMUNITY
    &AuraEffect::HandleAuraModSchoolImmunity,                     // 39 SPELL_AURA_SCHOOL_IMMUNITY
    &AuraEffect::HandleAuraModDmgImmunity,                        // 40 SPELL_AURA_DAMAGE_IMMUNITY
    &AuraEffect::HandleAuraModDispelImmunity,                     // 41 SPELL_AURA_DISPEL_IMMUNITY
    &AuraEffect::HandleNoImmediateEffect,                         // 42 SPELL_AURA_PROC_TRIGGER_SPELL  implemented in Unit::ProcDamageAndSpellFor and Unit::HandleProcTriggerSpell
    &AuraEffect::HandleNoImmediateEffect,                         // 43 SPELL_AURA_PROC_TRIGGER_DAMAGE implemented in Unit::ProcDamageAndSpellFor
    &AuraEffect::HandleAuraTrackCreatures,                        // 44 SPELL_AURA_TRACK_CREATURES
    &AuraEffect::HandleAuraTrackResources,                        // 45 SPELL_AURA_TRACK_RESOURCES
    &AuraEffect::HandleNULL,                                      // 46 SPELL_AURA_46 (used in test spells 54054 and 54058, and spell 48050) (3.0.8a)
    &AuraEffect::HandleAuraModParryPercent,                       // 47 SPELL_AURA_MOD_PARRY_PERCENT
    &AuraEffect::HandleNoImmediateEffect,                         // 48 SPELL_AURA_PERIODIC_TRIGGER_SPELL_BY_CLIENT
    &AuraEffect::HandleAuraModDodgePercent,                       // 49 SPELL_AURA_MOD_DODGE_PERCENT
    &AuraEffect::HandleNoImmediateEffect,                         // 50 SPELL_AURA_MOD_CRITICAL_HEALING_AMOUNT implemented in Unit::SpellCriticalHealingBonus
    &AuraEffect::HandleAuraModBlockPercent,                       // 51 SPELL_AURA_MOD_BLOCK_PERCENT
    &AuraEffect::HandleAuraModWeaponCritPercent,                  // 52 SPELL_AURA_MOD_WEAPON_CRIT_PERCENT
    &AuraEffect::HandleNoImmediateEffect,                         // 53 SPELL_AURA_PERIODIC_LEECH implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleModHitChance,                              // 54 SPELL_AURA_MOD_HIT_CHANCE
    &AuraEffect::HandleModSpellHitChance,                         // 55 SPELL_AURA_MOD_SPELL_HIT_CHANCE
    &AuraEffect::HandleAuraTransform,                             // 56 SPELL_AURA_TRANSFORM
    &AuraEffect::HandleModSpellCritChance,                        // 57 SPELL_AURA_MOD_SPELL_CRIT_CHANCE
    &AuraEffect::HandleAuraModIncreaseSwimSpeed,                  // 58 SPELL_AURA_MOD_INCREASE_SWIM_SPEED
    &AuraEffect::HandleNoImmediateEffect,                         // 59 SPELL_AURA_MOD_DAMAGE_DONE_CREATURE implemented in Unit::MeleeDamageBonus and Unit::SpellDamageBonus
    &AuraEffect::HandleAuraModPacifyAndSilence,                   // 60 SPELL_AURA_MOD_PACIFY_SILENCE
    &AuraEffect::HandleAuraModScale,                              // 61 SPELL_AURA_MOD_SCALE
    &AuraEffect::HandleNoImmediateEffect,                         // 62 SPELL_AURA_PERIODIC_HEALTH_FUNNEL implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleUnused,                                    // 63 unused (4.3.4) old SPELL_AURA_PERIODIC_MANA_FUNNEL
    &AuraEffect::HandleNoImmediateEffect,                         // 64 SPELL_AURA_PERIODIC_MANA_LEECH implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleModCastingSpeed,                           // 65 SPELL_AURA_MOD_CASTING_SPEED_NOT_STACK
    &AuraEffect::HandleFeignDeath,                                // 66 SPELL_AURA_FEIGN_DEATH
    &AuraEffect::HandleAuraModDisarm,                             // 67 SPELL_AURA_MOD_DISARM
    &AuraEffect::HandleAuraModStalked,                            // 68 SPELL_AURA_MOD_STALKED
    &AuraEffect::HandleNoImmediateEffect,                         // 69 SPELL_AURA_SCHOOL_ABSORB implemented in Unit::CalcAbsorbResist
    &AuraEffect::HandleUnused,                                    // 70 SPELL_AURA_EXTRA_ATTACKS clientside
    &AuraEffect::HandleModSpellCritChanceShool,                   // 71 SPELL_AURA_MOD_SPELL_CRIT_CHANCE_SCHOOL
    &AuraEffect::HandleModPowerCostPCT,                           // 72 SPELL_AURA_MOD_POWER_COST_SCHOOL_PCT
    &AuraEffect::HandleModPowerCost,                              // 73 SPELL_AURA_MOD_POWER_COST_SCHOOL
    &AuraEffect::HandleNoImmediateEffect,                         // 74 SPELL_AURA_REFLECT_SPELLS_SCHOOL  implemented in Unit::SpellHitResult
    &AuraEffect::HandleNoImmediateEffect,                         // 75 SPELL_AURA_MOD_LANGUAGE
    &AuraEffect::HandleNoImmediateEffect,                         // 76 SPELL_AURA_FAR_SIGHT
    &AuraEffect::HandleModMechanicImmunity,                       // 77 SPELL_AURA_MECHANIC_IMMUNITY
    &AuraEffect::HandleAuraMounted,                               // 78 SPELL_AURA_MOUNTED
    &AuraEffect::HandleModDamagePercentDone,                      // 79 SPELL_AURA_MOD_DAMAGE_PERCENT_DONE
    &AuraEffect::HandleModPercentStat,                            // 80 SPELL_AURA_MOD_PERCENT_STAT
    &AuraEffect::HandleNoImmediateEffect,                         // 81 SPELL_AURA_SPLIT_DAMAGE_PCT implemented in Unit::CalcAbsorbResist
    &AuraEffect::HandleWaterBreathing,                            // 82 SPELL_AURA_WATER_BREATHING
    &AuraEffect::HandleModBaseResistance,                         // 83 SPELL_AURA_MOD_BASE_RESISTANCE
    &AuraEffect::HandleNoImmediateEffect,                         // 84 SPELL_AURA_MOD_REGEN implemented in Player::RegenerateHealth
    &AuraEffect::HandleModPowerRegen,                             // 85 SPELL_AURA_MOD_POWER_REGEN implemented in Player::Regenerate
    &AuraEffect::HandleChannelDeathItem,                          // 86 SPELL_AURA_CHANNEL_DEATH_ITEM
    &AuraEffect::HandleNoImmediateEffect,                         // 87 SPELL_AURA_MOD_DAMAGE_PERCENT_TAKEN implemented in Unit::MeleeDamageBonus and Unit::SpellDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         // 88 SPELL_AURA_MOD_HEALTH_REGEN_PERCENT implemented in Player::RegenerateHealth
    &AuraEffect::HandleNoImmediateEffect,                         // 89 SPELL_AURA_PERIODIC_DAMAGE_PERCENT
    &AuraEffect::HandleUnused,                                    // 90 unused (4.3.4) old SPELL_AURA_MOD_RESIST_CHANCE
    &AuraEffect::HandleNoImmediateEffect,                         // 91 SPELL_AURA_MOD_DETECT_RANGE implemented in Creature::GetAttackDistance
    &AuraEffect::HandlePreventFleeing,                            // 92 SPELL_AURA_PREVENTS_FLEEING
    &AuraEffect::HandleModUnattackable,                           // 93 SPELL_AURA_MOD_UNATTACKABLE
    &AuraEffect::HandleNoImmediateEffect,                         // 94 SPELL_AURA_INTERRUPT_REGEN implemented in Player::Regenerate
    &AuraEffect::HandleAuraGhost,                                 // 95 SPELL_AURA_GHOST
    &AuraEffect::HandleNoImmediateEffect,                         // 96 SPELL_AURA_SPELL_MAGNET implemented in Unit::SelectMagnetTarget
    &AuraEffect::HandleNoImmediateEffect,                         // 97 SPELL_AURA_MANA_SHIELD implemented in Unit::CalcAbsorbResist
    &AuraEffect::HandleAuraModSkill,                              // 98 SPELL_AURA_MOD_SKILL_TALENT
    &AuraEffect::HandleAuraModAttackPower,                        // 99 SPELL_AURA_MOD_ATTACK_POWER
    &AuraEffect::HandleUnused,                                    //100 SPELL_AURA_AURAS_VISIBLE obsolete? all player can see all auras now, but still have spells including GM-spell
    &AuraEffect::HandleModResistancePercent,                      //101 SPELL_AURA_MOD_RESISTANCE_PCT
    &AuraEffect::HandleNoImmediateEffect,                         //102 SPELL_AURA_MOD_MELEE_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleAuraModTotalThreat,                        //103 SPELL_AURA_MOD_TOTAL_THREAT
    &AuraEffect::HandleAuraWaterWalk,                             //104 SPELL_AURA_WATER_WALK
    &AuraEffect::HandleAuraFeatherFall,                           //105 SPELL_AURA_FEATHER_FALL
    &AuraEffect::HandleAuraHover,                                 //106 SPELL_AURA_HOVER
    &AuraEffect::HandleNoImmediateEffect,                         //107 SPELL_AURA_ADD_FLAT_MODIFIER implemented in AuraEffect::CalculateSpellMod()
    &AuraEffect::HandleNoImmediateEffect,                         //108 SPELL_AURA_ADD_PCT_MODIFIER implemented in AuraEffect::CalculateSpellMod()
    &AuraEffect::HandleNoImmediateEffect,                         //109 SPELL_AURA_ADD_TARGET_TRIGGER
    &AuraEffect::HandleModPowerRegenPCT,                          //110 SPELL_AURA_MOD_POWER_REGEN_PERCENT implemented in Player::Regenerate, Creature::Regenerate
    &AuraEffect::HandleNoImmediateEffect,                         //111 SPELL_AURA_ADD_CASTER_HIT_TRIGGER implemented in Unit::SelectMagnetTarget
    &AuraEffect::HandleNoImmediateEffect,                         //112 SPELL_AURA_OVERRIDE_CLASS_SCRIPTS
    &AuraEffect::HandleNoImmediateEffect,                         //113 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         //114 SPELL_AURA_MOD_RANGED_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         //115 SPELL_AURA_MOD_HEALING                 implemented in Unit::SpellBaseHealingBonusTaken
    &AuraEffect::HandleNoImmediateEffect,                         //116 SPELL_AURA_MOD_REGEN_DURING_COMBAT
    &AuraEffect::HandleNoImmediateEffect,                         //117 SPELL_AURA_MOD_MECHANIC_RESISTANCE     implemented in Unit::MagicSpellHitResult
    &AuraEffect::HandleNoImmediateEffect,                         //118 SPELL_AURA_MOD_HEALING_PCT             implemented in Unit::SpellHealingBonus
    &AuraEffect::HandleUnused,                                    //119 unused (4.3.4) old SPELL_AURA_SHARE_PET_TRACKING
    &AuraEffect::HandleAuraUntrackable,                           //120 SPELL_AURA_UNTRACKABLE
    &AuraEffect::HandleAuraEmpathy,                               //121 SPELL_AURA_EMPATHY
    &AuraEffect::HandleModOffhandDamagePercent,                   //122 SPELL_AURA_MOD_OFFHAND_DAMAGE_PCT
    &AuraEffect::HandleModTargetResistance,                       //123 SPELL_AURA_MOD_TARGET_RESISTANCE
    &AuraEffect::HandleAuraModRangedAttackPower,                  //124 SPELL_AURA_MOD_RANGED_ATTACK_POWER
    &AuraEffect::HandleNoImmediateEffect,                         //125 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         //126 SPELL_AURA_MOD_MELEE_DAMAGE_TAKEN_PCT implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         //127 SPELL_AURA_RANGED_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleModPossessPet,                             //128 SPELL_AURA_MOD_POSSESS_PET
    &AuraEffect::HandleAuraModIncreaseSpeed,                      //129 SPELL_AURA_MOD_SPEED_ALWAYS
    &AuraEffect::HandleAuraModIncreaseMountedSpeed,               //130 SPELL_AURA_MOD_MOUNTED_SPEED_ALWAYS
    &AuraEffect::HandleNoImmediateEffect,                         //131 SPELL_AURA_MOD_RANGED_ATTACK_POWER_VERSUS implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleAuraModIncreaseEnergyPercent,              //132 SPELL_AURA_MOD_INCREASE_ENERGY_PERCENT
    &AuraEffect::HandleAuraModIncreaseHealthPercent,              //133 SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT
    &AuraEffect::HandleAuraModRegenInterrupt,                     //134 SPELL_AURA_MOD_MANA_REGEN_INTERRUPT
    &AuraEffect::HandleModHealingDone,                            //135 SPELL_AURA_MOD_HEALING_DONE
    &AuraEffect::HandleNoImmediateEffect,                         //136 SPELL_AURA_MOD_HEALING_DONE_PERCENT   implemented in Unit::SpellHealingBonus
    &AuraEffect::HandleModTotalPercentStat,                       //137 SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE
    &AuraEffect::HandleModMeleeSpeedPct,                          //138 SPELL_AURA_MOD_MELEE_HASTE
    &AuraEffect::HandleForceReaction,                             //139 SPELL_AURA_FORCE_REACTION
    &AuraEffect::HandleAuraModRangedHaste,                        //140 SPELL_AURA_MOD_RANGED_HASTE
    &AuraEffect::HandleUnused,                                    //141 SPELL_AURA_141
    &AuraEffect::HandleAuraModBaseResistancePCT,                  //142 SPELL_AURA_MOD_BASE_RESISTANCE_PCT
    &AuraEffect::HandleAuraModResistanceExclusive,                //143 SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE
    &AuraEffect::HandleNoImmediateEffect,                         //144 SPELL_AURA_SAFE_FALL                         implemented in WorldSession::HandleMovementOpcodes
    &AuraEffect::HandleAuraModPetTalentsPoints,                   //145 SPELL_AURA_MOD_PET_TALENT_POINTS
    &AuraEffect::HandleNoImmediateEffect,                         //146 SPELL_AURA_ALLOW_TAME_PET_TYPE
    &AuraEffect::HandleModStateImmunityMask,                      //147 SPELL_AURA_MECHANIC_IMMUNITY_MASK
    &AuraEffect::HandleAuraRetainComboPoints,                     //148 SPELL_AURA_RETAIN_COMBO_POINTS
    &AuraEffect::HandleNoImmediateEffect,                         //149 SPELL_AURA_REDUCE_PUSHBACK
    &AuraEffect::HandleShieldBlockValue,                          //150 SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT
    &AuraEffect::HandleAuraTrackStealthed,                        //151 SPELL_AURA_TRACK_STEALTHED
    &AuraEffect::HandleNoImmediateEffect,                         //152 SPELL_AURA_MOD_DETECTED_RANGE implemented in Creature::GetAttackDistance
    &AuraEffect::HandleUnused,                                    //153 Unused (4.3.4) old SPELL_AURA_SPLIT_DAMAGE_FLAT
    &AuraEffect::HandleModStealthLevel,                           //154 SPELL_AURA_MOD_STEALTH_LEVEL
    &AuraEffect::HandleNoImmediateEffect,                         //155 SPELL_AURA_MOD_WATER_BREATHING
    &AuraEffect::HandleNoImmediateEffect,                         //156 SPELL_AURA_MOD_REPUTATION_GAIN
    &AuraEffect::HandleNULL,                                      //157 SPELL_AURA_PET_DAMAGE_MULTI
    &AuraEffect::HandleShieldBlockValue,                          //158 SPELL_AURA_MOD_SHIELD_BLOCKVALUE
    &AuraEffect::HandleNoImmediateEffect,                         //159 SPELL_AURA_NO_PVP_CREDIT      only for Honorless Target spell
    &AuraEffect::HandleUnused,                                    //160 Unused (4.3.4) old SPELL_AURA_MOD_AOE_AVOIDANCE
    &AuraEffect::HandleNoImmediateEffect,                         //161 SPELL_AURA_MOD_HEALTH_REGEN_IN_COMBAT
    &AuraEffect::HandleNoImmediateEffect,                         //162 SPELL_AURA_POWER_BURN implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNoImmediateEffect,                         //163 SPELL_AURA_MOD_CRIT_DAMAGE_BONUS
    &AuraEffect::HandleUnused,                                    //164 unused (3.2.0), only one test spell
    &AuraEffect::HandleNoImmediateEffect,                         //165 SPELL_AURA_MELEE_ATTACK_POWER_ATTACKER_BONUS implemented in Unit::MeleeDamageBonus
    &AuraEffect::HandleAuraModAttackPowerPercent,                 //166 SPELL_AURA_MOD_ATTACK_POWER_PCT
    &AuraEffect::HandleAuraModRangedAttackPowerPercent,           //167 SPELL_AURA_MOD_RANGED_ATTACK_POWER_PCT
    &AuraEffect::HandleNoImmediateEffect,                         //168 SPELL_AURA_MOD_DAMAGE_DONE_VERSUS            implemented in Unit::SpellDamageBonus, Unit::MeleeDamageBonus
    &AuraEffect::HandleUnused,                                    //169 Unused (4.3.4) old SPELL_AURA_MOD_CRIT_PERCENT_VERSUS
    &AuraEffect::HandleNULL,                                      //170 SPELL_AURA_DETECT_AMORE       various spells that change visual of units for aura target (clientside?)
    &AuraEffect::HandleAuraModIncreaseSpeed,                      //171 SPELL_AURA_MOD_SPEED_NOT_STACK
    &AuraEffect::HandleAuraModIncreaseMountedSpeed,               //172 SPELL_AURA_MOD_MOUNTED_SPEED_NOT_STACK
    &AuraEffect::HandleUnused,                                    //173 unused (4.3.4) no spells, old SPELL_AURA_ALLOW_CHAMPION_SPELLS  only for Proclaim Champion spell
    &AuraEffect::HandleModSpellDamagePercentFromStat,             //174 SPELL_AURA_MOD_SPELL_DAMAGE_OF_STAT_PERCENT  implemented in Unit::SpellBaseDamageBonus
    &AuraEffect::HandleModSpellHealingPercentFromStat,            //175 SPELL_AURA_MOD_SPELL_HEALING_OF_STAT_PERCENT implemented in Unit::SpellBaseHealingBonus
    &AuraEffect::HandleSpiritOfRedemption,                        //176 SPELL_AURA_SPIRIT_OF_REDEMPTION   only for Spirit of Redemption spell, die at aura end
    &AuraEffect::HandleCharmConvert,                              //177 SPELL_AURA_AOE_CHARM
    &AuraEffect::HandleUnused,                                    //178 old SPELL_AURA_MOD_DEBUFF_RESISTANCE unused 4.3.4
    &AuraEffect::HandleNoImmediateEffect,                         //179 SPELL_AURA_MOD_ATTACKER_SPELL_CRIT_CHANCE implemented in Unit::SpellCriticalBonus
    &AuraEffect::HandleNoImmediateEffect,                         //180 SPELL_AURA_MOD_FLAT_SPELL_DAMAGE_VERSUS   implemented in Unit::SpellDamageBonus
    &AuraEffect::HandleUnused,                                    //181 unused (4.3.4) old SPELL_AURA_MOD_FLAT_SPELL_CRIT_DAMAGE_VERSUS
    &AuraEffect::HandleAuraModResistenceOfStatPercent,            //182 SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT
    &AuraEffect::HandleNULL,                                      //183 SPELL_AURA_MOD_CRITICAL_THREAT only used in 28746 - miscvalue - spell school
    &AuraEffect::HandleNoImmediateEffect,                         //184 SPELL_AURA_MOD_ATTACKER_MELEE_HIT_CHANCE  implemented in Unit::RollMeleeOutcomeAgainst
    &AuraEffect::HandleNoImmediateEffect,                         //185 SPELL_AURA_MOD_ATTACKER_RANGED_HIT_CHANCE implemented in Unit::RollMeleeOutcomeAgainst
    &AuraEffect::HandleNoImmediateEffect,                         //186 SPELL_AURA_MOD_ATTACKER_SPELL_HIT_CHANCE  implemented in Unit::MagicSpellHitResult
    &AuraEffect::HandleNoImmediateEffect,                         //187 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_CHANCE  implemented in Unit::GetUnitCriticalChance
    &AuraEffect::HandleNoImmediateEffect,                         //188 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_CHANCE implemented in Unit::GetUnitCriticalChance
    &AuraEffect::HandleModRating,                                 //189 SPELL_AURA_MOD_RATING
    &AuraEffect::HandleNoImmediateEffect,                         //190 SPELL_AURA_MOD_FACTION_REPUTATION_GAIN     implemented in Player::CalculateReputationGain
    &AuraEffect::HandleAuraModUseNormalSpeed,                     //191 SPELL_AURA_USE_NORMAL_MOVEMENT_SPEED
    &AuraEffect::HandleModMeleeRangedSpeedPct,                    //192 SPELL_AURA_MOD_MELEE_RANGED_HASTE
    &AuraEffect::HandleModCombatSpeedPct,                         //193 SPELL_AURA_MELEE_SLOW (in fact combat (any type attack) speed pct)
    &AuraEffect::HandleNoImmediateEffect,                         //194 SPELL_AURA_MOD_TARGET_ABSORB_SCHOOL implemented in Unit::CalcAbsorbResist
    &AuraEffect::HandleNoImmediateEffect,                         //195 SPELL_AURA_MOD_TARGET_ABILITY_ABSORB_SCHOOL implemented in Unit::CalcAbsorbResist
    &AuraEffect::HandleNULL,                                      //196 SPELL_AURA_MOD_COOLDOWN - flat mod of spell cooldowns
    &AuraEffect::HandleNoImmediateEffect,                         //197 SPELL_AURA_MOD_ATTACKER_SPELL_AND_WEAPON_CRIT_CHANCE implemented in Unit::SpellCriticalBonus Unit::GetUnitCriticalChance
    &AuraEffect::HandleUnused,                                    //198 unused (4.3.4) old SPELL_AURA_MOD_ALL_WEAPON_SKILLS
    &AuraEffect::HandleUnused,                                    //199 unused (4.3.4) old SPELL_AURA_MOD_INCREASES_SPELL_PCT_TO_HIT
    &AuraEffect::HandleNoImmediateEffect,                         //200 SPELL_AURA_MOD_XP_PCT implemented in Player::RewardPlayerAndGroupAtKill
    &AuraEffect::HandleAuraAllowFlight,                           //201 SPELL_AURA_FLY                             this aura enable flight mode...
    &AuraEffect::HandleNoImmediateEffect,                         //202 SPELL_AURA_CANNOT_BE_DODGED                implemented in Unit::RollPhysicalOutcomeAgainst
    &AuraEffect::HandleNoImmediateEffect,                         //203 SPELL_AURA_MOD_ATTACKER_MELEE_CRIT_DAMAGE  implemented in Unit::CalculateMeleeDamage and Unit::CalculateSpellDamage
    &AuraEffect::HandleNoImmediateEffect,                         //204 SPELL_AURA_MOD_ATTACKER_RANGED_CRIT_DAMAGE implemented in Unit::CalculateMeleeDamage and Unit::CalculateSpellDamage
    &AuraEffect::HandleNULL,                                      //205 SPELL_AURA_MOD_SCHOOL_CRIT_DMG_TAKEN
    &AuraEffect::HandleAuraModIncreaseFlightSpeed,                //206 SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED
    &AuraEffect::HandleAuraModIncreaseFlightSpeed,                //207 SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED
    &AuraEffect::HandleAuraModIncreaseFlightSpeed,                //208 SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED
    &AuraEffect::HandleAuraModIncreaseFlightSpeed,                //209 SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS
    &AuraEffect::HandleAuraModIncreaseFlightSpeed,                //210 SPELL_AURA_MOD_VEHICLE_SPEED_ALWAYS
    &AuraEffect::HandleAuraModIncreaseFlightSpeed,                //211 SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK
    &AuraEffect::HandleUnused,                                    //212 Unused (4.3.4) old SPELL_AURA_MOD_RANGED_ATTACK_POWER_OF_STAT_PERCENT
    &AuraEffect::HandleNoImmediateEffect,                         //213 SPELL_AURA_MOD_RAGE_FROM_DAMAGE_DEALT implemented in Player::RewardRage
    &AuraEffect::HandleNULL,                                      //214 Tamed Pet Passive
    &AuraEffect::HandleArenaPreparation,                          //215 SPELL_AURA_ARENA_PREPARATION
    &AuraEffect::HandleModCastingSpeed,                           //216 SPELL_AURA_HASTE_SPELLS
    &AuraEffect::HandleModMeleeSpeedPct,                          //217 SPELL_AURA_MOD_MELEE_HASTE_2
    &AuraEffect::HandleAuraModRangedHaste,                        //218 SPELL_AURA_HASTE_RANGED
    &AuraEffect::HandleModManaRegen,                              //219 SPELL_AURA_MOD_MANA_REGEN_FROM_STAT
    &AuraEffect::HandleModRatingFromStat,                         //220 SPELL_AURA_MOD_RATING_FROM_STAT
    &AuraEffect::HandleNULL,                                      //221 SPELL_AURA_MOD_DETAUNT
    &AuraEffect::HandleNoImmediateEffect,                         //222 SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE2
    &AuraEffect::HandleNoImmediateEffect,                         //223 SPELL_AURA_RAID_PROC_FROM_CHARGE
    &AuraEffect::HandleUnused,                                    //224 unused (4.3.4)
    &AuraEffect::HandleNoImmediateEffect,                         //225 SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE
    &AuraEffect::HandleNoImmediateEffect,                         //226 SPELL_AURA_PERIODIC_DUMMY implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNoImmediateEffect,                         //227 SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNoImmediateEffect,                         //228 SPELL_AURA_DETECT_STEALTH stealth detection
    &AuraEffect::HandleNoImmediateEffect,                         //229 SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE
    &AuraEffect::HandleAuraModIncreaseHealth,                     //230 SPELL_AURA_MOD_INCREASE_HEALTH_2
    &AuraEffect::HandleNoImmediateEffect,                         //231 SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE
    &AuraEffect::HandleNoImmediateEffect,                         //232 SPELL_AURA_MECHANIC_DURATION_MOD           implement in Unit::CalculateSpellDuration
    &AuraEffect::HandleUnused,                                    //233 set model id to the one of the creature with id GetMiscValue() - clientside
    &AuraEffect::HandleNoImmediateEffect,                         //234 SPELL_AURA_MECHANIC_DURATION_MOD_NOT_STACK implement in Unit::CalculateSpellDuration
    &AuraEffect::HandleNoImmediateEffect,                         //235 SPELL_AURA_MOD_DISPEL_RESIST               implement in Unit::MagicSpellHitResult
    &AuraEffect::HandleAuraControlVehicle,                        //236 SPELL_AURA_CONTROL_VEHICLE
    &AuraEffect::HandleModSpellDamagePercentFromAttackPower,      //237 SPELL_AURA_MOD_SPELL_DAMAGE_OF_ATTACK_POWER  implemented in Unit::SpellBaseDamageBonus
    &AuraEffect::HandleModSpellHealingPercentFromAttackPower,     //238 SPELL_AURA_MOD_SPELL_HEALING_OF_ATTACK_POWER implemented in Unit::SpellBaseHealingBonus
    &AuraEffect::HandleAuraModScale,                              //239 SPELL_AURA_MOD_SCALE_2 only in Noggenfogger Elixir (16595) before 2.3.0 aura 61
    &AuraEffect::HandleAuraModExpertise,                          //240 SPELL_AURA_MOD_EXPERTISE
    &AuraEffect::HandleForceMoveForward,                          //241 SPELL_AURA_FORCE_MOVE_FORWARD Forces the caster to move forward
    &AuraEffect::HandleNULL,                                      //242 SPELL_AURA_MOD_SPELL_DAMAGE_FROM_HEALING - 2 test spells: 44183 and 44182
    &AuraEffect::HandleAuraModFaction,                            //243 SPELL_AURA_MOD_FACTION
    &AuraEffect::HandleComprehendLanguage,                        //244 SPELL_AURA_COMPREHEND_LANGUAGE
    &AuraEffect::HandleNoImmediateEffect,                         //245 SPELL_AURA_MOD_AURA_DURATION_BY_DISPEL
    &AuraEffect::HandleNoImmediateEffect,                         //246 SPELL_AURA_MOD_AURA_DURATION_BY_DISPEL_NOT_STACK implemented in Spell::EffectApplyAura
    &AuraEffect::HandleAuraCloneCaster,                           //247 SPELL_AURA_CLONE_CASTER
    &AuraEffect::HandleNoImmediateEffect,                         //248 SPELL_AURA_MOD_COMBAT_RESULT_CHANCE         implemented in Unit::RollMeleeOutcomeAgainst
    &AuraEffect::HandleAuraConvertRune,                           //249 SPELL_AURA_CONVERT_RUNE
    &AuraEffect::HandleAuraModIncreaseHealth,                     //250 SPELL_AURA_MOD_INCREASE_HEALTH_3
    &AuraEffect::HandleNoImmediateEffect,                         //251 SPELL_AURA_MOD_ENEMY_DODGE
    &AuraEffect::HandleModCombatSpeedPct,                         //252 SPELL_AURA_252 Is there any difference between this and SPELL_AURA_MELEE_SLOW ? maybe not stacking mod?
    &AuraEffect::HandleNoImmediateEffect,                         //253 SPELL_AURA_MOD_BLOCK_CRIT_CHANCE  implemented in Unit::isBlockCritical
    &AuraEffect::HandleAuraModDisarm,                             //254 SPELL_AURA_MOD_DISARM_OFFHAND
    &AuraEffect::HandleNoImmediateEffect,                         //255 SPELL_AURA_MOD_MECHANIC_DAMAGE_TAKEN_PERCENT    implemented in Unit::SpellDamageBonus
    &AuraEffect::HandleNoReagentUseAura,                          //256 SPELL_AURA_NO_REAGENT_USE Use SpellClassMask for spell select
    &AuraEffect::HandleNULL,                                      //257 SPELL_AURA_MOD_TARGET_RESIST_BY_SPELL_CLASS Use SpellClassMask for spell select
    &AuraEffect::HandleNULL,                                      //258 SPELL_AURA_MOD_SPELL_VISUAL
    &AuraEffect::HandleNoImmediateEffect,                         //259 SPELL_AURA_MOD_HOT_PCT implemented in Unit::SpellHealingBonusTaken
    &AuraEffect::HandleNoImmediateEffect,                         //260 SPELL_AURA_SCREEN_EFFECT (miscvalue = id in ScreenEffect.dbc) not required any code
    &AuraEffect::HandlePhase,                                     //261 SPELL_AURA_PHASE
    &AuraEffect::HandleNoImmediateEffect,                         //262 SPELL_AURA_ABILITY_IGNORE_AURASTATE implemented in spell::cancast
    &AuraEffect::HandleAuraAllowOnlyAbility,                      //263 SPELL_AURA_ALLOW_ONLY_ABILITY player can use only abilities set in SpellClassMask
    &AuraEffect::HandleUnused,                                    //264 unused (3.2.0)
    &AuraEffect::HandleUnused,                                    //265 unused (4.3.4)
    &AuraEffect::HandleUnused,                                    //266 unused (4.3.4)
    &AuraEffect::HandleNoImmediateEffect,                         //267 SPELL_AURA_MOD_IMMUNE_AURA_APPLY_SCHOOL         implemented in Unit::IsImmunedToSpellEffect
    &AuraEffect::HandleUnused,                                    //268 unused (4.3.4) old SPELL_AURA_MOD_ATTACK_POWER_OF_STAT_PERCENT.
    &AuraEffect::HandleNoImmediateEffect,                         //269 SPELL_AURA_MOD_IGNORE_TARGET_RESIST implemented in Unit::CalcAbsorbResist and CalcArmorReducedDamage
    &AuraEffect::HandleUnused,                                    //270 unused (4.3.4) old SPELL_AURA_MOD_ABILITY_IGNORE_TARGET_RESIST
    &AuraEffect::HandleNoImmediateEffect,                         //271 SPELL_AURA_MOD_DAMAGE_FROM_CASTER    implemented in Unit::SpellDamageBonus
    &AuraEffect::HandleNoImmediateEffect,                         //272 SPELL_AURA_IGNORE_MELEE_RESET
    &AuraEffect::HandleUnused,                                    //273 clientside
    &AuraEffect::HandleUnused,                                    //274 unused (4.3.4)
    &AuraEffect::HandleNoImmediateEffect,                         //275 SPELL_AURA_MOD_IGNORE_SHAPESHIFT Use SpellClassMask for spell select
    &AuraEffect::HandleNULL,                                      //276 mod damage % mechanic?
    &AuraEffect::HandleUnused,                                    //277 unused (4.3.4) old SPELL_AURA_MOD_MAX_AFFECTED_TARGETS
    &AuraEffect::HandleAuraModDisarm,                             //278 SPELL_AURA_MOD_DISARM_RANGED disarm ranged weapon
    &AuraEffect::HandleAuraInitializeImages,                      //279 SPELL_AURA_INITIALIZE_IMAGES
    &AuraEffect::HandleNoImmediateEffect,                         //280 SPELL_AURA_MOD_ARMOR_PENETRATION_PCT implemented in Unit::CalcArmorReducedDamage
    &AuraEffect::HandleNoImmediateEffect,                         //281 SPELL_AURA_MOD_REPUTATION_GAIN_PCT implemented in Player::RewardHonor
    &AuraEffect::HandleAuraIncreaseBaseHealthPercent,             //282 SPELL_AURA_INCREASE_BASE_HEALTH_PERCENT
    &AuraEffect::HandleNoImmediateEffect,                         //283 SPELL_AURA_MOD_HEALING_RECEIVED       implemented in Unit::SpellHealingBonus
    &AuraEffect::HandleAuraLinked,                                //284 SPELL_AURA_LINKED
    &AuraEffect::HandleAuraModAttackPowerOfArmor,                 //285 SPELL_AURA_MOD_ATTACK_POWER_OF_ARMOR  implemented in Player::UpdateAttackPowerAndDamage
    &AuraEffect::HandleNoImmediateEffect,                         //286 SPELL_AURA_ABILITY_PERIODIC_CRIT implemented in AuraEffect::PeriodicTick
    &AuraEffect::HandleNoImmediateEffect,                         //287 SPELL_AURA_DEFLECT_SPELLS             implemented in Unit::MagicSpellHitResult and Unit::MeleeSpellHitResult
    &AuraEffect::HandleNoImmediateEffect,                         //288 SPELL_AURA_IGNORE_HIT_DIRECTION  implemented in Unit::MagicSpellHitResult and Unit::MeleeSpellHitResult Unit::RollMeleeOutcomeAgainst
    &AuraEffect::HandleNULL,                                      //289 unused (3.2.0)
    &AuraEffect::HandleAuraModCritPct,                            //290 SPELL_AURA_MOD_CRIT_PCT
    &AuraEffect::HandleNoImmediateEffect,                         //291 SPELL_AURA_MOD_XP_QUEST_PCT  implemented in Player::RewardQuest
    &AuraEffect::HandleAuraOpenStable,                            //292 SPELL_AURA_OPEN_STABLE
    &AuraEffect::HandleAuraOverrideSpells,                        //293 auras which probably add set of abilities to their target based on it's miscvalue
    &AuraEffect::HandleNoImmediateEffect,                         //294 SPELL_AURA_PREVENT_REGENERATE_POWER implemented in Player::Regenerate(Powers power)
    &AuraEffect::HandleUnused,                                    //295 unused (4.3.4)
    &AuraEffect::HandleAuraSetVehicle,                            //296 SPELL_AURA_SET_VEHICLE_ID sets vehicle on target
    &AuraEffect::HandleNULL,                                      //297 Spirit Burst spells
    &AuraEffect::HandleAuraStrangulate,                           //298 SPELL_AURA_STRANGULATE 70569 - Strangulating, maybe prevents talk or cast
    &AuraEffect::HandleUnused,                                    //299 unused (4.3.4)
    &AuraEffect::HandleNoImmediateEffect,                         //300 SPELL_AURA_SHARE_DAMAGE_PCT implemented in Unit::DealDamage
    &AuraEffect::HandleNoImmediateEffect,                         //301 SPELL_AURA_SCHOOL_HEAL_ABSORB implemented in Unit::CalcHealAbsorb
    &AuraEffect::HandleUnused,                                    //302 unused (4.3.4)
    &AuraEffect::HandleNoImmediateEffect,                         //303 SPELL_AURA_MOD_DAMAGE_DONE_VERSUS_AURASTATE implemented in Unit::SpellDamageBonus, Unit::MeleeDamageBonus
    &AuraEffect::HandleAuraModFakeInebriation,                    //304 SPELL_AURA_MOD_DRUNK
    &AuraEffect::HandleAuraModIncreaseSpeed,                      //305 SPELL_AURA_MOD_MINIMUM_SPEED
    &AuraEffect::HandleUnused,                                    //306 unused (4.3.4)
    &AuraEffect::HandleUnused,                                    //307 unused (4.3.4)
    &AuraEffect::HandleNULL,                                      //308 new aura for hunter traps
    &AuraEffect::HandleAuraModResiliencePct,                      //309 SPELL_AURA_MOD_RESILIENCE_PCT
    &AuraEffect::HandleNoImmediateEffect,                         //310 SPELL_AURA_MOD_CREATURE_AOE_DAMAGE_AVOIDANCE implemented in Spell::CalculateDamageDone
    &AuraEffect::HandleNULL,                                      //311 0 spells in 3.3.5
    &AuraEffect::HandleNULL,                                      //312 0 spells in 3.3.5
    &AuraEffect::HandleUnused,                                    //313 unused (4.3.4)
    &AuraEffect::HandlePreventResurrection,                       //314 SPELL_AURA_PREVENT_RESURRECTION todo
    &AuraEffect::HandleNoImmediateEffect,                         //315 SPELL_AURA_UNDERWATER_WALKING todo
    &AuraEffect::HandleNoImmediateEffect,                         //316 unused (4.3.4) old SPELL_AURA_PERIODIC_HASTE
    &AuraEffect::HandleAuraModSpellPowerPercent,                  //317 SPELL_AURA_MOD_SPELL_POWER_PCT
    &AuraEffect::HandleAuraMastery,                               //318 SPELL_AURA_MASTERY
    &AuraEffect::HandleModMeleeSpeedPct,                          //319 SPELL_AURA_MOD_MELEE_HASTE_3
    &AuraEffect::HandleAuraModRangedHaste,                        //320 SPELL_AURA_MOD_RANGED_HASTE_3
    &AuraEffect::HandleNULL,                                      //321 SPELL_AURA_321
    &AuraEffect::HandleNULL,                                      //322 SPELL_AURA_INTERFERE_TARGETTING
    &AuraEffect::HandleUnused,                                    //323 unused (4.3.4)
    &AuraEffect::HandleNULL,                                      //324 SPELL_AURA_324
    &AuraEffect::HandleUnused,                                    //325 unused (4.3.4)
    &AuraEffect::HandlePhase,                                     //326 SPELL_AURA_PHASE_2
    &AuraEffect::HandleUnused,                                    //327 unused (4.3.4)
    &AuraEffect::HandleNoImmediateEffect,                         //328 SPELL_AURA_PROC_ON_POWER_AMOUNT implemented in Unit::HandleAuraProcOnPowerAmount
    &AuraEffect::HandleNULL,                                      //329 SPELL_AURA_MOD_RUNE_REGEN_SPEED
    &AuraEffect::HandleNoImmediateEffect,                         //330 SPELL_AURA_CAST_WHILE_WALKING
    &AuraEffect::HandleAuraForceWeather,                          //331 SPELL_AURA_FORCE_WEATHER
    &AuraEffect::HandleOverrideActionbarSpells,                   //332 SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS implemented in WorldSession::HandleCastSpellOpcode
    &AuraEffect::HandleOverrideActionbarSpells,                   //333 SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS_2 implemented in WorldSession::HandleCastSpellOpcode
    &AuraEffect::HandleNULL,                                      //334 SPELL_AURA_MOD_BLIND
    &AuraEffect::HandleAuraSeeWhileInvisible,                     //335 SPELL_AURA_SEE_WHILE_INVISIBLE
    &AuraEffect::HandleNULL,                                      //336 SPELL_AURA_MOD_FLYING_RESTRICTIONS
    &AuraEffect::HandleNoImmediateEffect,                         //337 SPELL_AURA_MOD_VENDOR_ITEMS_PRICES
    &AuraEffect::HandleNoImmediateEffect,                         //338 SPELL_AURA_MOD_DURABILITY_LOSS
    &AuraEffect::HandleNULL,                                      //339 SPELL_AURA_INCREASE_SKILL_GAIN_CHANCE
    &AuraEffect::HandleNULL,                                      //340 SPELL_AURA_SPELL_AURA_MOD_RESURRECTED_HEALTH_BY_GUILD_MEMBER
    &AuraEffect::HandleAuraModCategoryCooldown,                   //341 SPELL_AURA_MOD_SPELL_CATEGORY_COOLDOWN
    &AuraEffect::HandleModMeleeRangedSpeedPct,                    //342 SPELL_AURA_MOD_MELEE_RANGED_HASTE_2
    &AuraEffect::HandleNULL,                                      //343 SPELL_AURA_343
    &AuraEffect::HandleNULL,                                      //344 SPELL_AURA_MOD_AUTOATTACK_DAMAGE
    &AuraEffect::HandleNoImmediateEffect,                         //345 SPELL_AURA_BYPASS_ARMOR_FOR_CASTER
    &AuraEffect::HandleProgressBar,                               //346 SPELL_AURA_ENABLE_ALT_POWER
    &AuraEffect::HandleNULL,                                      //347 SPELL_AURA_MOD_SPELL_COOLDOWN_BY_HASTE
    &AuraEffect::HandleNoImmediateEffect,                         //348 SPELL_AURA_AURA_DEPOSIT_BONUS_MONEY_IN_GUILD_BANK_ON_LOOT implemented in WorldSession::HandleLootMoneyOpcode
    &AuraEffect::HandleNoImmediateEffect,                         //349 SPELL_AURA_MOD_CURRENCY_GAIN implemented in Player::ModifyCurrency (TODO?)
    &AuraEffect::HandleNULL,                                      //350 SPELL_AURA_MOD_ITEM_LOOT
    &AuraEffect::HandleNULL,                                      //351 SPELL_AURA_MOD_CURRENCY_LOOT
    &AuraEffect::HandleNULL,                                      //352 SPELL_AURA_ALLOW_WORGEN_TRANSFORM
    &AuraEffect::HandleModCamouflage,                             //353 SPELL_AURA_MOD_CAMOUFLAGE
    &AuraEffect::HandleNULL,                                      //354 SPELL_AURA_MOD_HEALING_DONE_FROM_PCT_HEALTH
    &AuraEffect::HandleModCastingSpeed,                           //355 SPELL_AURA_MOD_CASTING_SPEED
    &AuraEffect::HandleNULL,                                      //356 SPELL_AURA_MOD_DAMAGE_DONE_FROM_PCT_POWER
    &AuraEffect::HandleNULL,                                      //357 SPELL_AURA_ENABLE_BOSS1_UNIT_FRAME
    &AuraEffect::HandleNULL,                                      //358 SPELL_AURA_358
    &AuraEffect::HandleNoImmediateEffect,                         //359 SPELL_AURA_MOD_HEALING_DONE2 implemented in Unit::SpellBaseHealingBonusDone
    &AuraEffect::HandleNULL,                                      //360 SPELL_AURA_360
    &AuraEffect::HandleNULL,                                      //361 SPELL_AURA_361
    &AuraEffect::HandleUnused,                                    //362 unused (4.3.4)
    &AuraEffect::HandleNULL,                                      //363 SPELL_AURA_MOD_NEXT_SPELL
    &AuraEffect::HandleUnused,                                    //364 unused (4.3.4)
    &AuraEffect::HandleNULL,                                      //365 SPELL_AURA_365
    &AuraEffect::HandleOverrideSpellPowerByAttackPower,           //366 SPELL_AURA_OVERRIDE_SPELL_POWER_BY_AP_PCT  implemented in Unit::SpellBaseDamageBonus
    &AuraEffect::HandleNULL,                                      //367 SPELL_AURA_367
    &AuraEffect::HandleUnused,                                    //368 unused (4.3.4)
    &AuraEffect::HandleNULL,                                      //369 SPELL_SPELL_AURA_ENABLE_POWER_BAR_TIMER
    &AuraEffect::HandleNULL,                                      //370 SPELL_AURA_SET_FAIR_FAR_CLIP
    &AuraEffect::HandleNULL,                                      //371 SPELL_AURA_372
    &AuraEffect::HandleNULL,                                      //372 SPELL_AURA_372
    &AuraEffect::HandleAuraModIncreaseSpeed,                      //373 SPELL_AURA_INCREASE_MIN_SWIM_SPEED
    &AuraEffect::HandleNULL,                                      //374 SPELL_AURA_MOD_FALL_DAMAGE
    &AuraEffect::HandleNULL,                                      //375 SPELL_AURA_375
    &AuraEffect::HandleNULL,                                      //376 SPELL_AURA_MOD_CURRENCY_GAIN_FROM_CREATURE
    &AuraEffect::HandleNULL,                                      //377 SPELL_AURA_377
    &AuraEffect::HandleNULL,                                      //378 SPELL_AURA_378
    &AuraEffect::HandleModManaRegen,                              //379 SPELL_AURA_MOD_BASE_MANA_REGEN_PERCENT
    &AuraEffect::HandleNULL,                                      //380 SPELL_AURA_380
    &AuraEffect::HandleNoImmediateEffect,                         //381 SPELL_AURA_MOD_PET_HEALTH_FROM_OWNER_PCT implemented in Guardian::UpdateMaxHealth
    &AuraEffect::HandleAuraModPetStatsModifier,                   //382 SPELL_AURA_MOD_PET_STATS_MODIFIER
    &AuraEffect::HandleNULL,                                      //383 SPELL_AURA_383
    &AuraEffect::HandleNULL,                                      //384 SPELL_AURA_384
    &AuraEffect::HandleNoImmediateEffect,                         //385 SPELL_AURA_STRIKE_SELF in Unit::AttackerStateUpdate
    &AuraEffect::HandleNULL,                                      //386 SPELL_AURA_MOD_REST_GAINED
    &AuraEffect::HandleNULL,                                      //387 SPELL_AURA_MOD_VOID_STORAGE_AND_TRANSMOGRIFY_COST
    &AuraEffect::HandleNULL,                                      //388 SPELL_AURA_MOD_FLY_PATH_SPEED
    &AuraEffect::HandleNULL,                                      //389 SPELL_AURA_MOD_CAST_TIME_WHILE_MOVING
    &AuraEffect::HandleNULL,                                      //390 SPELL_AURA_390
    &AuraEffect::HandleNULL,                                      //391 SPELL_AURA_391
    &AuraEffect::HandleNULL,                                      //392 SPELL_AURA_392
    &AuraEffect::HandleNULL,                                      //393 SPELL_AURA_MOD_DEFLECT_SPELLS_FROM_FRONT
    &AuraEffect::HandleNULL,                                      //394 SPELL_AURA_LOOT_BONUS
    &AuraEffect::HandleCreateAreaTrigger,                         //395 SPELL_AURA_CREATE_AREATRIGGER
    &AuraEffect::HandleNULL,                                      //396 SPELL_AURA_PROC_ON_POWER_AMOUNT_2
    &AuraEffect::HandleBattlegroundFlag,                          //397 SPELL_AURA_BATTLEGROUND_FLAG
    &AuraEffect::HandleBattlegroundFlag,                          //398 SPELL_AURA_BATTLEGROUND_FLAG_2
    &AuraEffect::HandleNULL,                                      //399 SPELL_AURA_399
    &AuraEffect::HandleAuraModSkill,                              //400 SPELL_AURA_MOD_SKILL_2
    &AuraEffect::HandleNULL,                                      //401 SPELL_AURA_CART_AURA
    &AuraEffect::HandleNULL,                                      //402 SPELL_AURA_ENABLE_POWER_TYPE
    &AuraEffect::HandleNoImmediateEffect,                         //403 SPELL_AURA_MOD_SPELL_VISUAL
    &AuraEffect::HandleOverrideAttackPowerBySpellPower,           //404 SPELL_AURA_OVERRIDE_AP_BY_SPELL_POWER_PCT
    &AuraEffect::HandleIncreaseHasteFromItemsByPct,               //405 SPELL_AURA_INCREASE_HASTE_FROM_ITEMS_BY_PCT
    &AuraEffect::HandleNoImmediateEffect,                         //406 SPELL_AURA_OVERRIDE_CLIENT_CONTROLS
    &AuraEffect::HandleModFear,                                   //407 SPELL_AURA_MOD_FEAR_2
    &AuraEffect::HandleNULL,                                      //408 SPELL_AURA_PROC_SPELL_CHARGE removed by spell defined in EffectTriggerSpell
    &AuraEffect::HandleAuraGlide,                                 //409 SPELL_AURA_GLIDE
    &AuraEffect::HandleNULL,                                      //410 SPELL_AURA_410
    &AuraEffect::HandleAuraModCharges,                            //411 SPELL_AURA_MOD_CHARGES
    &AuraEffect::HandleModPowerRegen,                             //412 SPELL_AURA_HASTE_AFFECTS_BASE_MANA_REGEN
    &AuraEffect::HandleAuraModParryPercent,                       //413 SPELL_AURA_MOD_PARRY_PERCENT2
    &AuraEffect::HandleNULL,                                      //414 SPELL_AURA_MOD_DEFLECT_RANGED_ATTACKS
    &AuraEffect::HandleNULL,                                      //415 SPELL_AURA_415
    &AuraEffect::HandleNoImmediateEffect,                         //416 SPELL_AURA_MOD_SPELL_COOLDOWN_BY_MELEE_HASTE implemented in AuraEffect::CalculateSpellMod()
    &AuraEffect::HandleNoImmediateEffect,                         //417 SPELL_AURA_MOD_SPELL_GDC_BY_MELEE_HASTE
    &AuraEffect::HandleAuraModMaxPower,                           //418 SPELL_AURA_MOD_MAX_POWER
    &AuraEffect::HandleAuraModAddEnergyPercent,                   //419 SPELL_AURA_MOD_ADD_ENERGY_PERCENT
    &AuraEffect::HandleNULL,                                      //420 SPELL_AURA_420
    &AuraEffect::HandleNULL,                                      //421 SPELL_AURA_MOD_ABSORB
    &AuraEffect::HandleNULL,                                      //422 SPELL_AURA_MOD_ABSORBTION_PERCENT
    &AuraEffect::HandleNULL,                                      //423 SPELL_AURA_423
    &AuraEffect::HandleNULL,                                      //424 SPELL_AURA_424
    &AuraEffect::HandleNULL,                                      //425 SPELL_AURA_425
    &AuraEffect::HandleNULL,                                      //426 SPELL_AURA_426
    &AuraEffect::HandleNULL,                                      //427 SPELL_AURA_427
    &AuraEffect::HandleNULL,                                      //428 SPELL_AURA_SUMMON_CONTROLLER
    &AuraEffect::HandleNULL,                                      //429 SPELL_AURA_PET_DAMAGE_DONE_PCT
    &AuraEffect::HandleNULL,                                      //430 SPELL_AURA_ACTIVATE_SCENE
    &AuraEffect::HandleNULL,                                      //431 SPELL_AURA_CONTESTED_PVP
    &AuraEffect::HandleNULL,                                      //432 SPELL_AURA_432
    &AuraEffect::HandleNULL,                                      //433 SPELL_AURA_433
    &AuraEffect::HandleNULL,                                      //434 SPELL_AURA_434
    &AuraEffect::HandleNULL,                                      //435 SPELL_AURA_435
    &AuraEffect::HandleNULL,                                      //436 SPELL_AURA_436
    &AuraEffect::HandleNULL,                                      //437 SPELL_AURA_437
};

AuraEffect::AuraEffect(Aura* base, uint8 effIndex, int32 *baseAmount, Unit* caster):
m_base(base), m_spellInfo(base->GetSpellInfo()),
m_baseAmount(baseAmount ? *baseAmount : m_spellInfo->GetEffect(effIndex, m_diffMode).BasePoints),
m_spellmod(NULL), m_periodicTimer(0), m_tickNumber(0), m_effIndex(effIndex),
m_canBeRecalculated(true), m_isPeriodic(false),
m_oldbaseAmount(0), saveTarget(NULL), m_crit_amount(0), m_crit_chance(0.0f),
m_diffMode(caster ? caster->GetSpawnMode() : 0)
{
    m_send_baseAmount = m_baseAmount;
}

AuraEffect::~AuraEffect()
{
    delete m_spellmod;
}

void AuraEffect::GetTargetList(std::list<Unit*> & targetList) const
{
    Aura::ApplicationMap const & targetMap = GetBase()->GetApplicationMap();
    // remove all targets which were not added to new list - they no longer deserve area aura
    for (Aura::ApplicationMap::const_iterator appIter = targetMap.begin(); appIter != targetMap.end(); ++appIter)
    {
        if (appIter->second->HasEffect(GetEffIndex()))
            targetList.push_back(appIter->second->GetTarget());
    }
}

void AuraEffect::GetApplicationList(std::list<AuraApplication*> & applicationList) const
{
    Aura::ApplicationMap const & targetMap = GetBase()->GetApplicationMap();
    for (Aura::ApplicationMap::const_iterator appIter = targetMap.begin(); appIter != targetMap.end(); ++appIter)
    {
        if (appIter->second->HasEffect(GetEffIndex()))
            applicationList.push_back(appIter->second);
    }
}

int32 AuraEffect::CalculateAmount(Unit* caster, int32 &m_aura_amount)
{
    int32 amount;
    Item* castItem = NULL;

    if (uint64 itemGUID = GetBase()->GetCastItemGUID())
        if (Player* playerCaster = caster->ToPlayer())
            castItem = playerCaster->GetItemByGuid(itemGUID);

    Unit* target = GetBase()->GetUnitOwner();

    // default amount calculation
    m_send_baseAmount = m_spellInfo->GetEffect(m_effIndex, m_diffMode).CalcValue(caster, &m_baseAmount, GetBase()->GetOwner()->ToUnit(), castItem);
    amount = m_send_baseAmount;

    // check item enchant aura cast
    if (!amount && caster && castItem)
        if (castItem->GetItemSuffixFactor())
        {
            ItemRandomSuffixEntry const* item_rand_suffix = sItemRandomSuffixStore.LookupEntry(abs(castItem->GetItemRandomPropertyId()));
            if (item_rand_suffix)
            {
                for (int k = 0; k < MAX_ITEM_ENCHANTMENT_EFFECTS; k++)
                {
                    SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(item_rand_suffix->enchant_id[k]);
                    if (pEnchant)
                    {
                        for (int t = 0; t < MAX_ITEM_ENCHANTMENT_EFFECTS; t++)
                            if (pEnchant->spellid[t] == m_spellInfo->Id)
                        {
                            amount = uint32((item_rand_suffix->prefix[k]*castItem->GetItemSuffixFactor()) / 10000);
                            break;
                        }
                    }

                    if (amount)
                        break;
                }
            }
        }

    float DoneActualBenefit = 0.0f;
    bool CalcStack = bool(m_spellInfo->StackAmount);

    if (caster && caster->GetTypeId() == TYPEID_PLAYER && (m_spellInfo->AttributesEx8 & SPELL_ATTR8_MASTERY_SPECIALIZATION))
    {
        m_canBeRecalculated = false;
        amount += int32(caster->GetFloatValue(PLAYER_MASTERY) * m_spellInfo->GetEffect(m_effIndex, m_diffMode).BonusMultiplier + 0.5f);
    }

    // custom amount calculations go here
    switch (GetAuraType())
    {
        // crowd control auras
        //case SPELL_AURA_MOD_CONFUSE:
        case SPELL_AURA_MOD_FEAR:
        case SPELL_AURA_MOD_FEAR_2:
        //case SPELL_AURA_MOD_STUN:
        case SPELL_AURA_MOD_ROOT:
        case SPELL_AURA_TRANSFORM:
            m_canBeRecalculated = false;
            if (!m_spellInfo->ProcFlags)
                break;
            amount = int32(target->CountPctFromMaxHealth(10));
            if (caster)
            {
                // Glyphs increasing damage cap
                Unit::AuraEffectList const& overrideClassScripts = caster->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_CLASS_SCRIPTS);
                for (Unit::AuraEffectList::const_iterator itr = overrideClassScripts.begin(); itr != overrideClassScripts.end(); ++itr)
                {
                    if ((*itr)->IsAffectingSpell(m_spellInfo))
                    {
                        // Glyph of Frost nova and similar auras
                        if ((*itr)->GetMiscValue() == 7801)
                        {
                            AddPct(amount, (*itr)->GetAmount());
                            break;
                        }
                    }
                }
            }
            break;
        case SPELL_AURA_MOD_BASE_RESISTANCE_PCT:
        {
            if (!caster)
                break;

            switch (m_spellInfo->Id)
            {
                case 5487: // Bear Form
                {
                    if (Aura* aur = caster->GetAura(16931)) // Thick Hide
                        amount = aur->GetEffect(EFFECT_1)->GetAmount();
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELL_AURA_SCHOOL_ABSORB:
        {
            m_canBeRecalculated = false;

            if (!caster)
                break;

            switch (m_spellInfo->Id)
            {
                case 48707: // Anti-Magic Shell
                {
                    amount = caster->CountPctFromMaxHealth(m_spellInfo->Effects[m_effIndex].BasePoints);
                    break;
                }
                case 108416: // Sacrificial Pact
                {
                    Unit* spelltarget = caster->GetGuardianPet() ? caster->GetGuardianPet(): caster;
                    int32 sacrifiedHealth = spelltarget->CountPctFromCurHealth(m_spellInfo->Effects[EFFECT_1].BasePoints);
                    spelltarget->ModifyHealth(-sacrifiedHealth);
                    amount = CalculatePct(sacrifiedHealth, m_spellInfo->Effects[EFFECT_0].BasePoints);
                    break;
                }
                default:
                    break;
            }

            amount = caster->CalcAbsorb(target, m_spellInfo, amount);

            break;
        }
        case SPELL_AURA_MOD_ABSORB:
        {
            if (!caster)
                break;

            switch (m_spellInfo->Id)
            {
                case 134735: // Battle Fatigue
                {
                    amount = -60;
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELL_AURA_MOD_HEALING_PCT:
        {
            if (!caster)
                break;

            switch (m_spellInfo->Id)
            {
                case 134735: // Battle Fatigue
                {
                    amount = -60;
                    break;
                }
                case 114232: // Sanctified Wrath
                {
                    if (Player* paladin = caster->ToPlayer())
                    {
                        if (paladin->GetSpecializationId(paladin->GetActiveSpec()) != SPEC_PALADIN_PROTECTION)
                            amount = 0;
                    }
                    break;
                }
                case 73651: // Recuperate
                {
                    if (!caster->HasAura(146625))
                        amount = 0;
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELL_AURA_ADD_FLAT_MODIFIER:
        {
            if (!caster)
                break;

            switch (m_spellInfo->Id)
            {
                case 1178:   // Bear Form Passive
                {
                    if (caster->HasAura(114237))
                        amount = 15000;
                    break;
                }
                case 114232: // Sanctified Wrath
                {
                    if (Player* paladin = caster->ToPlayer())
                    {
                        if (paladin->GetSpecializationId(paladin->GetActiveSpec()) != SPEC_PALADIN_HOLY)
                            amount = 0;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELL_AURA_ADD_PCT_MODIFIER:
        {
            if (!caster)
                break;

            switch (m_spellInfo->Id)
            {
                case 32216: // Impending Victory
                {
                    if (caster && caster->ToPlayer()->HasSpellCooldown(103840))
                        caster->ToPlayer()->RemoveSpellCooldown(103840, true);
                }
                case 137639: // Storm, Earth and Fire
                {
                    if (GetBase()->GetStackAmount() == 2)
                    {
                        SpellInfo const* _spellinf = sSpellMgr->GetSpellInfo(138228);
                        amount = -100 + _spellinf->Effects[EFFECT_1].BasePoints;
                        CalcStack = false;
                    }
                    break;
                }
                case 145958: // Readiness - Death Knight Blood
                case 145992: // Readiness - Warrior Protection
                case 145962: // Readiness - Druid Guardian
                case 145976: // Readiness - Paladin Protection
                case 145967: // Readiness - Monk Brewmaster
                {
                    if (Aura * aura = caster->GetAura(146025))
                    {
                        if (AuraEffect* eff = aura->GetEffect(EFFECT_0))
                            amount = -(eff->GetAmount());
                    }
                    break;
                }
                case 145978: // Readiness - Paladin Retribution
                case 145990: // Readiness - Warrior Arms
                case 145991: // Readiness - Warrior Fury
                case 145959: // Readiness - Death Knight Frost
                case 145960: // Readiness - Death Knight Unholy
                {
                    if (Aura * aura = caster->GetAura(145955))
                    {
                        if (AuraEffect* eff = aura->GetEffect(EFFECT_0))
                            amount = -(eff->GetAmount());
                    }
                    break;
                }
                case 145983: // Readiness - Rogue Assassination
                case 145984: // Readiness - Rogue Combat
                case 145985: // Readiness - Rogue Subtlety
                case 145964: // Readiness - Hunter Beast Mastery
                case 145965: // Readiness - Hunter Marksmanship
                case 145966: // Readiness - Hunter Survival
                case 145969: // Readiness - Monk Windwalker
                case 145961: // Readiness - Druid Feral
                case 145986: // Readiness - Shaman Enhancement
                {
                    if (Aura * aura = caster->GetAura(146019))
                    {
                        if (AuraEffect* eff = aura->GetEffect(EFFECT_0))
                            amount = -(eff->GetAmount());
                    }
                    break;
                }
                case 114232: // Sanctified Wrath
                {
                    if (Player* paladin = caster->ToPlayer())
                    {
                        switch (paladin->GetSpecializationId(paladin->GetActiveSpec()))
                        {
                            case SPEC_PALADIN_HOLY:        if (m_effIndex != 0) amount = 0; break;
                            case SPEC_PALADIN_PROTECTION:  if (m_effIndex != 1) amount = 0; break;
                            case SPEC_PALADIN_RETRIBUTION: if (m_effIndex != 2) amount = 0; break;
                            default: amount = 0; break;
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELL_AURA_MOD_RESILIENCE_PCT:
        {
            if (!caster)
                break;

            switch (GetId())
            {
                case 115043: // Player Damage Reduction
                {
                    if (caster->getLevel() == 90)
                    {
                        amount = -7700;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELL_AURA_DUMMY:
        {
            if (!caster)
                break;

            switch (GetId())
            {
                case 76657: // Mastery: Master of Beasts
                {
                    if (Guardian* pet = caster->ToPlayer()->GetPet())
                    {
                        if (Aura* aur = pet->GetAura(8875))
                        {
                            aur->GetEffect(EFFECT_0)->SetCanBeRecalculated(true);
                            aur->GetEffect(EFFECT_1)->SetCanBeRecalculated(true);
                            aur->RecalculateAmountOfEffects();
                        }
                    }
                    break;
                }
                default:
                    break;
            }
            if(m_aura_amount && !m_baseAmount)
            {
                amount = m_aura_amount;
                return amount;
            }
            break;
        }
        case SPELL_AURA_PERIODIC_DUMMY:
        {
            switch (m_spellInfo->Id)
            {
                case 146198: // Essence of Yu'lon
                {
                    SpellInfo const* _info = sSpellMgr->GetSpellInfo(148008);
                    amount = caster->GetSpellPowerDamage(_info->GetSchoolMask()) * _info->Effects[EFFECT_0].BonusMultiplier / GetTotalTicks(); 
                    break;
                }
                default:
                    break;
            }
            if(m_aura_amount)
            {
                amount = m_aura_amount;
                return amount;
            }
            break;
        }
        case SPELL_AURA_MOD_DECREASE_SPEED:
        {
            if (!target)
                break;
                
            switch (GetId())
            {
                case 119450:
                {
                    if (target->isPet())
                    amount = 0;
                }
            }
        }
        case SPELL_AURA_BYPASS_ARMOR_FOR_CASTER:
        {
            if (!caster)
                break;

            switch (m_spellInfo->Id)
            {
                case 91021: // Find Weakness
                {
                    if (target)
                    {
                        if (target->GetTypeId() == TYPEID_PLAYER)
                            amount = 50;
                    }
                    break;
                }
                default:
                    break;
            }
        }
        case SPELL_AURA_MOD_SPELL_COOLDOWN_BY_MELEE_HASTE:
        {
            switch (GetId())
            {
                case 25956: // Sanctity of Battle
                {
                    if (Player* paladin = caster->ToPlayer())
                    {
                        amount  = -100;
                        amount += paladin->GetFloatValue(UNIT_MOD_HASTE) * 100.0f;

                        if (amount > 0) amount = 0;
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELL_AURA_PERIODIC_DAMAGE:
        {
            if (!caster || !target)
                break;

            switch (GetId())
            {
                case 124280: // Touch of Karma (Dot)
                {
                    uint8 getticks = GetTotalTicks();
                    uint32 oldBp = 0;

                    oldBp += target->GetRemainingPeriodicAmount(caster->GetGUID(), 124280, SPELL_AURA_PERIODIC_DAMAGE);

                    if (oldBp)
                        getticks++;

                    amount /= getticks;
                    amount += oldBp;
                    break;
                }
                case 106830: // Thrash (Cat)
                case 77758:  // Thrash
                {
                    amount += caster->GetTotalAttackPowerValue(BASE_ATTACK) * m_spellInfo->Effects[EFFECT_3].BasePoints / 1000;
                    break;
                }
                case 109076: // Incendiary Fireworks Launcher
                {
                    amount = irand(6000, 9000);
                    break;
                }
                case 1943: // Rupture
                {
                    m_canBeRecalculated = false;

                    if (caster->GetTypeId() != TYPEID_PLAYER)
                        break;

                    uint8 cp = caster->ToPlayer()->GetComboPoints();
                    float ap = caster->GetTotalAttackPowerValue(BASE_ATTACK);

                    switch (cp)
                    {
                    case 1:
                        amount += int32(ap * 0.1f / 4);
                        break;
                    case 2:
                        amount += int32(ap * 0.24f / 6);
                        break;
                    case 3:
                        amount += int32(ap * 0.40f / 8);
                        break;
                    case 4:
                        amount += int32(ap * 0.56f / 10);
                        break;
                    case 5:
                        amount += int32(ap * 0.744f / 12);
                        break;
                    default:
                        break;
                    }
                    break;
                }
                case 1079: // Rip
                {
                    m_canBeRecalculated = false;

                    if (caster->GetTypeId() != TYPEID_PLAYER)
                        break;

                    // Basepoint hotfix
                    amount *= 10;

                    uint8 cp = caster->ToPlayer()->GetComboPoints();
                    int32 AP = caster->GetTotalAttackPowerValue(BASE_ATTACK);

                    // In feral spec : 0.484 * $AP * cp
                    if (caster->ToPlayer()->GetSpecializationId(caster->ToPlayer()->GetActiveSpec()) == SPEC_DROOD_CAT)
                        amount += int32(cp * AP * 0.484f);
                    // In other spec : 0.387 * $AP * cp
                    else
                        amount += int32(cp * AP * 0.387f);

                    amount /= int32(GetBase()->GetMaxDuration() / GetBase()->GetEffect(0)->GetAmplitude());
                    break;
                }
                case 50536: // Unholy Blight damage over time effect
                {
                    m_canBeRecalculated = false;
                    // we're getting total damage on aura apply, change it to be damage per tick
                    amount = int32((float)amount / GetTotalTicks());
                    break;
                }
                default:
                    break;
            }

            amount = caster->SpellDamageBonusDone(target, m_spellInfo, amount, DOT, (SpellEffIndex) GetEffIndex());

            switch (m_spellInfo->Id)
            {
                case 32409:  // Shadow Word: Death (DoT)
                {
                    if (Aura* aura = caster->GetAura(105843))
                        if (AuraEffect* eff = aura->GetEffect(EFFECT_1))
                            AddPct(amount, -(eff->GetAmount()));
                    break;
                }
                case 114923: // Nether Tempest
                case 44457:  // Living Bomb
                {
                    if (target->GetTypeId() == TYPEID_PLAYER)
                        amount = CalculatePct(amount, 70);
                    break;
                }
                case 15407: // Mind Flay
                {
                    if (caster->HasAura(139139))
                    {
                        if (Aura* aur = target->GetAura(2944, caster->GetGUID()))
                        {
                            int32 addBonusPct = aur->GetEffect(EFFECT_2)->GetAmount();
                            AddPct(amount, addBonusPct);
                        }
                    }
                    break;
                }
                default:
                    break;
            }

            caster->isSpellCrit(target, m_spellInfo, m_spellInfo->GetSchoolMask(), BASE_ATTACK, m_crit_chance);
            m_crit_amount = caster->SpellCriticalDamageBonus(m_spellInfo, amount, target);
            break;
        }
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        {
            if(target && caster)
            {
                amount = uint32(target->CountPctFromMaxHealth(amount));
                caster->isSpellCrit(target, m_spellInfo, m_spellInfo->GetSchoolMask(), BASE_ATTACK, m_crit_chance);
                m_crit_amount = caster->SpellCriticalDamageBonus(m_spellInfo, amount, target);
            }
            break;
        }
        case SPELL_AURA_PERIODIC_LEECH:
        {
            if(target && caster)
            {
                amount = caster->SpellDamageBonusDone(target, m_spellInfo, amount, DOT, (SpellEffIndex) GetEffIndex());
                caster->isSpellCrit(target, m_spellInfo, m_spellInfo->GetSchoolMask(), BASE_ATTACK, m_crit_chance);
                m_crit_amount = caster->SpellCriticalDamageBonus(m_spellInfo, amount, target);
            }
            break;
        }
        case SPELL_AURA_PERIODIC_ENERGIZE:
            switch (m_spellInfo->Id)
            {
            case 57669: // Replenishment (0.2% from max)
                amount = CalculatePct(target->GetMaxPower(POWER_MANA), amount);
                break;
            case 61782: // Infinite Replenishment
                amount = target->GetMaxPower(POWER_MANA) * 0.0025f;
                break;
            case 48391: // Owlkin Frenzy
                ApplyPct(amount, target->GetCreatePowers(POWER_MANA));
                break;
            case 54428: // Divine Plea
                int32 manaPerc;
                manaPerc = CalculatePct(caster->GetMaxPower(POWER_MANA), m_spellInfo->Effects[1].BasePoints) / GetTotalTicks();
                amount = CalculatePct(caster->GetStat(STAT_SPIRIT), m_spellInfo->Effects[0].BasePoints);
                if(amount < manaPerc)
                   amount = manaPerc;
                break;
            case 29166: // Innervate
                if (!caster)
                    break;
                manaPerc = CalculatePct(caster->GetMaxPower(POWER_MANA), m_spellInfo->Effects[1].BasePoints) / 10;
                amount = CalculatePct(caster->GetStat(STAT_SPIRIT), m_spellInfo->Effects[0].BasePoints);
                if(amount < manaPerc)
                   amount = manaPerc;
                break;
            default:
                break;
            }
            break;
        case SPELL_AURA_PERIODIC_HEAL:
        {
            if (!caster || !target)
                break;

            switch (GetId())
            {
                case 6262: // Healthstone
                {
                    amount = CalculatePct(caster->GetMaxHealth(), caster->HasAura(56224) ? (m_spellInfo->Effects[EFFECT_1].BasePoints / 10) : 0);
                    break;
                }
                case 73651: // Recuperate
                {
                    int32 bp = m_spellInfo->Effects[0].BasePoints;
                    
                    if (caster->HasAura(56806))
                        bp += 1;
                    
                    if (caster->HasAura(61249))
                        bp += 1;
                    
                    amount = CalculatePct(caster->GetMaxHealth(), bp);
                    break;
                }
                case 28880:  // Warrior     - Gift of the Naaru
                case 59542:  // Paladin     - Gift of the Naaru
                case 59543:  // Hunter      - Gift of the Naaru
                case 59544:  // Priest      - Gift of the Naaru
                case 59545:  // Deathknight - Gift of the Naaru
                case 59547:  // Shaman      - Gift of the Naaru
                case 59548:  // Mage        - Gift of the Naaru
                case 121093: // Monk        - Gift of the Naaru
                {
                    amount = CalculatePct(caster->GetMaxHealth(), m_spellInfo->Effects[1].BasePoints) / GetTotalTicks();
                    break;
                }
                default:
                    break;
            }
            amount = caster->SpellHealingBonusDone(target, m_spellInfo, amount, DOT, (SpellEffIndex) GetEffIndex());

            switch (m_spellInfo->Id)
            {
                // Spirit Mend
                case 90361:
                {
                    uint32 ap = caster->GetTotalAttackPowerValue(BASE_ATTACK) * 2.7;
                    uint32 spd = caster->SpellDamageBonusDone(caster, m_spellInfo, amount, DOT, (SpellEffIndex) GetEffIndex());
                    amount += int32((ap * 0.35f) * 0.335) + spd;
                    break;
                }
                case 114163: // Eternal Flame
                {
                    if (target == caster)
                    {
                        int32 pct = m_spellInfo->Effects[EFFECT_2].BasePoints;

                        if (Aura* Bastion_of_Glory = caster->GetAura(114637))
                        {
                            if (AuraEffect* eff = Bastion_of_Glory->GetEffect(EFFECT_0))
                                pct += eff->GetAmount();
                        }
                        AddPct(amount, pct);
                    }
                    break;
                }
                default:
                    break;
            }

            caster->isSpellCrit(target, m_spellInfo, m_spellInfo->GetSchoolMask(), BASE_ATTACK, m_crit_chance);
            m_crit_amount = caster->SpellCriticalHealingBonus(m_spellInfo, amount, target);
            break;
        }
        case SPELL_AURA_MOD_THREAT:
        {
            uint8 level_diff = 0;
            float multiplier = 0.0f;
            switch (GetId())
            {
                // Arcane Shroud
                case 26400:
                    level_diff = target->getLevel() - 60;
                    multiplier = 2;
                    break;
                // The Eye of Diminution
                case 28862:
                    level_diff = target->getLevel() - 60;
                    multiplier = 1;
                    break;
            }
            if (level_diff > 0)
                amount += int32(multiplier * level_diff);
            break;
        }
        case SPELL_AURA_HASTE_SPELLS:
        {
            switch (m_spellInfo->Id)
            {                
                case 5760:   // Mind-numbing Poison  
                case 31589:  // Slow
                case 50274:
                case 73975:  // Necrotic Strike             
                case 58604:  // Lava Breath
                case 90315:  // Tailspin
                case 109466: // Curse of Enfeeblement
                case 109468: // Curse of Enfeeblement (Soulburn)
                case 116198: // Enfeeblement Aura (Metamorphosis)
                case 126406: // Trample
                {
                    if (target && target->GetTypeId() == TYPEID_PLAYER)
                        amount /= 5;
                    break;
                }
                case 25810:
                {
                    if (target && target->GetTypeId() == TYPEID_PLAYER)
                        amount /= 3;
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELL_AURA_MOD_INCREASE_HEALTH:
            // Vampiric Blood
            if (GetId() == 55233)
                amount = target->CountPctFromMaxHealth(amount);
            break;
        case SPELL_AURA_MOD_STAT:
        {
            switch (m_spellInfo->Id)
            {
                case 104993: // Jade Spirit
                {
                    if (caster->GetManaPct() > 25 && m_effIndex == EFFECT_1)
                        amount = 0;

                    break;
                }
                case 120032: // Dancing Steel
                {
                    int32 str = caster->GetStat(STAT_STRENGTH);
                    int32 agi = caster->GetStat(STAT_AGILITY);
                    
                    switch (m_effIndex)
                    {
                        case EFFECT_0: if (str > agi) amount = 0; break;
                        case EFFECT_1: if (str < agi) amount = 0; break;
                    }
                    break;
                }
                case 108300: // Killer Instinct
                {
                    if (caster)
                    {
                        amount = caster->GetStat(STAT_INTELLECT);
                    }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELL_AURA_MOD_INCREASE_HEALTH_2:
        case SPELL_AURA_MOD_INCREASE_HEALTH_3:
        {
            switch (m_spellInfo->Id)
            {
                case 12975:// Last Stand
                case 106922:// Might of Ursoc
                case 113072:// Might of Ursoc (Symbiosis)
                {
                    amount = CalculatePct(caster->GetMaxHealth(), amount);
                    break;
                }
                case 137211: // Tremendous Fortitude
                {
                    amount = CalculatePct(caster->GetMaxHealth(), amount);
                    if (amount > 18000)
                        amount = 18000;
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELL_AURA_MOD_INCREASE_SPEED:
            // Dash - do not set speed if not in cat form
            if (GetSpellInfo()->SpellFamilyName == SPELLFAMILY_DRUID && GetSpellInfo()->SpellFamilyFlags[2] & 0x00000008)
                amount = target->GetShapeshiftForm() == FORM_CAT ? amount : 0;
            break;
        case SPELL_AURA_MOD_DAMAGE_PERCENT_DONE:
        case SPELL_AURA_MOD_HEALING_DONE_PERCENT:
        {
            if (!caster)
                break;

            switch (m_spellInfo->Id)
            {
                case 138130: // Storm, Earth and Fire (clone)
                {
                    if (m_effIndex != EFFECT_1)
                        break;

                    if (Unit* owner = caster->GetOwner())
                        if (Aura* ownerAura = owner->GetAura(137639))
                        {
                            SpellInfo const* _spellinf = sSpellMgr->GetSpellInfo(138228);
                            switch (ownerAura->GetStackAmount())
                            {
                                case 1: amount = -100 + _spellinf->Effects[EFFECT_0].BasePoints; break;
                                case 2: amount = -100 + _spellinf->Effects[EFFECT_1].BasePoints; break;
                            }
                        }
                    break;
                }
                case 137639: // Storm, Earth and Fire
                {
                    if (GetBase()->GetStackAmount() == 2)
                    {
                        SpellInfo const* _spellinf = sSpellMgr->GetSpellInfo(138228);
                        amount = -100 + _spellinf->Effects[EFFECT_1].BasePoints;
                        CalcStack = false;
                    }
                    break;
                }
                case 116740: // Tigereye Brew
                {
                    if (Aura * aura = caster->GetAura(125195))
                    {
                        uint8 stuck   = aura->GetStackAmount();
                        uint8 residue = stuck;

                        if (stuck >= 10)
                            stuck = 10;

                        residue -= stuck;

                        if (m_effIndex == EFFECT_1)
                        {
                            if (residue != 0)
                            {
                                aura->SetStackAmount(residue);

                                if (residue < 10)
                                    caster->RemoveAura(137591);
                            }
                            else caster->RemoveAura(125195);
                        }
                        
                        amount *= stuck;
                    }

                    if (m_effIndex == EFFECT_1)
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

                                    cloneUnit->CastCustomSpell(cloneUnit, m_spellInfo->Id, &amount, &amount, NULL, true);
                                }
                            }
                        }
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE:
        {
            if (caster)
            {
                // if Level <= 70 resist = player level
                int32 resist = caster->getLevel();

                if (resist > 70 && resist < 81)
                    resist += (resist - 70) * 5;
                else if (resist > 80)
                    resist += ((resist-70) * 5 + (resist - 80) * 7);

                switch (GetId())
                {
                    case 20043: // Aspect of the Wild
                    case 8185:  // Elemental Resistance
                    case 19891: // Resistance Aura
                    case 79106: // Shadow Protection
                    case 79107: // Shadow Protection
                        amount = resist;
                        break;
                    case 79060: // Mark of the Wild
                    case 79061: // Mark of the Wild
                    case 79062: // Blessing of Kings
                    case 79063: // Blessing of Kings
                    case 90363: // Embrace of the Shale Spider
                        amount = resist / 2;
                        break;
                    }
                break;
            }
        }
        default:
            break;
    }

    // Mixology - Effect value mod
    if (caster && caster->GetTypeId() == TYPEID_PLAYER)
    {
        if (GetSpellInfo()->SpellFamilyName == SPELLFAMILY_POTION && (
            sSpellMgr->IsSpellMemberOfSpellGroup(GetId(), SPELL_GROUP_ELIXIR_BATTLE) ||
            sSpellMgr->IsSpellMemberOfSpellGroup(GetId(), SPELL_GROUP_ELIXIR_GUARDIAN)))
        {
            if (caster->HasAura(53042) && caster->HasSpell(GetSpellInfo()->Effects[0].TriggerSpell))
            {
                switch (GetId())
                {
                    case 53749: // Guru's Elixir
                        amount += 8;
                        break;
                    case 11328: // Elixir of Agility
                        amount += 10;
                        break;
                    case 28497: // Elixir of Mighty Agility
                    case 53747: // Elixir of Spirit
                    case 54212: // Flask of Pure Mojo
                    case 60340: // Elixir of Accuracy
                    case 60341: // Elixir of Deadly Strikes
                    case 60343: // Elixir of Mighty Defense
                    case 60344: // Elixir of Expertise
                    case 60345: // Elixir of Armor Piercing
                    case 60346: // Elixir of Lightning Speed
                    case 60347: // Elixir of Mighty Thoughts
                        amount += 20;
                        break;
                    case 53752: // Lesser Flask of Toughness
                    case 62380: // Lesser Flask of Resistance
                        amount += 40;
                        break;
                    case 53755: // Flask of the Frost Wyrm
                        amount += 47;
                        break;
                    case 53760: // Flask of Endless Rage
                        amount += 64;
                        break;
                    case 53751: // Elixir of Mighty Fortitude
                        amount += 200;
                        break;
                    case 53763: // Elixir of Protection
                        amount += 280;
                        break;
                    case 53758: // Flask of Stoneblood
                        amount += 320;
                        break;
                    // Cataclysm
                    case 79469: // Flask of Steelskin
                    case 79470: // Flask of the Draconic Mind
                    case 79471: // Flask of the Winds
                    case 79472: // Flask of Titanic Strength
                    case 94160: // Flask of Flowing Water
                        amount += 80;
                        break;
                    case 79635: // Flask of the Master
                    case 79632: // Elixir of Mighty Speed
                    case 79481: // Elixir of Impossible Accuracy
                    case 79477: // Elixir of the Cobra
                    case 79474: // Elixir of the Naga
                    case 79468: // Ghost Elixir
                        amount += 40;
                        break;
                    case 79480: // Elixir of Deep Earth
                        amount += 345;
                        break;
                    case 79631: // Prismatic Elixir
                        amount += 45;
                        break;
                    // Pandaria
                    case 105689: // Flask of Spring Blossoms
                    case 105691: // Flask of the Warm Sun
                    case 105696: // Flask of Winter's Bite
                        amount += 320;
                        break;
                    case 105694: // Flask of the Earth
                    case 105693: // Flask of Falling Leaves
                        amount += 480;
                        break;
                    //need info    
                  // case 105686: // Elixir of Perfection
                  // case 105687: // Elixir of Mirrors
                  // case 105685: // Elixir of Peace
                  // case 105684: // Elixir of the Rapids
                  // case 105682: // Mad Hozen Elixir
                  // case 105681: // Mantid Elixir
                  // case 105683: // Elixir of Weaponry
                  // case 105688: // Monk's Elixir
                }
            }
        }
    }
    
    if (DoneActualBenefit != 0.0f)
    {
        DoneActualBenefit *= caster->CalculateLevelPenalty(GetSpellInfo());
        amount += (int32)DoneActualBenefit;
    }

    GetBase()->CallScriptEffectCalcAmountHandlers(const_cast<AuraEffect const*>(this), amount, m_canBeRecalculated);

    CalculateFromDummyAmount(caster, target, amount);

    if (CalcStack)
    {
        amount *= GetBase()->GetStackAmount();
        m_crit_amount *= GetBase()->GetStackAmount();
    }

    switch (GetAuraType())
    {
        // Set amount for aura
        case SPELL_AURA_OBS_MOD_HEALTH:
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        case SPELL_AURA_PERIODIC_DAMAGE:
            m_aura_amount = amount;
            break;
    }

    return amount;
}

void AuraEffect::CalculateFromDummyAmount(Unit* caster, Unit* target, int32 &amount)
{
    if(!caster)
        return;

    if (std::vector<SpellAuraDummy> const* spellAuraDummy = sSpellMgr->GetSpellAuraDummy(GetId()))
    {
        for (std::vector<SpellAuraDummy>::const_iterator itr = spellAuraDummy->begin(); itr != spellAuraDummy->end(); ++itr)
        {
            if (!(itr->effectmask & (1<<m_effIndex)))
                continue;

            Unit* _caster = caster;
            Unit* _targetAura = caster;
            Unit* _target = target;
            bool check = false;

            if(itr->caster == 1 && target) //get caster as target
                _caster = target;

            if(itr->targetaura == 1) //get caster aura
                _targetAura = GetCaster();
            if(itr->targetaura == 2 && target) //get target aura
                _targetAura = target;

            if(!_targetAura)
                _targetAura = caster;

            switch (itr->option)
            {
                case SPELL_DUMMY_ENABLE: //0
                {
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && !_caster->HasAura(itr->spellDummyId))
                    {
                        amount = 0;
                        check = true;
                    }
                    if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                    {
                        amount = 0;
                        check = true;
                    }
                    break;
                }
                case SPELL_DUMMY_ADD_PERC: //1
                {
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && _caster->HasAura(itr->spellDummyId))
                    {
                        if(SpellInfo const* dummyInfo = sSpellMgr->GetSpellInfo(itr->spellDummyId))
                        {
                            int32 bp = dummyInfo->Effects[itr->effectDummy].BasePoints;
                            amount += CalculatePct(amount, bp);
                            check = true;
                        }
                    }
                    if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                    {
                        if(SpellInfo const* dummyInfo = sSpellMgr->GetSpellInfo(itr->spellDummyId))
                        {
                            int32 bp = dummyInfo->Effects[itr->effectDummy].BasePoints;
                            amount -= CalculatePct(amount, bp);
                            check = true;
                        }
                    }
                    break;
                }
                case SPELL_DUMMY_ADD_VALUE: //2
                {
                    if(itr->aura > 0 && !_targetAura->HasAura(itr->aura))
                        continue;
                    if(itr->aura < 0 && _targetAura->HasAura(abs(itr->aura)))
                        continue;

                    if(itr->spellDummyId > 0 && _caster->HasAura(itr->spellDummyId))
                    {
                        if(SpellInfo const* dummyInfo = sSpellMgr->GetSpellInfo(itr->spellDummyId))
                        {
                            int32 bp = dummyInfo->Effects[itr->effectDummy].BasePoints;
                            amount += bp;
                            check = true;
                        }
                    }
                    if(itr->spellDummyId < 0 && _caster->HasAura(abs(itr->spellDummyId)))
                    {
                        if(SpellInfo const* dummyInfo = sSpellMgr->GetSpellInfo(itr->spellDummyId))
                        {
                            int32 bp = dummyInfo->Effects[itr->effectDummy].BasePoints;
                            amount -= bp;
                            check = true;
                        }
                    }
                    break;
                }
            }
            if(check && itr->removeAura)
                _caster->RemoveAurasDueToSpell(itr->removeAura);
        }
    }
}

void AuraEffect::CalculatePeriodic(Unit* caster, bool resetPeriodicTimer /*= true*/, bool load /*= false*/)
{
    m_amplitude = m_spellInfo->GetEffect(m_effIndex, m_diffMode).Amplitude;

    // prepare periodics
    switch (GetAuraType())
    {
        case SPELL_AURA_OBS_MOD_POWER:
            // 3 spells have no amplitude set
            if (!m_amplitude)
                m_amplitude = 1 * IN_MILLISECONDS;
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_OBS_MOD_HEALTH:
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
        case SPELL_AURA_PERIODIC_ENERGIZE:
        case SPELL_AURA_PERIODIC_LEECH:
        case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
        case SPELL_AURA_PERIODIC_MANA_LEECH:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
        case SPELL_AURA_POWER_BURN:
        case SPELL_AURA_PERIODIC_DUMMY:
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
            if(GetBase()->GetMaxDuration())
                m_isPeriodic = true;
            else
                m_isPeriodic = false;
            break;
        default:
            break;
    }

    GetBase()->CallScriptEffectCalcPeriodicHandlers(const_cast<AuraEffect const*>(this), m_isPeriodic, m_amplitude);

    if (!m_isPeriodic)
        return;

    Player* modOwner = caster ? caster->GetSpellModOwner() : NULL;

    // Apply casting time mods
    if (m_amplitude)
    {
        // Apply periodic time mod
        if (modOwner)
            modOwner->ApplySpellMod(GetId(), SPELLMOD_ACTIVATION_TIME, m_amplitude);

        if (caster)
        {
            // Haste modifies periodic time of channeled spells
            if (m_spellInfo->IsChanneled())
            {
                if (m_spellInfo->AttributesEx5 & SPELL_ATTR5_HASTE_AFFECT_TICK_AND_CASTTIME)
                    caster->ModSpellCastTime(m_spellInfo, m_amplitude);
            }
            else
            {
                if (m_spellInfo->AttributesEx5 & SPELL_ATTR5_HASTE_AFFECT_TICK_AND_CASTTIME)
                    m_amplitude = int32(m_amplitude * caster->GetFloatValue(UNIT_MOD_CAST_HASTE));

                if (m_spellInfo->AttributesEx8 & SPELL_ATTR8_HASTE_AFFECT_DURATION)
                {
                    int32 _duration = GetBase()->GetMaxDuration() * caster->GetFloatValue(UNIT_MOD_CAST_HASTE);
                    GetBase()->SetMaxDuration(_duration);
                    GetBase()->SetDuration(_duration);
                }
            }
        }

        //! If duration nod defined should we change duration? this remove aura.
        if (!resetPeriodicTimer && !load && !(m_spellInfo->AttributesEx5 & SPELL_ATTR5_HASTE_AFFECT_TICK_AND_CASTTIME) && !(m_spellInfo->AttributesEx8 & SPELL_ATTR8_HASTE_AFFECT_DURATION) && m_spellInfo->DurationEntry->ID != 21 /* && GetBase()->GetMaxDuration() >= 0*/)
        {
            int32 dotduration = GetBase()->GetMaxDuration() + m_periodicTimer;
            GetBase()->SetMaxDuration(dotduration);
            GetBase()->SetDuration(dotduration);
        }
    }

    if (load) // aura loaded from db
    {
        m_tickNumber = m_amplitude ? GetBase()->GetDuration() / m_amplitude : 0;
        m_periodicTimer = m_amplitude ? GetBase()->GetDuration() % m_amplitude : 0;
        if (m_spellInfo->AttributesEx5 & SPELL_ATTR5_START_PERIODIC_AT_APPLY)
            ++m_tickNumber;
    }
    else // aura just created or reapplied
    {
        m_tickNumber = 0;
        // reset periodic timer on aura create or on reapply when aura isn't dot
        // possibly we should not reset periodic timers only when aura is triggered by proc
        // or maybe there's a spell attribute somewhere
        if (resetPeriodicTimer)
        {
            m_periodicTimer = 0;
            // Start periodic on next tick or at aura apply
            if (m_amplitude && !(m_spellInfo->AttributesEx5 & SPELL_ATTR5_START_PERIODIC_AT_APPLY))
                m_periodicTimer += m_amplitude;
        }
    }
}

void AuraEffect::CalculateSpellMod()
{
    switch (GetAuraType())
    {
        case SPELL_AURA_DUMMY:
            switch (GetSpellInfo()->SpellFamilyName)
            {
                case SPELLFAMILY_DRUID:
                    switch (GetId())
                    {
                        case 34246:                                 // Idol of the Emerald Queen
                        case 60779:                                 // Idol of Lush Moss
                        {
                            if (!m_spellmod)
                            {
                                m_spellmod = new SpellModifier(GetBase());
                                m_spellmod->op = SPELLMOD_DOT;
                                m_spellmod->type = SPELLMOD_FLAT;
                                m_spellmod->spellId = GetId();
                                m_spellmod->mask[1] = 0x0010;
                            }
                            m_spellmod->value = GetAmount()/7;
                        }
                        break;
                    }
                    break;
                default:
                    break;
            }
            break;
        case SPELL_AURA_MOD_SPELL_COOLDOWN_BY_MELEE_HASTE:
        case SPELL_AURA_ADD_FLAT_MODIFIER:
        case SPELL_AURA_ADD_PCT_MODIFIER:
        {
            if (!m_spellmod)
            {
                m_spellmod = new SpellModifier(GetBase());
                m_spellmod->op = SpellModOp(GetMiscValue());
                ASSERT(m_spellmod->op < MAX_SPELLMOD);

                m_spellmod->type = GetAuraType() == SPELL_AURA_MOD_SPELL_COOLDOWN_BY_MELEE_HASTE ? SPELLMOD_PCT: SpellModType(GetAuraType());    // SpellModType value == spell aura types
                m_spellmod->spellId = GetId();
                m_spellmod->mask = GetSpellInfo()->GetEffect(GetEffIndex(), m_diffMode).SpellClassMask;
                m_spellmod->charges = GetBase()->GetCharges();
            }
            m_spellmod->value = GetAmount();
            break;
        }
        default:
            break;
    }
    GetBase()->CallScriptEffectCalcSpellModHandlers(const_cast<AuraEffect const*>(this), m_spellmod);
}

void AuraEffect::ChangeAmount(int32 newAmount, bool mark, bool onStackOrReapply)
{
    // Reapply if amount change
    uint8 handleMask = 0;
    if (newAmount != GetAmount())
        handleMask |= AURA_EFFECT_HANDLE_CHANGE_AMOUNT;
    if (onStackOrReapply)
        handleMask |= AURA_EFFECT_HANDLE_REAPPLY;

    if (!handleMask)
        return;

    std::list<AuraApplication*> effectApplications;
    GetApplicationList(effectApplications);

    for (std::list<AuraApplication*>::const_iterator apptItr = effectApplications.begin(); apptItr != effectApplications.end(); ++apptItr)
        if ((*apptItr)->HasEffect(GetEffIndex()))
            HandleEffect(*apptItr, handleMask, false);

    if (handleMask & AURA_EFFECT_HANDLE_CHANGE_AMOUNT)
    {
        if (!mark)
            m_amount = newAmount;
        else
            SetAmount(newAmount);
        CalculateSpellMod();
    }

    if (handleMask & AURA_EFFECT_HANDLE_REAPPLY)
        CalculateSpellMod();

    for (std::list<AuraApplication*>::const_iterator apptItr = effectApplications.begin(); apptItr != effectApplications.end(); ++apptItr)
        if ((*apptItr)->HasEffect(GetEffIndex()))
            HandleEffect(*apptItr, handleMask, true);
}

void AuraEffect::HandleEffect(AuraApplication * aurApp, uint8 mode, bool apply)
{
    // check if call is correct, we really don't want using bitmasks here (with 1 exception)
    ASSERT(mode == AURA_EFFECT_HANDLE_REAL
        || mode == AURA_EFFECT_HANDLE_SEND_FOR_CLIENT
        || mode == AURA_EFFECT_HANDLE_CHANGE_AMOUNT
        || mode == AURA_EFFECT_HANDLE_STAT
        || mode == AURA_EFFECT_HANDLE_SKILL
        || mode == AURA_EFFECT_HANDLE_REAPPLY
        || mode == (AURA_EFFECT_HANDLE_CHANGE_AMOUNT | AURA_EFFECT_HANDLE_REAPPLY));

    // register/unregister effect in lists in case of real AuraEffect apply/remove
    // registration/unregistration is done always before real effect handling (some effect handlers code is depending on this)
    if (mode & AURA_EFFECT_HANDLE_REAL)
        aurApp->GetTarget()->_RegisterAuraEffect(this, apply);

    // real aura apply/remove, handle modifier
    if (mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_REAPPLY))
        ApplySpellMod(aurApp->GetTarget(), apply);

    // call scripts helping/replacing effect handlers
    bool prevented = false;
    if (apply)
        prevented = GetBase()->CallScriptEffectApplyHandlers(const_cast<AuraEffect const*>(this), const_cast<AuraApplication const*>(aurApp), (AuraEffectHandleModes)mode);
    else
        prevented = GetBase()->CallScriptEffectRemoveHandlers(const_cast<AuraEffect const*>(this), const_cast<AuraApplication const*>(aurApp), (AuraEffectHandleModes)mode);

    // check if script events have removed the aura or if default effect prevention was requested
    if ((apply && aurApp->GetRemoveMode()) || prevented)
        return;

    if (GetAuraType() >= TOTAL_AURAS)
        return;

    (*this.*AuraEffectHandler [GetAuraType()])(const_cast<AuraApplication const*>(aurApp), mode, apply);

    // check if script events have removed the aura or if default effect prevention was requested
    if (apply && aurApp->GetRemoveMode())
        return;

    // call scripts triggering additional events after apply/remove
    if (apply)
        GetBase()->CallScriptAfterEffectApplyHandlers(const_cast<AuraEffect const*>(this), const_cast<AuraApplication const*>(aurApp), (AuraEffectHandleModes)mode);
    else
        GetBase()->CallScriptAfterEffectRemoveHandlers(const_cast<AuraEffect const*>(this), const_cast<AuraApplication const*>(aurApp), (AuraEffectHandleModes)mode);
}

void AuraEffect::HandleEffect(Unit* target, uint8 mode, bool apply)
{
    AuraApplication* aurApp = GetBase()->GetApplicationOfTarget(target->GetGUID());
    ASSERT(aurApp);
    HandleEffect(aurApp, mode, apply);
}

void AuraEffect::ApplySpellMod(Unit* target, bool apply)
{
    if (!m_spellmod || target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->ToPlayer()->AddSpellMod(m_spellmod, apply);

    // Auras with charges do not mod amount of passive auras
    if (GetBase()->IsUsingCharges())
        return;
    // reapply some passive spells after add/remove related spellmods
    // Warning: it is a dead loop if 2 auras each other amount-shouldn't happen
    switch (GetMiscValue())
    {
        case SPELLMOD_ALL_EFFECTS:
        case SPELLMOD_EFFECT1:
        case SPELLMOD_EFFECT2:
        case SPELLMOD_EFFECT3:
        case SPELLMOD_EFFECT4:
        case SPELLMOD_EFFECT5:
        {
            uint64 guid = target->GetGUID();
            Unit::AuraApplicationMap & auras = target->GetAppliedAuras();
            for (Unit::AuraApplicationMap::iterator iter = auras.begin(); iter != auras.end(); ++iter)
            {
                Aura* aura = iter->second->GetBase();
                // only passive and permament auras-active auras should have amount set on spellcast and not be affected
                // if aura is casted by others, it will not be affected
                if ((aura->IsPassive() || aura->IsPermanent()) && aura->GetCasterGUID() == guid && aura->GetSpellInfo()->IsAffectedBySpellMod(m_spellmod))
                {
                    if (GetMiscValue() == SPELLMOD_ALL_EFFECTS)
                    {
                        for (uint8 i = 0; i<MAX_SPELL_EFFECTS; ++i)
                        {
                            if (AuraEffect* aurEff = aura->GetEffect(i))
                                aurEff->RecalculateAmount();
                        }
                    }
                    else if (GetMiscValue() == SPELLMOD_EFFECT1)
                    {
                       if (AuraEffect* aurEff = aura->GetEffect(0))
                            aurEff->RecalculateAmount();
                    }
                    else if (GetMiscValue() == SPELLMOD_EFFECT2)
                    {
                       if (AuraEffect* aurEff = aura->GetEffect(1))
                            aurEff->RecalculateAmount();
                    }
                    else if (GetMiscValue() == SPELLMOD_EFFECT3)
                    {
                       if (AuraEffect* aurEff = aura->GetEffect(2))
                            aurEff->RecalculateAmount();
                    }
                    else if (GetMiscValue() == SPELLMOD_EFFECT4)
                    {
                       if (AuraEffect* aurEff = aura->GetEffect(3))
                            aurEff->RecalculateAmount();
                    }
                    else if (GetMiscValue() == SPELLMOD_EFFECT5)
                    {
                       if (AuraEffect* aurEff = aura->GetEffect(4))
                            aurEff->RecalculateAmount();
                    }
                }
            }
        }
        default:
            break;
    }
}

void AuraEffect::Update(uint32 diff, Unit* caster)
{
    GetBase()->CallScriptEffectUpdateHandlers(diff, this);

    if (m_isPeriodic && (GetBase()->GetDuration() >=0 || GetBase()->IsPassive() || GetBase()->IsPermanent()))
    {
        if (m_periodicTimer > int32(diff))
            m_periodicTimer -= diff;
        else // tick also at m_periodicTimer == 0 to prevent lost last tick in case max m_duration == (max m_periodicTimer)*N
        {
            ++m_tickNumber;

            // update before tick (aura can be removed in TriggerSpell or PeriodicTick calls)
            m_periodicTimer += m_amplitude - diff;
            UpdatePeriodic(caster);

            std::list<AuraApplication*> effectApplications;
            GetApplicationList(effectApplications);
            // tick on targets of effects
            for (std::list<AuraApplication*>::const_iterator apptItr = effectApplications.begin(); apptItr != effectApplications.end(); ++apptItr)
                if ((*apptItr)->HasEffect(GetEffIndex()))
                    PeriodicTick(*apptItr, caster, (SpellEffIndex) GetEffIndex());
        }
    }
}

void AuraEffect::UpdatePeriodic(Unit* caster)
{
    switch (GetAuraType())
    {
        case SPELL_AURA_PERIODIC_DUMMY:
            switch (GetSpellInfo()->SpellFamilyName)
            {
                case SPELLFAMILY_GENERIC:
                    switch (GetId())
                    {
                        case 58549: // Tenacity
                        case 59911: // Tenacity (vehicle)
                           GetBase()->RefreshDuration();
                           break;
                        case 66823: case 67618: case 67619: case 67620: // Paralytic Toxin
                            // Get 0 effect aura
                            if (AuraEffect* slow = GetBase()->GetEffect(0))
                            {
                                int32 newAmount = slow->GetAmount() - 10;
                                if (newAmount < -100)
                                    newAmount = -100;
                                slow->ChangeAmount(newAmount);
                            }
                            break;
                        default:
                            break;
                    }
                    break;
                case SPELLFAMILY_MAGE:
                    if (GetId() == 55342)// Mirror Image
                        m_isPeriodic = false;
                    break;
                case SPELLFAMILY_DEATHKNIGHT:
                    // Chains of Ice
                    if (GetSpellInfo()->SpellFamilyFlags[1] & 0x00004000)
                    {
                        // Get 0 effect aura
                        if (AuraEffect* slow = GetBase()->GetEffect(0))
                        {
                            int32 newAmount = slow->GetAmount() + GetAmount();
                            if (newAmount > 0)
                                newAmount = 0;
                            slow->ChangeAmount(newAmount);
                        }
                        return;
                    }
                    break;
                default:
                    break;
           }
       default:
           break;
    }
    GetBase()->CallScriptEffectUpdatePeriodicHandlers(this);
}

bool AuraEffect::IsAffectingSpell(SpellInfo const* spell) const
{
    if (!spell)
        return false;
    // Check family name
    if (spell->SpellFamilyName != m_spellInfo->SpellFamilyName)
        return false;

    // Check EffectClassMask
    if (m_spellInfo->GetEffect(m_effIndex, m_diffMode).SpellClassMask & spell->SpellFamilyFlags)
        return true;

    return false;
}

void AuraEffect::SendTickImmune(Unit* target, Unit* caster) const
{
    if (caster)
        caster->SendSpellDamageImmune(target, m_spellInfo->Id);
}

void AuraEffect::PeriodicTick(AuraApplication * aurApp, Unit* caster, SpellEffIndex effIndex) const
{
    bool prevented = GetBase()->CallScriptEffectPeriodicHandlers(this, aurApp);
    if (prevented)
        return;

    Unit* target = aurApp->GetTarget();

    switch (GetAuraType())
    {
        case SPELL_AURA_PERIODIC_DUMMY:
            HandlePeriodicDummyAuraTick(target, caster, effIndex);
            break;
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL:
            HandlePeriodicTriggerSpellAuraTick(target, caster, effIndex);
            break;
        case SPELL_AURA_PERIODIC_TRIGGER_SPELL_WITH_VALUE:
            HandlePeriodicTriggerSpellWithValueAuraTick(target, caster, effIndex);
            break;
        case SPELL_AURA_PERIODIC_DAMAGE:
        case SPELL_AURA_PERIODIC_DAMAGE_PERCENT:
            HandlePeriodicDamageAurasTick(target, caster, effIndex);
            break;
        case SPELL_AURA_PERIODIC_LEECH:
            HandlePeriodicHealthLeechAuraTick(target, caster, effIndex);
            break;
        case SPELL_AURA_PERIODIC_HEALTH_FUNNEL:
            HandlePeriodicHealthFunnelAuraTick(target, caster, effIndex);
            break;
        case SPELL_AURA_PERIODIC_HEAL:
        case SPELL_AURA_OBS_MOD_HEALTH:
            HandlePeriodicHealAurasTick(target, caster, effIndex);
            break;
        case SPELL_AURA_PERIODIC_MANA_LEECH:
            HandlePeriodicManaLeechAuraTick(target, caster, effIndex);
            break;
        case SPELL_AURA_OBS_MOD_POWER:
            HandleObsModPowerAuraTick(target, caster, effIndex);
            break;
        case SPELL_AURA_PERIODIC_ENERGIZE:
            HandlePeriodicEnergizeAuraTick(target, caster, effIndex);
            break;
        case SPELL_AURA_POWER_BURN:
            HandlePeriodicPowerBurnAuraTick(target, caster, effIndex);
            break;
        default:
            break;
    }
}

void AuraEffect::HandleProc(AuraApplication* aurApp, ProcEventInfo& eventInfo, SpellEffIndex effIndex)
{
    bool prevented = GetBase()->CallScriptEffectProcHandlers(this, const_cast<AuraApplication const*>(aurApp), eventInfo);
    if (prevented)
        return;

    switch (GetAuraType())
    {
        case SPELL_AURA_PROC_TRIGGER_SPELL:
            HandleProcTriggerSpellAuraProc(aurApp, eventInfo, effIndex);
            break;
        case SPELL_AURA_PROC_TRIGGER_SPELL_WITH_VALUE:
            HandleProcTriggerSpellWithValueAuraProc(aurApp, eventInfo, effIndex);
            break;
        case SPELL_AURA_PROC_TRIGGER_DAMAGE:
            HandleProcTriggerDamageAuraProc(aurApp, eventInfo, effIndex);
            break;
        case SPELL_AURA_RAID_PROC_FROM_CHARGE:
            HandleRaidProcFromChargeAuraProc(aurApp, eventInfo, effIndex);
            break;
        case SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE:
        case SPELL_AURA_RAID_PROC_FROM_CHARGE_WITH_VALUE2:
            HandleRaidProcFromChargeWithValueAuraProc(aurApp, eventInfo, effIndex);
            break;
        default:
            break;
    }

    GetBase()->CallScriptAfterEffectProcHandlers(this, const_cast<AuraApplication const*>(aurApp), eventInfo);
}

void AuraEffect::CleanupTriggeredSpells(Unit* target)
{
    uint32 tSpellId = m_spellInfo->GetEffect(GetEffIndex(), m_diffMode).TriggerSpell;
    if (!tSpellId)
        return;

    SpellInfo const* tProto = sSpellMgr->GetSpellInfo(tSpellId);
    if (!tProto)
        return;

    if (tProto->GetDuration() != -1)
        return;

    // needed for spell 43680, maybe others
    // TODO: is there a spell flag, which can solve this in a more sophisticated way?
    if (m_spellInfo->GetEffect(GetEffIndex(), m_diffMode).ApplyAuraName == SPELL_AURA_PERIODIC_TRIGGER_SPELL &&
            uint32(m_spellInfo->GetDuration()) == m_spellInfo->GetEffect(GetEffIndex(), m_diffMode).Amplitude)
        return;

    target->RemoveAurasDueToSpell(tSpellId, GetCasterGUID());
}

void AuraEffect::HandleShapeshiftBoosts(Unit* target, bool apply) const
{
    std::vector<uint32> spellId;
    std::list<uint32> actionBarReplaceAuras;

    switch (GetMiscValue())
    {
        case FORM_CAT:
            spellId.push_back(3025);     // Wild Charge
            spellId.push_back(48629);    // Swipe, Mangle, Thrash
            spellId.push_back(106840);   // Skull Bash, Stampeding Roar, Berserk

            if (target->HasAura(108299)) // Killer Instinct
                spellId.push_back(108300);
            break;
        case FORM_TREE:
            break;
        case FORM_TRAVEL:
            spellId.push_back(5419);
            break;
        case FORM_AQUA:
            spellId.push_back(5421);
            break;
        case FORM_BEAR:
            spellId.push_back(1178);
            spellId.push_back(21178);  // Swipe, Wild Charge
            spellId.push_back(106829); // Mangle, Thrash, Skull Bash
            spellId.push_back(106899); // Stampeding Roar, Berserk

            if (target->HasAura(108299)) // Killer Instinct
                spellId.push_back(108300);
            break;
        case FORM_BATTLESTANCE:
            spellId.push_back(21156);
            break;
        case FORM_DEFENSIVESTANCE:
            spellId.push_back(7376);
            break;
        case FORM_BERSERKERSTANCE:
            spellId.push_back(7381);
            break;
        case FORM_MOONKIN:
            spellId.push_back(24905);
            spellId.push_back(24907);
            // Glyph of the Start
            if (!apply || target->HasAura(114301))
                spellId.push_back(114302);  // Astral Form
            break;
        case FORM_FLIGHT:
            spellId.push_back(33948);
            spellId.push_back(34764);
            break;
        case FORM_FLIGHT_EPIC:
            spellId.push_back(40122);
            spellId.push_back(40121);
            break;
        case FORM_METAMORPHOSIS:
            spellId.push_back(103965);
            spellId.push_back(54817);
            break;
        case FORM_SPIRITOFREDEMPTION:
            spellId.push_back(27792);
            spellId.push_back(27795);                               // must be second, this important at aura remove to prevent to early iterator invalidation.
            break;
        case FORM_SHADOW:
            spellId.push_back(49868);
            spellId.push_back(71167);
            break;
        case FORM_GHOSTWOLF:
            spellId.push_back(67116);
            break;
        case FORM_GHOUL:
        case FORM_AMBIENT:
        case FORM_STEALTH:
        case FORM_CREATURECAT:
        case FORM_CREATUREBEAR:
            break;
        default:
            break;
    }

    if (apply)
    {
        if (Player* plrTarget = target->ToPlayer())
        {
            // Remove cooldown of spells triggered on stance change - they may share cooldown with stance spell
            for (std::vector<uint32>::iterator itr = spellId.begin(); itr != spellId.end(); ++itr)
            {
                plrTarget->RemoveSpellCooldown(*itr);
                plrTarget->CastSpell(target, *itr, true, NULL, this);
            }

            PlayerSpellMap const& sp_list = plrTarget->GetSpellMap();
            for (PlayerSpellMap::const_iterator itr = sp_list.begin(); itr != sp_list.end(); ++itr)
            {
                if (itr->second->state == PLAYERSPELL_REMOVED || itr->second->disabled)
                    continue;

                bool hasAnalogy = false;

                for (std::vector<uint32>::iterator i = spellId.begin(); i != spellId.end(); ++i)
                    if (itr->first == (*i))
                    {
                        hasAnalogy = true;
                        break;
                    }

                if (hasAnalogy)
                    continue;

                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(itr->first);
                if (!spellInfo || !(spellInfo->Attributes & (SPELL_ATTR0_PASSIVE | SPELL_ATTR0_HIDDEN_CLIENTSIDE)))
                    continue;

                if (spellInfo->Stances & (1<<(GetMiscValue()-1)))
                    target->CastSpell(target, itr->first, true, NULL, this);
            }

            // Also do it for Glyphs
            for (uint32 i = 0; i < MAX_GLYPH_SLOT_INDEX; ++i)
            {
                if (uint32 glyphId = plrTarget->GetGlyph(plrTarget->GetActiveSpec(), i))
                {
                    if (GlyphPropertiesEntry const* glyph = sGlyphPropertiesStore.LookupEntry(glyphId))
                    {
                        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(glyph->SpellId);
                        if (!spellInfo || !(spellInfo->Attributes & (SPELL_ATTR0_PASSIVE | SPELL_ATTR0_HIDDEN_CLIENTSIDE)))
                            continue;

                        if (spellInfo->Stances & (1<<(GetMiscValue()-1)))
                            target->CastSpell(target, glyph->SpellId, true, NULL, this);
                    }
                }
            }

            // Leader of the Pack
            if (plrTarget->HasSpell(17007))
            {
                SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(24932);
                if (spellInfo && spellInfo->Stances & (1<<(GetMiscValue()-1)))
                    target->CastSpell(target, 24932, true, NULL, this);
            }

            // Improved Barkskin - apply/remove armor bonus due to shapeshift
            if (plrTarget->HasSpell(63410) || target->ToPlayer()->HasSpell(63411))
            {
                target->RemoveAurasDueToSpell(66530);
                if (GetMiscValue() == FORM_TRAVEL || GetMiscValue() == FORM_NONE) // "while in Travel Form or while not shapeshifted"
                    target->CastSpell(target, 66530, true);
            }

            switch (GetMiscValue())
            {
                case FORM_CAT:
                    // Savage Roar
                    if (target->GetAuraEffect(SPELL_AURA_DUMMY, SPELLFAMILY_DRUID, 0, 0x10000000, 0))
                        target->CastSpell(target, 62071, true);
                    // Master Shapeshifter - Cat
                    if (AuraEffect const* aurEff = target->GetDummyAuraEffect(SPELLFAMILY_GENERIC, 2851, 0))
                    {
                        int32 bp = aurEff->GetAmount();
                        target->CastCustomSpell(target, 48420, &bp, NULL, NULL, true);
                    }

                break;
                case FORM_BEAR:
                    // Master Shapeshifter - Bear
                    if (AuraEffect const* aurEff = target->GetDummyAuraEffect(SPELLFAMILY_GENERIC, 2851, 0))
                    {
                        int32 bp = aurEff->GetAmount();
                        target->CastCustomSpell(target, 48418, &bp, NULL, NULL, true);
                    }
                    // Survival of the Fittest
                    if (AuraEffect const* aurEff = target->GetAuraEffect(SPELL_AURA_MOD_TOTAL_STAT_PERCENTAGE, SPELLFAMILY_DRUID, 961, 0))
                    {
                        int32 bp = aurEff->GetSpellInfo()->Effects[EFFECT_2].CalcValue(GetCaster());
                        target->CastCustomSpell(target, 62069, &bp, NULL, NULL, true, 0, this);
                    }
                break;
                case FORM_MOONKIN:
                    // Master Shapeshifter - Moonkin
                    if (AuraEffect const* aurEff = target->GetDummyAuraEffect(SPELLFAMILY_GENERIC, 2851, 0))
                    {
                        int32 bp = aurEff->GetAmount();
                        target->CastCustomSpell(target, 48421, &bp, NULL, NULL, true);
                    }
                break;
            }
        }
    }
    else
    {
        for (std::vector<uint32>::iterator itr = spellId.begin(); itr != spellId.end(); ++itr)
            target->RemoveAurasDueToSpell(*itr);

        // Improved Barkskin - apply/remove armor bonus due to shapeshift
        if (Player* player=target->ToPlayer())
        {
            if (player->HasSpell(63410) || player->HasSpell(63411))
            {
                target->RemoveAurasDueToSpell(66530);
                target->CastSpell(target, 66530, true);
            }
        }

        const Unit::AuraEffectList& shapeshifts = target->GetAuraEffectsByType(SPELL_AURA_MOD_SHAPESHIFT);
        AuraEffect* newAura = NULL;
        // Iterate through all the shapeshift auras that the target has, if there is another aura with SPELL_AURA_MOD_SHAPESHIFT, then this aura is being removed due to that one being applied
        for (Unit::AuraEffectList::const_iterator itr = shapeshifts.begin(); itr != shapeshifts.end(); ++itr)
        {
            if ((*itr) != this)
            {
                newAura = *itr;
                break;
            }
        }
        Unit::AuraApplicationMap& tAuras = target->GetAppliedAuras();
        for (Unit::AuraApplicationMap::iterator itr = tAuras.begin(); itr != tAuras.end();)
        {
            // Use the new aura to see on what stance the target will be
            uint32 newStance = (1<<((newAura ? newAura->GetMiscValue() : 0)-1));

            // If the stances are not compatible with the spell, remove it
            if (itr->second->GetBase()->IsRemovedOnShapeLost(target) && !(itr->second->GetBase()->GetSpellInfo()->Stances & newStance))
                target->RemoveAura(itr);
            else
                ++itr;
        }
    }

    // Heart of the Wild
    if (apply && target->GetTypeId() == TYPEID_PLAYER)
    {
        Player* player = target->ToPlayer();
        if (player->HasSpell(108288))
        {
            ShapeshiftForm form = ShapeshiftForm(GetMiscValue());

            // check Heart of the Wild spec-specific buffs
            bool checkers[] = { target->HasAura(108291), target->HasAura(108292), target->HasAura(108293), target->HasAura(108294) };
            if ((checkers[0] || checkers[1] || checkers[3]) && form == FORM_BEAR)
                target->CastSpell(target, 123738, true);
            if ((checkers[0] || checkers[2] || checkers[3]) && form == FORM_CAT)
                target->CastSpell(target, 123737, true);
        }
    }
    else
    {
        target->RemoveAurasDueToSpell(123737);
        target->RemoveAurasDueToSpell(123738);
    }
}

/*********************************************************/
/***               AURA EFFECT HANDLERS                ***/
/*********************************************************/

/**************************************/
/***       VISIBILITY & PHASES      ***/
/**************************************/

void AuraEffect::HandleModInvisibilityDetect(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();
    InvisibilityType type = InvisibilityType(GetMiscValue());

    if (apply)
    {
        target->m_invisibilityDetect.AddFlag(type);
        target->m_invisibilityDetect.AddValue(type, GetAmount());
    }
    else
    {
        if (!target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY_DETECT))
            target->m_invisibilityDetect.DelFlag(type);

        target->m_invisibilityDetect.AddValue(type, -GetAmount());
    }

    // call functions which may have additional effects after chainging state of unit
    target->UpdateObjectVisibility();
}

void AuraEffect::HandleModInvisibility(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();
    InvisibilityType type = InvisibilityType(GetMiscValue());

    if (apply)
    {
        // apply glow vision
        //if (target->GetTypeId() == TYPEID_PLAYER)
            //target->SetByteFlag(PLAYER_FIELD_BYTES2, 3, PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW); // TODO : Check PLAYER_FIELD_AURA_VISION

        target->m_invisibility.AddFlag(type);
        target->m_invisibility.AddValue(type, GetAmount());
    }
    else
    {
        if (!target->HasAuraType(SPELL_AURA_MOD_INVISIBILITY))
        {
            // if not have different invisibility auras.
            // remove glow vision
            //if (target->GetTypeId() == TYPEID_PLAYER)
                //target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 3, PLAYER_FIELD_BYTE2_INVISIBILITY_GLOW); // TODO : Check PLAYER_FIELD_AURA_VISION

            target->m_invisibility.DelFlag(type);
        }
        else
        {
            bool found = false;
            Unit::AuraEffectList const& invisAuras = target->GetAuraEffectsByType(SPELL_AURA_MOD_INVISIBILITY);
            for (Unit::AuraEffectList::const_iterator i = invisAuras.begin(); i != invisAuras.end(); ++i)
            {
                if (GetMiscValue() == (*i)->GetMiscValue())
                {
                    found = true;
                    break;
                }
            }
            if (!found)
                target->m_invisibility.DelFlag(type);
        }

        target->m_invisibility.AddValue(type, -GetAmount());
    }

    // call functions which may have additional effects after chainging state of unit
    if (apply && (mode & AURA_EFFECT_HANDLE_REAL))
    {
        // drop flag at invisibiliy in bg
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
    }
    target->UpdateObjectVisibility();
}

//TODO: Finish this aura
void AuraEffect::HandleModCamouflage(AuraApplication const *aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
    {
        //
    }
    else if (!(target->isCamouflaged()))
    {
        if (Unit* pet = GetCaster()->GetGuardianPet())
            pet->RemoveAurasByType(SPELL_AURA_MOD_CAMOUFLAGE);
    }
}

void AuraEffect::HandleModStealthDetect(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();
    StealthType type = StealthType(GetMiscValue());

    if (apply)
    {
        target->m_stealthDetect.AddFlag(type);
        target->m_stealthDetect.AddValue(type, GetAmount());
    }
    else
    {
        if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH_DETECT))
            target->m_stealthDetect.DelFlag(type);

        target->m_stealthDetect.AddValue(type, -GetAmount());
    }

    // call functions which may have additional effects after chainging state of unit
    target->UpdateObjectVisibility();
}

void AuraEffect::HandleModStealth(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();
    StealthType type = StealthType(GetMiscValue());

    if (apply)
    {
        target->m_stealth.AddFlag(type);
        target->m_stealth.AddValue(type, GetAmount());

        target->SetStandFlags(UNIT_STAND_FLAGS_CREEP);
        //if (target->GetTypeId() == TYPEID_PLAYER)
            //target->SetByteFlag(PLAYER_FIELD_BYTES2, 3, PLAYER_FIELD_BYTE2_STEALTH); // TODO : Check PLAYER_FIELD_AURA_VISION
    }
    else
    {
        target->m_stealth.AddValue(type, -GetAmount());

        if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH)) // if last SPELL_AURA_MOD_STEALTH
        {
            target->m_stealth.DelFlag(type);

            target->RemoveStandFlags(UNIT_STAND_FLAGS_CREEP);
            //if (target->GetTypeId() == TYPEID_PLAYER)
                //target->RemoveByteFlag(PLAYER_FIELD_BYTES2, 3, PLAYER_FIELD_BYTE2_STEALTH); // TODO : Check PLAYER_FIELD_AURA_VISION
        }
    }

    // call functions which may have additional effects after chainging state of unit
    if (apply && (mode & AURA_EFFECT_HANDLE_REAL))
    {
        // drop flag at stealth in bg
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
    }
    target->UpdateObjectVisibility();
}

void AuraEffect::HandleModStealthLevel(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();
    StealthType type = StealthType(GetMiscValue());

    if (apply)
        target->m_stealth.AddValue(type, GetAmount());
    else
        target->m_stealth.AddValue(type, -GetAmount());

    // call functions which may have additional effects after chainging state of unit
    target->UpdateObjectVisibility();
}

void AuraEffect::HandleSpiritOfRedemption(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // prepare spirit state
    if (apply)
    {
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            // disable breath/etc timers
            target->ToPlayer()->StopMirrorTimers();

            // set stand state (expected in this form)
            if (!target->IsStandState())
                target->SetStandState(UNIT_STAND_STATE_STAND);
        }

        target->SetHealth(1);
    }
    // die at aura end
    else if (target->isAlive())
        // call functions which may have additional effects after chainging state of unit
        target->setDeathState(JUST_DIED);

    if (apply)
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
    else
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
}

void AuraEffect::HandleAuraGhost(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
    {
        target->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
        target->m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_GHOST);
        target->m_serverSideVisibilityDetect.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_GHOST);
    }
    else
    {
        if (target->HasAuraType(SPELL_AURA_GHOST))
            return;

        target->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
        target->m_serverSideVisibility.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE);
        target->m_serverSideVisibilityDetect.SetValue(SERVERSIDE_VISIBILITY_GHOST, GHOST_VISIBILITY_ALIVE);
    }
}

void AuraEffect::HandlePhase(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();


    if (Player* player = target->ToPlayer())
    {
        if (apply)	
            player->GetPhaseMgr().RegisterPhasingAuraEffect(this);	
        else	
            player->GetPhaseMgr().UnRegisterPhasingAuraEffect(this);
    }
    else
    {
        uint32 newPhase = 0;
        Unit::AuraEffectList const& phases = target->GetAuraEffectsByType(SPELL_AURA_PHASE);	
        if (!phases.empty())	
            for (Unit::AuraEffectList::const_iterator itr = phases.begin(); itr != phases.end(); ++itr)	
                newPhase |= (*itr)->GetMiscValue();

        Unit::AuraEffectList const& phases2 = target->GetAuraEffectsByType(SPELL_AURA_PHASE_2);	
        if (!phases2.empty())	
            for (Unit::AuraEffectList::const_iterator itr = phases2.begin(); itr != phases2.end(); ++itr)	
                newPhase |= (*itr)->GetMiscValue();

        if (!newPhase)
        {
            newPhase = PHASEMASK_NORMAL;
            if (Creature* creature = target->ToCreature())
                if (CreatureData const* data = sObjectMgr->GetCreatureData(creature->GetDBTableGUIDLow()))
                    newPhase = data->phaseMask;
        }

        target->SetPhaseMask(newPhase, false);
    }

    // call functions which may have additional effects after chainging state of unit
    // phase auras normally not expected at BG but anyway better check
    if (apply && (mode & AURA_EFFECT_HANDLE_REAL))
    {
        // drop flag at invisibiliy in bg
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
    }

    // need triggering visibility update base at phase update of not GM invisible (other GMs anyway see in any phases)
    if (target->IsVisible())
        target->UpdateObjectVisibility();
}

/**********************/
/***   UNIT MODEL   ***/
/**********************/

void AuraEffect::HandleAuraModShapeshift(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    uint32 modelid = 0;
    Powers PowerType = POWER_MANA;
    ShapeshiftForm form = ShapeshiftForm(GetMiscValue());

    switch (form)
    {
        case FORM_FIERCE_TIGER:
        case FORM_STURDY_OX:
        case FORM_CAT:                                      // 0x01
        case FORM_GHOUL:                                    // 0x07
            PowerType = POWER_ENERGY;
            break;

        case FORM_BEAR:                                     // 0x05

        case FORM_BATTLESTANCE:                             // 0x11
        case FORM_DEFENSIVESTANCE:                          // 0x12
        case FORM_BERSERKERSTANCE:                          // 0x13
            PowerType = POWER_RAGE;
            break;

        case FORM_TREE:                                     // 0x02
        case FORM_TRAVEL:                                   // 0x03
        case FORM_AQUA:                                     // 0x04
        case FORM_AMBIENT:                                  // 0x06

        case FORM_STEVES_GHOUL:                             // 0x09
        case FORM_THARONJA_SKELETON:                        // 0x0A
        case FORM_TEST_OF_STRENGTH:                         // 0x0B
        case FORM_BLB_PLAYER:                               // 0x0C
        case FORM_SHADOW_DANCE:                             // 0x0D
        case FORM_CREATUREBEAR:                             // 0x0E
        case FORM_CREATURECAT:                              // 0x0F
        case FORM_GHOSTWOLF:                                // 0x10
            break;
        case FORM_WISE_SERPENT:                             // 0x14
            PowerType = POWER_MANA;
            break;
        case FORM_ZOMBIE:                                   // 0x15
        case FORM_METAMORPHOSIS:                            // 0x16
        case FORM_UNDEAD:                                   // 0x19
        case FORM_MASTER_ANGLER:                            // 0x1A
        case FORM_FLIGHT_EPIC:                              // 0x1B
        case FORM_SHADOW:                                   // 0x1C
        case FORM_FLIGHT:                                   // 0x1D
        case FORM_STEALTH:                                  // 0x1E
        case FORM_MOONKIN:                                  // 0x1F
        case FORM_SPIRITOFREDEMPTION:                       // 0x20
            break;
        default:
            sLog->outError(LOG_FILTER_SPELLS_AURAS, "Auras: Unknown Shapeshift Type: %u", GetMiscValue());
    }

    modelid = target->GetModelForForm(form);

    if (apply)
    {
        // remove polymorph before changing display id to keep new display id
        switch (form)
        {
            case FORM_CAT:
            case FORM_TREE:
            case FORM_TRAVEL:
            case FORM_AQUA:
            case FORM_BEAR:
            case FORM_FLIGHT_EPIC:
            case FORM_FLIGHT:
            case FORM_MOONKIN:
            {
                // remove movement affects
                uint32 mechanicMask = (1 << MECHANIC_SNARE);
                if (target->HasAura(96429) || form == FORM_MOONKIN)
                    mechanicMask |= (1 << MECHANIC_ROOT);

                target->RemoveAurasWithMechanic(mechanicMask);

                // and polymorphic affects
                if (target->IsPolymorphed())
                    target->RemoveAurasDueToSpell(target->getTransForm());
                break;
            }
            default:
               break;
        }

        // remove other shapeshift before applying a new one
        target->RemoveAurasByType(SPELL_AURA_MOD_SHAPESHIFT, 0, GetBase());

        // stop handling the effect if it was removed by linked event
        if (aurApp->GetRemoveMode())
            return;

        if (modelid > 0)
            target->SetDisplayId(modelid);

        if (PowerType != POWER_MANA)
        {
            int32 oldPower = target->GetPower(PowerType);
            // reset power to default values only at power change
            if (target->getPowerType() != PowerType)
                target->setPowerType(PowerType);
        }

        // stop handling the effect if it was removed by linked event
        if (aurApp->GetRemoveMode())
            return;

        target->SetShapeshiftForm(form);
    }
    else
    {
        // reset model id if no other auras present
        // may happen when aura is applied on linked event on aura removal
        if (!target->HasAuraType(SPELL_AURA_MOD_SHAPESHIFT))
        {
            target->SetShapeshiftForm(FORM_NONE);
            if (target->getClass() == CLASS_DRUID)
            {
                target->setPowerType(POWER_MANA);
                // Remove movement impairing effects also when shifting out
                target->RemoveMovementImpairingAuras();
            }
        }

        if (modelid > 0)
            target->RestoreDisplayId();

        switch (form)
        {
            // Nordrassil Harness - bonus
            case FORM_BEAR:
            case FORM_CAT:
                if (AuraEffect* dummy = target->GetAuraEffect(37315, 0))
                    target->CastSpell(target, 37316, true, NULL, dummy);
                break;
            // Nordrassil Regalia - bonus
            case FORM_MOONKIN:
                if (AuraEffect* dummy = target->GetAuraEffect(37324, 0))
                    target->CastSpell(target, 37325, true, NULL, dummy);
                break;
            default:
                break;
        }
    }

    // adding/removing linked auras
    // add/remove the shapeshift aura's boosts
    HandleShapeshiftBoosts(target, apply);

    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->InitDataForForm();

    if (target->getClass() == CLASS_DRUID)
    {
        // Dash
        if (AuraEffect* aurEff = target->GetAuraEffect(SPELL_AURA_MOD_INCREASE_SPEED, SPELLFAMILY_DRUID, 0, 0, 0x8))
            aurEff->RecalculateAmount();

        // Disarm handling
        // If druid shifts while being disarmed we need to deal with that since forms aren't affected by disarm
        // and also HandleAuraModDisarm is not triggered
        if (!target->CanUseAttackType(BASE_ATTACK))
        {
            if (Item* pItem = target->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
            {
                target->ToPlayer()->_ApplyWeaponDamage(EQUIPMENT_SLOT_MAINHAND, pItem->GetTemplate(), NULL, apply);
            }
        }
    }

    // stop handling the effect if it was removed by linked event
    if (apply && aurApp->GetRemoveMode())
        return;

    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        SpellShapeshiftFormEntry const* shapeInfo = sSpellShapeshiftFormStore.LookupEntry(form);
        // Learn spells for shapeshift form - no need to send action bars or add spells to spellbook
        for (uint8 i = 0; i<MAX_SHAPESHIFT_SPELLS; ++i)
        {
            if (!shapeInfo->stanceSpell[i])
                continue;
            if (apply)
                target->ToPlayer()->AddTemporarySpell(shapeInfo->stanceSpell[i]);
            else
                target->ToPlayer()->RemoveTemporarySpell(shapeInfo->stanceSpell[i]);
        }
    }
}

void AuraEffect::HandleAuraTransform(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
    {
        target->RemoveAurasByType(SPELL_AURA_CLONE_CASTER);

        // update active transform spell only when transform or shapeshift not set or not overwriting negative by positive case
        if (!target->GetModelForForm(target->GetShapeshiftForm()) || !GetSpellInfo()->IsPositive())
        {
            // special case (spell specific functionality)
            if (GetMiscValue() == 0)
            {
                switch (GetId())
                {
                    // Orb of Deception
                    case 16739:
                    {
                        if (target->GetTypeId() != TYPEID_PLAYER)
                            return;

                        switch (target->getRace())
                        {
                            // Blood Elf
                            case RACE_BLOODELF:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 17829 : 17830);
                                break;
                            // Orc
                            case RACE_ORC:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 10139 : 10140);
                                break;
                            // Troll
                            case RACE_TROLL:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 10135 : 10134);
                                break;
                            // Tauren
                            case RACE_TAUREN:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 10136 : 10147);
                                break;
                            // Undead
                            case RACE_UNDEAD_PLAYER:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 10146 : 10145);
                                break;
                            // Draenei
                            case RACE_DRAENEI:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 17827 : 17828);
                                break;
                            // Dwarf
                            case RACE_DWARF:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 10141 : 10142);
                                break;
                            // Gnome
                            case RACE_GNOME:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 10148 : 10149);
                                break;
                            // Human
                            case RACE_HUMAN:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 10137 : 10138);
                                break;
                            // Night Elf
                            case RACE_NIGHTELF:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 10143 : 10144);
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    // Murloc costume
                    case 42365:
                        target->SetDisplayId(21723);
                        break;
                    // Dread Corsair
                    case 50517:
                    // Corsair Costume
                    case 51926:
                    {
                        if (target->GetTypeId() != TYPEID_PLAYER)
                            return;

                        switch (target->getRace())
                        {
                            // Blood Elf
                            case RACE_BLOODELF:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 25032 : 25043);
                                break;
                            // Orc
                            case RACE_ORC:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 25039 : 25050);
                                break;
                            // Troll
                            case RACE_TROLL:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 25041 : 25052);
                                break;
                            // Tauren
                            case RACE_TAUREN:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 25040 : 25051);
                                break;
                            // Undead
                            case RACE_UNDEAD_PLAYER:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 25042 : 25053);
                                break;
                            // Draenei
                            case RACE_DRAENEI:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 25033 : 25044);
                                break;
                            // Dwarf
                            case RACE_DWARF:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 25034 : 25045);
                                break;
                            // Gnome
                            case RACE_GNOME:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 25035 : 25046);
                                break;
                            // Human
                            case RACE_HUMAN:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 25037 : 25048);
                                break;
                            // Night Elf
                            case RACE_NIGHTELF:
                                target->SetDisplayId(target->getGender() == GENDER_MALE ? 25038 : 25049);
                                break;
                            default:
                                break;
                        }
                        break;
                    }
                    // Pygmy Oil
                    case 53806:
                        target->SetDisplayId(22512);
                        break;
                    // Honor the Dead
                    case 65386:
                    case 65495:
                        target->SetDisplayId(target->getGender() == GENDER_MALE ? 29203 : 29204);
                        break;
                    // Darkspear Pride
                    case 75532:
                        target->SetDisplayId(target->getGender() == GENDER_MALE ? 31737 : 31738);
                        break;
                    // Gnomeregan Pride
                    case 75531:
                        target->SetDisplayId(target->getGender() == GENDER_MALE ? 31654 : 31655);
                        break;
                    default:
                        break;
                }
            }
            else
            {
                CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(GetMiscValue());
                if (!ci)
                {
                    target->SetDisplayId(16358);              // pig pink ^_^
                    sLog->outError(LOG_FILTER_SPELLS_AURAS, "Auras: unknown creature id = %d (only need its modelid) From Spell Aura Transform in Spell ID = %d", GetMiscValue(), GetId());
                }
                else
                {
                    uint32 model_id = 0;

                    if (uint32 modelid = ci->GetRandomValidModelId())
                        model_id = modelid;                     // Will use the default model here

                    // Polymorph (sheep)
                    if (GetSpellInfo()->SpellFamilyName == SPELLFAMILY_MAGE && GetSpellInfo()->SpellIconID == 82 && GetSpellInfo()->SpellVisual[0] == 12978)
                        if (Unit* caster = GetCaster())
                        {
                            if (caster->HasAura(52648))         // Glyph of the Penguin
                                model_id = 26452;
                            else if (caster->HasAura(57924))         // Glyph of a porcupine
                                model_id = 40119;
                            else if (caster->HasAura(57927))         // Glyph of a monkey
                                model_id = 29963;
                            else if (caster->HasAura(58136))         // Glyph of a polar bear
                                model_id = 23948;
                        }

                    target->SetDisplayId(model_id);

                    // Dragonmaw Illusion (set mount model also)
                    if (GetId() == 42016 && target->GetMountID() && !target->GetAuraEffectsByType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED).empty())
                        target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 16314);
                }
            }
        }

        // update active transform spell only when transform or shapeshift not set or not overwriting negative by positive case
        SpellInfo const* transformSpellInfo = sSpellMgr->GetSpellInfo(target->getTransForm());
        if (!transformSpellInfo || !GetSpellInfo()->IsPositive() || transformSpellInfo->IsPositive())
            target->setTransForm(GetId());

        // polymorph case
        if ((mode & AURA_EFFECT_HANDLE_REAL) && target->GetTypeId() == TYPEID_PLAYER && target->IsPolymorphed())
        {
            // for players, start regeneration after 1s (in polymorph fast regeneration case)
            // only if caster is Player (after patch 2.4.2)
            if (IS_PLAYER_GUID(GetCasterGUID()))
                target->ToPlayer()->setRegenTimerCount(1*IN_MILLISECONDS);

            //dismount polymorphed target (after patch 2.4.2)
            if (target->IsMounted())
                target->RemoveAurasByType(SPELL_AURA_MOUNTED);
        }
    }
    else
    {
        // HandleEffect(this, AURA_EFFECT_HANDLE_SEND_FOR_CLIENT, true) will reapply it if need
        if (target->getTransForm() == GetId())
            target->setTransForm(0);

        target->RestoreDisplayId();

        // Dragonmaw Illusion (restore mount model)
        if (GetId() == 42016 && target->GetMountID() == 16314)
        {
            if (!target->GetAuraEffectsByType(SPELL_AURA_MOUNTED).empty())
            {
                uint32 cr_id = target->GetAuraEffectsByType(SPELL_AURA_MOUNTED).front()->GetMiscValue();
                if (CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(cr_id))
                {
                    uint32 team = 0;
                    if (target->GetTypeId() == TYPEID_PLAYER)
                        team = target->ToPlayer()->GetTeam();

                    uint32 displayID = sObjectMgr->ChooseDisplayId(team, ci);
                    sObjectMgr->GetCreatureModelRandomGender(&displayID);

                    target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, displayID);
                }
            }
        }
    }
}

void AuraEffect::HandleAuraModScale(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    target->ApplyPercentModFloatValue(OBJECT_FIELD_SCALE_X, (float)GetAmount(), apply);
}

void AuraEffect::HandleAuraCloneCaster(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
    {
        Unit* caster = GetCaster();
        if (!caster || caster == target)
            return;

        // What must be cloned? at least display and scale
        target->SetDisplayId(caster->GetDisplayId());
        //target->SetObjectScale(caster->GetFloatValue(OBJECT_FIELD_SCALE_X)); // we need retail info about how scaling is handled (aura maybe?)
        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_MIRROR_IMAGE);
    }
    else
    {
        target->SetDisplayId(target->GetNativeDisplayId());
        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_MIRROR_IMAGE);
    }
}

void AuraEffect::HandleAuraInitializeImages(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
    {
        Unit* caster = GetCaster();
        if (!caster || caster == target)
            return;

        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_MIRROR_IMAGE);
    }
    else
        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_MIRROR_IMAGE);
}

/************************/
/***      FIGHT       ***/
/************************/

void AuraEffect::HandleFeignDeath(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
    {
        /*
        WorldPacket data(SMSG_FEIGN_DEATH_RESISTED, 0);
        target->SendMessageToSet(&data, true);
        */

        UnitList targets;
        Trinity::AnyUnfriendlyUnitInObjectRangeCheck u_check(target, target, target->GetMap()->GetVisibilityRange());
        Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(target, targets, u_check);
        target->VisitNearbyObject(target->GetMap()->GetVisibilityRange(), searcher);
        for (UnitList::iterator iter = targets.begin(); iter != targets.end(); ++iter)
        {
            if (!(*iter)->HasUnitState(UNIT_STATE_CASTING))
                continue;

            for (uint32 i = CURRENT_FIRST_NON_MELEE_SPELL; i < CURRENT_MAX_SPELL; i++)
            {
                if ((*iter)->GetCurrentSpell(i)
                && (*iter)->GetCurrentSpell(i)->m_targets.GetUnitTargetGUID() == target->GetGUID())
                {
                    (*iter)->InterruptSpell(CurrentSpellTypes(i), false);
                }
            }
        }
        target->CombatStop();
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

        // prevent interrupt message
        if (GetCasterGUID() == target->GetGUID() && target->GetCurrentSpell(CURRENT_GENERIC_SPELL))
            target->FinishSpell(CURRENT_GENERIC_SPELL, false);
        target->InterruptNonMeleeSpells(true);
        target->getHostileRefManager().deleteReferences();

        // stop handling the effect if it was removed by linked event
        if (aurApp->GetRemoveMode())
            return;
                                                            // blizz like 2.0.x
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
                                                            // blizz like 2.0.x
        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                                                            // blizz like 2.0.x
        target->SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);

        target->AddUnitState(UNIT_STATE_DIED);
    }
    else
    {
        /*
        WorldPacket data(SMSG_FEIGN_DEATH_RESISTED, 0);
        target->SendMessageToSet(&data, true);
        */
                                                            // blizz like 2.0.x
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29);
                                                            // blizz like 2.0.x
        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                                                            // blizz like 2.0.x
        target->RemoveFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_DEAD);

        target->ClearUnitState(UNIT_STATE_DIED);

        Powers power = target->getPowerType();
        target->SetPower(power, target->GetPower(power));
    }
}

void AuraEffect::HandleModUnattackable(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
    if (!apply && target->HasAuraType(SPELL_AURA_MOD_UNATTACKABLE))
        return;

    target->ApplyModFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE, apply);

    // call functions which may have additional effects after chainging state of unit
    if (apply && (mode & AURA_EFFECT_HANDLE_REAL))
    {
        target->CombatStop();
        target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);
    }
}

void AuraEffect::HandleAuraModDisarm(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    AuraType type = GetAuraType();

    //Prevent handling aura twice
    if ((apply) ? target->GetAuraEffectsByType(type).size() > 1 : target->HasAuraType(type))
        return;

    uint32 field, flag, slot;
    WeaponAttackType attType;
    switch (type)
    {
    case SPELL_AURA_MOD_DISARM:
        field=UNIT_FIELD_FLAGS;
        flag=UNIT_FLAG_DISARMED;
        slot=EQUIPMENT_SLOT_MAINHAND;
        attType=BASE_ATTACK;
        break;
    case SPELL_AURA_MOD_DISARM_OFFHAND:
        field=UNIT_FIELD_FLAGS_2;
        flag=UNIT_FLAG2_DISARM_OFFHAND;
        slot=EQUIPMENT_SLOT_OFFHAND;
        attType=OFF_ATTACK;
        break;
    case SPELL_AURA_MOD_DISARM_RANGED:
        /*field=UNIT_FIELD_FLAGS_2;
        flag=UNIT_FLAG2_DISARM_RANGED;
        slot=EQUIPMENT_SLOT_MAINHAND;
        attType=RANGED_ATTACK;
        break*/
    default:
        return;
    }

    // if disarm aura is to be removed, remove the flag first to reapply damage/aura mods
    if (!apply)
        target->RemoveFlag(field, flag);

    // Handle damage modification, shapeshifted druids are not affected
    if (target->GetTypeId() == TYPEID_PLAYER && !target->IsInFeralForm())
    {
        if (Item* pItem = target->ToPlayer()->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
        {
            uint8 attacktype = Player::GetAttackBySlot(slot);

            if (attacktype < MAX_ATTACK)
            {
                target->ToPlayer()->_ApplyWeaponDamage(slot, pItem->GetTemplate(), NULL, !apply);
                target->ToPlayer()->_ApplyWeaponDependentAuraMods(pItem, WeaponAttackType(attacktype), !apply);
            }
        }
    }

    // if disarm effects should be applied, wait to set flag until damage mods are unapplied
    if (apply)
        target->SetFlag(field, flag);

    if (target->GetTypeId() == TYPEID_UNIT && target->ToCreature()->GetCurrentEquipmentId())
        target->UpdateDamagePhysical(attType);
}

void AuraEffect::HandleAuraModSilence(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
    {
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);

        // call functions which may have additional effects after chainging state of unit
        // Stop cast only spells vs PreventionType == SPELL_PREVENTION_TYPE_SILENCE
        for (uint32 i = CURRENT_MELEE_SPELL; i < CURRENT_MAX_SPELL; ++i)
            if (Spell* spell = target->GetCurrentSpell(CurrentSpellTypes(i)))
                if (spell->m_spellInfo->PreventionType == SPELL_PREVENTION_TYPE_SILENCE)
                    // Stop spells on prepare or casting state
                    target->InterruptSpell(CurrentSpellTypes(i), false);
    }
    else
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(SPELL_AURA_MOD_SILENCE) || target->HasAuraType(SPELL_AURA_MOD_PACIFY_SILENCE))
            return;

        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SILENCED);
    }
}

void AuraEffect::HandleAuraModPacify(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
    else
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(SPELL_AURA_MOD_PACIFY) || target->HasAuraType(SPELL_AURA_MOD_PACIFY_SILENCE))
            return;
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PACIFIED);
    }
}

void AuraEffect::HandleAuraModPacifyAndSilence(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    // Vengeance of the Blue Flight (TODO: REMOVE THIS!)
    if (m_spellInfo->Id == 45839)
    {
        if (apply)
            target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
        else
            target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
    }
    if (!(apply))
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(SPELL_AURA_MOD_PACIFY_SILENCE))
            return;
    }
    HandleAuraModPacify(aurApp, mode, apply);
    HandleAuraModSilence(aurApp, mode, apply);
}

void AuraEffect::HandleAuraAllowOnlyAbility(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        if (apply)
            target->SetFlag(PLAYER_FLAGS, PLAYER_ALLOW_ONLY_ABILITY);
        else
        {
            // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
            if (target->HasAuraType(SPELL_AURA_ALLOW_ONLY_ABILITY))
                return;
            target->RemoveFlag(PLAYER_FLAGS, PLAYER_ALLOW_ONLY_ABILITY);
        }
    }
}

/****************************/
/***      TRACKING        ***/
/****************************/

void AuraEffect::HandleAuraTrackCreatures(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    uint32 mask = 1 << (GetMiscValue() - 1);
    // Track Humanoids
    if (GetId() == 5225)
        // Glyph of the Predator
        if (!apply && target->HasAuraTypeWithMiscvalue(SPELL_AURA_TRACK_CREATURES, GetMiscValue()) || target->HasAura(114280))
            mask |= 1 << (CREATURE_TYPE_BEAST - 1);

    target->ApplyModFlag(PLAYER_TRACK_CREATURES, mask, apply);
}

void AuraEffect::HandleAuraTrackResources(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->SetUInt32Value(PLAYER_TRACK_RESOURCES, (apply) ? ((uint32)1)<<(GetMiscValue()-1): 0);
}

void AuraEffect::HandleAuraTrackStealthed(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    if (!(apply))
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
    }
    target->ApplyModFlag(PLAYER_FIELD_BYTES, PLAYER_FIELD_BYTE_TRACK_STEALTHED, apply);
}

void AuraEffect::HandleAuraModStalked(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    // used by spells: Hunter's Mark, Mind Vision, Syndicate Tracker (MURP) DND
    if (apply)
        target->SetFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
    else
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (!target->HasAuraType(GetAuraType()))
            target->RemoveFlag(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_TRACK_UNIT);
    }

    // call functions which may have additional effects after chainging state of unit
    target->UpdateObjectVisibility();
}

void AuraEffect::HandleAuraUntrackable(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
        target->SetByteFlag(UNIT_FIELD_BYTES_1, 2, UNIT_STAND_FLAGS_UNTRACKABLE);
    else
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
        target->RemoveByteFlag(UNIT_FIELD_BYTES_1, 2, UNIT_STAND_FLAGS_UNTRACKABLE);
    }
}

/****************************/
/***  SKILLS & TALENTS    ***/
/****************************/

void AuraEffect::HandleAuraModPetTalentsPoints(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;
}

void AuraEffect::HandleAuraModSkill(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_SKILL)))
        return;

    Player* target = aurApp->GetTarget()->ToPlayer();
    if (!target)
        return;

    uint32 prot = GetMiscValue();
    int32 points = GetAmount();


    if (prot == SKILL_DEFENSE)
        return;

    target->ModifySkillBonus(prot, (apply ? points : -points), GetAuraType() == SPELL_AURA_MOD_SKILL_TALENT);
}

/****************************/
/***       MOVEMENT       ***/
/****************************/

void AuraEffect::HandleAuraMounted(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
    {
        uint32 creatureEntry = GetMiscValue();
        uint32 displayId = 0;
        uint32 vehicleId = 0;

        // Festive Holiday Mount
        if (target->HasAura(62061))
        {
            if (GetBase()->HasEffectType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED))
                creatureEntry = 24906;
            else
                creatureEntry = 15665;
        }

        if (CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(creatureEntry))
        {
            uint32 team = 0;
            if (target->GetTypeId() == TYPEID_PLAYER)
                team = target->ToPlayer()->GetTeam();

            displayId = ObjectMgr::ChooseDisplayId(team, ci);
            sObjectMgr->GetCreatureModelRandomGender(&displayId);

            vehicleId = ci->VehicleId;

            //some spell has one aura of mount and one of vehicle
            for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
                if (GetSpellInfo()->GetEffect(i, m_diffMode).Effect == SPELL_EFFECT_SUMMON
                    && GetSpellInfo()->GetEffect(i, m_diffMode).MiscValue == GetMiscValue())
                    displayId = 0;
        }

        target->Mount(displayId, vehicleId, GetMiscValue());
    }
    else
    {
        target->Dismount();
        //some mounts like Headless Horseman's Mount or broom stick are skill based spell
        // need to remove ALL arura related to mounts, this will stop client crash with broom stick
        // and never endless flying after using Headless Horseman's Mount
        if (mode & AURA_EFFECT_HANDLE_REAL)
            target->RemoveAurasByType(SPELL_AURA_MOUNTED);
    }
    target->UpdateSpeed(MOVE_RUN, true);
    target->UpdateMount();
}

void AuraEffect::HandleAuraAllowFlight(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (!apply)
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()) || target->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED))
            return;
    }

    //! Not entirely sure if this should be sent for creatures as well, but I don't think so.
    target->SetCanFly(apply);
    if (!apply)
    {
        target->RemoveUnitMovementFlag(MOVEMENTFLAG_MASK_MOVING_FLY);
        target->m_movementInfo.SetFallTime(0);
    }else
        target->RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING);

    Player* player = target->ToPlayer();
    if (!player)
        player = target->m_movedPlayer;

    if (player)
    {
        player->RemoveUnitMovementFlag(MOVEMENTFLAG_MASK_MOVING_FLY);
        target->m_anti_FlightTime = 1000;
        player->SendMovementCanFlyChange();
    }

    //! We still need to initiate a server-side MoveFall here,
    //! which requires MSG_MOVE_FALL_LAND on landing.
}

void AuraEffect::HandleAuraWaterWalk(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (!apply)
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
    }

    if (apply)
        target->AddUnitMovementFlag(MOVEMENTFLAG_WATERWALKING);
    else
        target->RemoveUnitMovementFlag(MOVEMENTFLAG_WATERWALKING);

    target->SendMovementWaterWalking();
}

void AuraEffect::HandleAuraFeatherFall(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (!apply)
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
    }

    if (apply)
        target->AddUnitMovementFlag(MOVEMENTFLAG_FALLING_SLOW);
    else
        target->RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING_SLOW);

    target->SendMovementFeatherFall();

    // start fall from current height
    if (!apply && target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->SetFallInformation(0, target->GetPositionZ());
}

void AuraEffect::HandleAuraGlide(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (!apply)
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
    }

    if (apply)
        target->AddExtraUnitMovementFlag(MOVEMENTFLAG2_0x1000);
    else
        target->RemoveExtraUnitMovementFlag(MOVEMENTFLAG2_0x1000);

    target->SendMoveflag2_0x1000_Update(apply);
}

void AuraEffect::HandleAuraHover(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (!apply)
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
    }

    target->SetHover(apply);    //! Sets movementflags
    target->SendMovementHover();
}

void AuraEffect::HandleWaterBreathing(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    // update timers in client
    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->UpdateMirrorTimers();
}

void AuraEffect::HandleForceMoveForward(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVEMENT);
    else
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FORCE_MOVEMENT);
    }
}

/****************************/
/***        THREAT        ***/
/****************************/

void AuraEffect::HandleModThreat(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();
    for (int8 i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if (GetMiscValue() & (1 << i))
            ApplyPercentModFloatVar(target->m_threatModifier[i], float(GetAmount()), apply);
}

void AuraEffect::HandleAuraModTotalThreat(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (!target->isAlive() || target->GetTypeId() != TYPEID_PLAYER)
        return;

    Unit* caster = GetCaster();
    if (caster && caster->isAlive())
        target->getHostileRefManager().addTempThreat((float)GetAmount(), apply);
}

void AuraEffect::HandleModTaunt(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (!target->isAlive() || !target->CanHaveThreatList())
        return;

    Unit* caster = GetCaster();
    if (!caster || !caster->isAlive())
        return;

    if (apply)
        target->TauntApply(caster);
    else
    {
        // When taunt aura fades out, mob will switch to previous target if current has less than 1.1 * secondthreat
        target->TauntFadeOut(caster);
    }
}

/*****************************/
/***        CONTROL        ***/
/*****************************/

void AuraEffect::HandleModConfuse(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if(apply)
        target->SetTimeForSpline(GetBase()->GetDuration());
    else
        target->SetTimeForSpline(0);

    target->SetControlled(apply, UNIT_STATE_CONFUSED);
}

void AuraEffect::HandleModFear(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if(apply)
        target->SetTimeForSpline(GetBase()->GetDuration());
    else
        target->SetTimeForSpline(0);

    target->SetControlled(apply, UNIT_STATE_FLEEING);
}

void AuraEffect::HandleAuraModStun(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    target->SetControlled(apply, UNIT_STATE_STUNNED);
}

void AuraEffect::HandleAuraModRoot(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();
    Unit* caster = GetCaster();

    // Earthgrab totem - Immunity
    if (apply && target->HasAura(116946))
        return;

    // Glyph of Intimidating Shout
    if (m_spellInfo->Id == 5246)
    {
        if (apply && caster && !caster->HasAura(63327))
            return;
    }

    target->SetControlled(apply, UNIT_STATE_ROOT);
}

void AuraEffect::HandlePreventFleeing(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->HasAuraType(SPELL_AURA_MOD_FEAR))
        target->SetControlled(!(apply), UNIT_STATE_FLEEING);
}

/***************************/
/***        CHARM        ***/
/***************************/

void AuraEffect::HandleModPossess(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    Unit* caster = GetCaster();

    // no support for posession AI yet
    if (caster && caster->GetTypeId() == TYPEID_UNIT)
    {
        HandleModCharm(aurApp, mode, apply);
        return;
    }

    if (apply)
        target->SetCharmedBy(caster, CHARM_TYPE_POSSESS, aurApp);
    else
        target->RemoveCharmedBy(caster);
}

// only one spell has this aura
void AuraEffect::HandleModPossessPet(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* caster = GetCaster();
    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    //seems it may happen that when removing it is no longer owner's pet
    //if (caster->ToPlayer()->GetPet() != target)
    //    return;

    Unit* target = aurApp->GetTarget();
    if (target->GetTypeId() != TYPEID_UNIT || !target->ToCreature()->isPet())
        return;

    Pet* pet = target->ToPet();

    if (apply)
    {
        if (caster->ToPlayer()->GetPet() != pet)
            return;

        pet->SetCharmedBy(caster, CHARM_TYPE_POSSESS, aurApp);
    }
    else
    {
        pet->RemoveCharmedBy(caster);

        if (!pet->IsWithinDistInMap(caster, pet->GetMap()->GetVisibilityRange()))
            pet->Remove(PET_SLOT_OTHER_PET, true);
        else
        {
            // Reinitialize the pet bar and make the pet come back to the owner
            caster->ToPlayer()->PetSpellInitialize();
            if (!pet->getVictim())
            {
                pet->GetMotionMaster()->MoveFollow(caster, PET_FOLLOW_DIST, pet->GetFollowAngle());
            }
        }
    }
}

void AuraEffect::HandleModCharm(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    Unit* caster = GetCaster();

    if (apply)
        target->SetCharmedBy(caster, CHARM_TYPE_CHARM, aurApp);
    else
        target->RemoveCharmedBy(caster);
}

void AuraEffect::HandleCharmConvert(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    Unit* caster = GetCaster();

    if (apply)
        target->SetCharmedBy(caster, CHARM_TYPE_CONVERT, aurApp);
    else
        target->RemoveCharmedBy(caster);
}

/**
 * Such auras are applied from a caster(=player) to a vehicle.
 * This has been verified using spell #49256
 */
void AuraEffect::HandleAuraControlVehicle(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (!target->IsVehicle())
        return;

    Unit* caster = GetCaster();

    if (!caster || caster == target)
        return;

    if (apply)
    {
        // Currently spells that have base points  0 and DieSides 0 = "0/0" exception are pushed to -1,
        // however the idea of 0/0 is to ingore flag VEHICLE_SEAT_FLAG_CAN_ENTER_OR_EXIT and -1 checks for it,
        // so this break such spells or most of them.
        // Current formula about m_amount: effect base points + dieside - 1
        // TO DO: Reasearch more about 0/0 and fix it.
        caster->_EnterVehicle(target->GetVehicleKit(), m_amount - 1, aurApp);
    }
    else
    {
        // Remove pending passengers before exiting vehicle - might cause an Uninstall
        target->GetVehicleKit()->RemovePendingEventsForPassenger(caster);

        if (GetId() == 53111) // Devour Humanoid
        {
            target->Kill(caster);
            if (caster->GetTypeId() == TYPEID_UNIT)
                caster->ToCreature()->RemoveCorpse();
        }

        if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT))
            caster->_ExitVehicle();
        else
            target->GetVehicleKit()->RemovePassenger(caster);  // Only remove passenger from vehicle without launching exit movement or despawning the vehicle

        // some SPELL_AURA_CONTROL_VEHICLE auras have a dummy effect on the player - remove them
        caster->RemoveAurasDueToSpell(GetId());
    }
}

/*********************************************************/
/***                  MODIFY SPEED                     ***/
/*********************************************************/
void AuraEffect::HandleAuraModIncreaseSpeed(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (GetAuraType() == SPELL_AURA_INCREASE_MIN_SWIM_SPEED)
    {
        target->UpdateSpeed(MOVE_SWIM, true);
        return;
    }

    target->UpdateSpeed(MOVE_RUN, true);

    if (GetAuraType() == SPELL_AURA_MOD_MINIMUM_SPEED)
    {
        target->UpdateSpeed(MOVE_RUN_BACK, true);
        target->UpdateSpeed(MOVE_FLIGHT, true);
    }
}

void AuraEffect::HandleAuraModIncreaseMountedSpeed(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    HandleAuraModIncreaseSpeed(aurApp, mode, apply);
}

void AuraEffect::HandleAuraModIncreaseFlightSpeed(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    //! Update ability to fly
    if (GetAuraType() == SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED)
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK && (apply || (!target->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) && !target->HasAuraType(SPELL_AURA_FLY))))
        {
            target->SetCanFly(apply);
            if (!apply)
            {
                target->RemoveUnitMovementFlag(MOVEMENTFLAG_FLYING);
                target->m_movementInfo.SetFallTime(0);
            }else
                target->RemoveUnitMovementFlag(MOVEMENTFLAG_FALLING);

            Player* player = target->ToPlayer();
            if (!player)
                player = target->m_movedPlayer;

            if (player)
                player->SendMovementCanFlyChange();

            //! We still need to initiate a server-side MoveFall here,
            //! which requires MSG_MOVE_FALL_LAND on landing.
        }

        //! Someone should clean up these hacks and remove it from this function. It doesn't even belong here.
        if (mode & AURA_EFFECT_HANDLE_REAL)
        {
            //Players on flying mounts must be immune to polymorph
            if (target->GetTypeId() == TYPEID_PLAYER)
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, apply);

            // Dragonmaw Illusion (overwrite mount model, mounted aura already applied)
            if (apply && target->HasAuraEffect(42016, 0) && target->GetMountID())
                target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, 16314);
        }
    }

    if (mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK)
        target->UpdateSpeed(MOVE_FLIGHT, true);
}

void AuraEffect::HandleAuraModIncreaseSwimSpeed(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    target->UpdateSpeed(MOVE_SWIM, true);
}

void AuraEffect::HandleAuraModDecreaseSpeed(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    target->UpdateSpeed(MOVE_RUN, true);
    target->UpdateSpeed(MOVE_SWIM, true);
    target->UpdateSpeed(MOVE_FLIGHT, true);
    target->UpdateSpeed(MOVE_RUN_BACK, true);
    target->UpdateSpeed(MOVE_SWIM_BACK, true);
    target->UpdateSpeed(MOVE_FLIGHT_BACK, true);
}

void AuraEffect::HandleAuraModUseNormalSpeed(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    target->UpdateSpeed(MOVE_RUN,  true);
    target->UpdateSpeed(MOVE_SWIM, true);
    target->UpdateSpeed(MOVE_FLIGHT,  true);
}

/*********************************************************/
/***                     IMMUNITY                      ***/
/*********************************************************/

void AuraEffect::HandleModStateImmunityMask(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();
    std::list <AuraType> aura_immunity_list;
    uint32 mechanic_immunity_list = 0;
    int32 miscVal = GetMiscValue();

    switch (miscVal)
    {
        case 96:
        case 1615:
        {
            if (GetAmount())
            {
                mechanic_immunity_list = (1 << MECHANIC_SNARE) | (1 << MECHANIC_ROOT)
                    | (1 << MECHANIC_FEAR) | (1 << MECHANIC_STUN)
                    | (1 << MECHANIC_SLEEP) | (1 << MECHANIC_CHARM)
                    | (1 << MECHANIC_SAPPED) | (1 << MECHANIC_HORROR)
                    | (1 << MECHANIC_POLYMORPH) | (1 << MECHANIC_DISORIENTED)
                    | (1 << MECHANIC_FREEZE) | (1 << MECHANIC_TURN);

                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SNARE, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_ROOT, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FEAR, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SLEEP, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_CHARM, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SAPPED, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_HORROR, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FREEZE, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_TURN, apply);
                aura_immunity_list.push_back(SPELL_AURA_MOD_STUN);
                aura_immunity_list.push_back(SPELL_AURA_MOD_DECREASE_SPEED);
                aura_immunity_list.push_back(SPELL_AURA_MOD_ROOT);
                aura_immunity_list.push_back(SPELL_AURA_MOD_CONFUSE);
                aura_immunity_list.push_back(SPELL_AURA_MOD_FEAR);
            }
            break;
        }
        case 679:
        case 1921:
        {
            if (GetId() == 57742 || GetId() == 115018)
            {
                mechanic_immunity_list = (1 << MECHANIC_SNARE) | (1 << MECHANIC_ROOT)
                    | (1 << MECHANIC_FEAR) | (1 << MECHANIC_STUN)
                    | (1 << MECHANIC_SLEEP) | (1 << MECHANIC_CHARM)
                    | (1 << MECHANIC_SAPPED) | (1 << MECHANIC_HORROR)
                    | (1 << MECHANIC_POLYMORPH) | (1 << MECHANIC_DISORIENTED)
                    | (1 << MECHANIC_FREEZE) | (1 << MECHANIC_TURN);

                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SNARE, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_ROOT, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FEAR, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SLEEP, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_CHARM, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SAPPED, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_HORROR, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FREEZE, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_TURN, apply);
                aura_immunity_list.push_back(SPELL_AURA_MOD_STUN);
                aura_immunity_list.push_back(SPELL_AURA_MOD_DECREASE_SPEED);
                aura_immunity_list.push_back(SPELL_AURA_MOD_ROOT);
                aura_immunity_list.push_back(SPELL_AURA_MOD_CONFUSE);
                aura_immunity_list.push_back(SPELL_AURA_MOD_FEAR);
            }
            break;
        }
        case 1557:
        {
            if (GetId() == 64187)
            {
                mechanic_immunity_list = (1 << MECHANIC_STUN);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
                aura_immunity_list.push_back(SPELL_AURA_MOD_STUN);
            }
            else
            {
                mechanic_immunity_list = (1 << MECHANIC_SNARE) | (1 << MECHANIC_ROOT)
                    | (1 << MECHANIC_FEAR) | (1 << MECHANIC_STUN)
                    | (1 << MECHANIC_SLEEP) | (1 << MECHANIC_CHARM)
                    | (1 << MECHANIC_SAPPED) | (1 << MECHANIC_HORROR)
                    | (1 << MECHANIC_POLYMORPH) | (1 << MECHANIC_DISORIENTED)
                    | (1 << MECHANIC_FREEZE) | (1 << MECHANIC_TURN);

                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SNARE, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_ROOT, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FEAR, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SLEEP, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_CHARM, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SAPPED, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_HORROR, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FREEZE, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_TURN, apply);
                aura_immunity_list.push_back(SPELL_AURA_MOD_STUN);
                aura_immunity_list.push_back(SPELL_AURA_MOD_DECREASE_SPEED);
                aura_immunity_list.push_back(SPELL_AURA_MOD_ROOT);
                aura_immunity_list.push_back(SPELL_AURA_MOD_CONFUSE);
                aura_immunity_list.push_back(SPELL_AURA_MOD_FEAR);
            }
            break;
        }
        case 1614:
        case 1694:
        {
            target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, apply);
            aura_immunity_list.push_back(SPELL_AURA_MOD_TAUNT);
            break;
        }
        case 1630:
        {
            if (!GetAmount())
            {
                target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, SPELL_EFFECT_ATTACK_ME, apply);
                aura_immunity_list.push_back(SPELL_AURA_MOD_TAUNT);
            }
            else
            {
                mechanic_immunity_list = (1 << MECHANIC_SNARE) | (1 << MECHANIC_ROOT)
                    | (1 << MECHANIC_FEAR) | (1 << MECHANIC_STUN)
                    | (1 << MECHANIC_SLEEP) | (1 << MECHANIC_CHARM)
                    | (1 << MECHANIC_SAPPED) | (1 << MECHANIC_HORROR)
                    | (1 << MECHANIC_POLYMORPH) | (1 << MECHANIC_DISORIENTED)
                    | (1 << MECHANIC_FREEZE) | (1 << MECHANIC_TURN);

                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SNARE, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_ROOT, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FEAR, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SLEEP, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_CHARM, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SAPPED, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_HORROR, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FREEZE, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_TURN, apply);
                aura_immunity_list.push_back(SPELL_AURA_MOD_STUN);
                aura_immunity_list.push_back(SPELL_AURA_MOD_DECREASE_SPEED);
                aura_immunity_list.push_back(SPELL_AURA_MOD_ROOT);
                aura_immunity_list.push_back(SPELL_AURA_MOD_CONFUSE);
                aura_immunity_list.push_back(SPELL_AURA_MOD_FEAR);
            }
            break;
        }
        case 477:
        case 1733:
        {
            if (!GetAmount())
            {
                mechanic_immunity_list = (1 << MECHANIC_SNARE) | (1 << MECHANIC_ROOT)
                    | (1 << MECHANIC_FEAR) | (1 << MECHANIC_STUN)
                    | (1 << MECHANIC_SLEEP) | (1 << MECHANIC_CHARM)
                    | (1 << MECHANIC_SAPPED) | (1 << MECHANIC_HORROR)
                    | (1 << MECHANIC_POLYMORPH) | (1 << MECHANIC_DISORIENTED)
                    | (1 << MECHANIC_FREEZE) | (1 << MECHANIC_TURN)
                    | (1 << MECHANIC_DISARM);

                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SNARE, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_ROOT, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FEAR, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SLEEP, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_CHARM, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SAPPED, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_HORROR, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FREEZE, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_TURN, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_DISARM, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, apply);
                aura_immunity_list.push_back(SPELL_AURA_MOD_STUN);
                aura_immunity_list.push_back(SPELL_AURA_MOD_DECREASE_SPEED);
                aura_immunity_list.push_back(SPELL_AURA_MOD_ROOT);
                aura_immunity_list.push_back(SPELL_AURA_MOD_CONFUSE);
                aura_immunity_list.push_back(SPELL_AURA_MOD_FEAR);
                aura_immunity_list.push_back(SPELL_AURA_MOD_DISARM);
            }
            break;
        }
        case 878:
        {
            if (GetAmount() == 1)
            {
                mechanic_immunity_list = (1 << MECHANIC_SNARE) | (1 << MECHANIC_STUN)
                    | (1 << MECHANIC_DISORIENTED) | (1 << MECHANIC_FREEZE);

                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SNARE, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FREEZE, apply);
                aura_immunity_list.push_back(SPELL_AURA_MOD_STUN);
                aura_immunity_list.push_back(SPELL_AURA_MOD_DECREASE_SPEED);
            }
            break;
        }
        case 1887:
        {
            // Frost Pillar
            if (GetId() == 51271)
            {
                target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, apply);
                target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK_DEST, apply);
                miscVal = 0;

                // Glyph of Frost Pillar
                if (target->HasAura(58635))
                {
                    mechanic_immunity_list = (1 << MECHANIC_FEAR) | (1 << MECHANIC_STUN)
                    | (1 << MECHANIC_SLEEP) | (1 << MECHANIC_CHARM)
                    | (1 << MECHANIC_SAPPED) | (1 << MECHANIC_HORROR)
                    | (1 << MECHANIC_POLYMORPH) | (1 << MECHANIC_DISORIENTED)
                    | (1 << MECHANIC_FREEZE) | (1 << MECHANIC_TURN);

                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FEAR, apply);
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SLEEP, apply);
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_CHARM, apply);
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SAPPED, apply);
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_HORROR, apply);
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_POLYMORPH, apply);
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_DISORIENTED, apply);
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_FREEZE, apply);
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_TURN, apply);
                    aura_immunity_list.push_back(SPELL_AURA_MOD_STUN);
                    aura_immunity_list.push_back(SPELL_AURA_TRANSFORM);
                    aura_immunity_list.push_back(SPELL_AURA_MOD_CONFUSE);
                    aura_immunity_list.push_back(SPELL_AURA_MOD_FEAR);

                    // Apply Root !
                    if (apply)
                        target->CastSpell(target, 90259, true);
                    else
                        target->RemoveAura(90259);
                }
            }

            break;
        }
        default:
            break;
    }

    if (aura_immunity_list.empty())
    {
            if (miscVal & (1<<10))
                aura_immunity_list.push_back(SPELL_AURA_MOD_STUN);
            if (miscVal & (1<<1))
                aura_immunity_list.push_back(SPELL_AURA_TRANSFORM);

            // These flag can be recognized wrong:
            if (miscVal & (1<<6))
                aura_immunity_list.push_back(SPELL_AURA_MOD_DECREASE_SPEED);
            if (miscVal & (1<<0))
                aura_immunity_list.push_back(SPELL_AURA_MOD_ROOT);
            if (miscVal & (1<<2))
                aura_immunity_list.push_back(SPELL_AURA_MOD_CONFUSE);
            if (miscVal & (1<<9))
                aura_immunity_list.push_back(SPELL_AURA_MOD_FEAR);
            if (miscVal & (1<<7))
                aura_immunity_list.push_back(SPELL_AURA_MOD_DISARM);
    }

    // apply immunities
    for (std::list <AuraType>::iterator iter = aura_immunity_list.begin(); iter != aura_immunity_list.end(); ++iter)
        target->ApplySpellImmune(GetId(), IMMUNITY_STATE, *iter, apply);

    if (apply && GetSpellInfo()->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY)
    {
        target->RemoveAurasWithMechanic(mechanic_immunity_list, AURA_REMOVE_BY_DEFAULT, GetId());
        for (std::list <AuraType>::iterator iter = aura_immunity_list.begin(); iter != aura_immunity_list.end(); ++iter)
            target->RemoveAurasByType(*iter);
    }
}

void AuraEffect::HandleModMechanicImmunity(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();
    uint32 mechanic;

    switch (GetId())
    {
        case 42292: // PvP trinket
        case 59752: // Every Man for Himself
        case 70029: // The Beast Within
            mechanic = IMMUNE_TO_MOVEMENT_IMPAIRMENT_AND_LOSS_CONTROL_MASK;
            // Actually we should apply immunities here, too, but the aura has only 100 ms duration, so there is practically no point
            break;
        case 54508: // Demonic Empowerment
            mechanic = (1 << MECHANIC_SNARE) | (1 << MECHANIC_ROOT);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_SNARE, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_ROOT, apply);
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, MECHANIC_STUN, apply);
            break;
        default:
            if (GetMiscValue() < 1)
                return;
            mechanic = 1 << GetMiscValue();
            target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, GetMiscValue(), apply);
            break;
    }

    if (apply && GetSpellInfo()->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY)
        target->RemoveAurasWithMechanic(mechanic, AURA_REMOVE_BY_DEFAULT, GetId());
}

void AuraEffect::HandleAuraModEffectImmunity(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

   target->ApplySpellImmune(GetId(), IMMUNITY_EFFECT, GetMiscValue(), apply);
}

void AuraEffect::HandleAuraModStateImmunity(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    target->ApplySpellImmune(GetId(), IMMUNITY_STATE, GetMiscValue(), apply);

    if (apply && GetSpellInfo()->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY)
        target->RemoveAurasByType(AuraType(GetMiscValue()), 0, GetBase());
}

void AuraEffect::HandleAuraModSchoolImmunity(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    target->ApplySpellImmune(GetId(), IMMUNITY_SCHOOL, GetMiscValue(), (apply));

    if (GetSpellInfo()->Mechanic == MECHANIC_BANISH)
    {
        if (apply)
            target->AddUnitState(UNIT_STATE_ISOLATED);
        else
        {
            bool banishFound = false;
            Unit::AuraEffectList const& banishAuras = target->GetAuraEffectsByType(GetAuraType());
            for (Unit::AuraEffectList::const_iterator i = banishAuras.begin(); i != banishAuras.end(); ++i)
                if ((*i)->GetSpellInfo()->Mechanic == MECHANIC_BANISH)
                {
                    banishFound = true;
                    break;
                }
            if (!banishFound)
                target->ClearUnitState(UNIT_STATE_ISOLATED);
        }
    }

    uint32 count = 0;
    if (apply && GetMiscValue() == SPELL_SCHOOL_MASK_NORMAL)
        count += target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

    // remove all flag auras (they are positive, but they must be removed when you are immune)
    if (GetSpellInfo()->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY
        && GetSpellInfo()->AttributesEx2 & SPELL_ATTR2_DAMAGE_REDUCED_SHIELD)
        count += target->RemoveAurasWithInterruptFlags(AURA_INTERRUPT_FLAG_IMMUNE_OR_LOST_SELECTION);

    // TODO: optimalize this cycle - use RemoveAurasWithInterruptFlags call or something else
    if ((apply)
        && GetSpellInfo()->AttributesEx & SPELL_ATTR1_DISPEL_AURAS_ON_IMMUNITY
        && GetSpellInfo()->IsPositive())                       //Only positive immunity removes auras
    {
        uint32 school_mask = GetMiscValue();
        Unit::AuraApplicationMap& Auras = target->GetAppliedAuras();
        for (Unit::AuraApplicationMap::iterator iter = Auras.begin(); iter != Auras.end();)
        {
            SpellInfo const* spell = iter->second->GetBase()->GetSpellInfo();
            if ((spell->GetSchoolMask() & school_mask)//Check for school mask
                && GetSpellInfo()->CanDispelAura(spell)
                && !iter->second->IsPositive()          //Don't remove positive spells
                && spell->Id != GetId())               //Don't remove self
            {
                target->RemoveAura(iter);
                count++;
            }
            else
                ++iter;
        }
    }

    if(Spell* modSpell = target->FindCurrentSpellBySpellId(GetId()))
        modSpell->m_count_dispeling += count;
}

void AuraEffect::HandleAuraModDmgImmunity(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    target->ApplySpellImmune(GetId(), IMMUNITY_DAMAGE, GetMiscValue(), apply);
}

void AuraEffect::HandleAuraModDispelImmunity(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    target->ApplySpellDispelImmunity(m_spellInfo, DispelType(GetMiscValue()), (apply));
}

/*********************************************************/
/***                  MODIFY STATS                     ***/
/*********************************************************/

/********************************/
/***        RESISTANCE        ***/
/********************************/

void AuraEffect::HandleAuraModResistanceExclusive(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; x++)
    {
        if (GetMiscValue() & int32(1<<x))
        {
            int32 amount = target->GetMaxPositiveAuraModifierByMiscMask(SPELL_AURA_MOD_RESISTANCE_EXCLUSIVE, 1<<x, this);
            if (amount < GetAmount())
            {
                float value = float(GetAmount() - amount);
                target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_VALUE, value, apply);
                if (target->GetTypeId() == TYPEID_PLAYER)
                    target->ApplyResistanceBuffModsMod(SpellSchools(x), aurApp->IsPositive(), value, apply);
            }
        }
    }
}

void AuraEffect::HandleAuraModResistance(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; x++)
    {
        if (GetMiscValue() & int32(1<<x))
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), TOTAL_VALUE, float(GetAmount()), apply);
            if (target->GetTypeId() == TYPEID_PLAYER || target->ToCreature()->isPet())
                target->ApplyResistanceBuffModsMod(SpellSchools(x), GetAmount() > 0, (float)GetAmount(), apply);
        }
    }
}

void AuraEffect::HandleAuraModBaseResistancePCT(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    // only players have base stats
    if (target->GetTypeId() != TYPEID_PLAYER)
    {
        //pets only have base armor
        if (target->ToCreature()->isPet() && (GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL))
            target->HandleStatModifier(UNIT_MOD_ARMOR, BASE_PCT, float(GetAmount()), apply);
    }
    else
    {
        for (int8 x = SPELL_SCHOOL_NORMAL; x < MAX_SPELL_SCHOOL; x++)
        {
            if (GetMiscValue() & int32(1<<x))
            {
                switch (GetMiscValue())
                {
                    case 1:
                        target->UpdateArmor();
                        break; 
                    default:
                        target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + x), BASE_PCT, float(GetAmount()), apply);
                        break;
                }
            }
        }
    }
}

void AuraEffect::HandleModResistancePercent(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    for (int8 i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
    {
        if (GetMiscValue() & int32(1<<i))
        {
            switch (GetMiscValue())
            {
                case 1:
                   target->UpdateArmor();
                   break; 
                default:
                {
                    target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_PCT, float(GetAmount()), apply);
                    if (target->GetTypeId() == TYPEID_PLAYER || target->ToCreature()->isPet())
                    {
                        target->ApplyResistanceBuffModsPercentMod(SpellSchools(i), true, (float)GetAmount(), apply);
                        target->ApplyResistanceBuffModsPercentMod(SpellSchools(i), false, (float)GetAmount(), apply);
                    }
                    break;
                }
            }
        }
    }
}

void AuraEffect::HandleModBaseResistance(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    // only players have base stats
    if (target->GetTypeId() != TYPEID_PLAYER)
    {
        //only pets have base stats
        if (target->ToCreature()->isPet() && (GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL))
            target->HandleStatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, float(GetAmount()), apply);
    }
    else
    {
        for (int i = SPELL_SCHOOL_NORMAL; i < MAX_SPELL_SCHOOL; i++)
            if (GetMiscValue() & (1<<i))
                target->HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + i), TOTAL_VALUE, float(GetAmount()), apply);
    }
}

void AuraEffect::HandleModTargetResistance(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    // applied to damage as HandleNoImmediateEffect in Unit::CalcAbsorbResist and Unit::CalcArmorReducedDamage

    // show armor penetration
    if (target->GetTypeId() == TYPEID_PLAYER && (GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL))
        target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE, GetAmount(), apply);

    // show as spell penetration only full spell penetration bonuses (all resistances except armor and holy
    if (target->GetTypeId() == TYPEID_PLAYER && (GetMiscValue() & SPELL_SCHOOL_MASK_SPELL) == SPELL_SCHOOL_MASK_SPELL)
        target->ApplyModInt32Value(PLAYER_FIELD_MOD_TARGET_RESISTANCE, GetAmount(), apply);
}

/********************************/
/***           STAT           ***/
/********************************/

void AuraEffect::HandleAuraModStat(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (GetMiscValue() < -2 || GetMiscValue() > 4)
    {
        sLog->outError(LOG_FILTER_SPELLS_AURAS, "WARNING: Spell %u effect %u has an unsupported misc value (%i) for SPELL_AURA_MOD_STAT ", GetId(), GetEffIndex(), GetMiscValue());
        return;
    }

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        // -1 or -2 is all stats (misc < -2 checked in function beginning)
        if (GetMiscValue() < 0 || GetMiscValue() == i)
        {
            //target->ApplyStatMod(Stats(i), m_amount, apply);
            target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_VALUE, float(GetAmount()), apply);
            if (target->GetTypeId() == TYPEID_PLAYER || target->ToCreature()->isPet())
                target->ApplyStatBuffMod(Stats(i), (float)GetAmount(), apply);
        }
    }
}

void AuraEffect::HandleModPercentStat(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (GetMiscValue() < -1 || GetMiscValue() > 4)
    {
        sLog->outError(LOG_FILTER_SPELLS_AURAS, "WARNING: Misc Value for SPELL_AURA_MOD_PERCENT_STAT not valid");
        return;
    }

    // only players have base stats
    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; ++i)
    {
        if (GetMiscValue() == i || GetMiscValue() == -1)
            target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), BASE_PCT, float(m_amount), apply);
    }
}

void AuraEffect::HandleModSpellDamagePercentFromStat(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Magic damage modifiers implemented in Unit::SpellDamageBonus
    // This information for client side use only
    // Recalculate bonus
    target->ToPlayer()->UpdateSpellDamageAndHealingBonus();
}

void AuraEffect::HandleModSpellHealingPercentFromStat(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    target->ToPlayer()->UpdateSpellDamageAndHealingBonus();
}

void AuraEffect::HandleAuraModSpellPowerPercent(AuraApplication const * aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit *target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    target->ToPlayer()->UpdateSpellDamageAndHealingBonus();
}

void AuraEffect::HandleModSpellDamagePercentFromAttackPower(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Magic damage modifiers implemented in Unit::SpellDamageBonus
    // This information for client side use only
    // Recalculate bonus
    target->ToPlayer()->UpdateSpellDamageAndHealingBonus();
}

void AuraEffect::HandleModSpellHealingPercentFromAttackPower(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Recalculate bonus
    target->ToPlayer()->UpdateSpellDamageAndHealingBonus();
}

void AuraEffect::HandleModHealingDone(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;
    // implemented in Unit::SpellHealingBonus
    // this information is for client side only
    target->ToPlayer()->UpdateSpellDamageAndHealingBonus();
}

void AuraEffect::HandleModTotalPercentStat(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    // save current health state
    float healthPct = target->GetHealthPct();

    for (int32 i = STAT_STRENGTH; i < MAX_STATS; i++)
    {
        if (GetMiscValueB() & 1 << i || !GetMiscValueB()) // 0 is also used for all stats
        {
            target->HandleStatModifier(UnitMods(UNIT_MOD_STAT_START + i), TOTAL_PCT, float(GetAmount()), apply);
            if (target->GetTypeId() == TYPEID_PLAYER || target->ToCreature()->isPet())
                target->ApplyStatPercentBuffMod(Stats(i), float(GetAmount()), apply);
        }
    }

    // recalculate current HP/MP after applying aura modifications (only for spells with SPELL_ATTR0_UNK4 0x00000010 flag)
    // this check is total bullshit i think
    if (target->isAlive())
    {
        if (GetMiscValueB() & 1 << STAT_STAMINA && (m_spellInfo->Attributes & SPELL_ATTR0_ABILITY))
            target->SetHealth(std::max<uint32>(uint32(healthPct * target->GetMaxHealth() * 0.01f), 1));
    }
}

void AuraEffect::HandleAuraModResistenceOfStatPercent(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    if (GetMiscValue() != SPELL_SCHOOL_MASK_NORMAL)
    {
        // support required adding replace UpdateArmor by loop by UpdateResistence at intellect update
        // and include in UpdateResistence same code as in UpdateArmor for aura mod apply.
        sLog->outError(LOG_FILTER_SPELLS_AURAS, "Aura SPELL_AURA_MOD_RESISTANCE_OF_STAT_PERCENT(182) does not work for non-armor type resistances!");
        return;
    }

    // Recalculate Armor
    target->UpdateArmor();
}

void AuraEffect::HandleAuraModExpertise(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->ToPlayer()->UpdateExpertise();
}

/********************************/
/***      HEAL & ENERGIZE     ***/
/********************************/
void AuraEffect::HandleModPowerRegen(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;
    
    switch (GetMiscValue())
    {
        case POWER_MANA:   target->ToPlayer()->UpdateManaRegen();     break;
        case POWER_RUNES:  target->ToPlayer()->UpdateAllRunesRegen(); break;
        case POWER_ENERGY: target->ToPlayer()->UpdateEnergyRegen();   break;
        default:
            break;
    }
    // Update manaregen value
    // other powers are not immediate effects - implemented in Player::Regenerate, Creature::Regenerate
}

void AuraEffect::HandleModPowerRegenPCT(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    HandleModPowerRegen(aurApp, mode, apply);
}

void AuraEffect::HandleModManaRegen(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    //Note: an increase in regen does NOT cause threat.
    target->ToPlayer()->UpdateManaRegen();
    target->ToPlayer()->UpdateAllRunesRegen();
}

void AuraEffect::HandleAuraModIncreaseHealth(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();
    int32 getHealth = target->GetHealth();
    int32 getMaxHealth = target->GetMaxHealth();
    int32 amount = GetAmount();

    if (apply)
    {
        target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(amount), apply);
        target->ModifyHealth(amount);
    }
    else
    {
        float pct = 0;
        if (!(m_spellInfo->Attributes & SPELL_ATTR0_DONT_AFFECT_SHEATH_STATE))
        {
            if (getHealth > amount)
                target->ModifyHealth(-amount);
            else
                target->SetHealth(1);
        }
        else if (m_spellInfo->AttributesEx3 & SPELL_ATTR3_NO_DONE_BONUS)
            pct = float(getMaxHealth) / float(getHealth);

        target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(amount), apply);

        if (pct)
        {
            target->SetHealth(target->GetMaxHealth() / pct);
        }
    }
}

void AuraEffect::HandleAuraModIncreaseMaxHealth(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    uint32 oldhealth = target->GetHealth();
    double healthPercentage = (double)oldhealth / (double)target->GetMaxHealth();

    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_VALUE, float(GetAmount()), apply);

    // refresh percentage
    if (oldhealth > 0)
    {
        uint32 newhealth = uint32(ceil((double)target->GetMaxHealth() * healthPercentage));
        if (newhealth == 0)
            newhealth = 1;

        target->SetHealth(newhealth);
    }
}

void AuraEffect::HandleAuraModIncreaseEnergy(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    Powers powerType = Powers(GetMiscValue());
    // do not check power type, we can always modify the maximum
    // as the client will not see any difference
    // also, placing conditions that may change during the aura duration
    // inside effect handlers is not a good idea
    //if (int32(powerType) != GetMiscValue())
    //    return;

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);

    target->HandleStatModifier(unitMod, TOTAL_VALUE, float(GetAmount()), apply);
}

void AuraEffect::HandleAuraModIncreaseEnergyPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    Powers powerType = Powers(GetMiscValue());
    // do not check power type, we can always modify the maximum
    // as the client will not see any difference
    // also, placing conditions that may change during the aura duration
    // inside effect handlers is not a good idea
    //if (int32(powerType) != GetMiscValue())
    //    return;

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);
    float amount = float(GetAmount());

    if (apply)
    {
        target->HandleStatModifier(unitMod, TOTAL_PCT, amount, apply);
        target->ModifyPowerPct(powerType, amount, apply);
    }
    else
    {
        target->ModifyPowerPct(powerType, amount, apply);
        target->HandleStatModifier(unitMod, TOTAL_PCT, amount, apply);
    }
}

void AuraEffect::HandleAuraModMaxPower(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    Powers powerType = Powers(GetMiscValue());

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);
    float amount = float(GetAmount());

    if (apply)
    {
        target->HandleStatModifier(unitMod, TOTAL_VALUE, amount, apply);
    }
    else
    {
        target->HandleStatModifier(unitMod, TOTAL_VALUE, amount, apply);
    }
}

void AuraEffect::HandleAuraModAddEnergyPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    Powers powerType = Powers(GetMiscValue());

    UnitMods unitMod = UnitMods(UNIT_MOD_POWER_START + powerType);
    float amount = float(GetAmount());

    if (apply)
    {
        target->HandleStatModifier(unitMod, TOTAL_PCT, amount, apply);
    }
    else
    {
        target->HandleStatModifier(unitMod, TOTAL_PCT, amount, apply);
    }
}

void AuraEffect::HandleAuraModIncreaseHealthPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    // Unit will keep hp% after MaxHealth being modified if unit is alive.
    float percent = target->GetHealthPct();

    target->HandleStatModifier(UNIT_MOD_HEALTH, TOTAL_PCT, float(GetAmount()), apply);

    if (target->isAlive())
        target->SetHealth(target->CountPctFromMaxHealth(int32(percent)));
}

void AuraEffect::HandleAuraModPetStatsModifier(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    if(Unit* target = aurApp->GetTarget())
    {
        Guardian* pet = NULL;
        if (Player* _player = target->ToPlayer())
            pet = _player->GetGuardianPet();

        if(!pet)
            pet = target->ToPet();

        if (pet)
        {
            if(GetMiscValue() == PETSPELLMOD_ARMOR)
                pet->UpdateArmor();
            if(GetMiscValue() == PETSPELLMOD_MAX_HP)
            {
                if(apply)
                    pet->ModifyHealth(GetAmount());
                pet->UpdateMaxHealth();
            }
        }
    }
}

void AuraEffect::HandleAuraIncreaseBaseHealthPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    target->HandleStatModifier(UNIT_MOD_HEALTH, BASE_PCT, float(GetAmount()), apply);
}

/********************************/
/***          FIGHT           ***/
/********************************/

void AuraEffect::HandleAuraModParryPercent(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->ToPlayer()->UpdateParryPercentage();
}

void AuraEffect::HandleAuraModDodgePercent(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->ToPlayer()->UpdateDodgePercentage();
}

void AuraEffect::HandleAuraModBlockPercent(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    target->ToPlayer()->UpdateBlockPercentage();
}

void AuraEffect::HandleAuraModRegenInterrupt(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    HandleModManaRegen(aurApp, mode, apply);
}

void AuraEffect::HandleAuraModWeaponCritPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int i = 0; i < MAX_ATTACK; ++i)
        if (Item* pItem = target->ToPlayer()->GetWeaponForAttack(WeaponAttackType(i), true))
            target->ToPlayer()->_ApplyWeaponDependentAuraCritMod(pItem, WeaponAttackType(i), this, apply);

    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // GetMiscValue() comparison with item generated damage types

    if (GetSpellInfo()->EquippedItemClass == -1)
    {
        target->ToPlayer()->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float (GetAmount()), apply);
        target->ToPlayer()->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float (GetAmount()), apply);
        target->ToPlayer()->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float (GetAmount()), apply);
    }
    else
    {
        // done in Player::_ApplyWeaponDependentAuraMods
    }
}

void AuraEffect::HandleModHitChance(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        target->ToPlayer()->UpdateMeleeHitChances();
        target->ToPlayer()->UpdateRangedHitChances();
    }
    else
    {
        target->m_modMeleeHitChance += (apply) ? GetAmount() : (-GetAmount());
        target->m_modRangedHitChance += (apply) ? GetAmount() : (-GetAmount());
    }
}

void AuraEffect::HandleModSpellHitChance(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->UpdateSpellHitChances();
    else
        target->m_modSpellHitChance += (apply) ? GetAmount(): (-GetAmount());
}

void AuraEffect::HandleModSpellCritChance(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->UpdateAllSpellCritChances();
    else
        target->m_baseSpellCritChance += (apply) ? GetAmount():-GetAmount();
}

void AuraEffect::HandleModSpellCritChanceShool(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    for (int school = SPELL_SCHOOL_NORMAL; school < MAX_SPELL_SCHOOL; ++school)
        if (GetMiscValue() & (1<<school))
            target->ToPlayer()->UpdateSpellCritChance(school);
}

void AuraEffect::HandleAuraModCritPct(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
    {
        target->m_baseSpellCritChance += (apply) ? GetAmount():-GetAmount();
        return;
    }

    target->ToPlayer()->HandleBaseModValue(CRIT_PERCENTAGE,         FLAT_MOD, float (GetAmount()), apply);
    target->ToPlayer()->HandleBaseModValue(OFFHAND_CRIT_PERCENTAGE, FLAT_MOD, float (GetAmount()), apply);
    target->ToPlayer()->HandleBaseModValue(RANGED_CRIT_PERCENTAGE,  FLAT_MOD, float (GetAmount()), apply);

    // included in Player::UpdateSpellCritChance calculation
    target->ToPlayer()->UpdateAllSpellCritChances();
}

void AuraEffect::HandleAuraModResiliencePct(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* _player = target->ToPlayer();

    float baseValue = _player->GetFloatValue(PLAYER_FIELD_MOD_RESILIENCE_PCT);

    if (apply)
        baseValue += GetAmount() / 100.0f;
    else
        baseValue -= GetAmount() / 100.0f;

    _player->SetFloatValue(PLAYER_FIELD_MOD_RESILIENCE_PCT, baseValue);
}

/********************************/
/***         ATTACK SPEED     ***/
/********************************/

void AuraEffect::HandleModCastingSpeed(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    target->UpdateHastMod();
}

void AuraEffect::HandleModMeleeRangedSpeedPct(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    //! ToDo: Haste auras with the same handler _CAN'T_ stack together
    Unit* target = aurApp->GetTarget();

    target->ApplyAttackTimePercentMod(BASE_ATTACK, (float)GetAmount(), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, (float)GetAmount(), apply);
    target->ApplyAttackTimePercentMod(RANGED_ATTACK, (float)GetAmount(), apply);

    target->UpdateMeleeHastMod();
    target->UpdateRangeHastMod();
}

void AuraEffect::HandleModCombatSpeedPct(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    target->ApplyAttackTimePercentMod(BASE_ATTACK, float(GetAmount()), apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK, float(GetAmount()), apply);
    target->ApplyAttackTimePercentMod(RANGED_ATTACK, float(GetAmount()), apply);
    
    target->UpdateHastMod();
    target->UpdateMeleeHastMod();
    target->UpdateRangeHastMod();
}

void AuraEffect::HandleModAttackSpeed(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    target->ApplyAttackTimePercentMod(BASE_ATTACK, (float)GetAmount(), apply);
    target->UpdateDamagePhysical(BASE_ATTACK);
}

void AuraEffect::HandleModMeleeSpeedPct(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    //! ToDo: Haste auras with the same handler _CAN'T_ stack together
    Unit* target = aurApp->GetTarget();

    int32 value = GetAmount();

    target->ApplyAttackTimePercentMod(BASE_ATTACK,   (float)value, apply);
    target->ApplyAttackTimePercentMod(OFF_ATTACK,    (float)value, apply);

    target->UpdateMeleeHastMod();
}

void AuraEffect::HandleAuraModRangedHaste(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    //! ToDo: Haste auras with the same handler _CAN'T_ stack together
    Unit* target = aurApp->GetTarget();

    target->ApplyAttackTimePercentMod(RANGED_ATTACK, (float)GetAmount(), apply);
    target->UpdateRangeHastMod();
}

/********************************/
/***       COMBAT RATING      ***/
/********************************/

void AuraEffect::HandleModRating(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
        if (GetMiscValue() & (1 << rating))
        {
            switch (rating)
            {
                case CR_HASTE_MELEE:
                case CR_HASTE_RANGED:
                case CR_HASTE_SPELL:
                    target->ToPlayer()->ApplyRatingMod(CombatRating(rating), GetAmount(), apply);
                    break;
                default:
                    target->ToPlayer()->UpdateRating(CombatRating(rating));
                    break;
            }
        }
}

void AuraEffect::HandleModRatingFromStat(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // Just recalculate ratings
    for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
        if (GetMiscValue() & (1 << rating))
            target->ToPlayer()->ApplyRatingMod(CombatRating(rating), 0, apply);
}

/********************************/
/***        ATTACK POWER      ***/
/********************************/

void AuraEffect::HandleAuraModAttackPower(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    target->UpdateAttackPowerAndDamage();
    //target->HandleStatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, float(GetAmount()), apply);
}

void AuraEffect::HandleAuraModRangedAttackPower(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if ((target->getClassMask() & CLASSMASK_WAND_USERS) != 0)
        return;

    target->UpdateAttackPowerAndDamage(true);
    //target->HandleStatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, float(GetAmount()), apply);
}

void AuraEffect::HandleAuraModAttackPowerPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    // Don't apply Markmanship aura twice on pet
    if (GetCaster() && GetCaster()->ToPlayer() && aurApp->GetBase()->GetId() == 19506)
        if (Pet* pet = GetCaster()->ToPlayer()->GetPet())
            if (target->GetGUID() == pet->GetGUID())
                return;

    if (target->ToPlayer())
        target->ToPlayer()->UpdateAttackPowerAndDamage(false);
    else if (Pet* pet = target->ToPet())
        pet->UpdateAttackPowerAndDamage(false);
}

void AuraEffect::HandleAuraModRangedAttackPowerPercent(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if ((target->getClassMask() & CLASSMASK_WAND_USERS) != 0)
        return;

    if (target->ToPlayer())
        target->ToPlayer()->UpdateAttackPowerAndDamage(true);
}

void AuraEffect::HandleAuraModAttackPowerOfArmor(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    // Recalculate bonus
    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->UpdateAttackPowerAndDamage(false);
}

void AuraEffect::HandleOverrideAttackPowerBySpellPower(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    // Recalculate bonus
    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->UpdateAttackPowerAndDamage(false);
}

void AuraEffect::HandleOverrideSpellPowerByAttackPower(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Player* target = aurApp->GetTarget()->ToPlayer();
    if (!target)
        return;

   target->UpdateSpellDamageAndHealingBonus();
}

void AuraEffect::HandleIncreaseHasteFromItemsByPct(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    if (!target->ToPlayer())
        return;

    for (uint32 rating = 0; rating < MAX_COMBAT_RATING; ++rating)
        if (GetMiscValue() & (1 << rating))
            target->ToPlayer()->UpdateRating(CombatRating(rating));
}

/********************************/
/***        DAMAGE BONUS      ***/
/********************************/
void AuraEffect::HandleModDamageDone(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    // apply item specific bonuses for already equipped weapon
    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
            if (Item* pItem = target->ToPlayer()->GetWeaponForAttack(WeaponAttackType(i), true))
                target->ToPlayer()->_ApplyWeaponDependentAuraDamageMod(pItem, WeaponAttackType(i), this, apply);
    }

    // GetMiscValue() is bitmask of spell schools
    // 1 (0-bit) - normal school damage (SPELL_SCHOOL_MASK_NORMAL)
    // 126 - full bitmask all magic damages (SPELL_SCHOOL_MASK_MAGIC) including wands
    // 127 - full bitmask any damages
    //
    // mods must be applied base at equipped weapon class and subclass comparison
    // with spell->EquippedItemClass and  EquippedItemSubClassMask and EquippedItemInventoryTypeMask
    // GetMiscValue() comparison with item generated damage types

    if ((GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL) != 0)
    {
        // apply generic physical damage bonuses including wand case
        if (GetSpellInfo()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER)
        {
            target->HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(GetAmount()), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(GetAmount()), apply);
            target->HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(GetAmount()), apply);

            if (target->GetTypeId() == TYPEID_PLAYER)
            {
                if (GetAmount() > 0)
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_POS, GetAmount(), apply);
                else
                    target->ApplyModUInt32Value(PLAYER_FIELD_MOD_DAMAGE_DONE_NEG, GetAmount(), apply);
            }
        }
        else
        {
            // done in Player::_ApplyWeaponDependentAuraMods
        }
    }

    // Skip non magic case for speedup
    if ((GetMiscValue() & SPELL_SCHOOL_MASK_MAGIC) == 0)
        return;

    if (GetSpellInfo()->EquippedItemClass != -1 || (GetSpellInfo()->EquippedItemInventoryTypeMask != 0 && GetSpellInfo()->EquippedItemInventoryTypeMask != -1))
    {
        // wand magic case (skip generic to all item spell bonuses)
        // done in Player::_ApplyWeaponDependentAuraMods

        // Skip item specific requirements for not wand magic damage
        return;
    }

    // Magic damage modifiers implemented in Unit::SpellDamageBonus
    // This information for client side use only
    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        target->ToPlayer()->ApplySpellPowerBonus(GetAmount(), apply);

        if (Guardian* pet = target->ToPlayer()->GetGuardianPet())
            pet->UpdateAttackPowerAndDamage();
    }
}

void AuraEffect::HandleModDamagePercentDone(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();
    if (!target)
        return;

    float amount = target->GetTotalAuraMultiplier(SPELL_AURA_MOD_DAMAGE_PERCENT_DONE);

    if (target->GetTypeId() == TYPEID_PLAYER)
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
            if (Item* item = target->ToPlayer()->GetWeaponForAttack(WeaponAttackType(i), false))
                target->ToPlayer()->_ApplyWeaponDependentAuraDamageMod(item, WeaponAttackType(i), this, apply);
    }

    if ((GetMiscValue() & SPELL_SCHOOL_MASK_NORMAL) && (GetSpellInfo()->EquippedItemClass == -1 || target->GetTypeId() != TYPEID_PLAYER))
    {
        for (int i = 0; i < MAX_ATTACK; ++i)
            target->UpdateDamagePhysical(WeaponAttackType(i));

        if (target->GetTypeId() == TYPEID_PLAYER)
            target->ToPlayer()->SetFloatValue(PLAYER_FIELD_MOD_DAMAGE_DONE_PCT, amount);
    }
}

void AuraEffect::HandleModOffhandDamagePercent(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    target->HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_PCT, float(GetAmount()), apply);
}

void AuraEffect::HandleShieldBlockValue(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_STAT)))
        return;

    Unit* target = aurApp->GetTarget();

    BaseModType modType = FLAT_MOD;
    if (GetAuraType() == SPELL_AURA_MOD_SHIELD_BLOCKVALUE_PCT)
        modType = PCT_MOD;

    if (target->GetTypeId() == TYPEID_PLAYER)
        target->ToPlayer()->HandleBaseModValue(SHIELD_BLOCK_VALUE, modType, float(GetAmount()), apply);
}

/********************************/
/***        POWER COST        ***/
/********************************/

void AuraEffect::HandleModPowerCostPCT(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    float amount = CalculatePct(1.0f, GetAmount());
    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if (GetMiscValue() & (1 << i))
            target->ApplyModSignedFloatValue(UNIT_FIELD_POWER_COST_MULTIPLIER + i, amount, apply);

    // Preparation
    // This allows changind spec while in battleground
    if (GetId() == 44521)
        target->ModifyAuraState(AURA_STATE_UNKNOWN20, apply);
}

void AuraEffect::HandleModPowerCost(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    for (int i = 0; i < MAX_SPELL_SCHOOL; ++i)
        if (GetMiscValue() & (1<<i))
            target->ApplyModInt32Value(UNIT_FIELD_POWER_COST_MODIFIER+i, GetAmount(), apply);
}

void AuraEffect::HandleArenaPreparation(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION);
    else
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
        target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PREPARATION);
    }

    // Preparation
    // This allows changind spec while in battleground
    if (apply || !target->HasAuraType(SPELL_AURA_ARENA_PREPARATION))
        target->ModifyAuraState(AURA_STATE_UNKNOWN20, apply);
}

void AuraEffect::HandleNoReagentUseAura(AuraApplication const* aurApp, uint8 mode, bool /*apply*/) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    flag128 mask;
    Unit::AuraEffectList const& noReagent = target->GetAuraEffectsByType(SPELL_AURA_NO_REAGENT_USE);
        for (Unit::AuraEffectList::const_iterator i = noReagent.begin(); i != noReagent.end(); ++i)
            mask |= (*i)->m_spellInfo->Effects[(*i)->m_effIndex].SpellClassMask;

    target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1  , mask[0]);
    target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1+1, mask[1]);
    target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1+2, mask[2]);
    target->SetUInt32Value(PLAYER_NO_REAGENT_COST_1+3, mask[3]);
}

void AuraEffect::HandleAuraRetainComboPoints(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    // combo points was added in SPELL_EFFECT_ADD_COMBO_POINTS handler
    // remove only if aura expire by time (in case combo points amount change aura removed without combo points lost)
    if (!(apply) && GetBase()->GetDuration() == 0 && target->ToPlayer()->GetComboTarget())
        if (Unit* unit = ObjectAccessor::GetUnit(*target, target->ToPlayer()->GetComboTarget()))
            target->ToPlayer()->AddComboPoints(unit, -GetAmount());
}

/*********************************************************/
/***                    OTHERS                         ***/
/*********************************************************/

void AuraEffect::HandleAuraDummy(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & (AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK | AURA_EFFECT_HANDLE_REAPPLY)))
        return;

    Unit* target = aurApp->GetTarget();

    Unit* caster = GetCaster();

    if (mode & (AURA_EFFECT_HANDLE_REAL | AURA_EFFECT_HANDLE_REAPPLY))
    {
        // AT APPLY
        if (apply)
        {
            switch (GetId())
            {
                case 96733: //Permanent Feign Death (Stun)
                    if (target)
                    {
                        target->SetUInt32Value(OBJECT_FIELD_DYNAMIC_FLAGS, 0x64);
                        target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29 | UNIT_FLAG_UNK_15 | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_IMMUNE_TO_PC);
                        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                    }
                    break;
                case 1515:                                      // Tame beast
                    // FIX_ME: this is 2.0.12 threat effect replaced in 2.1.x by dummy aura, must be checked for correctness
                    if (caster && target->CanHaveThreatList())
                        target->AddThreat(caster, 10.0f);
                    break;
                case 13139:                                     // net-o-matic
                    // root to self part of (root_target->charge->root_self sequence
                    if (caster)
                        caster->CastSpell(caster, 13138, true, NULL, this);
                    break;
                case 34026:   // kill command
                {
                    Unit* pet = target->GetGuardianPet();
                    if (!pet)
                        break;

                    target->CastSpell(target, 34027, true, NULL, this);

                    // set 3 stacks and 3 charges (to make all auras not disappear at once)
                    Aura* owner_aura = target->GetAura(34027, GetCasterGUID());
                    Aura* pet_aura  = pet->GetAura(58914, GetCasterGUID());
                    if (owner_aura != NULL)
                    {
                        owner_aura->SetStackAmount(owner_aura->GetSpellInfo()->StackAmount);
                        if (pet_aura != NULL)
                        {
                            pet_aura->SetCharges(0);
                            pet_aura->SetStackAmount(owner_aura->GetSpellInfo()->StackAmount);
                        }
                    }
                    break;
                }
                case 37096:                                     // Blood Elf Illusion
                {
                    if (caster)
                    {
                        switch (caster->getGender())
                        {
                            case GENDER_FEMALE:
                                caster->CastSpell(target, 37095, true, NULL, this); // Blood Elf Disguise
                                break;
                            case GENDER_MALE:
                                caster->CastSpell(target, 37093, true, NULL, this);
                                break;
                            default:
                                break;
                        }
                    }
                    break;
                }
                case 39850:                                     // Rocket Blast
                    if (roll_chance_i(20))                       // backfire stun
                        target->CastSpell(target, 51581, true, NULL, this);
                    break;
                case 43873:                                     // Headless Horseman Laugh
                    target->PlayDistanceSound(11965);
                    break;
                case 46354:                                     // Blood Elf Illusion
                    if (caster)
                    {
                        switch (caster->getGender())
                        {
                            case GENDER_FEMALE:
                                caster->CastSpell(target, 46356, true, NULL, this);
                                break;
                            case GENDER_MALE:
                                caster->CastSpell(target, 46355, true, NULL, this);
                                break;
                        }
                    }
                    break;
                case 46361:                                     // Reinforced Net
                    if (caster)
                        target->GetMotionMaster()->MoveFall();
                    break;
                case 71563:
                    {
                        Aura* newAura = target->AddAura(71564, target);
                        if (newAura != NULL)
                            newAura->SetStackAmount(newAura->GetSpellInfo()->StackAmount);
                    }
                    break;
                case 59628: // Tricks of the Trade
                    if (caster && caster->GetMisdirectionTarget())
                        target->SetReducedThreatPercent(100, caster->GetMisdirectionTarget()->GetGUID());
                    break;
            }
        }
        // AT REMOVE
        else
        {
            if ((GetSpellInfo()->IsQuestTame()) && caster && caster->isAlive() && target->isAlive())
            {
                uint32 finalSpelId = 0;
                switch (GetId())
                {
                    case 19548: finalSpelId = 19597; break;
                    case 19674: finalSpelId = 19677; break;
                    case 19687: finalSpelId = 19676; break;
                    case 19688: finalSpelId = 19678; break;
                    case 19689: finalSpelId = 19679; break;
                    case 19692: finalSpelId = 19680; break;
                    case 19693: finalSpelId = 19684; break;
                    case 19694: finalSpelId = 19681; break;
                    case 19696: finalSpelId = 19682; break;
                    case 19697: finalSpelId = 19683; break;
                    case 19699: finalSpelId = 19685; break;
                    case 19700: finalSpelId = 19686; break;
                    case 30646: finalSpelId = 30647; break;
                    case 30653: finalSpelId = 30648; break;
                    case 30654: finalSpelId = 30652; break;
                    case 30099: finalSpelId = 30100; break;
                    case 30102: finalSpelId = 30103; break;
                    case 30105: finalSpelId = 30104; break;
                }

                if (finalSpelId)
                    caster->CastSpell(target, finalSpelId, true, NULL, this);
            }

            switch (m_spellInfo->SpellFamilyName)
            {
                case SPELLFAMILY_GENERIC:
                    switch (GetId())
                    {
                        case 96733: //Permanent Feign Death (Stun)
                            if (target)
                            {
                                target->SetUInt32Value(OBJECT_FIELD_DYNAMIC_FLAGS, 0);
                                target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_UNK_29 | UNIT_FLAG_UNK_15 | UNIT_FLAG_IMMUNE_TO_NPC | UNIT_FLAG_IMMUNE_TO_PC);
                                target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_FEIGN_DEATH);
                            }
                            break;
                        case 2584: // Waiting to Resurrect
                            // Waiting to resurrect spell cancel, we must remove player from resurrect queue
                            if (target->GetTypeId() == TYPEID_PLAYER)
                            {
                                if (Battleground* bg = target->ToPlayer()->GetBattleground())
                                    bg->RemovePlayerFromResurrectQueue(target->GetGUID());
                                if (Battlefield* bf = sBattlefieldMgr->GetBattlefieldToZoneId(target->GetZoneId()))
                                    bf->RemovePlayerFromResurrectQueue(target->GetGUID());
                            }
                            break;
                        case 36730:                                     // Flame Strike
                        {
                            target->CastSpell(target, 36731, true, NULL, this);
                            break;
                        }
                        case 44191:                                     // Flame Strike
                        {
                            if (target->GetMap()->IsDungeon())
                            {
                                uint32 spellId = target->GetMap()->IsHeroic() ? 46163 : 44190;

                                target->CastSpell(target, spellId, true, NULL, this);
                            }
                            break;
                        }
                        case 43681: // Inactive
                        {
                            if (target->GetTypeId() != TYPEID_PLAYER || aurApp->GetRemoveMode() != AURA_REMOVE_BY_EXPIRE)
                                return;

                            if (target->GetMap()->IsBattleground())
                                target->ToPlayer()->LeaveBattleground();
                            break;
                        }
                        case 42783: // Wrath of the Astromancer
                            target->CastSpell(target, GetAmount(), true, NULL, this);
                            break;
                        case 46308: // Burning Winds casted only at creatures at spawn
                            target->CastSpell(target, 47287, true, NULL, this);
                            break;
                        case 52172:  // Coyote Spirit Despawn Aura
                        case 60244:  // Blood Parrot Despawn Aura
                            target->CastSpell((Unit*)NULL, GetAmount(), true, NULL, this);
                            break;
                        case 58600: // Restricted Flight Area
                        case 83100: // Restricted Flight Area
                        case 91604: // Restricted Flight Area
                            if (aurApp->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE)
                                target->CastSpell(target, 58601, true);
                            break;
                    }
                    break;
                case SPELLFAMILY_DEATHKNIGHT:
                    // Summon Gargoyle (Dismiss Gargoyle at remove)
                    if (GetId() == 61777)
                        target->CastSpell(target, GetAmount(), true);
                    break;
				case SPELLFAMILY_ROGUE:
					//  Tricks of the trade
                    switch (GetId())
                    {
                        case 59628: //Tricks of the trade buff on rogue (6sec duration)
                            target->SetReducedThreatPercent(0,0);
                            break;
                        case 57934: //Tricks of the trade buff on rogue (30sec duration)
                            if (aurApp->GetRemoveMode() == AURA_REMOVE_BY_EXPIRE || !caster->GetMisdirectionTarget())
                                target->SetReducedThreatPercent(0,0);
                            else
                                target->SetReducedThreatPercent(0,caster->GetMisdirectionTarget()->GetGUID());
                            break;
                    }
                    break;
				default:
					break;
			}
		}
    }

    // AT APPLY & REMOVE

    switch (m_spellInfo->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
        {
            if (!(mode & AURA_EFFECT_HANDLE_REAL))
                break;
            switch (GetId())
            {
                // Recently Bandaged
                case 11196:
                    target->ApplySpellImmune(GetId(), IMMUNITY_MECHANIC, GetMiscValue(), apply);
                    break;
                // Unstable Power
                case 24658:
                {
                    uint32 spellId = 24659;
                    if (apply && caster)
                    {
                        SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId);

                        for (uint32 i = 0; i < spell->StackAmount; ++i)
                            caster->CastSpell(target, spell->Id, true, NULL, NULL, GetCasterGUID());
                        break;
                    }
                    target->RemoveAurasDueToSpell(spellId);
                    break;
                }
                // Restless Strength
                case 24661:
                {
                    uint32 spellId = 24662;
                    if (apply && caster)
                    {
                        SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId);
                        for (uint32 i = 0; i < spell->StackAmount; ++i)
                            caster->CastSpell(target, spell->Id, true, NULL, NULL, GetCasterGUID());
                        break;
                    }
                    target->RemoveAurasDueToSpell(spellId);
                    break;
                }
                // Tag Murloc
                case 30877:
                {
                    // Tag/untag Blacksilt Scout
                    target->SetEntry(apply ? 17654 : 17326);
                    break;
                }
                case 57819: // Argent Champion
                case 57820: // Ebon Champion
                case 57821: // Champion of the Kirin Tor
                case 57822: // Wyrmrest Champion
                case 93337: // Champion of Ramkahen
                case 93339: // Champion of the Earthen Ring
                case 93341: // Champion of the Guardians of Hyjal
                case 93347: // Champion of Therazane
                case 93368: // Champion of the Wildhammer Clan
                case 94158: // Champion of the Dragonmaw Clan
                case 93795: // Stormwind Champion
                case 93805: // Ironforge Champion
                case 93806: // Darnassus Champion
                case 93811: // Exodar Champion
                case 93816: // Gilneas Champion
                case 93821: // Gnomeregan Champion
                case 93825: // Orgrimmar Champion
                case 93827: // Darkspear Champion
                case 93828: // Silvermoon Champion
                case 93830: // Bilgewater Champion
                case 94462: // Undercity Champion
                case 94463: // Thunder Bluff Champion
                case 126434: // Tushui
                case 126436: // Huojin
                {
                    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
                        break;

                    uint32 FactionID = 0;
                    uint32 DungeonLevel = 0;

                    if (apply)
                    {
                        switch (m_spellInfo->Id)
                        {
                            case 57819: FactionID = 1106; break; // Argent Crusade
                            case 57820: FactionID = 1098; break; // Knights of the Ebon Blade
                            case 57821: FactionID = 1090; break; // Kirin Tor
                            case 57822: FactionID = 1091; break; // The Wyrmrest Accord
                            // Alliance factions
                            case 93795: FactionID = 72;   DungeonLevel = 0;  break; // Stormwind
                            case 93805: FactionID = 47;   DungeonLevel = 0;  break; // Ironforge
                            case 93806: FactionID = 69;   DungeonLevel = 0;  break; // Darnassus
                            case 93811: FactionID = 930;  DungeonLevel = 0;  break; // Exodar
                            case 93816: FactionID = 1134; DungeonLevel = 0;  break; // Gilneas
                            case 93821: FactionID = 54;   DungeonLevel = 0;  break; // Gnomeregan
                            case 126434: FactionID = 1353; DungeonLevel = 0; break; // Tushui Pandaren
                            // Horde factions
                            case 93825: FactionID = 76;   DungeonLevel = 0;  break; // Orgrimmar
                            case 93827: FactionID = 530;  DungeonLevel = 0;  break; // Darkspear Trolls
                            case 93828: FactionID = 911;  DungeonLevel = 0;  break; // Silvermoon
                            case 93830: FactionID = 1133; DungeonLevel = 0;  break; // Bilgewater Cartel
                            case 94462: FactionID = 68;   DungeonLevel = 0;  break; // Undercity
                            case 94463: FactionID = 81;   DungeonLevel = 0;  break; // Thunder Bluff
                            case 126436: FactionID = 1352; DungeonLevel = 0; break; // Huojin Pandaren
                            // Cataclysm factions
                            case 93337: FactionID = 1173; DungeonLevel = 83; break; // Ramkahen
                            case 93339: FactionID = 1135; DungeonLevel = 83; break; // The Earthen Ring
                            case 93341: FactionID = 1158; DungeonLevel = 83; break; // Guardians of Hyjal
                            case 93347: FactionID = 1171; DungeonLevel = 83; break; // Therazane
                            case 93368: FactionID = 1174; DungeonLevel = 83; break; // Wildhammer Clan
                            case 94158: FactionID = 1172; DungeonLevel = 83; break; // Dragonmaw Clan
                        }
                    }
                    caster->ToPlayer()->SetChampioningFaction(FactionID, DungeonLevel);
                    break;
                }
                // LK Intro VO (1)
                case 58204:
                    if (target->GetTypeId() == TYPEID_PLAYER)
                    {
                        // Play part 1
                        if (apply)
                            target->PlayDirectSound(14970, target->ToPlayer());
                        // continue in 58205
                        else
                            target->CastSpell(target, 58205, true);
                    }
                    break;
                // LK Intro VO (2)
                case 58205:
                    if (target->GetTypeId() == TYPEID_PLAYER)
                    {
                        // Play part 2
                        if (apply)
                            target->PlayDirectSound(14971, target->ToPlayer());
                        // Play part 3
                        else
                            target->PlayDirectSound(14972, target->ToPlayer());
                    }
                    break;
                case 62061: // Festive Holiday Mount
                    if (target->HasAuraType(SPELL_AURA_MOUNTED))
                    {
                        uint32 creatureEntry = 0;
                        if (apply)
                        {
                            if (target->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED))
                                creatureEntry = 24906;
                            else
                                creatureEntry = 15665;
                        }
                        else
                            creatureEntry = target->GetAuraEffectsByType(SPELL_AURA_MOUNTED).front()->GetMiscValue();

                        if (CreatureTemplate const* creatureInfo = sObjectMgr->GetCreatureTemplate(creatureEntry))
                        {
                            uint32 team = 0;
                            if (target->GetTypeId() == TYPEID_PLAYER)
                                team = target->ToPlayer()->GetTeam();

                            uint32 displayID = sObjectMgr->ChooseDisplayId(team, creatureInfo);
                            sObjectMgr->GetCreatureModelRandomGender(&displayID);

                            target->SetUInt32Value(UNIT_FIELD_MOUNTDISPLAYID, displayID);
                        }
                    }
                    break;
            }

            break;
        }
        case SPELLFAMILY_MAGE:
        {
            //if (!(mode & AURA_EFFECT_HANDLE_REAL))
                //break;
            break;
        }
        case SPELLFAMILY_PRIEST:
        {
            //if (!(mode & AURA_EFFECT_HANDLE_REAL))
                //break;
            break;
        }
        case SPELLFAMILY_DRUID:
        {
            if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
                break;

            switch (GetId())
            {
                case 52610:                                 // Savage Roar
                case 127538:                                // Savage Roar
                {
                    if (apply)
                    {
                        target->RemoveAurasDueToSpell(GetId() == 52610 ? 127538 : 52610);
                        if (target->GetShapeshiftForm() == FORM_CAT)
                            target->CastSpell(target, 62071, true, NULL, NULL, GetCasterGUID());
                    }
                    else
                        target->RemoveAurasDueToSpell(62071);
                    break;
                }
                case 61336:                                 // Survival Instincts
                {
                    if (!(mode & AURA_EFFECT_HANDLE_REAL))
                        break;

                    if (apply)
                    {
                        if (!target->IsInFeralForm())
                            break;

                        target->CastSpell(target, 50322, true);
                    }
                    else
                        target->RemoveAurasDueToSpell(50322);
                    break;
                }
            }
            // Predatory Strikes
            if (target->GetTypeId() == TYPEID_PLAYER && GetSpellInfo()->SpellIconID == 1563)
            {
                target->ToPlayer()->UpdateAttackPowerAndDamage();
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
        {
            if (!(mode & AURA_EFFECT_HANDLE_REAL))
                break;
            // Sentry Totem
            if (GetId() == 6495 && caster && caster->GetTypeId() == TYPEID_PLAYER)
            {
                if (apply)
                {
                    if (uint64 guid = caster->m_SummonSlot[4])
                    {
                        if (Creature* totem = caster->GetMap()->GetCreature(guid))
                            if (totem->isTotem())
                                caster->ToPlayer()->CastSpell(totem, 6277, true);
                    }
                }
                else
                    caster->ToPlayer()->StopCastingBindSight();
                return;
            }
            if (GetSpellInfo()->SpellIconID == 4777) // Lava Surge
            {
                if (apply)
                {
                    if (caster && caster->ToPlayer()->HasSpellCooldown(51505)) // Lava Burst
                        caster->ToPlayer()->RemoveSpellCooldown(51505, true);
                }
            }
            break;
        }
        case SPELLFAMILY_PALADIN:
        {
            if (!(mode & AURA_EFFECT_HANDLE_REAL))
                break;

            switch (GetId())
            {
                // Guardian of Ancient Kings (Retribution spec)
                case 86698:
                {
                    if (apply)
                        caster->CastSpell(caster, 86701, true);
                    else
                    {
                        if (caster->HasAura(86700))
                            caster->CastSpell(caster, 86704, true);

                        caster->RemoveAurasDueToSpell(86700);
                        caster->RemoveAurasDueToSpell(86701);
                    }
                    break;
                }
                // Guardian of Ancient Kings (Holy spec)
                case 86669:
                {
                    if (!apply)
                        caster->RemoveAurasDueToSpell(86674);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
        {
            if (!(mode & AURA_EFFECT_HANDLE_REAL))
                break;
            // Improved Frost Presence
            if (m_spellInfo->SpellIconID == 2636)
            {
                if (apply)
                {
                    if (!target->HasAura(48266) && !target->HasAura(63611))
                        target->CastSpell(target, 63611, true);
                }
                else
                    target->RemoveAurasDueToSpell(63611);
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            if (!(mode & AURA_EFFECT_HANDLE_REAL))
                break;

            switch (GetId())
            {
                // Smoke Bomb
                case 76577:
                {
                    if (apply)
                    {
                        if (Aura* aur = caster->AddAura(88611, target))
                        {
                            aur->SetMaxDuration(GetBase()->GetDuration());
                            aur->SetDuration(GetBase()->GetDuration());
                        }
                    }
                    else
                        target->RemoveAura(88611);
                    break;
                }
            }

            break;
        }
		case SPELLFAMILY_WARLOCK:
			{
				switch(GetId())
				{
					// Demonic Pact
				case 47236:
					caster->CastSpell(caster, 53646, true);
					break;
				}
				break;   
			}
        }
    }
}

void AuraEffect::HandleChannelDeathItem(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    if (apply || aurApp->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
        return;

    Unit* caster = GetCaster();

    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* plCaster = caster->ToPlayer();
    Unit* target = aurApp->GetTarget();

    // Item amount
    if (GetAmount() <= 0)
        return;

    if (GetSpellInfo()->GetEffect(m_effIndex, m_diffMode).ItemType == 0)
        return;

    //Adding items
    uint32 noSpaceForCount = 0;
    uint32 count = m_amount;

    ItemPosCountVec dest;
    InventoryResult msg = plCaster->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, GetSpellInfo()->GetEffect(m_effIndex, m_diffMode).ItemType, count, &noSpaceForCount);
    if (msg != EQUIP_ERR_OK)
    {
        count-=noSpaceForCount;
        plCaster->SendEquipError(msg, NULL, NULL, GetSpellInfo()->GetEffect(m_effIndex, m_diffMode).ItemType);
        if (count == 0)
            return;
    }

    Item* newitem = plCaster->StoreNewItem(dest, GetSpellInfo()->GetEffect(m_effIndex, m_diffMode).ItemType, true);
    if (!newitem)
    {
        plCaster->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }
    plCaster->SendNewItem(newitem, NULL, count, true, true);
}

void AuraEffect::HandleBindSight(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    Unit* caster = GetCaster();

    if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
        return;

    caster->ToPlayer()->SetViewpoint(target, apply);
}

void AuraEffect::HandleForceReaction(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = (Player*)target;

    uint32 faction_id = GetMiscValue();
    ReputationRank faction_rank = ReputationRank(m_amount);

    player->GetReputationMgr().ApplyForceReaction(faction_id, faction_rank, apply);
    player->GetReputationMgr().SendForceReactions();

    // stop fighting if at apply forced rank friendly or at remove real rank friendly
    if ((apply && faction_rank >= REP_FRIENDLY) || (!apply && player->GetReputationRank(faction_id) >= REP_FRIENDLY))
        player->StopAttackFaction(faction_id);
}

void AuraEffect::HandleAuraEmpathy(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_UNIT)
        return;

    if (!apply)
    {
        // do not remove unit flag if there are more than this auraEffect of that kind on unit on unit
        if (target->HasAuraType(GetAuraType()))
            return;
    }

    CreatureTemplate const* ci = sObjectMgr->GetCreatureTemplate(target->GetEntry());
    if (ci && ci->type == CREATURE_TYPE_BEAST)
        target->ApplyModUInt32Value(OBJECT_FIELD_DYNAMIC_FLAGS, UNIT_DYNFLAG_SPECIALINFO, apply);
}

void AuraEffect::HandleAuraModFaction(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
    {
        target->setFaction(GetMiscValue());
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            //target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            target->SetFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP);
            target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_IGNORE_REPUTATION);
        }
    }
    else
    {
        target->RestoreFaction();
        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            //target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PVP_ATTACKABLE);
            target->RemoveFlag(PLAYER_FLAGS, PLAYER_FLAGS_CONTESTED_PVP);
            target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_IGNORE_REPUTATION);
        }
    }
}

void AuraEffect::HandleComprehendLanguage(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_SEND_FOR_CLIENT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
        target->SetFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_COMPREHEND_LANG);
    else
    {
        if (target->HasAuraType(GetAuraType()))
            return;

        target->RemoveFlag(UNIT_FIELD_FLAGS_2, UNIT_FLAG2_COMPREHEND_LANG);
    }
}

void AuraEffect::HandleAuraConvertRune(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    Player* player = (Player*)target;

    if (player->getClass() != CLASS_DEATH_KNIGHT)
        return;

    uint32 runes = m_amount;
    // convert number of runes specified in aura amount of rune type in miscvalue to runetype in miscvalueb
    if (apply)
    {
        for (uint32 i = 0; i < MAX_RUNES && runes; ++i)
        {
            if (GetMiscValue() != player->GetCurrentRune(i))
                continue;
            if (!player->GetRuneCooldown(i))
            {
                player->AddRuneBySpell(i, RuneType(GetMiscValueB()), GetId());
                --runes;
            }
        }
    }
    else
        player->RemoveRunesBySpell(GetId());
}

void AuraEffect::HandleAuraLinked(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    Unit* target = aurApp->GetTarget();

    uint32 triggeredSpellId = m_spellInfo->GetEffect(m_effIndex, m_diffMode).TriggerSpell;
    SpellInfo const* triggeredSpellInfo = sSpellMgr->GetSpellInfo(triggeredSpellId);
    if (!triggeredSpellInfo)
        return;

    if (mode & AURA_EFFECT_HANDLE_REAL)
    {
        if (apply)
        {
            Unit* caster = triggeredSpellInfo->NeedsToBeTriggeredByCaster() ? GetCaster() : target;

            if (!caster)
                return;
            // If amount avalible cast with basepoints (Crypt Fever for example)
            if (GetAmount())
                caster->CastCustomSpell(target, triggeredSpellId, &m_amount, NULL, NULL, true, NULL, this);
            else
                caster->CastSpell(target, triggeredSpellId, true, NULL, this);
        }
        else
        {
            uint64 casterGUID = triggeredSpellInfo->NeedsToBeTriggeredByCaster() ? GetCasterGUID() : target->GetGUID();
            target->RemoveAura(triggeredSpellId, casterGUID, 0, aurApp->GetRemoveMode());
        }
    }
    else if (mode & AURA_EFFECT_HANDLE_REAPPLY && apply)
    {
        uint64 casterGUID = triggeredSpellInfo->NeedsToBeTriggeredByCaster() ? GetCasterGUID() : target->GetGUID();
        // change the stack amount to be equal to stack amount of our aura
        Aura* triggeredAura = target->GetAura(triggeredSpellId, casterGUID);
        if (triggeredAura != NULL)
            triggeredAura->ModStackAmount(GetBase()->GetStackAmount() - triggeredAura->GetStackAmount());
    }
}

void AuraEffect::HandleAuraOpenStable(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (target->GetTypeId() != TYPEID_PLAYER || !target->IsInWorld())
        return;

    if (apply)
        target->ToPlayer()->GetSession()->SendStablePet(target->GetGUID());

     // client auto close stable dialog at !apply aura
}

void AuraEffect::HandleAuraModFakeInebriation(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_CHANGE_AMOUNT_MASK))
        return;

    Unit* target = aurApp->GetTarget();

    if (apply)
    {
        target->m_invisibilityDetect.AddFlag(INVISIBILITY_DRUNK);
        target->m_invisibilityDetect.AddValue(INVISIBILITY_DRUNK, GetAmount());

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            int32 oldval = target->ToPlayer()->GetInt32Value(PLAYER_FAKE_INEBRIATION);
            target->ToPlayer()->SetInt32Value(PLAYER_FAKE_INEBRIATION, oldval + GetAmount());
        }
    }
    else
    {
        bool removeDetect = !target->HasAuraType(SPELL_AURA_MOD_FAKE_INEBRIATE);

        target->m_invisibilityDetect.AddValue(INVISIBILITY_DRUNK, -GetAmount());

        if (target->GetTypeId() == TYPEID_PLAYER)
        {
            int32 oldval = target->ToPlayer()->GetInt32Value(PLAYER_FAKE_INEBRIATION);
            target->ToPlayer()->SetInt32Value(PLAYER_FAKE_INEBRIATION, oldval - GetAmount());

            if (removeDetect)
                removeDetect = !target->ToPlayer()->GetDrunkValue();
        }

        if (removeDetect)
            target->m_invisibilityDetect.DelFlag(INVISIBILITY_DRUNK);
    }

    // call functions which may have additional effects after chainging state of unit
    target->UpdateObjectVisibility();
}

void AuraEffect::HandleAuraOverrideSpells(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Player* target = aurApp->GetTarget()->ToPlayer();

    if (!target || !target->IsInWorld())
        return;

    uint32 overrideId = uint32(GetMiscValue());

    if (apply)
    {
        target->SetUInt32Value(PLAYER_FIELD_OVERRIDE_SPELLS_ID, overrideId << 16);
        if (OverrideSpellDataEntry const* overrideSpells = sOverrideSpellDataStore.LookupEntry(overrideId))
            for (uint8 i = 0; i < MAX_OVERRIDE_SPELL; ++i)
                if (uint32 spellId = overrideSpells->spellId[i])
                    target->AddTemporarySpell(spellId);
    }
    else
    {
        target->SetUInt32Value(PLAYER_FIELD_OVERRIDE_SPELLS_ID, 0);
        if (OverrideSpellDataEntry const* overrideSpells = sOverrideSpellDataStore.LookupEntry(overrideId))
            for (uint8 i = 0; i < MAX_OVERRIDE_SPELL; ++i)
                if (uint32 spellId = overrideSpells->spellId[i])
                    target->RemoveTemporarySpell(spellId);
    }
}

void AuraEffect::HandleAuraSetVehicle(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (!target)
        return;

    uint32 vehicleId = GetMiscValue();
    if (apply)
    {
        if (!target->CreateVehicleKit(vehicleId, 0, GetId()))
            return;
    }
    else if (target->GetVehicleKit())
        target->RemoveVehicleKit();

    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    WorldPacket data;
    if (apply)
    {
        //send before init vehicle
        WorldPacket data(SMSG_FORCE_SET_VEHICLE_REC_ID, 16);
        data.WriteGuidMask<1, 5, 0, 6, 4, 3, 7, 2>(target->GetObjectGuid());
        data.WriteGuidBytes<7, 2, 5, 6, 4>(target->GetObjectGuid());
        data << uint32(vehicleId);
        data.WriteGuidBytes<3, 1, 0>(target->GetObjectGuid());
        data << uint32(target->ToPlayer()->GetTimeSync()+1);          //CMSG_TIME_SYNC_RESP incremenet counter
        target->ToPlayer()->GetSession()->SendPacket(&data);

        // Initialize vehicle
        if (Vehicle * veh = target->GetVehicleKit())
            veh->Reset();
        else 
            return;
    }

    //SMSG_PLAYER_VEHICLE_DATA semd on HandleSetVehicleRecId

    if (apply)
        target->ToPlayer()->SendOnCancelExpectedVehicleRideAura();
}

void AuraEffect::HandlePreventResurrection(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    if (aurApp->GetTarget()->GetTypeId() != TYPEID_PLAYER)
        return;

    if (apply)
        aurApp->GetTarget()->RemoveByteFlag(PLAYER_FIELD_BYTES, 0, PLAYER_FIELD_BYTE_RELEASE_TIMER);
    else if (!aurApp->GetTarget()->GetBaseMap()->Instanceable())
        aurApp->GetTarget()->SetByteFlag(PLAYER_FIELD_BYTES, 0, PLAYER_FIELD_BYTE_RELEASE_TIMER);
}

void AuraEffect::HandlePeriodicDummyAuraTick(Unit* target, Unit* caster, SpellEffIndex /*effIndex*/) const
{
	if (GetId() == 102522)
    {
        switch(rand()%4)
        {
            case 1:
                caster->CastSpell(caster, 109090);
                break;
            case 2:
                caster->CastSpell(caster, 109105);
                break;
        }
        
        if(GetBase()->GetDuration() < 1000)
            caster->CastSpell(caster, 109107);
    }

    uint32 trigger_spell_id = m_spellInfo->GetEffect(m_effIndex, m_diffMode).TriggerSpell;

    switch (GetSpellInfo()->SpellFamilyName)
    {
        case SPELLFAMILY_GENERIC:
            switch (GetId())
            {
                case 146198: // Essence of Yu'lon
                {
                    if(caster && target)
                        caster->CastCustomSpell(target, 148008, &m_amount, 0, 0, true, 0, this);
                    break;
                }
                case 146199: // Spirit of Chi-Ji
                {
                    if (!caster->isInCombat())
                        if (roll_chance_i(50))
                            caster->CastSpell(caster, 148956, true);
                    break;
                }
                case 146195: // Flurry of Xuen
                {
                    if (!caster->isInCombat())
                        if (roll_chance_i(50))
                            caster->CastSpell(caster, 148957, true);
                    break;
                }
                case 146193: // Endurance of Niuzao
                {
                    if (!caster->isInCombat())
                        if (roll_chance_i(50))
                            caster->CastSpell(caster, 148958, true);
                    break;
                }
                case 146197: // Essence of Yu'lon
                {
                    if (!caster->isInCombat())
                        if (roll_chance_i(50))
                            caster->CastSpell(caster, 148954, true);
                    break;
                }
                case 145108: // Ysera's Gift
                {
                    uint32 triggerSpell = caster->IsFullHealth() ? 145110: 145109;
                    int32 heal = CalculatePct(caster->GetMaxHealth(), m_amount);

                    caster->CastCustomSpell(target, triggerSpell, &heal, 0, 0, true);
                    break;
                }
                case 146285: // Cruelty
                {
                    if(caster && target)
                        caster->CastCustomSpell(target, 146293, &m_amount, 0, 0, true, 0, this);
                    break;
                }
                case 146310: // Restless Agility
                case 146317: // Restless Spirit
                {
                    if (Aura* aur = GetBase())
                    {
                        if (aur->GetStackAmount() > 1)
                            aur->SetStackAmount(aur->GetStackAmount() - 1);
                    }
                    break;
                }
                case 25281: // Turkey Marker
                {
                    if (Aura* aur = GetBase())
                    {
                        if (aur->GetStackAmount() >= 15)
                        {
                            target->CastSpell(target, 25285, true);
                            target->RemoveAura(25281);
                        }
                    }
                    break;
                }
                case 146184: // Wrath
                {
                    caster->CastCustomSpell(target, 146202, &m_amount, 0, 0, true, 0, this);
                    break;
                }
                case 118694: // Spirit Bond
                {
                    if (caster->GetOwner() && (!caster->IsFullHealth() || !caster->GetOwner()->IsFullHealth()))
                        trigger_spell_id = 149254;
                    break;
                }
                case 113957: // Cooking
                    target->CastSpell(caster, 113950, true);
                    break;
                case 66149: // Bullet Controller Periodic - 10 Man
                {
                    if (!caster)
                        break;

                    caster->CastCustomSpell(66152, SPELLVALUE_MAX_TARGETS, urand(1, 6), target, true);
                    caster->CastCustomSpell(66153, SPELLVALUE_MAX_TARGETS, urand(1, 6), target, true);
                    break;
                }
                case 62399: // Overload Circuit
                    if (target->GetMap()->IsDungeon() && int(target->GetAppliedAuras().count(62399)) >= (target->GetMap()->IsHeroic() ? 4 : 2))
                    {
                         target->CastSpell(target, 62475, true); // System Shutdown
                         if (Unit* veh = target->GetVehicleBase())
                             veh->CastSpell(target, 62475, true);
                    }
                    break;
                case 64821: // Fuse Armor (Razorscale)
                    if (GetBase()->GetStackAmount() == GetSpellInfo()->StackAmount)
                    {
                        target->CastSpell(target, 64774, true, NULL, NULL, GetCasterGUID());
                        target->RemoveAura(64821);
                    }
                    break;
            }
            break;
        case SPELLFAMILY_DRUID:
        {
            switch (GetSpellInfo()->Id)
            {
                case 142423:
                {
                    trigger_spell_id = 142424;
                    break;
                }
                case 81262: // Efflorescence
                {
                    trigger_spell_id = 81269;
                    break;
                }
            }
            break;
        }
        case SPELLFAMILY_ROGUE:
        {
            switch (GetSpellInfo()->Id)
            {
                // Master of Subtlety
                case 31666:
                    if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                        target->RemoveAurasDueToSpell(31665);
                    break;
                // Overkill
                case 58428:
                    if (!target->HasAuraType(SPELL_AURA_MOD_STEALTH))
                        target->RemoveAurasDueToSpell(58427);
                    break;
            }
            break;
        }
        case SPELLFAMILY_HUNTER:
        {
            switch (GetSpellInfo()->Id)
            {
                // Camouflage
                case 80326:
                {
                    if (!caster || (caster->isMoving() && !caster->HasAura(119449) && !caster->isPet()) || caster->HasAura(80325))
                        return;

                    if (caster->HasAura(119449) || (caster->GetOwner() && caster->GetOwner()->HasAura(119449)))
                        caster->CastSpell(caster, 119450, true);
                    else
                        caster->CastSpell(caster, 80325, true);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_SHAMAN:
            if (GetId() == 52179) // Astral Shift
            {
                // Periodic need for remove visual on stun/fear/silence lost
                if (!(target->GetUInt32Value(UNIT_FIELD_FLAGS)&(UNIT_FLAG_STUNNED|UNIT_FLAG_FLEEING|UNIT_FLAG_SILENCED)))
                    target->RemoveAurasDueToSpell(52179);
                break;
            }
            break;
        // Custom MoP Script
        case SPELLFAMILY_MONK:
        {
            if (!caster)
                break;

            switch (GetId())
            {
                case 138130: // Storm, Earth and Fire (clone)
                {
                    if (Creature* crt = caster->ToCreature())
                        if (CreatureAI* ai = crt->AI())
                            ai->RecalcStats();
                    break;
                }
                case 116095: // Disable : duration refresh every 14 second if target remains within 10 yards of the Monk
                {
                    if (GetTickNumber() == GetTotalTicks())
                        if (target->IsInRange(caster, 0, 10))
                            if (Aura* Disable = GetBase())
                                Disable->RefreshTimers();
                    break;
                }
                case 125950: // Soothing Mist
                {
                    trigger_spell_id = 125953;
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case SPELLFAMILY_DEATHKNIGHT:
            switch (GetId())
            {
                // Death's Advance
                case 96268:
                {
                    if (caster->ToPlayer()->GetRuneCooldown(RUNE_UNHOLY*2) && caster->ToPlayer()->GetRuneCooldown(RUNE_UNHOLY*2+1))
                        GetBase()->RefreshDuration();
                    else
                        caster->RemoveAurasDueToSpell(96268);
                    break;
                }
                // Death and Decay
                case 43265:
                {
                    if (caster)
                        caster->CastCustomSpell(target, 52212, &m_amount, NULL, NULL, true, 0, this);
                    break;
                }
                default:
                    break;
            }
            // Blood Rites
            // Reaping
            if (GetSpellInfo()->Id == 50034 || GetSpellInfo()->Id == 56835)
            {
                if (target->GetTypeId() != TYPEID_PLAYER)
                    return;
                if (target->ToPlayer()->getClass() != CLASS_DEATH_KNIGHT)
                    return;

                 // timer expired - remove death runes
                target->ToPlayer()->RemoveRunesBySpell(GetId());
            }
            break;
        default:
            break;
    }

    if(caster && trigger_spell_id)
    {
        //if (DynamicObject* dynObj = caster->GetDynObject(GetId()))
        if(uint64 dynObjGuid = GetBase()->GetSpellDynamicObject())
        {
            Unit* owner = caster->GetAnyOwner();
            if(DynamicObject* dynObj = ObjectAccessor::GetDynamicObject(*caster, dynObjGuid))
                caster->CastSpell(dynObj->GetPositionX(), dynObj->GetPositionY(), dynObj->GetPositionZ(), trigger_spell_id, true, NULL, this, owner ? owner->GetGUID() : 0);
        }
        else if(target)
            caster->CastSpell(target, trigger_spell_id, true, NULL, this);
    }
}

void AuraEffect::HandlePeriodicTriggerSpellAuraTick(Unit* target, Unit* caster, SpellEffIndex /*effIndex*/) const
{
    // generic casting code with custom spells and target/caster customs
    uint32 triggerSpellId = GetSpellInfo()->GetEffect(GetEffIndex(), m_diffMode).TriggerSpell;

    SpellInfo const* triggeredSpellInfo = sSpellMgr->GetSpellInfo(triggerSpellId);
    SpellInfo const* auraSpellInfo = GetSpellInfo();
    uint32 auraId = auraSpellInfo->Id;

    // specific code for cases with no trigger spell provided in field
    if (triggeredSpellInfo == NULL)
    {
        switch (auraSpellInfo->SpellFamilyName)
        {
            case SPELLFAMILY_GENERIC:
            {
                switch (auraId)
                {
                    // Cluster Cluck: Remote Control Fireworks Visual
                    case 74177:
                        target->PlayDistanceSound(6820);
                        return;
                    // resistible Pool Pony: Naga Hatchling Proximity Control - Master
                    case 71917:
                        target->PlayDistanceSound(3437);
                        return;
                    // Thaumaturgy Channel
                    case 9712:
                        triggerSpellId = 21029;
                        break;
                    // Brood Affliction: Bronze
                    case 23170:
                        triggerSpellId = 23171;
                        break;
                    // Party G.R.E.N.A.D.E
                    case 51510:
                        triggerSpellId = 51508;
                        break;
                    // Restoration
                    case 24379:
                    case 23493:
                    {
                        if (caster)
                        {
                            int32 heal = caster->CountPctFromMaxHealth(10);
                            caster->HealBySpell(target, auraSpellInfo, heal);

                            if (int32 mana = caster->GetMaxPower(POWER_MANA))
                            {
                                mana /= 10;
                                caster->EnergizeBySpell(caster, 23493, mana, POWER_MANA);
                            }
                        }
                        return;
                    }
                    // Nitrous Boost
                    case 27746:
                        if (caster && target->GetPower(POWER_MANA) >= 10)
                        {
                            target->ModifyPower(POWER_MANA, -10, true);
                            target->SendEnergizeSpellLog(caster, 27746, 10, POWER_MANA);
                        }
                        else
                            target->RemoveAurasDueToSpell(27746);
                        return;
                    // Frost Blast
                    case 27808:
                        if (caster)
                            caster->CastCustomSpell(29879, SPELLVALUE_BASE_POINT0, int32(target->CountPctFromMaxHealth(21)), target, true, NULL, this);
                        return;
                    // Inoculate Nestlewood Owlkin
                    case 29528:
                        if (target->GetTypeId() != TYPEID_UNIT) // prevent error reports in case ignored player target
                            return;
                        break;
                    // Feed Captured Animal
                    case 29917:
                        triggerSpellId = 29916;
                        break;
                    // Extract Gas
                    case 30427:
                    {
                        // move loot to player inventory and despawn target
                        if (caster && caster->GetTypeId() == TYPEID_PLAYER &&
                                target->GetTypeId() == TYPEID_UNIT &&
                                target->ToCreature()->GetCreatureTemplate()->type == CREATURE_TYPE_GAS_CLOUD)
                        {
                            Player* player = caster->ToPlayer();
                            Creature* creature = target->ToCreature();
                            // missing lootid has been reported on startup - just return
                            if (!creature->GetCreatureTemplate()->SkinLootId)
                                return;

                            player->AutoStoreLoot(creature->GetCreatureTemplate()->SkinLootId, LootTemplates_Skinning, 0, true);

                            creature->DespawnOrUnsummon();
                        }
                        return;
                    }
                    // Quake
                    case 30576:
                        triggerSpellId = 30571;
                        break;
                    // Doom
                    // TODO: effect trigger spell may be independant on spell targets, and executed in spell finish phase
                    // so instakill will be naturally done before trigger spell
                    case 31347:
                    {
                        target->CastSpell(target, 31350, true, NULL, this);
                        target->Kill(target);
                        return;
                    }
                    // Spellcloth
                    case 31373:
                    {
                        // Summon Elemental after create item
                        target->SummonCreature(17870, 0, 0, 0, target->GetOrientation(), TEMPSUMMON_DEAD_DESPAWN, 0);
                        return;
                    }
                    // Flame Quills
                    case 34229:
                    {
                        // cast 24 spells 34269-34289, 34314-34316
                        for (uint32 spell_id = 34269; spell_id != 34290; ++spell_id)
                            target->CastSpell(target, spell_id, true, NULL, this);
                        for (uint32 spell_id = 34314; spell_id != 34317; ++spell_id)
                            target->CastSpell(target, spell_id, true, NULL, this);
                        return;
                    }
                    // Remote Toy
                    case 37027:
                        triggerSpellId = 37029;
                        break;
                    // Eye of Grillok
                    case 38495:
                        triggerSpellId = 38530;
                        break;
                    // Absorb Eye of Grillok (Zezzak's Shard)
                    case 38554:
                    {
                        if (!caster || target->GetTypeId() != TYPEID_UNIT)
                            return;

                        caster->CastSpell(caster, 38495, true, NULL, this);

                        Creature* creatureTarget = target->ToCreature();

                        creatureTarget->DespawnOrUnsummon();
                        return;
                    }
                    // Tear of Azzinoth Summon Channel - it's not really supposed to do anything, and this only prevents the console spam
                    case 39857:
                        triggerSpellId = 39856;
                        break;
                    // Personalized Weather
                    case 46736:
                        triggerSpellId = 46737;
                        break;
                    //Sha pool (Immerseus)
                    case 143462: 
                        {
                            if (caster && caster->ToCreature())
                            {
                                switch (caster->GetEntry())
                                {
                                case 71544: //npc_sha_pool
                                    triggerSpellId = 143297; //Sha splash
                                    break;
                                case 71543: //boss_immerseus
                                    triggerSpellId = 143460; //Sha pool dmg
                                    break;
                                default:
                                    break;
                                }
                            }
                        }
                        break;
                }
                break;
            }
            case SPELLFAMILY_SHAMAN:
            {
                switch (auraId)
                {
                    // Totemic Mastery (Skyshatter Regalia (Shaman Tier 6) - bonus)
                    case 38443:
                    {
                        bool all = true;
                        for (int i = SUMMON_SLOT_TOTEM; i < MAX_TOTEM_SLOT; ++i)
                        {
                            if (!target->m_SummonSlot[i])
                            {
                                all = false;
                                break;
                            }
                        }

                        if (all)
                            target->CastSpell(target, 38437, true, NULL, this);
                        else
                            target->RemoveAurasDueToSpell(38437);
                        return;
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    else
    {
        // Spell exist but require custom code
        switch (auraId)
        {
            case 113656: // Fists of Fury
            {
                if (caster->GetTypeId() != TYPEID_PLAYER)
                    return;
                break;
            }
            // The Biggest Egg Ever: Mechachicken's Rocket Barrage Aura Effect
            case 71416:
                // prock 71419 with this action.
                target->PlayDistanceSound(23829);
                target->HandleEmoteCommand(403);
                break;
            case 114889: // Stone Bulwark Totem
            {
                if (Unit * shaman = caster->GetOwner())
                {
                    int32 SPD    = shaman->GetSpellPowerDamage();
                    int32 amount = SPD * 0.875f * (GetTickNumber() != 1 ? 1: 4);
                    caster->CastCustomSpell(shaman, triggerSpellId, &amount, NULL, NULL, true);
                }
                return;
            }
            // Pursuing Spikes (Anub'arak)
            case 65920:
            case 65922:
            case 65923:
            {
                Unit* permafrostCaster = NULL;
                Aura* permafrostAura = target->GetAura(66193);
                if (!permafrostAura)
                    permafrostAura = target->GetAura(67855);
                if (!permafrostAura)
                    permafrostAura = target->GetAura(67856);
                if (!permafrostAura)
                    permafrostAura = target->GetAura(67857);

                if (permafrostAura)
                    permafrostCaster = permafrostAura->GetCaster();

                if (permafrostCaster)
                {
                    if (Creature* permafrostCasterCreature = permafrostCaster->ToCreature())
                        permafrostCasterCreature->DespawnOrUnsummon(3000);

                    target->CastSpell(target, 66181, false);
                    target->RemoveAllAuras();
                    if (Creature* targetCreature = target->ToCreature())
                        targetCreature->DisappearAndDie();
                }
                break;
            }
            // Mana Tide
            case 16191:
                target->CastCustomSpell(target, triggerSpellId, &m_amount, NULL, NULL, true, NULL, this);
                return;
            // Negative Energy Periodic
            case 46284:
                target->CastCustomSpell(triggerSpellId, SPELLVALUE_MAX_TARGETS, m_tickNumber / 10 + 1, NULL, true, NULL, this);
                return;
            // Poison (Grobbulus)
            case 28158:
            case 54362:
            // Slime Pool (Dreadscale & Acidmaw)
            case 66882:
                target->CastCustomSpell(triggerSpellId, SPELLVALUE_RADIUS_MOD, (int32)((((float)m_tickNumber / 60) * 0.9f + 0.1f) * 10000 * 2 / 3), NULL, true, NULL, this);
                return;
            // Beacon of Light
            case 53563:
            {
                // area aura owner casts the spell
                GetBase()->GetUnitOwner()->CastSpell(target, triggeredSpellInfo, true, 0, this, GetBase()->GetUnitOwner()->GetGUID());
                return;
            }
            // Slime Spray - temporary here until preventing default effect works again
            // added on 9.10.2010
            case 69508:
            {
                if (caster)
                    caster->CastSpell(target, triggerSpellId, true, NULL, NULL, caster->GetGUID());
                return;
            }
            case 24745: // Summon Templar, Trigger
            case 24747: // Summon Templar Fire, Trigger
            case 24757: // Summon Templar Air, Trigger
            case 24759: // Summon Templar Earth, Trigger
            case 24761: // Summon Templar Water, Trigger
            case 24762: // Summon Duke, Trigger
            case 24766: // Summon Duke Fire, Trigger
            case 24769: // Summon Duke Air, Trigger
            case 24771: // Summon Duke Earth, Trigger
            case 24773: // Summon Duke Water, Trigger
            case 24785: // Summon Royal, Trigger
            case 24787: // Summon Royal Fire, Trigger
            case 24791: // Summon Royal Air, Trigger
            case 24792: // Summon Royal Earth, Trigger
            case 24793: // Summon Royal Water, Trigger
            {
                // All this spells trigger a spell that requires reagents; if the
                // triggered spell is cast as "triggered", reagents are not consumed
                if (caster)
                    caster->CastSpell(target, triggerSpellId, false);
                return;
            }
            case 106768:
                return;
        }
    }

    // Reget trigger spell proto
    triggeredSpellInfo = sSpellMgr->GetSpellInfo(triggerSpellId);

    if (triggeredSpellInfo)
    {
        if (Unit* triggerCaster = triggeredSpellInfo->NeedsToBeTriggeredByCaster() ? caster : target)
        {
            //if (DynamicObject* dynObj = triggerCaster->GetDynObject(GetId()))
            if(uint64 dynObjGuid = GetBase()->GetSpellDynamicObject())
            {
                Unit* owner = triggerCaster->GetAnyOwner();
                if(DynamicObject* dynObj = ObjectAccessor::GetDynamicObject(*triggerCaster, dynObjGuid))
                    triggerCaster->CastSpell(dynObj->GetPositionX(), dynObj->GetPositionY(), dynObj->GetPositionZ(), triggerSpellId, true, NULL, this, owner ? owner->GetGUID() : 0);
                else
                    triggerCaster->CastSpell(target, triggeredSpellInfo, true, NULL, this);
            }
            else
                triggerCaster->CastSpell(target, triggeredSpellInfo, true, NULL, this);
        }
    }
    else
    {
        Creature* c = target->ToCreature();
        if (!c || !caster || !sScriptMgr->OnDummyEffect(caster, GetId(), SpellEffIndex(GetEffIndex()), target->ToCreature()) ||
            !c->AI()->sOnDummyEffect(caster, GetId(), SpellEffIndex(GetEffIndex())))
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AuraEffect::HandlePeriodicTriggerSpellAuraTick: Spell %u has non-existent spell %u in EffectTriggered[%d] and is therefor not triggered.", GetId(), triggerSpellId, GetEffIndex());
    }
}

void AuraEffect::HandlePeriodicTriggerSpellWithValueAuraTick(Unit* target, Unit* caster, SpellEffIndex /*effIndex*/) const
{
    uint32 triggerSpellId = GetSpellInfo()->GetEffect(m_effIndex, m_diffMode).TriggerSpell;
    if (SpellInfo const* triggeredSpellInfo = sSpellMgr->GetSpellInfo(triggerSpellId))
    {
        if (Unit* triggerCaster = triggeredSpellInfo->NeedsToBeTriggeredByCaster() ? caster : target)
        {
            int32 basepoints0 = GetAmount();
            GetBase()->CallScriptEffectChangeTickDamageHandlers(const_cast<AuraEffect const*>(this), basepoints0, target);
            triggerCaster->CastCustomSpell(target, triggerSpellId, &basepoints0, 0, 0, true, 0, this);
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AuraEffect::HandlePeriodicTriggerSpellWithValueAuraTick: Spell %u Trigger %u", GetId(), triggeredSpellInfo->Id);
        }
    }
    else
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS,"AuraEffect::HandlePeriodicTriggerSpellWithValueAuraTick: Spell %u has non-existent spell %u in EffectTriggered[%d] and is therefor not triggered.", GetId(), triggerSpellId, GetEffIndex());
}

void AuraEffect::HandlePeriodicDamageAurasTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const
{
    if (!caster || !target->isAlive())
        return;

    if (target->HasUnitState(UNIT_STATE_ISOLATED) || target->IsImmunedToDamage(GetSpellInfo()))
    {
        SendTickImmune(target, caster);
        return;
    }

    // Consecrate ticks can miss and will not show up in the combat log
    if (GetSpellInfo()->GetEffect(GetEffIndex(), m_diffMode).Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
        caster->SpellHitResult(target, GetSpellInfo(), false, 1 << GetEffIndex()) != SPELL_MISS_NONE)
        return;

    // some auras remove at specific health level or more
    if (GetAuraType() == SPELL_AURA_PERIODIC_DAMAGE)
    {
        switch (GetSpellInfo()->Id)
        {
            case 43093: case 31956: case 38801:  // Grievous Wound
            case 35321: case 38363: case 39215:  // Gushing Wound
            {
                if (target->IsFullHealth())
                {
                    target->RemoveAurasDueToSpell(GetSpellInfo()->Id);
                    return;
                }
                break;
            }
            case 38772: // Grievous Wound
            {
                uint32 percent = GetSpellInfo()->Effects[EFFECT_1].CalcValue(caster);
                if (!target->HealthBelowPct(percent))
                {
                    target->RemoveAurasDueToSpell(GetSpellInfo()->Id);
                    return;
                }
                break;
            }
        }
    }

    uint32 absorb = 0;
    uint32 resist = 0;
    CleanDamage cleanDamage = CleanDamage(0, 0, BASE_ATTACK, MELEE_HIT_NORMAL);

    // ignore non positive values (can be result apply spellmods to aura damage
    uint32 damage = std::max(GetAmount(), 0);

    bool crit = roll_chance_f(GetCritChance());
    if (crit)
        damage = GetCritAmount();

    if (GetAuraType() == SPELL_AURA_PERIODIC_DAMAGE)
    {
        if (GetSpellInfo()->GetEffect(effIndex, m_diffMode).Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA)
        {
            float critChance = 0.0f;
            damage = caster->SpellDamageBonusDone(target, GetSpellInfo(), damage, DOT, effIndex, GetBase()->GetStackAmount());
            caster->isSpellCrit(target, GetSpellInfo(), GetSpellInfo()->GetSchoolMask(), BASE_ATTACK, critChance);
            crit = roll_chance_f(critChance);
            if (crit)
                damage = caster->SpellCriticalDamageBonus(GetSpellInfo(), damage, target);;
        }
        damage = target->SpellDamageBonusTaken(caster, GetSpellInfo(), damage, DOT, effIndex, GetBase()->GetStackAmount());

        // Calculate armor mitigation
        if (Unit::IsDamageReducedByArmor(GetSpellInfo()->GetSchoolMask(), GetSpellInfo(), (1 << effIndex)))
        {
            uint32 damageReductedArmor = caster->CalcArmorReducedDamage(target, damage, GetSpellInfo());
            cleanDamage.mitigated_damage += damage - damageReductedArmor;
            damage = damageReductedArmor;
        }

        // Curse of Agony damage-per-tick calculation
        if (GetSpellInfo()->Id == 980 && m_tickNumber != 1) // Agony
            GetBase()->ModStackAmount(1);

        // Malefic Grasp
        if (GetSpellInfo()->Id == 103103)
        {
            int32 afflictionDamage = 0;

            // Every tick, Malefic Grasp deals instantly 50% of tick-damage for each affliction effects on the target
            // Corruption ...
            if (Aura* corruption = target->GetAura(146739, caster->GetGUID()))
            {
                afflictionDamage = corruption->GetEffect(0)->GetAmount();
                afflictionDamage = CalculatePct(afflictionDamage, GetSpellInfo()->Effects[2].BasePoints);

                caster->CastCustomSpell(target, 131740, &afflictionDamage, NULL, NULL, true);
            }
            // Unstable Affliction ...
            if (Aura* unstableAffliction = target->GetAura(30108, caster->GetGUID()))
            {
                afflictionDamage = unstableAffliction->GetEffect(0)->GetAmount();
                afflictionDamage = CalculatePct(afflictionDamage, GetSpellInfo()->Effects[2].BasePoints);

                caster->CastCustomSpell(target, 131736, &afflictionDamage, NULL, NULL, true);
            }
            // Agony ...
            if (Aura* agony = target->GetAura(980, caster->GetGUID()))
            {
                afflictionDamage = agony->GetEffect(0)->GetAmount();
                afflictionDamage = CalculatePct(afflictionDamage, GetSpellInfo()->Effects[2].BasePoints);

                caster->CastCustomSpell(target, 131737, &afflictionDamage, NULL, NULL, true);
            }
        }
        // Soul Drain
        if (GetSpellInfo()->Id == 1120)
        {
            // Energize one soul shard every 2 ticks
            if (m_tickNumber == 2 || m_tickNumber == 4 || m_tickNumber == 6)
                caster->ModifyPower(POWER_SOUL_SHARDS, 100);

            // if target is below 20% of life ...
            if (target->GetHealthPct() <= 20)
            {
                // ... drain soul deal 100% more damage ...
                damage *= 2;

                int32 afflictionDamage = 0;
                bool grimoireOfSacrifice = caster->HasAura(108503);

                // ... and deals instantly 100% of tick-damage for each affliction effects on the target
                // Corruption ...
                if (Aura* corruption = target->GetAura(146739, caster->GetGUID()))
                {
                    afflictionDamage = corruption->GetEffect(0)->GetAmount();

                    if (grimoireOfSacrifice)
                        AddPct(afflictionDamage, 50);

                    caster->CastCustomSpell(target, 131740, &afflictionDamage, NULL, NULL, true);
                }
                // Unstable Affliction ...
                if (Aura* unstableAffliction = target->GetAura(30108, caster->GetGUID()))
                {
                    afflictionDamage = unstableAffliction->GetEffect(0)->GetAmount();

                    if (grimoireOfSacrifice)
                        AddPct(afflictionDamage, 50);

                    caster->CastCustomSpell(target, 131736, &afflictionDamage, NULL, NULL, true);
                }
                // Agony ...
                if (Aura* agony = target->GetAura(980, caster->GetGUID()))
                {
                    afflictionDamage = agony->GetEffect(0)->GetAmount();

                    if (grimoireOfSacrifice)
                        AddPct(afflictionDamage, 50);

                    caster->CastCustomSpell(target, 131737, &afflictionDamage, NULL, NULL, true);
                }
            }
        }
        if (GetSpellInfo()->SpellFamilyName == SPELLFAMILY_GENERIC)
        {
            switch (GetId())
            {
                case 70911: // Unbound Plague
                case 72854: // Unbound Plague
                case 72855: // Unbound Plague
                case 72856: // Unbound Plague
                    damage *= uint32(pow(1.25f, int32(m_tickNumber)));
                    break;
                default:
                    break;
            }
        }
    }
    else if (GetSpellInfo()->GetEffect(effIndex, m_diffMode).Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA)
    {
        float critChance = 0.0f;
        damage = uint32(target->CountPctFromMaxHealth(damage));
        caster->isSpellCrit(target, GetSpellInfo(), GetSpellInfo()->GetSchoolMask(), BASE_ATTACK, critChance);
        crit = roll_chance_f(critChance);
        if (crit)
            damage = caster->SpellCriticalDamageBonus(GetSpellInfo(), damage, target);;
    }

    // If Doom critical tick, a Wild Imp will appear to fight with the Warlock
    if (m_spellInfo->Id == 603 && crit)
        caster->CastSpell(caster, 104317, true);

    int32 dmg = damage;

    GetBase()->CallScriptEffectChangeTickDamageHandlers(const_cast<AuraEffect const*>(this), dmg, target);

    if (!(GetSpellInfo()->AttributesEx6 & SPELL_ATTR6_NO_DONE_PCT_DAMAGE_MODS) && m_spellInfo->Id != 110914)
        caster->ApplyResilience(target, &dmg, crit);

    caster->CalcAbsorbResist(target, GetSpellInfo()->GetSchoolMask(), DOT, dmg, &absorb, &resist, GetSpellInfo());

    damage = dmg;

    sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "PeriodicTick: %u (TypeId: %u) attacked %u (TypeId: %u) for %u dmg inflicted by %u abs is %u",
        GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), damage, GetId(), absorb);

    caster->DealDamageMods(target, damage, &absorb);

    DamageInfo dmgInfoProc = DamageInfo(caster, target, damage, GetSpellInfo(), GetSpellInfo() ? SpellSchoolMask(GetSpellInfo()->SchoolMask) : SPELL_SCHOOL_MASK_NORMAL, SPELL_DIRECT_DAMAGE);
    dmgInfoProc.AbsorbDamage(absorb);
    dmgInfoProc.ResistDamage(resist);

    // Set trigger flag
    uint32 procAttacker = PROC_FLAG_DONE_PERIODIC;
    uint32 procVictim   = PROC_FLAG_TAKEN_PERIODIC;
    uint32 procEx = (crit ? PROC_EX_CRITICAL_HIT : PROC_EX_NORMAL_HIT) | PROC_EX_INTERNAL_DOT;
    damage = (damage <= absorb+resist) ? 0 : (damage-absorb-resist);
    if (damage)
        procVictim |= PROC_FLAG_TAKEN_DAMAGE;

    if (damage > 0)
    {
        if (GetSpellInfo()->Effects[m_effIndex].IsTargetingArea() || GetSpellInfo()->Effects[m_effIndex].Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA)
        {
            resist += damage;
            damage = int32(float(damage) * target->GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_AOE_DAMAGE_AVOIDANCE, GetSpellInfo()->GetSchoolMask()));
            if (GetCaster() && GetCaster()->GetTypeId() == TYPEID_UNIT)
                damage = int32(float(damage) * target->GetTotalAuraMultiplierByMiscMask(SPELL_AURA_MOD_CREATURE_AOE_DAMAGE_AVOIDANCE, GetSpellInfo()->GetSchoolMask()));
                resist -= damage;
        }
    }

    int32 overkill = damage - target->GetHealth();
    if (overkill < 0)
        overkill = 0;

    SpellPeriodicAuraLogInfo pInfo(this, damage, overkill, absorb, resist, 0.0f, crit);
    target->SendPeriodicAuraLog(&pInfo);

    if (absorb && !damage && target->HasAuraType(SPELL_AURA_MOD_STEALTH))
        if (GetTickNumber() == 1)
            target->RemoveAurasByType(SPELL_AURA_MOD_STEALTH);

    caster->ProcDamageAndSpell(target, procAttacker, procVictim, procEx, &dmgInfoProc, BASE_ATTACK, GetSpellInfo());

    caster->DealDamage(target, damage, &cleanDamage, DOT, GetSpellInfo()->GetSchoolMask(), GetSpellInfo(), true);
}

void AuraEffect::HandlePeriodicHealthLeechAuraTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const
{
    if (!caster || !target->isAlive())
        return;

    if (target->HasUnitState(UNIT_STATE_ISOLATED) || target->IsImmunedToDamage(GetSpellInfo()))
    {
        SendTickImmune(target, caster);
        return;
    }

    if (GetSpellInfo()->GetEffect(GetEffIndex(), m_diffMode).Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
        caster->SpellHitResult(target, GetSpellInfo(), false, 1 << GetEffIndex()) != SPELL_MISS_NONE)
        return;

    uint32 absorb = 0;
    uint32 resist = 0;
    CleanDamage cleanDamage = CleanDamage(0, 0, BASE_ATTACK, MELEE_HIT_NORMAL);

    uint32 damage = std::max(GetAmount(), 0);

    damage = target->SpellDamageBonusTaken(caster, GetSpellInfo(), damage, DOT, effIndex, GetBase()->GetStackAmount());

    bool crit = roll_chance_f(GetCritChance());
    if (crit)
        damage = GetCritAmount();

    // Calculate armor mitigation
    if (Unit::IsDamageReducedByArmor(GetSpellInfo()->GetSchoolMask(), GetSpellInfo(), (1 << effIndex)))
    {
        uint32 damageReductedArmor = caster->CalcArmorReducedDamage(target, damage, GetSpellInfo());
        cleanDamage.mitigated_damage += damage - damageReductedArmor;
        damage = damageReductedArmor;
    }

    int32 dmg = damage;

    GetBase()->CallScriptEffectChangeTickDamageHandlers(const_cast<AuraEffect const*>(this), dmg, target);

    caster->ApplyResilience(target, &dmg, crit);
    damage = dmg;
    caster->CalcAbsorbResist(target, GetSpellInfo()->GetSchoolMask(), DOT, damage, &absorb, &resist, m_spellInfo);

    if (target->GetHealth() < damage)
        damage = uint32(target->GetHealth());

    sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "PeriodicTick: %u (TypeId: %u) health leech of %u (TypeId: %u) for %u dmg inflicted by %u abs is %u",
        GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), damage, GetId(), absorb);

    caster->SendSpellNonMeleeDamageLog(target, GetId(), damage, GetSpellInfo()->GetSchoolMask(), absorb, resist, false, 0, crit);

    DamageInfo dmgInfoProc = DamageInfo(caster, target, damage, GetSpellInfo(), GetSpellInfo() ? SpellSchoolMask(GetSpellInfo()->SchoolMask) : SPELL_SCHOOL_MASK_NORMAL, SPELL_DIRECT_DAMAGE);
    dmgInfoProc.AbsorbDamage(absorb);
    dmgInfoProc.ResistDamage(resist);

    // Set trigger flag
    uint32 procAttacker = PROC_FLAG_DONE_PERIODIC;
    uint32 procVictim   = PROC_FLAG_TAKEN_PERIODIC;
    uint32 procEx = (crit ? PROC_EX_CRITICAL_HIT : PROC_EX_NORMAL_HIT) | PROC_EX_INTERNAL_DOT;
    damage = (damage <= absorb+resist) ? 0 : (damage-absorb-resist);

    if (damage)
        procVictim |= PROC_FLAG_TAKEN_DAMAGE;
    if (caster->isAlive())
        caster->ProcDamageAndSpell(target, procAttacker, procVictim, procEx, &dmgInfoProc, BASE_ATTACK, GetSpellInfo());
    int32 new_damage = caster->DealDamage(target, damage, &cleanDamage, DOT, GetSpellInfo()->GetSchoolMask(), GetSpellInfo(), false);
    if (caster->isAlive())
    {
        float gainMultiplier = GetSpellInfo()->GetEffect(GetEffIndex(), m_diffMode).CalcValueMultiplier(caster);

        uint32 heal = uint32(caster->SpellHealingBonusDone(caster, GetSpellInfo(), uint32(new_damage * gainMultiplier), DOT, effIndex, GetBase()->GetStackAmount()));
        heal = uint32(caster->SpellHealingBonusTaken(caster, GetSpellInfo(), heal, DOT, effIndex, GetBase()->GetStackAmount()));

        int32 gain = caster->HealBySpell(caster, GetSpellInfo(), heal);
        caster->getHostileRefManager().threatAssist(caster, gain * 0.5f, GetSpellInfo());
    }
}

void AuraEffect::HandlePeriodicHealthFunnelAuraTick(Unit* target, Unit* caster, SpellEffIndex /*effIndex*/) const
{
    if (!caster || !caster->isAlive() || !target->isAlive())
        return;

    if (target->HasUnitState(UNIT_STATE_ISOLATED))
    {
        SendTickImmune(target, caster);
        return;
    }

    int32 damage = std::max(GetAmount(), 0);
    // do not kill health donator
    if ((int32)caster->GetHealth() < damage)
        damage = caster->GetHealth() - 1;
    if (!damage)
        return;

    GetBase()->CallScriptEffectChangeTickDamageHandlers(const_cast<AuraEffect const*>(this), damage, target);

    caster->ModifyHealth(-(int32)damage);
    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "PeriodicTick: donator %u target %u damage %u.", caster->GetEntry(), target->GetEntry(), damage);

    float gainMultiplier = GetSpellInfo()->GetEffect(GetEffIndex(), m_diffMode).CalcValueMultiplier(caster);

    damage = int32(damage * gainMultiplier);

    caster->HealBySpell(target, GetSpellInfo(), damage);
}

void AuraEffect::HandlePeriodicHealAurasTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const
{
    if (!caster || !target->isAlive())
        return;

    if (target->HasUnitState(UNIT_STATE_ISOLATED))
    {
        SendTickImmune(target, caster);
        return;
    }

    // heal for caster damage (must be alive)
    if (target != caster && GetSpellInfo()->AttributesEx2 & SPELL_ATTR2_HEALTH_FUNNEL && !caster->isAlive())
        return;

    // don't regen when permanent aura target has full power
    if (GetBase()->IsPermanent() && target->IsFullHealth())
        return;

    // ignore negative values (can be result apply spellmods to aura damage
    int32 damage = std::max(m_amount, 0);

    // Fix Second Wind only in AURA_STATE_HEALTHLESS_35_PERCENT
    if (m_spellInfo->Id == 16491)
        if (caster->GetHealthPct() > 35.0f)
            return;

    bool crit = roll_chance_f(GetCritChance());
    if (crit)
        damage = GetCritAmount();

    if (GetAuraType() == SPELL_AURA_OBS_MOD_HEALTH)
    {
        // Taken mods
        float TakenTotalMod = 1.0f;

        // Tenacity increase healing % taken
       if (AuraEffect const* Tenacity = target->GetAuraEffect(58549, 0))
            AddPct(TakenTotalMod, Tenacity->GetAmount());

        // Healing taken percent
        float minval = (float)target->GetMaxNegativeAuraModifier(SPELL_AURA_MOD_HEALING_PCT);
        if (minval)
            AddPct(TakenTotalMod, minval);

        float maxval = (float)target->GetMaxPositiveAuraModifier(SPELL_AURA_MOD_HEALING_PCT);
        if (maxval)
            AddPct(TakenTotalMod, maxval);

        TakenTotalMod = std::max(TakenTotalMod, 0.0f);

        damage = uint32(target->CountPctFromMaxHealth(damage));
        damage = uint32(damage * TakenTotalMod);
        
        switch (GetSpellInfo()->Id)
        {
            case 136:
            {
                if(target->isPet())
                    if (Unit* owner = target->GetOwner())
                    {
                        if(owner->HasAura(19573))
                           owner->CastSpell(target, 24406, true);
                    }
                break;
            }
            case 114635:
            {
                // Mastery: Emberstorm
                if (AuraEffect const* aurEff = caster->GetAuraEffect(77220, EFFECT_0))
                    AddPct(damage, aurEff->GetAmount());
                break;
            }
            default:
                break;
        }
    }
    else
    {
        // Wild Growth = amount + (6 - 2*doneTicks) * ticks* amount / 100
        if (m_spellInfo->SpellFamilyName == SPELLFAMILY_DRUID && m_spellInfo->SpellIconID == 2864)
        {
            int32 addition = int32(float(damage * GetTotalTicks()) * ((6-float(2*(GetTickNumber()-1)))/100));

            // Item - Druid T10 Restoration 2P Bonus
            if (AuraEffect* aurEff = caster->GetAuraEffect(70658, 0))
                // divided by 50 instead of 100 because calculated as for every 2 tick
                addition += abs(int32((addition * aurEff->GetAmount()) / 50));

            damage += addition;
        }

        if (GetSpellInfo()->GetEffect(effIndex, m_diffMode).Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA)
        {
            float critChance = 0.0f;
            damage = caster->SpellHealingBonusDone(target, GetSpellInfo(), damage, DOT, effIndex, GetBase()->GetStackAmount());
            caster->isSpellCrit(target, GetSpellInfo(), GetSpellInfo()->GetSchoolMask(), BASE_ATTACK, critChance);
            crit = roll_chance_f(critChance);
            if (crit)
                damage = caster->SpellCriticalHealingBonus(GetSpellInfo(), damage, target);;
        }
        damage = target->SpellHealingBonusTaken(caster, GetSpellInfo(), damage, DOT, effIndex, GetBase()->GetStackAmount());
    }

    sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "PeriodicTick: %u (TypeId: %u) heal of %u (TypeId: %u) for %u health inflicted by %u",
        GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), damage, GetId());

    GetBase()->CallScriptEffectChangeTickDamageHandlers(const_cast<AuraEffect const*>(this), damage, target);

    uint32 absorb = 0;
    uint32 heal = uint32(damage);
    caster->CalcHealAbsorb(target, GetSpellInfo(), heal, absorb);
    int32 gain = caster->DealHeal(target, heal, GetSpellInfo());

    DamageInfo dmgInfoProc = DamageInfo(caster, target, damage, GetSpellInfo(), GetSpellInfo() ? SpellSchoolMask(GetSpellInfo()->SchoolMask) : SPELL_SCHOOL_MASK_NORMAL, SPELL_DIRECT_DAMAGE);
    dmgInfoProc.AbsorbDamage(absorb);

    SpellPeriodicAuraLogInfo pInfo(this, heal, heal - gain, absorb, 0, 0.0f, crit);
    target->SendPeriodicAuraLog(&pInfo);

    target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f, GetSpellInfo());

    bool haveCastItem = GetBase()->GetCastItemGUID() != 0;

    // Health Funnel
    // damage caster for heal amount
    if (target != caster && GetSpellInfo()->Id == 755)
    {
        uint32 funnelDamage = caster->CountPctFromMaxHealth(2);
        caster->DealDamage(caster, funnelDamage, NULL, NODAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
    }

    uint32 procAttacker = PROC_FLAG_DONE_PERIODIC;
    uint32 procVictim   = PROC_FLAG_TAKEN_PERIODIC;
    uint32 procEx = (crit ? PROC_EX_CRITICAL_HIT : PROC_EX_NORMAL_HIT) | PROC_EX_INTERNAL_HOT;
    // ignore item heals
    if (!haveCastItem)
        caster->ProcDamageAndSpell(target, procAttacker, procVictim, procEx, &dmgInfoProc, BASE_ATTACK, GetSpellInfo());
}

void AuraEffect::HandlePeriodicManaLeechAuraTick(Unit* target, Unit* caster, SpellEffIndex /*effIndex*/) const
{
    Powers powerType = Powers(GetMiscValue());

    if (!caster || !caster->isAlive() || !target->isAlive() || target->getPowerType() != powerType)
        return;

    if (target->HasUnitState(UNIT_STATE_ISOLATED) || target->IsImmunedToDamage(GetSpellInfo()))
    {
        SendTickImmune(target, caster);
        return;
    }

    if (GetSpellInfo()->GetEffect(GetEffIndex(), m_diffMode).Effect == SPELL_EFFECT_PERSISTENT_AREA_AURA &&
        caster->SpellHitResult(target, GetSpellInfo(), false, 1 << GetEffIndex()) != SPELL_MISS_NONE)
        return;

    // ignore negative values (can be result apply spellmods to aura damage
    int32 drainAmount = std::max(m_amount, 0);

    SpellPowerEntry power;
    if (!GetSpellInfo()->GetSpellPowerByCasterPower(GetCaster(), power))
        power = GetSpellInfo()->GetPowerInfo(0);

    // Special case: draining x% of mana (up to a maximum of 2*x% of the caster's maximum mana)
    // It's mana percent cost spells, m_amount is percent drain from target
    if (power.powerCostPercentage)
    {
        // max value
        int32 maxmana = CalculatePct(caster->GetMaxPower(powerType), drainAmount * 2.0f);
        ApplyPct(drainAmount, target->GetMaxPower(powerType));
        if (drainAmount > maxmana)
            drainAmount = maxmana;
    }

    GetBase()->CallScriptEffectChangeTickDamageHandlers(const_cast<AuraEffect const*>(this), drainAmount, target);

    sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "PeriodicTick: %u (TypeId: %u) power leech of %u (TypeId: %u) for %u dmg inflicted by %u",
        GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), drainAmount, GetId());

    int32 drainedAmount = -target->ModifyPower(powerType, -drainAmount, true);

    float gainMultiplier = GetSpellInfo()->GetEffect(GetEffIndex(), m_diffMode).CalcValueMultiplier(caster);

    SpellPeriodicAuraLogInfo pInfo(this, drainedAmount, 0, 0, 0, gainMultiplier, false);
    target->SendPeriodicAuraLog(&pInfo);

    int32 gainAmount = int32(drainedAmount * gainMultiplier);
    int32 gainedAmount = 0;
    if (gainAmount)
    {
        gainedAmount = caster->ModifyPower(powerType, gainAmount);
        target->AddThreat(caster, float(gainedAmount) * 0.5f, GetSpellInfo()->GetSchoolMask(), GetSpellInfo());
    }

    // Drain Mana
    if (m_spellInfo->SpellFamilyName == SPELLFAMILY_WARLOCK
        && m_spellInfo->SpellFamilyFlags[0] & 0x00000010)
    {
        int32 manaFeedVal = 0;
        if (AuraEffect const* aurEff = GetBase()->GetEffect(1))
            manaFeedVal = aurEff->GetAmount();
        // Mana Feed - Drain Mana
        if (manaFeedVal > 0)
        {
            int32 feedAmount = CalculatePct(gainedAmount, manaFeedVal);
            caster->CastCustomSpell(caster, 32554, &feedAmount, NULL, NULL, true, NULL, this);
        }
    }
}

void AuraEffect::HandleObsModPowerAuraTick(Unit* target, Unit* caster, SpellEffIndex /*effIndex*/) const
{
    Powers powerType;
    if (GetMiscValue() == POWER_ALL)
        powerType = target->getPowerType();
    else
        powerType = Powers(GetMiscValue());

    if (!target->isAlive() || !target->GetMaxPower(powerType))
        return;

    if (target->HasUnitState(UNIT_STATE_ISOLATED))
    {
        SendTickImmune(target, caster);
        return;
    }

    // don't regen when permanent aura target has full power
    if (GetBase()->IsPermanent() && target->GetPower(powerType) == target->GetMaxPower(powerType))
        return;

    // ignore negative values (can be result apply spellmods to aura damage
    int32 amount = std::max(m_amount, 0) * target->GetMaxPower(powerType) /100;

    GetBase()->CallScriptEffectChangeTickDamageHandlers(const_cast<AuraEffect const*>(this), amount, target);

    sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "PeriodicTick: %u (TypeId: %u) energize %u (TypeId: %u) for %u dmg inflicted by %u",
        GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), amount, GetId());

    SpellPeriodicAuraLogInfo pInfo(this, amount, 0, 0, 0, 0.0f, false);
    target->SendPeriodicAuraLog(&pInfo);

    int32 gain = target->ModifyPower(powerType, amount);

    if (caster)
        target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f, GetSpellInfo());
}

void AuraEffect::HandlePeriodicEnergizeAuraTick(Unit* target, Unit* caster, SpellEffIndex /*effIndex*/) const
{
    Powers powerType = Powers(GetMiscValue());

    if (target->GetTypeId() == TYPEID_PLAYER && target->getPowerType() != powerType && !(m_spellInfo->AttributesEx7 & SPELL_ATTR7_CAN_RESTORE_SECONDARY_POWER))
        return;

    if (!target->isAlive() || !target->GetMaxPower(powerType))
        return;

    if (target->HasUnitState(UNIT_STATE_ISOLATED))
    {
        SendTickImmune(target, caster);
        return;
    }

    // don't regen when permanent aura target has full power
    if (GetBase()->IsPermanent() && target->GetPower(powerType) == target->GetMaxPower(powerType))
        return;

    // ignore negative values (can be result apply spellmods to aura damage
    int32 amount = std::max(m_amount, 0);

    GetBase()->CallScriptEffectChangeTickDamageHandlers(const_cast<AuraEffect const*>(this), amount, target);

    SpellPeriodicAuraLogInfo pInfo(this, amount, 0, 0, 0, 0.0f, false);
    target->SendPeriodicAuraLog(&pInfo);

    sLog->outInfo(LOG_FILTER_SPELLS_AURAS, "PeriodicTick: %u (TypeId: %u) energize %u (TypeId: %u) for %u dmg inflicted by %u",
        GUID_LOPART(GetCasterGUID()), GuidHigh2TypeId(GUID_HIPART(GetCasterGUID())), target->GetGUIDLow(), target->GetTypeId(), amount, GetId());

    int32 gain = target->ModifyPower(powerType, amount);

    if (caster)
        target->getHostileRefManager().threatAssist(caster, float(gain) * 0.5f, GetSpellInfo());
}

void AuraEffect::HandlePeriodicPowerBurnAuraTick(Unit* target, Unit* caster, SpellEffIndex effIndex) const
{
    Powers powerType = Powers(GetMiscValue());

    if (!caster || !target->isAlive() || target->getPowerType() != powerType)
        return;

    if (target->HasUnitState(UNIT_STATE_ISOLATED) || target->IsImmunedToDamage(GetSpellInfo()))
    {
        SendTickImmune(target, caster);
        return;
    }

    // ignore negative values (can be result apply spellmods to aura damage
    int32 damage = std::max(m_amount, 0);

    GetBase()->CallScriptEffectChangeTickDamageHandlers(const_cast<AuraEffect const*>(this), damage, target);

    uint32 gain = uint32(-target->ModifyPower(powerType, -damage));

    float dmgMultiplier = GetSpellInfo()->GetEffect(GetEffIndex(), m_diffMode).CalcValueMultiplier(caster);

    SpellInfo const* spellProto = GetSpellInfo();
    // maybe has to be sent different to client, but not by SMSG_PERIODICAURALOG
    SpellNonMeleeDamage damageInfo(caster, target, spellProto->Id, spellProto->SchoolMask);
    // no SpellDamageBonus for burn mana
    caster->CalculateSpellDamageTaken(&damageInfo, int32(gain * dmgMultiplier), spellProto, (1 << effIndex));

    caster->DealDamageMods(damageInfo.target, damageInfo.damage, &damageInfo.absorb);

    caster->SendSpellNonMeleeDamageLog(&damageInfo);

    DamageInfo dmgInfoProc = DamageInfo(damageInfo);

    // Set trigger flag
    uint32 procAttacker = PROC_FLAG_DONE_PERIODIC;
    uint32 procVictim   = PROC_FLAG_TAKEN_PERIODIC;
    uint32 procEx       = createProcExtendMask(&damageInfo, SPELL_MISS_NONE) | PROC_EX_INTERNAL_DOT;
    if (damageInfo.damage)
        procVictim |= PROC_FLAG_TAKEN_DAMAGE;

    caster->ProcDamageAndSpell(damageInfo.target, procAttacker, procVictim, procEx, &dmgInfoProc, BASE_ATTACK, spellProto);

    caster->DealSpellDamage(&damageInfo, true);
}

void AuraEffect::HandleProcTriggerSpellAuraProc(AuraApplication* aurApp, ProcEventInfo& eventInfo, SpellEffIndex /*effIndex*/)
{
    Unit* triggerCaster = aurApp->GetTarget();
    Unit* triggerTarget = eventInfo.GetProcTarget();

    uint32 triggerSpellId = GetSpellInfo()->GetEffect(GetEffIndex(), m_diffMode).TriggerSpell;
    if (SpellInfo const* triggeredSpellInfo = sSpellMgr->GetSpellInfo(triggerSpellId))
    {
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AuraEffect::HandleProcTriggerSpellAuraProc: Triggering spell %u from aura %u proc", triggeredSpellInfo->Id, GetId());
        triggerCaster->CastSpell(triggerTarget, triggeredSpellInfo, true, NULL, this);
    }
    else
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS,"AuraEffect::HandleProcTriggerSpellAuraProc: Could not trigger spell %u from aura %u proc, because the spell does not have an entry in Spell.dbc.", triggerSpellId, GetId());
}

void AuraEffect::HandleProcTriggerSpellWithValueAuraProc(AuraApplication* aurApp, ProcEventInfo& eventInfo, SpellEffIndex /*effIndex*/)
{
    Unit* triggerCaster = aurApp->GetTarget();
    Unit* triggerTarget = eventInfo.GetProcTarget();

    uint32 triggerSpellId = GetSpellInfo()->GetEffect(m_effIndex, m_diffMode).TriggerSpell;
    if (SpellInfo const* triggeredSpellInfo = sSpellMgr->GetSpellInfo(triggerSpellId))
    {
        int32 basepoints0 = GetAmount();
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AuraEffect::HandleProcTriggerSpellWithValueAuraProc: Triggering spell %u with value %d from aura %u proc", triggeredSpellInfo->Id, basepoints0, GetId());
        triggerCaster->CastCustomSpell(triggerTarget, triggerSpellId, &basepoints0, NULL, NULL, true, NULL, this);
    }
    else
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS,"AuraEffect::HandleProcTriggerSpellWithValueAuraProc: Could not trigger spell %u from aura %u proc, because the spell does not have an entry in Spell.dbc.", triggerSpellId, GetId());
}

void AuraEffect::HandleProcTriggerDamageAuraProc(AuraApplication* aurApp, ProcEventInfo& eventInfo, SpellEffIndex effIndex)
{
    Unit* target = aurApp->GetTarget();
    Unit* triggerTarget = eventInfo.GetProcTarget();
    SpellNonMeleeDamage damageInfo(target, triggerTarget, GetId(), GetSpellInfo()->SchoolMask);
    uint32 damage = target->SpellDamageBonusDone(triggerTarget, GetSpellInfo(), GetAmount(), SPELL_DIRECT_DAMAGE, effIndex);
    damage = triggerTarget->SpellDamageBonusTaken(target, GetSpellInfo(), damage, SPELL_DIRECT_DAMAGE, effIndex);
    target->CalculateSpellDamageTaken(&damageInfo, damage, GetSpellInfo(), (1 << effIndex));
    target->DealDamageMods(damageInfo.target, damageInfo.damage, &damageInfo.absorb);
    target->SendSpellNonMeleeDamageLog(&damageInfo);
    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AuraEffect::HandleProcTriggerDamageAuraProc: Triggering %u spell damage from aura %u proc", damage, GetId());
    target->DealSpellDamage(&damageInfo, true);
}

void AuraEffect::HandleRaidProcFromChargeAuraProc(AuraApplication* aurApp, ProcEventInfo& /*eventInfo*/, SpellEffIndex /*effIndex*/)
{
    Unit* target = aurApp->GetTarget();

    uint32 triggerSpellId;
    switch (GetId())
    {
        case 57949:            // Shiver
            triggerSpellId = 57952;
            //animationSpellId = 57951; dummy effects for jump spell have unknown use (see also 41637)
            break;
        case 59978:            // Shiver
            triggerSpellId = 59979;
            break;
        case 43593:            // Cold Stare
            triggerSpellId = 43594;
            break;
        default:
            sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AuraEffect::HandleRaidProcFromChargeAuraProc: received not handled spell: %u", GetId());
            return;
    }

    int32 jumps = GetBase()->GetCharges();

    // current aura expire on proc finish
    GetBase()->SetCharges(0);
    GetBase()->SetUsingCharges(true);

    // next target selection
    if (jumps > 0)
    {
        if (Unit* caster = GetCaster())
        {
            float radius = GetSpellInfo()->GetEffect(GetEffIndex(), m_diffMode).CalcRadius(caster);

            if (Unit* triggerTarget = target->GetNextRandomRaidMemberOrPet(radius))
            {
                target->CastSpell(triggerTarget, GetSpellInfo(), true, NULL, this, GetCasterGUID());
                Aura* aura = triggerTarget->GetAura(GetId(), GetCasterGUID());
                if (aura != NULL)
                    aura->SetCharges(jumps);
            }
        }
    }

    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AuraEffect::HandleRaidProcFromChargeAuraProc: Triggering spell %u from aura %u proc", triggerSpellId, GetId());
    target->CastSpell(target, triggerSpellId, true, NULL, this, GetCasterGUID());
}


void AuraEffect::HandleRaidProcFromChargeWithValueAuraProc(AuraApplication* aurApp, ProcEventInfo& /*eventInfo*/, SpellEffIndex /*effIndex*/)
{
    Unit* target = aurApp->GetTarget();

    // Currently only Prayer of Mending
    if (!(GetSpellInfo()->SpellFamilyName == SPELLFAMILY_PRIEST && GetSpellInfo()->SpellFamilyFlags[1] & 0x20))
    {
        sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AuraEffect::HandleRaidProcFromChargeWithValueAuraProc: received not handled spell: %u", GetId());
        return;
    }
    uint32 triggerSpellId = 33110;

    int32 value = GetAmount();

    int32 jumps = GetBase()->GetCharges();

    // current aura expire on proc finish
    GetBase()->SetCharges(0);
    GetBase()->SetUsingCharges(true);

    // next target selection
    if (jumps > 0)
    {
        if (Unit* caster = GetCaster())
        {
            float radius = GetSpellInfo()->GetEffect(GetEffIndex(), m_diffMode).CalcRadius(caster);

            if (Unit* triggerTarget = target->GetNextRandomRaidMemberOrPet(radius))
            {
                target->CastCustomSpell(triggerTarget, GetId(), &value, NULL, NULL, true, NULL, this, GetCasterGUID());
                Aura* aura = triggerTarget->GetAura(GetId(), GetCasterGUID());
                if (aura != NULL)
                    aura->SetCharges(jumps);
            }
        }
    }

    sLog->outDebug(LOG_FILTER_SPELLS_AURAS, "AuraEffect::HandleRaidProcFromChargeWithValueAuraProc: Triggering spell %u from aura %u proc", triggerSpellId, GetId());
    target->CastCustomSpell(target, triggerSpellId, &value, NULL, NULL, true, NULL, this, GetCasterGUID());
}

void AuraEffect::HandleAuraForceWeather(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Player* target = aurApp->GetTarget()->ToPlayer();

    if (!target)
        return;

    if (apply)
    {
        WorldPacket data(SMSG_WEATHER, 4 + 4 + 1);

        data << uint32(GetMiscValue()) << 1.0f;
        data.WriteBit(false);
        target->GetSession()->SendPacket(&data);
    }
    else
    {
        // send weather for current zone
        if (Weather* weather = WeatherMgr::FindWeather(target->GetZoneId()))
            weather->SendWeatherUpdateToPlayer(target);
        else
        {
            if (!WeatherMgr::AddWeather(target->GetZoneId()))
            {
                // send fine weather packet to remove old weather
                WeatherMgr::SendFineWeatherUpdateToPlayer(target);
            }
        }
    }
}

void AuraEffect::HandleProgressBar(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Player* target = aurApp->GetTarget()->ToPlayer();

    if (!target)
        return;

    if (!apply)
    {
        target->SetMaxPower(POWER_ALTERNATE_POWER, 0);
        target->SetPower(POWER_ALTERNATE_POWER, 0);
        return;
    }

    uint32 startPower = 0;
    uint32 maxPower = 0;

    // Unknow max misc : 116

    switch (GetMiscValue())
    {
        case 80:
            maxPower = 3;
            break;
        case 32:
        case 89:
            maxPower = 4;
            break;
        case 30:
        case 34:
        case 90:
            maxPower = 5;
            break;
        case 33:
        case 35:
            maxPower = 7;
            break;
        case 88:
            maxPower = 10;
            break;
        case 117:
            maxPower = 25;
            break;
        case 129:
        case 133:
            maxPower = 30;
            break;
        case 29:
            maxPower = 34;
            break;
        case 114:
        case 203:
            maxPower = 40;
            break;
        case 64:
            maxPower = 50;
            break;
        case 84:
            maxPower = 60;
            break;
        case 137:
        case 149:
        case 195:
            maxPower = 90;
            break;
        case 23:
        case 37:
        case 39:
        case 61:
        case 65:
        case 68:
        case 69:
        case 72:
        case 78:
        case 91:
        case 93:
        case 94:
        case 103:
        case 107:
        case 110:
        case 113:
        case 134:
        case 148:
        case 151:
        case 165:
        case 176:
        case 178:
        case 179:
        case 183:
        case 199:
        case 204:
        case 205:
        case 206:
        case 207:
        default:
            maxPower = 100;
            break;
        case 258:
            maxPower = 101;
            break;
        case 63:
            maxPower = 105;
            break;
        case 87:
            maxPower = 120;
            break;
        case 66:
        case 67:
            maxPower = 180;
            break;
        case 24:
            maxPower = 250;
            break;
        case 26:
            maxPower = 300;
            break;
        case 158:
            maxPower = 700;
            break;
        case 36:
            maxPower = 35000;
            break;
    }

    switch (GetMiscValue())
    {
        case 89:
            startPower = 4;
            break;
        case 34:
        case 90:
        case 204:
        case 205:
        //case 206:
        case 207:
            startPower = 5;
            break;
        case 258:
            startPower = 75;
            break;
        case 103:
            startPower = 10;
            break;
        case 64:
            startPower = 25;
            break;
        case 87:
        case 93:
        case 148:
        case 151:
        case 176:
        case 183:
            startPower = 50;
            break;
        case 178:
            startPower = 100;
            break;
        default:
            startPower = 0;
            break;
    }

    target->SetMaxPower(POWER_ALTERNATE_POWER, maxPower);
    target->SetPower(POWER_ALTERNATE_POWER, startPower);
}

void AuraEffect::HandleAuraStrangulate(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();

    if (!target)
        return;

    // Asphyxiate
    if (m_spellInfo->Id == 108194)
    {
        int32 newZ = 1;
        target->SetControlled(apply, UNIT_STATE_STUNNED);

        if (apply)
            target->UpdateHeight(target->GetPositionZ() + newZ);
        else
            target->UpdateHeight(target->GetPositionZ() - newZ);
    }
}

void AuraEffect::HandleOverrideActionbarSpells(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();
    if (target->GetTypeId() != TYPEID_PLAYER)
        return;

    if (!apply)
    {
        Unit::AuraEffectList swaps = target->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS);
        Unit::AuraEffectList const& swaps2 = target->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS_2);
        if (!swaps2.empty())
            swaps.insert(swaps.end(), swaps2.begin(), swaps2.end());

        SpellInfo const* thisInfo = GetSpellInfo();

        // update auras that replace same spells that unapplying aura do
        for (Unit::AuraEffectList::const_iterator itr = swaps.begin(); itr != swaps.end(); ++itr)
        {
            SpellInfo const* otherInfo = (*itr)->GetSpellInfo();
            if (thisInfo->SpellFamilyName == otherInfo->SpellFamilyName)
            {
                bool updated = false;
                for (int i = 0; i < MAX_SPELL_EFFECTS; ++i)
                {
                    for (int j = 0; j < MAX_SPELL_EFFECTS; ++j)
                    {
                        if (thisInfo->Effects[i].SpellClassMask & otherInfo->Effects[j].SpellClassMask)
                        {
                            updated = true;
                            (*itr)->GetBase()->SetNeedClientUpdateForTargets();
                            break;
                        }
                    }
                    if (updated)
                        break;
                }
            }
        }
    }
}

void AuraEffect::HandleAuraModCategoryCooldown(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Player* target = aurApp->GetTarget()->ToPlayer();
    if (!target || target->GetSession()->PlayerLoading())
        return;

    target->SendCategoryCooldownMods();
}

void AuraEffect::HandleAuraSeeWhileInvisible(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Player* target = aurApp->GetTarget()->ToPlayer();
    if (!target)
        return;

    target->UpdateObjectVisibility();
}

void AuraEffect::HandleAuraMastery(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Player* target = aurApp->GetTarget()->ToPlayer();
    if (!target)
        return;

    target->UpdateMasteryAuras();
}

void AuraEffect::HandleAuraModCharges(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Player* target = aurApp->GetTarget()->ToPlayer();
    if (!target)
        return;

    target->RecalculateSpellCategoryCharges(GetMiscValue());
    target->SendSpellChargeData();
}

void AuraEffect::HandleBattlegroundFlag(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Player* target = aurApp->GetTarget()->ToPlayer();

    // when removing flag aura, handle flag drop
    if (!apply && target)
    {
        if (target->InBattleground())
        {
            if (Battleground* bg = target->GetBattleground())
                bg->EventPlayerDroppedFlag(target);
        }
        else
            sOutdoorPvPMgr->HandleDropFlag(target, GetSpellInfo()->Id);
    }
}

void AuraEffect::HandleCreateAreaTrigger(AuraApplication const* aurApp, uint8 mode, bool apply) const
{
    //Use custom summon at spell_norushen_residual_corruption id 145074
    if (!(mode & AURA_EFFECT_HANDLE_REAL))
        return;

    Unit* target = aurApp->GetTarget();
    uint32 triggerEntry = GetMiscValue();
    if (!triggerEntry || !target || !GetCaster() || GetCaster()->GetMap() != target->GetMap())
        return;

    // when removing flag aura, handle flag drop
    if (apply)
    {
        Position pos;
        target->GetPosition(&pos);

        AreaTrigger* areaTrigger = new AreaTrigger;
        if (!areaTrigger->CreateAreaTrigger(sObjectMgr->GenerateLowGuid(HIGHGUID_AREATRIGGER), triggerEntry, GetCaster(), GetSpellInfo(), pos, NULL, target->GetGUID()))
        {
            delete areaTrigger;
            return;
        }

        GetBase()->SetSpellAreaTrigger(areaTrigger->GetGUID());
    }
    else if(!apply)
    {
        if(uint64 areaTriggerGuid = GetBase()->GetSpellAreaTrigger())
            if(AreaTrigger* areaTrigger = ObjectAccessor::GetAreaTrigger(*GetCaster(), areaTriggerGuid))
                areaTrigger->SetDuration(0);
    }
}

