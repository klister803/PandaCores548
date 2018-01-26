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
#include "WorldSession.h"
#include "ObjectMgr.h"
#include "SpellMgr.h"
#include "Log.h"
#include "Opcodes.h"
#include "Spell.h"
#include "ObjectAccessor.h"
#include "CreatureAI.h"
#include "Util.h"
#include "Pet.h"
#include "World.h"
#include "Group.h"
#include "SpellInfo.h"
#include "SpellAuraEffects.h"

void WorldSession::HandleDismissCritter(WorldPacket& recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<6, 2, 3, 7, 0, 4, 1, 5>(guid);
    recvData.ReadGuidBytes<5, 3, 1, 6, 4, 7, 0, 2>(guid);

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_DISMISS_CRITTER for GUID " UI64FMTD, uint64(guid));

    Unit* pet = ObjectAccessor::GetCreatureOrPetOrVehicle(*_player, guid);

    if (!pet)
    {
        TC_LOG_DEBUG("network", "Vanitypet (guid: %u) does not exist - player '%s' (guid: %u / account: %u) attempted to dismiss it (possibly lagged out)",
                uint32(GUID_LOPART(guid)), GetPlayer()->GetName(), GetPlayer()->GetGUIDLow(), GetAccountId());
        return;
    }

    if (_player->GetCritterGUID() == pet->GetGUID())
    {
         if (pet->GetTypeId() == TYPEID_UNIT && pet->ToCreature()->isSummon())
             pet->ToTempSummon()->UnSummon();
    }
}

//! 5.4.1
void WorldSession::HandlePetAction(WorldPacket & recvData)
{
    ObjectGuid guid1;   //pet guid
    ObjectGuid guid2;   //tag guid
    uint32 data;
    float x, y, z;

    recvData >> z;
    recvData >> data;
    recvData >> y;
    recvData >> x;

    guid2[2] = recvData.ReadBit() != 0;
    guid1[0] = recvData.ReadBit() != 0;
    guid2[1] = recvData.ReadBit() != 0;
    guid1[4] = recvData.ReadBit() != 0;
    guid2[7] = recvData.ReadBit() != 0;
    guid2[6] = recvData.ReadBit() != 0;
    guid1[1] = recvData.ReadBit() != 0;
    guid1[5] = recvData.ReadBit() != 0;
    guid1[2] = recvData.ReadBit() != 0;
    guid1[7] = recvData.ReadBit() != 0;
    guid2[4] = recvData.ReadBit() != 0;
    guid1[6] = recvData.ReadBit() != 0;
    guid2[5] = recvData.ReadBit() != 0;
    guid2[3] = recvData.ReadBit() != 0;
    guid2[0] = recvData.ReadBit() != 0;
    guid1[3] = recvData.ReadBit() != 0;
   
    recvData.ReadByteSeq(guid1[7]);
    recvData.ReadByteSeq(guid1[2]);
    recvData.ReadByteSeq(guid2[4]);
    recvData.ReadByteSeq(guid2[0]);
    recvData.ReadByteSeq(guid2[6]);
    recvData.ReadByteSeq(guid1[6]);
    recvData.ReadByteSeq(guid1[5]);
    recvData.ReadByteSeq(guid2[2]);
    recvData.ReadByteSeq(guid2[7]);
    recvData.ReadByteSeq(guid1[0]);
    recvData.ReadByteSeq(guid2[3]);
    recvData.ReadByteSeq(guid1[1]);
    recvData.ReadByteSeq(guid1[4]);
    recvData.ReadByteSeq(guid1[3]);
    recvData.ReadByteSeq(guid2[5]);
    recvData.ReadByteSeq(guid2[1]);

    uint32 spellid = UNIT_ACTION_BUTTON_ACTION(data);
    uint8 flag = UNIT_ACTION_BUTTON_TYPE(data);             //delete = 0x07 CastSpell = C1

    // used also for charmed creature
    Unit* pet= ObjectAccessor::GetUnit(*_player, guid1);
    TC_LOG_DEBUG("network", "HandlePetAction: Pet %u - flag: %u, spellid: %u, target: %u.", uint32(GUID_LOPART(guid1)), uint32(flag), spellid, uint32(GUID_LOPART(guid2)));

    if (!pet)
    {
        TC_LOG_ERROR("network", "HandlePetAction: Pet (GUID: %u) doesn't exist for player '%s'", uint32(GUID_LOPART(guid1)), GetPlayer()->GetName());
        return;
    }

    if (pet != GetPlayer()->GetFirstControlled())
    {
        TC_LOG_ERROR("network", "HandlePetAction: Pet (GUID: %u) does not belong to player '%s'", uint32(GUID_LOPART(guid1)), GetPlayer()->GetName());
        return;
    }

    if (!pet->IsAlive())
    {
        SpellInfo const* spell = (flag == ACT_ENABLED || flag == ACT_PASSIVE) ? sSpellMgr->GetSpellInfo(spellid) : NULL;
        if (!spell)
            return;
        if (!(spell->Attributes & SPELL_ATTR0_CASTABLE_WHILE_DEAD))
            return;
    }

    //TODO: allow control charmed player?
    if (pet->GetTypeId() == TYPEID_PLAYER && !(flag == ACT_COMMAND && spellid == COMMAND_ATTACK))
        return;

    if (GetPlayer()->m_Controlled.size() == 1)
        HandlePetActionHelper(pet, guid1, spellid, flag, guid2, x, y ,z);
    else
    {
        //If a pet is dismissed, m_Controlled will change
        std::vector<Unit*> controlled;
        for (Unit::ControlList::iterator itr = GetPlayer()->m_Controlled.begin(); itr != GetPlayer()->m_Controlled.end(); ++itr)
            if ((*itr)->GetEntry() == pet->GetEntry() && (*itr)->IsAlive())
            {
                if((*itr)->ToCreature())
                {
                    if(!(*itr)->ToCreature()->m_Stampeded && (*itr)->HasUnitTypeMask(UNIT_MASK_CONTROLABLE_GUARDIAN))
                        controlled.push_back(*itr);
                }
                else
                    controlled.push_back(*itr);
            }
        for (std::vector<Unit*>::iterator itr = controlled.begin(); itr != controlled.end(); ++itr)
            HandlePetActionHelper(*itr, guid1, spellid, flag, guid2, x, y, z);
    }
}

