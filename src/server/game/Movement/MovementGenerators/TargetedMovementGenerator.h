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

#ifndef TRINITY_TARGETEDMOVEMENTGENERATOR_H
#define TRINITY_TARGETEDMOVEMENTGENERATOR_H

#include "MovementGenerator.h"
#include "FollowerReference.h"
#include "Timer.h"
#include "Unit.h"

class ClassFactory;

class TargetedMovementGeneratorBase
{
    friend class ClassFactory;
    protected:
        TargetedMovementGeneratorBase(UnitPtr &target) : i_target(std::shared_ptr<FollowerReference>(new FollowerReference())) {} //gen->i_target.link(target, gen);
    public:
        void stopFollowing() { }
    protected:
        std::shared_ptr<FollowerReference> i_target;
};

template<class T, typename D>
class TargetedMovementGeneratorMedium : public MovementGeneratorMedium< T, D >, public TargetedMovementGeneratorBase
{
    protected:
        TargetedMovementGeneratorMedium(UnitPtr &target, float offset, float angle) :
            TargetedMovementGeneratorBase(target), i_recheckDistance(0),
            i_offset(offset), i_angle(angle),
            i_recalculateTravel(false), i_targetReached(false)
        {
        }
        ~TargetedMovementGeneratorMedium() {}

    public:
        bool Update(std::shared_ptr<T> &, const uint32 &);
        UnitPtr GetTarget() const { return i_target->getTarget(); }

        void unitSpeedChanged() { i_recalculateTravel=true; }
        void UpdateFinalDistance(float fDistance);

    protected:
        void _setTargetLocation(std::shared_ptr<T> &);

        TimeTrackerSmall i_recheckDistance;
        float i_offset;
        float i_angle;
        bool i_recalculateTravel : 1;
        bool i_targetReached : 1;
};

template<class T>
class ChaseMovementGenerator : public TargetedMovementGeneratorMedium<T, ChaseMovementGenerator<T> >
{
    friend class ClassFactory;
    protected:
        ChaseMovementGenerator(UnitPtr &target)
            : TargetedMovementGeneratorMedium<T, ChaseMovementGenerator<T> >(target) {}
        ChaseMovementGenerator(UnitPtr &target, float offset, float angle)
            : TargetedMovementGeneratorMedium<T, ChaseMovementGenerator<T> >(target, offset, angle) {}
    public:
        ~ChaseMovementGenerator() {}

        MovementGeneratorType GetMovementGeneratorType() { return CHASE_MOTION_TYPE; }

        void Initialize(std::shared_ptr<T> &);
        void Finalize(std::shared_ptr<T> &);
        void Reset(std::shared_ptr<T> &);
        void MovementInform(std::shared_ptr<T> &);

        static void _clearUnitStateMove(std::shared_ptr<T> &u) { u->ClearUnitState(UNIT_STATE_CHASE_MOVE); }
        static void _addUnitStateMove(std::shared_ptr<T> &u)  { u->AddUnitState(UNIT_STATE_CHASE_MOVE); }
        bool EnableWalking() const { return false;}
        bool _lostTarget(std::shared_ptr<T> &u) const { return u->getVictim() != this->GetTarget(); }
        void _reachTarget(std::shared_ptr<T> &);
};

template<class T>
class FollowMovementGenerator : public TargetedMovementGeneratorMedium<T, FollowMovementGenerator<T> >
{
    friend class ClassFactory;
    private:
        FollowMovementGenerator(UnitPtr &target)
            : TargetedMovementGeneratorMedium<T, FollowMovementGenerator<T> >(target){}
        FollowMovementGenerator(UnitPtr &target, float offset, float angle)
            : TargetedMovementGeneratorMedium<T, FollowMovementGenerator<T> >(target, offset, angle) {}
    public:
        ~FollowMovementGenerator() {}

        MovementGeneratorType GetMovementGeneratorType() { return FOLLOW_MOTION_TYPE; }

        void Initialize(std::shared_ptr<T> &);
        void Finalize(std::shared_ptr<T> &);
        void Reset(std::shared_ptr<T> &);
        void MovementInform(std::shared_ptr<T> &);

        static void _clearUnitStateMove(std::shared_ptr<T> &u) { u->ClearUnitState(UNIT_STATE_FOLLOW_MOVE); }
        static void _addUnitStateMove(std::shared_ptr<T> &u)  { u->AddUnitState(UNIT_STATE_FOLLOW_MOVE); }
        bool EnableWalking() const;
        bool _lostTarget(std::shared_ptr<T> &) const { return false; }
        void _reachTarget(std::shared_ptr<T> &) {}
    private:
        void _updateSpeed(std::shared_ptr<T> &u);
};

#endif

