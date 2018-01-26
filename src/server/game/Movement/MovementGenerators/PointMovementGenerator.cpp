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

#include "PointMovementGenerator.h"
#include "Errors.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "World.h"
#include "MoveSplineInit.h"
#include "MoveSpline.h"
#include "Player.h"

//----- Point Movement Generator
template<class T>
void PointMovementGenerator<T>::DoInitialize(T &unit)
{
    if (!unit.IsStopped())
        unit.StopMoving();

    unit.AddUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
    i_recalculateSpeed = false;
    Movement::MoveSplineInit init(unit);
    init.MoveTo(i_x, i_y, i_z, m_generatePath);
    if (speed > 0.0f)
        init.SetVelocity(speed);
    init.Launch();
}

template<class T>
bool PointMovementGenerator<T>::DoUpdate(T &unit, const uint32 & /*diff*/)
{
    if (!&unit)
        return false;

    if (unit.HasUnitState(UNIT_STATE_ROOT | UNIT_STATE_STUNNED))
    {
        unit.ClearUnitState(UNIT_STATE_ROAMING_MOVE);
        return true;
    }

    unit.AddUnitState(UNIT_STATE_ROAMING_MOVE);

    if (i_recalculateSpeed && !unit.movespline->Finalized())
    {
        i_recalculateSpeed = false;
        Movement::MoveSplineInit init(unit);
        init.MoveTo(i_x, i_y, i_z, m_generatePath);
        if (speed > 0.0f) // Default value for point motion type is 0.0, if 0.0 spline will use GetSpeed on unit
            init.SetVelocity(speed);
        init.Launch();
    }

    return !unit.movespline->Finalized();
}

template<class T>
void PointMovementGenerator<T>::DoFinalize(T &unit)
{
    unit.ClearUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);

    if (unit.movespline->Finalized())
        MovementInform(unit);
    if (unit.GetTypeId() == TYPEID_PLAYER && unit.IsInWorld())
        unit.UpdatePosition(i_x, i_y, i_z, unit.GetOrientation(), false);
}

template<class T>
void PointMovementGenerator<T>::DoReset(T &unit)
{
    if (!unit.IsStopped())
        unit.StopMoving();

    unit.AddUnitState(UNIT_STATE_ROAMING|UNIT_STATE_ROAMING_MOVE);
}

template<class T>
void PointMovementGenerator<T>::MovementInform(T & /*unit*/)
{
}

template <> void PointMovementGenerator<Creature>::MovementInform(Creature &unit)
{
    if (unit.AI())
        unit.AI()->MovementInform(POINT_MOTION_TYPE, id);
}

template void PointMovementGenerator<Player>::DoInitialize(Player&);
template void PointMovementGenerator<Creature>::DoInitialize(Creature&);
template void PointMovementGenerator<Player>::DoFinalize(Player&);
template void PointMovementGenerator<Creature>::DoFinalize(Creature&);
template void PointMovementGenerator<Player>::DoReset(Player&);
template void PointMovementGenerator<Creature>::DoReset(Creature&);
template bool PointMovementGenerator<Player>::DoUpdate(Player &, const uint32 &);
template bool PointMovementGenerator<Creature>::DoUpdate(Creature&, const uint32 &);

void AssistanceMovementGenerator::DoFinalize(Unit &unit)
{
    unit.ToCreature()->SetNoCallAssistance(false);
    unit.ToCreature()->CallAssistance();
    if (unit.isAlive())
        unit.GetMotionMaster()->MoveSeekAssistanceDistract(sWorld->getIntConfig(CONFIG_CREATURE_FAMILY_ASSISTANCE_DELAY));
}

bool EffectMovementGenerator::Update(Unit &unit, const uint32&)
{
    return !unit.movespline->Finalized();
}

void EffectMovementGenerator::Finalize(Unit &unit)
{
    if (unit.GetTypeId() != TYPEID_UNIT && unit.GetTypeId() != TYPEID_PLAYER)
        return;

    unit.ClearUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);

    if (((Creature&)unit).AI())
        ((Creature&)unit).AI()->MovementInform(EFFECT_MOTION_TYPE, m_Id);

    if (unit.GetTypeId() == TYPEID_PLAYER && unit.IsInWorld())
    {
        unit.UpdatePosition(i_x, i_y, i_z, unit.GetOrientation(), false);
        unit.UnitMovementInform(m_Id);
    }

    // Effect event. Used for delay cast after jump for example
    if (m_event)
    {
        m_event->Execute(&unit);
        delete m_event;
        m_event = NULL;
    }
}

//----- Charge Movement Generator
void ChargeMovementGenerator::_setTargetLocation(Unit &unit)
{
    //TC_LOG_DEBUG("spell", "ChargeMovementGenerator GetDestination (%f %f %f)", i_x, i_y, i_z);

    if(!i_path)
        i_path = new PathGenerator(&unit);

    i_path->CalculatePath(i_x, i_y, i_z, false);

    if (i_path->GetPathType() & PATHFIND_NOPATH)
        return;

    if (i_path->GetTotalLength() > 40)
        return;

    i_targetReached = false;
    i_recalculateTravel = false;

    Movement::MoveSplineInit init(unit);
    if (i_path->GetPathType() & PATHFIND_NOPATH || unit.GetTransGUID())
        init.MoveTo(i_x, i_y, i_z);
    else
        init.MovebyPath(i_path->GetPath());
    if (speed > 0.0f)
        init.SetVelocity(speed);
    init.Launch();
}

bool ChargeMovementGenerator::Update(Unit &unit, const uint32&)
{
    return !unit.movespline->Finalized();
}

void ChargeMovementGenerator::Initialize(Unit &unit)
{
    //TC_LOG_DEBUG("spell", "ChargeMovementGenerator Initialize (%f %f %f)", i_x, i_y, i_z);

    if (!unit.IsStopped())
        unit.StopMoving();

    unit.AddUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);
    _setTargetLocation(unit);
}

void ChargeMovementGenerator::Finalize(Unit &unit)
{
    if (unit.GetTypeId() != TYPEID_UNIT && unit.GetTypeId() != TYPEID_PLAYER)
        return;

    //TC_LOG_DEBUG("spell", "ChargeMovementGenerator Finalize (%f %f %f) spellId %i", i_x, i_y, i_z, triggerspellId);

    unit.ClearUnitState(UNIT_STATE_ROAMING | UNIT_STATE_ROAMING_MOVE);

    if (((Creature&)unit).AI())
        ((Creature&)unit).AI()->MovementInform(POINT_MOTION_TYPE, m_Id);
    if (unit.GetTypeId() == TYPEID_PLAYER && unit.IsInWorld())
        unit.UpdatePosition(i_x, i_y, i_z, unit.GetOrientation(), false);

    if(triggerspellId)
        unit.CastSpell(i_x, i_y, i_z, triggerspellId, true);
}