void WorldSession::HandlePetStopAttack(WorldPacket &recvData)
{
    ObjectGuid guid;
    recvData.ReadGuidMask<3, 2, 0, 6, 1, 4, 5, 7>(guid);
    recvData.ReadGuidBytes<3, 1, 2, 6, 5, 4, 0, 7>(guid);

    TC_LOG_DEBUG("network", "WORLD: Received CMSG_PET_STOP_ATTACK for GUID %u", uint32(GUID_LOPART(guid)));

    Unit* pet = ObjectAccessor::GetCreatureOrPetOrVehicle(*_player, guid);

    if (!pet)
    {
        TC_LOG_ERROR("network", "HandlePetStopAttack: Pet %u does not exist", uint32(GUID_LOPART(guid)));
        return;
    }

    if (pet != GetPlayer()->GetPet() && pet != GetPlayer()->GetCharm())
    {
        TC_LOG_ERROR("network", "HandlePetStopAttack: Pet GUID %u isn't a pet or charmed creature of player %s", uint32(GUID_LOPART(guid)), GetPlayer()->GetName());
        return;
    }

    if (!pet->IsAlive())
        return;

    pet->AttackStop();
}

void WorldSession::HandlePetActionHelper(Unit* pet, uint64 guid1, uint32 spellid, uint16 flag, uint64 guid2, float x, float y, float z)
{
    CharmInfo* charmInfo = pet->GetCharmInfo();
    if (!charmInfo)
    {
        TC_LOG_ERROR("network", "WorldSession::HandlePetAction(petGuid: " UI64FMTD ", tagGuid: " UI64FMTD ", spellId: %u, flag: %u): object (entry: %u TypeId: %u) is considered pet-like but doesn't have a charminfo!",
            guid1, guid2, spellid, flag, pet->GetGUIDLow(), pet->GetTypeId());
        return;
    }

    switch (flag)
    {
        case ACT_COMMAND:                                   //0x07
            switch (spellid)
            {
                case COMMAND_STAY:                          //flat=1792  //STAY
                    pet->StopMoving();
                    pet->GetMotionMaster()->Clear(false);
                    pet->GetMotionMaster()->MoveIdle();
                    charmInfo->SetCommandState(COMMAND_STAY);

                    charmInfo->SetIsCommandAttack(false);
                    charmInfo->SetIsAtStay(true);
                    charmInfo->SetIsFollowing(false);
                    charmInfo->SetIsReturning(false);
                    charmInfo->SaveStayPosition();
                    break;
                case COMMAND_FOLLOW:                        //spellid=1792  //FOLLOW
                    pet->AttackStop();
                    pet->InterruptNonMeleeSpells(false);
                    charmInfo->SetCommandState(COMMAND_FOLLOW);
                    charmInfo->SetIsCommandAttack(false);
                    charmInfo->SetIsAtStay(false);
                    charmInfo->SetIsReturning(true);
                    charmInfo->SetIsFollowing(true);
                    if (Creature* crt = pet->ToCreature())
                        crt->SetNeedToUpdatePetFollowPosition(true);
                    break;
                case COMMAND_ATTACK:                        //spellid=1792  //ATTACK
                {
                    // Can't attack if owner is pacified
                    if (_player->HasAuraType(SPELL_AURA_MOD_PACIFY))
                    {
                        //pet->SendPetCastFail(spellid, SPELL_FAILED_PACIFIED);
                        //TODO: Send proper error message to client
                        return;
                    }

                    // only place where pet can be player
                    Unit* TargetUnit = ObjectAccessor::GetUnit(*_player, guid2);
                    if (!TargetUnit)
                        return;

                    if (Unit* owner = pet->GetOwner())
                        if (!owner->IsValidAttackTarget(TargetUnit))
                            return;

                    // Not let attack through obstructions
                    if (sWorld->getBoolConfig(CONFIG_PET_LOS))
                    {
                        if (!pet->IsWithinLOSInMap(TargetUnit))
                            return;
                    }

                    pet->ClearUnitState(UNIT_STATE_FOLLOW);
                    // This is true if pet has no target or has target but targets differs.
                    if (pet->getVictim() != TargetUnit || (pet->getVictim() == TargetUnit && !pet->GetCharmInfo()->IsCommandAttack()))
                    {
                        if (pet->getVictim())
                            pet->AttackStop();

                        if (pet->GetTypeId() != TYPEID_PLAYER && pet->ToCreature()->IsAIEnabled)
                        {
                            charmInfo->SetIsCommandAttack(true);
                            charmInfo->SetIsAtStay(false);
                            charmInfo->SetIsFollowing(false);
                            charmInfo->SetIsReturning(false);

                            pet->ToCreature()->AI()->AttackStart(TargetUnit);

                            //10% chance to play special pet attack talk, else growl
                            if (pet->ToCreature()->IsPet() && ((Pet*)pet)->getPetType() == SUMMON_PET && pet != TargetUnit && urand(0, 100) < 10)
                                pet->SendPetTalk((uint32)PET_TALK_ATTACK);
                            else
                            {
                                // 90% chance for pet and 100% chance for charmed creature
                                pet->SendPetAIReaction(guid1);
                            }
                        }
                        else                                // charmed player
                        {
                            if (pet->getVictim() && pet->getVictim() != TargetUnit)
                                pet->AttackStop();

                            charmInfo->SetIsCommandAttack(true);
                            charmInfo->SetIsAtStay(false);
                            charmInfo->SetIsFollowing(false);
                            charmInfo->SetIsReturning(false);

                            pet->Attack(TargetUnit, true);
                            pet->SendPetAIReaction(guid1);
                        }
                    }
                    break;
                }
                case COMMAND_ABANDON:                       // abandon (hunter pet) or dismiss (summoned pet)
                    if (pet->GetCharmerGUID() == GetPlayer()->GetGUID())
                        _player->StopCastingCharm();
                    else if (pet->GetOwnerGUID() == GetPlayer()->GetGUID())
                    {
                        ASSERT(pet->GetTypeId() == TYPEID_UNIT);
                        if (pet->IsPet())
                            GetPlayer()->RemovePet((Pet*)pet);
                        else if (pet->HasUnitTypeMask(UNIT_MASK_MINION))
                        {
                            ((Minion*)pet)->UnSummon();
                        }
                    }
                    break;
                case COMMAND_MOVE_TO:
                {
                    pet->AttackStop();

                    if (!pet->HasUnitState(UNIT_STATE_LOST_CONTROL | UNIT_STATE_NOT_MOVE))
                    {
                        pet->StopMoving();
                        pet->GetMotionMaster()->Clear(false);
                        charmInfo->SetIsCommandAttack(false);
                        charmInfo->SetIsAtStay(true);
                        charmInfo->SetIsFollowing(false);
                        charmInfo->SetIsReturning(false);
                    }
                    charmInfo->SetCommandState(COMMAND_MOVE_TO);
                    charmInfo->SetStayPositionX(x);
                    charmInfo->SetStayPositionY(y);
                    charmInfo->SetStayPositionZ(z);
                    break;
                }
                default:
                    TC_LOG_ERROR("network", "WORLD: unknown PET flag Action %i and spellid %i.", uint32(flag), spellid);
            }
            break;
        case ACT_REACTION:                                  // 0x6
            switch (spellid)
            {
                case REACT_PASSIVE:                         //passive
                    pet->AttackStop();
                    //pet->GetMotionMaster()->Clear();
                    if (Creature* crt = pet->ToCreature())
                        crt->SetNeedToUpdatePetFollowPosition(true);
                    charmInfo->SetIsReturning(true);
                    charmInfo->SetIsFollowing(true);
                case REACT_DEFENSIVE:                       //recovery
                case REACT_AGGRESSIVE:
                {
                    if (pet->GetTypeId() == TYPEID_UNIT)
                        pet->ToCreature()->SetReactState(ReactStates(spellid));
                    break;
                }
                case REACT_HELPER:
                {
                    if (pet->GetTypeId() == TYPEID_UNIT)
                        pet->ToCreature()->SetReactState(ReactStates(spellid));

                    charmInfo->SetIsReturning(false);
                    charmInfo->SetIsFollowing(false);
                    charmInfo->SetIsAtStay(false);
                    charmInfo->SetIsCommandAttack(false);
                }
                default:
                    break;
            }
            break;
        case ACT_DISABLED:                                  // 0x81    spell (disabled), ignore
        case ACT_PASSIVE:                                   // 0x01
        case ACT_ENABLED:                                   // 0xC1    spell
        {
            Unit* unit_target = NULL;

            if (guid2)
                unit_target = ObjectAccessor::GetUnit(*_player, guid2);

            // do not cast unknown spells
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
            if (!spellInfo)
            {
                TC_LOG_ERROR("network", "WORLD: unknown PET spell id %i", spellid);
                return;
            }

            if (spellInfo->StartRecoveryCategory > 0)
                if (pet->GetCharmInfo() && pet->GetCharmInfo()->GetGlobalCooldownMgr().HasGlobalCooldown(spellInfo))
                    return;

            for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
            {
                if (spellInfo->Effects[i]->TargetA.GetTarget() == TARGET_UNIT_SRC_AREA_ENEMY || spellInfo->Effects[i]->TargetA.GetTarget() == TARGET_UNIT_DEST_AREA_ENEMY || spellInfo->Effects[i]->TargetA.GetTarget() == TARGET_DEST_DYNOBJ_ENEMY)
                    return;
            }

            // do not cast not learned spells
            if (!pet->HasSpell(spellid) || spellInfo->IsPassive())
                return;

            //  Clear the flags as if owner clicked 'attack'. AI will reset them
            //  after AttackStart, even if spell failed
            if (pet->GetCharmInfo())
            {
                pet->GetCharmInfo()->SetIsAtStay(false);
                pet->GetCharmInfo()->SetIsCommandAttack(true);
                pet->GetCharmInfo()->SetIsReturning(false);
                pet->GetCharmInfo()->SetIsFollowing(false);
            }

            Spell* spell = new Spell(pet, spellInfo, TRIGGERED_NONE);

            SpellCastResult result = spell->CheckPetCast(unit_target);

            //auto turn to target unless possessed
            if (result == SPELL_FAILED_UNIT_NOT_INFRONT && !pet->isPossessed() && !pet->IsVehicle())
            {
                if (unit_target)
                {
                    pet->SetInFront(unit_target);
                    if (unit_target->GetTypeId() == TYPEID_PLAYER)
                        pet->SendUpdateToPlayer((Player*)unit_target);
                }
                else if (Unit* unit_target2 = spell->m_targets.GetUnitTarget())
                {
                    pet->SetInFront(unit_target2);
                    if (unit_target2->GetTypeId() == TYPEID_PLAYER)
                        pet->SendUpdateToPlayer((Player*)unit_target2);
                }

                if (Unit* powner = pet->GetCharmerOrOwner())
                    if (powner->GetTypeId() == TYPEID_PLAYER)
                        pet->SendUpdateToPlayer(powner->ToPlayer());

                result = SPELL_CAST_OK;
            }

            if (result == SPELL_CAST_OK)
            {
                pet->ToCreature()->AddCreatureSpellCooldown(spellid);

                unit_target = spell->m_targets.GetUnitTarget();

                //10% chance to play special pet attack talk, else growl
                //actually this only seems to happen on special spells, fire shield for imp, torment for voidwalker, but it's stupid to check every spell
                if (pet->ToCreature()->IsPet() && (((Pet*)pet)->getPetType() == SUMMON_PET) && (pet != unit_target) && (urand(0, 100) < 10))
                    pet->SendPetTalk((uint32)PET_TALK_SPECIAL_SPELL);
                else
                {
                    pet->SendPetAIReaction(guid1);
                }

                if (unit_target && !GetPlayer()->IsFriendlyTo(unit_target) && !pet->isPossessed() && !pet->IsVehicle())
                {
                    // This is true if pet has no target or has target but targets differs.
                    if (pet->getVictim() != unit_target)
                    {
                        if (pet->getVictim())
                            pet->AttackStop();
                        pet->GetMotionMaster()->Clear();
                        if (pet->ToCreature()->IsAIEnabled)
                            pet->ToCreature()->AI()->AttackStart(unit_target);
                    }
                }

                spell->prepare(&(spell->m_targets));
            }
            else
            {
                if (pet->isPossessed() || pet->IsVehicle())
                    Spell::SendCastResult(GetPlayer(), spellInfo, 0, result);
                else
                    pet->SendPetCastFail(spellid, result);

                if (!pet->ToCreature()->HasSpellCooldown(spellid))
                    GetPlayer()->SendClearCooldown(spellid, pet);

                spell->finish(false);
                delete spell;

                // reset specific flags in case of spell fail. AI will reset other flags
                if (pet->GetCharmInfo())
                    pet->GetCharmInfo()->SetIsCommandAttack(false);
            }
            break;
        }
        default:
            TC_LOG_ERROR("network", "WORLD: unknown PET flag Action %i and spellid %i.", uint32(flag), spellid);
    }
}

