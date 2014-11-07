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
#include "DBCStores.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include "ObjectMgr.h"
#include "GuildMgr.h"
#include "SpellMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Spell.h"
#include "Totem.h"
#include "TemporarySummon.h"
#include "SpellAuras.h"
#include "CreatureAI.h"
#include "ScriptMgr.h"
#include "GameObjectAI.h"
#include "SpellAuraEffects.h"

void WorldSession::HandleUseItemOpcode(WorldPacket& recvPacket)
{
    // TODO: add targets.read() check
    Player* pUser = _player;

    // ignore for remote control state
    if (pUser->m_mover != pUser)
        return;

    SpellCastTargets targets;
    uint8 bagIndex, slot;
    uint8 castCount = 0;                                        // next cast if exists (single or not)
    uint8 castFlags = 0;
    ObjectGuid itemGUID;
    ObjectGuid targetGuid;
    ObjectGuid dstTransportGuid;
    ObjectGuid srcTransportGuid;
    ObjectGuid guid38;                                      // posible item target
    uint32 glyphIndex = 0;                                      // something to do with glyphs?
    uint32 spellId = 0;                                         // casted spell id

    recvPacket >> bagIndex >> slot;
    bool hasElevation = !recvPacket.ReadBit();
    recvPacket.ReadGuidMask<2>(itemGUID);
    recvPacket.ReadBit();                                   // target guid marker
    recvPacket.ReadGuidMask<5>(itemGUID);
    uint32 archCount = recvPacket.ReadBits(2);
    bool hasCastFlags = !recvPacket.ReadBit();
    recvPacket.ReadBit();                                   // guid38 marker
    glyphIndex = !recvPacket.ReadBit() ? 1 : 0;
    castCount = !recvPacket.ReadBit() ? 1 : 0;
    recvPacket.ReadGuidMask<3, 0>(itemGUID);
    bool hasDst = recvPacket.ReadBit();
    recvPacket.ReadGuidMask<6>(itemGUID);
    uint32 strTargetLen = !recvPacket.ReadBit() ? 1 : 0;
    recvPacket.ReadGuidMask<7, 4>(itemGUID);
    bool hasTargetMask = !recvPacket.ReadBit();
    recvPacket.ReadGuidMask<1>(itemGUID);

    for (uint32 i = 0; i < archCount; ++i)
        recvPacket.ReadBits(2);

    bool hasMovement = recvPacket.ReadBit();
    bool hasSrc = recvPacket.ReadBit();
    spellId = !recvPacket.ReadBit() ? 1 : 0;
    bool hasSpeed = !recvPacket.ReadBit();

    recvPacket.ReadGuidMask<0, 2, 5, 3, 4, 1, 6, 7>(targetGuid);
    if (hasSrc)
        recvPacket.ReadGuidMask<0, 5, 2, 4, 1, 7, 3, 6>(srcTransportGuid);

    if (strTargetLen)
        strTargetLen = recvPacket.ReadBits(7);

    recvPacket.ReadGuidMask<6, 2, 0, 4, 3, 7, 5, 1>(guid38);
    if (hasDst)
        recvPacket.ReadGuidMask<5, 2, 3, 1, 7, 4, 6, 0>(dstTransportGuid);

    bool dword1A0 = false;
    bool hasFloat128 = false;
    bool hasFloat168 = false;
    bool hasFloat188 = false;
    bool hasTransport = false;
    ObjectGuid transportGuid;
    bool hasTransportTime[2];
    ObjectGuid moverGuid;
    bool hasFallData = false;
    bool hasFallDirection = false;
    bool dword118 = false;
    uint32 dword190 = 0;
    if (hasMovement)
    {
        hasFallData = recvPacket.ReadBit();
        dword1A0 = !recvPacket.ReadBit();
        hasFloat168 = !recvPacket.ReadBit();
        hasTransport = recvPacket.ReadBit();
        if (hasTransport)
        {
            recvPacket.ReadGuidMask<4, 5, 7, 2>(transportGuid);
            hasTransportTime[1] = recvPacket.ReadBit();
            recvPacket.ReadGuidMask<3>(transportGuid);
            hasTransportTime[0] = recvPacket.ReadBit();
            recvPacket.ReadGuidMask<6, 0, 1>(transportGuid);
        }
        recvPacket.ReadGuidMask<1, 6>(moverGuid);
        if (hasFallData)
            hasFallDirection = recvPacket.ReadBit();
        recvPacket.ReadGuidMask<0>(moverGuid);
        dword118 = !recvPacket.ReadBit();
        recvPacket.ReadGuidMask<5>(moverGuid);
        recvPacket.ReadBit();
        hasFloat188 = !recvPacket.ReadBit();
        recvPacket.ReadGuidMask<2>(moverGuid);
        recvPacket.ReadBit();
        bool hasMoveFlags2 = !recvPacket.ReadBit();
        recvPacket.ReadGuidMask<4, 3>(moverGuid);
        dword190 = recvPacket.ReadBits(22);
        bool hasMoveFlags = !recvPacket.ReadBit();
        recvPacket.ReadGuidMask<7>(moverGuid);
        if (hasMoveFlags)
            recvPacket.ReadBits(30);
        if (hasMoveFlags2)
            recvPacket.ReadBits(13);
        recvPacket.ReadBit();
        hasFloat128 = !recvPacket.ReadBit();
    }

    if (hasCastFlags)
        castFlags = recvPacket.ReadBits(5);

    if (hasTargetMask)
        targets.SetTargetMask(recvPacket.ReadBits(20));

    recvPacket.ReadGuidBytes<2, 0, 7, 4, 3>(itemGUID);
    for (uint32 i = 0; i < archCount; ++i)
        recvPacket >> Unused<uint32>() >> Unused<uint32>();

    recvPacket.ReadGuidBytes<1, 6, 5>(itemGUID);
    if (hasDst)
    {
        recvPacket.ReadGuidBytes<4, 6, 7, 1, 3, 0>(dstTransportGuid);
        if (dstTransportGuid)
            recvPacket >> targets.m_dst._transportOffset.m_positionX;
        else
            recvPacket >> targets.m_dst._position.m_positionX;
        recvPacket.ReadGuidBytes<2>(dstTransportGuid);
        if (dstTransportGuid)
            recvPacket >> targets.m_dst._transportOffset.m_positionZ;
        else
            recvPacket >> targets.m_dst._position.m_positionZ;
        recvPacket.ReadGuidBytes<5>(dstTransportGuid);
        if (dstTransportGuid)
            recvPacket >> targets.m_dst._transportOffset.m_positionY;
        else
            recvPacket >> targets.m_dst._position.m_positionY;

        targets.m_dst._transportGUID = dstTransportGuid;
    }

    if (hasMovement)
    {
        for (uint32 i = 0; i < dword190; ++i)
            recvPacket.read_skip<uint32>();

        if (hasTransport)
        {
            if (hasTransportTime[0])
                recvPacket.read_skip<uint32>();
            recvPacket.ReadGuidBytes<2>(transportGuid);
            recvPacket.read_skip<float>();
            recvPacket.ReadGuidBytes<1>(transportGuid);
            if (hasTransportTime[1])
                recvPacket.read_skip<uint32>();
            recvPacket.ReadGuidBytes<3>(transportGuid);
            recvPacket.read_skip<float>();
            recvPacket.read_skip<uint8>();
            recvPacket.read_skip<uint32>();
            recvPacket.ReadGuidBytes<5, 6>(transportGuid);
            recvPacket.read_skip<float>();
            recvPacket.ReadGuidBytes<0>(transportGuid);
            recvPacket.read_skip<float>();
            recvPacket.ReadGuidBytes<4, 7>(transportGuid);
        }

        if (hasFallData)
        {
            if (hasFallData)
                recvPacket >> Unused<float>() >> Unused<float>() >> Unused<float>();
            recvPacket.read_skip<float>();
            recvPacket.read_skip<uint32>();
        }
        if (hasFloat128)
            recvPacket.read_skip<float>();
        recvPacket.read_skip<float>();
        if (dword118)
            recvPacket.read_skip<uint32>();
        recvPacket.ReadGuidBytes<6, 7>(moverGuid);
        recvPacket.read_skip<float>();
        recvPacket.ReadGuidBytes<4, 0, 2, 1>(moverGuid);
        if (hasFloat168)
            recvPacket.read_skip<float>();
        if (dword1A0)
            recvPacket.read_skip<uint32>();
        if (hasFloat188)
            recvPacket.read_skip<float>();
        recvPacket.read_skip<float>();
        recvPacket.ReadGuidBytes<3, 5>(moverGuid);
    }

    recvPacket.ReadGuidBytes<1, 5, 7, 0, 6, 2, 3, 4>(targetGuid);

    if (hasElevation)
        recvPacket >> targets.m_elevation;

    if (hasSrc)
    {
        if (srcTransportGuid)
            recvPacket >> targets.m_src._transportOffset.m_positionZ;
        else
            recvPacket >> targets.m_src._position.m_positionZ;
        recvPacket.ReadGuidBytes<2, 3, 4>(srcTransportGuid);
        if (srcTransportGuid)
            recvPacket >> targets.m_src._transportOffset.m_positionY;
        else
            recvPacket >> targets.m_src._position.m_positionY;
        recvPacket.ReadGuidBytes<5, 1, 6, 7, 0>(srcTransportGuid);
        if (srcTransportGuid)
            recvPacket >> targets.m_src._transportOffset.m_positionX;
        else
            recvPacket >> targets.m_src._position.m_positionX;

        targets.m_src._transportGUID = srcTransportGuid;
    }

    if (hasSpeed)
        recvPacket >> targets.m_speed;

    recvPacket.ReadGuidBytes<6, 5, 0, 7, 1, 3, 4, 2>(guid38);

    if (spellId)
        recvPacket >> spellId;

    if (castCount)
        recvPacket >> castCount;

    if (glyphIndex)
        recvPacket >> glyphIndex;

    if (strTargetLen)
        targets.m_strTarget = recvPacket.ReadString(strTargetLen);

    targets.m_itemTargetGUID = guid38;
    targets.m_objectTargetGUID = targetGuid;
    targets.Update(_player->m_mover);

    if (glyphIndex >= MAX_GLYPH_SLOT_INDEX)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    Item* pItem = pUser->GetUseableItemByPos(bagIndex, slot);
    if (!pItem)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    if (pItem->GetGUID() != itemGUID)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_USE_ITEM packet, bagIndex: %u, slot: %u, castCount: %u, spellId: %u, Item: %u, glyphIndex: %u, data length = %i", bagIndex, slot, castCount, spellId, pItem->GetEntry(), glyphIndex, (uint32)recvPacket.size());

    ItemTemplate const* proto = pItem->GetTemplate();
    if (!proto)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, pItem, NULL);
        return;
    }

    // some item classes can be used only in equipped state
    if (proto->InventoryType != INVTYPE_NON_EQUIP && !pItem->IsEquipped())
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, pItem, NULL);
        return;
    }

    InventoryResult msg = pUser->CanUseItem(pItem);
    if (msg != EQUIP_ERR_OK)
    {
        pUser->SendEquipError(msg, pItem, NULL);
        return;
    }

    // only allow conjured consumable, bandage, poisons (all should have the 2^21 item flag set in DB)
    if (proto->Class == ITEM_CLASS_CONSUMABLE && !(proto->Flags & ITEM_PROTO_FLAG_USEABLE_IN_ARENA) && pUser->InArena())
    {
        pUser->SendEquipError(EQUIP_ERR_NOT_DURING_ARENA_MATCH, pItem, NULL);
        return;
    }

    //Citron-infused bandages, hack fix
    if (pItem->GetEntry() == 82787 && pUser->GetMap()->IsDungeon())
    {
        pUser->SendEquipError(EQUIP_ERR_CLIENT_LOCKED_OUT, pItem, NULL);
        return;
    }

    // don't allow items banned in arena
    if (proto->Flags & ITEM_PROTO_FLAG_NOT_USEABLE_IN_ARENA && pUser->InArena())
    {
        pUser->SendEquipError(EQUIP_ERR_NOT_DURING_ARENA_MATCH, pItem, NULL);
        return;
    }

    if (pUser->isInCombat())
    {
        for (int i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        {
            if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(proto->Spells[i].SpellId))
            {
                if (!spellInfo->CanBeUsedInCombat())
                {
                    pUser->SendEquipError(EQUIP_ERR_NOT_IN_COMBAT, pItem, NULL);
                    return;
                }
            }
        }
    }

    // check also  BIND_WHEN_PICKED_UP and BIND_QUEST_ITEM for .additem or .additemset case by GM (not binded at adding to inventory)
    if (pItem->GetTemplate()->Bonding == BIND_WHEN_USE || pItem->GetTemplate()->Bonding == BIND_WHEN_PICKED_UP || pItem->GetTemplate()->Bonding == BIND_QUEST_ITEM)
    {
        if (!pItem->IsSoulBound())
        {
            pItem->SetState(ITEM_CHANGED, pUser);
            pItem->SetBinding(true);
        }
    }

    // Note: If script stop casting it must send appropriate data to client to prevent stuck item in gray state.
    if (!sScriptMgr->OnItemUse(pUser, pItem, targets))
    {
        // no script or script not process request by self
        pUser->CastItemUseSpell(pItem, targets, castCount, glyphIndex);
    }
}

