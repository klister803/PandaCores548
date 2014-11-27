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

#ifndef _UPDATEFIELDS_H
#define _UPDATEFIELDS_H

// Updatefields auto generated for version 5.4.1 17538
enum EObjectFields
{
    OBJECT_FIELD_GUID                                = 0x0,                 // Size: 0x2
    OBJECT_FIELD_DATA                                = 0x2,                 // Size: 0x2
    OBJECT_FIELD_TYPE                                = 0x4,                 // Size: 0x1
    OBJECT_FIELD_ENTRY                               = 0x5,                 // Size: 0x1
    OBJECT_FIELD_DYNAMIC_FLAGS                       = 0x6,                 // Size: 0x1
    OBJECT_FIELD_SCALE_X                             = 0x7,                 // Size: 0x1
    OBJECT_END                                       = 0x8
};

enum EUnitFields
{
    UNIT_FIELD_CHARM                                 = OBJECT_END + 0x0,    // Size: 0x2
    UNIT_FIELD_SUMMON                                = OBJECT_END + 0x2,    // Size: 0x2
    UNIT_FIELD_CRITTER                               = OBJECT_END + 0x4,    // Size: 0x2
    UNIT_FIELD_CHARMEDBY                             = OBJECT_END + 0x6,    // Size: 0x2
    UNIT_FIELD_SUMMONEDBY                            = OBJECT_END + 0x8,    // Size: 0x2
    UNIT_FIELD_CREATEDBY                             = OBJECT_END + 0xA,    // Size: 0x2
    UNIT_FIELD_DEMON_CREATOR                         = OBJECT_END + 0xC,    // Size: 0x2
    UNIT_FIELD_TARGET                                = OBJECT_END + 0xE,    // Size: 0x2
    UNIT_FIELD_BATTLE_PET_COMPANION_GUID             = OBJECT_END + 0x10,   // Size: 0x2
    UNIT_FIELD_CHANNEL_OBJECT                        = OBJECT_END + 0x12,   // Size: 0x2
    UNIT_CHANNEL_SPELL                               = OBJECT_END + 0x14,   // Size: 0x1
    UNIT_SUMMONED_BY_HOME_REALM                      = OBJECT_END + 0x15,   // Size: 0x1
    UNIT_FIELD_BYTES_0                               = OBJECT_END + 0x16,   // Size: 0x1
    UNIT_FIELD_DISPLAY_POWER                         = OBJECT_END + 0x17,   // Size: 0x1
    UNIT_FIELD_OVERRIDE_DISPLAY_POWER_ID             = OBJECT_END + 0x18,   // Size: 0x1
    UNIT_FIELD_HEALTH                                = OBJECT_END + 0x19,   // Size: 0x1
    UNIT_FIELD_POWER1                                = OBJECT_END + 0x1A,   // Size: 0x5
    UNIT_FIELD_MAXHEALTH                             = OBJECT_END + 0x1F,   // Size: 0x1
    UNIT_FIELD_MAXPOWER1                             = OBJECT_END + 0x20,   // Size: 0x5
    UNIT_FIELD_POWER_REGEN_FLAT_MODIFIER             = OBJECT_END + 0x25,   // Size: 0x5
    UNIT_FIELD_POWER_REGEN_INTERRUPTED_FLAT_MODIFIER = OBJECT_END + 0x2A,   // Size: 0x5
    UNIT_FIELD_LEVEL                                 = OBJECT_END + 0x2F,   // Size: 0x1
    UNIT_FIELD_EFFECTIVE_LEVEL                       = OBJECT_END + 0x30,   // Size: 0x1
    UNIT_FIELD_FACTIONTEMPLATE                       = OBJECT_END + 0x31,   // Size: 0x1
    UNIT_VIRTUAL_ITEM_SLOT_ID                        = OBJECT_END + 0x32,   // Size: 0x3
    UNIT_FIELD_FLAGS                                 = OBJECT_END + 0x35,   // Size: 0x1
    UNIT_FIELD_FLAGS_2                               = OBJECT_END + 0x36,   // Size: 0x1
    UNIT_FIELD_AURASTATE                             = OBJECT_END + 0x37,   // Size: 0x1
    UNIT_FIELD_BASEATTACKTIME                        = OBJECT_END + 0x38,   // Size: 0x2
    UNIT_FIELD_RANGEDATTACKTIME                      = OBJECT_END + 0x3A,   // Size: 0x1
    UNIT_FIELD_BOUNDINGRADIUS                        = OBJECT_END + 0x3B,   // Size: 0x1
    UNIT_FIELD_COMBATREACH                           = OBJECT_END + 0x3C,   // Size: 0x1
    UNIT_FIELD_DISPLAYID                             = OBJECT_END + 0x3D,   // Size: 0x1
    UNIT_FIELD_NATIVEDISPLAYID                       = OBJECT_END + 0x3E,   // Size: 0x1
    UNIT_FIELD_MOUNTDISPLAYID                        = OBJECT_END + 0x3F,   // Size: 0x1
    UNIT_FIELD_MINDAMAGE                             = OBJECT_END + 0x40,   // Size: 0x1
    UNIT_FIELD_MAXDAMAGE                             = OBJECT_END + 0x41,   // Size: 0x1
    UNIT_FIELD_MINOFFHANDDAMAGE                      = OBJECT_END + 0x42,   // Size: 0x1
    UNIT_FIELD_MAXOFFHANDDAMAGE                      = OBJECT_END + 0x43,   // Size: 0x1
    UNIT_FIELD_BYTES_1                               = OBJECT_END + 0x44,   // Size: 0x1
    UNIT_FIELD_PETNUMBER                             = OBJECT_END + 0x45,   // Size: 0x1
    UNIT_FIELD_PET_NAME_TIMESTAMP                    = OBJECT_END + 0x46,   // Size: 0x1
    UNIT_FIELD_PETEXPERIENCE                         = OBJECT_END + 0x47,   // Size: 0x1
    UNIT_FIELD_PETNEXTLEVELEXP                       = OBJECT_END + 0x48,   // Size: 0x1
    UNIT_MOD_CAST_SPEED                              = OBJECT_END + 0x49,   // Size: 0x1
    UNIT_MOD_CAST_HASTE                              = OBJECT_END + 0x4A,   // Size: 0x1
    UNIT_MOD_HASTE                                   = OBJECT_END + 0x4B,   // Size: 0x1
    UNIT_FIELD_MOD_RANGED_HASTE                      = OBJECT_END + 0x4C,   // Size: 0x1
    UNIT_MOD_HASTE_REGEN                             = OBJECT_END + 0x4D,   // Size: 0x1
    UNIT_CREATED_BY_SPELL                            = OBJECT_END + 0x4E,   // Size: 0x1
    UNIT_NPC_FLAGS                                   = OBJECT_END + 0x4F,   // Size: 0x1
    UNIT_NPC_FLAGS2                                  = OBJECT_END + 0x50,   // Size: 0x1
    UNIT_NPC_EMOTESTATE                              = OBJECT_END + 0x51,   // Size: 0x1
    UNIT_FIELD_STAT0                                 = OBJECT_END + 0x52,   // Size: 0x5
    UNIT_FIELD_POSSTAT0                              = OBJECT_END + 0x57,   // Size: 0x5
    UNIT_FIELD_NEGSTAT0                              = OBJECT_END + 0x5C,   // Size: 0x5
    UNIT_FIELD_RESISTANCES                           = OBJECT_END + 0x61,   // Size: 0x7
    UNIT_FIELD_RESISTANCEBUFFMODSPOSITIVE            = OBJECT_END + 0x68,   // Size: 0x7
    UNIT_FIELD_RESISTANCEBUFFMODSNEGATIVE            = OBJECT_END + 0x6F,   // Size: 0x7
    UNIT_FIELD_BASE_MANA                             = OBJECT_END + 0x76,   // Size: 0x1
    UNIT_FIELD_BASE_HEALTH                           = OBJECT_END + 0x77,   // Size: 0x1
    UNIT_FIELD_BYTES_2                               = OBJECT_END + 0x78,   // Size: 0x1
    UNIT_FIELD_ATTACK_POWER                          = OBJECT_END + 0x79,   // Size: 0x1
    UNIT_FIELD_ATTACK_POWER_MOD_POS                  = OBJECT_END + 0x7A,   // Size: 0x1
    UNIT_FIELD_ATTACK_POWER_MOD_NEG                  = OBJECT_END + 0x7B,   // Size: 0x1
    UNIT_FIELD_ATTACK_POWER_MULTIPLIER               = OBJECT_END + 0x7C,   // Size: 0x1
    UNIT_FIELD_RANGED_ATTACK_POWER                   = OBJECT_END + 0x7D,   // Size: 0x1
    UNIT_FIELD_RANGED_ATTACK_POWER_MOD_POS           = OBJECT_END + 0x7E,   // Size: 0x1
    UNIT_FIELD_RANGED_ATTACK_POWER_MOD_NEG           = OBJECT_END + 0x7F,   // Size: 0x1
    UNIT_FIELD_RANGED_ATTACK_POWER_MULTIPLIER        = OBJECT_END + 0x80,   // Size: 0x1
    UNIT_FIELD_MINRANGEDDAMAGE                       = OBJECT_END + 0x81,   // Size: 0x1
    UNIT_FIELD_MAXRANGEDDAMAGE                       = OBJECT_END + 0x82,   // Size: 0x1
    UNIT_FIELD_POWER_COST_MODIFIER                   = OBJECT_END + 0x83,   // Size: 0x7
    UNIT_FIELD_POWER_COST_MULTIPLIER                 = OBJECT_END + 0x8A,   // Size: 0x7
    UNIT_FIELD_MAXHEALTHMODIFIER                     = OBJECT_END + 0x91,   // Size: 0x1
    UNIT_FIELD_HOVERHEIGHT                           = OBJECT_END + 0x92,   // Size: 0x1
    UNIT_FIELD_MINITEMLEVEL                          = OBJECT_END + 0x93,   // Size: 0x1
    UNIT_FIELD_MAXITEMLEVEL                          = OBJECT_END + 0x94,   // Size: 0x1
    UNIT_FIELD_WILD_BATTLE_PET_LEVEL                 = OBJECT_END + 0x95,   // Size: 0x1
    UNIT_FIELD_BATTLE_PET_COMPANION_NAME_TIMESTAMP   = OBJECT_END + 0x96,   // Size: 0x1
    UNIT_FIELD_INTERACT_SPELL_ID                     = OBJECT_END + 0x97,   // Size: 0x1
    UNIT_END                                         = OBJECT_END + 0x98,
};