//! 5.4.1
void WorldSession::HandlePetNameQuery(WorldPacket & recvData)
{
    TC_LOG_DEBUG("network", "HandlePetNameQuery. CMSG_PET_NAME_QUERY");

    ObjectGuid petGuid;
    ObjectGuid petNumber;

    recvData.ReadGuidMask<0>(petGuid);
    recvData.ReadGuidMask<0, 5, 2>(petNumber);
    recvData.ReadGuidMask<6, 4>(petGuid);
    recvData.ReadGuidMask<6>(petNumber);
    recvData.ReadGuidMask<5>(petGuid);
    recvData.ReadGuidMask<1>(petNumber);
    recvData.ReadGuidMask<1, 2, 3>(petGuid);
    recvData.ReadGuidMask<7, 4>(petNumber);
    recvData.ReadGuidMask<7>(petGuid);
    recvData.ReadGuidMask<3>(petNumber);

    recvData.ReadGuidBytes<0, 5>(petNumber);
    recvData.ReadGuidBytes<6>(petGuid);
    recvData.ReadGuidBytes<4, 7, 6, 2>(petNumber);
    recvData.ReadGuidBytes<3, 7, 1, 0, 2, 5>(petGuid);
    recvData.ReadGuidBytes<3>(petNumber);
    recvData.ReadGuidBytes<4>(petGuid);
    recvData.ReadGuidBytes<1>(petNumber);

    SendPetNameQuery(petGuid, petNumber);
}