void WorldSession::HandleOpenItemOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_OPEN_ITEM packet, data length = %i", (uint32)recvPacket.size());

    Player* pUser = _player;

    // ignore for remote control state
    if (pUser->m_mover != pUser)
        return;

    uint8 bagIndex, slot;

    recvPacket >> bagIndex >> slot;

    sLog->outInfo(LOG_FILTER_NETWORKIO, "bagIndex: %u, slot: %u", bagIndex, slot);

    Item* item = pUser->GetItemByPos(bagIndex, slot);
    if (!item)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, NULL, NULL);
        return;
    }

    ItemTemplate const* proto = item->GetTemplate();
    if (!proto)
    {
        pUser->SendEquipError(EQUIP_ERR_ITEM_NOT_FOUND, item, NULL);
        return;
    }

    // Verify that the bag is an actual bag or wrapped item that can be used "normally"
    if (!(proto->Flags & ITEM_PROTO_FLAG_OPENABLE) && !item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_WRAPPED))
    {
        pUser->SendEquipError(EQUIP_ERR_CLIENT_LOCKED_OUT, item, NULL);
        sLog->outError(LOG_FILTER_NETWORKIO, "Possible hacking attempt: Player %s [guid: %u] tried to open item [guid: %u, entry: %u] which is not openable!",
                pUser->GetName(), pUser->GetGUIDLow(), item->GetGUIDLow(), proto->ItemId);
        return;
    }

    // locked item
    uint32 lockId = proto->LockID;
    if (lockId)
    {
        LockEntry const* lockInfo = sLockStore.LookupEntry(lockId);

        if (!lockInfo)
        {
            pUser->SendEquipError(EQUIP_ERR_ITEM_LOCKED, item, NULL);
            sLog->outError(LOG_FILTER_NETWORKIO, "WORLD::OpenItem: item [guid = %u] has an unknown lockId: %u!", item->GetGUIDLow(), lockId);
            return;
        }

        // was not unlocked yet
        if (item->IsLocked())
        {
            pUser->SendEquipError(EQUIP_ERR_ITEM_LOCKED, item, NULL);
            return;
        }
    }

    if (item->HasFlag(ITEM_FIELD_FLAGS, ITEM_FLAG_WRAPPED))// wrapped?
    {
        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_SEL_CHARACTER_GIFT_BY_ITEM);

        stmt->setUInt32(0, item->GetGUIDLow());

        PreparedQueryResult result = CharacterDatabase.Query(stmt);

        if (result)
        {
            Field* fields = result->Fetch();
            uint32 entry = fields[0].GetUInt32();
            uint32 flags = fields[1].GetUInt32();

            item->SetUInt64Value(ITEM_FIELD_GIFTCREATOR, 0);
            item->SetEntry(entry);
            item->SetUInt32Value(ITEM_FIELD_FLAGS, flags);
            item->SetState(ITEM_CHANGED, pUser);
        }
        else
        {
            sLog->outError(LOG_FILTER_NETWORKIO, "Wrapped item %u don't have record in character_gifts table and will deleted", item->GetGUIDLow());
            pUser->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
            return;
        }

        stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_GIFT);

        stmt->setUInt32(0, item->GetGUIDLow());

        CharacterDatabase.Execute(stmt);
    }
    else
        pUser->SendLoot(item->GetGUID(), LOOT_CORPSE);
}