enum EItemFields
{
    ITEM_FIELD_OWNER                                 = OBJECT_END + 0x0,    // Size: 0x2
    ITEM_FIELD_CONTAINED                             = OBJECT_END + 0x2,    // Size: 0x2
    ITEM_FIELD_CREATOR                               = OBJECT_END + 0x4,    // Size: 0x2
    ITEM_FIELD_GIFTCREATOR                           = OBJECT_END + 0x6,    // Size: 0x2
    ITEM_FIELD_STACK_COUNT                           = OBJECT_END + 0x8,    // Size: 0x1
    ITEM_FIELD_DURATION                              = OBJECT_END + 0x9,    // Size: 0x1
    ITEM_FIELD_SPELL_CHARGES                         = OBJECT_END + 0xA,    // Size: 0x5
    ITEM_FIELD_FLAGS                                 = OBJECT_END + 0xF,    // Size: 0x1
    ITEM_FIELD_ENCHANTMENT_1_1                       = OBJECT_END + 0x10,   // Size: 0x27
    ITEM_FIELD_PROPERTY_SEED                         = OBJECT_END + 0x37,   // Size: 0x1
    ITEM_FIELD_RANDOM_PROPERTIES_ID                  = OBJECT_END + 0x38,   // Size: 0x1
    ITEM_FIELD_DURABILITY                            = OBJECT_END + 0x39,   // Size: 0x1
    ITEM_FIELD_MAXDURABILITY                         = OBJECT_END + 0x3A,   // Size: 0x1
    ITEM_FIELD_CREATE_PLAYED_TIME                    = OBJECT_END + 0x3B,   // Size: 0x1
    ITEM_FIELD_MODIFIERS_MASK                        = OBJECT_END + 0x3C,   // Size: 0x1
    ITEM_END                                         = OBJECT_END + 0x3D
};

