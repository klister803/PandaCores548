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
#include "GuildMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "DatabaseEnv.h"
#include "AchievementMgr.h"
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

        BattlePetSpeciesBySpellIdMap::const_iterator it = sBattlePetSpeciesBySpellId.find(itr->first);
        if (it == sBattlePetSpeciesBySpellId.end())
            continue;

        SpellInfo const* spell = sSpellMgr->GetSpellInfo(itr->first);
        if (!spell)
            continue;

        CreatureTemplate const* creature = sObjectMgr->GetCreatureTemplate(it->second->CreatureEntry);
        if (!creature)
            continue;

        battlePetList.push_back(PetBattleData(it->second, 12, creature->Modelid1, 10, 5, 100, 100, 4, 50));
    }
}

void BattlePetMgr::BuildBattlePetJournal(WorldPacket *data)
{
    PetBattleDataList petList;
    GetBattlePetList(petList);

    bool hasBreed = false;

    data->Initialize(SMSG_BATTLE_PET_JOURNAL, 400);
    data->WriteBits(petList.size(), 19);

    for (PetBattleDataList::const_iterator pet = petList.begin(); pet != petList.end(); ++pet)
    {
        ObjectGuid guid = uint64(pet->m_speciesEntry->spellId);

        data->WriteBit(!hasBreed);          // hasBreed, inverse
        data->WriteGuidMask<1, 5, 3>(guid);
        data->WriteBit(0);                  // has guid
        data->WriteBit(!pet->m_quality);    // hasQuality, inverse
        data->WriteGuidMask<6, 7>(guid);
        data->WriteBit(1);                  // has flags, inverse
        data->WriteGuidMask<0, 4>(guid);
        data->WriteBits(0, 7);              // custom name length
        data->WriteGuidMask<2>(guid);
        data->WriteBit(1);                  // hasUnk, inverse
    }

    uint32 unk_count = 3;                   // unk counter, may be related to battle pet slot
    data->WriteBits(unk_count, 25);

    for (uint32 i = 0; i < unk_count; ++i)
    {
        data->WriteBit(!i);
        data->WriteBit(1);
        data->WriteBit(1);
        data->WriteBits(0, 8);
        data->WriteBit(1);
    }

    data->WriteBit(1);                      // unk

    for (uint32 i = 0; i < unk_count; ++i)
    {
        if (i)
            *data << uint8(i);
    }

    for (PetBattleDataList::const_iterator pet = petList.begin(); pet != petList.end(); ++pet)
    {
        ObjectGuid guid = uint64(pet->m_speciesEntry->spellId);

        if (hasBreed)
            *data << uint16(7);
        *data << uint32(pet->m_speciesEntry->ID);
        *data << uint32(pet->m_speed);                          // speed
        data->WriteGuidBytes<1, 6, 4>(guid);
        *data << uint32(pet->m_displayID);
        *data << uint32(pet->m_maxHealth);                      // max health
        *data << uint32(pet->m_power);                          // power
        data->WriteGuidBytes<2>(guid);
        *data << uint32(pet->m_speciesEntry->CreatureEntry);    // Creature ID
        *data << uint16(pet->m_level);                          // level
        //*data->WriteString("");                               // custom name
        *data << uint32(pet->m_health);                         // health
        *data << uint16(pet->m_experience);                     // xp
        if (pet->m_quality)
            *data << uint8(pet->m_quality);
        data->WriteGuidBytes<3, 7, 0, 5>(guid);
    }

    *data << uint16(1);         // unk
}

void WorldSession::HandleSummonBattlePet(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<6, 0, 1, 5, 3, 4, 7, 2>(guid);
    recvData.ReadGuidBytes<2, 5, 3, 7, 1, 0, 6, 4>(guid);

    uint32 spellId = (uint64(guid) & 0xFFFFFFFF);

    if (!_player->HasActiveSpell(spellId))
        return;

    if (_player->m_SummonSlot[SUMMON_SLOT_MINIPET])
    {
        Creature* oldSummon = _player->GetMap()->GetCreature(_player->m_SummonSlot[SUMMON_SLOT_MINIPET]);
        if (oldSummon && oldSummon->isSummon() && oldSummon->GetUInt64Value(UNIT_FIELD_BATTLE_PET_COMPANION_GUID) == guid)
            oldSummon->ToTempSummon()->UnSummon();
        else
            _player->CastSpell(_player, spellId, true);
    }
    else
        _player->CastSpell(_player, spellId, true);
}