void WorldSession::SendPetNameQuery(uint64 petguid, uint32 petnumber)
{
    Creature* pet = ObjectAccessor::GetCreatureOrPetOrVehicle(*_player, petguid);
    if (!pet)
    {
        //! 5.4.1
        WorldPacket data(SMSG_PET_NAME_QUERY_RESPONSE, (4+1+4+1));
        data << uint64(petnumber);
        data.WriteBit(0);
        data.FlushBits();
        _player->GetSession()->SendPacket(&data);
        return;
    }

    std::string name = pet->GetName();

    //! 5.4.1
    WorldPacket data(SMSG_PET_NAME_QUERY_RESPONSE, (4+4+name.size()+1));
    data << uint64(petnumber);
    data.WriteBit(pet->IsPet() ? 1 : 0);
    data.WriteBits(name.size(), 8);

    if (Pet* playerPet = pet->ToPet())
    {
        DeclinedName const* declinedNames = playerPet->GetDeclinedNames();
        if (declinedNames)
        {
            for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
                data.WriteBits(declinedNames->name[i].size(), 7);
        }
        else
        {
            for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
                data.WriteBits(0, 7);
        }

        data.WriteBit(0);   // Unk bit

        data.FlushBits();
      
        data.WriteString(name);

        if (declinedNames)
        {
            for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
                if (declinedNames->name[i].size())
                    data.WriteString(declinedNames->name[i]);
        }
        
        data << uint32(playerPet->GetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP));
    }

    _player->GetSession()->SendPacket(&data);
}

bool WorldSession::CheckStableMaster(uint64 guid)
{
    // spell case or GM
    if (guid == GetPlayer()->GetGUID())
    {
        if (!GetPlayer()->isGameMaster() && !GetPlayer()->HasAuraType(SPELL_AURA_OPEN_STABLE))
        {
            TC_LOG_DEBUG("network", "Player (GUID:%u) attempt open stable in cheating way.", GUID_LOPART(guid));
            return false;
        }
    }
    // stable master case
    else
    {
        if (!GetPlayer()->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_STABLEMASTER))
        {
            TC_LOG_DEBUG("network", "Stablemaster (GUID:%u) not found or you can't interact with him.", GUID_LOPART(guid));
            return false;
        }
    }
    return true;
}

//! 5.4.1
void WorldSession::HandlePetSetAction(WorldPacket & recvData)
{
    TC_LOG_DEBUG("network", "HandlePetSetAction. CMSG_PET_SET_ACTION");

    uint32 position;
    uint32 data;
    ObjectGuid petguid;

    recvData >> position >> data;

    recvData.ReadGuidMask<3, 4, 5, 7, 1, 0, 2, 6>(petguid);
    recvData.ReadGuidBytes<2, 5, 6, 4, 3, 0, 7, 1>(petguid);

    Unit* pet = ObjectAccessor::GetUnit(*_player, petguid);

    if (!pet || pet != _player->GetFirstControlled())
    {
        TC_LOG_ERROR("network", "HandlePetSetAction: Unknown pet (GUID: %u) or pet owner (GUID: %u)", GUID_LOPART(petguid), _player->GetGUIDLow());
        return;
    }

    CharmInfo* charmInfo = pet->GetCharmInfo();
    if (!charmInfo)
    {
        TC_LOG_ERROR("network", "WorldSession::HandlePetSetAction: object (GUID: %u TypeId: %u) is considered pet-like but doesn't have a charminfo!", pet->GetGUIDLow(), pet->GetTypeId());
        return;
    }

    bool move_command = false;

    uint32 spell_id = UNIT_ACTION_BUTTON_ACTION(data);
    uint8 act_state = UNIT_ACTION_BUTTON_TYPE(data);

    //ignore invalid position
    if (position >= MAX_UNIT_ACTION_BAR_INDEX)
        return;

    // remove batton
    if (act_state == ACT_DECIDE)
    {
        UnitActionBarEntry const* actionEntry = charmInfo->GetActionBarEntry(position);
        if (!actionEntry)
            return;

        //charmInfo->RemoveSpellFromActionBar(actionEntry->GetAction());
        //return;
    }

    TC_LOG_DEBUG("network", "Player %s has changed pet spell action. Position: %u, Spell: %u, State: 0x%X HasSpell %i", _player->GetName(), position, spell_id, uint32(act_state), pet->HasSpell(spell_id));

    //if it's act for spell (en/disable/cast) and there is a spell given (0 = remove spell) which pet doesn't know, don't add
    if (!((act_state == ACT_ENABLED || act_state == ACT_DISABLED || act_state == ACT_PASSIVE) && spell_id && !pet->HasSpell(spell_id)))
    {
        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spell_id))
        {
            //sign for autocast
            if (act_state == ACT_ENABLED)
            {
                if (pet->GetCharmInfo())
                    ((Pet*)pet)->ToggleAutocast(spellInfo, true);
                else
                    for (Unit::ControlList::iterator itr = GetPlayer()->m_Controlled.begin(); itr != GetPlayer()->m_Controlled.end(); ++itr)
                        if ((*itr)->GetEntry() == pet->GetEntry())
                            (*itr)->GetCharmInfo()->ToggleCreatureAutocast(spellInfo, true);
            }
            //sign for no/turn off autocast
            else if (act_state == ACT_DISABLED)
            {
                if (pet->GetCharmInfo())
                    ((Pet*)pet)->ToggleAutocast(spellInfo, false);
                else
                    for (Unit::ControlList::iterator itr = GetPlayer()->m_Controlled.begin(); itr != GetPlayer()->m_Controlled.end(); ++itr)
                        if ((*itr)->GetEntry() == pet->GetEntry())
                            (*itr)->GetCharmInfo()->ToggleCreatureAutocast(spellInfo, false);
            }
        }

        charmInfo->SetActionBar(position, spell_id, ActiveStates(act_state));
    }
}