enum EPlayerFields
{
    PLAYER_DUEL_ARBITER                              = UNIT_END + 0x0,      // Size: 0x2
    PLAYER_FLAGS                                     = UNIT_END + 0x2,      // Size: 0x1
    PLAYER_GUILDRANK                                 = UNIT_END + 0x3,      // Size: 0x1
    PLAYER_GUILDDELETE_DATE                          = UNIT_END + 0x4,      // Size: 0x1
    PLAYER_GUILDLEVEL                                = UNIT_END + 0x5,      // Size: 0x1
    PLAYER_BYTES                                     = UNIT_END + 0x6,      // Size: 0x1
    PLAYER_BYTES_2                                   = UNIT_END + 0x7,      // Size: 0x1
    PLAYER_BYTES_3                                   = UNIT_END + 0x8,      // Size: 0x1
    PLAYER_DUEL_TEAM                                 = UNIT_END + 0x9,      // Size: 0x1
    PLAYER_GUILD_TIMESTAMP                           = UNIT_END + 0xA,      // Size: 0x1
    PLAYER_QUEST_LOG_1_1                             = UNIT_END + 0xB,      // Size: 0x2EE
    PLAYER_VISIBLE_ITEM_1_ENTRYID                    = UNIT_END + 0x2F9,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_1_ENCHANTMENT                = UNIT_END + 0x2FA,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_2_ENTRYID                    = UNIT_END + 0x2FB,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_2_ENCHANTMENT                = UNIT_END + 0x2FC,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_3_ENTRYID                    = UNIT_END + 0x2FD,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_3_ENCHANTMENT                = UNIT_END + 0x2FE,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_4_ENTRYID                    = UNIT_END + 0x2FF,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_4_ENCHANTMENT                = UNIT_END + 0x300,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_5_ENTRYID                    = UNIT_END + 0x301,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_5_ENCHANTMENT                = UNIT_END + 0x302,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_6_ENTRYID                    = UNIT_END + 0x303,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_6_ENCHANTMENT                = UNIT_END + 0x304,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_7_ENTRYID                    = UNIT_END + 0x305,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_7_ENCHANTMENT                = UNIT_END + 0x306,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_8_ENTRYID                    = UNIT_END + 0x307,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_8_ENCHANTMENT                = UNIT_END + 0x308,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_9_ENTRYID                    = UNIT_END + 0x309,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_9_ENCHANTMENT                = UNIT_END + 0x30A,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_10_ENTRYID                   = UNIT_END + 0x30B,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_10_ENCHANTMENT               = UNIT_END + 0x30C,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_11_ENTRYID                   = UNIT_END + 0x30D,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_11_ENCHANTMENT               = UNIT_END + 0x30E,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_12_ENTRYID                   = UNIT_END + 0x30F,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_12_ENCHANTMENT               = UNIT_END + 0x310,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_13_ENTRYID                   = UNIT_END + 0x311,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_13_ENCHANTMENT               = UNIT_END + 0x312,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_14_ENTRYID                   = UNIT_END + 0x313,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_14_ENCHANTMENT               = UNIT_END + 0x314,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_15_ENTRYID                   = UNIT_END + 0x315,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_15_ENCHANTMENT               = UNIT_END + 0x316,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_16_ENTRYID                   = UNIT_END + 0x317,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_16_ENCHANTMENT               = UNIT_END + 0x318,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_17_ENTRYID                   = UNIT_END + 0x319,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_17_ENCHANTMENT               = UNIT_END + 0x31A,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_18_ENTRYID                   = UNIT_END + 0x31B,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_18_ENCHANTMENT               = UNIT_END + 0x31C,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_19_ENTRYID                   = UNIT_END + 0x31D,    // Size: 0x1
    PLAYER_VISIBLE_ITEM_19_ENCHANTMENT               = UNIT_END + 0x31E,    // Size: 0x1
    PLAYER_CHOSEN_TITLE                              = UNIT_END + 0x31F,    // Size: 0x1
    PLAYER_FAKE_INEBRIATION                          = UNIT_END + 0x320,    // Size: 0x1
    PLAYER_HOME_PLAYER_REALM                         = UNIT_END + 0x321,    // Size: 0x1
    PLAYER_CURRENT_SPEC_ID                           = UNIT_END + 0x322,    // Size: 0x1
    PLAYER_TAXI_MOUNT_ANIM_KIT_ID                    = UNIT_END + 0x323,    // Size: 0x1
    PLAYER_CURRENT_BATTLE_PET_BREED_QUALITY          = UNIT_END + 0x324,    // Size: 0x1
    PLAYER_FIELD_INV_SLOT_HEAD                       = UNIT_END + 0x325,    // Size: 0x94
    PLAYER_FIELD_VENDORBUYBACK_SLOT_1                = UNIT_END + 0x3B9,    // Size: 0x18
    PLAYER_FARSIGHT                                  = UNIT_END + 0x3D1,    // Size: 0x2
    PLAYER_FIELD_KNOWN_TITLES                        = UNIT_END + 0x3D3,    // Size: 0xA
    PLAYER_FIELD_COINAGE                             = UNIT_END + 0x3DD,    // Size: 0x2
    PLAYER_XP                                        = UNIT_END + 0x3DF,    // Size: 0x1
    PLAYER_NEXT_LEVEL_XP                             = UNIT_END + 0x3E0,    // Size: 0x1
    PLAYER_SKILL_LINEID_0                            = UNIT_END + 0x3E1,    // Size: 0x40
    PLAYER_SKILL_STEP_0                              = UNIT_END + 0x421,    // Size: 0x40
    PLAYER_SKILL_RANK_0                              = UNIT_END + 0x461,    // Size: 0x40
    PLAYER_SKILL_MODIFIER_0                          = UNIT_END + 0x4A1,    // Size: 0x40
    PLAYER_SKILL_MAX_RANK_0                          = UNIT_END + 0x4E1,    // Size: 0x40
    PLAYER_SKILL_TALENT_0                            = UNIT_END + 0x521,    // Size: 0x40
    PLAYER_SKILL_UNKNOWN                             = UNIT_END + 0x561,    // Size: 0x40
    PLAYER_CHARACTER_POINTS                          = UNIT_END + 0x5A1,    // Size: 0x1
    PLAYER_MAX_TALENT_TIERS                          = UNIT_END + 0x5A2,    // Size: 0x1
    PLAYER_TRACK_CREATURES                           = UNIT_END + 0x5A3,    // Size: 0x1
    PLAYER_TRACK_RESOURCES                           = UNIT_END + 0x5A4,    // Size: 0x1
    PLAYER_EXPERTISE                                 = UNIT_END + 0x5A5,    // Size: 0x1
    PLAYER_OFFHAND_EXPERTISE                         = UNIT_END + 0x5A6,    // Size: 0x1
    PLAYER_RANGED_EXPERTISE                          = UNIT_END + 0x5A7,    // Size: 0x1
    PLAYER_COMBAT_RATING_EXPERTISE                   = UNIT_END + 0x5A8,    // Size: 0x1
    PLAYER_BLOCK_PERCENTAGE                          = UNIT_END + 0x5A9,    // Size: 0x1
    PLAYER_DODGE_PERCENTAGE                          = UNIT_END + 0x5AA,    // Size: 0x1
    PLAYER_PARRY_PERCENTAGE                          = UNIT_END + 0x5AB,    // Size: 0x1
    PLAYER_CRIT_PERCENTAGE                           = UNIT_END + 0x5AC,    // Size: 0x1
    PLAYER_RANGED_CRIT_PERCENTAGE                    = UNIT_END + 0x5AD,    // Size: 0x1
    PLAYER_OFFHAND_CRIT_PERCENTAGE                   = UNIT_END + 0x5AE,    // Size: 0x1
    PLAYER_SPELL_CRIT_PERCENTAGE1                    = UNIT_END + 0x5AF,    // Size: 0x7
    PLAYER_SHIELD_BLOCK                              = UNIT_END + 0x5B6,    // Size: 0x1
    PLAYER_SHIELD_BLOCK_CRIT_PERCENTAGE              = UNIT_END + 0x5B7,    // Size: 0x1
    PLAYER_MASTERY                                   = UNIT_END + 0x5B8,    // Size: 0x1
    PLAYER_PVP_POWER_DAMAGE                          = UNIT_END + 0x5B9,    // Size: 0x1
    PLAYER_PVP_POWER_HEALING                         = UNIT_END + 0x5BA,    // Size: 0x1
    PLAYER_EXPLORED_ZONES_1                          = UNIT_END + 0x5BB,    // Size: 0xC8
    PLAYER_REST_STATE_EXPERIENCE                     = UNIT_END + 0x683,    // Size: 0x1
    PLAYER_FIELD_MOD_DAMAGE_DONE_POS                 = UNIT_END + 0x684,    // Size: 0x7
    PLAYER_FIELD_MOD_DAMAGE_DONE_NEG                 = UNIT_END + 0x68B,    // Size: 0x7
    PLAYER_FIELD_MOD_DAMAGE_DONE_PCT                 = UNIT_END + 0x692,    // Size: 0x7
    PLAYER_FIELD_MOD_HEALING_DONE_POS                = UNIT_END + 0x699,    // Size: 0x1
    PLAYER_FIELD_MOD_HEALING_PCT                     = UNIT_END + 0x69A,    // Size: 0x1
    PLAYER_FIELD_MOD_HEALING_DONE_PCT                = UNIT_END + 0x69B,    // Size: 0x1
    PLAYER_FIELD_MOD_PERIODIC_HEALING_DONE_PCT       = UNIT_END + 0x69C,    // Size: 0x1
    PLAYER_FIELD_WEAPON_DMG_MULTIPLIERS              = UNIT_END + 0x69D,    // Size: 0x3
    PLAYER_FIELD_MOD_SPELL_POWER_PCT                 = UNIT_END + 0x6A0,    // Size: 0x1
    PLAYER_FIELD_MOD_RESILIENCE_PCT                  = UNIT_END + 0x6A1,    // Size: 0x1
    PLAYER_FIELD_OVERRIDE_SPELL_POWER_BY_AP_PCT      = UNIT_END + 0x6A2,    // Size: 0x1
    PLAYER_FIELD_OVERRIDE_AP_BY_SPELL_POWER_PCT      = UNIT_END + 0x6A3,    // Size: 0x1
    PLAYER_FIELD_MOD_TARGET_RESISTANCE               = UNIT_END + 0x6A4,    // Size: 0x1
    PLAYER_FIELD_MOD_TARGET_PHYSICAL_RESISTANCE      = UNIT_END + 0x6A5,    // Size: 0x1
    PLAYER_FIELD_BYTES                               = UNIT_END + 0x6A6,    // Size: 0x1
    PLAYER_SELF_RES_SPELL                            = UNIT_END + 0x6A7,    // Size: 0x1
    PLAYER_FIELD_PVP_MEDALS                          = UNIT_END + 0x6A8,    // Size: 0x1
    PLAYER_FIELD_BUYBACK_PRICE_1                     = UNIT_END + 0x6A9,    // Size: 0xC
    PLAYER_FIELD_BUYBACK_TIMESTAMP_1                 = UNIT_END + 0x6B5,    // Size: 0xC
    PLAYER_FIELD_KILLS                               = UNIT_END + 0x6C1,    // Size: 0x1
    PLAYER_FIELD_LIFETIME_HONORABLE_KILLS            = UNIT_END + 0x6C2,    // Size: 0x1
    PLAYER_FIELD_WATCHED_FACTION_INDEX               = UNIT_END + 0x6C3,    // Size: 0x1
    PLAYER_FIELD_COMBAT_RATING_1                     = UNIT_END + 0x6C4,    // Size: 0x1B
    PLAYER_FIELD_ARENA_TEAM_INFO_1_1                 = UNIT_END + 0x6DF,    // Size: 0x18
    PLAYER_FIELD_MAX_LEVEL                           = UNIT_END + 0x6F7,    // Size: 0x1
    PLAYER_RUNE_REGEN_1                              = UNIT_END + 0x6F8,    // Size: 0x4
    PLAYER_NO_REAGENT_COST_1                         = UNIT_END + 0x6FC,    // Size: 0x4
    PLAYER_FIELD_GLYPH_SLOTS_1                       = UNIT_END + 0x700,    // Size: 0x6
    PLAYER_FIELD_GLYPHS_1                            = UNIT_END + 0x706,    // Size: 0x6
    PLAYER_GLYPHS_ENABLED                            = UNIT_END + 0x70C,    // Size: 0x1
    PLAYER_PET_SPELL_POWER                           = UNIT_END + 0x70D,    // Size: 0x1
    PLAYER_FIELD_RESEARCHING_1                       = UNIT_END + 0x70E,    // Size: 0x8
    PLAYER_PROFESSION_SKILL_LINE_1                   = UNIT_END + 0x716,    // Size: 0x2
    PLAYER_FIELD_UI_HIT_MODIFIER                     = UNIT_END + 0x718,    // Size: 0x1
    PLAYER_FIELD_UI_SPELL_HIT_MODIFIER               = UNIT_END + 0x719,    // Size: 0x1
    PLAYER_FIELD_HOME_REALM_TIME_OFFSET              = UNIT_END + 0x71A,    // Size: 0x1
    PLAYER_FIELD_MOD_PET_HASTE                       = UNIT_END + 0x71B,    // Size: 0x1
    PLAYER_FIELD_SUMMONED_BATTLE_PET_GUID            = UNIT_END + 0x71C,    // Size: 0x2
    PLAYER_FIELD_OVERRIDE_SPELLS_ID                  = UNIT_END + 0x71E,    // Size: 0x1
    PLAYER_FIELD_LFG_BONUS_FACTION                   = UNIT_END + 0x71F,    // Size: 0x1
    PLAYER_FIELD_LOOT_SPEC_ID                        = UNIT_END + 0x720,    // Size: 0x1
    PLAYER_FIELD_OVERRIDE_ZONE_PVP_TYPE              = UNIT_END + 0x721,    // Size: 0x1
    PLAYER_FIELD_ITEM_LEVEL_DELTA                    = UNIT_END + 0x722,    // Size: 0x1
    PLAYER_END                                       = UNIT_END + 0x723
};