void WorldSession::HandleGameObjectUseOpcode(WorldPacket & recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<6, 2, 0, 5, 7, 4, 1, 3>(guid);
    recvData.ReadGuidBytes<4, 1, 5, 2, 3, 7, 6, 0>(guid);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_GAMEOBJ_USE Message [guid=%u]", GUID_LOPART(guid));

    // ignore for remote control state
    if (_player->m_mover != _player)
        return;

    if (GameObject* obj = GetPlayer()->GetMap()->GetGameObject(guid))
        obj->Use(_player);
}

void WorldSession::HandleGameobjectReportUse(WorldPacket& recvPacket)
{
    ObjectGuid guid;
    recvPacket.ReadGuidMask<7, 0, 1, 3, 6, 2, 5, 4>(guid);
    recvPacket.ReadGuidBytes<0, 2, 7, 5, 6, 4, 1, 3>(guid);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_GAMEOBJ_REPORT_USE Message [in game guid: %u]", GUID_LOPART(guid));

    // ignore for remote control state
    if (_player->m_mover != _player)
        return;

    GameObject* go = GetPlayer()->GetMap()->GetGameObject(guid);
    if (!go)
        return;

    if (!go->IsWithinDistInMap(_player, INTERACTION_DISTANCE))
        return;

    if (go->AI()->GossipHello(_player))
        return;

    _player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_USE_GAMEOBJECT, go->GetEntry());
}

