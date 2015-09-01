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

#ifndef TRINITY_DB2STORES_H
#define TRINITY_DB2STORES_H

#include "Common.h"
#include "DB2Store.h"
#include "DB2Structure.h"

#include <list>

std::list<uint32> GetPackageItemList(uint32 packageEntry);
std::list<uint32> GetGameObjectsList();

extern DB2Storage <ItemEntry>                     sItemStore;
extern DB2Storage <ItemCurrencyCostEntry>         sItemCurrencyCostStore;
extern DB2Storage <ItemExtendedCostEntry>         sItemExtendedCostStore;
extern DB2Storage <ItemSparseEntry>               sItemSparseStore;
extern DB2Storage <BattlePetAbilityEntry>         sBattlePetAbilityStore;
extern DB2Storage <BattlePetAbilityTurnEntry>     sBattlePetAbilityTurnStore;
extern DB2Storage <BattlePetAbilityEffectEntry>   sBattlePetAbilityEffectStore;
extern DB2Storage <BattlePetEffectPropertiesEntry>   sBattlePetEffectPropertiesStore;
extern DB2Storage <BattlePetAbilityStateEntry>    sBattlePetAbilityStateStore;
extern DB2Storage <BattlePetSpeciesEntry>         sBattlePetSpeciesStore;
extern DB2Storage <BattlePetSpeciesStateEntry>    sBattlePetSpeciesStateStore;
extern DB2Storage <BattlePetSpeciesXAbilityEntry> sBattlePetSpeciesXAbilityStore;
extern DB2Storage <BattlePetStateEntry>           sBattlePetStateStore;
extern DB2Storage <BattlePetBreedQualityEntry>    sBattlePetBreedQualityStore;
extern DB2Storage <BattlePetBreedStateEntry>      sBattlePetBreedStateStore;
extern DB2Storage <QuestPackageItem>              sQuestPackageItemStore;
extern DB2Storage <SpellReagentsEntry>            sSpellReagentsStore;
extern DB2Storage <ItemUpgradeEntry>              sItemUpgradeStore;
extern DB2Storage <RuleSetItemUpgradeEntry>       sRuleSetItemUpgradeEntryStore;
extern DB2Storage <GameObjectsEntry>              sGameObjectsStore;
extern DB2Storage <MapChallengeModeEntry>         sMapChallengeModeStore;
extern DB2Storage <SpellVisualEntry>              sSpellVisualStore;

void LoadDB2Stores(const std::string& dataPath);

typedef UNORDERED_MAP<uint32, ItemUpgradeData> ItemUpgradeDataMap;
ItemUpgradeData const* GetItemUpgradeData(uint32 itemEntry);
extern ItemUpgradeDataMap sItemUpgradeDataMap;

typedef UNORDERED_MAP<uint32, BattlePetSpeciesEntry const*> BattlePetSpeciesBySpellIdMap;
extern BattlePetSpeciesBySpellIdMap sBattlePetSpeciesBySpellId;

typedef std::multimap<uint32, std::pair<uint32, int32>> BattlePetBreedStateByBreedMap;
typedef std::pair<BattlePetBreedStateByBreedMap::const_iterator, BattlePetBreedStateByBreedMap::const_iterator> BattlePetBreedStateByBreedMapBounds;
extern BattlePetBreedStateByBreedMap sBattlePetBreedStateByBreedId;

typedef std::multimap<uint32, std::pair<uint32, int32>> BattlePetSpeciesStateBySpecMap;
typedef std::pair<BattlePetSpeciesStateBySpecMap::const_iterator, BattlePetSpeciesStateBySpecMap::const_iterator> BattlePetSpeciesStateBySpecMapBounds;
extern BattlePetSpeciesStateBySpecMap sBattlePetSpeciesStateBySpecId;

typedef UNORDERED_MAP<uint32, MapChallengeModeEntry const*> MapChallengeModeEntryMap;
extern MapChallengeModeEntryMap sMapChallengeModeEntrybyMap;

#endif