void WorldSession::HandleBattlePetOpcode166F(WorldPacket& recvData)
{
    float playerX, playerY, playerZ, playerOrient;
    float petAllyX, petEnemyX;
    float petAllyY, petEnemyY;
    float petAllyZ, petEnemyZ;
    uint32 unkCounter;

    recvData >> playerX;
    recvData >> playerZ;
    recvData >> playerY;

    for (int i = 0; i < 2; ++i)
    {
        if (i == 0)
        {
            recvData >> petAllyY;
            recvData >> petAllyZ;
            recvData >> petAllyX;
        }
        else
        {
            recvData >> petEnemyY;
            recvData >> petEnemyZ;
            recvData >> petEnemyX;
        }
    }

    bool facing = recvData.ReadBit();
    bool uunk = recvData.ReadBit();

    if (!uunk)
        recvData >> unkCounter;

    if (!facing)
        recvData >> playerOrient;

    WorldPacket data(SMSG_BATTLE_PET_FINALIZE_LOCATION);

    data << playerY;

    for (int i = 0; i < 2; ++i)
    {
        if (i == 0)
        {
            data << petAllyY;
            data << petAllyX;
            data << petAllyZ;
        }
        else
        {
            data << petEnemyY;
            data << petEnemyX;
            data << petEnemyZ;
        }
    }

    data << playerX;
    data << playerZ;

    data.WriteBit(0);
    data.WriteBit(0);

    data << _player->GetOrientation();
    data << uint32(21);
    SendPacket(&data);

    // send full update
    /*WorldPacket data1(SMSG_BATTLE_PET_FULL_UPDATE);
    ObjectGuid guid;
    for (uint8 i = 0; i < 3; ++i)
    {
        data1.WriteBits(0, 21);
        data1.WriteBits(0, 21);
    }

    data1.WriteBit(1);
    data1.WriteBit(1);
    data1.WriteBit(1);

    if (_player->m_SummonSlot[SUMMON_SLOT_MINIPET])
    {
        Creature* oldSummon = _player->GetMap()->GetCreature(_player->m_SummonSlot[SUMMON_SLOT_MINIPET]);
        if (oldSummon && oldSummon->isSummon() && oldSummon->GetUInt64Value(UNIT_FIELD_BATTLE_PET_COMPANION_GUID))
            guid = oldSummon->GetObjectGuid();
    }

    ObjectGuid guid2 = guid;

    for (uint8 i = 0; i < 2; ++i)
    {
        data1.WriteGuidMask<2>(guid);
        data1.WriteBit(1);
        data1.WriteGuidMask<5, 7, 4>(guid);
        data1.WriteBit(0);
        data1.WriteGuidMask<0>(guid);
        data1.WriteBits(0, 2);
        data1.WriteGuidMask<1, 6>(guid);
        data1.WriteBit(1);
        data1.WriteGuidMask<3>(guid);
    }

    data1.WriteBit(1);
    data1.WriteBit(1);
    data1.WriteBit(1);

    data1.WriteGuidMask<6, 0, 1, 4, 2, 5, 3, 7>(guid2);

    data1.WriteBit(1);
    data1.WriteBit(0);
    data1.WriteBit(0);

    for (uint8 i = 0; i < 2; ++i)
    {
        data1 << uint32(427);
        data1.WriteGuidBytes<2, 5>(guid);
        data1 << uint32(2);
        data1.WriteGuidBytes<4, 0, 7, 6>(guid);
        data1 << uint8(6);
        data1.WriteGuidBytes<1, 3>(guid);
    }

    data1.WriteGuidBytes<6, 2, 1, 3, 0, 4, 7, 5>(guid2);

    data1 << uint32(1);*/


    // send full update
    WorldPacket data1(SMSG_BATTLE_PET_FULL_UPDATE);
    for (uint8 i = 0; i < 3; ++i)
    {
        data1.WriteBits(0, 21);
        data1.WriteBits(0, 21);
    }

    data1.WriteBit(0);
    data1.WriteBit(0);
    data1.WriteBit(0);

    ObjectGuid guid2 = _player->GetSelectedUnit()->GetObjectGuid();
    ObjectGuid guid3 = 0;
    ObjectGuid ownerGuid = _player->GetObjectGuid();
    // important strange GUID...
    // [0] Guid: Full: 0x1D3B6D0500000007 Type: 259 Low: 7
    // = player GUID with inverse byte order! WTF???
    // TEST
    ObjectGuid guid = ((ownerGuid & 0x00000000000000FF) << 56) | ((ownerGuid & 0x000000000000FF00) << 40) | ((ownerGuid & 0x0000000000FF0000) << 24) | ((ownerGuid & 0x00000000FF000000) <<  8) |
        ((ownerGuid & 0x000000FF00000000) >>  8) | ((ownerGuid & 0x0000FF0000000000) >> 24) | ((ownerGuid & 0x00FF000000000000) >> 40) | ((ownerGuid & 0xFF00000000000000) >> 56);
    ObjectGuid guid4 = 0;

    for (uint8 i = 0; i < 2; ++i)
    {
        if (i == 0)
            data1.WriteGuidMask<2>(guid);
        else
            data1.WriteGuidMask<2>(guid4);

        data1.WriteBit(1);

        if (i == 0)
            data1.WriteGuidMask<5, 7, 4>(guid);
        else
            data1.WriteGuidMask<5, 7, 4>(guid4);

        data1.WriteBit(0);

        if (i == 0)
            data1.WriteGuidMask<0>(guid);
        else
            data1.WriteGuidMask<0>(guid4);

        data1.WriteBits(1, 2);                              // pet count

        if (i == 0)
            data1.WriteGuidMask<1>(guid);
        else
            data1.WriteGuidMask<1>(guid4);

        //for (uint8 j = 0; j < 1; ++j)
        //{
            // TEST, i=0 - player, i=1 - wild pet
            if (i == 0)
            {
                data1.WriteGuidMask<3, 4, 0>(ownerGuid);
                data1.WriteBit(0);
                data1.WriteGuidMask<6>(ownerGuid);
                data1.WriteBits(6, 21);  // state count?
                data1.WriteBits(1, 20);
                data1.WriteGuidMask<5>(ownerGuid);

                data1.WriteBit(0);
                data1.WriteGuidMask<2, 1, 7>(ownerGuid);
                data1.WriteBit(1);
                data1.WriteBits(0, 7);
                data1.WriteBits(1, 21);

                data1.WriteBit(1);
                data1.WriteBit(0);
                data1.WriteBit(0);
            }
            else
            {
                data1.WriteGuidMask<3, 4, 0>(guid3);
                data1.WriteBit(0);
                data1.WriteGuidMask<6>(guid3);
                data1.WriteBits(5, 21);   // state count?
                data1.WriteBits(1, 20);
                data1.WriteGuidMask<5>(guid3);

                data1.WriteBit(0);
                data1.WriteGuidMask<2, 1, 7>(guid3);
                data1.WriteBit(1);
                data1.WriteBits(0, 7);
                data1.WriteBits(0, 21);
            }
        //}

        if (i == 0)
            data1.WriteGuidMask<6>(guid);
        else
            data1.WriteGuidMask<6>(guid4);

        data1.WriteBit(0);

        if (i == 0)
            data1.WriteGuidMask<3>(guid);
        else
            data1.WriteGuidMask<3>(guid4);
    }

    data1.WriteBit(1);
    data1.WriteBit(0);
    data1.WriteBit(1);

    data1.WriteGuidMask<6, 0, 1, 4, 2, 5, 3, 7>(guid2);

    data1.WriteBit(0);
    data1.WriteBit(1);
    data1.WriteBit(0);

    for (uint8 i = 0; i < 2; ++i)
    {
        for (uint8 j = 0; j < 1; ++j)
        {
            // TEST, i=0 - player, i=1 - wild pet
            if (i == 0)
            {
                // 1 ability (for 1)
                data1 << uint8(0);
                data1 << uint16(0);
                data1 << uint8(0);  // slot index
                data1 << uint16(0);
                data1 << uint32(111); // ability ID
            }
            else
            {
                // 1 ability (for 1)
                data1 << uint8(0);
                data1 << uint16(0);
                data1 << uint8(0);  // slot index
                data1 << uint16(0);
                data1 << uint32(595); // ability ID
            }

            data1 << uint16(22);  // experience
            data1 << uint32(151); // current/total HP

            if (i == 0)
                data1.WriteGuidBytes<1>(ownerGuid);
            else
                data1.WriteGuidBytes<1>(guid3);

            //
            if (i == 0)
            {
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
            }
            else
            {
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
                data1 << uint32(0);
            }

            data1 << uint32(0);
            data1 << uint32(200); // speed

            // aura handle test
            // 239 - flying creature passive
            if (i == 0)
            {
                data1 << uint32(-1); // duration?
                data1 << uint32(1);
                data1 << uint8(0);
                data1 << uint32(239); // auraID = spellID
            }

            if (i == 0)
                data1.WriteGuidBytes<4>(ownerGuid);
            else
                data1.WriteGuidBytes<4>(guid3);

            // Custom Name
            //data1.WriteString("");
            // Quality
            data1 << uint16(3);

            if (i == 0)
                data1.WriteGuidBytes<0>(ownerGuid);
            else
                data1.WriteGuidBytes<0>(guid3);

            // current/Total HP?
            if (i == 0)
                data1 << uint32(151);
            else
                data1 << uint32(151);

            if (i == 0)
                data1.WriteGuidBytes<3>(ownerGuid);
            else
                data1.WriteGuidBytes<3>(guid3);

            // Species ID
            if (i == 0)
                data1 << uint32(292);
            else
                data1 << uint32(293);

            data1 << uint32(0);

            if (i == 0)
                data1.WriteGuidBytes<2, 6>(ownerGuid);
            else
                data1.WriteGuidBytes<2, 6>(guid3);

            // display ID?
            if (i == 0)
                data1 << uint32(36944);
            else
                data1 << uint32(1924);

            // Level
            if (i == 0)
                data1 << uint16(12);
            else
                data1 << uint16(15);

            if (i == 0)
                data1.WriteGuidBytes<7, 5>(ownerGuid);
            else
                data1.WriteGuidBytes<7, 5>(guid3);

            // Attack Power
            if (i == 0)
                data1 << uint32(300);
            else
                data1 << uint32(500);

            data1 << uint8(0); // pet slot index?
        }

        data1 << uint32(427);
        if (i == 0)
            data1.WriteGuidBytes<2, 5>(guid);
        else
            data1.WriteGuidBytes<2, 5>(guid4);
        
        data1 << uint32(2);

        if (i == 0)
            data1.WriteGuidBytes<4, 0, 7, 6>(guid);
        else
            data1.WriteGuidBytes<4, 0, 7, 6>(guid4);

        data1 << uint8(6);
        data1 << uint8(0);

        if (i == 0)
            data1.WriteGuidBytes<1, 3>(guid);
        else
            data1.WriteGuidBytes<1, 3>(guid4);
    }

    data1.WriteGuidBytes<6, 2, 1, 3, 0, 4, 7, 5>(guid2);

    data1 << uint16(30);
    data1 << uint16(30);
    data1 << uint8(1);
    data1 << uint32(0);
    data1 << uint8(10);

    SendPacket(&data1);
}

