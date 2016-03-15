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
#include "Opcodes.h"
#include "Log.h"
#include "Corpse.h"
#include "Player.h"
#include "SpellAuras.h"
#include "MapManager.h"
#include "Transport.h"
#include "Battleground.h"
#include "WaypointMovementGenerator.h"
#include "InstanceSaveMgr.h"
#include "ObjectMgr.h"
#include "MovementStructures.h"
#include "VMapFactory.h"
#include "Vehicle.h"

void WorldSession::HandleMoveWorldportAckOpcode(WorldPacket& /*recvPacket*/)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: got MSG_MOVE_WORLDPORT_ACK.");
    HandleMoveWorldportAckOpcode();
}

void WorldSession::HandleMoveWorldportAckOpcode()
{
    // ignore unexpected far teleports
    if (!GetPlayer()->IsBeingTeleportedFar())
        return;

    GetPlayer()->SetSemaphoreTeleportFar(false);
    if(Unit* mover = _player->m_mover)
        mover->ClearUnitState(UNIT_STATE_JUMPING);

    // get the teleport destination
    WorldLocation const loc = GetPlayer()->GetTeleportDest();

    // possible errors in the coordinate validity check
    if (!MapManager::IsValidMapCoord(loc))
    {
        LogoutPlayer(false);
        return;
    }

    // get the destination map entry, not the current one, this will fix homebind and reset greeting
    MapEntry const* mEntry = sMapStore.LookupEntry(loc.GetMapId());
    InstanceTemplate const* mInstance = sObjectMgr->GetInstanceTemplate(loc.GetMapId());

    // reset instance validity, except if going to an instance inside an instance
    if (GetPlayer()->m_InstanceValid == false && !mInstance)
        GetPlayer()->m_InstanceValid = true;

    Map* oldMap = GetPlayer()->GetMap();
    if (GetPlayer()->IsInWorld())
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Player (Name %s) is still in world when teleported from map %u to new map %u", GetPlayer()->GetName(), oldMap->GetId(), loc.GetMapId());
        oldMap->RemovePlayerFromMap(GetPlayer(), false);
    }

    // relocate the player to the teleport destination
    Map* newMap = sMapMgr->CreateMap(loc.GetMapId(), GetPlayer());
    // the CanEnter checks are done in TeleporTo but conditions may change
    // while the player is in transit, for example the map may get full
    if (!newMap || !newMap->CanEnter(GetPlayer()))
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "Map %d could not be created for player %d, porting player to homebind", loc.GetMapId(), GetPlayer()->GetGUIDLow());
        GetPlayer()->TeleportTo(GetPlayer()->m_homebindMapId, GetPlayer()->m_homebindX, GetPlayer()->m_homebindY, GetPlayer()->m_homebindZ, GetPlayer()->GetOrientation());
        return;
    }
    else
        GetPlayer()->Relocate(&loc);

    GetPlayer()->ResetMap();
    GetPlayer()->SetMap(newMap);

    GetPlayer()->SendInitialPacketsBeforeAddToMap();
    if (!GetPlayer()->GetMap()->AddPlayerToMap(GetPlayer()))
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "WORLD: failed to teleport player %s (%d) to map %d because of unknown reason!", GetPlayer()->GetName(), GetPlayer()->GetGUIDLow(), loc.GetMapId());
        GetPlayer()->ResetMap();
        GetPlayer()->SetMap(oldMap);
        GetPlayer()->TeleportTo(GetPlayer()->m_homebindMapId, GetPlayer()->m_homebindX, GetPlayer()->m_homebindY, GetPlayer()->m_homebindZ, GetPlayer()->GetOrientation());
        return;
    }

    // battleground state prepare (in case join to BG), at relogin/tele player not invited
    // only add to bg group and object, if the player was invited (else he entered through command)
    if (_player->InBattleground())
    {
        // cleanup setting if outdated
        if (!mEntry->IsBattlegroundOrArena())
        {
            if (Battleground* bg = _player->GetBattleground())
                _player->LeaveBattleground(false);

            // We're not in BG
            _player->SetBattlegroundId(0, BATTLEGROUND_TYPE_NONE);
            // reset destination bg team
            _player->SetBGTeam(0);
        }
        // join to bg case
        else if (Battleground* bg = _player->GetBattleground())
        {
            if (_player->IsInvitedForBattlegroundInstance(_player->GetBattlegroundId()))
                bg->AddPlayer(_player);
        }
    }

    GetPlayer()->SendInitialPacketsAfterAddToMap();

    // Update position client-side to avoid undermap
    WorldPacket data(SMSG_PLAYER_MOVE);
    _player->m_movementInfo.time = getMSTime();
    _player->m_movementInfo.pos.m_positionX = loc.m_positionX;
    _player->m_movementInfo.pos.m_positionY = loc.m_positionY;
    _player->m_movementInfo.pos.m_positionZ = loc.m_positionZ;
    _player->m_movementInfo.pos.m_orientation = loc.m_orientation;
    WorldSession::WriteMovementInfo(data, &_player->m_movementInfo);
    _player->GetSession()->SendPacket(&data);

    // flight fast teleport case
    if (GetPlayer()->GetMotionMaster()->GetCurrentMovementGeneratorType() == FLIGHT_MOTION_TYPE)
    {
        if (!_player->InBattleground())
        {
            // short preparations to continue flight
            FlightPathMovementGenerator* flight = (FlightPathMovementGenerator*)(GetPlayer()->GetMotionMaster()->top());
            flight->Initialize(*GetPlayer());
            return;
        }

        // battleground state prepare, stop flight
        GetPlayer()->GetMotionMaster()->MovementExpired();
        GetPlayer()->CleanupAfterTaxiFlight();
    }

    // resurrect character at enter into instance where his corpse exist after add to map
    Corpse* corpse = GetPlayer()->GetCorpse();
    if (corpse && corpse->GetType() != CORPSE_BONES && corpse->GetMapId() == GetPlayer()->GetMapId())
    {
        if (mEntry->IsDungeon())
        {
            GetPlayer()->ResurrectPlayer(0.5f, false);
            GetPlayer()->SpawnCorpseBones();
        }
    }

    bool allowMount = !mEntry->IsDungeon() || mEntry->IsBattlegroundOrArena();
    if (mInstance)
    {
        Difficulty diff = GetPlayer()->GetDifficulty(mEntry->IsRaid());
        if (MapDifficulty const* mapDiff = GetMapDifficultyData(mEntry->MapID, diff))
        {
            if (mapDiff->resetTime)
            {
                if (time_t timeReset = sWorld->getInstanceResetTime(mapDiff->resetTime))
                {
                    uint32 timeleft = uint32(timeReset - time(NULL));
                    GetPlayer()->SendInstanceResetWarning(mEntry->MapID, diff, timeleft);
                }
            }
        }
        allowMount = mInstance->AllowMount;
    }

    // mount allow check
    if (!allowMount || (GetPlayer()->GetMapId() == 530 && GetPlayer()->GetZoneId() == 0)) //530 - uwow event map
        _player->RemoveAurasByType(SPELL_AURA_MOUNTED);

    // update zone immediately, otherwise leave channel will cause crash in mtmap
    uint32 newzone, newarea;
    GetPlayer()->GetZoneAndAreaId(newzone, newarea);
    GetPlayer()->UpdateZone(newzone, newarea);

    // honorless target
    if (GetPlayer()->pvpInfo.inHostileArea)
        GetPlayer()->CastSpell(GetPlayer(), 2479, true);

    // in friendly area
    else if (GetPlayer()->IsPvP() && !GetPlayer()->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP))
        GetPlayer()->UpdatePvP(false, false);

    // resummon pet
    GetPlayer()->ResummonPetTemporaryUnSummonedIfAny();

    //lets process all delayed operations on successful teleport
    GetPlayer()->ProcessDelayedOperations();
}

