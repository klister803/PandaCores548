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

typedef std::list<uint32> PackageItemList;
typedef std::map<uint32, PackageItemList> PackageItemMap;
static PackageItemMap sPackageItemMap;
std::list<uint32> GetPackageItemList(uint32 packageEntry);

extern DB2Storage <ItemEntry> sItemStore;
extern DB2Storage <ItemCurrencyCostEntry> sItemCurrencyCostStore;
extern DB2Storage <ItemExtendedCostEntry> sItemExtendedCostStore;
extern DB2Storage <ItemSparseEntry> sItemSparseStore;
extern DB2Storage <BattlePetSpeciesEntry> sBattlePetSpeciesStore;
extern DB2Storage <QuestPackageItem> sQuestPackageItemStore;
extern DB2Storage <SpellReagentsEntry> sSpellReagentsStore;

void LoadDB2Stores(const std::string& dataPath);

SpellReagentsEntry const* GetSpellReagentEntry(uint32 spellId, uint8 reagent);

#endif