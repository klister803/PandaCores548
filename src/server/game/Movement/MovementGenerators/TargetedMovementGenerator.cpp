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

#include "ByteBuffer.h"
#include "TargetedMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "World.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "Player.h"

#define RECHECK_DISTANCE_TIMER 100
#define TARGET_NOT_ACCESSIBLE_MAX_TIMER 5000

template<class T, typename D>
void TargetedMovementGeneratorMedium<T,D>::_setTargetLocation(T &owner)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
        return;

    if (owner.HasUnitState(UNIT_STATE_NOT_MOVE))
        return;

    // Hack for Beth'tilac
    switch((&owner)->GetEntry())
    {
        case 52498: // Beth'tilac
            if ((&owner)->GetMapId() == 720)
                if ((&owner)->GetAI()->GetData(2) == 0 && i_target->GetPositionZ() < 100.0f)
                    return;
            break;
        case 52581: // Cinderweb Drone
        case 52447: // Cinderweb Spiderling
        case 53745: // Engorged Broodling
            if ((&owner)->GetMapId() == 720)
                if (i_target->GetPositionZ() > 100.0f)
                    return;
            break;
    }

    if (owner.GetTypeId() == TYPEID_UNIT && !i_target->isInAccessiblePlaceFor(((Creature*)&owner)))
        return;

    float x, y, z;
    bool targetIsVictim = owner.getVictim() && owner.getVictim()->GetGUID() == i_target->GetGUID();

    if (!i_offset)
    {
        // to nearest contact position
        float dist = 0.0f;
        if (targetIsVictim)
            dist = owner.GetFloatValue(UNIT_FIELD_COMBATREACH) + i_target->GetFloatValue(UNIT_FIELD_COMBATREACH) - i_target->GetObjectSize() - owner.GetObjectSize() - 1.0f;

        if (dist < 0.5f)
            dist = 0.5f;

        if(owner.GetTypeId() == TYPEID_UNIT && ((Creature*)&owner)->isPet() && !i_target->IsWithinLOSInMap(&owner))
        {
            Position pos;
            i_target->GetFirstCollisionPosition(pos, MELEE_RANGE - 0.5f, 180.0f);
            x = pos.m_positionX;
            y = pos.m_positionY;
            z = pos.m_positionZ;
        }
        else
            i_target->GetContactPoint(&owner, x, y, z, dist);
    }
    else
    {
        // to at i_offset distance from target and i_angle from target facing
        i_target->GetClosePoint(x, y, z, owner.GetObjectSize(), i_offset, i_angle);
    }

    if (!i_path)
        i_path = new PathFinderMovementGenerator(&owner);

    // allow pets following their master to cheat while generating paths
    bool forceDest = (owner.GetTypeId() == TYPEID_UNIT && ((Creature*)&owner)->isPet()
                        && owner.HasUnitState(UNIT_STATE_FOLLOW));
    i_path->calculate(x, y, z, forceDest);
    if (i_path->getPathType() & PATHFIND_NOPATH)
    {
        if (!forceDest || !i_target->IsWithinLOSInMap(&owner))
            return;
    }

    //Hack for transport walkline
    if (i_target.getTarget()->GetTransGUID())
        z = i_target.getTarget()->GetPositionZ();

    D::_addUnitStateMove(owner);
    i_targetReached = false;
    i_recalculateTravel = false;

    Movement::MoveSplineInit init(owner);
    if (i_path->getPathType() & PATHFIND_NOPATH || i_target.getTarget()->GetTransGUID())
        init.MoveTo(x,y,z);
    else
        init.MovebyPath(i_path->getPath());
    init.SetWalk(((D*)this)->EnableWalking());
    init.Launch();
}

template<>
void TargetedMovementGeneratorMedium<Player,ChaseMovementGenerator<Player> >::UpdateFinalDistance(float /*fDistance*/)
{
    // nothing to do for Player
}

template<>
void TargetedMovementGeneratorMedium<Player,FollowMovementGenerator<Player> >::UpdateFinalDistance(float /*fDistance*/)
{
    // nothing to do for Player
}

template<>
void TargetedMovementGeneratorMedium<Creature,ChaseMovementGenerator<Creature> >::UpdateFinalDistance(float fDistance)
{
    i_offset = fDistance;
    i_recalculateTravel = true;
}

template<>
void TargetedMovementGeneratorMedium<Creature,FetchMovementGenerator<Creature> >::UpdateFinalDistance(float fDistance)
{
    i_offset = fDistance;
    i_recalculateTravel = true;
}