void WorldSession::HandleBattlePetReadyForBattle(WorldPacket& recvData)
{
    uint8 state;
    recvData >> state;

    if (!state)
    {
        WorldPacket data(SMSG_BATTLE_PET_FIRST_ROUND);
        for (uint8 i = 0; i < 2; ++i)
        {
            data << uint16(0);
            data << uint8(2);
            data << uint8(0);
        }

        data << uint32(0);
        data.WriteBits(0, 3);
        data.WriteBit(0);
        data.WriteBits(2, 22);

        for (uint8 i = 0; i < 2; ++i)
        {
            data.WriteBit(0);
            data.WriteBit(1);
            data.WriteBit(1);
            data.WriteBits(1, 25);

            data.WriteBit(1);

            // for bits25
            data.WriteBit(0);
            data.WriteBits(3, 3);
            //

            data.WriteBit(1);
            data.WriteBit(1);
            data.WriteBit(0);
        }

        data.WriteBits(0, 20);

        // for
        data << uint8(0); //0
        data << uint8(0); //0 0
        data << uint8(4); //0

        data << uint8(3); //1
        data << uint8(3); //1 0
        data << uint8(4); //1
        //
        data << uint8(2);
        SendPacket(&data);
    }
}

void WorldSession::HandleBattlePetOpcode1ACF(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<6, 2, 3, 7, 0, 4>(guid);
    recvData.ReadBit();
    recvData.ReadGuidMask<5, 1>(guid);
    recvData.ReadGuidBytes<3, 5, 6, 7, 1, 0, 2, 4>(guid);

    sLog->outError(LOG_FILTER_GENERAL, "GUID from packet 0x1ACF - %u", guid);
}