//! 5.4.1
void WorldSession::HandlePetRename(WorldPacket & recvData)
{
    TC_LOG_DEBUG("network", "HandlePetRename. CMSG_PET_RENAME");

    uint32 petnumber;
    uint8 isdeclined;

    std::string name;
    DeclinedName declinedname;

    recvData >> petnumber;

    bool namepart = !recvData.ReadBit();
    isdeclined  = recvData.ReadBit();

    Pet* pet = _player->GetPet();
    if (!pet)
    {
        recvData.rfinish();
        TC_LOG_DEBUG("network", "HandlePetRename pet not found");
        return;
    }
                                                            // check it!
    if (!pet || !pet->IsPet() || ((Pet*)pet)->getPetType()!= HUNTER_PET ||
        !pet->HasByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED) ||
        pet->GetOwnerGUID() != _player->GetGUID() || !pet->GetCharmInfo())
    {
        recvData.rfinish();
        TC_LOG_DEBUG("network", "HandlePetRename error pet");
        return;
    }

    if (pet->GetCharmInfo()->GetPetNumber() != petnumber)
    {
        recvData.rfinish();
        TC_LOG_DEBUG("network", "HandlePetRename petnumber not correct");
        return;
    }

    uint32 declname[MAX_DECLINED_NAME_CASES];
    if (isdeclined)
    {
        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            declname[i] = recvData.ReadBits(7);
    }

    uint32 len = 0;
    if (namepart)
        len = recvData.ReadBits(8);

    SQLTransaction trans = CharacterDatabase.BeginTransaction();
    if (namepart)
    {
        name = recvData.ReadString(len);
        PetNameInvalidReason res = ObjectMgr::CheckPetName(name);
        if (res != PET_NAME_SUCCESS)
        {
            SendPetNameInvalid(res, name, NULL);
            recvData.rfinish();
            TC_LOG_DEBUG("network", "HandlePetRename CheckPetName res %i", res);
            return;
        }

        if (sObjectMgr->IsReservedName(name))
        {
            SendPetNameInvalid(PET_NAME_RESERVED, name, NULL);
            recvData.rfinish();
            return;
        }

        pet->SetName(name);

        PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_UPD_CHAR_PET_NAME);
        stmt->setString(0, name);
        stmt->setUInt32(1, _player->GetGUIDLow());
        stmt->setUInt32(2, pet->GetCharmInfo()->GetPetNumber());
        trans->Append(stmt);
    }

    if (isdeclined)
    {
        for (uint8 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            declinedname.name[i] = recvData.ReadString(declname[i]);

        if (sWorld->getBoolConfig(CONFIG_DECLINED_NAMES_USED))
        {
            std::wstring wname;
            Utf8toWStr(name, wname);
            if (!ObjectMgr::CheckDeclinedNames(wname, declinedname))
            {
                SendPetNameInvalid(PET_NAME_DECLENSION_DOESNT_MATCH_BASE_NAME, name, &declinedname);
                return;
            }

            PreparedStatement* stmt = CharacterDatabase.GetPreparedStatement(CHAR_DEL_CHAR_PET_DECLINEDNAME);
            stmt->setUInt32(0, pet->GetCharmInfo()->GetPetNumber());
            trans->Append(stmt);

            stmt = CharacterDatabase.GetPreparedStatement(CHAR_ADD_CHAR_PET_DECLINEDNAME);
            stmt->setUInt32(0, _player->GetGUIDLow());

            for (uint8 i = 0; i < 5; i++)
                stmt->setString(i+1, declinedname.name[i]);

            trans->Append(stmt);
        }
    }

    CharacterDatabase.CommitTransaction(trans);

    Unit* owner = pet->GetOwner();
    if (owner && (owner->GetTypeId() == TYPEID_PLAYER) && owner->ToPlayer()->GetGroup())
        owner->ToPlayer()->SetGroupUpdateFlag(GROUP_UPDATE_FLAG_PET_NAME);

    pet->RemoveByteFlag(UNIT_FIELD_BYTES_2, 2, UNIT_CAN_BE_RENAMED);
    pet->SetUInt32Value(UNIT_FIELD_PET_NAME_TIMESTAMP, uint32(time(NULL))); // cast can't be helped
}

//! 5.4.1
void WorldSession::HandlePetAbandon(WorldPacket& recvData)
{
    ObjectGuid guid;

    recvData.ReadGuidMask<6, 5, 2, 0, 3, 4, 7, 1>(guid);
    recvData.ReadGuidBytes<1, 2, 4, 3, 7, 5, 6, 0>(guid);

    TC_LOG_DEBUG("network", "HandlePetAbandon. CMSG_PET_ABANDON pet guid is %u", GUID_LOPART(guid));

    if (!_player->IsInWorld())
        return;

    // pet/charmed
    Creature* pet = ObjectAccessor::GetCreatureOrPetOrVehicle(*_player, guid);
    if (pet)
    {
        if (pet->IsPet())
        {
            _player->RemovePet((Pet*)pet, true);
            _player->GetSession()->SendStablePet(0);
        }
        else if (pet->GetGUID() == _player->GetCharmGUID())
            _player->StopCastingCharm();
    }
}