void WorldSession::HandleMoveTeleportAck(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_MOVE_TELEPORT_ACK");

    ObjectGuid guid;
    uint32 flags, time;
    recvPacket >> flags >> time;

    recvPacket.ReadGuidMask<1, 7, 2, 5, 0, 6, 3, 4>(guid);
    recvPacket.ReadGuidBytes<1, 5, 4, 3, 0, 7, 6, 2>(guid);

    sLog->outDebug(LOG_FILTER_NETWORKIO, "Guid " UI64FMTD, uint64(guid));
    sLog->outDebug(LOG_FILTER_NETWORKIO, "Flags %u, time %u", flags, time/IN_MILLISECONDS);

    Player* plMover = _player->m_mover->ToPlayer();

    if (!plMover || !plMover->IsBeingTeleportedNear())
        return;

    if (guid != plMover->GetGUID())
        return;

    plMover->SetSemaphoreTeleportNear(false);
    plMover->AddMoveEventsMask(MOVE_EVENT_TELEPORT);

    uint32 old_zone = plMover->GetZoneId();

    WorldLocation const& dest = plMover->GetTeleportDest();

    plMover->UpdatePosition(dest, true);

    uint32 newzone, newarea;
    plMover->GetZoneAndAreaId(newzone, newarea);
    plMover->UpdateZone(newzone, newarea);

    // new zone
    if (old_zone != newzone)
    {
        // honorless target
        if (plMover->pvpInfo.inHostileArea)
            plMover->CastSpell(plMover, 2479, true);

        // in friendly area
        else if (plMover->IsPvP() && !plMover->HasFlag(PLAYER_FLAGS, PLAYER_FLAGS_IN_PVP))
            plMover->UpdatePvP(false, false);
    }

    // resummon pet
    GetPlayer()->ResummonPetTemporaryUnSummonedIfAny();

    //lets process all delayed operations on successful teleport
    GetPlayer()->ProcessDelayedOperations();

    if(Unit* mover = _player->m_mover)
    {
        WorldPacket data(SMSG_PLAYER_MOVE);
        mover->m_movementInfo.time = getMSTime();
        mover->m_movementInfo.pos.m_positionX = mover->GetPositionX();
        mover->m_movementInfo.pos.m_positionY = mover->GetPositionY();
        mover->m_movementInfo.pos.m_positionZ = mover->GetPositionZ();
        WorldSession::WriteMovementInfo(data, &mover->m_movementInfo);
        mover->SendMessageToSet(&data, _player);
        mover->ClearUnitState(UNIT_STATE_JUMPING);
    }
}