enum EObjectDynamicFields
{
    OBJECT_DYNAMIC_END                               = 0x0,
};

enum EUnitDynamicFields
{
    UNIT_DYNAMIC_PASSIVE_SPELLS                      = OBJECT_DYNAMIC_END + 0x0, // Size: 0x1
    UNIT_DYNAMIC_WORLD_EFFECTS                       = OBJECT_DYNAMIC_END + 0x1, // Size: 0x1
    UNIT_DYNAMIC_END                                 = OBJECT_DYNAMIC_END + 0x2,
};

enum EPlayerDynamicFields
{
    PLAYER_DYNAMIC_RESEARCH_SITES                    = UNIT_DYNAMIC_END + 0x0, // Size: 0x1
    PLAYER_DYNAMIC_RESEARCH_SITE_PROGRESS            = UNIT_DYNAMIC_END + 0x1, // Size: 0x1
    PLAYER_DYNAMIC_DAILY_QUESTS_COMPLETED            = UNIT_DYNAMIC_END + 0x2, // Size: 0x1
    PLAYER_DYNAMIC_END                               = UNIT_DYNAMIC_END + 0x3
};

enum EItemDynamicFields
{
    ITEM_DYNAMIC_MODIFIERS                           = OBJECT_DYNAMIC_END + 0x0, // Size: 0x1
    ITEM_DYNAMIC_END                                 = OBJECT_DYNAMIC_END + 0x1
};

