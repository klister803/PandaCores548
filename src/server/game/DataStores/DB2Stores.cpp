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

#include "DB2Stores.h"
#include "Log.h"
#include "SharedDefines.h"
#include "SpellMgr.h"
#include "DB2fmt.h"
#include <iostream>
#include <fstream>

#include <map>

DB2Storage <ItemEntry> sItemStore(Itemfmt);
DB2Storage <ItemCurrencyCostEntry> sItemCurrencyCostStore(ItemCurrencyCostfmt);
DB2Storage <ItemExtendedCostEntry> sItemExtendedCostStore(ItemExtendedCostEntryfmt);
DB2Storage <ItemSparseEntry> sItemSparseStore (ItemSparsefmt);
DB2Storage <BattlePetAbilityEntry> sBattlePetAbilityStore(BattlePetAbilityEntryfmt);
DB2Storage <BattlePetAbilityTurnEntry> sBattlePetAbilityTurnStore(BattlePetAbilityTurnEntryfmt);
DB2Storage <BattlePetAbilityEffectEntry> sBattlePetAbilityEffectStore(BattlePetAbilityEffectEntryfmt);
DB2Storage <BattlePetEffectPropertiesEntry> sBattlePetEffectPropertiesStore(BattlePetEffectPropertiesEntryfmt);
DB2Storage <BattlePetAbilityStateEntry> sBattlePetAbilityStateStore(BattlePetAbilityStateEntryfmt);
DB2Storage <BattlePetSpeciesEntry> sBattlePetSpeciesStore(BattlePetSpeciesEntryfmt);
DB2Storage <BattlePetSpeciesStateEntry> sBattlePetSpeciesStateStore(BattlePetSpeciesStateEntryfmt);
DB2Storage <BattlePetSpeciesXAbilityEntry> sBattlePetSpeciesXAbilityStore(BattlePetSpeciesXAbilityEntryfmt);
DB2Storage <BattlePetStateEntry> sBattlePetStateStore(BattlePetStateEntryfmt);
DB2Storage <BattlePetBreedQualityEntry> sBattlePetBreedQualityStore(BattlePetBreedQualityEntryfmt);
DB2Storage <BattlePetBreedStateEntry> sBattlePetBreedStateStore(BattlePetBreedStateEntryfmt);
DB2Storage <QuestPackageItem> sQuestPackageItemStore(QuestPackageItemfmt);
DB2Storage <SpellReagentsEntry> sSpellReagentsStore(SpellReagentsEntryfmt);
DB2Storage <ItemUpgradeEntry> sItemUpgradeStore(ItemUpgradeEntryfmt);
DB2Storage <RuleSetItemUpgradeEntry> sRuleSetItemUpgradeEntryStore(RuleSetItemUpgradeEntryfmt);
DB2Storage <GameObjectsEntry> sGameObjectsStore(GameObjectsEntryfmt);
DB2Storage <MapChallengeModeEntry> sMapChallengeModeStore(MapChallengeModeEntryfmt);
DB2Storage <SpellVisualEntry> sSpellVisualStore(SpellVisualEntryfmt);

typedef std::list<std::string> StoreProblemList1;
static std::map<uint32, std::list<uint32> > sPackageItemList;
std::list<uint32> sGameObjectsList;

ItemUpgradeDataMap sItemUpgradeDataMap;

BattlePetSpeciesBySpellIdMap sBattlePetSpeciesBySpellId;
BattlePetBreedStateByBreedMap sBattlePetBreedStateByBreedId;
BattlePetSpeciesStateBySpecMap sBattlePetSpeciesStateBySpecId;

MapChallengeModeEntryMap sMapChallengeModeEntrybyMap;

uint32 DB2FilesCount = 0;

static bool LoadDB2_assert_print(uint32 fsize,uint32 rsize, const std::string& filename)
{
    sLog->outError(LOG_FILTER_GENERAL, "Size of '%s' setted by format string (%u) not equal size of C++ structure (%u).", filename.c_str(), fsize, rsize);

    // ASSERT must fail after function call
    return false;
}

struct LocalDB2Data
{
    LocalDB2Data(LocaleConstant loc) : defaultLocale(loc), availableDb2Locales(0xFFFFFFFF) {}

    LocaleConstant defaultLocale;

    // bitmasks for index of fullLocaleNameList
    uint32 availableDb2Locales;
};