void WorldSession::HandleMovementOpcodes(WorldPacket& recvPacket)
{
    Opcodes opcode = recvPacket.GetOpcode();

    uint32 diff = sWorld->GetUpdateTime();
    Unit* mover = _player->m_mover;

    if(!mover || mover == NULL)                                  // there must always be a mover
    {
        recvPacket.rfinish();                     // prevent warnings spam
        return;
    }

    Player* plrMover = mover->ToPlayer();

    Vehicle *vehMover = mover->GetVehicleKit();
    if (vehMover)
        if (mover->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_PLAYER_CONTROLLED))
            if (Unit *charmer = mover->GetCharmer())
                if (charmer->GetTypeId() == TYPEID_PLAYER)
                    plrMover = (Player*)charmer;

    uint8 forvehunit = 0;
    if(plrMover && plrMover->GetTypeId() == TYPEID_PLAYER && plrMover->GetVehicle())
        if(Unit* VehUnit = plrMover->GetVehicle()->GetBase())
            if(VehUnit->HasUnitMovementFlag(MOVEMENTFLAG_CAN_FLY) || VehUnit->HasUnitMovementFlag(MOVEMENTFLAG_FLYING))
                forvehunit = 1;

    // ignore, waiting processing in WorldSession::HandleMoveWorldportAckOpcode and WorldSession::HandleMoveTeleportAck
    if (plrMover && plrMover->IsBeingTeleported())
    {
        recvPacket.rfinish();                     // prevent warnings spam
        return;
    }

    /* extract packet */
    MovementInfo movementInfo;
    ReadMovementInfo(recvPacket, &movementInfo);

    // prevent tampered movement data
    if (movementInfo.guid != mover->GetGUID() || !mover->IsInWorld())
    {
        //sLog->outError(LOG_FILTER_NETWORKIO, "HandleMovementOpcodes: guid error");
        recvPacket.rfinish();                     // prevent warnings spam
        return;
    }

    if (!movementInfo.pos.IsPositionValid())
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "HandleMovementOpcodes: Invalid Position");
        recvPacket.rfinish();                     // prevent warnings spam
        return;
    }

    // Disable walk mode in charmer
    if(mover->HasAuraType(SPELL_AURA_MOD_POSSESS) || (plrMover && plrMover->HasAuraType(SPELL_AURA_MOD_POSSESS)))
    {
        if (movementInfo.flags & MOVEMENTFLAG_WALKING)
            movementInfo.flags &= ~MOVEMENTFLAG_WALKING;
    }

    /* handle special cases */
    if (movementInfo.t_guid)
    {
        if(World::GetEnableMvAnticheatDebug())
            sLog->outError(LOG_FILTER_NETWORKIO, "HandleMovementOpcodes t_guid %u, opcode[%s]", movementInfo.t_guid, GetOpcodeNameForLogging(opcode).c_str());

        // transports size limited
        // (also received at zeppelin leave by some reason with t_* as absolute in continent coordinates, can be safely skipped)
        if (movementInfo.t_pos.GetPositionX() > 50 || movementInfo.t_pos.GetPositionY() > 50 || movementInfo.t_pos.GetPositionZ() > 50)
        {
            recvPacket.rfinish();                 // prevent warnings spam
            return;
        }

        if (!Trinity::IsValidMapCoord(movementInfo.pos.GetPositionX() + movementInfo.t_pos.GetPositionX(), movementInfo.pos.GetPositionY() + movementInfo.t_pos.GetPositionY(),
            movementInfo.pos.GetPositionZ() + movementInfo.t_pos.GetPositionZ(), movementInfo.pos.GetOrientation() + movementInfo.t_pos.GetOrientation()))
        {
            recvPacket.rfinish();                 // prevent warnings spam
            return;
        }

        // if we boarded a transport, add us to it
        if (plrMover)
        {
            if (!plrMover->GetTransport())
            {
                // elevators also cause the client to send MOVEMENTFLAG_ONTRANSPORT - just dismount if the guid can be found in the transport list
                for (MapManager::TransportSet::const_iterator iter = sMapMgr->m_Transports.begin(); iter != sMapMgr->m_Transports.end(); ++iter)
                {
                    if ((*iter)->GetGUID() == movementInfo.t_guid)
                    {
                        plrMover->m_transport = *iter;
                        (*iter)->AddPassenger(plrMover);
                        break;
                    }
                }

                if (!plrMover->m_transport)
                    if (Map *tempMap = mover->GetMap())
                        if (GameObject *tempTransport = tempMap->GetGameObject(movementInfo.t_guid))
                            if (tempTransport->IsTransport())
                                plrMover->m_temp_transport = tempTransport;
            }
            else if (plrMover->GetTransport()->GetGUID() != movementInfo.t_guid)
            {
                bool foundNewTransport = false;
                plrMover->m_transport->RemovePassenger(plrMover);
                for (MapManager::TransportSet::const_iterator iter = sMapMgr->m_Transports.begin(); iter != sMapMgr->m_Transports.end(); ++iter)
                {
                    if ((*iter)->GetGUID() == movementInfo.t_guid)
                    {
                        foundNewTransport = true;
                        plrMover->m_transport = *iter;
                        (*iter)->AddPassenger(plrMover);
                        break;
                    }
                }

                if (!foundNewTransport)
                {
                    plrMover->m_transport = NULL;
                    movementInfo.t_pos.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
                    movementInfo.t_time = 0;
                    movementInfo.t_seat = -1;
                }
            }
        }

        if (!mover->GetTransport() && !mover->GetVehicle())
        {
            GameObject* go = mover->GetMap()->GetGameObject(movementInfo.t_guid);
            if (!go || go->GetGoType() != GAMEOBJECT_TYPE_TRANSPORT)
                movementInfo.t_guid = 0;
        }
    }
    else if (plrMover && (plrMover->m_transport || plrMover->m_temp_transport))                // if we were on a transport, leave
    {
        if (plrMover->m_transport)
        {
            plrMover->m_transport->RemovePassenger(plrMover);
            plrMover->m_transport = NULL;
        }
        plrMover->m_temp_transport = NULL;
        movementInfo.t_pos.Relocate(0.0f, 0.0f, 0.0f, 0.0f);
        movementInfo.t_time = 0;
        movementInfo.t_seat = -1;
    }

    // first...
    if (plrMover && !plrMover->GetVehicle())
    {
        // water walk
        if (!plrMover->HasMoveEventsMask(MOVE_EVENT_WATER_WALK) && movementInfo.HasMovementFlag(MOVEMENTFLAG_WATERWALKING))
            KickPlayer();

        // hover
        if (!plrMover->HasMoveEventsMask(MOVE_EVENT_HOVER) && movementInfo.HasMovementFlag(MOVEMENTFLAG_HOVER))
            KickPlayer();

        // fly
        if (!plrMover->HasMoveEventsMask(MOVE_EVENT_FLYING) && movementInfo.HasMovementFlag(MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING))
            KickPlayer();
    }

    // second...
    if (plrMover && !plrMover->GetTransport())
    {
        // teleport
        // delta = 20 yards
        float dist = plrMover->GetDistance(movementInfo.pos);
        if (dist > 20.0f)
        {
            if (!plrMover->HasMoveEventsMask(MOVE_EVENT_TELEPORT) && !plrMover->HasUnitState(UNIT_STATE_JUMPING))
                KickPlayer();
            else if (plrMover->HasMoveEventsMask(MOVE_EVENT_TELEPORT))
                plrMover->RemoveMoveEventsMask(MOVE_EVENT_TELEPORT);
        }
    }

    // fall damage generation (ignore in flight case that can be triggered also at lags in moment teleportation to another map).
    if (plrMover && plrMover->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING_FAR) && !movementInfo.HasMovementFlag(MOVEMENTFLAG_FALLING_FAR) && plrMover && !plrMover->isInFlight())
    {
        // movement anticheat
        plrMover->m_anti_JumpCount = 0;
        plrMover->m_anti_JumpBaseZ = 0;
        if(!plrMover->Zliquid_status)
            plrMover->HandleFall(movementInfo);
    }

    if (plrMover && ((movementInfo.flags & MOVEMENTFLAG_SWIMMING) != 0) != plrMover->IsInWater())
    {
        // now client not include swimming flag in case jumping under water
        plrMover->SetInWater(!plrMover->IsInWater() || plrMover->GetBaseMap()->IsUnderWater(movementInfo.pos.GetPositionX(), movementInfo.pos.GetPositionY(), movementInfo.pos.GetPositionZ()));
    }

    // check for swimming vehicles 
    if (plrMover && mover)
    {
        Vehicle *veh = mover->GetVehicleKit();
        if (veh && veh->GetBase())
        {
            if (Creature * vehCreature = veh->GetBase()->ToCreature())
            {
                if (!vehCreature->isInAccessiblePlaceFor(vehCreature))
                    plrMover->ExitVehicle();
            }
        }
    }

    /*----------------------*/
    // begin anti cheat
    bool check_passed = true;
    if(World::GetEnableMvAnticheatDebug())
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "AC2-%s > time: %d fall-time: %d | xyzo: %f, %f, %fo(%f) flags[%X] flags2[%X] UnitState[%X] opcode[%s] | mover (xyzo): %f, %f, %fo(%f)",
            plrMover->GetName(), movementInfo.time, movementInfo.fallTime, movementInfo.pos.GetPositionX(), movementInfo.pos.GetPositionY(), movementInfo.pos.GetPositionZ(), movementInfo.pos.GetOrientation(),
            movementInfo.flags, movementInfo.flags2, mover->GetUnitState(), GetOpcodeNameForLogging(opcode).c_str(), mover->GetPositionX(), mover->GetPositionY(), mover->GetPositionZ(), mover->GetOrientation());
    }

    if (plrMover && plrMover->GetTypeId() == TYPEID_PLAYER && !plrMover->HasUnitState(UNIT_STATE_LOST_CONTROL) &&
        !plrMover->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_TAXI_FLIGHT) && 
        mover->GetMotionMaster()->GetCurrentMovementGeneratorType() != POINT_MOTION_TYPE &&
        !(plrMover->m_transport || plrMover->m_temp_transport) && (plrMover->GetMapId() != 578 || plrMover->GetMapId() != 603))
    {
        float speed_plus = 1.5f;
        bool m_anty_check = true;
        bool m_anty_vechicle = plrMover->GetCharmerOrOwnerPlayerOrPlayerItself()->GetVehicle();
        bool m_anty_possess = plrMover->HasAuraType(SPELL_AURA_MOD_POSSESS);
        float delta_check_plus = 0.0f;
        if(m_anty_vechicle || m_anty_possess)
            delta_check_plus = 200.0f;

        if (World::GetEnableMvAnticheat() && plrMover->GetCharmerOrOwnerPlayerOrPlayerItself()->GetSession()->GetSecurity() < SEC_GAMEMASTER)
        {
            //check CX walkmode + time sinc
            if (movementInfo.flags & MOVEMENTFLAG_WALKING && _player->m_anti_MistiCount == 1)
            {
                plrMover->m_anti_MistiCount = 0;
                check_passed = false;
            }
            //end

            // calc time deltas
            int32 cClientTimeDelta = 1500;
            if (plrMover->m_anti_LastClientTime != 0)
            {
                cClientTimeDelta = movementInfo.time - plrMover->m_anti_LastClientTime;
                plrMover->m_anti_DeltaClientTime += cClientTimeDelta;
                plrMover->m_anti_LastClientTime = movementInfo.time;
            }
            else
                plrMover->m_anti_LastClientTime = movementInfo.time;
 
            const uint64 cServerTime = getMSTime();
            uint32 cServerTimeDelta = 1500;
            if (plrMover->m_anti_LastServerTime != 0)
            {
                cServerTimeDelta = cServerTime - plrMover->m_anti_LastServerTime;
                plrMover->m_anti_DeltaServerTime += cServerTimeDelta;
                plrMover->m_anti_LastServerTime = cServerTime;
            }
            else
                plrMover->m_anti_LastServerTime = cServerTime;

            // resync times on client login (first 15 sec for heavy areas)
            if (plrMover->m_anti_DeltaServerTime < 15000 && plrMover->m_anti_DeltaClientTime < 15000)
                plrMover->m_anti_DeltaClientTime = plrMover->m_anti_DeltaServerTime;

            const int32 sync_time = plrMover->m_anti_DeltaClientTime - plrMover->m_anti_DeltaServerTime;

            if(World::GetEnableMvAnticheatDebug())
                sLog->outError(LOG_FILTER_NETWORKIO, "AC2-%s Time > cClientTimeDelta: %d, cServerTime: %d | deltaC: %d - deltaS: %d | SyncTime: %d, opcode[%s]",
                plrMover->GetName(), cClientTimeDelta, cServerTime, plrMover->m_anti_DeltaClientTime, plrMover->m_anti_DeltaServerTime, sync_time, GetOpcodeNameForLogging(opcode).c_str());

            // mistiming checks
            const int32 GetMistimingDelta = abs(int32(World::GetMistimingDelta()));
            if (sync_time > GetMistimingDelta)
            {
                cClientTimeDelta = cServerTimeDelta;
                ++(plrMover->m_anti_MistimingCount);

                const bool bMistimingModulo = plrMover->m_anti_MistimingCount % 50 == 0;

                if (bMistimingModulo)
                {
                    if(World::GetEnableMvAnticheatDebug())
                        sLog->outError(LOG_FILTER_NETWORKIO, "AC2-%s, mistiming exception #%d, mistiming: %dms, opcode[%s]", plrMover->GetName(), plrMover->m_anti_MistimingCount, sync_time, GetOpcodeNameForLogging(opcode).c_str());

                    check_passed = false;
                }                   
            }
            // end mistiming checks

            const uint32 curDest = plrMover->m_taxi.GetTaxiDestination(); // check taxi flight
            if (!curDest)
            {
                // calculating section
                // current speed
                if(plrMover->HasAuraType(SPELL_AURA_FEATHER_FALL))
                    speed_plus = 7.0f;
                if(plrMover->HasAura(19503))
                    speed_plus = 15.0f;
                if(plrMover->HasAura(2983) || plrMover->HasAura(48594) || plrMover->HasAura(56354) || plrMover->HasAura(32720) || plrMover->HasAura(3714))
                    speed_plus = 4.0f;

                float current_speed = mover->GetSpeed(MOVE_RUN) > mover->GetSpeed(MOVE_FLIGHT) ? mover->GetSpeed(MOVE_RUN) : mover->GetSpeed(MOVE_FLIGHT);
                if(current_speed < mover->GetSpeed(MOVE_SWIM))
                    current_speed = mover->GetSpeed(MOVE_SWIM);
                current_speed *= speed_plus + mover->m_TempSpeed;
                bool speed_check = true;

                if(mover->m_anti_JupmTime && mover->m_anti_JupmTime > 0)
                {
                    plrMover->m_anti_LastSpeedChangeTime = movementInfo.time + mover->m_anti_JupmTime;
                    speed_check = false;

                    if(mover->m_anti_JupmTime <= diff)
                    {
                        mover->m_anti_JupmTime = 0;
                        speed_check = true;
                    }
                    else
                        mover->m_anti_JupmTime -= diff;
                }
                // end current speed

                // movement distance
                const float delta_x = (plrMover->m_transport || plrMover->m_temp_transport) ? 0 : mover->GetPositionX() - movementInfo.pos.GetPositionX();
                const float delta_y = (plrMover->m_transport || plrMover->m_temp_transport) ? 0 : mover->GetPositionY() - movementInfo.pos.GetPositionY();
                const float delta_z = (plrMover->m_transport || plrMover->m_temp_transport) ? 0 : mover->GetPositionZ() - movementInfo.pos.GetPositionZ();
                const float real_delta = (plrMover->m_transport || plrMover->m_temp_transport) ? 0 : (pow(delta_x, 2) + pow(delta_y, 2));
                 // end movement distance

                const bool fly_auras = (plrMover->HasAuraType(SPELL_AURA_FLY) || plrMover->HasAuraType(SPELL_AURA_MOD_INCREASE_VEHICLE_FLIGHT_SPEED)
                    || plrMover->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED) || plrMover->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED)
                    || plrMover->HasAuraType(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS));
                const bool fly_flags = (movementInfo.flags & (MOVEMENTFLAG_CAN_FLY | MOVEMENTFLAG_FLYING | MOVEMENTFLAG_DISABLE_GRAVITY | MOVEMENTFLAG_ASCENDING));
                bool exeption_fly = true;

                if(mover->m_anti_FlightTime && mover->m_anti_FlightTime > 0)
                {
                    if (!fly_auras && fly_flags)
                        exeption_fly = false;

                    if(mover->m_anti_FlightTime <= diff)
                    {
                        mover->m_anti_FlightTime = 0;
                        exeption_fly = true;
                    }
                    else
                        mover->m_anti_FlightTime -= diff;
                }

                float _vmapHeight = 0.0f;
                float _Height = 0.0f;
                //if in fly crash on check VmapHeight
                if(!fly_auras)
                {
                    _vmapHeight = plrMover->GetMap()->GetVmapHeight(movementInfo.pos.GetPositionX(), movementInfo.pos.GetPositionY(), movementInfo.pos.GetPositionZ());
                    _Height = plrMover->GetMap()->GetHeight(movementInfo.pos.GetPositionX(), movementInfo.pos.GetPositionY(), movementInfo.pos.GetPositionZ());
                }
                const float ground_Z = movementInfo.pos.GetPositionZ() - _vmapHeight;

                if(cClientTimeDelta == 0)
                    cClientTimeDelta = 1500;
                if (cClientTimeDelta < 0)
                    cClientTimeDelta = 0;
                const float time_delta = cClientTimeDelta < 1500 ? float(cClientTimeDelta)/1000.0f : 1.5f; // normalize time - 1.5 second allowed for heavy loaded server

                const float tg_z = (real_delta != 0 && !fly_auras && !plrMover->Zliquid_status) ? (pow(delta_z, 2) / real_delta) : -99999; // movement distance tangents

                if (current_speed < plrMover->m_anti_Last_HSpeed && plrMover->m_anti_LastSpeedChangeTime == 0)
                    plrMover->m_anti_LastSpeedChangeTime = movementInfo.time + uint32(floor(((plrMover->m_anti_Last_HSpeed / current_speed) * 1500)) + 100); // 100ms above for random fluctuation

                const float allowed_delta = (plrMover->m_transport || plrMover->m_temp_transport) ? 2 : // movement distance allowed delta
                    pow(std::max(current_speed, plrMover->m_anti_Last_HSpeed) * time_delta, 2)
                    + 2                                                                             // minimum allowed delta
                    + (tg_z > 2.2 ? pow(delta_z, 2)/2.37f : 0);                                      // mountain fall allowed delta

                    if(World::GetEnableMvAnticheatDebug())
                        sLog->outError(LOG_FILTER_NETWORKIO, "AC444 out m_anti_JupmTime %u current_speed %f allowed_delta %f real_delta %f fly_auras %u fly_flags %u _vmapHeight %f, _Height %f, ZLiquidStatus %u, opcode[%s]",
                                        mover->m_anti_JupmTime, current_speed, allowed_delta, real_delta, fly_auras, fly_flags, _vmapHeight, _Height, plrMover->Zliquid_status, GetOpcodeNameForLogging(opcode).c_str());

                if (movementInfo.time > plrMover->m_anti_LastSpeedChangeTime)
                {
                    plrMover->m_anti_Last_HSpeed = current_speed;                                    // store current speed
                    plrMover->m_anti_Last_VSpeed = -2.3f;
                    plrMover->m_anti_LastSpeedChangeTime = 0;
                }
                // end calculating section

                // speed and teleport hack checks
                if (real_delta > (allowed_delta + delta_check_plus))
                {
                    if(World::GetEnableMvAnticheatDebug())
                        if (real_delta < 4900.0f)
                            sLog->outError(LOG_FILTER_NETWORKIO, "AC2-%s, speed exception | cDelta=%f aDelta=%f | cSpeed=%f lSpeed=%f deltaTime=%f, opcode[%s]", plrMover->GetName(), real_delta, allowed_delta, current_speed, plrMover->m_anti_Last_HSpeed, time_delta, GetOpcodeNameForLogging(opcode).c_str());
                        else
                            sLog->outError(LOG_FILTER_NETWORKIO, "AC2-%s, teleport exception | cDelta=%f aDelta=%f | cSpeed=%f lSpeed=%f deltaTime=%f, opcode[%s]", plrMover->GetName(), real_delta, allowed_delta, current_speed, plrMover->m_anti_Last_HSpeed, time_delta, GetOpcodeNameForLogging(opcode).c_str());

                    if(speed_check || real_delta > 4900.0f)
                        check_passed = false;

                    plrMover->FallGroundAnt();
                }
 
                // Fly hack checks
                if (!fly_auras && (fly_flags || ground_Z > 2.3f) && !forvehunit && exeption_fly && !plrMover->Zliquid_status)
                {
                    if(World::GetEnableMvAnticheatDebug())
                        sLog->outError(LOG_FILTER_NETWORKIO, "AC2-%s, flight exception. {SPELL_AURA_FLY=[%X]} {SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED=[%X]} {SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED=[%X]} {SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS=[%X]} {SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK=[%X]} {plrMover->GetVehicle()=[%X]} forvehunit=[%X], opcode[%s]",
                            plrMover->GetName(),
                            plrMover->HasAuraType(SPELL_AURA_FLY), plrMover->HasAuraType(SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED),
                            plrMover->HasAuraType(SPELL_AURA_MOD_INCREASE_FLIGHT_SPEED), plrMover->HasAuraType(SPELL_AURA_MOD_MOUNTED_FLIGHT_SPEED_ALWAYS),
                            plrMover->HasAuraType(SPELL_AURA_MOD_FLIGHT_SPEED_NOT_STACK), plrMover->GetVehicle(), forvehunit, GetOpcodeNameForLogging(opcode).c_str());

                    //check_passed = false;
                    plrMover->SendMovementSetCanFly(true);
                    plrMover->SendMovementSetCanFly(false);
                    //plrMover->FallGroundAnt();
                }

                // Teleport To Plane checks
                if (!plrMover->Zliquid_status && movementInfo.pos.GetPositionZ() < 0.0001f && movementInfo.pos.GetPositionZ() > -0.0001f)
                {
                    if (const Map *map = plrMover->GetMap())
                    {
                        float plane_z = map->GetHeight(movementInfo.pos.GetPositionX(), movementInfo.pos.GetPositionY(), MAX_HEIGHT) - movementInfo.pos.GetPositionZ();
                        plane_z = (plane_z < -500.0f) ? 0.0f : plane_z; // check holes in height map
                        if (plane_z > 0.1f || plane_z < -0.1f)
                        {
                            if(World::GetEnableMvAnticheatDebug())
                                sLog->outError(LOG_FILTER_NETWORKIO, "AC2-%s, teleport to plane exception. plane_z: %f, opcode[%s]", plrMover->GetName(), plane_z, GetOpcodeNameForLogging(opcode).c_str());

                            if(World::GetEnableMvAnticheatDebug())
                                if (plrMover->m_anti_TeleToPlane_Count > World::GetTeleportToPlaneAlarms())
                                    sLog->outError(LOG_FILTER_NETWORKIO, "AC2-%s, teleport to plane exception. Exception count: %d, opcode[%s]", plrMover->GetName(), plrMover->m_anti_TeleToPlane_Count, GetOpcodeNameForLogging(opcode).c_str());

                            ++(plrMover->m_anti_TeleToPlane_Count);
                            check_passed = false;
                        }
                    }
                }
                else
                    plrMover->m_anti_TeleToPlane_Count = 0;
            }
        }
    }

    /* process position-change */
    if (check_passed)
    {
        WorldPacket data(SMSG_PLAYER_MOVE, recvPacket.size());
        movementInfo.time = getMSTime();
        movementInfo.guid = mover->GetGUID();
        WorldSession::WriteMovementInfo(data, &movementInfo);
        mover->SendMessageToSet(&data, _player);

        mover->m_movementInfo = movementInfo;

        // this is almost never true (not sure why it is sometimes, but it is), normally use mover->IsVehicle()
        if (mover->GetVehicle())
        {
            mover->SetOrientation(movementInfo.pos.GetOrientation());
            return;
        }

        mover->UpdatePosition(movementInfo.pos);

        if (opcode == CMSG_MOVE_KNOCK_BACK_ACK)
            mover->AddUnitState(UNIT_STATE_JUMPING);

        if (opcode == CMSG_MOVE_FALL_LAND)
        {
            if (mover->HasAuraType(SPELL_AURA_MOD_CONFUSE) && mover->HasUnitState(UNIT_STATE_JUMPING))
            {
                 mover->GetMotionMaster()->MoveConfused();

                 if (plrMover)
                    plrMover->SetClientControl(mover, false);
            }

            mover->ClearUnitState(UNIT_STATE_JUMPING);
            mover->m_TempSpeed = 0.0f;
        }

        if (plrMover)                                            // nothing is charmed, or player charmed
        {
            plrMover->UpdateFallInformationIfNeed(movementInfo, opcode);

            AreaTableEntry const* zone = GetAreaEntryByAreaID(plrMover->GetAreaId());
            float depth = zone ? zone->MaxDepth : -500.0f;
            if (movementInfo.pos.GetPositionZ() < depth)
            {
                if (!(plrMover->GetBattleground() && plrMover->GetBattleground()->HandlePlayerUnderMap(_player)))
                {
                    // by CyberBrest: Nice! Best regards. Just kill man,  maybe return them to the grave??? 
                    // move to grave, and then kill.
                    plrMover->RepopAtGraveyard();

                    // NOTE: this is actually called many times while falling
                    // even after the player has been teleported away
                    // TODO: discard movement packets after the player is rooted
                    if (plrMover->isAlive())
                    {
                        plrMover->EnvironmentalDamage(DAMAGE_FALL_TO_VOID, GetPlayer()->GetMaxHealth());
                        // player can be alive if GM/etc
                        // change the death state to CORPSE to prevent the death timer from
                        // starting in the next player update
                        if (!plrMover->isAlive())
                            plrMover->KillPlayer();
                    }
                }
            }
        }
    }
    else if (plrMover)
    {
        if (plrMover->m_transport)
        {
            plrMover->m_transport->RemovePassenger(plrMover);
            plrMover->m_transport = NULL;
        }
        plrMover->m_temp_transport = NULL;
        ++(plrMover->m_anti_AlarmCount);
        WorldPacket data(SMSG_PLAYER_MOVE);
        plrMover->SetUnitMovementFlags(0);
        //plrMover->SendTeleportPacket();
        plrMover->WriteMovementUpdate(data);
        plrMover->SendMessageToSet(&data, true);
        plrMover->SendMovementSetCanFly(true);
        plrMover->SendMovementSetCanFly(false);
        plrMover->FallGroundAnt();
    }
}