enum ItemDynamicModifiersOffset
{
    ITEM_DYN_MOD_1                                   = 0,
    ITEM_DYN_MOD_2                                   = 1,
    ITEM_DYN_MOD_3                                   = 2,
    ITEM_DYN_MOD_4                                   = 3,
    ITEM_DYN_MOD_END                                 = 4,
};

enum EContainerFields
{
    CONTAINER_FIELD_SLOT_1                           = ITEM_END + 0x0,      // Size: 0x48
    CONTAINER_FIELD_NUM_SLOTS                        = ITEM_END + 0x48,     // Size: 0x1
    CONTAINER_END                                    = ITEM_END + 0x49,
};

enum EGameObjectFields
{
    GAMEOBJECT_FIELD_CREATED_BY                      = OBJECT_END + 0x0,    // Size: 0x2
    GAMEOBJECT_DISPLAYID                             = OBJECT_END + 0x2,    // Size: 0x1
    GAMEOBJECT_FLAGS                                 = OBJECT_END + 0x3,    // Size: 0x1
    GAMEOBJECT_PARENTROTATION                        = OBJECT_END + 0x4,    // Size: 0x4
    GAMEOBJECT_FACTION                               = OBJECT_END + 0x8,    // Size: 0x1
    GAMEOBJECT_LEVEL                                 = OBJECT_END + 0x9,    // Size: 0x1
    GAMEOBJECT_BYTES_1                               = OBJECT_END + 0xA,    // Size: 0x1
    GAMEOBJECT_DYNAMIC                               = OBJECT_END + 0xB,    // Size: 0x1
    GAMEOBJECT_END                                   = OBJECT_END + 0xC
};

