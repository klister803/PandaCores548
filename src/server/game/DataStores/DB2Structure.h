/*
 * Copyright (C) 2011 TrintiyCore <http://www.trinitycore.org/>
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

#ifndef TRINITY_DB2STRUCTURE_H
#define TRINITY_DB2STRUCTURE_H

#include "Common.h"
#include "DBCEnums.h"
#include "Define.h"
#include "Path.h"
#include "Util.h"
//#include "Vehicle.h"
#include "SharedDefines.h"
#include "ItemPrototype.h"

#include <map>
#include <set>
#include <vector>

// GCC has alternative #pragma pack(N) syntax and old gcc version does not support pack(push, N), also any gcc version does not support it at some platform
#if defined(__GNUC__)
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif

// Structures used to access raw DB2 data and required packing to portability
struct ItemEntry
{
    uint32   ID;                                             // 0
    uint32   Class;                                          // 1
    uint32   SubClass;                                       // 2
    int32    SoundOverrideSubclass;                          // 3
    int32    Material;                                       // 4
    uint32   DisplayId;                                      // 5
    uint32   InventoryType;                                  // 6
    uint32   Sheath;                                         // 7
};

struct ItemCurrencyCostEntry
{
    //uint32  Id;
    uint32  ItemId;
};

struct ItemSparseEntry
{
    uint32     ID;                                           // 0
    uint32     Quality;                                      // 1
    uint32     Flags;                                        // 2
    uint32     Flags2;                                       // 3
    uint32     Flags3;
    float      Unk430_1;
    float      Unk430_2;
    uint32     BuyCount;
    uint32     BuyPrice;                                     // 4
    uint32     SellPrice;                                    // 5
    uint32     InventoryType;                                // 6
    int32      AllowableClass;                               // 7
    int32      AllowableRace;                                // 8
    int32      ItemLevel;                                    // 9
    int32      RequiredLevel;                                // 10
    uint32     RequiredSkill;                                // 11
    uint32     RequiredSkillRank;                            // 12
    uint32     RequiredSpell;                                // 13
    uint32     RequiredHonorRank;                            // 14
    uint32     RequiredCityRank;                             // 15
    uint32     RequiredReputationFaction;                    // 16
    uint32     RequiredReputationRank;                       // 17
    uint32     MaxCount;                                     // 18
    uint32     Stackable;                                    // 19
    uint32     ContainerSlots;                               // 20
    int32      ItemStatType[MAX_ITEM_PROTO_STATS];           // 21 - 30
    int32      ItemStatValue[MAX_ITEM_PROTO_STATS];          // 31 - 40
    int32      ItemStatUnk1[MAX_ITEM_PROTO_STATS];           // 41 - 50
    float      ItemStatUnk2[MAX_ITEM_PROTO_STATS];           // 51 - 60
    uint32     ScalingStatDistribution;                      // 61
    uint32     DamageType;                                   // 62
    uint32     Delay;                                        // 63
    float      RangedModRange;                               // 64
    int32      SpellId[MAX_ITEM_PROTO_SPELLS];               // 65 - 69
    int32      SpellTrigger[MAX_ITEM_PROTO_SPELLS];          // 70 - 74
    int32      SpellCharges[MAX_ITEM_PROTO_SPELLS];          // 75 - 79
    int32      SpellCooldown[MAX_ITEM_PROTO_SPELLS];         // 80 - 84
    int32      SpellCategory[MAX_ITEM_PROTO_SPELLS];         // 85 - 89
    int32      SpellCategoryCooldown[MAX_ITEM_PROTO_SPELLS]; // 90 - 94
    uint32     Bonding;                                      // 95
    char*      Name;                                         // 96
    char*      Name2;                                        // 97
    char*      Name3;                                        // 98
    char*      Name4;                                        // 99
    char*      Description;                                  // 100
    uint32     PageText;                                     // 101
    uint32     LanguageID;                                   // 102
    uint32     PageMaterial;                                 // 103
    uint32     StartQuest;                                   // 104
    uint32     LockID;                                       // 105
    int32      Material;                                     // 106
    uint32     Sheath;                                       // 107
    uint32     RandomProperty;                               // 108
    uint32     RandomSuffix;                                 // 109
    uint32     ItemSet;                                      // 110
    uint32     Area;                                         // 112
    uint32     Map;                                          // 113
    uint32     BagFamily;                                    // 114
    uint32     TotemCategory;                                // 115
    uint32     Color[MAX_ITEM_PROTO_SOCKETS];                // 116 - 118
    uint32     Content[MAX_ITEM_PROTO_SOCKETS];              // 119 - 121
    int32      SocketBonus;                                  // 122
    uint32     GemProperties;                                // 123
    float      ArmorDamageModifier;                          // 124
    uint32     Duration;                                     // 125
    uint32     ItemLimitCategory;                            // 126
    uint32     HolidayId;                                    // 127
    float      StatScalingFactor;                            // 128
    int32      CurrencySubstitutionId;                       // 129
    int32      CurrencySubstitutionCount;                    // 130
};

#define MAX_ITEM_EXT_COST_ITEMS         5
#define MAX_ITEM_EXT_COST_CURRENCIES    5

struct ItemExtendedCostEntry
{
    uint32      ID;                                         // 0 extended-cost entry id
    //uint32    reqhonorpoints;                             // 1 required honor points
    //uint32    reqarenapoints;                             // 2 required arena points
    uint32      RequiredArenaSlot;                          // 3 arena slot restrictions (min slot value)
    uint32      RequiredItem[MAX_ITEM_EXT_COST_ITEMS];      // 4-8 required item id
    uint32      RequiredItemCount[MAX_ITEM_EXT_COST_ITEMS]; // 9-13 required count of 1st item
    uint32      RequiredPersonalArenaRating;                // 14 required personal arena rating
    //uint32    ItemPurchaseGroup;                          // 15
    uint32      RequiredCurrency[MAX_ITEM_EXT_COST_CURRENCIES];// 16-20 required curency id
    uint32      RequiredCurrencyCount[MAX_ITEM_EXT_COST_CURRENCIES];// 21-25 required curency count
                                                            // 26       reputation-related
                                                            // 27       reputation-related
    uint32      flags;                                      // 28
    //                                                      // 29
    //                                                      // 30

    bool IsSeasonCurrencyRequirement(uint32 i) const
    {
        if(i > MAX_ITEM_EXT_COST_CURRENCIES)
            return 0;

        // start from ITEM_EXTENDED_COST_FLAG_SEASON_IN_INDEX_0
        return flags & (1 << (i + 1));
    }
};

struct BattlePetAbility
{
    uint32 ID;                  // 0
    uint32 Type;                // 1
    //uint32 someFlags;         // 2
    //uint32 unk;               // 3
    //uint32 unk;               // 4
    //uint32 unk;               // 5
    //string name               // 6
    //string description        // 7
};

struct BattlePetAbilityEffect
{
    uint32 ID;                  // 0
    uint32 AbilityID;           // 1
    //uint32 unk;               // 2
    //uint32 AuraID?;           // 3
    //uint32 unk;               // 4
    //uint32 effectNumber;      // 5
    //uint32 unk;               // 6
    //uint32 hitChance;         // 7
    //uint32 unk;               // 8
    //uint32 unk;               // 9
    //uint32 unk;               // 10
    //uint32 unk;               // 11
};

struct BattlePetAbilityState
{
    uint32 EffectID;            // 0
    uint32 AbilityID;           // 1
    uint32 stateID;             // 2
    //uint32 unk;               // 3
};

struct BattlePetState
{
    uint32 ID;                  // 0
    //uint32 unk;               // 1
    //string name;              // 2
    //uint32 unk;               // 3
};

struct BattlePetSpeciesEntry
{
    uint32 ID;                  // 0
    uint32 CreatureEntry;       // 1
    //uint32 fileDataEntry;     // 2
    uint32 spellId;             // 3
    //uint32 petType;           // 4
    uint32 source;              // 5
    uint32 flags;               // 6
    //string                    // 7
    //string                    // 8
};

struct QuestPackageItem
{
    uint32 ID;
    uint32 packageEntry;
    uint32 ItemID;
    uint32 count;
    //uint32 unk2;
};

#define MAX_SPELL_REAGENTS 8

// SpellReagents.dbc
struct SpellReagentsEntry
{
    //uint32    Id;                                           // 0        m_ID
    int32     Reagent[MAX_SPELL_REAGENTS];                  // 1-8      m_reagent
    uint32    ReagentCount[MAX_SPELL_REAGENTS];             // 9-16     m_reagentCount
    uint32    ReagentCurrency;                              // 17
    uint32    ReagentCurrencyCount;                         // 18
};

struct ItemUpgradeEntry
{
    uint32 id;                  // 0 rules id from RuleSetItemUpgradeEntry startUpgrade
    uint32 itemUpgradePathId;   // 1 extended-cost entry id
    uint32 levelBonus;          // 2 total level bonus related to non-upgraded item
    uint32 prevUpgradeId;       // 3
    uint32 currencyReqId;       // 4 currency Id
    uint32 currencyReqAmt;      // 5 currency count
};

struct RuleSetItemUpgradeEntry
{
    uint32 id;                  // 0 m_ID
    uint32 levelUpd;            // 1 level upgrade
    uint32 startUpgrade;        // 2 start update rules for ItemUpgradeEntry
    uint32 itemEntry;           // 3 Item ID
};

struct GameObjectsEntry
{
    uint32 id;                  // 0 m_ID
    uint32 map;                 // 1 mapId
    uint32 displayId;           // 2
    float position_x;           // 3
    float position_y;           // 4
    float position_z;           // 5
    float rotation0;            // 6
    float rotation1;            // 7
    float rotation2;            // 8
    float rotation3;            // 9
    float size;                 // 10
    uint32 type;                // 11
    uint32 data0;               // 12
    uint32 data1;               // 13
    uint32 data2;               // 14
    uint32 data3;               // 15
    char* name;                 // 16
};

#define MAX_ITEM_UPDGRADES 3

struct ItemUpgradeData
{
    ItemUpgradeData()
    {
        memset(upgrade, 0, sizeof(upgrade));
    }

    ItemUpgradeEntry const* upgrade[MAX_ITEM_UPDGRADES];
};

struct MapChallengeModeEntry
{
    uint32 id;                  // 0
    uint32 map;                 // 1
    uint32 unk1;                // 2
    uint32 unk2;                // 3
    uint32 season;              // 4
    uint32 bronze;              // 5
    uint32 silver;              // 6
    uint32 gold;                // 7
    uint32 unk3;                // 8
    uint32 unk4;                // 9
};

struct SpellVisualEntry
{
    uint32 ID;                  // 0
    //uint32 unk;               // 1
    //uint32 unk;               // 2
    //uint32 unk?;              // 3
    //uint32 unk;               // 4
    //uint32 unk;               // 5
    //uint32 unk;               // 6
    //uint32 unk;               // 7
    //uint32 unk;               // 8
    //uint32 unk;               // 9
    //uint32 unk;               // 10
    //uint32 unk;               // 11
    //uint32 unk;               // 12
    //uint32 unk;               // 13
    //uint32 unk;               // 14
    //uint32 unk;               // 15
    //uint32 unk;               // 16
    //uint32 unk;               // 17
    //uint32 unk;               // 18
    //float unk;                // 19
    //float unk;                // 20
    //float unk;                // 21
    //float unk;                // 22
    //float unk;                // 23
    //float unk;                // 24
    uint32 hostileId;           // 25
    //uint32 unk;               // 26
    //uint32 unk;               // 27
    //uint32 unk;               // 28
    //uint32 unk;               // 29
};

// GCC has alternative #pragma pack(N) syntax and old gcc version does not support pack(push, N), also any gcc version does not support it at some platform
#if defined(__GNUC__)
#pragma pack()
#else
#pragma pack(pop)
#endif

#endif