void WorldSession::HandleSetActiveMoverOpcode(WorldPacket& recvPacket)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_SET_ACTIVE_MOVER");

    ObjectGuid guid;

    recvPacket.ReadBit(); //unk
    recvPacket.ReadGuidMask<4, 7, 0, 1, 5, 6, 2, 3>(guid);
    recvPacket.ReadGuidBytes<6, 5, 4, 3, 1, 0, 7, 2>(guid);

    if (GetPlayer()->IsInWorld())
    {
        if (_player->m_mover->GetGUID() != guid)
            sLog->outError(LOG_FILTER_NETWORKIO, "HandleSetActiveMoverOpcode: incorrect mover guid: mover is " UI64FMTD " (%s - Entry: %u) and should be " UI64FMTD, uint64(guid), GetLogNameForGuid(guid), GUID_ENPART(guid), _player->m_mover->GetGUID());
    }
}

void WorldSession::HandleMoveNotActiveMover(WorldPacket &recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "WORLD: Recvd CMSG_MOVE_NOT_ACTIVE_MOVER");

    uint64 old_mover_guid;
    recvData.readPackGUID(old_mover_guid);

    MovementInfo mi;
    ReadMovementInfo(recvData, &mi);

    mi.guid = old_mover_guid;

    _player->m_movementInfo = mi;
}