enum EDynamicObjectFields
{
    DYNAMICOBJECT_CASTER                             = OBJECT_END + 0x0,    // Size: 0x2
    DYNAMICOBJECT_BYTES                              = OBJECT_END + 0x2,    // Size: 0x1
    DYNAMICOBJECT_SPELLID                            = OBJECT_END + 0x3,    // Size: 0x1
    DYNAMICOBJECT_RADIUS                             = OBJECT_END + 0x4,    // Size: 0x1
    DYNAMICOBJECT_CASTTIME                           = OBJECT_END + 0x5,    // Size: 0x1
    DYNAMICOBJECT_END                                = OBJECT_END + 0x6
};

enum ECorpseFields
{
    CORPSE_FIELD_OWNER                               = OBJECT_END + 0x0,    // Size: 0x2
    CORPSE_FIELD_PARTY                               = OBJECT_END + 0x2,    // Size: 0x2
    CORPSE_FIELD_DISPLAY_ID                          = OBJECT_END + 0x4,    // Size: 0x1
    CORPSE_FIELD_ITEM                                = OBJECT_END + 0x5,    // Size: 0x13
    CORPSE_FIELD_BYTES_1                             = OBJECT_END + 0x18,   // Size: 0x1
    CORPSE_FIELD_BYTES_2                             = OBJECT_END + 0x19,   // Size: 0x1
    CORPSE_FIELD_FLAGS                               = OBJECT_END + 0x1A,   // Size: 0x1
    CORPSE_FIELD_DYNAMIC_FLAGS                       = OBJECT_END + 0x1B,   // Size: 0x1
    CORPSE_END                                       = OBJECT_END + 0x1C
};