void WorldSession::HandlePetSpellAutocastOpcode(WorldPacket& recvPacket)
{
    TC_LOG_DEBUG("network", "CMSG_PET_SPELL_AUTOCAST");
    uint64 guid;
    uint32 spellid;
    uint32  state;                                           //1 for on, 0 for off
    recvPacket >> guid >> spellid >> state;

    if (!_player->GetGuardianPet() && !_player->GetCharm())
        return;

    if (ObjectAccessor::FindPlayer(guid))
        return;

    Creature* pet=ObjectAccessor::GetCreatureOrPetOrVehicle(*_player, guid);

    if (!pet || (pet != _player->GetGuardianPet() && pet != _player->GetCharm()))
    {
        TC_LOG_ERROR("network", "HandlePetSpellAutocastOpcode.Pet %u isn't pet of player %s .", uint32(GUID_LOPART(guid)), GetPlayer()->GetName());
        return;
    }

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellid);
    // do not add not learned spells/ passive spells
    if (!pet->HasSpell(spellid) || spellInfo->IsAutocastable())
        return;

    CharmInfo* charmInfo = pet->GetCharmInfo();
    if (!charmInfo)
    {
        TC_LOG_ERROR("network", "WorldSession::HandlePetSpellAutocastOpcod: object (GUID: %u TypeId: %u) is considered pet-like but doesn't have a charminfo!", pet->GetGUIDLow(), pet->GetTypeId());
        return;
    }

    if (pet->IsPet())
        ((Pet*)pet)->ToggleAutocast(spellInfo, state);
    else
        pet->GetCharmInfo()->ToggleCreatureAutocast(spellInfo, state);

    charmInfo->SetSpellAutocast(spellInfo, state);
}

