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
#include "DatabaseEnv.h"
#include "Opcodes.h"
#include "Log.h"
#include "Player.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "ObjectAccessor.h"
#include "UpdateMask.h"

void WorldSession::HandleSetSpecialization(WorldPacket& recvData)
{
    uint32 tab = recvData.read<uint32>();
    uint8 classId = _player->getClass();

    // Avoid cheat - hack
    if(_player->GetSpecializationId(_player->GetActiveSpec()))
        return;

    uint32 specializationId = 0;
    uint32 specializationSpell = 0;

    for (uint32 i = 0; i < sChrSpecializationsStore.GetNumRows(); i++)
    {
        ChrSpecializationsEntry const* specialization = sChrSpecializationsStore.LookupEntry(i);
        if (!specialization)
            continue;

        if (specialization->classId == classId && specialization->tabId == tab)
        {
            specializationId = specialization->entry;
            specializationSpell = specialization->specializationSpell;
            break;
        }
    }

    if (specializationId)
    {
        _player->SetSpecializationId(_player->GetActiveSpec(), specializationId);
        _player->SendTalentsInfoData(false);
        _player->InitSpellForLevel();
        _player->InitialPowers();
        _player->_ApplyOrRemoveItemEquipDependentAuras(0, false);
    }
}

void WorldSession::HandleLearnTalents(WorldPacket& recvData)
{
    uint32 count = recvData.ReadBits(23);

    // Cheat - Hack check
    if (count > 6)
        return;

    if (count > _player->GetFreeTalentPoints())
        return;

    Battleground* bg = _player->GetBattleground();
    if (bg && bg->GetStatus() != STATUS_WAIT_JOIN)
    {
        recvData.rfinish();
        return;
    }

    for (uint32 i = 0; i < count; ++i)
    {
        uint16 talentId = recvData.read<uint16>();
        _player->LearnTalent(talentId);
    }

    _player->SendTalentsInfoData(false);
}

void WorldSession::HandleTalentWipeConfirmOpcode(WorldPacket& recvData)
{
    TC_LOG_DEBUG("network", "MSG_TALENT_WIPE_CONFIRM");

    uint8 specializationReset = recvData.read<uint8>();
    ObjectGuid guid;
    recvData.ReadGuidMask<7, 4, 0, 5, 2, 6, 3, 1>(guid);
    recvData.ReadGuidBytes<5, 1, 7, 0, 3, 6, 4, 2>(guid);

    // Hack
    if (GetPlayer()->HasAura(33786))
        return;

    // remove fake death
    if (GetPlayer()->HasUnitState(UNIT_STATE_DIED))
        GetPlayer()->RemoveAurasByType(SPELL_AURA_FEIGN_DEATH);

    if(!specializationReset)
    {
        if (!_player->ResetTalents())
        {
            WorldPacket data(SMSG_RESPEC_WIPE_CONFIRM, 8+4);    //you have not any talent
            data << uint8(0); // 0 guid bit
            data << uint32(0);
            data << uint8(0);
            SendPacket(&data);
            return;
        }
    }
    else
    {
        _player->ResetSpec();
    }

    _player->SendTalentsInfoData(false);

    if(Unit* unit = _player->GetSelectedUnit())
        unit->CastSpell(_player, 14867, true);                  //spell: "Untalent Visual Effect"
}

void WorldSession::HandleUnlearnSkillOpcode(WorldPacket& recvData)
{
    uint32 skillId;
    recvData >> skillId;

    if (!IsPrimaryProfessionSkill(skillId))
        return;

    GetPlayer()->SetSkill(skillId, 0, 0, 0);
}

void WorldSession::HandleQueryPlayerRecipes(WorldPacket& recvPacket)
{
    uint32 spellId, skillId;
    ObjectGuid guid;

    recvPacket >> spellId >> skillId;
    recvPacket.ReadGuidMask<6, 0, 2, 3, 5, 7, 1, 4>(guid);
    recvPacket.ReadGuidBytes<5, 6, 1, 3, 4, 0, 7, 2>(guid);

    TC_LOG_DEBUG("network", "CMSG_QUERY_PLAYER_RECIPES player: %s spell: %u skill: %u guid: %u", _player->GetName(), spellId, skillId, (uint64)guid);

    if (!sSkillLineStore.LookupEntry(skillId) || !sSpellMgr->GetSpellInfo(spellId))
    {
        TC_LOG_DEBUG("network", "CMSG_QUERY_PLAYER_RECIPES player: no such spell or skill.");
        return;
    }

    Player* player = sObjectAccessor->FindPlayer(guid);
    if (!player)
    {
        TC_LOG_DEBUG("network", "CMSG_QUERY_PLAYER_RECIPES player %u is not in world.", (uint64)guid);
        return;
    }

    std::set<uint32> relatedSkills;
    relatedSkills.insert(skillId);

    for (uint32 i = 0; i < sSkillLineStore.GetNumRows(); ++i)
    {
        SkillLineEntry const* skillLine = sSkillLineStore.LookupEntry(i);
        if (!skillLine)
            continue;

        if (skillLine->parentSkillId != skillId)
            continue;

        if (!player->HasSkill(skillLine->parentSkillId))
            continue;

        relatedSkills.insert(skillLine->parentSkillId);
    }

    std::set<uint32> profSpells;
    PlayerSpellMap const& spellMap = player->GetSpellMap();
    for (PlayerSpellMap::const_iterator itr = spellMap.begin(); itr != spellMap.end(); ++itr)
    {
        if (itr->second->state == PLAYERSPELL_REMOVED)
            continue;

        if (!itr->second->active || itr->second->disabled)
            continue;

        for (std::set<uint32>::const_iterator itr2 = relatedSkills.begin(); itr2 != relatedSkills.end(); ++itr2)
            if (IsPartOfSkillLine(*itr2, itr->first))
                profSpells.insert(itr->first);
    }

    if (profSpells.empty())
        return;

    WorldPacket data(SMSG_PLAYER_RECIPES, 4 + 3 * 4 + relatedSkills.size() * 4 * 3 + profSpells.size() * 4);
    data.WriteBits(relatedSkills.size(), 22);
    data.WriteGuidMask<3, 7>(guid);
    data.WriteBits(relatedSkills.size(), 22);
    data.WriteBits(profSpells.size(), 22);
    data.WriteBits(relatedSkills.size(), 22);
    data.WriteGuidMask<0, 4, 6, 2, 5, 1>(guid);

    data << uint32(spellId);
    data.WriteGuidBytes<3, 1, 0, 7, 5>(guid);
    for (std::set<uint32>::const_iterator itr = profSpells.begin(); itr != profSpells.end(); ++itr)
        data << uint32(*itr);
    data.WriteGuidBytes<2, 6>(guid);
    for (std::set<uint32>::const_iterator itr = relatedSkills.begin(); itr != relatedSkills.end(); ++itr)
        data << uint32(player->GetMaxSkillValue(*itr));
    data.WriteGuidBytes<4>(guid);
    for (std::set<uint32>::const_iterator itr = relatedSkills.begin(); itr != relatedSkills.end(); ++itr)
        data << uint32(player->GetSkillValue(*itr));
    for (std::set<uint32>::const_iterator itr = relatedSkills.begin(); itr != relatedSkills.end(); ++itr)
        data << uint32(*itr);

    _player->SendDirectMessage(&data);
}