void WorldSession::HandleMountSpecialAnimOpcode(WorldPacket& /*recvData*/)
{
    ObjectGuid guid = _player->GetObjectGuid();

    WorldPacket data(SMSG_MOUNTSPECIAL_ANIM, 8 + 1);
    data.WriteGuidMask<2, 4, 3, 7, 1, 0, 5, 6>(guid);
    data.WriteGuidBytes<4, 1, 3, 0, 7, 2, 5, 6>(guid);

    GetPlayer()->SendMessageToSet(&data, false);
}

// ACKS

void WorldSession::HandleForceSpeedChangeAck(WorldPacket &recvData)
{
    uint32 opcode = recvData.GetOpcode();

    /* extract packet */
    uint64 guid;
    uint32 unk1;
    float  newspeed;

    recvData.readPackGUID(guid);

    // now can skip not our packet
    if (_player->GetGUID() != guid)
    {
        recvData.rfinish();                   // prevent warnings spam
        return;
    }

    // continue parse packet

    recvData >> unk1;                                      // counter or moveEvent

    MovementInfo movementInfo;
    movementInfo.guid = guid;
    ReadMovementInfo(recvData, &movementInfo);

    recvData >> newspeed;
    /*----------------*/

    // client ACK send one packet for mounted/run case and need skip all except last from its
    // in other cases anti-cheat check can be fail in false case
    UnitMoveType move_type = MOVE_WALK;
    UnitMoveType force_move_type = MOVE_WALK;

    static char const* move_type_name[MAX_MOVE_TYPE] = { "Walk", "Run", "RunBack", "Swim", "SwimBack", "TurnRate", "Flight", "FlightBack", "PitchRate" };

    // skip all forced speed changes except last and unexpected
    // in run/mounted case used one ACK and it must be skipped.m_forced_speed_changes[MOVE_RUN} store both.
    if (_player->m_forced_speed_changes[force_move_type] > 0)
    {
        --_player->m_forced_speed_changes[force_move_type];
        if (_player->m_forced_speed_changes[force_move_type] > 0)
            return;
    }

    if (!_player->GetTransport() && fabs(_player->GetSpeed(move_type) - newspeed) > 0.01f)
    {
        if (_player->GetSpeed(move_type) > newspeed)         // must be greater - just correct
        {
            sLog->outError(LOG_FILTER_NETWORKIO, "%sSpeedChange player %s is NOT correct (must be %f instead %f), force set to correct value",
                move_type_name[move_type], _player->GetName(), _player->GetSpeed(move_type), newspeed);
            _player->SetSpeed(move_type, _player->GetSpeedRate(move_type), true);
        }
        else                                                // must be lesser - cheating
        {
            sLog->outDebug(LOG_FILTER_GENERAL, "Player %s from account id %u kicked for incorrect speed (must be %f instead %f)",
                _player->GetName(), _player->GetSession()->GetAccountId(), _player->GetSpeed(move_type), newspeed);
            _player->GetSession()->KickPlayer();
        }
    }
}