void WorldSession::HandleCastSpellOpcode(WorldPacket& recvPacket)
{
    uint32 spellId = 0, glyphIndex = 0, castFlags = 0, flags = 0, flags2 = 0;
    uint8 castCount = 0;
    // client provided targets
    SpellCastTargets targets;
    ObjectGuid itemTargetGuid, dstTransportGuid, srcTransportGuid, objectTargetGuid;

    bool hasGlyphIndex = !recvPacket.ReadBit();
    uint8 stringTargetLen = !recvPacket.ReadBit() ? 1 : 0;
    bool hasSrc = recvPacket.ReadBit();
    bool hasSpellId = !recvPacket.ReadBit();
    if (!hasSpellId)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: got cast spell packet without spell id - don't know what to do! (player %s guid: %u)",
            _player->GetName(), _player->GetGUIDLow());
        recvPacket.rfinish();
        return;
    }

    bool hasCastCount = !recvPacket.ReadBit();
    bool hasTargetMask = !recvPacket.ReadBit();

    bool hasSpeed = !recvPacket.ReadBit();
    recvPacket.ReadBit();   // item target guid marker
    uint8 archeologyCount = recvPacket.ReadBits(2);
    targets.m_weights.resize(archeologyCount);
    bool hasMovement = recvPacket.ReadBit();
    recvPacket.ReadBit();   // target guid marker
    bool hasCastFlags = !recvPacket.ReadBit();
    bool hasDst = recvPacket.ReadBit();

    bool hasElevation = !recvPacket.ReadBit();
    for (uint8 i = 0; i < archeologyCount; ++i)
        targets.m_weights[i].type = recvPacket.ReadBits(2);

    if (stringTargetLen)
        stringTargetLen = recvPacket.ReadBits(7);

    if (hasDst)
        recvPacket.ReadGuidMask<3, 5, 1, 7, 0, 6, 2, 4>(dstTransportGuid);

    bool dword198 = false;
    ObjectGuid moverGuid;
    bool hasSplineElevation = false;
    uint32 counter = 0;
    bool hasFallData = false;
    bool hasFallDirection = false;
    bool hasPitch = false;
    bool hasOrientation = false;
    bool hasTransportData = false;
    bool hasTransportTime2 = false;
    bool hasTransportTime3 = false;
    ObjectGuid transportGuid;
    bool byte185 = false;
    bool hasTimeStamp = false;
    if (hasMovement)
    {
        recvPacket.ReadBit();               // byte184
        recvPacket.ReadGuidMask<4>(moverGuid);
        bool hasMoveFlags = !recvPacket.ReadBit();
        recvPacket.ReadGuidMask<0, 1>(moverGuid);
        hasFallData = recvPacket.ReadBit();

        hasPitch = !recvPacket.ReadBit();
        recvPacket.ReadGuidMask<6>(moverGuid);
        if (hasMoveFlags)
            flags = recvPacket.ReadBits(30);
        hasOrientation = !recvPacket.ReadBit();
        recvPacket.ReadBit();               // byte19C
        hasTransportData = recvPacket.ReadBit();
        if (hasTransportData)
        {
            recvPacket.ReadGuidMask<6, 0, 7>(transportGuid);
            hasTransportTime3 = recvPacket.ReadBit();
            recvPacket.ReadGuidMask<3, 2, 1, 5, 4>(transportGuid);
            hasTransportTime2 = recvPacket.ReadBit();
        }

        hasTimeStamp = !recvPacket.ReadBit();
        recvPacket.ReadGuidMask<7>(moverGuid);
        if (hasFallData)
            hasFallDirection = recvPacket.ReadBit();
        bool hasMoveFlags2 = !recvPacket.ReadBit();
        recvPacket.ReadBit();               // byte185
        hasSplineElevation = !recvPacket.ReadBit();
        recvPacket.ReadGuidMask<2>(moverGuid);
        if (hasMoveFlags2)
            flags2 = recvPacket.ReadBits(13);
        dword198 = !recvPacket.ReadBit();
        recvPacket.ReadGuidMask<5>(moverGuid);
        counter = recvPacket.ReadBits(22);
        recvPacket.ReadGuidMask<3>(moverGuid);
    }

    if (hasSrc)
        recvPacket.ReadGuidMask<2, 6, 4, 7, 0, 1, 3, 5>(srcTransportGuid);

    recvPacket.ReadGuidMask<0, 4, 3, 1, 6, 5, 7, 2>(objectTargetGuid);
    recvPacket.ReadGuidMask<3, 4, 2, 0, 7, 6, 5, 1>(itemTargetGuid);

    if (hasCastFlags)
        castFlags = recvPacket.ReadBits(5);
    if (hasTargetMask)
        targets.SetTargetMask(recvPacket.ReadBits(20));

    for (uint8 i = 0; i < archeologyCount; ++i)
    {
        switch (targets.m_weights[i].type)
        {
            case WEIGHT_KEYSTONE:
                recvPacket >> targets.m_weights[i].keystone.itemId;
                recvPacket >> targets.m_weights[i].keystone.itemCount;
                break;
            case WEIGHT_FRAGMENT:
                recvPacket >> targets.m_weights[i].fragment.currencyId;
                recvPacket >> targets.m_weights[i].fragment.currencyCount;
                break;
            default:
                recvPacket >> targets.m_weights[i].raw.id;
                recvPacket >> targets.m_weights[i].raw.count;
                break;
        }
    }

    recvPacket.ReadGuidBytes<4, 3, 5, 6, 0, 7, 2, 1>(itemTargetGuid);

    if (hasMovement)
    {
        recvPacket.ReadGuidBytes<3>(moverGuid);
        if (hasTransportData)
        {
            recvPacket.ReadGuidBytes<2>(transportGuid);
            recvPacket.read_skip<float>();      // transport Z
            recvPacket.ReadGuidBytes<3, 5>(transportGuid);
            recvPacket.read_skip<uint8>();      // transport seat
            recvPacket.ReadGuidBytes<6>(transportGuid);
            recvPacket.read_skip<float>();      // transport X
            recvPacket.read_skip<uint32>();     // transport time
            recvPacket.ReadGuidBytes<7, 1>(transportGuid);
            if (hasTransportTime3)
                recvPacket.read_skip<uint32>();
            recvPacket.read_skip<float>();      // transport Y
            if (hasTransportTime2)
                recvPacket.read_skip<uint32>();
            recvPacket.ReadGuidBytes<0>(transportGuid);
            recvPacket.read_skip<float>();      // transport O
            recvPacket.ReadGuidBytes<4>(transportGuid);
        }

        recvPacket.ReadGuidBytes<7, 1>(moverGuid);
        if (dword198)
            recvPacket.read_skip<uint32>();
        if (hasOrientation)
            recvPacket.read_skip<float>();
        if (hasTimeStamp)
            recvPacket.read_skip<uint32>();
        if (hasSplineElevation)
            recvPacket.read_skip<float>();
        if (hasFallData)
        {
            recvPacket.read_skip<float>();      // fall z speed
            if (hasFallDirection)
            {
                recvPacket.read_skip<float>();  // fall sin angle
                recvPacket.read_skip<float>();  // fall cos angle
                recvPacket.read_skip<float>();  // fall xy speed
            }
            recvPacket.read_skip<uint32>();     // fall time
        }
        recvPacket.read_skip<float>();          // position Y
        recvPacket.ReadGuidBytes<0, 6>(moverGuid);
        for (uint32 i = 0; i < counter; ++i)
            recvPacket.read_skip<uint32>();
        recvPacket.read_skip<float>();          // position Z
        recvPacket.ReadGuidBytes<2>(moverGuid);
        recvPacket.read_skip<float>();          // position X
        if (hasPitch)
            recvPacket.read_skip<float>();
        recvPacket.ReadGuidBytes<4, 5>(moverGuid);
    }

    if (hasDst)
    {
        if (dstTransportGuid)
            recvPacket >> targets.m_dst._transportOffset.m_positionZ;
        else
            recvPacket >> targets.m_dst._position.m_positionZ;

        recvPacket.ReadGuidBytes<1, 3, 7, 6, 0, 2>(dstTransportGuid);

        if (dstTransportGuid)
            recvPacket >> targets.m_dst._transportOffset.m_positionY;
        else
            recvPacket >> targets.m_dst._position.m_positionY;

        recvPacket.ReadGuidBytes<5>(dstTransportGuid);

        if (dstTransportGuid)
            recvPacket >> targets.m_dst._transportOffset.m_positionX;
        else
            recvPacket >> targets.m_dst._position.m_positionX;

        recvPacket.ReadGuidBytes<4>(dstTransportGuid);

        targets.m_dst._transportGUID = dstTransportGuid;
    }
    if (hasSrc)
    {
        if (srcTransportGuid)
            recvPacket >> targets.m_src._transportOffset.m_positionX;
        else
            recvPacket >> targets.m_src._position.m_positionX;

        recvPacket.ReadGuidBytes<5, 7, 0, 4>(srcTransportGuid);

        if (srcTransportGuid)
            recvPacket >> targets.m_src._transportOffset.m_positionZ;
        else
            recvPacket >> targets.m_src._position.m_positionZ;

        recvPacket.ReadGuidBytes<2, 1, 3>(srcTransportGuid);

        if (srcTransportGuid)
            recvPacket >> targets.m_src._transportOffset.m_positionY;
        else
            recvPacket >> targets.m_src._position.m_positionY;

        recvPacket.ReadGuidBytes<6>(srcTransportGuid);

        targets.m_src._transportGUID = srcTransportGuid;
    }

    recvPacket.ReadGuidBytes<7, 3, 2, 0, 4, 6, 5, 1>(objectTargetGuid);

    if (hasGlyphIndex)
        recvPacket >> glyphIndex;

    if (hasElevation)
        recvPacket >> targets.m_elevation;
    if (hasSpellId)
        recvPacket >> spellId;
    if (stringTargetLen)
        targets.m_strTarget = recvPacket.ReadString(stringTargetLen);
    if (hasSpeed)
        recvPacket >> targets.m_speed;
    if (hasCastCount)
        recvPacket >> castCount;

    targets.m_objectTargetGUID = objectTargetGuid;
    targets.m_itemTargetGUID = itemTargetGuid;
    targets.Update(_player->m_mover);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: got cast spell packet, castCount: %u, spellId: %u, glyphIndex %u, data length = %u", castCount, spellId, glyphIndex, (uint32)recvPacket.size());

    // ignore for remote control state (for player case)
    Unit* mover = _player->m_mover;
    if (mover != _player && mover->GetTypeId() == TYPEID_PLAYER)
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "WORLD: unknown spell id %u", spellId);
        return;
    }

    if (mover->GetTypeId() == TYPEID_PLAYER)
    {
        // not have spell in spellbook or spell passive and not casted by client
        if ((!mover->ToPlayer()->HasActiveSpell(spellId) || spellInfo->IsPassive()) && !spellInfo->ResearchProject && spellId != 101054 && !spellInfo->HasEffect(SPELL_EFFECT_OPEN_LOCK) &&
            !(spellInfo->AttributesEx8 & SPELL_ATTR8_RAID_MARKER))
        {
            if(spellId == 101603)
            {
                mover->RemoveAurasDueToSpell(107837);
                mover->RemoveAurasDueToSpell(101601);
            }
            else
            {
                //cheater? kick? ban?
                sLog->outError(LOG_FILTER_NETWORKIO, "WORLD: cheater? kick? ban? TYPEID_PLAYER spell id %u", spellId);
                return;
            }
        }
    }
    else
    {
        // not have spell in spellbook or spell passive and not casted by client
        if ((mover->GetTypeId() == TYPEID_UNIT && !mover->ToCreature()->HasSpell(spellId)) || spellInfo->IsPassive())
        {
            //cheater? kick? ban?
            return;
        }
    }

    // process spells overriden by SpecializationSpells.dbc
    for (std::set<SpecializationSpellEntry const*>::const_iterator itr = spellInfo->SpecializationOverrideSpellList.begin(); itr != spellInfo->SpecializationOverrideSpellList.end(); ++itr)
    {
        if (_player->HasSpell((*itr)->LearnSpell))
        {
            if (SpellInfo const* overrideSpellInfo = sSpellMgr->GetSpellInfo((*itr)->LearnSpell))
            {
                spellInfo = overrideSpellInfo;
                spellId = overrideSpellInfo->Id;
            }
            break;
        }
    }

    // process spellOverride column replacements of Talent.dbc
    if (Player* plMover = mover->ToPlayer())
    {
        PlayerTalentMap const* talents = plMover->GetTalentMap(plMover->GetActiveSpec());
        for (PlayerTalentMap::const_iterator itr = talents->begin(); itr != talents->end(); ++itr)
        {
            if (itr->second->state == PLAYERSPELL_REMOVED)
                continue;

            if (itr->second->talentEntry->spellOverride == spellId)
            {
                if (SpellInfo const* newInfo = sSpellMgr->GetSpellInfo(itr->second->talentEntry->spellId))
                {
                    spellInfo = newInfo;
                    spellId = newInfo->Id;
                }
                break;
            }
        }
    }

    Unit::AuraEffectList swaps = mover->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS);
    Unit::AuraEffectList const& swaps2 = mover->GetAuraEffectsByType(SPELL_AURA_OVERRIDE_ACTIONBAR_SPELLS_2);
    if (!swaps2.empty())
        swaps.insert(swaps.end(), swaps2.begin(), swaps2.end());

    if (!swaps.empty())
    {
        for (Unit::AuraEffectList::const_iterator itr = swaps.begin(); itr != swaps.end(); ++itr)
        {
            if ((*itr)->IsAffectingSpell(spellInfo))
            {
                if (SpellInfo const* newInfo = sSpellMgr->GetSpellInfo((*itr)->GetAmount()))
                {
                    _player->SwapSpellUncategoryCharges(spellId, newInfo->Id);
                    spellInfo = newInfo;
                    spellId = newInfo->Id;
                }
                break;
            }
        }
    }

    // Client is resending autoshot cast opcode when other spell is casted during shoot rotation
    // Skip it to prevent "interrupt" message
    if (spellInfo->IsAutoRepeatRangedSpell() && _player->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL)
        && _player->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL)->m_spellInfo == spellInfo)
        return;

    // can't use our own spells when we're in possession of another unit,
    if (_player->isPossessing())
        return;

    //HandleClientCastFlags(recvPacket, castFlags, targets);

    // auto-selection buff level base at target level (in spellInfo)
    if (targets.GetUnitTarget())
    {
        SpellInfo const* actualSpellInfo = spellInfo->GetAuraRankForLevel(targets.GetUnitTarget()->getLevel());

        // if rank not found then function return NULL but in explicit cast case original spell can be casted and later failed with appropriate error message
        if (actualSpellInfo)
            spellInfo = actualSpellInfo;
    }

    // Custom spell overrides
    // are most of these still needed?
    switch (spellInfo->Id)
    {
//         case 15407: // Mind Flay
//         {
//             if (Unit * target = targets.GetUnitTarget())
//             {
//                 if (_player->HasAura(139139) && target->HasAura(2944, _player->GetGUID()))
//                 {
//                     if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(129197))
//                     {
//                         spellInfo = newSpellInfo;
//                         spellId = newSpellInfo->Id;
//                     }
//                 }
//             }
//             break;
//         }
        case 18540: //Summon Terrorguard
        {
            if (_player->HasSpell(112927))
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(112927))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 1122: //Summon Abyssal
        {
            if (_player->HasSpell(112921))
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(112921))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 30146: //Summon Wrathguard
        {
            if (_player->HasSpell(112870))
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(112870))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 712: //Summon Shivarra
        {
            if (_player->HasSpell(112868))
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(112868))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 691: //Summon Observer
        {
            if (_player->HasSpell(112869))
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(112869))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 697: //Summon Voidlord
        {
            if (_player->HasSpell(112867))
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(112867))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 688: //Summon Fel Imp
        {
            if (_player->HasSpell(112866))
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(112866))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 19434:         // Aimed Shot - 19434 and Aimed Shot (for Master Marksman) - 82928
        {
            if (_player->HasAura(82926))
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(82928))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 113860:        // Fix Dark Soul for Destruction warlocks
        {
            if (_player->GetSpecializationId(_player->GetActiveSpec()) == SPEC_WARLOCK_DESTRUCTION)
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(113858))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 116467:        // Consecration - 116467 and Consecration - 26573
        {
            if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(26573))
            {
                spellInfo = newSpellInfo;
                spellId = newSpellInfo->Id;
            }
            break;
        }
        case 123273:        // Surging Mist - 123273 and Surging Mist - 116995
        case 116694:        // Surging Mist - 116694 and Surging Mist - 116995
        {
            // Surging Mist is instantly casted if player is channeling Soothing Mist
            if (_player->GetCurrentSpell(CURRENT_CHANNELED_SPELL) && _player->GetCurrentSpell(CURRENT_CHANNELED_SPELL)->GetSpellInfo()->Id == 115175)
            {
                recvPacket.rfinish();
                _player->CastSpell(targets.GetUnitTarget(), 116995, true);
                _player->EnergizeBySpell(_player, 116995, 1, POWER_CHI);
                int32 powerCost = spellInfo->CalcPowerCost(_player, spellInfo->GetSchoolMask());
                _player->ModifyPower(POWER_MANA, -powerCost, true);
                return;
            }
            break;
        }
        case 120517:         // Halo - 120517 and Halo - 120644 (shadow form)
        {
            if (_player->HasAura(15473))
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(120644))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 121135:        // Cascade (shadow) - 127632 and Cascade - 121135
        {
            if (_player->HasAura(15473))
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(127632))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 124682:        // Enveloping Mist - 124682 and Enveloping Mist - 132120
        {
            // Enveloping Mist is instantly casted if player is channeling Soothing Mist
            if (_player->GetCurrentSpell(CURRENT_CHANNELED_SPELL) && _player->GetCurrentSpell(CURRENT_CHANNELED_SPELL)->GetSpellInfo()->Id == 115175)
            {
                recvPacket.rfinish();
                _player->CastSpell(targets.GetUnitTarget(), 132120, true);
                int32 powerCost = spellInfo->CalcPowerCost(_player, spellInfo->GetSchoolMask());
                _player->ModifyPower(POWER_CHI, -powerCost, true);
                return;
            }
            break;
        }
        case 126892:        // Zen Pilgrimage - 126892 and Zen Pilgrimage : Return - 126895
        {
            if (_player->HasAura(126896))
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(126895))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 127538:        // Savage Roar
        {
            if (_player->GetComboPoints())
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(52610))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
        case 129250:        // Power Word : Solace - 129250 and Power Word : Insanity - 129249
        {
            if (_player->GetShapeshiftForm() == FORM_SHADOW)
            {
                if (SpellInfo const* newSpellInfo = sSpellMgr->GetSpellInfo(129249))
                {
                    spellInfo = newSpellInfo;
                    spellId = newSpellInfo->Id;
                }
            }
            break;
        }
    }

    Spell* spell = new Spell(mover, spellInfo, TRIGGERED_NONE, 0, false);
    spell->m_cast_count = castCount;                       // set count of casts (5.0.5 disable client crash 132)
    spell->m_glyphIndex = glyphIndex;
    spell->prepare(&targets);
}