template<class T>
inline void LoadDB2(StoreProblemList1& errlist, DB2Storage<T>& storage, const std::string& db2_path, const std::string& filename, std::string const* customFormat = NULL, std::string const* customIndexName = NULL)
{
    // compatibility format and C++ structure sizes
    ASSERT(DB2FileLoader::GetFormatRecordSize(storage.GetFormat()) == sizeof(T) || LoadDB2_assert_print(DB2FileLoader::GetFormatRecordSize(storage.GetFormat()), sizeof(T), filename));

    ++DB2FilesCount;

    std::string db2_filename = db2_path + filename;
    SqlDb2 * sql = NULL;
    if (customFormat)
        sql = new SqlDb2(&filename, customFormat, customIndexName, storage.GetFormat());

    if (!storage.Load(db2_filename.c_str(), sql))
    {
        // sort problematic db2 to (1) non compatible and (2) nonexistent
        if (FILE * f = fopen(db2_filename.c_str(), "rb"))
        {
            char buf[100];
            snprintf(buf, 100,"(exist, but have %d fields instead " SIZEFMTD ") Wrong client version DBC file?", storage.GetFieldCount(), strlen(storage.GetFormat()));
            errlist.push_back(db2_filename + buf);
            fclose(f);
        }
        else
            errlist.push_back(db2_filename);
    }

    delete sql;
}