void WorldSession::HandleMoveKnockBackAck(WorldPacket & recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_MOVE_KNOCK_BACK_ACK");

    if (Unit* mover = _player->m_mover)
        mover->AddUnitState(UNIT_STATE_JUMPING);

    HandleMovementOpcodes(recvData);

    /*MovementInfo movementInfo;
    ReadMovementInfo(recvData, &movementInfo);

    if (_player->m_mover->GetGUID() != movementInfo.guid)
    return;

    _player->m_movementInfo = movementInfo;

    WorldPacket data(SMSG_MOVE_UPDATE_KNOCK_BACK, 66);
    WriteMovementInfo(data, &movementInfo);

    _player->SendMessageToSet(&data, false);*/
}

void WorldSession::HandleMoveFeatherFallAck(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_MOVE_FEATHER_FALL_ACK");

    // no used
    recvData.rfinish();                       // prevent warnings spam
}

void WorldSession::HandleMoveGravityEnableAck(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_MOVE_GRAVITY_ENABLE_ACK");

    uint32 ackIndex;
    recvData.read_skip<float>();
    recvData.read_skip<float>();
    recvData.read_skip<float>();
    recvData >> ackIndex;

    recvData.rfinish();

    //_player->ToggleMoveEventsMask(MOVE_EVENT_GRAVITY);
}

void WorldSession::HandleMoveHoverAck(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_MOVE_HOVER_ACK");

    uint32 ackIndex;
    recvData.read_skip<float>();
    recvData.read_skip<float>();
    recvData >> ackIndex;

    recvData.rfinish();

    _player->ToggleMoveEventsMask(MOVE_EVENT_HOVER);
}

void WorldSession::HandleMoveWaterwalkAck(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_MOVE_WATER_WALK_ACK");

    uint32 ackIndex;
    recvData >> ackIndex;

    recvData.rfinish();

    _player->ToggleMoveEventsMask(MOVE_EVENT_WATER_WALK);
}

