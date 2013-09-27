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
#include "DBCEnums.h"
#include "ObjectMgr.h"
#include "ArenaTeamMgr.h"
#include "GuildMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "DatabaseEnv.h"
#include "AchievementMgr.h"
#include "ArenaTeam.h"
#include "CellImpl.h"
#include "GameEventMgr.h"
#include "GridNotifiersImpl.h"
#include "Guild.h"
#include "Language.h"
#include "Player.h"
#include "SpellMgr.h"
#include "DisableMgr.h"
#include "ScriptMgr.h"
#include "MapManager.h"
#include "Battleground.h"
#include "BattlegroundAB.h"
#include "Map.h"
#include "InstanceScript.h"
#include "Group.h"
#include "BattlePetMgr.h"

BattlePetMgr::BattlePetMgr(Player* owner) : m_player(owner)
{
}

void BattlePetMgr::GetBattlePetList(PetBattleDataList &battlePetList) const
{
    PlayerSpellMap spellMap = m_player->GetSpellMap();
    for (PlayerSpellMap::const_iterator itr = spellMap.begin(); itr != spellMap.end(); ++itr)
    {
        if (itr->second->state == PLAYERSPELL_REMOVED)
            continue;

        if (!itr->second->active || itr->second->disabled)
            continue;

        SpellInfo const* spell = sSpellMgr->GetSpellInfo(itr->first);
        if (!spell)
            continue;

        // Is summon pet spell
        if (spell->Effects[0].Effect != SPELL_EFFECT_SUMMON || spell->Effects[0].MiscValueB != 3221)
            continue;

        CreatureTemplate const* creature = sObjectMgr->GetCreatureTemplate(spell->Effects[0].MiscValue);
        if (!creature)
            continue;

        BattlePetSpeciesEntry const* species = sBattlePetSpeciesStore.LookupEntry(creature->Entry);
        if (!species)
            continue;

        PetBattleData pet(creature->Entry, creature->Modelid1, species->ID, spell->Id);
        battlePetList.push_back(pet);
    }
}

void BattlePetMgr::BuildBattlePetJournal(WorldPacket *data)
{
    PetBattleDataList petList;

    data->Initialize(SMSG_BATTLE_PET_JOURNAL);
    *data << uint16(0);         // unk
    data->WriteBit(1);          // unk
    data->WriteBits(0, 20);     // unk counter, may be related to battle pet slot

    GetBattlePetList(petList);
    data->WriteBits(petList.size(), 19);

    for (PetBattleDataList::const_iterator pet = petList.begin(); pet != petList.end(); ++pet)
    {
        data->WriteBit(true);   // hasBreed, inverse
        data->WriteBit(true);   // hasQuality, inverse
        data->WriteBit(true);   // hasUnk, inverse
        data->WriteBits(0, 7);  // name lenght
        data->WriteBit(false);  // unk bit
        data->WriteBit(false);  // has guid
    }

    data->FlushBits();

    for (PetBattleDataList::const_iterator pet = petList.begin(); pet != petList.end(); ++pet)
    {
        *data << uint32(pet->m_displayID);
        *data << uint32(pet->m_summonSpellID); // Pet Entry
        *data << uint16(0);     // xp
        *data << uint32(1);     // health
        *data << uint16(1);     // level
        // name
        *data << uint32(1);     // speed
        *data << uint32(1);     // max health
        *data << uint32(pet->m_entry); // Creature ID
        *data << uint32(1);     // power
        *data << uint32(pet->m_speciesID); // species
    }
}

void WorldSession::HandleSummonBattlePet(WorldPacket& recvData)
{
    uint32 spellID;
    recvData >> spellID;

    if (!_player->HasActiveSpell(spellID))
        return;

    _player->CastSpell(_player, spellID, true);
}