template<>
void TargetedMovementGeneratorMedium<Creature,FollowMovementGenerator<Creature> >::UpdateFinalDistance(float fDistance)
{
    i_offset = fDistance;
    i_recalculateTravel = true;
}

template<class T, typename D>
bool TargetedMovementGeneratorMedium<T,D>::Update(T &owner, const uint32 & time_diff)
{
    if (!i_target.isValid() || !i_target->IsInWorld())
    {
        if (i_targetSearchingTimer >= TARGET_NOT_ACCESSIBLE_MAX_TIMER)
            return false;
        else
        {
            i_targetSearchingTimer += time_diff;
            return true;
        }
    }

    if (!owner.isAlive())
        return false;

    if (owner.HasUnitState(UNIT_STATE_NOT_MOVE))
    {
        D::_clearUnitStateMove(owner);
        return true;
    }

    // prevent movement while casting spells with cast time or channel time
    if (owner.HasUnitState(UNIT_STATE_CASTING))
    {
        if (!owner.IsStopped())
        {
            // some spells should be able to be cast while moving
            // maybe some attribute? here, check the entry of creatures useing these spells
            switch(owner.GetEntry())
            {
                case 36633: // Ice Sphere (Lich King)
                case 37562: // Volatile Ooze and Gas Cloud (Putricide)
                case 37697:
                    break;
                default:
                    owner.StopMoving();
            }
        }
        return true;
    }

    // prevent crash after creature killed pet
    if (static_cast<D*>(this)->_lostTarget(owner))
    {
        D::_clearUnitStateMove(owner);
        if (i_targetSearchingTimer >= TARGET_NOT_ACCESSIBLE_MAX_TIMER)
            return false;
        else
        {
            i_targetSearchingTimer += time_diff;
            return true;
        }
    }

    i_recheckDistance.Update(time_diff);
    if (i_recheckDistance.Passed())
    {
        i_recheckDistance.Reset(RECHECK_DISTANCE_TIMER);

        G3D::Vector3 dest = owner.movespline->FinalDestination();
        float allowed_dist = 0.0f;
        bool targetIsVictim = owner.getVictim() && owner.getVictim()->GetGUID() == i_target->GetGUID();
        if (targetIsVictim)
            allowed_dist = owner.GetMeleeReach() + owner.getVictim()->GetMeleeReach()/* + owner.getVictim()->GetObjectSize()*/;
        else
            allowed_dist = i_target->GetObjectSize() + owner.GetObjectSize() + sWorld->getRate(RATE_TARGET_POS_RECALCULATION_RANGE);

        if (allowed_dist < owner.GetObjectSize())
            allowed_dist = owner.GetObjectSize();

        bool targetMoved = false;
        if (owner.GetTypeId() == TYPEID_UNIT && ((Creature*)&owner)->IsFlying())
            targetMoved = !i_target->IsWithinDist3d(dest.x, dest.y, dest.z, allowed_dist);
        else
            targetMoved = !i_target->IsWithinDist2d(dest.x, dest.y, allowed_dist);

        if (targetIsVictim && owner.GetTypeId() == TYPEID_UNIT && !((Creature*)&owner)->isPet())
        {
            if ((!owner.getVictim() || !owner.getVictim()->isAlive()) && owner.movespline->Finalized())
                return false;

            if (!i_offset && owner.movespline->Finalized() && !owner.IsWithinMeleeRange(owner.getVictim())
                && !i_target->m_movementInfo.HasMovementFlag(MOVEMENTFLAG_PENDING_STOP))
            {
                if (i_targetSearchingTimer >= TARGET_NOT_ACCESSIBLE_MAX_TIMER)
                {
                    owner.DeleteFromThreatList(owner.getVictim());
                    return false;
                }
                else
                {
                    i_targetSearchingTimer += time_diff;
                    targetMoved = true;
                }
            }
            else
                i_targetSearchingTimer = 0;
        }
        else
            i_targetSearchingTimer = 0;

        if (targetMoved)
            _setTargetLocation(owner);
    }

    if (owner.movespline->Finalized())
    {
        static_cast<D*>(this)->MovementInform(owner);
        if (i_angle == 0.f && !owner.HasInArc(0.01f, i_target.getTarget()))
            owner.SetInFront(i_target.getTarget());

        if (!i_targetReached)
        {
            i_targetReached = true;
            static_cast<D*>(this)->_reachTarget(owner);
        }
    }
    else
    {
        if (i_recalculateTravel)
            _setTargetLocation(owner);
    }
    return true;
}

//-----------------------------------------------//
template<class T>
void ChaseMovementGenerator<T>::_reachTarget(T &owner)
{
    if (owner.IsWithinMeleeRange(this->i_target.getTarget()))
        owner.Attack(this->i_target.getTarget(), true);
}