void WorldSession::HandleMoveSetCanFlyAck(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_MOVE_SET_CAN_FLY_ACK");

    uint32 ackIndex;
    recvData.read_skip<float>();
    recvData.read_skip<float>();
    recvData >> ackIndex;

    recvData.rfinish();

    _player->ToggleMoveEventsMask(MOVE_EVENT_FLYING);
}

void WorldSession::HandleMoveSetCanTransBtwSwimFlyAck(WorldPacket& recvData)
{
    sLog->outDebug(LOG_FILTER_NETWORKIO, "CMSG_MOVE_SET_CAN_TRANS_BETWEEN_SWIM_AND_FLY_ACK");

    uint32 ackIndex;
    recvData.read_skip<float>();
    recvData.read_skip<float>();
    recvData >> ackIndex;

    recvData.rfinish();
}

void WorldSession::HandleSummonResponseOpcode(WorldPacket& recvData)
{
    if (!_player->isAlive() || _player->isInCombat())
        return;

    ObjectGuid summonerGuid;
    bool agree;

    recvData.ReadGuidMask<0, 7, 3, 1, 6, 2, 4>(summonerGuid);
    agree = recvData.ReadBit();
    recvData.ReadGuidMask<5>(summonerGuid);

    recvData.ReadGuidBytes<2, 0, 4, 5, 7, 1, 3, 6>(summonerGuid);

    _player->SummonIfPossible(agree);
}

void WorldSession::ReadMovementInfo(WorldPacket& data, MovementInfo* mi)
{
    MovementStatusElements* sequence = GetMovementStatusElementsSequence(data.GetOpcode());
    if (sequence == NULL)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "WorldSession::ReadMovementInfo: No movement sequence found for opcode 0x%04X", uint32(data.GetOpcode()));
        return;
    }

    ObjectGuid guid;
    ObjectGuid tguid;
    bool hasTransportData = false;
    bool hasMovementFlags = false;
    bool hasMovementFlags2 = false;
    bool hasOrientation = false;
    bool hasTimeStamp = false;
    uint32 counter = 0;

    for (uint32 i = 0; i < MSE_COUNT; ++i)
    {
        MovementStatusElements element = sequence[i];
        if (element == MSEEnd)
            break;

        if (element >= MSEHasGuidByte0 && element <= MSEHasGuidByte7)
        {
            guid[element - MSEHasGuidByte0] = data.ReadBit();
            continue;
        }

        if (element >= MSEHasTransportGuidByte0 &&
            element <= MSEHasTransportGuidByte7)
        {
            if (hasTransportData)
                tguid[element - MSEHasTransportGuidByte0] = data.ReadBit();
            continue;
        }

        if (element >= MSEGuidByte0 && element <= MSEGuidByte7)
        {
            data.ReadByteSeq(guid[element - MSEGuidByte0]);
            continue;
        }

        if (element >= MSETransportGuidByte0 &&
            element <= MSETransportGuidByte7)
        {
            if (hasTransportData)
                data.ReadByteSeq(tguid[element - MSETransportGuidByte0]);
            continue;
        }

        switch (element)
        {
            case MSEBitCounter:
                counter = data.ReadBits(22);
                break;
            case MSEBitCounterValues:
                for (uint32 i = 0; i < counter; ++i)
                    mi->unkCounter.push_back(data.read<uint32>());
                break;
            case MSEFlushBits:
                data.FlushBits();
                break;
            case MSEHasMovementFlags:
                hasMovementFlags = !data.ReadBit();
                break;
            case MSEHasMovementFlags2:
                hasMovementFlags2 = !data.ReadBit();
                break;
            case MSEHasTimestamp:
                hasTimeStamp = !data.ReadBit();
                break;
            case MSEHasOrientation:
                hasOrientation = !data.ReadBit();
                break;
            case MSEHasTransportData:
                hasTransportData = data.ReadBit();
                break;
            case MSEHasTransportTime2:
                if (hasTransportData)
                    mi->hasTransportTime2 = data.ReadBit();
                break;
            case MSEHasTransportTime3:
                if (hasTransportData)
                    mi->hasTransportTime3 = data.ReadBit();
                break;
            case MSEHasPitch:
                mi->hasPitch = !data.ReadBit();
                break;
            case MSEHasFallData:
                mi->hasFallData = data.ReadBit();
                break;
            case MSEHasFallDirection:
                if (mi->hasFallData)
                    mi->hasFallDirection = data.ReadBit();
                break;
            case MSEHasSplineElevation:
                mi->hasSplineElevation = !data.ReadBit();
                break;
            case MSEHasSpline:
                mi->hasSpline = data.ReadBit();
                break;
            case MSEMovementFlags:
                if (hasMovementFlags)
                    mi->flags = data.ReadBits(30);
                break;
            case MSEMovementFlags2:
                if (hasMovementFlags2)
                    mi->flags2 = data.ReadBits(13);
                break;
            case MSETimestamp:
                if (hasTimeStamp)
                    data >> mi->time;
                break;
            case MSEPositionX:
                data >> mi->pos.m_positionX;
                break;
            case MSEPositionY:
                data >> mi->pos.m_positionY;
                break;
            case MSEPositionZ:
                data >> mi->pos.m_positionZ;
                break;
            case MSEOrientation:
                if (hasOrientation)
                    mi->pos.SetOrientation(data.read<float>());
                break;
            case MSETransportPositionX:
                if (hasTransportData)
                    data >> mi->t_pos.m_positionX;
                break;
            case MSETransportPositionY:
                if (hasTransportData)
                    data >> mi->t_pos.m_positionY;
                break;
            case MSETransportPositionZ:
                if (hasTransportData)
                    data >> mi->t_pos.m_positionZ;
                break;
            case MSETransportOrientation:
                if (hasTransportData)
                    mi->pos.SetOrientation(data.read<float>());
                break;
            case MSETransportSeat:
                if (hasTransportData)
                    data >> mi->t_seat;
                break;
            case MSETransportTime:
                if (hasTransportData)
                    data >> mi->t_time;
                break;
            case MSETransportTime2:
                if (hasTransportData && mi->hasTransportTime2)
                    data >> mi->t_time2;
                break;
            case MSETransportTime3:
                if (hasTransportData && mi->hasTransportTime3)
                    data >> mi->t_time3;
                break;
            case MSEPitch:
                if (mi->hasPitch)
                    data >> mi->pitch;
                break;
            case MSEFallTime:
                if (mi->hasFallData)
                    data >> mi->fallTime;
                break;
            case MSEFallVerticalSpeed:
                if (mi->hasFallData)
                    data >> mi->j_zspeed;
                break;
            case MSEFallCosAngle:
                if (mi->hasFallData && mi->hasFallDirection)
                    data >> mi->j_cosAngle;
                break;
            case MSEFallSinAngle:
                if (mi->hasFallData && mi->hasFallDirection)
                    data >> mi->j_sinAngle;
                break;
            case MSEFallHorizontalSpeed:
                if (mi->hasFallData && mi->hasFallDirection)
                    data >> mi->j_xyspeed;
                break;
            case MSESplineElevation:
                if (mi->hasSplineElevation)
                    data >> mi->splineElevation;
                break;
            case MSEBit95:
                mi->byte95 = data.ReadBit();
                break;
            case MSEBitAC:
                mi->byteAC = data.ReadBit();
                break;
            case MSEHasUnkInt32:
                mi->hasUnkInt32 = !data.ReadBit();
                break;
            case MSEUnkInt32:
                if (mi->hasUnkInt32)
                    data >> mi->unkInt32;
                break;
            case MSECounter:
                data.read_skip<uint32>();
                break;
            default:
                ASSERT(false && "Incorrect sequence element detected at ReadMovementInfo");
                break;
        }
    }

    mi->guid = guid;
    mi->t_guid = tguid;

   if (hasTransportData && mi->pos.m_positionX != mi->t_pos.m_positionX)
       if (GetPlayer()->GetTransport())
           GetPlayer()->GetTransport()->UpdatePosition(mi);
}