void WorldSession::HandleCancelCastOpcode(WorldPacket& recvPacket)
{
    uint32 spellId;

    bool hasSpell = !recvPacket.ReadBit();
    bool hasCastCount = !recvPacket.ReadBit();
    if (hasCastCount)
        recvPacket.read_skip<uint8>();                      // counter, increments with every CANCEL packet, don't use for now
    if (hasSpell)
        recvPacket >> spellId;

    if (!spellId)
        return;

    if (_player->IsNonMeleeSpellCasted(false))
        _player->InterruptNonMeleeSpells(false, spellId, false);
}

void WorldSession::HandleCancelAuraOpcode(WorldPacket& recvPacket)
{
    ObjectGuid guid;
    uint32 spellId;
    recvPacket >> spellId;

    recvPacket.ReadBit();   // guid marker
    recvPacket.ReadGuidMask<0, 4, 7, 1, 6, 2, 3, 5>(guid);
    recvPacket.ReadGuidBytes<4, 2, 7, 1, 5, 0, 3, 6>(guid);

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
        return;

    // not allow remove spells with attr SPELL_ATTR0_CANT_CANCEL
    if (spellInfo->Attributes & SPELL_ATTR0_CANT_CANCEL)
        return;

    // channeled spell case (it currently casted then)
    if (spellInfo->IsChanneled())
    {
        if (Spell* curSpell = _player->GetCurrentSpell(CURRENT_CHANNELED_SPELL))
            if (curSpell->m_spellInfo->Id == spellId)
                _player->InterruptSpell(CURRENT_CHANNELED_SPELL);
        return;
    }

    // non channeled case:
    // don't allow remove non positive spells
    // don't allow cancelling passive auras (some of them are visible)
    if (!spellInfo->IsPositive() || spellInfo->IsPassive())
        return;

    // maybe should only remove one buff when there are multiple?
    _player->RemoveOwnedAura(spellId, 0, 0, AURA_REMOVE_BY_CANCEL);
}