enum EAreaTriggerFields
{
    AREATRIGGER_CASTER                               = OBJECT_END + 0x0,    // Size: 0x2
    AREATRIGGER_DURATION                             = OBJECT_END + 0x2,    // Size: 0x1
    AREATRIGGER_SPELLID                              = OBJECT_END + 0x3,    // Size: 0x1
    AREATRIGGER_SPELLVISUALID                        = OBJECT_END + 0x4,    // Size: 0x1
    AREATRIGGER_EXPLICIT_SCALE                       = OBJECT_END + 0x5,    // Size: 0x1
    AREATRIGGER_END                                  = OBJECT_END + 0x6
};

enum ESceneObjectFields
{
    SCENE_SCRIPT_PACKAGE_ID                          = OBJECT_END + 0x0,    // Size: 0x1
    SCENE_RND_SEED_VAL                               = OBJECT_END + 0x1,    // Size: 0x1
    SCENE_CREATED_BY                                 = OBJECT_END + 0x2,    // Size: 0x2
    SCENE_SCENE_TYPE                                 = OBJECT_END + 0x4,    // Size: 0x1
    SCENE_END                                        = OBJECT_END + 0x5
};

#define PLAYER_END_NOT_SELF PLAYER_FIELD_INV_SLOT_HEAD

#endif // _UPDATEFIELDS_H