//! 5.4.1
void WorldSession::HandlePetCastSpellOpcode(WorldPacket& recvPacket)
{
    TC_LOG_DEBUG("network", "WORLD: CMSG_PET_CAST_SPELL");

    uint8 castCount = 0;
    uint8 castFlags = 0;
    uint32 spellId = 0;
    uint32 glyphIndex = 0;
    uint32 targetStringLength = 0;
    ObjectGuid CasterGUID = 0;
    ObjectGuid targetGuid = 0;
    ObjectGuid itemTargetGuid = 0;
    ObjectGuid destTransportGuid = 0;
    ObjectGuid srcTransportGuid = 0;

    // client provided targets
    SpellCastTargets targets;

    recvPacket.ReadBit();
    CasterGUID[0] = recvPacket.ReadBit();
    bool hasGlyphIndex = !recvPacket.ReadBit();
    CasterGUID[6] = recvPacket.ReadBit();
    uint32 researchDataCount = recvPacket.ReadBits(2);
    
    bool hasDestLocation = recvPacket.ReadBit();   
    bool hasMovement = recvPacket.ReadBit();
    CasterGUID[2] = recvPacket.ReadBit();
    bool hasElevation = !recvPacket.ReadBit();
    CasterGUID[5] = recvPacket.ReadBit();
    bool hasTargetMask = !recvPacket.ReadBit();
    CasterGUID[4] = recvPacket.ReadBit();
    for (uint32 i = 0; i < researchDataCount; ++i)
        recvPacket.ReadBits(2);
    bool hasSrcLocation = recvPacket.ReadBit(); 

    bool hasCastFlags = !recvPacket.ReadBit();
    bool hasCastCount = !recvPacket.ReadBit();
    CasterGUID[1] = recvPacket.ReadBit();
    bool hasTargetString = !recvPacket.ReadBit();
    recvPacket.ReadBit();
    bool hasSpellId = !recvPacket.ReadBit();
    CasterGUID[7] = recvPacket.ReadBit();
    CasterGUID[3] = recvPacket.ReadBit();
    bool hasMissileSpeed = !recvPacket.ReadBit();

    uint8 bitsOrder1[8] = { 4, 6, 3, 5, 0, 2, 1, 7 };
    recvPacket.ReadBitInOrder(targetGuid, bitsOrder1);

    uint8 bitsOrder2[8] = { 3, 7, 2, 1, 4, 0, 6, 5 };
    recvPacket.ReadBitInOrder(itemTargetGuid, bitsOrder2);

    // Movement data
    MovementInfo movementInfo;
    bool hasMoveIndex = false;
    uint32 removeForcesCount = 0;

    if (hasMovement)
    {
        bool hasMovementFlags2 = !recvPacket.ReadBit();
        movementInfo.remoteTimeValid = recvPacket.ReadBit();
        bool hasMovementFlags = !recvPacket.ReadBit();
        recvPacket.ReadGuidMask<4>(movementInfo.moverGUID);
        removeForcesCount = recvPacket.ReadBits(22);
        recvPacket.ReadGuidMask<3, 6>(movementInfo.moverGUID);
        movementInfo.hasTransportData = recvPacket.ReadBit();
        recvPacket.ReadGuidMask<1>(movementInfo.moverGUID);

        if (movementInfo.hasTransportData)
        {
            recvPacket.ReadGuidMask<1, 0, 5, 4, 6>(movementInfo.transportGUID);
            movementInfo.hasTransportPrevMoveTime = recvPacket.ReadBit();
            movementInfo.hasTransportVehicleRecID = recvPacket.ReadBit();
            recvPacket.ReadGuidMask<3, 2, 7>(movementInfo.transportGUID);
        }

        movementInfo.hasFacing = !recvPacket.ReadBit();
        hasMoveIndex = !recvPacket.ReadBit();
        movementInfo.hasMoveTime = !recvPacket.ReadBit();
        if (hasMovementFlags)
            movementInfo.flags = recvPacket.ReadBits(30);
        recvPacket.ReadGuidMask<7, 0>(movementInfo.moverGUID);
        movementInfo.hasPitch = !recvPacket.ReadBit();
        recvPacket.ReadGuidMask<2>(movementInfo.moverGUID);
        movementInfo.hasSpline = recvPacket.ReadBit();
        recvPacket.ReadGuidMask<5>(movementInfo.moverGUID);
        movementInfo.hasStepUpStartElevation = !recvPacket.ReadBit();
        movementInfo.heightChangeFailed = recvPacket.ReadBit();
        if (hasMovementFlags2)
            movementInfo.flags2 = recvPacket.ReadBits(13);
        movementInfo.hasFallData = recvPacket.ReadBit();
        if (movementInfo.hasFallData)
            movementInfo.hasFallDirection = recvPacket.ReadBit();
    }

    if (hasDestLocation)
    {
        uint8 bitsOrder[8] = { 3, 5, 4, 2, 1, 7, 0, 6  };
        recvPacket.ReadBitInOrder(destTransportGuid, bitsOrder);
    }

    if (hasSrcLocation)
    {
        uint8 bitsOrder[8] = { 7, 6, 2, 3, 5, 4, 1, 0  };
        recvPacket.ReadBitInOrder(srcTransportGuid, bitsOrder);
    }

    if (hasTargetMask)
        targets.SetTargetMask(recvPacket.ReadBits(20));

    if (hasTargetString)
        targetStringLength = recvPacket.ReadBits(7);

    if (hasCastFlags)
        castFlags = recvPacket.ReadBits(5);

    recvPacket.ReadByteSeq(CasterGUID[4]);
    for (uint32 i = 0; i < researchDataCount; ++i)
    {
        recvPacket.read_skip<uint32>(); // Archaeology research keystone/fragment id
        recvPacket.read_skip<uint32>(); // Archaeology research keystone/fragment count
    }
    recvPacket.ReadByteSeq(CasterGUID[0]);
    recvPacket.ReadByteSeq(CasterGUID[7]);
    recvPacket.ReadByteSeq(CasterGUID[2]);
    recvPacket.ReadByteSeq(CasterGUID[5]);
    recvPacket.ReadByteSeq(CasterGUID[3]);
    recvPacket.ReadByteSeq(CasterGUID[6]);
    recvPacket.ReadByteSeq(CasterGUID[1]);

    uint8 bytesOrder1[8] = { 3, 0, 1, 2, 7, 4, 5, 6  };
    recvPacket.ReadBytesSeq(itemTargetGuid, bytesOrder1);

    if (hasMovement)
    {
        if (movementInfo.hasTransportData)
        {
            if (movementInfo.hasTransportVehicleRecID)
                recvPacket >> movementInfo.transportVehicleRecID;

            recvPacket >> movementInfo.transportPosition.m_positionX;
            recvPacket.ReadGuidBytes<6>(movementInfo.transportGUID);

            if (movementInfo.hasTransportPrevMoveTime)
                recvPacket >> movementInfo.transportPrevMoveTime;

            movementInfo.transportPosition.SetOrientation(recvPacket.read<float>());
            recvPacket.ReadGuidBytes<7>(movementInfo.transportGUID);
            recvPacket >> movementInfo.transportVehicleSeatIndex;
            recvPacket.ReadGuidBytes<3>(movementInfo.transportGUID);
            recvPacket >> movementInfo.transportPosition.m_positionZ;
            recvPacket >> movementInfo.transportPosition.m_positionY;
            recvPacket.ReadGuidBytes<1, 2, 0>(movementInfo.transportGUID);
            recvPacket >> movementInfo.transportMoveTime;
            recvPacket.ReadGuidBytes<5, 4>(movementInfo.transportGUID);
        }

        recvPacket.ReadGuidBytes<4>(movementInfo.moverGUID);
        recvPacket >> movementInfo.position.m_positionZ;

        for (uint32 i = 0; i < removeForcesCount; ++i)
            movementInfo.removeForcesIDs.push_back(recvPacket.read<uint32>());

        if (movementInfo.hasFallData)
        {
            if (movementInfo.hasFallDirection)
            {
                recvPacket >> movementInfo.fallSpeed;
                recvPacket >> movementInfo.fallSinAngle;
                recvPacket >> movementInfo.fallCosAngle;
            }
            recvPacket >> movementInfo.fallTime;
            recvPacket >> movementInfo.fallJumpVelocity;
        }

        recvPacket.ReadGuidBytes<1>(movementInfo.moverGUID);

        if (hasMoveIndex)
            recvPacket.read_skip<uint32>();

        recvPacket >> movementInfo.position.m_positionX;
        recvPacket.ReadGuidBytes<3, 6>(movementInfo.moverGUID);

        if (movementInfo.hasMoveTime)
            recvPacket >> movementInfo.moveTime;

        recvPacket.ReadGuidBytes<5>(movementInfo.moverGUID);
        recvPacket >> movementInfo.position.m_positionY;

        if (movementInfo.hasStepUpStartElevation)
            recvPacket >> movementInfo.stepUpStartElevation;

        if (movementInfo.hasPitch)
            movementInfo.pitch = G3D::wrap(recvPacket.read<float>(), float(-M_PI), float(M_PI));

        recvPacket.ReadGuidBytes<0, 7, 2>(movementInfo.moverGUID);

        if (movementInfo.hasFacing)
            movementInfo.position.SetOrientation(recvPacket.read<float>());
    }

    if (hasDestLocation)
    {       
        recvPacket.ReadByteSeq(destTransportGuid[1]);
        recvPacket.ReadByteSeq(destTransportGuid[0]);
        recvPacket.ReadByteSeq(destTransportGuid[5]);
        if (destTransportGuid)
            recvPacket >> targets.m_dst._transportOffset.m_positionX;
        else
            recvPacket >> targets.m_dst._position.m_positionX;
        if (destTransportGuid)
            recvPacket >> targets.m_dst._transportOffset.m_positionZ;
        else
            recvPacket >> targets.m_dst._position.m_positionZ;
        recvPacket.ReadByteSeq(destTransportGuid[4]);
        recvPacket.ReadByteSeq(destTransportGuid[7]);
        recvPacket.ReadByteSeq(destTransportGuid[2]);
        recvPacket.ReadByteSeq(destTransportGuid[3]);
        if (destTransportGuid)
            recvPacket >> targets.m_dst._transportOffset.m_positionY;
        else
            recvPacket >> targets.m_dst._position.m_positionY;
        recvPacket.ReadByteSeq(destTransportGuid[6]);

        targets.m_dst._transportGUID = destTransportGuid;
    }

    uint8 bytesOrder2[8] = { 6, 5, 4, 0, 7, 3, 1, 2  };
    recvPacket.ReadBytesSeq(targetGuid, bytesOrder2);

    if (hasSrcLocation)
    {
        if (srcTransportGuid)
            recvPacket >> targets.m_src._transportOffset.m_positionX;
        else
            recvPacket >> targets.m_src._position.m_positionX;
        recvPacket.ReadByteSeq(srcTransportGuid[7]);
        recvPacket.ReadByteSeq(srcTransportGuid[1]);
        recvPacket.ReadByteSeq(srcTransportGuid[3]);
        recvPacket.ReadByteSeq(srcTransportGuid[4]);
        if (srcTransportGuid)
            recvPacket >> targets.m_src._transportOffset.m_positionZ;
        else
            recvPacket >> targets.m_src._position.m_positionZ;
        recvPacket.ReadByteSeq(srcTransportGuid[0]);
        recvPacket.ReadByteSeq(srcTransportGuid[6]);
        if (srcTransportGuid)
            recvPacket >> targets.m_src._transportOffset.m_positionY;
        else
            recvPacket >> targets.m_src._position.m_positionY;
        recvPacket.ReadByteSeq(srcTransportGuid[5]);
        recvPacket.ReadByteSeq(srcTransportGuid[2]);
        
        targets.m_src._transportGUID = srcTransportGuid;
    }


    if (hasTargetString)
        targets.m_strTarget = recvPacket.ReadString(targetStringLength);

    if (hasCastCount)
        recvPacket >> castCount;

    if (hasMissileSpeed)
        recvPacket >> targets.m_speed;

    if (hasSpellId)
        recvPacket >> spellId;

    if (hasElevation)
        recvPacket >> targets.m_elevation;

    if (hasGlyphIndex)
        recvPacket >> glyphIndex;

    targets.m_itemTargetGUID = itemTargetGuid;
    targets.m_objectTargetGUID = targetGuid;

    TC_LOG_DEBUG("network", "WORLD: CMSG_PET_CAST_SPELL, castCount: %u, spellId %u, castFlags %u", castCount, spellId, castFlags);

    // This opcode is also sent from charmed and possessed units (players and creatures)
    if (!_player->GetGuardianPet() && !_player->GetCharm())
        return;

    Unit* caster = ObjectAccessor::GetUnit(*_player, CasterGUID);

    if (!caster || (caster != _player->GetGuardianPet() && caster != _player->GetCharm()))
    {
        TC_LOG_ERROR("network", "HandlePetCastSpellOpcode: Pet %u isn't pet of player %s .", uint32(GUID_LOPART(CasterGUID)), GetPlayer()->GetName());
        return;
    }

    targets.Update(caster);

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo)
    {
        TC_LOG_ERROR("network", "WORLD: unknown PET spell id %i", spellId);
        return;
    }

    bool triggered = false;
    for (uint32 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (spellInfo->Effects[i]->TargetA.GetTarget() == TARGET_DEST_TRAJ || spellInfo->Effects[i]->TargetB.GetTarget() == TARGET_DEST_TRAJ || spellInfo->Effects[i]->Effect == SPELL_EFFECT_TRIGGER_MISSILE)
            triggered = true;
    }

    if (spellInfo->StartRecoveryCategory > 0) // Check if spell is affected by GCD
        if (caster->GetTypeId() == TYPEID_UNIT && caster->GetCharmInfo() && caster->GetCharmInfo()->GetGlobalCooldownMgr().HasGlobalCooldown(spellInfo))
        {
            TC_LOG_ERROR("network", "HandlePetCastSpellOpcode: Check if spell is affected by GCD");
            caster->SendPetCastFail(spellId, SPELL_FAILED_NOT_READY);
            return;
        }

    // do not cast not learned spells
    if (!triggered && (!caster->HasSpell(spellId) || spellInfo->IsPassive()))
    {
        TC_LOG_ERROR("network", "HandlePetCastSpellOpcode: !HasSpell or IsPassive");
        return;
    }

    caster->ClearUnitState(UNIT_STATE_FOLLOW);

    uint32 triggeredCastFlags = triggered ? TRIGGERED_FULL_MASK : TRIGGERED_NONE;
    triggeredCastFlags &= ~TRIGGERED_IGNORE_POWER_AND_REAGENT_COST;

    Spell* spell = new Spell(caster, spellInfo, TriggerCastFlags(triggeredCastFlags));
    spell->m_cast_count = castCount;                    // probably pending spell cast
    spell->m_targets = targets;

    // TODO: need to check victim?
    SpellCastResult result;
    if (caster->m_movedPlayer)
        result = spell->CheckPetCast(caster->m_movedPlayer->GetSelectedUnit());
    else
        result = spell->CheckPetCast(NULL);
    if (result == SPELL_CAST_OK)
    {
        if (caster->GetTypeId() == TYPEID_UNIT)
        {
            Creature* pet = caster->ToCreature();
            pet->AddCreatureSpellCooldown(spellId);
            if (pet->IsPet())
            {
                Pet* p = (Pet*)pet;
                // 10% chance to play special pet attack talk, else growl
                // actually this only seems to happen on special spells, fire shield for imp, torment for voidwalker, but it's stupid to check every spell
                if (p->getPetType() == SUMMON_PET && (urand(0, 100) < 10))
                    pet->SendPetTalk((uint32)PET_TALK_SPECIAL_SPELL);
                else
                    pet->SendPetAIReaction(CasterGUID);
            }
        }

        spell->prepare(&(spell->m_targets));
    }
    else
    {
        caster->SendPetCastFail(spellId, result);
        if (caster->GetTypeId() == TYPEID_PLAYER)
        {
            if (!caster->ToPlayer()->HasSpellCooldown(spellId))
                GetPlayer()->SendClearCooldown(spellId, caster);
        }
        else
        {
            if (!caster->ToCreature()->HasSpellCooldown(spellId))
                GetPlayer()->SendClearCooldown(spellId, caster);
        }

        spell->finish(false);
        delete spell;
    }
}