template<>
void ChaseMovementGenerator<Player>::Initialize(Player &owner)
{
    owner.AddUnitState(UNIT_STATE_CHASE | UNIT_STATE_CHASE_MOVE);
    _setTargetLocation(owner);
}

template<>
void ChaseMovementGenerator<Creature>::Initialize(Creature &owner)
{
    owner.SetWalk(false);
    owner.AddUnitState(UNIT_STATE_CHASE | UNIT_STATE_CHASE_MOVE);
    _setTargetLocation(owner);
}

template<class T>
void ChaseMovementGenerator<T>::Finalize(T &owner)
{
    owner.ClearUnitState(UNIT_STATE_CHASE | UNIT_STATE_CHASE_MOVE);
    if (owner.GetTypeId() == TYPEID_UNIT && !((Creature*)&owner)->isPet() && owner.isAlive())
    {
        if (!owner.isInCombat() || ( this->i_target.getTarget() && !this->i_target.getTarget()->isInAccessiblePlaceFor(((Creature*)&owner))))
        {
            if (owner.isInCombat())
                owner.CombatStop(true);
            owner.GetMotionMaster()->MoveTargetedHome();
        }
    }
}

template<class T>
void ChaseMovementGenerator<T>::Reset(T &owner)
{
    Initialize(owner);
}

template<class T>
void ChaseMovementGenerator<T>::MovementInform(T & /*unit*/)
{
}