//! 5.4.1
void WorldSession::HandlePetCancelAuraOpcode(WorldPacket& recvPacket)
{
    ObjectGuid guid;
    uint32 spellId;

    recvPacket >> spellId;

    recvPacket.WriteGuidMask<7, 2, 6, 4, 1, 5, 0, 3>(guid);
    recvPacket.WriteGuidBytes<0, 2, 3, 7, 4, 1, 6, 5>(guid);

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "WORLD: unknown PET spell id %u", spellId);
        return;
    }

    Creature* pet=ObjectAccessor::GetCreatureOrPetOrVehicle(*_player, guid);

    if (!pet)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "HandlePetCancelAura: Attempt to cancel an aura for non-existant pet %u by player '%s'", uint32(GUID_LOPART(guid)), GetPlayer()->GetName());
        return;
    }

    if (pet != GetPlayer()->GetGuardianPet() && pet != GetPlayer()->GetCharm())
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "HandlePetCancelAura: Pet %u is not a pet of player '%s'", uint32(GUID_LOPART(guid)), GetPlayer()->GetName());
        return;
    }

    if (!pet->isAlive())
    {
        pet->SendPetActionFeedback(FEEDBACK_PET_DEAD);
        return;
    }

    pet->RemoveOwnedAura(spellId, 0, 0, AURA_REMOVE_BY_CANCEL);

    pet->AddCreatureSpellCooldown(spellId);
}