void LoadDB2Stores(const std::string& dataPath)
{
    std::string db2Path = dataPath + "dbc/";

    StoreProblemList1 bad_db2_files;

    LoadDB2(bad_db2_files, sBattlePetSpeciesStore,  db2Path, "BattlePetSpecies.db2");

    for (uint32 i = 0; i < sBattlePetSpeciesStore.GetNumRows(); ++i)
    {
        BattlePetSpeciesEntry const* entry = sBattlePetSpeciesStore.LookupEntry(i);
        if (!entry)
            continue;

        // Ruby Droplet DBC fix
        if (entry->CreatureEntry == 73356)
            const_cast<BattlePetSpeciesEntry*>(entry)->spellId = 148050;

        sBattlePetSpeciesBySpellId[entry->CreatureEntry] = entry;
    }

    LoadDB2(bad_db2_files, sBattlePetAbilityStore,  db2Path, "BattlePetAbility.db2");
    LoadDB2(bad_db2_files, sBattlePetAbilityEffectStore,  db2Path, "BattlePetAbilityEffect.db2");
    LoadDB2(bad_db2_files, sBattlePetEffectPropertiesStore,  db2Path, "BattlePetEffectProperties.db2");
    LoadDB2(bad_db2_files, sBattlePetAbilityTurnStore,  db2Path, "BattlePetAbilityTurn.db2");
    LoadDB2(bad_db2_files, sBattlePetAbilityStateStore,  db2Path, "BattlePetAbilityState.db2");
    LoadDB2(bad_db2_files, sBattlePetSpeciesStateStore,  db2Path, "BattlePetSpeciesState.db2");
    LoadDB2(bad_db2_files, sBattlePetSpeciesXAbilityStore,  db2Path, "BattlePetSpeciesXAbility.db2");
    LoadDB2(bad_db2_files, sBattlePetStateStore,  db2Path, "BattlePetState.db2");
    LoadDB2(bad_db2_files, sBattlePetBreedQualityStore,  db2Path, "BattlePetBreedQuality.db2");
    LoadDB2(bad_db2_files, sBattlePetBreedStateStore,  db2Path, "BattlePetBreedState.db2");

    for (uint32 i = 0; i < sBattlePetSpeciesStateStore.GetNumRows(); ++i)
    {
        BattlePetSpeciesStateEntry const* entry = sBattlePetSpeciesStateStore.LookupEntry(i);

        if (!entry)
            continue;

        sBattlePetSpeciesStateBySpecId.insert(BattlePetSpeciesStateBySpecMap::value_type(entry->speciesID, std::make_pair(entry->stateID, entry->stateModifier)));
    }

    for (uint32 i = 0; i < sBattlePetBreedStateStore.GetNumRows(); ++i)
    {
        BattlePetBreedStateEntry const* entry = sBattlePetBreedStateStore.LookupEntry(i);

        if (!entry)
            continue;

        sBattlePetBreedStateByBreedId.insert(BattlePetBreedStateByBreedMap::value_type(entry->breedID, std::make_pair(entry->stateID, entry->stateModifier)));
    }

    LoadDB2(bad_db2_files, sItemStore,              db2Path, "Item.db2");
    LoadDB2(bad_db2_files, sItemCurrencyCostStore,  db2Path, "ItemCurrencyCost.db2");
    LoadDB2(bad_db2_files, sItemSparseStore,        db2Path, "Item-sparse.db2");
    LoadDB2(bad_db2_files, sItemExtendedCostStore,  db2Path, "ItemExtendedCost.db2", &CustomItemExtendedCostEntryfmt, &CustomItemExtendedCostEntryIndex);
    LoadDB2(bad_db2_files, sQuestPackageItemStore,  db2Path, "QuestPackageItem.db2");
    LoadDB2(bad_db2_files, sGameObjectsStore,       db2Path, "GameObjects.db2");
    LoadDB2(bad_db2_files, sMapChallengeModeStore,  db2Path, "MapChallengeMode.db2");
    LoadDB2(bad_db2_files, sSpellVisualStore,       db2Path, "SpellVisual.db2");
    
    for (uint32 i = 0; i < sMapChallengeModeStore.GetNumRows(); ++i)
    {
        MapChallengeModeEntry const* entry = sMapChallengeModeStore.LookupEntry(i);
        if (!entry)
            continue;

        sMapChallengeModeEntrybyMap[entry->map] = entry;
    }

    for (uint32 i = 0; i < sQuestPackageItemStore.GetNumRows(); ++i)
    {
        if (QuestPackageItem const* sp = sQuestPackageItemStore.LookupEntry(i))
            sPackageItemList[sp->packageEntry].push_back(i);
    }

    for (uint32 i = 0; i < sGameObjectsStore.GetNumRows(); ++i)
    {
        if (GameObjectsEntry const* goe = sGameObjectsStore.LookupEntry(i))
            sGameObjectsList.push_back(i);
    }

    LoadDB2(bad_db2_files, sSpellReagentsStore,     db2Path,"SpellReagents.db2");
    LoadDB2(bad_db2_files, sItemUpgradeStore,       db2Path,"ItemUpgrade.db2");
    LoadDB2(bad_db2_files, sRuleSetItemUpgradeEntryStore,db2Path,"RulesetItemUpgrade.db2");

    for (uint32 i = 0; i < sRuleSetItemUpgradeEntryStore.GetNumRows(); ++i)
    {
        RuleSetItemUpgradeEntry const* rsiu = sRuleSetItemUpgradeEntryStore.LookupEntry(i);
        if (!rsiu)
            continue;

        ItemUpgradeDataMap::iterator itr = sItemUpgradeDataMap.find(rsiu->itemEntry);
        if (itr != sItemUpgradeDataMap.end())
            continue;

        ItemUpgradeData& data = sItemUpgradeDataMap[rsiu->itemEntry];

        uint32 offs = 0;
        uint32 prevUpd = 0;
        for (uint32 j = 0; j < sItemUpgradeStore.GetNumRows(); ++j)
        {
            ItemUpgradeEntry const* ue = sItemUpgradeStore.LookupEntry(j);
            if (!ue)
                continue;

            if (!prevUpd)
            {
                if (ue->id == rsiu->startUpgrade)
                {
                    prevUpd = ue->id;
                    data.upgrade[offs++] = ue;
                    j = 0;
                }
            }
            else if (ue->prevUpgradeId == prevUpd)
            {
                prevUpd = ue->id;
                data.upgrade[offs++] = ue;
                j = 0;
            }
        }
    }

    // error checks
    if (bad_db2_files.size() >= DB2FilesCount)
    {
        sLog->outError(LOG_FILTER_GENERAL, "\nIncorrect DataDir value in worldserver.conf or ALL required *.db2 files (%d) not found by path: %sdb2", DB2FilesCount, dataPath.c_str());
        exit(1);
    }
    else if (!bad_db2_files.empty())
    {
        std::string str;
        for (std::list<std::string>::iterator i = bad_db2_files.begin(); i != bad_db2_files.end(); ++i)
            str += *i + "\n";

        sLog->outError(LOG_FILTER_GENERAL, "\nSome required *.db2 files (%u from %d) not found or not compatible:\n%s", (uint32)bad_db2_files.size(), DB2FilesCount,str.c_str());
        exit(1);
    }

    // Check loaded DB2 files proper version
    if (!sItemStore.LookupEntry(107499)            ||       // last item added in 5.4.1 17538
        !sItemExtendedCostStore.LookupEntry(5268)  )        // last item extended cost added in 5.4.1 17538
    {
        sLog->outError(LOG_FILTER_GENERAL, "Please extract correct db2 files from client 5.0.5 16057.");
        exit(1);
    }

    sLog->outInfo(LOG_FILTER_GENERAL, ">> Initialized %d DB2 data stores.", DB2FilesCount);
}

std::list<uint32> GetPackageItemList(uint32 packageEntry)
{
    return sPackageItemList[packageEntry];
}

std::list<uint32> GetGameObjectsList()
{
    return sGameObjectsList;
}

ItemUpgradeData const* GetItemUpgradeData(uint32 itemEntry)
{
    ItemUpgradeDataMap::iterator itr = sItemUpgradeDataMap.find(itemEntry);
    if (itr == sItemUpgradeDataMap.end())
        return NULL;

    return &itr->second;
}