//! 5.4.1
void WorldSession::SendPetNameInvalid(uint32 error, const std::string& name, DeclinedName *declinedName)
{
    WorldPacket data(SMSG_PET_NAME_INVALID, 4 + name.size() + 1 + 1);
    data << uint32(0);
    data << uint8(error);    // unk

    data.WriteBit(declinedName ? 1 : 0);
    data.WriteBit(0);   //has name
    data.WriteBits(name.size(), 8);
    if (declinedName)
    {
        for (uint32 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            data.WriteBits(declinedName->name[i].size(), 7);

        data.FlushBits();

        for (uint32 i = 0; i < MAX_DECLINED_NAME_CASES; ++i)
            data.WriteString(declinedName->name[i]);
    }

    data.FlushBits();

    data.WriteString(name);

    SendPacket(&data);
}

//! 5.4.1
void WorldSession::HandleLearnPetSpecialization(WorldPacket & recvData)
{
    uint32 index = recvData.read<uint32>();
    // GUID : useless =P
    recvData.rfinish();

    if(_player->isInCombat() || _player->getClass() != CLASS_HUNTER)
        return;

    uint32 specializationId = 0;

    switch(index)
    {
        case 0:
            specializationId = SPEC_PET_FEROCITY;   // Ferocity
            break;
        case 1:
            specializationId = SPEC_PET_TENACITY;   // Tenacity
            break;
        case 2:
            specializationId = SPEC_PET_CUNNING;    // Cunning
            break;
        default:
            break;
    }

    if (!specializationId)
        return;

    Pet* pet = _player->GetPet();
    if (!pet || !pet->IsAlive())
        return;

    if(pet->GetSpecializationId())
        pet->UnlearnSpecializationSpell();

    pet->SetSpecializationId(specializationId);
    pet->LearnSpecializationSpell();
    _player->SendTalentsInfoData(true);
}