void WorldSession::HandleCancelGrowthAuraOpcode(WorldPacket& /*recvPacket*/)
{
}

void WorldSession::HandleCancelAutoRepeatSpellOpcode(WorldPacket& /*recvPacket*/)
{
    // may be better send SMSG_CANCEL_AUTO_REPEAT?
    // cancel and prepare for deleting
    _player->InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
}

void WorldSession::HandleCancelChanneling(WorldPacket& recvData)
{
    recvData.read_skip<uint32>();                          // spellid, not used

    // ignore for remote control state (for player case)
    Unit* mover = _player->m_mover;
    if (mover != _player && mover->GetTypeId() == TYPEID_PLAYER)
        return;

    mover->InterruptSpell(CURRENT_CHANNELED_SPELL);
}

void WorldSession::HandleTotemDestroyed(WorldPacket& recvPacket)
{
    // ignore for remote control state
    if (_player->m_mover != _player)
        return;

    uint8 slotId;
    ObjectGuid guid;

    recvPacket >> slotId;
    recvPacket.ReadGuidMask<3, 5, 2, 0, 4, 1, 6, 7>(guid);
    recvPacket.ReadGuidBytes<4, 1, 5, 2, 3, 6, 7, 0>(guid);
    uint64 logGuid = guid;

    ++slotId;

    if (slotId >= MAX_TOTEM_SLOT)
        return;

    if (!_player->m_SummonSlot[slotId])
        return;

    if(Creature* summon = GetPlayer()->GetMap()->GetCreature(_player->m_SummonSlot[slotId]))
    {
        if(uint32 spellId = summon->GetUInt32Value(UNIT_CREATED_BY_SPELL))
            if(AreaTrigger* arTrigger = _player->GetAreaObject(spellId))
                arTrigger->SetDuration(0);
        summon->DespawnOrUnsummon();
    }
}