template<>
void ChaseMovementGenerator<Creature>::MovementInform(Creature &unit)
{
    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
    if (unit.AI())
        unit.AI()->MovementInform(CHASE_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
}

//-----------------------------------------------//
template<class T>
void FetchMovementGenerator<T>::_reachTarget(T &owner)
{
    //sLog->outDebug(LOG_FILTER_NETWORKIO, "FetchMovementGenerator _reachTarget i_target %u, owner %u", this->i_target.getTarget()->GetGUIDLow(), owner.GetGUIDLow());
}

template<>
void FetchMovementGenerator<Creature>::Initialize(Creature &owner)
{
    owner.SetWalk(false);
    owner.AddUnitState(UNIT_STATE_CHASE|UNIT_STATE_CHASE_MOVE);
    _setTargetLocation(owner);
    //sLog->outDebug(LOG_FILTER_NETWORKIO, "FetchMovementGenerator Initialize i_target %u, owner %u", i_target.getTarget()->GetGUIDLow(), owner.GetGUIDLow());
}

template<class T>
void FetchMovementGenerator<T>::Finalize(T &owner)
{
    //sLog->outDebug(LOG_FILTER_NETWORKIO, "FetchMovementGenerator Finalize i_target %u, owner %u", this->i_target.getTarget()->GetGUIDLow(), owner.GetGUIDLow());
    owner.ClearUnitState(UNIT_STATE_CHASE|UNIT_STATE_CHASE_MOVE);
}

template<class T>
void FetchMovementGenerator<T>::Reset(T &owner)
{
    //sLog->outDebug(LOG_FILTER_NETWORKIO, "FetchMovementGenerator Reset i_target %u, owner %u", this->i_target.getTarget()->GetGUIDLow(), owner.GetGUIDLow());
    Initialize(owner);
}

template<class T>
void FetchMovementGenerator<T>::MovementInform(T & /*unit*/)
{
}

template<>
void FetchMovementGenerator<Creature>::MovementInform(Creature &unit)
{
    if(Unit* _petowner = unit.GetOwner())
    {
        if (Player* _player = _petowner->ToPlayer())
        {
            if(Pet* pet = _player->GetPet())
            {
                _player->GetSession()->LootCorps(i_target.getTarget()->GetGUID(), pet);
                pet->CastSpell(i_target.getTarget(), 125056, false);

                pet->AttackStop();
                pet->InterruptNonMeleeSpells(false);
                pet->GetMotionMaster()->Clear(false);
                //pet->GetMotionMaster()->MoveFollow(_petowner, PET_FOLLOW_DIST, pet->GetFollowAngle());
                //if (CharmInfo* charmInfo = pet->GetCharmInfo())
                //{
                //    charmInfo->SetCommandState(COMMAND_FOLLOW);
                //    charmInfo->SetIsCommandAttack(false);
                //    charmInfo->SetIsAtStay(false);
                //    charmInfo->SetIsReturning(true);
                //    charmInfo->SetIsFollowing(false);
                //}
            }
        }
    }
    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
    if (unit.AI())
        unit.AI()->MovementInform(CHASE_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
}

//-----------------------------------------------//
template<>
bool FollowMovementGenerator<Creature>::EnableWalking() const
{
    return i_target.isValid() && i_target->IsWalking();
}

template<>
bool FollowMovementGenerator<Player>::EnableWalking() const
{
    return false;
}

template<>
void FollowMovementGenerator<Player>::_updateSpeed(Player &/*u*/)
{
    // nothing to do for Player
}

template<>
void FollowMovementGenerator<Creature>::_updateSpeed(Creature &u)
{
    // pet only sync speed with owner
    if (!((Creature&)u).isPet() || !i_target.isValid() || i_target->GetGUID() != u.GetOwnerGUID())
        return;

    u.UpdateSpeed(MOVE_RUN,true);
    u.UpdateSpeed(MOVE_WALK,true);
    u.UpdateSpeed(MOVE_SWIM,true);
}

template<>
void FollowMovementGenerator<Player>::Initialize(Player &owner)
{
    owner.AddUnitState(UNIT_STATE_FOLLOW|UNIT_STATE_FOLLOW_MOVE);
    _updateSpeed(owner);
    _setTargetLocation(owner);
}

template<>
void FollowMovementGenerator<Creature>::Initialize(Creature &owner)
{
    owner.AddUnitState(UNIT_STATE_FOLLOW|UNIT_STATE_FOLLOW_MOVE);
    _updateSpeed(owner);
    _setTargetLocation(owner);
}

template<class T>
void FollowMovementGenerator<T>::Finalize(T &owner)
{
    owner.ClearUnitState(UNIT_STATE_FOLLOW|UNIT_STATE_FOLLOW_MOVE);
    _updateSpeed(owner);
}

template<class T>
void FollowMovementGenerator<T>::Reset(T &owner)
{
    Initialize(owner);
}

template<class T>
void FollowMovementGenerator<T>::MovementInform(T & /*unit*/)
{
}

template<>
void FollowMovementGenerator<Creature>::MovementInform(Creature &unit)
{
    // Pass back the GUIDLow of the target. If it is pet's owner then PetAI will handle
    if (unit.AI())
        unit.AI()->MovementInform(FOLLOW_MOTION_TYPE, i_target.getTarget()->GetGUIDLow());
}

//-----------------------------------------------//
template void TargetedMovementGeneratorMedium<Player,ChaseMovementGenerator<Player> >::_setTargetLocation(Player &);
template void TargetedMovementGeneratorMedium<Player,FollowMovementGenerator<Player> >::_setTargetLocation(Player &);
template void TargetedMovementGeneratorMedium<Creature,ChaseMovementGenerator<Creature> >::_setTargetLocation(Creature &);
template void TargetedMovementGeneratorMedium<Creature,FollowMovementGenerator<Creature> >::_setTargetLocation(Creature &);
template void TargetedMovementGeneratorMedium<Creature,FetchMovementGenerator<Creature> >::_setTargetLocation(Creature &);
template bool TargetedMovementGeneratorMedium<Player,ChaseMovementGenerator<Player> >::Update(Player &, const uint32 &);
template bool TargetedMovementGeneratorMedium<Player,FollowMovementGenerator<Player> >::Update(Player &, const uint32 &);
template bool TargetedMovementGeneratorMedium<Creature,ChaseMovementGenerator<Creature> >::Update(Creature &, const uint32 &);
template bool TargetedMovementGeneratorMedium<Creature,FollowMovementGenerator<Creature> >::Update(Creature &, const uint32 &);
template bool TargetedMovementGeneratorMedium<Creature,FetchMovementGenerator<Creature> >::Update(Creature &, const uint32 &);

template void ChaseMovementGenerator<Player>::_reachTarget(Player &);
template void ChaseMovementGenerator<Creature>::_reachTarget(Creature &);
template void ChaseMovementGenerator<Player>::Finalize(Player &);
template void ChaseMovementGenerator<Creature>::Finalize(Creature &);
template void ChaseMovementGenerator<Player>::Reset(Player &);
template void ChaseMovementGenerator<Creature>::Reset(Creature &);
template void ChaseMovementGenerator<Player>::MovementInform(Player &unit);

template void FetchMovementGenerator<Creature>::_reachTarget(Creature &);
template void FetchMovementGenerator<Creature>::Finalize(Creature &);
template void FetchMovementGenerator<Creature>::Reset(Creature &);

template void FollowMovementGenerator<Player>::Finalize(Player &);
template void FollowMovementGenerator<Creature>::Finalize(Creature &);
template void FollowMovementGenerator<Player>::Reset(Player &);
template void FollowMovementGenerator<Creature>::Reset(Creature &);
template void FollowMovementGenerator<Player>::MovementInform(Player &unit);