void WorldSession::WriteMovementInfo(WorldPacket &data, MovementInfo* mi, Unit* unit /* = NULL*/)
{
    bool hasMovementFlags = mi->GetMovementFlags() != 0;
    bool hasMovementFlags2 = mi->GetExtraMovementFlags() != 0;
    bool hasTimestamp = mi->time != 0;
    bool hasOrientation = !G3D::fuzzyEq(mi->pos.GetOrientation(), 0.0f);
    bool hasTransportData = mi->t_guid != 0;

    MovementStatusElements* sequence = GetMovementStatusElementsSequence(data.GetOpcode());
    if (!sequence)
    {
        sLog->outError(LOG_FILTER_NETWORKIO, "WorldSession::WriteMovementInfo: No movement sequence found for opcode 0x%04X", uint32(data.GetOpcode()));
        return;
    }

    ObjectGuid guid = mi->guid;
    ObjectGuid tguid = mi->t_guid;

    for(uint32 i = 0; i < MSE_COUNT; ++i)
    {
        MovementStatusElements element = sequence[i];
        if (element == MSEEnd)
            break;

        if (element >= MSEHasGuidByte0 && element <= MSEHasGuidByte7)
        {
            data.WriteBit(guid[element - MSEHasGuidByte0]);
            continue;
        }

        if (element >= MSEHasTransportGuidByte0 &&
            element <= MSEHasTransportGuidByte7)
        {
            if (hasTransportData)
                data.WriteBit(tguid[element - MSEHasTransportGuidByte0]);
            continue;
        }

        if (element >= MSEGuidByte0 && element <= MSEGuidByte7)
        {
            data.WriteByteSeq(guid[element - MSEGuidByte0]);
            continue;
        }

        if (element >= MSETransportGuidByte0 &&
            element <= MSETransportGuidByte7)
        {
            if (hasTransportData)
                data.WriteByteSeq(tguid[element - MSETransportGuidByte0]);
            continue;
        }

        switch (element)
        {
            case MSEBitCounter:
                data.WriteBits(mi->unkCounter.size(), 22);
                break;
            case MSEBitCounterValues:
                for (uint32 i = 0; i < mi->unkCounter.size(); ++i)
                    data << uint32(mi->unkCounter[i]);
                break;
            case MSEFlushBits:
                data.FlushBits();
                break;
            case MSEHasMovementFlags:
                data.WriteBit(!hasMovementFlags);
                break;
            case MSEHasMovementFlags2:
                data.WriteBit(!hasMovementFlags2);
                break;
            case MSEHasTimestamp:
                data.WriteBit(!hasTimestamp);
                break;
            case MSEHasOrientation:
                data.WriteBit(!hasOrientation);
                break;
            case MSEHasTransportData:
                data.WriteBit(hasTransportData);
                break;
            case MSEHasTransportTime2:
                if (hasTransportData)
                    data.WriteBit(mi->hasTransportTime2);
                break;
            case MSEHasTransportTime3:
                if (hasTransportData)
                    data.WriteBit(mi->hasTransportTime3);
                break;
            case MSEHasPitch:
                data.WriteBit(!mi->hasPitch);
                break;
            case MSEHasFallData:
                data.WriteBit(mi->hasFallData);
                break;
            case MSEHasFallDirection:
                if (mi->hasFallData)
                    data.WriteBit(mi->hasFallDirection);
                break;
            case MSEHasSplineElevation:
                data.WriteBit(!mi->hasSplineElevation);
                break;
            case MSEHasSpline:
                data.WriteBit(mi->hasSpline);
                break;
            case MSEMovementFlags:
                if (hasMovementFlags)
                    data.WriteBits(mi->flags, 30);
                break;
            case MSEMovementFlags2:
                if (hasMovementFlags2)
                    data.WriteBits(mi->flags2, 13);
                break;
            case MSETimestamp:
                if (hasTimestamp)
                    data << mi->time;
                break;
            case MSEPositionX:
                data << mi->pos.m_positionX;
                break;
            case MSEPositionY:
                data << mi->pos.m_positionY;
                break;
            case MSEPositionZ:
                data << mi->pos.m_positionZ;
                break;
            case MSEOrientation:
                if (hasOrientation)
                    data << Position::NormalizeOrientation(mi->pos.GetOrientation());
                break;
            case MSETransportPositionX:
                if (hasTransportData)
                    data << mi->t_pos.m_positionX;
                break;
            case MSETransportPositionY:
                if (hasTransportData)
                    data << mi->t_pos.m_positionY;
                break;
            case MSETransportPositionZ:
                if (hasTransportData)
                    data << mi->t_pos.m_positionZ;
                break;
            case MSETransportOrientation:
                if (hasTransportData)
                    data << Position::NormalizeOrientation(mi->t_pos.GetOrientation());
                break;
            case MSETransportSeat:
                if (hasTransportData)
                    data << mi->t_seat;
                break;
            case MSETransportTime:
                if (hasTransportData)
                    data << mi->t_time;
                break;
            case MSETransportTime2:
                if (hasTransportData && mi->hasTransportTime2)
                    data << mi->t_time2;
                break;
            case MSETransportTime3:
                if (hasTransportData && mi->hasTransportTime3)
                    data << mi->t_time3;
                break;
            case MSEPitch:
                if (mi->hasPitch)
                    data << Position::NormalizePitch(mi->pitch);
                break;
            case MSEFallTime:
                if (mi->hasFallData)
                    data << mi->fallTime;
                break;
            case MSEFallVerticalSpeed:
                if (mi->hasFallData)
                    data << mi->j_zspeed;
                break;
            case MSEFallCosAngle:
                if (mi->hasFallData && mi->hasFallDirection)
                    data << mi->j_cosAngle;
                break;
            case MSEFallSinAngle:
                if (mi->hasFallData && mi->hasFallDirection)
                    data << mi->j_sinAngle;
                break;
            case MSEFallHorizontalSpeed:
                if (mi->hasFallData && mi->hasFallDirection)
                    data << mi->j_xyspeed;
                break;
            case MSESplineElevation:
                if (mi->hasSplineElevation)
                    data << mi->splineElevation;
                break;
            case MSEBit95:
                data.WriteBit(mi->byte95);
                break;
            case MSEBitAC:
                data.WriteBit(mi->byteAC);
                break;
            case MSEHasUnkInt32:
                data.WriteBit(!mi->hasUnkInt32);
                break;
            case MSEUnkInt32:
                if (mi->hasUnkInt32)
                    data << mi->unkInt32;
                break;
            case MSECounter:
                data << uint32(0);
                break;
            default:
                ASSERT(false && "Incorrect sequence element detected at WriteMovementInfo");
                break;
        }
    }
}