void WorldSession::HandleSelfResOpcode(WorldPacket& /*recvData*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_SELF_RES");                  // empty opcode

    if (_player->HasAuraType(SPELL_AURA_PREVENT_RESURRECTION))
        return; // silent return, client should display error by itself and not send this opcode

    if (_player->GetUInt32Value(PLAYER_SELF_RES_SPELL))
    {
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(_player->GetUInt32Value(PLAYER_SELF_RES_SPELL));
        if (spellInfo)
            _player->CastSpell(_player, spellInfo, false, 0);

        _player->SetUInt32Value(PLAYER_SELF_RES_SPELL, 0);
    }
}

void WorldSession::HandleSpellClick(WorldPacket& recvData)
{
    time_t now = time(NULL);
    if (now - timeLastHandleSpellClick < 2)
    {
        recvData.rfinish();
        return;
    }
    else
       timeLastHandleSpellClick = now;

    ObjectGuid guid;
    recvData.ReadGuidMask<1, 7, 2, 5, 0, 6>(guid);
    recvData.ReadBit();
    recvData.ReadGuidMask<3, 4>(guid);

    recvData.ReadGuidBytes<2, 3, 6, 5, 4, 1, 0, 7>(guid);

    // this will get something not in world. crash
    Creature* unit = ObjectAccessor::GetCreatureOrPetOrVehicle(*_player, guid);

    if (!unit)
        return;

    // TODO: Unit::SetCharmedBy: 28782 is not in world but 0 is trying to charm it! -> crash
    if (!unit->IsInWorld())
        return;

    // flags in Deepwind Gorge
    if (unit->GetEntry() == 53194)
    {
        _player->CastSpell(unit, unit->GetInt32Value(UNIT_FIELD_INTERACT_SPELL_ID));
        return;
    }

    unit->HandleSpellClick(_player);
}

void WorldSession::HandleMirrorImageDataRequest(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_GET_MIRRORIMAGE_DATA");
    ObjectGuid guid;
    uint32 displayId;

    recvData >> displayId;
    recvData.ReadGuidMask<0, 2, 5, 6, 4, 3, 1, 7>(guid);
    recvData.ReadGuidBytes<2, 0, 1, 3, 7, 6, 5, 4>(guid);

    // Get unit for which data is needed by client
    Unit* unit = ObjectAccessor::GetObjectInWorld(guid, (Unit*)NULL);
    if (!unit)
        return;

    if (!unit->HasAuraType(SPELL_AURA_CLONE_CASTER))
        return;

    // Get creator of the unit (SPELL_AURA_CLONE_CASTER does not stack)
    Unit* creator = unit->GetAuraEffectsByType(SPELL_AURA_CLONE_CASTER).front()->GetCaster();
    if (!creator)
    {
        sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_GET_MIRRORIMAGE_DATA displayId %u, creator not found", displayId);
        creator = unit;
    }

    Player* player = creator->ToPlayer();
    ObjectGuid guildGuid;
    if (uint32 guildId = player ? player->GetGuildId() : 0)
        if (Guild* guild = sGuildMgr->GetGuildById(guildId))
            guildGuid = guild->GetGUID();

    WorldPacket data(SMSG_MIRRORIMAGE_DATA, 80);
    data.WriteGuidMask<5>(guildGuid);
    data.WriteGuidMask<0, 5>(guid);
    data.WriteGuidMask<4>(guildGuid);
    data.WriteGuidMask<7, 1, 4>(guid);
    data.WriteGuidMask<3, 7, 2>(guildGuid);
    data.WriteGuidMask<6>(guid);
    data.WriteGuidMask<1, 6>(guildGuid);
    data.WriteGuidMask<3>(guid);

    uint32 slotCount = 0;
    uint32 bitpos = data.bitwpos();
    data.WriteBits(slotCount, 22);

    data.WriteGuidMask<2>(guid);
    data.WriteGuidMask<0>(guildGuid);

    data << uint8(player ? player->GetByteValue(PLAYER_BYTES, 1) : 0);   // face
    data.WriteGuidBytes<2, 7>(guid);
    data.WriteGuidBytes<6, 1>(guildGuid);
    data << uint8(creator->getGender());
    data << uint8(creator->getClass());
    data.WriteGuidBytes<1>(guid);
    data.WriteGuidBytes<0>(guildGuid);
    data << uint8(player ? player->GetByteValue(PLAYER_BYTES, 2) : 0);   // hair
    data.WriteGuidBytes<5, 0>(guid);
    data.WriteGuidBytes<4, 2>(guildGuid);

    static EquipmentSlots const itemSlots[] =
    {
        EQUIPMENT_SLOT_HEAD,
        EQUIPMENT_SLOT_SHOULDERS,
        EQUIPMENT_SLOT_BODY,
        EQUIPMENT_SLOT_CHEST,
        EQUIPMENT_SLOT_WAIST,
        EQUIPMENT_SLOT_LEGS,
        EQUIPMENT_SLOT_FEET,
        EQUIPMENT_SLOT_WRISTS,
        EQUIPMENT_SLOT_HANDS,
        EQUIPMENT_SLOT_TABARD,
        EQUIPMENT_SLOT_BACK,
        EQUIPMENT_SLOT_END
    };

    // Display items in visible slots
    for (EquipmentSlots const* itr = &itemSlots[0]; *itr != EQUIPMENT_SLOT_END; ++itr)
    {
        if (!player)
            data << uint32(0);
        else if (*itr == EQUIPMENT_SLOT_HEAD && player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_HELM))
            data << uint32(0);
        else if (*itr == EQUIPMENT_SLOT_BACK && player->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_HIDE_CLOAK))
            data << uint32(0);
        else if (Item const* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, *itr))
            data << uint32(item->GetTemplate()->DisplayInfoID);
        else
            data << uint32(0);
        ++slotCount;
    }

    data << uint8(creator->getRace());
    data.WriteGuidBytes<4>(guid);
    data.WriteGuidBytes<7>(guildGuid);
    data << uint8(player ? player->GetByteValue(PLAYER_BYTES_2, 0) : 0); // facialhair
    data.WriteGuidBytes<6>(guid);
    data << uint32(creator->GetDisplayId());
    data << uint8(player ? player->GetByteValue(PLAYER_BYTES, 0) : 0);   // skin
    data.WriteGuidBytes<3>(guid);
    data.WriteGuidBytes<3>(guildGuid);
    data << uint8(player ? player->GetByteValue(PLAYER_BYTES, 3) : 0);   // haircolor
    data.WriteGuidBytes<5>(guildGuid);

    data.PutBits<uint32>(bitpos, slotCount, 22);

    SendPacket(&data);
}

void WorldSession::HandleUpdateProjectilePosition(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_UPDATE_PROJECTILE_POSITION");

    uint64 casterGuid;
    uint32 spellId;
    uint8 castCount;
    float x, y, z;    // Position of missile hit

    recvPacket >> casterGuid;
    recvPacket >> spellId;
    recvPacket >> castCount;
    recvPacket >> x;
    recvPacket >> y;
    recvPacket >> z;

    Unit* caster = ObjectAccessor::GetUnit(*_player, casterGuid);
    if (!caster)
        return;

    Spell* spell = caster->FindCurrentSpellBySpellId(spellId);
    if (!spell || !spell->m_targets.HasDst())
        return;

    Position pos = *spell->m_targets.GetDstPos();
    pos.Relocate(x, y, z);
    spell->m_targets.ModDst(pos);

    WorldPacket data(SMSG_SET_PROJECTILE_POSITION, 21);
    data << uint64(casterGuid);
    data << uint8(castCount);
    data << float(x);
    data << float(y);
    data << float(z);
    caster->SendMessageToSet(&data, true);
}

void WorldSession::HandleUpdateMissileTrajectory(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: CMSG_UPDATE_MISSILE_TRAJECTORY");

    uint64 guid;
    uint32 spellId;
    float elevation, speed;
    float curX, curY, curZ;
    float targetX, targetY, targetZ;
    uint8 moveStop;

    recvPacket >> guid >> spellId >> elevation >> speed;
    recvPacket >> curX >> curY >> curZ;
    recvPacket >> targetX >> targetY >> targetZ;
    recvPacket >> moveStop;

    Unit* caster = ObjectAccessor::GetUnit(*_player, guid);
    Spell* spell = caster ? caster->GetCurrentSpell(CURRENT_GENERIC_SPELL) : NULL;
    if (!spell || spell->m_spellInfo->Id != spellId || !spell->m_targets.HasDst() || !spell->m_targets.HasSrc())
    {
        recvPacket.rfinish();
        return;
    }

    Position pos = *spell->m_targets.GetSrcPos();
    pos.Relocate(curX, curY, curZ);
    spell->m_targets.ModSrc(pos);

    pos = *spell->m_targets.GetDstPos();
    pos.Relocate(targetX, targetY, targetZ);
    spell->m_targets.ModDst(pos);

    spell->m_targets.SetElevation(elevation);
    spell->m_targets.SetSpeed(speed);

    if (moveStop)
    {
        uint32 opcode;
        recvPacket >> opcode;
        recvPacket.SetOpcode(MSG_MOVE_STOP); // always set to MSG_MOVE_STOP in client SetOpcode
        HandleMovementOpcodes(recvPacket);
    }
}

void WorldSession::HandlerCategoryCooldownOpocde(WorldPacket& recvPacket)
{
    _player->SendCategoryCooldownMods();
}
